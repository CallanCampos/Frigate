// Unit tests of the decomp's C of libfx's matrix products and vector transforms (lib/nitro/src/fx), as the host
// compiles it, with no ROM: each result written apart from its inputs, over the first input, over the second, and over
// both when they are the same. The ROM's MTX_Concat33, MTX_Concat43, MTX_Concat44 and MTX_MultVec43 load each row (the
// vector) of their first input before they store that row of the result, so on the DS a product or transform into its
// own input gives the same result as one into a matrix of its own, and the game relies on it (MCSSSys_Draw, GFL's
// MAT3_RotationEulerZYX, NitroSystem's SBC and SPL). The expected results are worked out by hand from small whole
// numbers. port/tests/asm_oracle/test_fx_decomp.py compares the same functions with the ROM's code on random inputs.
// Exits 0 when every check passes, 1 otherwise.
#include "gfl/calctool.h"
#include "nitro/fx.h"
#include <stdio.h>
#include <string.h>

// A whole number in fixed point
#define N(n) ((n) * FX32_ONE)
// A matrix's or vector's words, row by row
#define WORDS(value) ((const fx32 *)&(value))

static int sFailures;

static void CheckWords(const char *what, const fx32 *actual, const fx32 *expected, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (actual[i] != expected[i]) {
            printf("FAIL %s: word %d is %#x, expected %#x\n", what, i, (u32)actual[i], (u32)expected[i]);
            sFailures++;
            return;
        }
    }
}

// a's rows [1 2 3], [4 5 6], [7 8 9]; b swaps the first two columns and doubles the third, so the product's rows are
// a's as [2 1 6], [5 4 12], [8 7 18]. b times b is [1 0 0], [0 1 0], [0 0 4]
static void TestConcat33(void) {
    static const MtxFx33 a = { .m = { { N(1), N(2), N(3) }, { N(4), N(5), N(6) }, { N(7), N(8), N(9) } } };
    static const MtxFx33 b = { .m = { { 0, N(1), 0 }, { N(1), 0, 0 }, { 0, 0, N(2) } } };
    static const MtxFx33 ab = { .m = { { N(2), N(1), N(6) }, { N(5), N(4), N(12) }, { N(8), N(7), N(18) } } };
    static const MtxFx33 bb = { .m = { { N(1), 0, 0 }, { 0, N(1), 0 }, { 0, 0, N(4) } } };
    MtxFx33 left = a;
    MtxFx33 right = b;
    MtxFx33 result;

    MTX_Concat33(&left, &right, &result);
    CheckWords("MTX_Concat33 into a matrix of its own", WORDS(result), WORDS(ab), 9);
    MTX_Concat33(&left, &right, &left);
    CheckWords("MTX_Concat33 into a", WORDS(left), WORDS(ab), 9);
    left = a;
    MTX_Concat33(&left, &right, &right);
    CheckWords("MTX_Concat33 into b", WORDS(right), WORDS(ab), 9);
    right = b;
    MTX_Concat33(&right, &right, &right);
    CheckWords("MTX_Concat33 of b and b into b", WORDS(right), WORDS(bb), 9);
}

// As TestConcat33, with a translation: a's is [10 11 12], b's [100 200 300], so the product's is a's through b's
// rotation plus b's, [111 210 324]
static void TestConcat43(void) {
    static const MtxFx43 a = {
        .m = { { N(1), N(2), N(3) }, { N(4), N(5), N(6) }, { N(7), N(8), N(9) }, { N(10), N(11), N(12) } }
    };
    static const MtxFx43 b = { .m = { { 0, N(1), 0 }, { N(1), 0, 0 }, { 0, 0, N(2) }, { N(100), N(200), N(300) } } };
    static const MtxFx43 ab = {
        .m = { { N(2), N(1), N(6) }, { N(5), N(4), N(12) }, { N(8), N(7), N(18) }, { N(111), N(210), N(324) } }
    };
    MtxFx43 left = a;
    MtxFx43 right = b;
    MtxFx43 result;

    MTX_Concat43(&left, &right, &result);
    CheckWords("MTX_Concat43 into a matrix of its own", WORDS(result), WORDS(ab), 12);
    MTX_Concat43(&left, &right, &left);
    CheckWords("MTX_Concat43 into a", WORDS(left), WORDS(ab), 12);
    left = a;
    MTX_Concat43(&left, &right, &right);
    CheckWords("MTX_Concat43 into b", WORDS(right), WORDS(ab), 12);
}

// a's rows [1 2 3 4] to [13 14 15 16]; b swaps the first two columns and doubles the third, so the product's rows are
// [2 1 6 4], [6 5 14 8], [10 9 22 12], [14 13 30 16]
static void TestConcat44(void) {
    static const MtxFx44 a = { .m = { { N(1), N(2), N(3), N(4) },
                                      { N(5), N(6), N(7), N(8) },
                                      { N(9), N(10), N(11), N(12) },
                                      { N(13), N(14), N(15), N(16) } } };
    static const MtxFx44 b = { .m = { { 0, N(1), 0, 0 }, { N(1), 0, 0, 0 }, { 0, 0, N(2), 0 }, { 0, 0, 0, N(1) } } };
    static const MtxFx44 ab = { .m = { { N(2), N(1), N(6), N(4) },
                                       { N(6), N(5), N(14), N(8) },
                                       { N(10), N(9), N(22), N(12) },
                                       { N(14), N(13), N(30), N(16) } } };
    MtxFx44 left = a;
    MtxFx44 right = b;
    MtxFx44 result;

    MTX_Concat44(&left, &right, &result);
    CheckWords("MTX_Concat44 into a matrix of its own", WORDS(result), WORDS(ab), 16);
    MTX_Concat44(&left, &right, &left);
    CheckWords("MTX_Concat44 into a", WORDS(left), WORDS(ab), 16);
    left = a;
    MTX_Concat44(&left, &right, &right);
    CheckWords("MTX_Concat44 into b", WORDS(right), WORDS(ab), 16);
}

// (1 2 3) through TestConcat43's b: (2 1 6) by its rotation, plus (100 200 300); through TestConcat33's b, (2 1 6)
static void TestMultVec(void) {
    static const MtxFx43 mtx43 = { .m = {
                                       { 0, N(1), 0 }, { N(1), 0, 0 }, { 0, 0, N(2) }, { N(100), N(200), N(300) } } };
    static const MtxFx33 mtx33 = { .m = { { 0, N(1), 0 }, { N(1), 0, 0 }, { 0, 0, N(2) } } };
    static const VecFx32 vec = { N(1), N(2), N(3) };
    static const fx32 by43[] = { N(102), N(201), N(306) };
    static const fx32 by33[] = { N(2), N(1), N(6) };
    VecFx32 in = vec;
    VecFx32 out;

    MTX_MultVec43(&in, &mtx43, &out);
    CheckWords("MTX_MultVec43 into a vector of its own", WORDS(out), by43, 3);
    MTX_MultVec43(&in, &mtx43, &in);
    CheckWords("MTX_MultVec43 in place", WORDS(in), by43, 3);
    in = vec;
    MTX_MultVec33(&in, &mtx33, &out);
    CheckWords("MTX_MultVec33 into a vector of its own", WORDS(out), by33, 3);
    MTX_MultVec33(&in, &mtx33, &in);
    CheckWords("MTX_MultVec33 in place", WORDS(in), by33, 3);
}

// MAT3_RotationEulerZYX multiplies the X, Y and Z rotations into its result in place. The table gives sine 1 and cosine
// 0 for the angle 0x4000 (90 degrees) exactly. Y alone is MTX_RotY33_'s [0 0 -1], [0 1 0], [1 0 0]; X, Y and Z
// together: X's [1 0 0], [0 0 1], [0 -1 0] times Y is [0 0 -1], [1 0 0], [0 -1 0], and that times Z's [0 1 0],
// [-1 0 0], [0 0 1] is [0 0 -1], [0 1 0], [1 0 0]
static void TestEulerRotation(void) {
    static const fx32 rotY[] = { 0, 0, N(-1), 0, N(1), 0, N(1), 0, 0 };
    static const fx32 rotXYZ[] = { 0, 0, N(-1), 0, N(1), 0, N(1), 0, 0 };
    static const fx32 rotXY[] = { 0, 0, N(-1), N(1), 0, 0, 0, N(-1), 0 };
    MtxFx33 mtx;

    MAT3_RotationEulerZYX(0, 0x4000, 0, &mtx);
    CheckWords("MAT3_RotationEulerZYX(0, 90, 0)", WORDS(mtx), rotY, 9);
    MAT3_RotationEulerZYX(0x4000, 0x4000, 0, &mtx);
    CheckWords("MAT3_RotationEulerZYX(90, 90, 0)", WORDS(mtx), rotXY, 9);
    MAT3_RotationEulerZYX(0x4000, 0x4000, 0x4000, &mtx);
    CheckWords("MAT3_RotationEulerZYX(90, 90, 90)", WORDS(mtx), rotXYZ, 9);
}

int main(void) {
    TestConcat33();
    TestConcat43();
    TestConcat44();
    TestMultVec();
    TestEulerRotation();
    if (sFailures != 0) {
        printf("%d checks failed\n", sFailures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
