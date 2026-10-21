#include "dwc/hashtable.h"
#include "dwc/ext.h"

// GameSpy's hash tables (hashtable.c)

HashTable *TableNew(int elementSize, int bucketCount, TableHashFunc hashFunc, TableCompareFunc compareFunc,
                    TableFreeFunc freeFunc) {
    return TableNew2(elementSize, bucketCount, 4, hashFunc, compareFunc, freeFunc);
}

HashTable *TableNew2(int elementSize, int bucketCount, int bucketGrowBy, TableHashFunc hashFunc,
                     TableCompareFunc compareFunc, TableFreeFunc freeFunc) {
    HashTable *table = gsimalloc(sizeof(HashTable));
    int i;

    table->buckets = gsimalloc(bucketCount * sizeof(DArray *));
    for (i = 0; i < bucketCount; i++) {
        table->buckets[i] = func_ov011_02171514(elementSize, bucketGrowBy, freeFunc);
    }
    table->bucketCount = bucketCount;
    table->freeFunc = freeFunc;
    table->compareFunc = compareFunc;
    table->hashFunc = hashFunc;
    return table;
}

void TableFree(HashTable *table) {
    int i;

    if (table != NULL) {
        for (i = 0; i < table->bucketCount; i++) {
            func_ov011_02171558(table->buckets[i]);
        }
        gsifree(table->buckets);
        gsifree(table);
    }
}

int TableCount(HashTable *table) {
    int count = 0;
    int i;

    if (table == NULL) {
        return count;
    }
    for (i = 0; i < table->bucketCount; i++) {
        count += func_ov011_02171588(table->buckets[i]);
    }
    return count;
}

void TableEnter(HashTable *table, const void *element) {
    int hash;
    int index;

    if (table != NULL) {
        hash = table->hashFunc(element, table->bucketCount);
        index = func_ov011_021716ac(table->buckets[hash], element, table->compareFunc, 0, gsi_false);
        if (index == -1) {
            func_ov011_021715a4(table->buckets[hash], element);
        } else {
            ArrayReplaceAt(table->buckets[hash], element, index);
        }
    }
}

gsi_bool TableRemove(HashTable *table, const void *element) {
    int hash;
    int index;

    if (table == NULL) {
        return gsi_false;
    }
    hash = table->hashFunc(element, table->bucketCount);
    index = func_ov011_021716ac(table->buckets[hash], element, table->compareFunc, 0, gsi_false);
    if (index == -1) {
        return gsi_false;
    }
    func_ov011_02171668(table->buckets[hash], index);
    return gsi_true;
}

void *TableLookup(HashTable *table, const void *element) {
    int hash;
    int index;
    void *result;

    if (table == NULL) {
        return NULL;
    }
    hash = table->hashFunc(element, table->bucketCount);
    result = NULL;
    index = func_ov011_021716ac(table->buckets[hash], element, table->compareFunc, 0, gsi_false);
    if (index != -1) {
        result = func_ov011_0217158c(table->buckets[hash], index);
    }
    return result;
}

void TableMapSafe(HashTable *table, TableMapFunc func, void *clientData) {
    int i;

    for (i = 0; i < table->bucketCount; i++) {
        ArrayMapBackwards(table->buckets[i], func, clientData);
    }
}

void *TableMapSafe2(HashTable *table, TableMapFunc2 func, void *clientData) {
    int i;
    void *element;

    for (i = 0; i < table->bucketCount; i++) {
        element = ArrayMapBackwards2(table->buckets[i], func, clientData);
        if (element != NULL) {
            return element;
        }
    }
    return NULL;
}
