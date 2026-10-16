#ifndef POKEBW2_DWC_HASHTABLE_H
#define POKEBW2_DWC_HASHTABLE_H

#include "types.h"
#include "dwc/darray.h"

// GameSpy's hash tables (hashtable.c), in overlay 11: an array of buckets, each a dynamic array (dwc/darray.h) of
// elements. The ROM has no names for the file or its functions: the file is named after GameSpy's, as pret/pokerevo's
// Wii build has it (src/SDK/DWC/gamespy/hashtable.c), and so are the functions in the comments beside them, while the
// symbols keep their addresses

typedef int (*TableHashFunc)(const void *element, int bucketCount);
typedef int (*TableCompareFunc)(const void *a, const void *b);
typedef void (*TableFreeFunc)(void *element);
typedef void (*TableMapFunc)(void *element, void *clientData);
typedef int (*TableMapFunc2)(void *element, void *clientData);

typedef struct HashTable {
    DArray **buckets;
    int bucketCount;
    TableFreeFunc freeFunc;
    TableHashFunc hashFunc;
    TableCompareFunc compareFunc;
} HashTable;

// TableNew, whose buckets grow by 4, and TableNew2; TableFree and TableCount
HashTable *func_ov011_02171818(int elementSize, int bucketCount, TableHashFunc hashFunc, TableCompareFunc compareFunc,
                               TableFreeFunc freeFunc);
HashTable *func_ov011_02171830(int elementSize, int bucketCount, int bucketGrowBy, TableHashFunc hashFunc,
                               TableCompareFunc compareFunc, TableFreeFunc freeFunc);
void func_ov011_0217187c(HashTable *table);
int func_ov011_021718ac(HashTable *table);
// TableEnter, which adds the element or replaces the one that compares equal, TableRemove and TableLookup
void func_ov011_021718dc(HashTable *table, const void *element);
BOOL func_ov011_02171920(HashTable *table, const void *element);
void *func_ov011_02171964(HashTable *table, const void *element);
// TableMapSafe, and TableMapSafe2, which returns the first element the function returns 0 for, or NULL
void func_ov011_021719a4(HashTable *table, TableMapFunc func, void *clientData);
void *func_ov011_021719cc(HashTable *table, TableMapFunc2 func, void *clientData);

#endif // POKEBW2_DWC_HASHTABLE_H
