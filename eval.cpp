#include "chess.h"
#include "bb_consts.h"

// simple evaluation function taken from chess programming wiki
// References:
// https://chessprogramming.wikispaces.com/Evaluation
// https://chessprogramming.wikispaces.com/Simplified+evaluation+function
// Ankan - TODO: experiment with this and tune it further!

// all values in centi-pawns
#define QUEEN_MATERIAL_VAL  900
#define ROOK_MATERIAL_VAL   500
#define BISHOP_MATERIAL_VAL 330
#define KNIGHT_MATERIAL_VAL 320
#define PAWN_MATERIAL_VAL   100

// 5 centipawn bonus for every extra legal move
#define MOBILITY_FACTOR       5

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


// we use different mobility multipliers for each piece and have a non-linear scale to encourage uniform development of pieces
// good explanation can be found here: http://www.madchess.net/post/madchess-2-0-beta-build-29

#if 0
// TODO: tune these values
const int16 pawnMobilityFactor[]   = { -10,   0,  5 };                          // slight advantage for double push, negative for no moves available
const int16 knightMobilityFactor[] = { -15, -10, -5,  0,  3,  6,  8,  10, 11};
const int16 bishopMobilityFactor[] = { -20, -15, -7, -3,  0,  2,  4,   6,  7,  8,  9, 10, 11, 12};
const int16 rookMobilityFactor[] =   {  -5,  -3, -2, -1,  0,  2,  4,   6,  8, 10, 11, 12, 12, 13, 13};      // TODO: maybe rook mobility is more important in end game
const int16 queenMobilityFactor[] =  { -10,  -8, -6, -3,  0,  2,  4,   6,  8, 10, 11, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20};
const int16 kingMobilityFactor[]  =  { -20, -15,  0,  5,  10, 13, 15, 15, 15};
#endif

// values from the above link
// the most surprising thing is that end game and middle game values when reversed works much better than anything else
const int16 knightMgMobility[] = {-15, -5, -1, 2, 5, 7, 9, 11, 13}; // (10 * x Pow 0.5) - 15};
const int16 knightEgMobility[] = {-30, -10, -2, 4, 10, 14, 18, 22, 26}; // (20 * x Pow 0.5) - 30};
const int16 bishopMgMobility[] = {-25, -11, -6, -1, 3, 6, 9, 12, 14, 17, 19, 21, 23, 25}; // (14 * x Pow 0.5) - 25};
const int16 bishopEgMobility[] = {-50, -22, -11, -2, 6, 12, 18, 24, 29, 34, 38, 42, 46, 50}; // (28 * x Pow 0.5) - 50};
const int16 rookMgMobility[] =   {-10, -4, -2, 0, 2, 3, 4, 5, 6, 8, 8, 9, 10, 11, 12}; // (6 * x Pow 0.5) - 10};
const int16 rookEgMobility[] =   {-50, -22, -11, -2, 6, 12, 18, 24, 29, 34, 38, 42, 46, 50, 54}; // (28 * x Pow 0.5) - 50};
const int16 queenMgMobility[] =  {-10, -6, -5, -4, -2, -2, -1, 0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 10}; // (4 * x Pow 0.5) - 10};
const int16 queenEgMobility[] =  {-50, -30, -22, -16, -10, -6, -2, 2, 6, 10, 13, 16, 19, 22, 24, 27, 30, 32, 34, 37, 39, 41, 43, 45, 47, 50, 51, 53}; // (20 * x Pow 0.5) - 50};

int16 BitBoardUtils::evaluateMobility(const EvalBitBoard &ebb, bool endGame)
{
// use madchess values
#if 1
    const int16 *knightMobilityFactor;
    const int16 *bishopMobilityFactor;
    const int16 *rookMobilityFactor;
    const int16 *queenMobilityFactor;

    // THIS IS NOT A BUG
    // accidently found that the mobility values (from Madchess) when reversed work better
    // This might be because giving too much weight to mobility makes the engine lose pawns in the end game (and/or discourage promotion)
    if (endGame)
    {
        knightMobilityFactor = knightMgMobility;
        bishopMobilityFactor = bishopMgMobility;
        rookMobilityFactor   = rookMgMobility;
        queenMobilityFactor  = queenMgMobility;
    }
    else
    {
        knightMobilityFactor = knightEgMobility;
        bishopMobilityFactor = bishopEgMobility;
        rookMobilityFactor   = rookEgMobility;
        queenMobilityFactor  = queenEgMobility;
    }
#endif


    // these are not strictly valid moves (ignores pinned piece checking, etc)
    // TODO: also try with exact move counts ?

    int16 mobility = 0;
    // 1. pawn moves
#if 0
    uint64 whitePawns = ebb.whitePawns;
    while (whitePawns)
    {
        int count = 0;
        uint64 pawn = getOne(whitePawns);
        uint64 dst1 = northOne(pawn) & (~ebb.allPieces);
        uint64 dst2 = northOne(dst1 & RANK3) & (~ebb.allPieces);
        
        count += !!dst1;
        count += !!dst2;

        mobility += pawnMobilityFactor[count];

        whitePawns ^= pawn;
    }

    uint64 blackPawns = ebb.blackPawns;
    while (blackPawns)
    {
        int count = 0;
        uint64 pawn = getOne(blackPawns);
        uint64 dst1 = southOne(pawn) & (~ebb.allPieces);
        uint64 dst2 = southOne(dst1 & RANK6) & (~ebb.allPieces);

        count += !!dst1;
        count += !!dst2;

        mobility -= pawnMobilityFactor[count];

        blackPawns ^= pawn;
    }
#endif

    // 2. knight moves
    uint64 whiteKnights = ebb.whiteKnights;
    while (whiteKnights)
    {
        uint64 knight = getOne(whiteKnights);
        uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & ~ebb.whitePieces;
        mobility += knightMobilityFactor[popCount(knightMoves)];
        whiteKnights ^= knight;
    }

    uint64 blackKnights = ebb.blackKnights;
    while (blackKnights)
    {
        uint64 knight = getOne(blackKnights);
        uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & ~ebb.blackPieces;
        mobility -= knightMobilityFactor[popCount(knightMoves)];
        blackKnights ^= knight;
    }

    // 3. bishop moves
    uint64 whiteBishops = ebb.whiteBishops;
    while (whiteBishops)
    {
        uint64 bishop = getOne(whiteBishops);
        uint64 bishopMoves = bishopAttacks(bishop, ~ebb.allPieces) & ~ebb.whitePieces;
        mobility += bishopMobilityFactor[popCount(bishopMoves)];
        whiteBishops ^= bishop;
    }

    uint64 blackBishops = ebb.blackBishops;
    while (blackBishops)
    {
        uint64 bishop = getOne(blackBishops);
        uint64 bishopMoves = bishopAttacks(bishop, ~ebb.allPieces) & ~ebb.blackPieces;
        mobility -= bishopMobilityFactor[popCount(bishopMoves)];
        blackBishops ^= bishop;
    }

    // 4. rook moves
    uint64 whiteRooks = ebb.whiteRooks;
    while (whiteRooks)
    {
        uint64 rook = getOne(whiteRooks);
        uint64 rookMoves = rookAttacks(rook, ~ebb.allPieces) & ~ebb.whitePieces;
        mobility += rookMobilityFactor[popCount(rookMoves)];
        whiteRooks ^= rook;
    }

    uint64 blackRooks = ebb.blackRooks;
    while (blackRooks)
    {
        uint64 rook = getOne(blackRooks);
        uint64 rookMoves = rookAttacks(rook, ~ebb.allPieces) & ~ebb.blackPieces;
        mobility -= rookMobilityFactor[popCount(rookMoves)];
        blackRooks ^= rook;
    }

    // 5. queen moves
    uint64 whiteQueens = ebb.whiteQueens;
    while (whiteQueens)
    {
        uint64 queen = getOne(whiteQueens);
        uint64 queenMoves = bishopAttacks(queen, ~ebb.allPieces) & ~ebb.whitePieces;
        queenMoves |= rookAttacks(queen, ~ebb.allPieces) & ~ebb.whitePieces;
        mobility += queenMobilityFactor[popCount(queenMoves)];
        whiteQueens ^= queen;
    }

    uint64 blackQueens = ebb.blackQueens;
    while (blackQueens)
    {
        uint64 queen = getOne(blackQueens);
        uint64 queenMoves = bishopAttacks(queen, ~ebb.allPieces) & ~ebb.blackPieces;
        queenMoves |= rookAttacks(queen, ~ebb.allPieces) & ~ebb.blackPieces;
        mobility -= queenMobilityFactor[popCount(queenMoves)];
        blackQueens ^= queen;
    }

#if 0
    // 6. king moves (only use king mobility in end game)
    if (endGame)
    {

        uint64 whiteControlled = findAttackedSquares(~ebb.allPieces, ebb.whiteBishops | ebb.whiteQueens, ebb.whiteRooks | ebb.whiteQueens, ebb.whitePawns, ebb.whiteKnights, ebb.whiteKing, ebb.blackKing, WHITE);
        uint64 blackControlled = findAttackedSquares(~ebb.allPieces, ebb.blackBishops | ebb.blackQueens, ebb.blackRooks | ebb.blackQueens, ebb.blackPawns, ebb.blackKnights, ebb.blackKing, ebb.whiteKing, BLACK);

        uint64 whiteKingMoves = sqKingAttacks(bitScan(ebb.whiteKing));
        whiteKingMoves &= ~(blackControlled | ebb.whitePieces);
        mobility += kingMobilityFactor[popCount(whiteKingMoves)];

        uint64 blackKingMoves = sqKingAttacks(bitScan(ebb.blackKing));
        blackKingMoves &= ~(whiteControlled | ebb.blackPieces);
    }
#endif
    return mobility;
}


// call templated version (on chance) of the function internally?
int16 BitBoardUtils::Evaluate(HexaBitBoardPosition *pos)
{
    uint64 allPawns = pos->pawns & RANKS2TO7;    // get rid of game state variables

    EvalBitBoard ebb;
    ebb.allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;

    uint64 allQueens = pos->bishopQueens & pos->rookQueens;
    uint64 allRooks = pos->rookQueens ^ allQueens;
    uint64 allBishops = pos->bishopQueens ^ allQueens;
    uint64 allKnights = pos->knights;

    ebb.whitePieces = pos->whitePieces;
    ebb.blackPieces = ebb.allPieces ^ ebb.whitePieces;

    ebb.whitePawns   = allPawns    & ebb.whitePieces;
    ebb.whiteKnights = allKnights  & ebb.whitePieces;
    ebb.whiteQueens  = allQueens   & ebb.whitePieces;
    ebb.whiteRooks   = allRooks    & ebb.whitePieces;
    ebb.whiteBishops = allBishops  & ebb.whitePieces;
    ebb.whiteKing    = pos->kings  & ebb.whitePieces;

    ebb.blackPawns   = allPawns    & ebb.blackPieces;
    ebb.blackKnights = allKnights  & ebb.blackPieces;
    ebb.blackQueens  = allQueens   & ebb.blackPieces;
    ebb.blackRooks   = allRooks    & ebb.blackPieces;
    ebb.blackBishops = allBishops  & ebb.blackPieces;
    ebb.blackKing    = pos->kings  & ebb.blackPieces;

    // material eval
    int16 material = 0;

    material += (popCount(ebb.whiteQueens)  - popCount(ebb.blackQueens))  * QUEEN_MATERIAL_VAL;
    material += (popCount(ebb.whiteRooks)   - popCount(ebb.blackRooks))   * ROOK_MATERIAL_VAL;
    material += (popCount(ebb.whiteBishops) - popCount(ebb.blackBishops)) * BISHOP_MATERIAL_VAL;
    material += (popCount(ebb.whiteKnights) - popCount(ebb.blackKnights)) * KNIGHT_MATERIAL_VAL;
    material += (popCount(ebb.whitePawns)   - popCount(ebb.blackPawns))   * PAWN_MATERIAL_VAL;

    // positional eval (using piece square tables)
    // maybe use two tables - for white and black pieces?
    int16 positional = 0;

    positional += getPieceSquareScore(allPawns,   ebb.whitePieces, squareEvalPawn);
    positional += getPieceSquareScore(allKnights, ebb.whitePieces, squareEvalKnight);
    positional += getPieceSquareScore(allBishops, ebb.whitePieces, squareEvalBishop);
    positional += getPieceSquareScore(allRooks,   ebb.whitePieces, squareEvalRook);
    positional += getPieceSquareScore(allQueens,  ebb.whitePieces, squareEvalQueen);

    bool endGame = false;
    uint64 whiteValPieces = ebb.whiteRooks | ebb.whiteBishops | ebb.whiteKnights;
    uint64 blackValPieces = ebb.blackRooks | ebb.blackBishops | ebb.blackKnights;
    if ((allQueens == 0) || (whiteValPieces == 0) || (blackValPieces == 0))
    {
        endGame = true;
    }

    if (endGame)
    {
        positional += getPieceSquareScore(pos->kings, ebb.whitePieces, squareEvalKingEnd);
    }
    else
    {
        positional += getPieceSquareScore(pos->kings, ebb.whitePieces, squareEvalKingMid);
    }

    // mobility eval
    int16 mobility = evaluateMobility(ebb, endGame);

    // just counting valid moves seems to work better than the above :-/
    //int whiteMoves = countMoves<WHITE>(pos);
    //int blackMoves = countMoves<BLACK>(pos);
    //int16 mobility = (whiteMoves - blackMoves) * MOBILITY_FACTOR;

    int16 finalEval = material + positional + mobility;

    // we computed everything when viewed from white's side, reverse the sign if it's black's chance
    if (pos->chance == BLACK)
    {
        finalEval = -finalEval;
    }


#if 0
    // this can speed up search a bit with a small in-accuracy in evaluation
    // Hack: never return an eval that has LSB set
    finalEval = finalEval & 0xFFFE;
#endif

    return finalEval;
}


