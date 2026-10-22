#ifndef POKEBW2_DWC_DARRAY_H
#define POKEBW2_DWC_DARRAY_H

#include "types.h"

// GameSpy's dynamic arrays (darray.c), in overlay 11, which GameSpy's code in overlays 11 and 189 keeps its lists in.
// The ROM has no names for the file or its functions: the file is named after GameSpy's, as pret/pokerevo's Wii build
// has it (src/SDK/DWC/gamespy/darray.c), and so are the functions, after GameSpy's darray.c

typedef int (*ArrayCompareFn)(const void *a, const void *b);
typedef void (*ArrayElementFreeFn)(void *element);
typedef void (*ArrayMapFn)(void *element, void *clientData);
typedef int (*ArrayMapFn2)(void *element, void *clientData);

// An array of count elements of elementSize bytes, with room for capacity; it grows by growBy, and frees each element
// that it drops with freeFunc
typedef struct DArrayImplementation *DArray;

struct DArrayImplementation {
    int count;
    int capacity;
    int elemsize;
    int growby;
    ArrayElementFreeFn elemfreefn;
    void *list;
};

// ArrayNew, ArrayFree, ArrayLength and ArrayNth (NULL for an index out of the array)
DArray ArrayNew(int elementSize, int growBy, ArrayElementFreeFn freeFunc);
void ArrayFree(DArray array);
int ArrayLength(const DArray array);
void *ArrayNth(DArray array, int index);
// ArrayAppend, then insertions at an index and in the order of a compare function
void ArrayAppend(DArray array, const void *element);
void ArrayInsertAt(DArray array, const void *element, int index);
void ArrayInsertSorted(DArray array, const void *element, ArrayCompareFn compare);
// ArrayRemoveAt, which doesn't free the element, ArrayDeleteAt and ArrayReplaceAt, which do, and ArraySort
void ArrayRemoveAt(DArray array, int index);
void ArrayDeleteAt(DArray array, int index);
void ArrayReplaceAt(DArray array, const void *element, int index);
void ArraySort(DArray array, ArrayCompareFn compare);
// ArraySearch: the index of the element from start on that compares equal to the key, or -1, searching a sorted array
// by halves
int ArraySearch(DArray array, const void *key, ArrayCompareFn compare, int start, int sorted);
// ArrayMapBackwards2 stops at the first element the function returns 0 for and returns it
void ArrayMapBackwards(DArray array, ArrayMapFn func, void *clientData);
void *ArrayMapBackwards2(DArray array, ArrayMapFn2 func, void *clientData);
void ArrayClear(DArray array);

#endif // POKEBW2_DWC_DARRAY_H
