#include "dwc/darray.h"
#include "dwc/ext.h"

// GameSpy's dynamic arrays (darray.c). The names of the functions that keep their addresses are in dwc/darray.h

static void FreeElement(DArray *array, int index);
static void ArrayGrow(DArray *array);
static void SetElement(DArray *array, const void *element, int index);
static void *mylsearch(const void *key, void *base, int count, int size, DArrayCompareFunc compare);
static void *mybsearch(const void *key, void *base, int count, int size, DArrayCompareFunc compare, int *found);

// Frees the element at the index with the array's free function, if it has one
static void FreeElement(DArray *array, int index) {
    if (array->freeFunc != NULL) {
        array->freeFunc(func_ov011_0217158c(array, index));
    }
}

// Makes room for growBy more elements
static void ArrayGrow(DArray *array) {
    int capacity = array->capacity + array->growBy;

    array->capacity = capacity;
    array->list = gsirealloc(array->list, capacity * array->elementSize);
}

// Copies the element to the index
static void SetElement(DArray *array, const void *element, int index) {
    memcpy(func_ov011_0217158c(array, index), element, array->elementSize);
}

DArray *func_ov011_02171514(int elementSize, int growBy, DArrayFreeFunc freeFunc) {
    DArray *array = gsimalloc(sizeof(DArray));

    if (growBy == 0) {
        growBy = 8;
    }
    array->count = 0;
    array->capacity = growBy;
    array->elementSize = elementSize;
    array->growBy = growBy;
    array->freeFunc = freeFunc;
    if (growBy != 0) {
        array->list = gsimalloc(growBy * array->elementSize);
        memset(array->list, 0, array->capacity * array->elementSize);
    } else {
        array->list = NULL;
    }
    return array;
}

void func_ov011_02171558(DArray *array) {
    int i;

    for (i = 0; i < array->count; i++) {
        FreeElement(array, i);
    }
    if (array->list != NULL) {
        gsifree(array->list);
    }
    gsifree(array);
}

int func_ov011_02171588(DArray *array) {
    return array->count;
}

void *func_ov011_0217158c(DArray *array, int index) {
    if (index < 0 || index >= array->count) {
        return NULL;
    }
    return array->list + array->elementSize * index;
}

void func_ov011_021715a4(DArray *array, const void *element) {
    if (array != NULL) {
        ArrayInsertAt(array, element, array->count);
    }
}

void ArrayInsertAt(DArray *array, const void *element, int index) {
    int count;

    if (array->count == array->capacity) {
        ArrayGrow(array);
    }
    array->count++;
    count = array->count - 1;
    if (index < count) {
        memmove(func_ov011_0217158c(array, index + 1), func_ov011_0217158c(array, index),
                (count - index) * array->elementSize);
    }
    SetElement(array, element, index);
}

void ArrayInsertSorted(DArray *array, const void *element, DArrayCompareFunc compare) {
    int found;
    char *position = mybsearch(element, array->list, array->count, array->elementSize, compare, &found);

    ArrayInsertAt(array, element, (position - array->list) / array->elementSize);
}

void func_ov011_02171634(DArray *array, int index) {
    int last = array->count - 1;

    if (index < last) {
        memmove(func_ov011_0217158c(array, index), func_ov011_0217158c(array, index + 1),
                (last - index) * array->elementSize);
    }
    array->count--;
}

void func_ov011_02171668(DArray *array, int index) {
    FreeElement(array, index);
    func_ov011_02171634(array, index);
}

void ArrayReplaceAt(DArray *array, const void *element, int index) {
    FreeElement(array, index);
    SetElement(array, element, index);
}

void ArraySort(DArray *array, DArrayCompareFunc compare) {
    qsort(array->list, array->count, array->elementSize, compare);
}

int func_ov011_021716ac(DArray *array, const void *key, DArrayCompareFunc compare, int start, int sorted) {
    int found = TRUE;
    int count;
    char *position;

    if (array == NULL || (count = array->count) == 0) {
        return -1;
    }
    if (sorted) {
        position =
            mybsearch(key, func_ov011_0217158c(array, start), count - start, array->elementSize, compare, &found);
    } else {
        position = mylsearch(key, func_ov011_0217158c(array, start), count - start, array->elementSize, compare);
    }
    if (position != NULL && found) {
        return (position - array->list) / array->elementSize;
    }
    return -1;
}

void ArrayMapBackwards(DArray *array, DArrayMapFunc func, void *clientData) {
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        func(func_ov011_0217158c(array, i), clientData);
    }
}

void *ArrayMapBackwards2(DArray *array, DArrayMapFunc2 func, void *clientData) {
    int i;
    void *element;

    for (i = array->count - 1; i >= 0; i--) {
        element = func_ov011_0217158c(array, i);
        if (func(element, clientData) == 0) {
            return element;
        }
    }
    return NULL;
}

void ArrayClear(DArray *array) {
    int i;

    for (i = func_ov011_02171588(array) - 1; i >= 0; i--) {
        func_ov011_02171668(array, i);
    }
}

// A linear search, for an unsorted array
static void *mylsearch(const void *key, void *base, int count, int size, DArrayCompareFunc compare) {
    int i;
    int offset;

    for (i = 0; i < count; i++) {
        offset = size * i;
        if (compare(key, (char *)base + offset) == 0) {
            return (char *)base + offset;
        }
    }
    return NULL;
}

// A binary search: the place where the key is or goes, and whether it is there
static void *mybsearch(const void *key, void *base, int count, int size, DArrayCompareFunc compare, int *found) {
    int low = 0;
    int high = count - 1;
    int middle;
    int result;

    *found = FALSE;
    while (low <= high) {
        middle = (low + high) >> 1;
        result = compare((char *)base + middle * size, key);
        if (result == 0) {
            *found = TRUE;
        }
        if (result < 0) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return (char *)base + low * size;
}
