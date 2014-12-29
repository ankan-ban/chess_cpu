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
