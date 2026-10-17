/*
 * Every kind of object with static storage that game code defines, compiled as overlay KINDS_OV with that
 * overlay's forced include (and the host flags of PLAN.md 4.4: -fno-common, -ftrivial-auto-var-init=zero).
 * Kinds<OV>_List says where each object should land; test_kinds.c checks it against the scanned sections.
 * Kinds<OV>_Dirty and _Pristine cover the objects whose address is not taken, which the optimizer may shrink or
 * split (GlobalOpt keeps the section on the new globals).
 */
#include "kinds.h"

#define PASTE2(a, b) a##b
#define PASTE(a, b) PASTE2(a, b)
/* External names get the overlay number so that both copies link into one test. */
#define G(name) PASTE(name, KINDS_OV)
#define FN(name) PASTE(PASTE(Kinds, KINDS_OV), name)

typedef struct Pair {
    int a;
    int *p;
} Pair;

int G(gInit) = 5;
static int sInit = 7;
int G(gZero) = 0;
static int sZero;
int G(gTentative);
static char sBuf[] = "mutable";
const int G(gConst) = 9;
const int G(gConstTable)[4] = { 1, 2, 3, 4 };
const int G(gConstZero)[4];
int *const G(gConstPtrTable)[2] = { &G(gInit), &G(gZero) };
int *G(gPtrTable)[2] = { &G(gInit), &G(gZero) };
const char *G(gStrPtr) = "literal";
int *G(gCompound) = (int[]){ 10, 20, 30 };
static Pair sPair = { 2, &sInit };
static int sBig[4096];
/* Only ever set to 1, and its address is never taken: GlobalOpt may shrink it to a bool. */
static int sFlag;
/* Accessed by field only: GlobalOpt may split it into two globals. */
static struct {
    int a;
    int b;
} sSplit;

static int *LocalZero(void)
{
    static int sCounter;
    return &sCounter;
}

static int *LocalInit(void)
{
    static int sSeeded = 3;
    return &sSeeded;
}

static const int *LocalConst(void)
{
    static const int sTable[3] = { 4, 5, 6 };
    return sTable;
}

/* What a static inline function in a header does: each file that uses it gets its own copy of the static. */
static inline int *HeaderCounter(void)
{
    static int sCount;
    return &sCount;
}

/* An automatic array with an initializer and one without: -ftrivial-auto-var-init=zero must not create any
 * writable global for them (the initializer lives in a constant that is copied in). */
static int Automatic(int n)
{
    int filled[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int unset[4];
    return filled[n & 7] + unset[n & 3];
}

#define ENTRY(desc, obj, where_) \
    out[n++] = (KindsEntry){ desc, &(obj), sizeof(obj), where_ }

int FN(_List)(KindsEntry *out)
{
    int n = 0;

    ENTRY("initialized global", G(gInit), KINDS_DATA);
    ENTRY("initialized static", sInit, KINDS_DATA);
    ENTRY("global = 0", G(gZero), KINDS_BSS);
    ENTRY("static without initializer", sZero, KINDS_BSS);
    ENTRY("tentative definition (-fno-common)", G(gTentative), KINDS_BSS);
    ENTRY("char array from a string", sBuf, KINDS_DATA);
    ENTRY("pointer table (relocations)", G(gPtrTable), KINDS_DATA);
    ENTRY("pointer to a string literal", G(gStrPtr), KINDS_DATA);
    ENTRY("struct with a pointer", sPair, KINDS_DATA);
    ENTRY("16 KB zeroed array", sBig, KINDS_BSS);
    ENTRY("function-local static = 0", *LocalZero(), KINDS_BSS);
    ENTRY("function-local static = 3", *LocalInit(), KINDS_DATA);
    ENTRY("static inline's local static", *HeaderCounter(), KINDS_BSS);
    ENTRY("const scalar", G(gConst), KINDS_OUTSIDE);
    ENTRY("const table", G(gConstTable), KINDS_OUTSIDE);
    ENTRY("const without initializer", G(gConstZero), KINDS_OUTSIDE);
    ENTRY("const pointer table (relocations)", G(gConstPtrTable), KINDS_OUTSIDE);
    ENTRY("function-local static const", *LocalConst(), KINDS_OUTSIDE);
    out[n++] = (KindsEntry){ "string literal", G(gStrPtr), 8, KINDS_OUTSIDE };
    out[n++] = (KindsEntry){ "file-scope compound literal (not reset)", G(gCompound), 3 * sizeof(int),
                             KINDS_OUTSIDE };
    return n;
}

void FN(_Dirty)(void)
{
    G(gInit) = -1;
    sInit = -1;
    G(gZero) = 1;
    sZero = 1;
    G(gTentative) = 1;
    sBuf[0] = 'M';
    G(gPtrTable)[0] = 0;
    G(gStrPtr) = 0;
    sPair.p = 0;
    sBig[4095] = 1;
    *LocalZero() = 1;
    *LocalInit() = 1;
    *HeaderCounter() = 1;
    sFlag = 1;
    sSplit.a = 1;
    sSplit.b = 2;
}

int FN(_Pristine)(void)
{
    return G(gInit) == 5 && sInit == 7 && G(gZero) == 0 && sZero == 0 && G(gTentative) == 0 && sBuf[0] == 'm'
        && G(gPtrTable)[0] == &G(gInit) && G(gStrPtr) != 0 && G(gStrPtr)[0] == 'l' && sPair.p == &sInit
        && sBig[4095] == 0 && *LocalZero() == 0 && *LocalInit() == 3 && *HeaderCounter() == 0 && sFlag == 0
        && sSplit.a + sSplit.b == 0 && Automatic(sZero) == 1;
}
