#include "dwc/darray.h"
#include "dwc/ext.h"

// GameSpy's dynamic arrays (darray.c): see dwc/darray.h

static void FreeElement(DArray array, int index);
static void ArrayGrow(DArray array);
static void SetElement(DArray array, const void *element, int index);
static void *mylsearch(const void *key, void *base, int count, int size, ArrayCompareFn compare);
static void *mybsearch(const void *key, void *base, int count, int size, ArrayCompareFn compare, int *found);

// Frees the element at the index with the array's free function, if it has one
static void FreeElement(DArray array, int index) {
    if (array->elemfreefn != NULL) {
        array->elemfreefn(ArrayNth(array, index));
    }
}

// Makes room for growBy more elements
static void ArrayGrow(DArray array) {
    int capacity = array->capacity + array->growby;

    array->capacity = capacity;
    array->list = gsirealloc(array->list, capacity * array->elemsize);
}

// Copies the element to the index
static void SetElement(DArray array, const void *element, int index) {
    memcpy(ArrayNth(array, index), element, array->elemsize);
}

DArray ArrayNew(int elementSize, int growBy, ArrayElementFreeFn freeFunc) {
    DArray array = gsimalloc(sizeof(struct DArrayImplementation));

    if (growBy == 0) {
        growBy = 8;
    }
    array->count = 0;
    array->capacity = growBy;
    array->elemsize = elementSize;
    array->growby = growBy;
    array->elemfreefn = freeFunc;
    if (growBy != 0) {
        array->list = gsimalloc(growBy * array->elemsize);
        memset(array->list, 0, array->capacity * array->elemsize);
    } else {
        array->list = NULL;
    }
    return array;
}

void ArrayFree(DArray array) {
    int i;

    for (i = 0; i < array->count; i++) {
        FreeElement(array, i);
    }
    if (array->list != NULL) {
        gsifree(array->list);
    }
    gsifree(array);
}

int ArrayLength(const DArray array) {
    return array->count;
}

void *ArrayNth(DArray array, int index) {
    if (index < 0 || index >= array->count) {
        return NULL;
    }
    return (char *)array->list + array->elemsize * index;
}

void ArrayAppend(DArray array, const void *element) {
    if (array != NULL) {
        ArrayInsertAt(array, element, array->count);
    }
}

void ArrayInsertAt(DArray array, const void *element, int index) {
    int count;

    if (array->count == array->capacity) {
        ArrayGrow(array);
    }
    array->count++;
    count = array->count - 1;
    if (index < count) {
        memmove(ArrayNth(array, index + 1), ArrayNth(array, index), (count - index) * array->elemsize);
    }
    SetElement(array, element, index);
}

void ArrayInsertSorted(DArray array, const void *element, ArrayCompareFn compare) {
    int found;
    void *position = mybsearch(element, array->list, array->count, array->elemsize, compare, &found);

    ArrayInsertAt(array, element, ((char *)position - (char *)array->list) / array->elemsize);
}

void ArrayRemoveAt(DArray array, int index) {
    int last = array->count - 1;

    if (index < last) {
        memmove(ArrayNth(array, index), ArrayNth(array, index + 1), (last - index) * array->elemsize);
    }
    array->count--;
}

void ArrayDeleteAt(DArray array, int index) {
    FreeElement(array, index);
    ArrayRemoveAt(array, index);
}

void ArrayReplaceAt(DArray array, const void *element, int index) {
    FreeElement(array, index);
    SetElement(array, element, index);
}

void ArraySort(DArray array, ArrayCompareFn compare) {
    qsort(array->list, array->count, array->elemsize, compare);
}

int ArraySearch(DArray array, const void *key, ArrayCompareFn compare, int start, int sorted) {
    int found = gsi_true;
    int count;
    void *position;

    if (array == NULL || (count = array->count) == 0) {
        return -1;
    }
    if (sorted) {
        position = mybsearch(key, ArrayNth(array, start), count - start, array->elemsize, compare, &found);
    } else {
        position = mylsearch(key, ArrayNth(array, start), count - start, array->elemsize, compare);
    }
    if (position != NULL && found) {
        return ((char *)position - (char *)array->list) / array->elemsize;
    }
    return -1;
}

void ArrayMapBackwards(DArray array, ArrayMapFn func, void *clientData) {
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        func(ArrayNth(array, i), clientData);
    }
}

void *ArrayMapBackwards2(DArray array, ArrayMapFn2 func, void *clientData) {
    int i;
    void *element;

    for (i = array->count - 1; i >= 0; i--) {
        element = ArrayNth(array, i);
        if (func(element, clientData) == 0) {
            return element;
        }
    }
    return NULL;
}

void ArrayClear(DArray array) {
    int i;

    for (i = ArrayLength(array) - 1; i >= 0; i--) {
        ArrayDeleteAt(array, i);
    }
}

// A linear search, for an unsorted array
static void *mylsearch(const void *key, void *base, int count, int size, ArrayCompareFn compare) {
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
static void *mybsearch(const void *key, void *base, int count, int size, ArrayCompareFn compare, int *found) {
    int low = 0;
    int high = count - 1;
    int middle;
    int result;

    *found = gsi_false;
    while (low <= high) {
        middle = (low + high) >> 1;
        result = compare((char *)base + middle * size, key);
        if (result == 0) {
            *found = gsi_true;
        }
        if (result < 0) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return (char *)base + low * size;
}
