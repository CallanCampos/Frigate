#include "types.h"

// Near misses that check_style.py must leave alone.

// A forward declaration of a static, and of a function defined below
static int Clean_Helper(int value);
int Clean_Public(int value);

// Written by the interrupt handler while the loop waits
static volatile int sReady;

// A global defined with an alignment, which is not a prototype
u32 gCleanAligned ATTRIBUTE_ALIGN(32);

typedef volatile u16 vu16_alias;
#define REG_TEST (*(volatile u16 *)0x04000000)
#define STEP(x)                                                                                                        \
    do {                                                                                                               \
        (x)++;                                                                                                         \
    } while (0)

static int Clean_Helper(int value) {
    return value + 1;
}

int Clean_Public(int value) {
    int i;
    int *buffer = (int *)0;

    // A shared cleanup exit
    for (i = 0; i < value; i++) {
        if (buffer == NULL) {
            goto fail;
        }
    }
    STEP(value);
    return Clean_Helper(value);
fail:
    buffer = NULL;
    return -1;
}

int Clean_PointerSum(const u8 *data, int pos) {
    // A pointer sum of the element's own type is plain C
    return *(data + pos) | (*(data + pos + 1) << 8);
}

u16 Clean_Allowed(void *work) {
    // style-ok: byte-offset the work is untyped here
    return *(u16 *)((u8 *)work + 0x10);
}

void Clean_Strings(void) {
    // A string mentioning the words is not code
    const char *text = "TODO: goto end; extern";
    (void)text;
}

int Clean_Bug(int x) {
#ifdef BUGFIX
    return x + 1;
#else
    // BUG: off by one
    return x;
#endif
}
