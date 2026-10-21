#ifndef POKEBW2_DWC_HASHTABLE_H
#define POKEBW2_DWC_HASHTABLE_H

#include "dwc/darray.h"
#include "dwc/nonport.h"

// GameSpy's hash tables (hashtable.c), in overlay 11: an array of buckets, each a dynamic array (dwc/darray.h) of
// elements. The ROM has no names for the file or its functions: the file is named after GameSpy's, as pret/pokerevo's
// Wii build has it (src/SDK/DWC/gamespy/hashtable.c), and so are the functions, after GameSpy's hashtable.c

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

// TableNew's buckets grow by 4
HashTable *TableNew(int elementSize, int bucketCount, TableHashFunc hashFunc, TableCompareFunc compareFunc,
                    TableFreeFunc freeFunc);
HashTable *TableNew2(int elementSize, int bucketCount, int bucketGrowBy, TableHashFunc hashFunc,
                     TableCompareFunc compareFunc, TableFreeFunc freeFunc);
void TableFree(HashTable *table);
int TableCount(HashTable *table);
// TableEnter adds the element or replaces the one that compares equal
void TableEnter(HashTable *table, const void *element);
gsi_bool TableRemove(HashTable *table, const void *element);
void *TableLookup(HashTable *table, const void *element);
// TableMapSafe2 returns the first element the function returns 0 for, or NULL
void TableMapSafe(HashTable *table, TableMapFunc func, void *clientData);
void *TableMapSafe2(HashTable *table, TableMapFunc2 func, void *clientData);

#endif // POKEBW2_DWC_HASHTABLE_H
