// MSL C's qsort.c: qsort of <stdlib.h>, a heapsort that swaps the elements bytewise, so it needs no memory of its own.
// The compiler leaves the dead epilogue after the loop's end in the function: it is at 0x02085cac, past qsort's symbol.
#include <stdlib.h>

static inline void swap_elements(unsigned char *a, unsigned char *b, size_t size) {
    unsigned char t;

    if (size != 0) {
        do {
            t = *a;
            *a++ = *b;
            *b++ = t;
        } while (--size != 0);
    }
}

// The heap has the elements from 1 to last (counting from 1) as nodes of the tree where node j has the children 2j and
// 2j + 1. The first loop builds it from its lower half up, as the second takes the largest element off its top and
// puts it behind the heap, until one element is left.
void qsort(void *base, size_t nmemb, size_t size, int (*compare)(const void *, const void *)) {
    unsigned char *array = base;
    size_t node;
    size_t last;
    size_t child;
    unsigned char *nodePtr;
    unsigned char *lastPtr;
    unsigned char *parentPtr;
    unsigned char *childPtr;

    if (nmemb < 2) {
        return;
    }
    node = (nmemb >> 1) + 1;
    last = nmemb;
    nodePtr = array + size * (node - 1);
    lastPtr = array + size * (last - 1);
    for (;;) {
        if (node > 1) {
            node--;
            nodePtr -= size;
        } else {
            swap_elements(nodePtr, lastPtr, size);
            last--;
            if (last == 1) {
                return;
            }
            lastPtr -= size;
        }
        child = node;
        childPtr = array + size * (node - 1);
        while (last >= (child << 1)) {
            parentPtr = childPtr;
            child <<= 1;
            childPtr = array + size * (child - 1);
            if (child < last && compare(childPtr, childPtr + size) < 0) {
                child++;
                childPtr += size;
            }
            if (compare(parentPtr, childPtr) >= 0) {
                break;
            }
            swap_elements(parentPtr, childPtr, size);
        }
    }
}
