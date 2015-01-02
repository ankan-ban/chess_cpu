#include "chess.h"
#include "bb_consts.h"

// simple evaluation function taken from chess programming wiki
// References:
// https://chessprogramming.wikispaces.com/Evaluation
// https://chessprogramming.wikispaces.com/Simplified+evaluation+function
// Ankan - TODO: experiment with this and tune it further!

// good page on quiescent-search
// http://web.archive.org/web/20040427014440/brucemo.com/compchess/programming/quiescent.htm#MVVLVA


// all values in centi-pawns
#define QUEEN_MATERIAL_VAL  900
#define ROOK_MATERIAL_VAL   500
#define BISHOP_MATERIAL_VAL 330
#define KNIGHT_MATERIAL_VAL 320
#define PAWN_MATERIAL_VAL   100

// piece-square tables - from black's point of view and then from white's pov
const int16 squareEvalPawn[] =
{
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,

     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0,
};

const int16 squareEvalKnight[] =
{
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,

    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

const int16 squareEvalBishop[] =
{
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,    

    -20,-10,-10,-10,-10,-10,-10,-20,    
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};

const int16 squareEvalRook[] =
{
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0,

      0,  0,  0,  5,  5,  0,  0,  0,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      5, 10, 10, 10, 10, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0,
};

const int16 squareEvalQueen[] =
{
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,


    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};

const int16 squareEvalKingMid[] =
{
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20,

     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
};

const int16 squareEvalKingEnd[] =
{
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50,

    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -50,-40,-30,-20,-20,-30,-40,-50,
};


int16 BitBoardUtils::getPieceSquareScore(uint64 pieceSet, uint64 whiteSet, const int16 table[])
{
    int16 val = 0;
    while (pieceSet)
    {
        uint64 piece = getOne(pieceSet);
        int index = bitScan(piece);
        if (piece & whiteSet)
        {
            val += table[index + 64];
        }
        else
        {
            val -= table[index];
        }
        pieceSet ^= piece;
    }
    return val;
}

// call templated version (on chance) of the function internally?
int16 BitBoardUtils::Evaluate(HexaBitBoardPosition *pos)
{
    uint64 allPawns = pos->pawns & RANKS2TO7;    // get rid of game state variables
    uint64 allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;

    uint64 allQueens = pos->bishopQueens & pos->rookQueens;
    uint64 allRooks = pos->rookQueens ^ allQueens;
    uint64 allBishops = pos->bishopQueens ^ allQueens;
    uint64 allKnights = pos->knights;

    uint64 whitePieces = pos->whitePieces;
    uint64 blackPieces = allPieces & (~pos->whitePieces);

    uint64 whitePawns = allPawns    & whitePieces;
    uint64 whiteKnights = allKnights  & whitePieces;
    uint64 whiteQueens = allQueens   & whitePieces;
    uint64 whiteRooks = allRooks    & whitePieces;
    uint64 whiteBishops = allBishops  & whitePieces;

    // material eval
    int16 material = 0;

    material += (2.0f * popCount(whiteQueens)  - popCount(allQueens))  * QUEEN_MATERIAL_VAL;
    material += (2.0f * popCount(whiteRooks)   - popCount(allRooks))   * ROOK_MATERIAL_VAL;
    material += (2.0f * popCount(whiteBishops) - popCount(allBishops)) * BISHOP_MATERIAL_VAL;
    material += (2.0f * popCount(whiteKnights) - popCount(allKnights)) * KNIGHT_MATERIAL_VAL;
    material += (2.0f * popCount(whitePawns)   - popCount(allPawns))   * PAWN_MATERIAL_VAL;

    // positional eval (using piece square tables)
    // maybe use two tables - for white and black pieces?
    int16 positional = 0;

    positional += getPieceSquareScore(allPawns,   whitePieces, squareEvalPawn);
    positional += getPieceSquareScore(allKnights, whitePieces, squareEvalKnight);
    positional += getPieceSquareScore(allBishops, whitePieces, squareEvalBishop);
    positional += getPieceSquareScore(allRooks,   whitePieces, squareEvalRook);
    positional += getPieceSquareScore(allQueens,  whitePieces, squareEvalQueen);

    bool endGame = false;
    uint64 allValPieces   = allRooks | allKnights | allBishops;
    uint64 whiteValPieces = whiteRooks | whiteBishops | whiteKnights;
    uint64 blackValPieces = allValPieces ^ whiteValPieces;
    if ((allQueens == 0) || (whiteValPieces == 0) || (blackValPieces == 0))
    {
        endGame = true;
    }

    if (endGame)
    {
        positional += getPieceSquareScore(pos->kings, whitePieces, squareEvalKingEnd);
    }
    else
    {
        positional += getPieceSquareScore(pos->kings, whitePieces, squareEvalKingMid);
    }

    int16 finalEval = material + positional;

    // we computed everything when viewed from white's side, reverse the sign if it's black's chance
    if (pos->chance == BLACK)
    {
        finalEval = -finalEval;
    }

    // Hack: never return an eval that has LSB set
    finalEval = finalEval & 0xFFFE;

    return finalEval;
}


