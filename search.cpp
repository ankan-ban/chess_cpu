#include "chess.h"

// Quiescence search
template<uint8 chance>
int16 Game::q_search(HexaBitBoardPosition *pos, int depth, int16 alpha, int16 beta)
{
    int16 currentMax = -INF;

    // check current position
    int16 stand_pat = BitBoardUtils::Evaluate(pos);
    
    if (stand_pat >= beta)
        return stand_pat;

    currentMax = stand_pat;
    if (currentMax > alpha)
        alpha = currentMax;


    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);
    bool inCheck = !!(bb.threatened & bb.myKing);

    CMove newMoves[MAX_MOVES];
    int nMoves;

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, newMoves);

        if (nMoves == 0) // mate!
            return -(MATE_SCORE_BASE + depth);
    }
    else
    {
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, newMoves);                // generate captures in MVV-LVA order

        if (nMoves == 0) // no more captures
            return currentMax;
    }

    for (int i = 0; i < nMoves; i++)
    {
        HexaBitBoardPosition newPos = *pos;
        uint64 hash = 0;

        // TODO: prune out bad captures (check out SEE and delta pruning!)

        BitBoardUtils::MakeMove(&newPos, hash, newMoves[i]);

        int16 curScore = -q_search<!chance>(&newPos, depth - 1, -beta, -alpha);
        if (curScore >= beta)
        {
            return curScore;
        }

        if (curScore > currentMax)
        {
            currentMax = curScore;
            if (currentMax > alpha)
                alpha = currentMax;
        }
    }

    return currentMax;
}


// negamax forumlation of alpha-beta search
template<uint8 chance>
int16 Game::alphabeta(HexaBitBoardPosition *pos, uint64 hash, int depth, int curPly, int16 alpha, int16 beta, bool allowNullMove)
{
    if (depth <= 0)
    {
        nodes++;
        //return BitBoardUtils::Evaluate(pos);

        int16 qSearchVal = q_search<chance>(pos, depth, alpha, beta);
        return qSearchVal;
    }

    // detect draw by repetition
    posHashes[curPly] = hash;
    for (int i = 0; i < curPly; i++)
    {
        if (posHashes[i] == hash)
        {
            // don't update TT for draw by repetition
            return 0;   // draw by repetition
        }
    }


    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);
    bool inCheck = !!(bb.threatened & bb.myKing);

    // try null move
    
    // the reduction factor (no of plies we save by doing null move search first)
    int R = DEFAULT_NULL_MOVE_REDUCTION;

    // TODO: more checks to avoid dangerous positions
    if (allowNullMove &&                                                    // avoid doing null move twice in a row
        !inCheck &&                                                         // can't do null move when in check
        depth > R &&                                                        // can't do near horizon
        BitBoardUtils::countMoves<chance>(pos) >= MIN_MOVES_FOR_NULL_MOVE)  // avoid when there are very few valid moves
    {

        if (depth >= MIN_DEPTH_FOR_EXTRA_REDUCTION)
        {
            R = EXTRA_NULL_MOVE_REDUCTION;
        }

        // make null move
        pos->chance = !pos->chance;
        uint64 newHash = hash ^ BitBoardUtils::zob.chance;
        int16 nullMoveScore = -alphabeta<!chance>(pos, newHash, depth - 1 - R, curPly + 1, -beta, -beta + 1, false);

        // undo null move
        pos->chance = !pos->chance;

        if (nullMoveScore >= beta) return nullMoveScore;
    }
    


    // lookup in the transposition table
    TTEntry ttEntry;
    bool foundInTT = TranspositionTable::lookup(hash, &ttEntry, depth);
    if (foundInTT && ttEntry.depth >= depth)
    {
        // exact score at same or better depth => done, return TT value
        if (ttEntry.scoreType == SCORE_EXACT)
        {
            return ttEntry.score;
        }

        // score at same or better depth causes beta cutoff - again return TT value
        if (ttEntry.scoreType == SCORE_GE && ttEntry.score >= beta)
        {
            return ttEntry.score;
        }

        // score causes alpha-cutoff
        if (ttEntry.scoreType == SCORE_LE && ttEntry.score <= alpha)
        {
            return ttEntry.score;
        }
    }

    ttEntry.hashKey = hash;
    ttEntry.age = plyNo;
    ttEntry.depth = depth;


    int16 currentMax = -INF;

    bool improvedAlpha = false;

    // check hash move first
    CMove currentBestMove = {};
    CMove ttMove = {};
    if (foundInTT)
    {
        ttMove = ttEntry.bestMove;

        if (ttMove.isValid())
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = hash;
            BitBoardUtils::MakeMove(&newPos, newHash, ttMove);

            int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true);
            if (curScore >= beta)
            {
                ttEntry.scoreType = SCORE_GE; ttEntry.score = curScore;
                TranspositionTable::update(hash, &ttEntry);
                return curScore;
            }

            if (curScore > currentMax)
            {
                currentMax = curScore;
                if (currentMax > alpha)
                {
                    alpha = currentMax;
                    improvedAlpha = true;
                }

                currentBestMove = ttMove;
            }
        }

    }

    int searched = 0;


    // generate child nodes
    CMove newMoves[MAX_MOVES];
    int nMoves;

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, newMoves);
    }
    else
    {
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, newMoves);                // generate captures in MVV-LVA order

        // search captures first
        for (int i = 0; i < nMoves; i++)
        {
            if (newMoves[i] != ttMove)
            {
                HexaBitBoardPosition newPos = *pos;
                uint64 newHash = hash;
                BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

                int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true);
                if (curScore >= beta)
                {
                    ttEntry.scoreType = SCORE_GE; ttEntry.score = curScore; ttEntry.bestMove = newMoves[i];
                    TranspositionTable::update(hash, &ttEntry);

                    return curScore;
                }

                if (curScore > currentMax)
                {
                    currentMax = curScore;
                    if (currentMax > alpha)
                    {
                        alpha = currentMax;
                        improvedAlpha = true;
                    }

                    currentBestMove = newMoves[i];
                }
            }
        }
        searched = nMoves;

        nMoves += BitBoardUtils::generateNonCaptures<chance>(&bb, &newMoves[nMoves]);   // then rest of the moves
    }


    // special case: Check if it's checkmate or stalemate
    if (nMoves == 0)
    {
        if (inCheck)
        {
            ttEntry.scoreType = SCORE_EXACT; ttEntry.score = -(MATE_SCORE_BASE + depth);
            TranspositionTable::update(hash, &ttEntry);

            return -(MATE_SCORE_BASE + depth);
        }
        else
        {
            ttEntry.scoreType = SCORE_EXACT; ttEntry.score = 0;
            TranspositionTable::update(hash, &ttEntry);
            return 0;
        }
    }


    for (int i = searched; i < nMoves; i++)
    {
        if (newMoves[i] != ttMove)
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = hash;
            BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

            int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true);
            if (curScore >= beta)
            {
                ttEntry.scoreType = SCORE_GE; ttEntry.score = curScore; ttEntry.bestMove = newMoves[i];
                TranspositionTable::update(hash, &ttEntry);
                return curScore;
            }

            if (curScore > currentMax)
            {
                currentMax = curScore;
                if (currentMax > alpha)
                {
                    alpha = currentMax;
                    improvedAlpha = true;
                }

                currentBestMove = newMoves[i];
            }
        }
    }

    // default node type is ALL node and the score returned in a upper bound on the score of the node
    ttEntry.score = currentMax; 
    ttEntry.bestMove = currentBestMove;

    if (improvedAlpha)
    {
        ttEntry.scoreType = SCORE_EXACT; 
    }
    else
    {
        // ALL node
        ttEntry.scoreType = SCORE_LE;
    }
    TranspositionTable::update(hash, &ttEntry);

    return currentMax;
}

void Game::GetPVFromTT(HexaBitBoardPosition *pos)
{
    HexaBitBoardPosition nextPos = *pos;

    int depth = 0;
    while (depth < MAX_GAME_LENGTH)
    {
        uint64 posHash = BitBoardUtils::ComputeZobristKey(&nextPos);

        TTEntry ttEntry;
        bool foundInTT = TranspositionTable::lookup(posHash, &ttEntry, 0);
        CMove bestMove = ttEntry.bestMove;
        if (foundInTT && bestMove.isValid())
        {
            pv[depth++] = bestMove;
            BitBoardUtils::MakeMove(&nextPos, posHash, bestMove);
        }
        else
        {
            break;
        }
    }

    pvLen = depth;
}

// root of alpha-beta search
template<uint8 chance>
int16 Game::alphabetaRoot(HexaBitBoardPosition *pos, int depth, int curPly)
{
    int16 alpha = -INF, beta = INF;
    uint64 posHash = BitBoardUtils::ComputeZobristKey(pos);

    // used to detect draw by repetition
    posHashes[curPly] = posHash;

    // lookup in the transposition table
    TTEntry ttEntry;
    bool foundInTT = TranspositionTable::lookup(posHash, &ttEntry, depth);

    ttEntry.hashKey = posHash;
    ttEntry.age = plyNo;
    ttEntry.depth = depth;


    // check hash move first
    CMove currentBestMove = {};
    CMove ttMove = {};
    if (foundInTT)
    {
        ttMove = ttEntry.bestMove;

        if (ttMove.isValid())
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = posHash;
            BitBoardUtils::MakeMove(&newPos, newHash, ttMove);

            int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true);

            if (curScore > alpha)
            {
                alpha = curScore;
                currentBestMove = ttMove;
            }
        }

    }

    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);
    int searched = 0;
    bool inCheck = !!(bb.threatened & bb.myKing);

    // generate child nodes
    CMove newMoves[MAX_MOVES];
    int nMoves;

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, newMoves);
    }
    else
    {
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, newMoves);                // generate captures in MVV-LVA order

        // search captures first
        for (int i = 0; i < nMoves; i++)
        {
            if (newMoves[i] != ttMove)
            {
                HexaBitBoardPosition newPos = *pos;
                uint64 newHash = posHash;
                BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

                int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true);

                if (curScore > alpha)
                {
                    alpha = curScore;
                    currentBestMove = newMoves[i];
                }
            }
        }
        searched = nMoves;

        nMoves += BitBoardUtils::generateNonCaptures<chance>(&bb, &newMoves[nMoves]);   // then rest of the moves
    }


    for (int i = searched; i < nMoves; i++)
    {
        if (newMoves[i] != ttMove)
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = posHash;
            BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

            int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true);

            if (curScore > alpha)
            {
                alpha = curScore;
                currentBestMove = newMoves[i];
            }
        }
    }

    ttEntry.score = alpha;
    ttEntry.bestMove = currentBestMove;
    ttEntry.scoreType = SCORE_EXACT;

    TranspositionTable::update(posHash, &ttEntry);

    bestMove = currentBestMove;

    return alpha;
}



template int16 Game::alphabeta<WHITE>(HexaBitBoardPosition *pos, uint64 hash, int depth, int curPly, int16 alpha, int16 beta, bool tryNullMove);
template int16 Game::alphabeta<BLACK>(HexaBitBoardPosition *pos, uint64 hash, int depth, int curPly, int16 alpha, int16 beta, bool tryNullMove);


template int16 Game::alphabetaRoot<WHITE>(HexaBitBoardPosition *pos, int depth, int curPly);
template int16 Game::alphabetaRoot<BLACK>(HexaBitBoardPosition *pos, int depth, int curPly);

template int16 Game::q_search<WHITE>(HexaBitBoardPosition *pos, int depth, int16 alpha, int16 beta);
template int16 Game::q_search<BLACK>(HexaBitBoardPosition *pos, int depth, int16 alpha, int16 beta);




// transposition table related stuff
TTEntry* TranspositionTable::TT;         // the transposition table
uint64   TranspositionTable::size;       // in elements
int      TranspositionTable::indexBits;  // bits of hash key used for the index part in hash table (size-1)

void  TranspositionTable::init(int byteSize)
{
    size  = byteSize / sizeof(TTEntry);
    indexBits = size - 1;
    TT = (TTEntry *) malloc(byteSize);
    reset();
}

void  TranspositionTable::destroy()
{
    free (TT);
}

void  TranspositionTable::reset()
{
    memset(TT, 0, size * sizeof(TTEntry));
}

bool TranspositionTable::lookup(uint64 hash, TTEntry *entry, int depth)
{
    *entry = TT[hash & indexBits];
    if (entry->hashKey == hash)
    {
        if (abs(entry->score) >= MATE_SCORE_BASE/2)
        {
            if (entry->score < 0)
                entry->score = entry->score - depth;
            else
                entry->score = entry->score + depth;
        }

        return true;
    }
    else
    {
        return false;
    }
}

void TranspositionTable::update(uint64 hash, TTEntry *entry)
{
    // normally we return -(MATE_SCORE_BASE + depth);

    // hack: fix mate score. Always store mate as distance to mate from the current position
    if (abs(entry->score) >= MATE_SCORE_BASE/2)
    {
        if (entry->score < 0)
            entry->score = entry->score + entry->depth;
        else
            entry->score = entry->score - entry->depth;
    }

    // TODO: better replacement strategy
    TTEntry *oldentry = &TT[hash & indexBits];
    if (entry->age - oldentry->age > 32 ||
        entry->depth >= oldentry->depth)
    {
        TT[hash & indexBits] = *entry;
    }
}