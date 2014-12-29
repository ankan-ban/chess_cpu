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
int   Game::pv[MAX_GAME_LENGTH];

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


// negamax forumlation of alpha-beta search
float Game::alphabeta(HexaBitBoardPosition *pos, int depth, int curPly, float alpha, float beta)
{
    if (depth == 0)
    {
        // TODO: Quiescence Search
        nodes++;
        return BitBoardUtils::Evaluate(pos);
    }

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

    // generate child nodes
    HexaBitBoardPosition newPositions[MAX_MOVES];
    int nMoves = BitBoardUtils::GenerateBoards(pos, newPositions);

    // special case: Check if it's checkmate or stalemate
    if (nMoves == 0)
    {
        if (BitBoardUtils::IsInCheck(pos))
        {
            return - ((float) INF/MAX_GAME_LENGTH) * depth;
        }
        else
        {
            return 0;
        }
    }

    float bestScore = -INF;
    int bestChild = 0;
    
    for (int i = 0; i < nMoves; i++)
    {
        float curScore = -alphabeta(&newPositions[i], depth - 1, curPly + 1, -beta, -alpha);
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

    // this is wrong.. TODO: Ankan - fis this.
    pv[depth] = bestChild;

    return alpha;
}


void Game::StartSearch()
{
    searching = true;
    nodes = 0;

    Timer timer;
    timer.start();

    for (int depth = 1; depth < maxSearchDepth; depth++)
    {
        float eval = alphabeta(&pos, depth, plyNo + 1, -INF, INF);

        CMove allMoves[MAX_MOVES];
        BitBoardUtils::GenerateMoves(&pos, allMoves);
        CMove currentDepthBest = allMoves[pv[depth]];

        // TODO: update bestmove inside a critical section?
        bestMove = currentDepthBest;

        uint64 timeElapsed = timer.stop();
        uint64 nps = nodes * 1000;
        if (timeElapsed)
        {
            nps /= timeElapsed;    // time is in ms
        }
        printf("info depth %d score cp %d nodes %llu nps %llu currmove ", depth, (int) eval, nodes, nps);
        Utils::displayCompactMove(bestMove);
        fflush(stdout);

        // TODO: better time management
        if (timeElapsed > (searchTime / 2.0f))
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

    for (uint32 i = 0; i < nMoves; i++)
    {
        uint64 childPerft = perft(&newPositions[i], depth - 1);
        count += childPerft;
    }

    return count;
}


uint64 Game::Perft(int depth)
{
    return perft(&pos, depth);
}