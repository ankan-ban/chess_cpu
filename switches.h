// various knobs (switches)

// move generator switches
// bitwise magic instead of if/else for castle flag updation
#define USE_BITWISE_MAGIC_FOR_CASTLE_FLAG_UPDATION 0

// intel core 2 doesn't have popcnt instruction
#define USE_POPCNT 1

// pentium 4 doesn't have fast HW bitscan
#define USE_HW_BITSCAN 1

// use lookup tabls for figuring out squares in line and squares in between
#define USE_IN_BETWEEN_LUT 1

// use lookup table for king moves
#define USE_KING_LUT 1

// use lookup table for knight moves
#define USE_KNIGHT_LUT 1

// use lookup table (magics) for sliding moves
// reduces performance by ~7% for GPU version
// but very impressive speedup for CPU version (~50%)
#define USE_SLIDING_LUT 1

// use fancy fixed-shift version - ~ 800 KB lookup tables
// (setting this to 0 enables plain magics - with 2.3 MB lookup table)
// plain magics is a bit faster at least for perft (on core 2 duo)
// fancy magics is clearly faster on more recent processors (ivy bridge)
// intel compiler with haswell clearly like plain magics (~7% perf gain over fancy)
#define USE_FANCY_MAGICS 0

// use byte lookup for fancy magics (~150 KB lookup tables)
// >10% slower than fixed shift fancy magics on both CPU and GPU
#define USE_BYTE_LOOKUP_FANCY 0


#define INCREMENTAL_ZOBRIST_UPDATE 1

// null move pruning related constants
#define DEFAULT_NULL_MOVE_REDUCTION 2
#define EXTRA_NULL_MOVE_REDUCTION 3
#define MIN_DEPTH_FOR_EXTRA_REDUCTION 8
#define MIN_MOVES_FOR_NULL_MOVE 4

// check extensions in q-search (seems to weaken the engine - maybe makes it significantly slow?)
#define Q_SEARCH_CHECK_EXTENSIONS 0

// check extension in regular search - worth ~65 ELO!
#define CHECK_EXTENSIONS 1

// use a small (2 MB) Transposition table for q-search
// doesn't seem to help much (or at all ?)
#define USE_Q_TT 1

// min depth to engage IID (internal iterative deepening)
// using IID near horizon can cause lot of extra overhead
#define MIN_DEPTH_FOR_IID 5

// use dual slot Transposition table
// every entry (of 192 bits/24 bytes) has one deepest and one most-recent slot
#define USE_DUAL_SLOT_TT 1


// 16 million slots is default TT size
#if USE_DUAL_SLOT_TT == 1
// 192 MB 
#define DEAFULT_TT_SIZE (192*1024*1024)
#else
// 256 MB 
#define DEAFULT_TT_SIZE (256*1024*1024)
#endif

// no of killer moves per level
#define MAX_KILLERS 2

// use SEE for move ordering - doesn't seem to help
// computing and sorting moves based on SEE adds more cost than benefit of SEE
#define SEE_MOVE_ORDERING 0

#define MIN_DEPTH_FOR_SEE 2

#define Q_SEARCH_SEE_PRUNING 0

// debugging switches
#define GATHER_STATS 0
