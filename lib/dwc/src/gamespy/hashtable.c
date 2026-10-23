#include "dwc/hashtable.h"
#include "dwc/ext.h"

// GameSpy's hash tables (hashtable.c)

HashTable TableNew(int elementSize, int bucketCount, TableHashFn hashFunc, TableCompareFn compareFunc,
                   TableElementFreeFn freeFunc) {
    return TableNew2(elementSize, bucketCount, 4, hashFunc, compareFunc, freeFunc);
}

HashTable TableNew2(int elementSize, int bucketCount, int bucketGrowBy, TableHashFn hashFunc,
                    TableCompareFn compareFunc, TableElementFreeFn freeFunc) {
    HashTable table = gsimalloc(sizeof(struct HashImplementation));
    int i;

    table->buckets = gsimalloc(bucketCount * sizeof(DArray));
    for (i = 0; i < bucketCount; i++) {
        table->buckets[i] = ArrayNew(elementSize, bucketGrowBy, freeFunc);
    }
    table->nbuckets = bucketCount;
    table->freefn = freeFunc;
    table->compfn = compareFunc;
    table->hashfn = hashFunc;
    return table;
}

void TableFree(HashTable table) {
    int i;

    if (table != NULL) {
        for (i = 0; i < table->nbuckets; i++) {
            ArrayFree(table->buckets[i]);
        }
        gsifree(table->buckets);
        gsifree(table);
    }
}

int TableCount(HashTable table) {
    int count = 0;
    int i;

    if (table == NULL) {
        return count;
    }
    for (i = 0; i < table->nbuckets; i++) {
        count += ArrayLength(table->buckets[i]);
    }
    return count;
}

void TableEnter(HashTable table, const void *element) {
    int hash;
    int index;

    if (table != NULL) {
        hash = table->hashfn(element, table->nbuckets);
        index = ArraySearch(table->buckets[hash], element, table->compfn, 0, gsi_false);
        if (index == -1) {
            ArrayAppend(table->buckets[hash], element);
        } else {
            ArrayReplaceAt(table->buckets[hash], element, index);
        }
    }
}

int TableRemove(HashTable table, const void *element) {
    int hash;
    int index;

    if (table == NULL) {
        return 0;
    }
    hash = table->hashfn(element, table->nbuckets);
    index = ArraySearch(table->buckets[hash], element, table->compfn, 0, gsi_false);
    if (index == -1) {
        return 0;
    }
    ArrayDeleteAt(table->buckets[hash], index);
    return 1;
}

void *TableLookup(HashTable table, const void *element) {
    int hash;
    int index;
    void *result;

    if (table == NULL) {
        return NULL;
    }
    hash = table->hashfn(element, table->nbuckets);
    result = NULL;
    index = ArraySearch(table->buckets[hash], element, table->compfn, 0, gsi_false);
    if (index != -1) {
        result = ArrayNth(table->buckets[hash], index);
    }
    return result;
}

void TableMapSafe(HashTable table, TableMapFn func, void *clientData) {
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        ArrayMapBackwards(table->buckets[i], func, clientData);
    }
}

void *TableMapSafe2(HashTable table, TableMapFn2 func, void *clientData) {
    int i;
    void *element;

    for (i = 0; i < table->nbuckets; i++) {
        element = ArrayMapBackwards2(table->buckets[i], func, clientData);
        if (element != NULL) {
            return element;
        }
    }
    return NULL;
}
