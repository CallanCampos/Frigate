#ifndef POKEBW2_TYPES_H
#define POKEBW2_TYPES_H

// What differs between the DS and a 64-bit host (PLAN.md §4.8). Each is a macro, not a typedef, so that what MWCC
// compiles stays token for token as it was
#ifdef PLATFORM_PC
// The host build keeps the DS's 4-byte alignment of 64-bit values, so that structs holding them keep the DS's layout:
// saves, files, and the sizes the game passes as constants. On a typedef the attribute may lower the alignment
#define PORT_DS_ALIGN_64 __attribute__((aligned(4)))
// An integer that holds an address: u32 on the DS, as wide as a pointer on a host
#define uptr __UINTPTR_TYPE__
#define sptr __INTPTR_TYPE__
// The DS's unsigned long, 32 bits, for structs that must keep the DS's layout (a host's long can be 64 bits)
#define ulong32 unsigned int
#else
#define PORT_DS_ALIGN_64
#define uptr u32
#define sptr s32
#define ulong32 unsigned long
#endif

typedef unsigned char u8;
typedef unsigned short u16;
#ifdef PLATFORM_PC
typedef unsigned int u32;
typedef unsigned long long u64 PORT_DS_ALIGN_64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64 PORT_DS_ALIGN_64;
#else
typedef unsigned long u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed long long s64;
#endif

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;

typedef float f32;
typedef double f64 PORT_DS_ALIGN_64;

typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define NULL ((void *)0)

#define NELEMS(array) (sizeof(array) / sizeof((array)[0]))

// NitroSDK's alignment attribute, as in `static u8 buffer[64] ATTRIBUTE_ALIGN(32);`
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))

#endif // POKEBW2_TYPES_H
