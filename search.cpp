#include "chess.h"

// Quiescence search
template<uint8 chance>
float Game::q_search(HexaBitBoardPosition *pos, int depth, float alpha, float beta)
{
    // check current position
    float stand_pat = BitBoardUtils::Evaluate(pos);
    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;


    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);
    bool inCheck = !!(bb.threatened & bb.myKing);

    CMove newMoves[MAX_MOVES];
    int nMoves;

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, newMoves);

        if (nMoves == 0) // mate!
            return -MATE_SCORE_BASE*(MAX_GAME_LENGTH + depth);
    }
    else
    {
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, newMoves);                // generate captures in MVV-LVA order

        if (nMoves == 0) // no more captures
            return alpha;
    }

    for (int i = 0; i < nMoves; i++)
    {
        HexaBitBoardPosition newPos = *pos;
        uint64 hash = 0;

        // TODO: prune out bad captures (check out SEE and delta pruning!)

        BitBoardUtils::MakeMove(&newPos, hash, newMoves[i]);

        float curScore = -q_search<!chance>(&newPos, depth - 1, -beta, -alpha);
        if (curScore >= beta)
        {
            return beta;
        }

        if (curScore > alpha)
        {
            alpha = curScore;
        }
    }

    return alpha;
}


// negamax forumlation of alpha-beta search
template<uint8 chance>
float Game::alphabeta(HexaBitBoardPosition *pos, int depth, int curPly, float alpha, float beta)
{
    if (depth == 0)
    {
        // TODO: Quiescence Search
        nodes++;
        //return BitBoardUtils::Evaluate(pos);
        return q_search<chance>(pos, depth, alpha, beta);
    }

    // detect draw by repetition
    uint64 posHash = BitBoardUtils::ComputeZobristKey(pos);
    posHashes[curPly] = posHash;

    if (curPly != plyNo + 1)    // avoid evaluating draws at root
    {
        for (int i = 0; i < curPly; i++)
        {
            if (posHashes[i] == posHash)
            {
                return 0;   // draw by repetition
            }
        }
    }


    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);

    int bestChild = 0;
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
            HexaBitBoardPosition newPos = *pos;
            uint64 hash = 0;
            BitBoardUtils::MakeMove(&newPos, hash, newMoves[i]);

            float curScore = -alphabeta<!chance>(&newPos, depth - 1, curPly + 1, -beta, -alpha);
            if (curScore >= beta)
            {
                return beta;
            }

            if (curScore > alpha)
            {
                alpha = curScore;
                bestChild = i;
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
            return -MATE_SCORE_BASE * (MAX_GAME_LENGTH + depth);
        }
        else
        {
            return 0;
        }
    }


    for (int i = searched; i < nMoves; i++)
    {
        HexaBitBoardPosition newPos = *pos;
        uint64 hash = 0;
        BitBoardUtils::MakeMove(&newPos, hash, newMoves[i]);

        float curScore = -alphabeta<!chance>(&newPos, depth - 1, curPly + 1, -beta, -alpha);
        if (curScore >= beta)
        {
            return beta;
        }

        if (curScore > alpha)
        {
            alpha = curScore;
            bestChild = i;
        }
    }

    // this is wrong.. TODO: Ankan - fix this.
    pv[depth] = newMoves[bestChild];

    return alpha;
}


template float Game::alphabeta<WHITE>(HexaBitBoardPosition *pos, int depth, int curPly, float alpha, float beta);
template float Game::alphabeta<BLACK>(HexaBitBoardPosition *pos, int depth, int curPly, float alpha, float beta);

template float Game::q_search<WHITE>(HexaBitBoardPosition *pos, int depth, float alpha, float beta);
template float Game::q_search<BLACK>(HexaBitBoardPosition *pos, int depth, float alpha, float beta);