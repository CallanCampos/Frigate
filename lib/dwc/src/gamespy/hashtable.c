#include "dwc/hashtable.h"
#include "dwc/ext.h"

// GameSpy's hash tables (hashtable.c). The functions' names, from pret/pokerevo's Wii build, are in dwc/hashtable.h

HashTable *func_ov011_02171818(int elementSize, int bucketCount, TableHashFunc hashFunc, TableCompareFunc compareFunc,
                               TableFreeFunc freeFunc) {
    return func_ov011_02171830(elementSize, bucketCount, 4, hashFunc, compareFunc, freeFunc);
}

HashTable *func_ov011_02171830(int elementSize, int bucketCount, int bucketGrowBy, TableHashFunc hashFunc,
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

void func_ov011_0217187c(HashTable *table) {
    int i;

    if (table != NULL) {
        for (i = 0; i < table->bucketCount; i++) {
            func_ov011_02171558(table->buckets[i]);
        }
        gsifree(table->buckets);
        gsifree(table);
    }
}

int func_ov011_021718ac(HashTable *table) {
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

void func_ov011_021718dc(HashTable *table, const void *element) {
    int hash;
    int index;

    if (table != NULL) {
        hash = table->hashFunc(element, table->bucketCount);
        index = func_ov011_021716ac(table->buckets[hash], element, table->compareFunc, 0, FALSE);
        if (index == -1) {
            func_ov011_021715a4(table->buckets[hash], element);
        } else {
            func_ov011_0217167c(table->buckets[hash], element, index);
        }
    }
}

BOOL func_ov011_02171920(HashTable *table, const void *element) {
    int hash;
    int index;

    if (table == NULL) {
        return FALSE;
    }
    hash = table->hashFunc(element, table->bucketCount);
    index = func_ov011_021716ac(table->buckets[hash], element, table->compareFunc, 0, FALSE);
    if (index == -1) {
        return FALSE;
    }
    func_ov011_02171668(table->buckets[hash], index);
    return TRUE;
}

void *func_ov011_02171964(HashTable *table, const void *element) {
    int hash;
    int index;
    void *result;

    if (table == NULL) {
        return NULL;
    }
    hash = table->hashFunc(element, table->bucketCount);
    result = NULL;
    index = func_ov011_021716ac(table->buckets[hash], element, table->compareFunc, 0, FALSE);
    if (index != -1) {
        result = func_ov011_0217158c(table->buckets[hash], index);
    }
    return result;
}

void func_ov011_021719a4(HashTable *table, TableMapFunc func, void *clientData) {
    int i;

    for (i = 0; i < table->bucketCount; i++) {
        func_ov011_02171724(table->buckets[i], func, clientData);
    }
}

void *func_ov011_021719cc(HashTable *table, TableMapFunc2 func, void *clientData) {
    int i;
    void *element;

    for (i = 0; i < table->bucketCount; i++) {
        element = func_ov011_02171744(table->buckets[i], func, clientData);
        if (element != NULL) {
            return element;
        }
    }
    return NULL;
}
