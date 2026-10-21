#ifndef POKEBW2_DWC_DARRAY_H
#define POKEBW2_DWC_DARRAY_H

#include "types.h"

// GameSpy's dynamic arrays (darray.c), in overlay 11, which GameSpy's code in overlays 11 and 189 keeps its lists in.
// The ROM has no names for the file or its functions: the file is named after GameSpy's, as pret/pokerevo's Wii build
// has it (src/SDK/DWC/gamespy/darray.c), and so are the functions, after GameSpy's darray.c. The ones that gt2.c and
// GameSpy's other files still being worked on call keep their addresses, with their names in the comments beside them

typedef int (*DArrayCompareFunc)(const void *a, const void *b);
typedef void (*DArrayFreeFunc)(void *element);
typedef void (*DArrayMapFunc)(void *element, void *clientData);
typedef int (*DArrayMapFunc2)(void *element, void *clientData);

// An array of count elements of elementSize bytes, with room for capacity; it grows by growBy, and frees each element
// that it drops with freeFunc
typedef struct DArray {
    int count;
    int capacity;
    int elementSize;
    int growBy;
    DArrayFreeFunc freeFunc;
    char *list;
} DArray;

// ArrayNew, ArrayFree, ArrayLength and ArrayNth (NULL for an index out of the array)
DArray *func_ov011_02171514(int elementSize, int growBy, DArrayFreeFunc freeFunc);
void func_ov011_02171558(DArray *array);
int func_ov011_02171588(DArray *array);
void *func_ov011_0217158c(DArray *array, int index);
// ArrayAppend, then insertions at an index and in the order of a compare function
void func_ov011_021715a4(DArray *array, const void *element);
void ArrayInsertAt(DArray *array, const void *element, int index);
void ArrayInsertSorted(DArray *array, const void *element, DArrayCompareFunc compare);
// ArrayRemoveAt, which doesn't free the element, ArrayDeleteAt and ArrayReplaceAt, which do, and ArraySort
void func_ov011_02171634(DArray *array, int index);
void func_ov011_02171668(DArray *array, int index);
void ArrayReplaceAt(DArray *array, const void *element, int index);
void ArraySort(DArray *array, DArrayCompareFunc compare);
// ArraySearch: the index of the element from start on that compares equal to the key, or -1, searching a sorted array
// by halves
int func_ov011_021716ac(DArray *array, const void *key, DArrayCompareFunc compare, int start, int sorted);
// ArrayMapBackwards2 stops at the first element the function returns 0 for and returns it
void ArrayMapBackwards(DArray *array, DArrayMapFunc func, void *clientData);
void *ArrayMapBackwards2(DArray *array, DArrayMapFunc2 func, void *clientData);
void ArrayClear(DArray *array);

#endif // POKEBW2_DWC_DARRAY_H
