#include "chess.h"

int main()
{
    printf("\nAnkan's chess engine\n");

    UciInterface::ProcessCommands();

    return 0;
}

// ----------------------------------------------- Game Class ---------------------------------------------------//

// static variable definations
HexaBitBoardPosition Game::pos;
int Game::searchTime;
int Game::maxSearchDepth;

CMove Game::bestMove;
CMove Game::pv[MAX_GAME_LENGTH];
int   Game::pvLen;

uint64 Game::nodes;

uint64 Game::posHashes[MAX_GAME_LENGTH];
int Game::plyNo;


volatile bool Game::searching;

void Game::Reset()
{
    // initialize variables, etc
    memset(&pos, 0, sizeof(pos));
    memset(&bestMove, 0, sizeof(bestMove));
    memset(posHashes, 0, sizeof(posHashes));
    searchTime = 0;
    maxSearchDepth = MAX_GAME_LENGTH;
    plyNo = 0;
}

void Game::SetTimeControls(int wtime, int btime, int movestogo, int winc, int binc, int searchTimeExact)
{
    int chance = pos.chance;

    if (searchTimeExact)
    {
        searchTime = searchTimeExact;
    }
    else
    {
        int x;
        int inc;
        if (chance == WHITE)
        {
            x = wtime;
            inc = winc;
        }
        else
        {
            x = btime;
            inc = binc;
        }

        if (movestogo == 0)
        {
            movestogo = 60;
        }
        searchTime = x / (movestogo) + inc;

    }
}

void Game::StartSearch()
{
    searching = true;
    nodes = 0;

    Timer timer;
    timer.start();

    for (int depth = 1; depth < maxSearchDepth; depth++)
    {

        int16 eval = 0;
        if (pos.chance == WHITE)
            eval = alphabetaRoot<WHITE>(&pos, depth, plyNo + 1);
        else
            eval = alphabetaRoot<BLACK>(&pos, depth, plyNo + 1);

        GetPVFromTT(&pos);

        uint64 timeElapsed = timer.stop();
        uint64 nps = nodes * 1000;
        if (timeElapsed)
        {
            nps /= timeElapsed;    // time is in ms
        }

        bool foundMate = false; // don't waste anymore time if we already found a mate

        // print mate score correctly
        if (abs(eval) >= MATE_SCORE_BASE/2)
        {
            foundMate = true;
            int16 mateDepth = abs(eval) - MATE_SCORE_BASE;

            // convert the depth to be relative to current position (distance from mate)
            mateDepth = (depth - mateDepth);

            if (eval < 0)
                mateDepth = -mateDepth;

            // mateDepth is in plies -> convert it into moves
            mateDepth /= 2;

            printf("info depth %d score mate %d nodes %llu time %llu nps %llu pv ", depth, mateDepth, nodes, timeElapsed, nps);
        }
        else
        {
            printf("info depth %d score cp %d nodes %llu time %llu nps %llu pv ", depth, (int)eval, nodes, timeElapsed, nps);
        }

        // display the PV (TODO: currently the PV is wrong after second move)
        for (int i = 0; i < pvLen; i++)
        {
            Utils::displayCompactMove(pv[i]);
        }
        printf("\n");
        fflush(stdout);

        // TODO: better time management
        if (foundMate || (timeElapsed > (searchTime / 2.0f)))
        {
            break;
        }
    }

    searching = false;
}


// super fast optimized perft routine
// one of the fastest w/o using hash tables
// on 4790k, start pos ~620 million nps, pos2 (of cpw): 1.08 billion nps
uint64 Game::perft(HexaBitBoardPosition *pos, int depth)
{
    HexaBitBoardPosition newPositions[MAX_MOVES];
    int nMoves = 0;

    if (depth == 1)
    {
        // optimization 1: bulk counting
        nMoves = BitBoardUtils::CountMoves(pos);
        return (uint64) nMoves;
    }

    // optimization 2: directly generate boards instead of first generating moves and then boards
    nMoves = BitBoardUtils::GenerateBoards(pos, newPositions);
    uint64 count = 0;

    for (int i = 0; i < nMoves; i++)
    {
        uint64 childPerft = perft(&newPositions[i], depth - 1);
        count += childPerft;
    }

    return count;
}


// tester perft routine
// check if ordered (MVV-LVA) move gen is working

// MVV-LVA move generation is 2.5X slower than regular move gen
// regular without bulk counting: start pos: 243 million nps, pos2: 367 million nps
// MVV-LVA without bulk counting: start pos:  96 million nps, pos2: 150 million nps
template<uint8 chance>
uint64 Game::perft_test(HexaBitBoardPosition *pos, int depth)
{
    CMove genMoves[MAX_MOVES];
    int nMoves = 0;
    /*
    if (depth == 1)
    {
        nMoves = BitBoardUtils::countMoves<chance>(pos);
        return (uint64)nMoves;
    }
    */

    if (BitBoardUtils::IsInCheck(pos))
    {
        uint64 allPawns = pos->pawns & RANKS2TO7;
        uint64 allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;
        uint64 blackPieces = allPieces & (~pos->whitePieces);

        uint64 myPieces = (chance == WHITE) ? pos->whitePieces : blackPieces;
        uint64 enemyPieces = (chance == WHITE) ? blackPieces : pos->whitePieces;

        uint64 enemyBishops = pos->bishopQueens & enemyPieces;
        uint64 enemyRooks = pos->rookQueens & enemyPieces;

        uint64 myKing = pos->kings & myPieces;
        uint8  kingIndex = BitBoardUtils::bitScan(myKing);

        uint64 pinned = BitBoardUtils::findPinnedPieces(pos->kings & myPieces, myPieces, enemyBishops, enemyRooks, allPieces, kingIndex);

        uint64 threatened = BitBoardUtils::findAttackedSquares(~allPieces, enemyBishops, enemyRooks, allPawns & enemyPieces,
                                                               pos->knights & enemyPieces, pos->kings & enemyPieces, myKing, !chance);

        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(pos, genMoves, allPawns, allPieces, myPieces, enemyPieces, pinned, threatened, kingIndex);
    }
    else
    {
        // TODO: see if refactoring out common code (e.g, find pinned, threatened, etc) can improve performance significantly?
        // have a structure - ExpandedBitboardPosition containing - all mypieces, enemypieces, mypawns, enemypawns, ... pinned, threatened, etc... and pass the structure around
        nMoves = BitBoardUtils::generateCaptures<chance>(pos, genMoves);
        nMoves += BitBoardUtils::generateNonCaptures<chance>(pos, &genMoves[nMoves]);
    }

    if (depth == 1)
        return (uint64) nMoves;

    uint64 count = 0;

    for (uint32 i = 0; i < nMoves; i++)
    {
        HexaBitBoardPosition newPos = *pos;
        uint64 hash = 0;
        BitBoardUtils::makeMove<chance>(&newPos, hash, genMoves[i]);

        uint64 childPerft = perft_test<!chance>(&newPos, depth - 1);
        count += childPerft;
    }

    return count;
}



uint64 Game::Perft(int depth)
{
    return perft(&pos, depth);
    /*
    if (pos.chance == WHITE)
        return perft_test<WHITE>(&pos, depth);
    else
        return perft_test<BLACK>(&pos, depth);
    */
}
