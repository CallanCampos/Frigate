// The host probe's stand-in for the decomp's include/types.h, for the struct layout runs of run_all.sh
// (layouts.py with -I shim/include_types): the base types with 64-bit integers aligned to 4 as MWCC aligns them, so
// that the probe can see which of the game's structs (the save's blocks, the files' records) keep the DS's layout on
// a 64-bit host once the decomp's typedefs do the same (PLAN.md §4.8, rule 4).
#ifndef POKEBW2_TYPES_H
#define POKEBW2_TYPES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64 __attribute__((aligned(4))); /* host shim: MWCC aligns 64-bit types to 4 */
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64 __attribute__((aligned(4)));

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;

typedef float f32;
typedef double f64 __attribute__((aligned(4)));

typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define NULL ((void *)0)

#define NELEMS(array) (sizeof(array) / sizeof((array)[0]))

// NitroSDK's alignment attribute, as in `static u8 buffer[64] ATTRIBUTE_ALIGN(32);`
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))

#endif // POKEBW2_TYPES_H
