// MSL's floating-point and integer runtime for NITRO (FP_fastI_v5te_LE.a): the routines that MWCC's code calls for what
// ARMv5TE has no instruction for, which are float and double arithmetic, comparison and conversion, 64-bit
// multiplication, shifts and division, and 32-bit division, and sqrt. The library is hand-written assembly, so every
// routine is an MWCC asm function, in the compiler's pre-UAL syntax (`ldmltia`, `subges`).
//
// The names are the runtime's own, which the compiler's calls use and pret's pokediamond labels (fp_fastI.h gives
// swan's where they differ); the entries in the middle of routines are static here. A double or a 64-bit value has its
// low word in the lower register: r0 and r1 for the first operand and the result, r2 and r3 for the second. Floats go
// in r0 and r1. The routines round to nearest even, deal with denormals, infinities and NaNs (a NaN result is
// 0x7fffffff, or 0x7fffffffffffffff for a double) and raise no exceptions.
//
// An MWCC asm function has one entry, but the runtime enters some routines in the middle and lets others fall into the
// next. The routines are split at those entries, in the ROM's order: a routine that ends without a return falls into
// the function after it, and the parts that nothing outside the file reaches are static.
#include "fp_fastI.h"
#include <errno.h>
#include <math.h>

static void __dadd_start(void);
static void __dsub_start(void);
static void ll_div_body(void);
static void ll_udiv_core(void);
static void ull_div_body(void);
static void _u32_div_not_0_f(void);
static void __fadd_start(void);
static void __fsub_start(void);

// clang-format off
// One step of the unrolled division loops: shifts the next dividend bit into the remainder in r3, subtracts the divisor
// (r1 holds its negative) when it fits, and shifts the quotient bit into r0
#define DIV_STEP \
    adcs r3, r1, r3, lsl #1; \
    sublo r3, r3, r1; \
    adcs r0, r0, r0;
#define DIV_STEP4 DIV_STEP DIV_STEP DIV_STEP DIV_STEP

// clang-format on

// _dadd: a + b. Operands of different signs enter __dsub_start with b negated; operands of the same
// sign fall into __dadd_start.
// clang-format off
asm double _dadd(double a, double b) {
    stmdb sp!, {r4, lr}
    eors r12, r1, r3
    eormi r3, r3, #0x80000000
    bmi __dsub_start
}

// clang-format on

// Adds two doubles of the same sign (the second half of _dadd, also entered by _dsub after it negated b).
// The operands are swapped so that |a| >= |b|, their mantissas are unpacked into 64 bits with the integer
// bit on top, b is shifted right by the exponent difference into a sticky bit, and the sum is renormalized
// and rounded to nearest even. Zero, denormal, infinite and NaN operands leave at the unpacking.
// clang-format off
static asm void __dadd_start(void) {
    // Orders the operands so that a >= b as bit patterns, which is |a| >= |b| for one sign
    subs r12, r0, r2
    sbcs lr, r1, r3
    bhs @unpack
    adds r2, r2, r12
    adc r3, r3, lr
    subs r0, r0, r12
    sbc r1, r1, lr
@unpack:
    // Unpacks a: r12 = its sign and exponent, r1:r0 = its mantissa with the integer bit at bit 63 (lr = 0x80000000).
    // b follows in r4 and r3:r2
    mov lr, #0x80000000
    mov r12, r1, lsr #20
    orr r1, lr, r1, lsl #11
    orr r1, r1, r0, lsr #21
    mov r0, r0, lsl #11
    movs r4, r12, lsl #21
    cmnne r4, #0x200000
    beq @a_special
    mov r4, r3, lsr #20
    orr r3, lr, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs lr, r4, lsl #21
    beq @b_special
@align:
    // r4 = the exponent difference. Shifts b right by it, with a sticky bit in bit 0 for what falls off
    subs r4, r12, r4
    beq @add
    cmp r4, #0x20
    ble @align_short
    cmp r4, #0x38
    movge r4, #0x3f
    sub r4, r4, #0x20
    rsb lr, r4, #0x20
    orrs lr, r2, r3, lsl lr
    mov r2, r3, lsr r4
    orrne r2, r2, #1
    adds r0, r0, r2
    adcs r1, r1, #0
    blo @pack
    b @carry
@align_short:
    rsb lr, r4, #0x20
    movs lr, r2, lsl lr
    rsb lr, r4, #0x20
    mov r2, r2, lsr r4
    orr r2, r2, r3, lsl lr
    mov r3, r3, lsr r4
    orrne r2, r2, #1
@add:
    // Adds the mantissas
    adds r0, r0, r2
    adcs r1, r1, r3
    blo @pack
@carry:
    // The sum carried out: shifts it back in with the carry and raises the exponent, keeping the lost bit as sticky
    add r12, r12, #1
    and r4, r0, #1
    movs r1, r1, rrx
    orr r0, r4, r0, rrx
    mov lr, r12, lsl #21
    cmn lr, #0x200000
    beq @overflow
@pack:
    // Packs the result: the low 11 bits of r0 are the rounding bits, and r12 has the sign and exponent
    movs r2, r0, lsl #21
    mov r0, r0, lsr #11
    orr r0, r0, r1, lsl #21
    add r1, r1, r1
    mov r1, r1, lsr #12
    orr r1, r1, r12, lsl #20
    tst r2, #0x80000000
    ldmeqia sp!, {r4, lr}
    bxeq lr
    movs r2, r2, lsl #1
    andeqs r2, r0, #1
    ldmeqia sp!, {r4, lr}
    bxeq lr
    adds r0, r0, #1
    adc r1, r1, #0
    ldmia sp!, {r4, lr}
    bx lr
@a_special:
    // a's exponent is 0 or 0x7ff; lr = its sign
    cmp r12, #0x800
    movge lr, #0x80000000
    movlt lr, #0
    bics r12, r12, #0x800
    beq @a_denormal
    orrs r4, r0, r1, lsl #1
    bne @nan
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r4, r4, lsl #21
    beq @inf
    cmn r4, #0x200000
    bne @inf
    orrs r4, r2, r3, lsl #1
    beq @inf
    b @nan
@b_special:
    cmp r4, #0x800
    movge lr, #0x80000000
    movlt lr, #0
    bic r12, r12, #0x800
    bics r4, r4, #0x800
    beq @b_denormal
    orrs r4, r2, r3, lsl #1
    bne @nan
    b @inf
@a_denormal:
    orrs r4, r0, r1, lsl #1
    beq @a_zero
    mov r12, #1
    bic r1, r1, #0x80000000
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r4, r4, lsl #21
    cmnne r4, #0x200000
    mov r4, r4, lsr #21
    orr r4, r4, lr, lsr #20
    beq @b_special
    orr r3, r3, #0x80000000
    orr r12, r12, lr, lsr #20
    b @align
@a_zero:
    mov r12, r3, lsr #20
    mov r1, r3, lsl #11
    orr r1, r1, r2, lsr #21
    mov r0, r2, lsl #11
    movs r4, r12, lsl #21
    beq @return_b
    cmn r4, #0x200000
    bne @return_b
    orrs r4, r0, r1, lsl #1
    beq @inf
    b @nan_b
@b_denormal:
    orrs r4, r2, r3, lsl #1
    beq @return_a
    mov r4, #1
    bic r3, r3, #0x80000000
    cmp r1, #0
    bpl @both_denormal
    orr r12, r12, lr, lsr #20
    orr r4, r4, lr, lsr #20
    b @align
@both_denormal:
    adds r0, r0, r2
    adcs r1, r1, r3
    blo @pack_denormal
    add r12, r12, #1
    and r4, r0, #1
    movs r1, r1, rrx
    mov r0, r0, rrx
    orr r0, r0, r4
@pack_denormal:
    cmp r1, #0
    subges r12, r12, #1
    movs r2, r0, lsl #21
    mov r0, r0, lsr #11
    orr r0, r0, r1, lsl #21
    add r1, r1, r1
    orr r1, lr, r1, lsr #12
    orr r1, r1, r12, lsl #20
    ldmeqia sp!, {r4, lr}
    bxeq lr
    tst r2, #0x80000000
    ldmeqia sp!, {r4, lr}
    bxeq lr
    movs r2, r2, lsl #1
    andeqs r2, r0, #1
    ldmeqia sp!, {r4, lr}
    bxeq lr
@return_b:
    mov r1, r3
    mov r0, r2
    ldmia sp!, {r4, lr}
    bx lr
@return_a:
    cmp r1, #0
    subges r12, r12, #1
    mov r0, r0, lsr #11
    orr r0, r0, r1, lsl #21
    add r1, r1, r1
    orr r1, lr, r1, lsr #12
    orr r1, r1, r12, lsl #20
    ldmia sp!, {r4, lr}
    bx lr
@inf:
    ldr r1, =0x7ff00000
    orr r1, lr, r1
    mov r0, #0
    ldmia sp!, {r4, lr}
    bx lr
@nan:
    mov r1, r3
@nan_b:
    mvn r0, #0
    bic r1, r0, #0x80000000
    ldmia sp!, {r4, lr}
    bx lr
    // Unreachable: no branch leads here
    mvn r0, #0
    bic r1, r0, #0x80000000
    ldmia sp!, {r4, lr}
    bx lr
@overflow:
    // The exponent overflowed: the result is an infinity
    cmp r12, #0x800
    movge lr, #0x80000000
    movlt lr, #0
    ldr r1, =0x7ff00000
    orr r1, lr, r1
    mov r0, #0
    ldmia sp!, {r4, lr}
    bx lr
}

// clang-format on

// _drsb, the reverse subtraction b - a: swaps the operands and falls into _dsub.
// clang-format off
asm double _drsb(double a, double b) {
    eor r1, r1, r3
    eor r3, r1, r3
    eor r1, r1, r3
    eor r0, r0, r2
    eor r2, r0, r2
    eor r0, r0, r2
}

// clang-format on

// _dsub: a - b. Operands of the same sign fall into __dsub_start; those of different signs have b negated
// and enter __dadd_start.
// clang-format off
asm double _dsub(double a, double b) {
    stmdb sp!, {r4, lr}
    eors r12, r1, r3
    eormi r3, r3, #0x80000000
    bmi __dadd_start
}

// clang-format on

// Subtracts two doubles of the same sign (the second half of _dsub, also entered by _dadd after it negated
// b). The smaller magnitude is taken from the larger, swapping the operands and the sign when a is the
// smaller; cancellation is normalized with clz and a result below the normal range is made a denormal.
// Equal operands give +0.
// clang-format off
static asm void __dsub_start(void) {
    // Orders the operands by magnitude, as __dadd_start does. When b is the larger the sign of the result flips
    subs r12, r0, r2
    sbcs lr, r1, r3
    bhs @unpack
    eor lr, lr, #0x80000000
    adds r2, r2, r12
    adc r3, r3, lr
    subs r0, r0, r12
    sbc r1, r1, lr
@unpack:
    // Unpacks both operands as __dadd_start does
    mov lr, #0x80000000
    mov r12, r1, lsr #20
    orr r1, lr, r1, lsl #11
    orr r1, r1, r0, lsr #21
    mov r0, r0, lsl #11
    movs r4, r12, lsl #21
    cmnne r4, #0x200000
    beq @a_special
    mov r4, r3, lsr #20
    orr r3, lr, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs lr, r4, lsl #21
    beq @b_special
@align:
    // r4 = the exponent difference. Shifts b right by it, with a sticky bit, and subtracts
    subs r4, r12, r4
    beq @same_exponent
    cmp r4, #0x20
    ble @align_short
    cmp r4, #0x38
    movge r4, #0x3f
    sub r4, r4, #0x20
    rsb lr, r4, #0x20
    orrs lr, r2, r3, lsl lr
    mov r2, r3, lsr r4
    orrne r2, r2, #1
    subs r0, r0, r2
    sbcs r1, r1, #0
    bmi @pack
    b @renormalize
@align_short:
    rsb lr, r4, #0x20
    movs lr, r2, lsl lr
    rsb lr, r4, #0x20
    mov r2, r2, lsr r4
    orr r2, r2, r3, lsl lr
    mov r3, r3, lsr r4
    orrne r2, r2, #1
    subs r0, r0, r2
    sbcs r1, r1, r3
    bpl @renormalize
@pack:
    // Packs the normalized result, rounding to nearest even by the low 11 bits of r0
    movs r2, r0, lsl #21
    mov r0, r0, lsr #11
    orr r0, r0, r1, lsl #21
    add r1, r1, r1
    mov r1, r1, lsr #12
    orr r1, r1, r12, lsl #20
    tst r2, #0x80000000
    ldmeqia sp!, {r4, lr}
    bxeq lr
    movs r2, r2, lsl #1
    andeqs r2, r0, #1
    ldmeqia sp!, {r4, lr}
    bxeq lr
    adds r0, r0, #1
    adc r1, r1, #0
    ldmia sp!, {r4, lr}
    bx lr
@same_exponent:
    // The exponents are equal, so the difference is exact: it is 0 or needs normalizing
    subs r0, r0, r2
    sbc r1, r1, r3
    orrs lr, r1, r0
    beq @zero
    mov lr, r12, lsl #20
    and lr, lr, #0x80000000
    bic r12, r12, #0x800
    cmp r1, #0
    bmi @normalized
    bne @normalize
    sub r12, r12, #0x20
    movs r1, r0
    mov r0, #0
    bmi @denormal_result
@normalize:
    clz r4, r1
    movs r1, r1, lsl r4
    rsb r4, r4, #0x20
    orr r1, r1, r0, lsr r4
    rsb r4, r4, #0x20
    mov r0, r0, lsl r4
    sub r12, r12, r4
@denormal_result:
    cmp r12, #0
    bgt @pack_exact
    rsb r12, r12, #0xc
    cmp r12, #0x20
    movge r0, r1
    movge r1, #0
    subge r12, r12, #0x20
    rsb r4, r12, #0x20
    mov r0, r0, lsr r12
    orr r0, r0, r1, lsl r4
    orr r1, lr, r1, lsr r12
    ldmia sp!, {r4, lr}
    bx lr
@normalized:
    cmp r1, #0
    subges r12, r12, #1
@pack_exact:
    mov r0, r0, lsr #11
    orr r0, r0, r1, lsl #21
    add r1, r1, r1
    orr r1, lr, r1, lsr #12
    orr r1, r1, r12, lsl #20
    ldmia sp!, {r4, lr}
    bx lr
@renormalize:
    // The difference lost leading bits: normalizes it with clz and lowers the exponent
    mov lr, r12, lsl #20
    and lr, lr, #0x80000000
    bic r12, r12, #0x800
    cmp r1, #0
    bne @renormalize_clz
    sub r12, r12, #0x20
    movs r1, r0
    mov r0, #0
    bmi @renormalize_exponent
@renormalize_clz:
    clz r4, r1
    movs r1, r1, lsl r4
    rsb r4, r4, #0x20
    orr r1, r1, r0, lsr r4
    rsb r4, r4, #0x20
    mov r0, r0, lsl r4
    sub r12, r12, r4
@renormalize_exponent:
    cmp r12, #0
    orrgt r12, r12, lr, lsr #20
    bgt @pack
    rsb r12, r12, #0xc
    cmp r12, #0x20
    movge r0, r1
    movge r1, #0
    subge r12, r12, #0x20
    rsb r4, r12, #0x20
    mov r0, r0, lsr r12
    orr r0, r0, r1, lsl r4
    orr r1, lr, r1, lsr r12
    ldmia sp!, {r4, lr}
    bx lr
@a_special:
    cmp r12, #0x800
    movge lr, #0x80000000
    movlt lr, #0
    bics r12, r12, #0x800
    beq @a_denormal
    orrs r4, r0, r1, lsl #1
    bne @nan_a
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r4, r4, lsl #21
    beq @inf
    cmn r4, #0x200000
    bne @inf
    orrs r4, r2, r3, lsl #1
    beq @nan_inf_inf
    b @nan_a
@b_special:
    cmp r4, #0x800
    movge lr, #0x80000000
    movlt lr, #0
    bic r12, r12, #0x800
    bics r4, r4, #0x800
    beq @b_denormal
    orrs r4, r2, r3, lsl #1
    bne @nan_a
    b @inf
@a_denormal:
    orrs r4, r0, r1, lsl #1
    beq @a_zero
    mov r12, #1
    bic r1, r1, #0x80000000
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r4, r4, lsl #21
    cmnne r4, #0x200000
    mov r4, r4, lsr #21
    orr r4, r4, lr, lsr #20
    beq @b_special
    orr r3, r3, #0x80000000
    orr r12, r12, lr, lsr #20
    b @align
@a_zero:
    mov r12, r3, lsr #20
    mov r1, r3, lsl #11
    orr r1, r1, r2, lsr #21
    mov r0, r2, lsl #11
    movs r4, r12, lsl #21
    beq @a_zero_b_exp0
    cmn r4, #0x200000
    bne @return_b
    orrs r4, r0, r1, lsl #1
    bne @nan
    b @inf
@a_zero_b_exp0:
    orrs r4, r0, r1, lsl #1
    beq @zero
    b @return_b
@b_denormal:
    orrs r4, r2, r3, lsl #1
    beq @return_a
    mov r4, #1
    bic r3, r3, #0x80000000
    orr r12, r12, lr, lsr #20
    orr r4, r4, lr, lsr #20
    b @align
@return_b:
    mov r1, r3
    mov r0, r2
    ldmia sp!, {r4, lr}
    bx lr
@return_a:
    cmp r1, #0
    subges r12, r12, #1
    mov r0, r0, lsr #11
    orr r0, r0, r1, lsl #21
    add r1, r1, r1
    orr r1, lr, r1, lsr #12
    orr r1, r1, r12, lsl #20
    ldmia sp!, {r4, lr}
    bx lr
@inf:
    ldr r1, =0x7ff00000
    orr r1, lr, r1
    mov r0, #0
    ldmia sp!, {r4, lr}
    bx lr
@nan_a:
    mov r1, r3
@nan:
    mvn r0, #0
    bic r1, r0, #0x80000000
    ldmia sp!, {r4, lr}
    bx lr
@nan_inf_inf:
    mvn r0, #0
    bic r1, r0, #0x80000000
    ldmia sp!, {r4, lr}
    bx lr
@zero:
    mov r1, #0
    mov r0, #0
    ldmia sp!, {r4, lr}
    bx lr
}

// clang-format on

// _dmul: a * b. The 64-bit mantissas are multiplied with umull into a 128-bit product whose low half only
// feeds the sticky bit. Denormal operands are normalized with clz first, and a product below the normal
// range is shifted down into a denormal with rounding.
// clang-format off
asm double _dmul(double a, double b) {
    // Unpacks a: lr = the sign of the product, r12 = a's exponent, r1:r0 = its mantissa with the integer bit at bit 63
    stmdb sp!, {r4, r5, r6, r7, lr}
    eor lr, r1, r3
    and lr, lr, #0x80000000
    mov r12, r1, lsr #20
    mov r1, r1, lsl #11
    orr r1, r1, r0, lsr #21
    mov r0, r0, lsl #11
    movs r6, r12, lsl #21
    cmnne r6, #0x200000
    beq @a_special
    orr r1, r1, #0x80000000
    bic r12, r12, #0x800
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r5, r4, lsl #21
    cmnne r5, #0x200000
    beq @b_special
    orr r3, r3, #0x80000000
    bic r4, r4, #0x800
@multiply:
    // r12 = the sum of the exponents. The 128-bit product of the mantissas is r1:r0:r4:r5, whose low words fold into
    // a sticky bit in r0
    add r12, r4, r12
    umull r5, r4, r0, r2
    umull r7, r6, r0, r3
    adds r4, r7, r4
    adc r6, r6, #0
    umull r7, r0, r1, r2
    adds r4, r7, r4
    adcs r0, r0, r6
    umull r7, r2, r1, r3
    adc r1, r2, #0
    adds r0, r0, r7
    adc r1, r1, #0
    orrs r4, r4, r5
    orrne r0, r0, #1
    cmp r1, #0
    blt @bias
    sub r12, r12, #1
    adds r0, r0, r0
    adc r1, r1, r1
@bias:
    // Removes the bias and sends results with an exponent of 0 or less to underflow
    add r12, r12, #2
    subs r12, r12, #0x400
    bmi @underflow
    beq @underflow
    mov r6, r12, lsl #20
    cmn r6, #0x100000
    bmi @overflow
    movs r2, r0, lsl #21
    mov r0, r0, lsr #11
    orr r0, r0, r1, lsl #21
    add r1, r1, r1
    orr r1, lr, r1, lsr #12
    orr r1, r1, r12, lsl #20
    ldmeqia sp!, {r4, r5, r6, r7, lr}
    bxeq lr
    tst r2, #0x80000000
    ldmeqia sp!, {r4, r5, r6, r7, lr}
    bxeq lr
    movs r2, r2, lsl #1
    andeqs r2, r0, #1
    ldmeqia sp!, {r4, r5, r6, r7, lr}
    bxeq lr
    adds r0, r0, #1
    adc r1, r1, #0
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@a_special:
    bics r12, r12, #0x800
    beq @a_denormal
    orrs r6, r0, r1, lsl #1
    bne @nan
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r5, r4, lsl #21
    beq @a_inf_b_exp0
    cmn r5, #0x200000
    bne @inf
    orrs r5, r2, r3, lsl #1
    beq @inf
    b @nan
@a_inf_b_exp0:
    orrs r5, r3, r2
    beq @nan_inf_zero
    b @inf
@b_special:
    bics r4, r4, #0x800
    beq @b_denormal
    orrs r6, r2, r3, lsl #1
    bne @nan
    b @inf
@a_denormal:
    orrs r6, r0, r1, lsl #1
    beq @a_zero
    mov r12, #1
    cmp r1, #0
    bne @a_normalize
    sub r12, r12, #0x20
    movs r1, r0
    mov r0, #0
    bmi @unpack_b
@a_normalize:
    clz r6, r1
    movs r1, r1, lsl r6
    rsb r6, r6, #0x20
    orr r1, r1, r0, lsr r6
    rsb r6, r6, #0x20
    mov r0, r0, lsl r6
    sub r12, r12, r6
@unpack_b:
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r5, r4, lsl #21
    cmnne r5, #0x200000
    beq @b_special
    orr r3, r3, #0x80000000
    bic r4, r4, #0x800
    b @multiply
@a_zero:
    mov r4, r3, lsr #20
    mov r3, r3, lsl #11
    orr r3, r3, r2, lsr #21
    mov r2, r2, lsl #11
    movs r5, r4, lsl #21
    beq @zero
    cmn r5, #0x200000
    bne @zero
    orrs r6, r2, r3, lsl #1
    beq @nan_inf_zero
    b @nan
@b_denormal:
    orrs r5, r2, r3, lsl #1
    beq @zero
    mov r4, #1
    cmp r3, #0
    bne @b_normalize
    sub r4, r4, #0x20
    movs r3, r2
    mov r2, #0
    bmi @multiply
@b_normalize:
    clz r6, r3
    movs r3, r3, lsl r6
    rsb r6, r6, #0x20
    orr r3, r3, r2, lsr r6
    rsb r6, r6, #0x20
    mov r2, r2, lsl r6
    sub r4, r4, r6
    b @multiply
@underflow:
    cmn r12, #0x34
    beq @half_min
    bmi @zero_underflow
    mov r2, r1
    mov r3, r0
    add r4, r12, #0x34
    cmp r4, #0x20
    movge r2, r3
    movge r3, #0
    subge r4, r4, #0x20
    rsb r5, r4, #0x20
    mov r2, r2, lsl r4
    orr r2, r2, r3, lsr r5
    movs r3, r3, lsl r4
    orrne r2, r2, #1
    rsb r12, r12, #0xc
    cmp r12, #0x20
    movge r0, r1
    movge r1, #0
    subge r12, r12, #0x20
    rsb r4, r12, #0x20
    mov r0, r0, lsr r12
    orr r0, r0, r1, lsl r4
    orr r1, lr, r1, lsr r12
    cmp r2, #0
    ldmeqia sp!, {r4, r5, r6, r7, lr}
    bxeq lr
    tst r2, #0x80000000
    ldmeqia sp!, {r4, r5, r6, r7, lr}
    bxeq lr
    movs r2, r2, lsl #1
    andeqs r2, r0, #1
    ldmeqia sp!, {r4, r5, r6, r7, lr}
    bxeq lr
    adds r0, r0, #1
    adc r1, r1, #0
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@half_min:
    orr r0, r0, r1, lsl #1
    b @min_denormal
@inf:
    ldr r1, =0x7ff00000
    orr r1, lr, r1
    mov r0, #0
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@nan:
    mov r1, r3
    mvn r0, #0
    bic r1, r0, #0x80000000
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@nan_inf_zero:
    mvn r0, #0
    bic r1, r0, #0x80000000
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@min_denormal:
    movs r2, r0
    mov r1, lr
    mov r0, #0
    addne r0, r0, #1
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@zero_underflow:
    mov r1, lr
    mov r0, #0
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@overflow:
    ldr r1, =0x7ff00000
    orr r1, lr, r1
    mov r0, #0
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
@zero:
    mov r1, lr
    mov r0, #0
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
}

// clang-format on

// _d2f: converts a double to a float, rounding to nearest even. A value too large gives infinity, one too
// small a float denormal or a signed zero.
// clang-format off
asm float _d2f(double x) {
    // r2 = the sign; r12 = the exponent, rebased from 0x3ff to 0x7f at 0x380
    and r2, r1, #0x80000000
    mov r12, r1, lsr #20
    bics r12, r12, #0x800
    beq @zero_or_denormal
    mov r3, r12, lsl #21
    cmn r3, #0x200000
    bhs @inf_or_nan
    subs r12, r12, #0x380
    bls @denormal_result
    cmp r12, #0xff
    bge @overflow
    mov r1, r1, lsl #12
    orr r3, r2, r1, lsr #9
    orr r3, r3, r0, lsr #29
    movs r1, r0, lsl #3
    orr r0, r3, r12, lsl #23
    bxeq lr
    tst r1, #0x80000000
    bxeq lr
    movs r1, r1, lsl #1
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
@inf_or_nan:
    orrs r3, r0, r1, lsl #12
    bne @nan
    mov r0, #0xff000000
    orr r0, r2, r0, lsr #1
    bx lr
@nan:
    mvn r0, #0x80000000
    bx lr
@zero_or_denormal:
    orrs r3, r0, r1, lsl #12
    bne @zero
    mov r0, r2
    bx lr
@denormal_result:
    cmn r12, #0x17
    beq @min_denormal
    bmi @zero
    mov r1, r1, lsl #11
    orr r1, r1, #0x80000000
    mov r3, r1, lsr #8
    orr r3, r3, r0, lsr #29
    rsb r12, r12, #1
    movs r1, r0, lsl #3
    orr r0, r2, r3, lsr r12
    rsb r12, r12, #0x20
    mov r3, r3, lsl r12
    orrne r3, r3, #1
    movs r1, r3
    bxeq lr
    tst r1, #0x80000000
    bxeq lr
    movs r1, r1, lsl #1
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
@min_denormal:
    orr r0, r0, r1, lsl #12
    movs r1, r0
    mov r0, r2
    addne r0, r0, #1
    bx lr
@zero:
    mov r0, r2
    bx lr
@overflow:
    mov r0, #0xff000000
    orr r0, r2, r0, lsr #1
    bx lr
}

// clang-format on

// _dflt: converts an int to a double.
// clang-format off
asm double _dflt(int x) {
    ands r2, r0, #0x80000000
    rsbmi r0, r0, #0
    cmp r0, #0
    mov r1, #0
    bxeq lr
    mov r3, #0x400
    add r3, r3, #0x1e
    clz r12, r0
    movs r0, r0, lsl r12
    sub r3, r3, r12
    movs r1, r0
    mov r0, r1, lsl #21
    add r1, r1, r1
    orr r1, r2, r1, lsr #12
    orr r1, r1, r3, lsl #20
    bx lr
}

// clang-format on

// _dfltu: converts an unsigned int to a double.
// clang-format off
asm double _dfltu(unsigned int x) {
    cmp r0, #0
    mov r1, #0
    bxeq lr
    mov r3, #0x400
    add r3, r3, #0x1e
    bmi @pack
    clz r12, r0
    movs r0, r0, lsl r12
    sub r3, r3, r12
@pack:
    mov r1, r0
    mov r0, r1, lsl #21
    add r1, r1, r1
    mov r1, r1, lsr #12
    orr r1, r1, r3, lsl #20
    bx lr
}

// clang-format on

// _dgeq: a >= b. r0 is 1 or 0, and the answer is also left in the condition flags for the caller to branch
// on, true being hs. Any comparison with a NaN is false. Negative operands compare with the operands
// swapped, and +0 equals -0.
// clang-format off
asm int _dgeq(double a, double b) {
    mov r12, #0x200000
    cmn r12, r1, lsl #1
    bhs @a_inf_or_nan
    cmn r12, r3, lsl #1
    bhs @b_inf_or_nan
@ordered:
    orrs r12, r3, r1
    bmi @negative
    cmp r1, r3
    cmpeq r0, r2
    movhs r0, #1
    movlo r0, #0
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
@negative:
    orr r12, r0, r12, lsl #1
    orrs r12, r12, r2
    moveq r0, #1
    mrs r12, cpsr
    orr r12, r12, #0x20000000
    msr cpsr_f, r12
    bxeq lr
    cmp r3, r1
    cmpeq r2, r0
    movhs r0, #1
    movlo r0, #0
    bx lr
@a_inf_or_nan:
    bne @unordered
    cmp r0, #0
    bhi @unordered
    cmn r12, r3, lsl #1
    blo @ordered
@b_inf_or_nan:
    bne @unordered
    cmp r2, #0
    bhi @unordered
    b @ordered
}

// clang-format on

// _dgr: a > b, true being hi in the flags; see _dgeq.
// clang-format off
asm int _dgr(double a, double b) {
    mov r12, #0x200000
    cmn r12, r1, lsl #1
    bhs @a_inf_or_nan
    cmn r12, r3, lsl #1
    bhs @b_inf_or_nan
@ordered:
    orrs r12, r3, r1
    bmi @negative
    cmp r1, r3
    cmpeq r0, r2
    movhi r0, #1
    movls r0, #0
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
@negative:
    orr r12, r0, r12, lsl #1
    orrs r12, r12, r2
    moveq r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x20000000
    msr cpsr_f, r12
    bxeq lr
    cmp r3, r1
    cmpeq r2, r0
    movhi r0, #1
    movls r0, #0
    bx lr
@a_inf_or_nan:
    bne @unordered
    cmp r0, #0
    bhi @unordered
    cmn r12, r3, lsl #1
    blo @ordered
@b_inf_or_nan:
    bne @unordered
    cmp r2, #0
    bhi @unordered
    b @ordered
}

// clang-format on

// _dleq: a <= b, true being ls in the flags; see _dgeq.
// clang-format off
asm int _dleq(double a, double b) {
    mov r12, #0x200000
    cmn r12, r1, lsl #1
    bhs @a_inf_or_nan
    cmn r12, r3, lsl #1
    bhs @b_inf_or_nan
@ordered:
    orrs r12, r3, r1
    bmi @negative
    cmp r1, r3
    cmpeq r0, r2
    movls r0, #1
    movhi r0, #0
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x40000000
    orr r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
@negative:
    orr r12, r0, r12, lsl #1
    orrs r12, r12, r2
    moveq r0, #1
    bne @negative_compare
    mrs r12, cpsr
    bic r12, r12, #0x20000000
    orr r12, r12, #0x40000000
    msr cpsr_f, r12
    bxeq lr
@negative_compare:
    cmp r3, r1
    cmpeq r2, r0
    movls r0, #1
    movhi r0, #0
    bx lr
@a_inf_or_nan:
    bne @unordered
    cmp r0, #0
    bhi @unordered
    cmn r12, r3, lsl #1
    blo @ordered
@b_inf_or_nan:
    bne @unordered
    cmp r2, #0
    bhi @unordered
    b @ordered
}

// clang-format on

// _dls: a < b, true being lo in the flags; see _dgeq.
// clang-format off
asm int _dls(double a, double b) {
    mov r12, #0x200000
    cmn r12, r1, lsl #1
    bhs @a_inf_or_nan
    cmn r12, r3, lsl #1
    bhs @b_inf_or_nan
@ordered:
    orrs r12, r3, r1
    bmi @negative
    cmp r1, r3
    cmpeq r0, r2
    movlo r0, #1
    movhs r0, #0
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    orr r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
@negative:
    orr r12, r0, r12, lsl #1
    orrs r12, r12, r2
    moveq r0, #0
    bne @negative_compare
    mrs r12, cpsr
    orr r12, r12, #0x20000000
    msr cpsr_f, r12
    bxeq lr
@negative_compare:
    cmp r3, r1
    cmpeq r2, r0
    movlo r0, #1
    movhs r0, #0
    bx lr
@a_inf_or_nan:
    bne @unordered
    cmp r0, #0
    bhi @unordered
    cmn r12, r3, lsl #1
    blo @ordered
@b_inf_or_nan:
    bne @unordered
    cmp r2, #0
    bhi @unordered
    b @ordered
}

// clang-format on

// _deq: a == b, true being eq in the flags; see _dgeq.
// clang-format off
asm int _deq(double a, double b) {
    mov r12, #0x200000
    cmn r12, r1, lsl #1
    bhs @a_inf_or_nan
    cmn r12, r3, lsl #1
    bhs @b_inf_or_nan
@ordered:
    orrs r12, r3, r1
    bmi @negative
    cmp r1, r3
    cmpeq r0, r2
    moveq r0, #1
    movne r0, #0
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x40000000
    msr cpsr_f, r12
    bx lr
@negative:
    orr r12, r0, r12, lsl #1
    orrs r12, r12, r2
    moveq r0, #1
    bxeq lr
    cmp r3, r1
    cmpeq r2, r0
    moveq r0, #1
    movne r0, #0
    bx lr
@a_inf_or_nan:
    bne @unordered
    cmp r0, #0
    bhi @unordered
    cmn r12, r3, lsl #1
    blo @ordered
@b_inf_or_nan:
    bne @unordered
    cmp r2, #0
    bhi @unordered
    b @ordered
}

// clang-format on

// _dneq: a != b, true being ne in the flags; a NaN operand makes it true. See _dgeq.
// clang-format off
asm int _dneq(double a, double b) {
    mov r12, #0x200000
    cmn r12, r1, lsl #1
    bhs @a_inf_or_nan
    cmn r12, r3, lsl #1
    bhs @b_inf_or_nan
@ordered:
    orrs r12, r3, r1
    bmi @negative
    cmp r1, r3
    cmpeq r0, r2
    movne r0, #1
    moveq r0, #0
    bx lr
@unordered:
    mov r0, #1
    mrs r12, cpsr
    bic r12, r12, #0x40000000
    msr cpsr_f, r12
    bx lr
@negative:
    orr r12, r0, r12, lsl #1
    orrs r12, r12, r2
    moveq r0, #0
    bxeq lr
    cmp r3, r1
    cmpeq r2, r0
    movne r0, #1
    moveq r0, #0
    bx lr
@a_inf_or_nan:
    bne @unordered
    cmp r0, #0
    bhi @unordered
    cmn r12, r3, lsl #1
    blo @ordered
@b_inf_or_nan:
    bne @unordered
    cmp r2, #0
    bhi @unordered
    b @ordered
}

// clang-format on

// _fgeq: a >= b for floats. The operands are changed from sign and magnitude to two's complement to compare as
// signed integers. r0 is 1 or 0, and the answer is also in the flags (true being hs); a NaN makes it false.
// clang-format off
asm int _fgeq(float a, float b) {
    mov r3, #0xff000000
    cmp r3, r0, lsl #1
    cmphs r3, r1, lsl #1
    blo @unordered
    cmp r0, #0
    bicmi r0, r0, #0x80000000
    rsbmi r0, r0, #0
    cmp r1, #0
    bicmi r1, r1, #0x80000000
    rsbmi r1, r1, #0
    cmp r0, r1
    movge r0, #1
    movlt r0, #0
    mrs r12, cpsr
    biclt r12, r12, #0x20000000
    orrge r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
}

// clang-format on

// _fgr: a > b for floats, true being hi in the flags; see _fgeq.
// clang-format off
asm int _fgr(float a, float b) {
    mov r3, #0xff000000
    cmp r3, r0, lsl #1
    cmphs r3, r1, lsl #1
    blo @unordered
    cmp r0, #0
    bicmi r0, r0, #0x80000000
    rsbmi r0, r0, #0
    cmp r1, #0
    bicmi r1, r1, #0x80000000
    rsbmi r1, r1, #0
    cmp r0, r1
    movgt r0, #1
    movle r0, #0
    mrs r12, cpsr
    bicle r12, r12, #0x20000000
    orrgt r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
}

// clang-format on

// _fleq: a <= b for floats, true being ls in the flags; see _fgeq.
// clang-format off
asm int _fleq(float a, float b) {
    mov r3, #0xff000000
    cmp r3, r0, lsl #1
    cmphs r3, r1, lsl #1
    blo @unordered
    cmp r0, #0
    bicmi r0, r0, #0x80000000
    rsbmi r0, r0, #0
    cmp r1, #0
    bicmi r1, r1, #0x80000000
    rsbmi r1, r1, #0
    cmp r0, r1
    movle r0, #1
    movgt r0, #0
    mrs r12, cpsr
    orrgt r12, r12, #0x20000000
    bicgt r12, r12, #0x40000000
    bicle r12, r12, #0x20000000
    orrle r12, r12, #0x40000000
    msr cpsr_f, r12
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    bic r12, r12, #0x40000000
    orr r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
}

// clang-format on

// _fls: a < b for floats, true being lo in the flags; see _fgeq.
// clang-format off
asm int _fls(float a, float b) {
    mov r3, #0xff000000
    cmp r3, r0, lsl #1
    cmphs r3, r1, lsl #1
    blo @unordered
    cmp r0, #0
    bicmi r0, r0, #0x80000000
    rsbmi r0, r0, #0
    cmp r1, #0
    bicmi r1, r1, #0x80000000
    rsbmi r1, r1, #0
    cmp r0, r1
    movlt r0, #1
    movge r0, #0
    mrs r12, cpsr
    orrge r12, r12, #0x20000000
    biclt r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
@unordered:
    mov r0, #0
    mrs r12, cpsr
    orr r12, r12, #0x20000000
    msr cpsr_f, r12
    bx lr
}

// clang-format on

// _f2d: converts a float to a double. A float denormal is normalized with clz.
// clang-format off
asm double _f2d(float x) {
    and r2, r0, #0x80000000
    mov r12, r0, lsr #23
    mov r3, r0, lsl #9
    ands r12, r12, #0xff
    beq @zero_or_denormal
    cmp r12, #0xff
    beq @inf_or_nan
@pack:
    add r12, r12, #0x380
    mov r0, r3, lsl #20
    orr r1, r2, r3, lsr #12
    orr r1, r1, r12, lsl #20
    bx lr
@zero_or_denormal:
    cmp r3, #0
    bne @denormal
    mov r1, r2
    mov r0, #0
    bx lr
@denormal:
    mov r3, r3, lsr #1
    clz r12, r3
    movs r3, r3, lsl r12
    rsb r12, r12, #1
    add r3, r3, r3
    b @pack
@inf_or_nan:
    cmp r3, #0
    bhi @nan
    ldr r1, =0x7ff00000
    orr r1, r1, r2
    mov r0, #0
    bx lr
@nan:
    mvn r0, #0
    bic r1, r0, #0x80000000
    bx lr
}

// clang-format on

// _fflt: converts an int to a float, rounding to nearest even.
// clang-format off
asm float _fflt(int x) {
    ands r2, r0, #0x80000000
    rsbmi r0, r0, #0
    cmp r0, #0
    bxeq lr
    clz r3, r0
    movs r0, r0, lsl r3
    rsb r3, r3, #0x9e
    ands r1, r0, #0xff
    add r0, r0, r0
    orr r0, r2, r0, lsr #9
    orr r0, r0, r3, lsl #23
    bxeq lr
    tst r1, #0x80
    bxeq lr
    ands r3, r1, #0x7f
    andeqs r3, r0, #1
    addne r0, r0, #1
    bx lr
}

// clang-format on

// _ffltu: converts an unsigned int to a float, rounding to nearest even.
// clang-format off
asm float _ffltu(unsigned int x) {
    cmp r0, #0
    bxeq lr
    mov r3, #0x9e
    bmi @pack
    clz r12, r0
    movs r0, r0, lsl r12
    sub r3, r3, r12
@pack:
    ands r2, r0, #0xff
    add r0, r0, r0
    mov r0, r0, lsr #9
    orr r0, r0, r3, lsl #23
    bxeq lr
    tst r2, #0x80
    bxeq lr
    ands r1, r2, #0x7f
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
}

// clang-format on

// The signed 64-bit remainder, r1:r0 % r3:r2, with the sign of the dividend. The entries of the
// 64-bit divisions keep their state in r4: its bit 31 asks for the result to be negated and its bit 0
// for the remainder rather than the quotient. This one asks for the remainder, and joins ll_div_body.
// clang-format off
asm long long _ll_mod(long long a, long long b) {
    // r4 = the dividend's sign in bit 31, and bit 0 set: the caller wants the remainder
    stmdb sp!, {r4, r5, r6, r7, r11, r12, lr}
    mov r4, r1
    orr r4, r4, #1
    b ll_div_body
}

// clang-format on

// The signed 64-bit division, r1:r0 / r3:r2: r4 is the sign of the quotient, the operands' signs xor-ed, with bit 0
// clear to ask for the quotient. Falls into ll_div_body.
// clang-format off
asm long long _ll_sdiv(long long a, long long b) {
    // r4 = the sign of the quotient in bit 31, and bit 0 clear: the caller wants the quotient
    stmdb sp!, {r4, r5, r6, r7, r11, r12, lr}
    eor r4, r1, r3
    mov r4, r4, asr #1
    mov r4, r4, lsl #1
}

// clang-format on

// The part of the signed 64-bit division and remainder after their entries. A zero divisor returns the dividend;
// operands that both fit in 32 bits are divided by _s32_div_f; the others are made positive for ll_udiv_core.
// clang-format off
static asm void ll_div_body(void) {
    // A zero divisor returns the dividend
    orrs r5, r3, r2
    bne @divisor_ok
    ldmia sp!, {r4, r5, r6, r7, r11, r12, lr}
    bx lr
@divisor_ok:
    // r5 and r6 are zero when both operands fit in 32 bits, the high word being the sign extension of the low
    mov r5, r0, lsr #31
    add r5, r5, r1
    mov r6, r2, lsr #31
    add r6, r6, r3
    orrs r6, r5, r6
    bne @abs_a
    mov r1, r2
    bl _s32_div_f
    ands r4, r4, #1
    movne r0, r1
    mov r1, r0, asr #31
    ldmia sp!, {r4, r5, r6, r7, r11, r12, lr}
    bx lr
@abs_a:
    cmp r1, #0
    bge @abs_b
    rsbs r0, r0, #0
    rsc r1, r1, #0
@abs_b:
    cmp r3, #0
    bge ll_udiv_core
    rsbs r2, r2, #0
    rsc r3, r3, #0
}

// clang-format on

// The unsigned 64-bit division, a shift-and-subtract loop: the divisor is aligned to the top bit of the dividend
// and each step makes one quotient bit with a non-restoring subtraction or addition. r4 selects the
// remainder or the quotient, and the sign (see _ll_mod). Returns the result in r1:r0.
// clang-format off
static asm void ll_udiv_core(void) {
    // r1:r0 = the dividend and r3:r2 the divisor, both positive; r4 as the entries set it
    orrs r5, r1, r0
    beq @zero
    mov r5, #0
    mov r6, #1
    cmp r3, #0
    bmi @trim_count
@align_divisor:
    // Shifts the divisor left until its top bit is set, counting the shifts in r5. r6 = the quotient's bits, r5 + 1
    add r5, r5, #1
    adds r2, r2, r2
    adcs r3, r3, r3
    bpl @align_divisor
    add r6, r6, r5
@trim_count:
    // Drops the quotient bits that the dividend's leading zeros make needless, shifting the dividend left instead
    cmp r1, #0
    blt @divide
@trim_loop:
    cmp r6, #1
    beq @divide
    sub r6, r6, #1
    adds r0, r0, r0
    adcs r1, r1, r1
    bpl @trim_loop
@divide:
    // r12:r11 = the quotient being built, r7 = the bits of the partial remainder above r1:r0
    mov r7, #0
    mov r12, #0
    mov r11, #0
    b @subtract
@quotient_bit:
    orr r12, r12, #1
    subs r6, r6, #1
    beq @finish
    adds r0, r0, r0
    adcs r1, r1, r1
    adcs r7, r7, r7
@subtract:
    subs r0, r0, r2
    sbcs r1, r1, r3
    sbcs r7, r7, #0
    adds r12, r12, r12
    adc r11, r11, r11
    cmp r7, #0
    bge @quotient_bit
@add_back:
    subs r6, r6, #1
    beq @restore
    adds r0, r0, r0
    adcs r1, r1, r1
    adc r7, r7, r7
    adds r0, r0, r2
    adcs r1, r1, r3
    adc r7, r7, #0
    adds r12, r12, r12
    adc r11, r11, r11
    cmp r7, #0
    bge @quotient_bit
    b @add_back
@restore:
    adds r0, r0, r2
    adc r1, r1, r3
@finish:
    // For the remainder: shifts the partial remainder back down by r5, the divisor's alignment
    ands r7, r4, #1
    moveq r0, r12
    moveq r1, r11
    beq @sign
    subs r7, r5, #0x20
    movge r0, r1, lsr r7
    bge @zero_high
    rsb r7, r5, #0x20
    mov r0, r0, lsr r5
    orr r0, r0, r1, lsl r7
    mov r1, r1, lsr r5
    b @sign
    // Unreachable: no branch leads here
    mov r0, r1, lsr r7
    mov r1, #0
@sign:
    cmp r4, #0
    blt @negate
    ldmia sp!, {r4, r5, r6, r7, r11, r12, lr}
    bx lr
@negate:
    rsbs r0, r0, #0
    rsc r1, r1, #0
    ldmia sp!, {r4, r5, r6, r7, r11, r12, lr}
    bx lr
@zero:
    mov r0, #0
@zero_high:
    mov r1, #0
    cmp r4, #0
    blt @negate
    ldmia sp!, {r4, r5, r6, r7, r11, r12, lr}
    bx lr
}

// clang-format on

// The unsigned 64-bit division (MWCC's _ll_udiv), r1:r0 / r3:r2. r4 = 0 asks for the quotient; joins
// ull_div_body.
// clang-format off
asm unsigned long long _ll_udiv(unsigned long long a, unsigned long long b) {
    // r4 = 0: the caller wants the quotient
    stmdb sp!, {r4, r5, r6, r7, r11, r12, lr}
    mov r4, #0
    b ull_div_body
}

// clang-format on

// The unsigned 64-bit remainder: r4 = 1 asks for the remainder. Falls into ull_div_body.
// clang-format off
asm unsigned long long _ull_mod(unsigned long long a, unsigned long long b) {
    // r4 = 1: the caller wants the remainder
    stmdb sp!, {r4, r5, r6, r7, r11, r12, lr}
    mov r4, #1
}

// clang-format on

// The part of the unsigned 64-bit division and remainder after their entries. A zero divisor returns the
// dividend; operands whose high words are both zero are divided by _u32_div_not_0_f, the others by
// ll_udiv_core.
// clang-format off
static asm void ull_div_body(void) {
    orrs r5, r3, r2
    bne @divisor_ok
    ldmia sp!, {r4, r5, r6, r7, r11, r12, lr}
    bx lr
@divisor_ok:
    orrs r5, r1, r3
    bne ll_udiv_core
    mov r1, r2
    bl _u32_div_not_0_f
    cmp r4, #0
    movne r0, r1
    mov r1, #0
    ldmia sp!, {r4, r5, r6, r7, r11, r12, lr}
    bx lr
}

// clang-format on

// _ll_mul (the runtime has one multiplication for signed and unsigned values): the low 64 bits of r1:r0 * r3:r2.
// clang-format off
asm long long _ull_mul(long long a, long long b) {
    stmdb sp!, {r4, r5, lr}
    umull r5, r4, r0, r2
    mla r4, r0, r3, r4
    mla r4, r2, r1, r4
    mov r1, r4
    mov r0, r5
    ldmia sp!, {r4, r5, lr}
    bx lr
}

// clang-format on

// _ll_shl: shifts r1:r0 left by r2 & 63.
// clang-format off
asm long long _ll_shl(long long a, int shift) {
    ands r2, r2, #0x3f
    bxeq lr
    subs r3, r2, #0x20
    bge @shift_word
    rsb r3, r2, #0x20
    mov r1, r1, lsl r2
    orr r1, r1, r0, lsr r3
    mov r0, r0, lsl r2
    bx lr
@shift_word:
    mov r1, r0, lsl r3
    mov r0, #0
    bx lr
}

// clang-format on

// The signed 32-bit division, r0 / r1: the quotient in r0 and the remainder, with the dividend's sign, in r1. The
// magnitudes are divided by a restoring loop unrolled 32 times, entered by a computed jump to the step for the
// size of the quotient. Dividing by zero returns the dividend and a zero remainder.
// clang-format off
asm int _s32_div_f(int a, int b) {
    // r12 = the sign of the quotient in bit 31 and, in bit 0, whether the dividend was negative: the remainder takes its sign
    eor r12, r0, r1
    and r12, r12, #0x80000000
    cmp r0, #0
    rsblt r0, r0, #0
    addlt r12, r12, #1
    cmp r1, #0
    rsblt r1, r1, #0
    beq @fix_signs
    cmp r0, r1
    movlo r1, r0
    movlo r0, #0
    blo @fix_signs
    // Sizes the quotient in steps of 16, 8 and 4 bits: r2 = 28, less 16, 8 and 4 where the divisor is small enough to
    // leave the quotient that many more bits. The dividend is shifted left by r2 to put its first bit on top, and the
    // unrolled steps do the 32 less the r2 skipped ones
    mov r2, #0x1c
    mov r3, r0, lsr #4
    cmp r1, r3, lsr #12
    suble r2, r2, #0x10
    movle r3, r3, lsr #16
    cmp r1, r3, lsr #4
    suble r2, r2, #8
    movle r3, r3, lsr #8
    cmp r1, r3
    suble r2, r2, #4
    movle r3, r3, lsr #4
    mov r0, r0, lsl r2
    rsb r1, r1, #0
    adds r0, r0, r0
    add r2, r2, r2, lsl #1
    // Skips r2 of the unrolled steps below: each is three instructions of 4 bytes, and pc reads 8 bytes ahead,
    // past the nop after this instruction
    add pc, pc, r2, lsl #2
    mov r0, r0
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    mov r1, r3
@fix_signs:
    ands r3, r12, #0x80000000
    rsbne r0, r0, #0
    ands r3, r12, #1
    rsbne r1, r1, #0
    bx lr
}

// clang-format on

// _u32_div_f: the unsigned 32-bit division, r0 / r1: the quotient in r0 and the remainder in r1. A zero
// divisor returns the dividend and a zero remainder at once; the division is _u32_div_not_0_f.
// clang-format off
asm unsigned int _u32_div_f(unsigned int a, unsigned int b) {
    cmp r1, #0
    bxeq lr
}

// clang-format on

// The division of _u32_div_f once the divisor is known not to be zero, which the 32-bit case of _ll_udiv
// calls directly: the unrolled loop of _s32_div_f without its sign handling.
// clang-format off
static asm void _u32_div_not_0_f(void) {
    cmp r0, r1
    movlo r1, r0
    movlo r0, #0
    bxlo lr
    // Sizes the quotient in steps of 16, 8 and 4 bits: r2 = 28, less 16, 8 and 4 where the divisor is small enough to
    // leave the quotient that many more bits. The dividend is shifted left by r2 to put its first bit on top, and the
    // unrolled steps do the 32 less the r2 skipped ones
    mov r2, #0x1c
    mov r3, r0, lsr #4
    cmp r1, r3, lsr #12
    suble r2, r2, #0x10
    movle r3, r3, lsr #16
    cmp r1, r3, lsr #4
    suble r2, r2, #8
    movle r3, r3, lsr #8
    cmp r1, r3
    suble r2, r2, #4
    movle r3, r3, lsr #4
    mov r0, r0, lsl r2
    rsb r1, r1, #0
    adds r0, r0, r0
    add r2, r2, r2, lsl #1
    // Skips r2 of the unrolled steps below: each is three instructions of 4 bytes, and pc reads 8 bytes ahead,
    // past the nop after this instruction
    add pc, pc, r2, lsl #2
    mov r0, r0
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    DIV_STEP4
    mov r1, r3
    bx lr
}

// clang-format on

// _ffix: converts a float to an int, truncating. A value out of range saturates by its sign, and so does a NaN.
// clang-format off
asm int _ffix(float x) {
    bic r1, r0, #0x80000000
    // r2 = 0x9e less the exponent: the shift that makes an integer of the mantissa, 0x9e being the exponent of 2^31
    mov r2, #0x9e
    subs r2, r2, r1, lsr #23
    ble @saturate
    mov r1, r1, lsl #8
    orr r1, r1, #0x80000000
    cmp r0, #0
    mov r0, r1, lsr r2
    rsbmi r0, r0, #0
    bx lr
@saturate:
    mvn r0, r0, asr #31
    add r0, r0, #0x80000000
    bx lr
}

// clang-format on

// _ffixu: converts a float to an unsigned int, truncating. A negative value gives 0 (a negative NaN
// 0xffffffff) and one too large 0xffffffff.
// clang-format off
asm unsigned int _ffixu(float x) {
    tst r0, #0x80000000
    bne @negative
    mov r1, #0x9e
    subs r1, r1, r0, lsr #23
    blt @overflow
    mov r2, r0, lsl #8
    orr r0, r2, #0x80000000
    mov r0, r0, lsr r1
    bx lr
@negative:
    mov r2, #0xff000000
    cmp r2, r0, lsl #1
    movhs r0, #0
    mvnlo r0, #0
    bx lr
@overflow:
    mvn r0, #0
    bx lr
}

// clang-format on

// _dfix: converts a double to an int, truncating. A value out of range saturates by its sign, and so does
// a NaN.
// clang-format off
asm int _dfix(double x) {
    bic r3, r1, #0x80000000
    // 0x41e is the exponent of 2^31
    ldr r2, =0x41e
    subs r2, r2, r3, lsr #20
    ble @saturate
    cmp r2, #0x20
    bge @zero
    mov r3, r1, lsl #11
    orr r3, r3, #0x80000000
    orr r3, r3, r0, lsr #21
    cmp r1, #0
    mov r0, r3, lsr r2
    rsbmi r0, r0, #0
    bx lr
@zero:
    mov r0, #0
    bx lr
@saturate:
    mvn r0, r1, asr #31
    add r0, r0, #0x80000000
    bx lr
}

// clang-format on

// Converts a double to an unsigned 64-bit integer, truncating. A negative value gives 0 (a negative NaN
// all ones) and one too large all ones.
// clang-format off
asm unsigned long long _ll_ufrom_d(double x) {
    tst r1, #0x80000000
    // 0x43e is the exponent of 2^63
    bne @negative
    ldr r2, =0x43e
    subs r2, r2, r1, lsr #20
    blt @overflow
    cmp r2, #0x40
    bge @zero
    mov r12, r1, lsl #11
    orr r12, r12, #0x80000000
    orr r12, r12, r0, lsr #21
    cmp r2, #0x20
    ble @wide
    sub r2, r2, #0x20
    mov r1, #0
    mov r0, r12, lsr r2
    bx lr
@wide:
    mov r3, r0, lsl #11
    mov r1, r12, lsr r2
    mov r0, r3, lsr r2
    rsb r2, r2, #0x20
    orr r0, r0, r12, lsl r2
    bx lr
@zero:
    mov r1, #0
    mov r0, #0
    bx lr
@negative:
    cmn r1, #0x100000
    cmpeq r0, #0
    bhi @overflow
    mov r1, #0
    mov r0, #0
    bx lr
@overflow:
    mvn r1, #0
    mvn r0, #0
    bx lr
}

// clang-format on

// sqrt: the square root of a double, found bit by bit as two 32-bit words. A negative argument or NaN is a domain
// error: errno is set to EDOM and the result is a NaN; sqrt(-0) is -0 and sqrt(+inf) is +inf.
// clang-format off
asm double sqrt(double x) {
    stmdb sp!, {r4, r5, r6, lr}
    ldr r2, =0x7ff00000
    // A high word of 0x7ff00000 or more is a negative number, an infinity or a NaN
    cmp r1, r2
    bhs @negative_or_special
    movs r12, r1, lsr #20
    beq @denormal
    bic r1, r1, r2
    orr r1, r1, #0x100000
@halve_exponent:
    // Halves the exponent; an unbiased exponent that was odd doubles the mantissa instead
    movs r12, r12, asr #1
    bhs @root_high
    sub r12, r12, #1
    movs r0, r0, lsl #1
    adc r1, r1, r1
@root_high:
    // Takes the root a bit at a time, the high word first: r2 = the root so far, r1 = the remainder, lr = the bit tried
    movs r3, r0, lsl #1
    adc r1, r1, r1
    mov r2, #0
    mov r4, #0
    mov lr, #0x200000
@root_high_loop:
    add r6, r4, lr
    cmp r6, r1
    addle r4, r6, lr
    suble r1, r1, r6
    addle r2, r2, lr
    movs r3, r3, lsl #1
    adc r1, r1, r1
    movs lr, lr, lsr #1
    bne @root_high_loop
    mov r0, #0
    mov r5, #0
    cmp r1, r4
    cmpeq r3, #0x80000000
    blo @root_low
    subs r3, r3, #0x80000000
    sbc r1, r1, r4
    add r4, r4, #1
    mov r0, #0x80000000
@root_low:
    // The same for the low word
    movs r3, r3, lsl #1
    adc r1, r1, r1
    mov lr, #0x40000000
@root_low_loop:
    add r6, r5, lr
    cmp r4, r1
    cmpeq r6, r3
    bhi @root_low_next
    add r5, r6, lr
    subs r3, r3, r6
    sbc r1, r1, r4
    add r0, r0, lr
@root_low_next:
    movs r3, r3, lsl #1
    adc r1, r1, r1
    movs lr, lr, lsr #1
    bne @root_low_loop
    orrs r1, r1, r3
    biceq r0, r0, #1
    movs r1, r2, lsr #1
    movs r0, r0, rrx
    adcs r0, r0, #0
    adc r1, r1, #0
    add r1, r1, #0x20000000
    sub r1, r1, #0x100000
    add r1, r1, r12, lsl #20
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@denormal:
    // The argument is a denormal: normalizes it with clz
    cmp r1, #0
    bne @denormal_high
    cmp r0, #0
    ldmeqia sp!, {r4, r5, r6, lr}
    bxeq lr
    mvn r12, #0x13
    clz r5, r0
    movs r0, r0, lsl r5
    sub r12, r12, r5
    mov r1, r0, lsr #11
    mov r0, r0, lsl #21
    b @halve_exponent
@denormal_high:
    clz r2, r1
    movs r1, r1, lsl r2
    rsb r2, r2, #0x2b
    mov r1, r1, lsr #11
    orr r1, r1, r0, lsr r2
    rsb r2, r2, #0x20
    mov r0, r0, lsl r2
    rsb r12, r2, #1
    b @halve_exponent
@negative_or_special:
    tst r1, #0x80000000
    beq @inf_or_nan
    bics r3, r1, #0x80000000
    cmpeq r0, #0
    ldmeqia sp!, {r4, r5, r6, lr}
    bxeq lr
    b @domain_error
@inf_or_nan:
    orrs r2, r0, r1, lsl #12
    ldmeqia sp!, {r4, r5, r6, lr}
    bxeq lr
@domain_error:
    // Sets errno to EDOM and returns a quiet NaN
    ldr r2, =0x7ff80000
    orr r1, r1, r2
    ldr r3, =errno
    mov r4, #0x21
    str r4, [r3]
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

// clang-format on

// _fmul: a * b for floats: the mantissas are multiplied with umull, a product below the normal range is made a
// denormal, and the result is rounded to nearest even. Denormals are normalized first with clz.
// clang-format off
asm float _fmul(float a, float b) {
    eor r2, r0, r1
    and r2, r2, #0x80000000
    mov r12, #0xff
    ands r3, r12, r0, lsr #23
    mov r0, r0, lsl #8
    cmpne r3, #0xff
    beq @a_special
    orr r0, r0, #0x80000000
    ands r12, r12, r1, lsr #23
    mov r1, r1, lsl #8
    cmpne r12, #0xff
    beq @b_special
    orr r1, r1, #0x80000000
@multiply:
    add r12, r3, r12
    umull r1, r3, r0, r1
    movs r0, r3
    addpl r0, r0, r0
    subpl r12, r12, #1
    subs r12, r12, #0x7f
    bmi @underflow
    cmp r12, #0xfe
    bge @overflow
    ands r3, r0, #0xff
    orr r0, r2, r0, lsr #8
    add r0, r0, r12, lsl #23
    tst r3, #0x80
    bxeq lr
    orrs r1, r1, r3, lsl #25
    andeqs r3, r0, #1
    addne r0, r0, #1
    bx lr
@a_special:
    cmp r3, #0
    beq @a_denormal
    movs r0, r0, lsl #1
    bne @nan
    mov r12, r1, lsr #23
    mov r1, r1, lsl #9
    ands r12, r12, #0xff
    beq @a_inf_b_exp0
    cmp r12, #0xff
    blt @inf
    cmp r1, #0
    beq @inf
    b @nan
@a_inf_b_exp0:
    cmp r1, #0
    beq @nan_inf_zero
    b @inf
@b_special:
    cmp r12, #0
    beq @b_denormal
@b_inf_or_nan:
    movs r1, r1, lsl #1
    bne @nan
    b @inf
@a_denormal:
    movs r0, r0, lsl #1
    beq @a_zero
    mov r0, r0, lsr #1
    clz r3, r0
    movs r0, r0, lsl r3
    rsb r3, r3, #1
    mov r12, r1, lsr #23
    mov r1, r1, lsl #8
    ands r12, r12, #0xff
    beq @b_denormal
    cmp r12, #0xff
    beq @b_inf_or_nan
    orr r1, r1, #0x80000000
    b @multiply
@a_zero:
    mov r12, r1, lsr #23
    mov r1, r1, lsl #9
    ands r12, r12, #0xff
    beq @zero
    cmp r12, #0xff
    blt @zero
    cmp r1, #0
    beq @nan_inf_zero
    b @nan
@b_denormal:
    movs r1, r1, lsl #1
    beq @zero
    mov r1, r1, lsr #1
    clz r12, r1
    movs r1, r1, lsl r12
    rsb r12, r12, #1
    b @multiply
@underflow:
    cmn r12, #0x18
    beq @half_min
    bmi @zero_underflow
    cmp r1, #0
    orrne r0, r0, #1
    mov r3, r0
    mov r0, r0, lsr #8
    rsb r12, r12, #0
    orr r0, r2, r0, lsr r12
    rsb r12, r12, #0x18
    movs r1, r3, lsl r12
    bxeq lr
    tst r1, #0x80000000
    bxeq lr
    movs r1, r1, lsl #1
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
@half_min:
    mov r0, r0, lsl #1
    b @min_denormal
@inf:
    mov r0, #0xff000000
    orr r0, r2, r0, lsr #1
    bx lr
@nan:
    mvn r0, #0x80000000
    bx lr
@nan_inf_zero:
    mvn r0, #0x80000000
    bx lr
@overflow:
    mov r0, #0xff000000
    orr r0, r2, r0, lsr #1
    bx lr
@min_denormal:
    movs r1, r0
    mov r0, r2
    addne r0, r0, #1
    bx lr
@zero_underflow:
    mov r0, r2
    bx lr
@zero:
    mov r0, r2
    bx lr
}

// clang-format on

// _fadd: a + b for floats. Operands of different signs enter __fsub_start with b negated; operands of the same
// sign fall into __fadd_start.
// clang-format off
asm float _fadd(float a, float b) {
    eors r2, r0, r1
    eormi r1, r1, #0x80000000
    bmi __fsub_start
}

// clang-format on

// Adds two floats of the same sign: the operands are swapped so that |a| >= |b|, b is shifted right into a sticky
// bit by the exponent difference, and the sum is renormalized and rounded to nearest even.
// clang-format off
static asm void __fadd_start(void) {
    subs r12, r0, r1
    sublo r0, r0, r12
    addlo r1, r1, r12
    mov r2, #0x80000000
    mov r3, r0, lsr #23
    orr r0, r2, r0, lsl #8
    ands r12, r3, #0xff
    cmpne r12, #0xff
    beq @a_special
    mov r12, r1, lsr #23
    orr r1, r2, r1, lsl #8
    ands r2, r12, #0xff
    beq @b_special
@align:
    subs r12, r3, r12
    beq @add
    rsb r2, r12, #0x20
    movs r2, r1, lsl r2
    mov r1, r1, lsr r12
    orrne r1, r1, #1
@add:
    adds r0, r0, r1
    blo @pack
    and r1, r0, #1
    orr r0, r1, r0, rrx
    add r3, r3, #1
    and r2, r3, #0xff
    cmp r2, #0xff
    beq @overflow
@pack:
    ands r1, r0, #0xff
    add r0, r0, r0
    mov r0, r0, lsr #9
    orr r0, r0, r3, lsl #23
    tst r1, #0x80
    bxeq lr
    ands r1, r1, #0x7f
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
@a_special:
    cmp r3, #0x100
    movge r2, #0x80000000
    movlt r2, #0
    ands r3, r3, #0xff
    beq @a_denormal
    movs r0, r0, lsl #1
    bne @nan
    mov r12, r1, lsr #23
    mov r1, r1, lsl #9
    ands r12, r12, #0xff
    beq @inf
    cmp r12, #0xff
    blt @inf
    cmp r1, #0
    beq @inf
    b @nan
@b_special:
    cmp r3, #0x100
    movge r2, #0x80000000
    movlt r2, #0
    and r3, r3, #0xff
    ands r12, r12, #0xff
    beq @b_denormal
@b_inf_or_nan:
    movs r1, r1, lsl #1
    bne @nan
    b @inf
@a_denormal:
    movs r0, r0, lsl #1
    beq @a_zero
    mov r3, #1
    mov r0, r0, lsr #1
    mov r12, r1, lsr #23
    mov r1, r1, lsl #8
    ands r12, r12, #0xff
    beq @b_denormal
    cmp r12, #0xff
    beq @b_inf_or_nan
    orr r1, r1, #0x80000000
    orr r3, r3, r2, lsr #23
    orr r12, r12, r2, lsr #23
    b @align
@a_zero:
    mov r3, r1, lsr #23
    mov r0, r1, lsl #9
    ands r3, r3, #0xff
    beq @return_b
    cmp r3, #0xff
    blt @return_b
    cmp r0, #0
    beq @inf
    b @nan_b
@b_denormal:
    movs r1, r1, lsl #1
    beq @return_a
    mov r1, r1, lsr #1
    mov r12, #1
    orr r3, r3, r2, lsr #23
    orr r12, r12, r2, lsr #23
    cmp r0, #0
    bmi @align
    adds r0, r0, r1
    blo @pack_denormal
    and r1, r0, #1
    orr r0, r1, r0, rrx
    add r12, r12, #1
@pack_denormal:
    cmp r0, #0
    subge r12, r12, #1
    ands r1, r0, #0xff
    add r0, r0, r0
    mov r0, r0, lsr #9
    orr r0, r0, r12, lsl #23
    bxeq lr
    tst r1, #0x80
    bxeq lr
    ands r1, r1, #0x7f
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
@return_b:
    mov r0, r1
    bx lr
@return_a:
    cmp r0, #0
    subges r3, r3, #1
    add r0, r0, r0
    orr r0, r2, r0, lsr #9
    orr r0, r0, r3, lsl #23
    bx lr
@overflow:
    cmp r3, #0x100
    movge r2, #0x80000000
    movlt r2, #0
    mov r0, #0xff000000
    orr r0, r2, r0, lsr #1
    bx lr
@nan_b:
    mvn r0, #0x80000000
    bx lr
@inf:
    mov r0, #0xff000000
    orr r0, r2, r0, lsr #1
    bx lr
@nan:
    mvn r0, #0x80000000
    bx lr
}

// clang-format on

// Unreachable: a second copy of the NaN return above. dsd took it for a function.
// clang-format off
asm void func_0208e130(void) {
    mvn r0, #0x80000000
    bx lr
}

// clang-format on

// _frsb, the reverse subtraction b - a: swaps the operands and falls into _fsub.
// clang-format off
asm float _frsb(float a, float b) {
    eor r0, r0, r1
    eor r1, r0, r1
    eor r0, r0, r1
}

// clang-format on

// _fsub: a - b for floats. Operands of the same sign fall into __fsub_start; those of different signs have b
// negated and enter __fadd_start.
// clang-format off
asm float _fsub(float a, float b) {
    eors r2, r0, r1
    eormi r1, r1, #0x80000000
    bmi __fadd_start
}

// clang-format on

// Subtracts two floats of the same sign: the smaller magnitude is taken from the larger, swapping the
// operands and the sign when a is the smaller, and cancellation is normalized with clz.
// clang-format off
static asm void __fsub_start(void) {
    subs r12, r0, r1
    eorlo r12, r12, #0x80000000
    sublo r0, r0, r12
    addlo r1, r1, r12
    mov r2, #0x80000000
    mov r3, r0, lsr #23
    orr r0, r2, r0, lsl #8
    ands r12, r3, #0xff
    cmpne r12, #0xff
    beq @a_special
    mov r12, r1, lsr #23
    orr r1, r2, r1, lsl #8
    ands r2, r12, #0xff
    beq @b_special
@align:
    subs r12, r3, r12
    beq @same_exponent
    rsb r2, r12, #0x20
    movs r2, r1, lsl r2
    mov r1, r1, lsr r12
    orrne r1, r1, #1
    subs r0, r0, r1
    bpl @renormalize
    ands r1, r0, #0xff
    add r0, r0, r0
    mov r0, r0, lsr #9
    orr r0, r0, r3, lsl #23
    tst r1, #0x80
    bxeq lr
    ands r1, r1, #0x7f
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
@same_exponent:
    subs r0, r0, r1
    beq @zero
    mov r2, r3, lsl #23
    and r2, r2, #0x80000000
    bic r3, r3, #0x100
    clz r12, r0
    movs r0, r0, lsl r12
    sub r3, r3, r12
    cmp r3, #0
    bgt @pack_exact
    rsb r3, r3, #9
    orr r0, r2, r0, lsr r3
    bx lr
@pack_exact:
    add r0, r0, r0
    orr r0, r2, r0, lsr #9
    orr r0, r0, r3, lsl #23
    bx lr
@renormalize:
    mov r2, r3, lsl #23
    and r2, r2, #0x80000000
    bic r3, r3, #0x100
    clz r12, r0
    movs r0, r0, lsl r12
    sub r3, r3, r12
    cmp r3, #0
    bgt @pack
    rsb r3, r3, #9
    orr r0, r2, r0, lsr r3
    bx lr
@pack:
    ands r1, r0, #0xff
    add r0, r0, r0
    orr r0, r2, r0, lsr #9
    orr r0, r0, r3, lsl #23
    bxeq lr
    tst r1, #0x80
    bxeq lr
    ands r1, r1, #0x7f
    andeqs r1, r0, #1
    addne r0, r0, #1
    bx lr
@a_special:
    cmp r3, #0x100
    movge r2, #0x80000000
    movlt r2, #0
    ands r3, r3, #0xff
    beq @a_denormal
    movs r0, r0, lsl #1
    bne @nan
    mov r12, r1, lsr #23
    mov r1, r1, lsl #9
    ands r12, r12, #0xff
    beq @inf
    cmp r12, #0xff
    blt @inf
    cmp r1, #0
    beq @nan_inf_inf
    b @nan
@b_special:
    cmp r12, #0x100
    movge r2, #0x80000000
    movlt r2, #0
    and r3, r3, #0xff
    ands r12, r12, #0xff
    beq @b_denormal
@b_inf_or_nan:
    eor r2, r2, #0x80000000
    movs r1, r1, lsl #1
    bne @nan
    b @inf
@a_denormal:
    movs r0, r0, lsl #1
    beq @a_zero
    mov r0, r0, lsr #1
    mov r3, #1
    mov r12, r1, lsr #23
    mov r1, r1, lsl #8
    ands r12, r12, #0xff
    beq @b_denormal
    cmp r12, #0xff
    beq @b_inf_or_nan
    orr r1, r1, #0x80000000
    orr r3, r3, r2, lsr #23
    orr r12, r12, r2, lsr #23
    b @align
@a_zero:
    mov r3, r1, lsr #23
    mov r0, r1, lsl #9
    ands r2, r3, #0xff
    beq @a_zero_b_exp0
    cmp r2, #0xff
    blt @return_b
    cmp r0, #0
    bne @nan_b
    b @inf
@a_zero_b_exp0:
    cmp r0, #0
    beq @zero
    b @return_b
@b_denormal:
    movs r1, r1, lsl #1
    beq @return_a
    mov r1, r1, lsr #1
    mov r12, #1
    orr r12, r12, r2, lsr #23
    orr r3, r3, r2, lsr #23
    b @align
@return_b:
    mov r0, r1
    bx lr
@return_a:
    cmp r0, #0
    subges r3, r3, #1
    add r0, r0, r0
    orr r0, r2, r0, lsr #9
    orr r0, r0, r3, lsl #23
    bx lr
@zero:
    mov r0, #0
    bx lr
    // Unreachable: no branch leads here
    cmp r0, #0
    subges r3, r3, #1
    add r0, r0, r0
    mov r0, r0, lsr #9
    orr r0, r0, r3, lsl #23
    bx lr
@nan_b:
    mvn r0, #0x80000000
    bx lr
@inf:
    mov r0, #0xff000000
    orr r0, r2, r0, lsr #1
    bx lr
@nan:
    mvn r0, #0x80000000
    bx lr
@nan_inf_inf:
    mvn r0, #0x80000000
    bx lr
}

// clang-format on

// _frdiv, the reverse division b / a: swaps the operands and falls into _fdiv.
// clang-format off
asm float _frdiv(float a, float b) {
    eor r0, r0, r1
    eor r1, r0, r1
    eor r0, r0, r1
}

// clang-format on

// _fdiv: a / b for floats. The divisor's top mantissa bits index a 256-byte reciprocal table in the middle of the
// routine; two multiply steps refine it into a quotient estimate, and the remainder decides the final rounding.
// clang-format off
asm float _fdiv(float a, float b) {
    stmdb sp!, {lr}
    mov r12, #0xff
    ands r3, r12, r0, lsr #23
    cmpne r3, #0xff
    beq @a_special
    ands r12, r12, r1, lsr #23
    cmpne r12, #0xff
    beq @b_special
    orr r1, r1, #0x800000
    orr r0, r0, #0x800000
    bic r2, r0, #0xff000000
    bic lr, r1, #0xff000000
@divide:
    cmp r2, lr
    movlo r2, r2, lsl #1
    sublo r3, r3, #1
    // N = the signs differ: the sign of the quotient, which orrmi puts in the result
    teq r0, r1
    // r0 = the reciprocal table less 0x100 entries: the divisor's mantissa bits 22 to 15 and its integer bit index it
    sub r0, pc, #0x94
    ldrb r1, [r0, lr, lsr #15]
    rsb lr, lr, #0
    mov r0, lr, asr #1
    mul r0, r1, r0
    add r0, r0, #0x80000000
    mov r0, r0, lsr #6
    mul r0, r1, r0
    mov r0, r0, lsr #14
    mul r1, lr, r0
    sub r12, r3, r12
    mov r1, r1, lsr #12
    mul r1, r0, r1
    mov r0, r0, lsl #14
    add r0, r0, r1, lsr #15
    umull r1, r0, r2, r0
    mov r3, r0
    orrmi r0, r0, #0x80000000
    adds r12, r12, #0x7e
    bmi @underflow
    cmp r12, #0xfe
    bge @overflow
    add r0, r0, r12, lsl #23
    mov r12, r1, lsr #28
    cmp r12, #7
    beq @exact_tie
    add r0, r0, r1, lsr #31
    ldmia sp!, {lr}
    bx lr
    // The reciprocal table, one byte per mantissa from 1.0 to 2.0, from 0xff down to 0x83
    // Each word holds four of the table's bytes, the first in the low byte
    dcd 0xfdfeffff
    dcd 0xf9fafbfc
    dcd 0xf5f6f7f8
    dcd 0xf1f2f3f4
    dcd 0xeeeff0f0
    dcd 0xeaebeced
    dcd 0xe7e8e9ea
    dcd 0xe4e5e6e6
    dcd 0xe1e2e2e3
    dcd 0xdedfdfe0
    dcd 0xdbdcdcdd
    dcd 0xd8d9d9da
    dcd 0xd5d6d7d7
    dcd 0xd2d3d4d4
    dcd 0xd0d0d1d2
    dcd 0xcdcececf
    dcd 0xcbcbcccc
    dcd 0xc8c9c9ca
    dcd 0xc6c6c7c8
    dcd 0xc3c4c5c5
    dcd 0xc1c2c2c3
    dcd 0xbfbfc0c0
    dcd 0xbdbdbebe
    dcd 0xbabbbcbc
    dcd 0xb8b9b9ba
    dcd 0xb6b7b7b8
    dcd 0xb4b5b5b6
    dcd 0xb2b3b3b4
    dcd 0xb0b1b1b2
    dcd 0xafafafb0
    dcd 0xadadaeae
    dcd 0xababacac
    dcd 0xa9aaaaaa
    dcd 0xa7a8a8a9
    dcd 0xa6a6a7a7
    dcd 0xa4a4a5a5
    dcd 0xa2a3a3a4
    dcd 0xa1a1a2a2
    dcd 0x9fa0a0a0
    dcd 0x9e9e9e9f
    dcd 0x9c9d9d9d
    dcd 0x9b9b9b9c
    dcd 0x999a9a9a
    dcd 0x98989999
    dcd 0x96979798
    dcd 0x95959696
    dcd 0x94949495
    dcd 0x92939393
    dcd 0x91919292
    dcd 0x90909191
    dcd 0x8f8f8f90
    dcd 0x8d8e8e8e
    dcd 0x8c8c8d8d
    dcd 0x8b8b8c8c
    dcd 0x8a8a8a8b
    dcd 0x8989898a
    dcd 0x88888888
    dcd 0x86878787
    dcd 0x85868686
    dcd 0x84858585
    dcd 0x83838484
    dcd 0x82828383
    dcd 0x81818282
    dcd 0x80808181
@exact_tie:
    mov r1, r3, lsl #1
    add r1, r1, #1
    rsb lr, lr, #0
    mul r1, lr, r1
    cmp r1, r2, lsl #24
    addmi r0, r0, #1
    ldmia sp!, {lr}
    bx lr
@a_special:
    eor lr, r0, r1
    and lr, lr, #0x80000000
    cmp r3, #0
    beq @a_denormal
    movs r0, r0, lsl #9
    bne @nan
    mov r12, r1, lsr #23
    mov r1, r1, lsl #9
    ands r12, r12, #0xff
    beq @inf
    cmp r12, #0xff
    blt @inf
    cmp r1, #0
    beq @nan_inf_inf
    b @nan_b
@b_special:
    eor lr, r0, r1
    and lr, lr, #0x80000000
    cmp r12, #0
    beq @b_denormal
@b_inf_or_nan:
    movs r1, r1, lsl #9
    bne @nan_b
    b @zero
@a_denormal:
    movs r2, r0, lsl #9
    beq @a_zero
    clz r3, r2
    movs r2, r2, lsl r3
    rsb r3, r3, #0
    mov r2, r2, lsr #8
    ands r12, r12, r1, lsr #23
    beq @a_denormal_b_denormal
    cmp r12, #0xff
    beq @b_inf_or_nan
    orr r1, r1, #0x800000
    bic lr, r1, #0xff000000
    b @divide
@a_zero:
    mov r12, r1, lsr #23
    mov r1, r1, lsl #9
    ands r12, r12, #0xff
    beq @a_zero_b_exp0
    cmp r12, #0xff
    blt @zero
    cmp r1, #0
    beq @zero
    b @nan_b
@a_zero_b_exp0:
    cmp r1, #0
    beq @nan_inf_inf
    b @zero
@b_denormal:
    movs r12, r1, lsl #9
    beq @inf
    mov lr, r12
    clz r12, lr
    movs lr, lr, lsl r12
    rsb r12, r12, #0
    mov lr, lr, lsr #8
    orr r0, r0, #0x800000
    bic r2, r0, #0xff000000
    b @divide
@a_denormal_b_denormal:
    movs r12, r1, lsl #9
    beq @inf
    mov lr, r12
    clz r12, lr
    movs lr, lr, lsl r12
    rsb r12, r12, #0
    mov lr, lr, lsr #8
    b @divide
@underflow:
    and r0, r0, #0x80000000
    cmn r12, #0x18
    beq @half_min
    bmi @return_sign
    add r1, r12, #0x17
    mov r2, r2, lsl r1
    rsb r12, r12, #0
    mov r3, r3, lsr r12
    orr r0, r0, r3
    rsb lr, lr, #0
    mul r1, lr, r3
    cmp r1, r2
    ldmeqia sp!, {lr}
    bxeq lr
    add r1, r1, lr
    cmp r1, r2
    beq @round_up
    addmi r0, r0, #1
    subpl r1, r1, lr
    add r1, lr, r1, lsl #1
    cmp r1, r2, lsl #1
    and r3, r0, #1
    addmi r0, r0, #1
    addeq r0, r0, r3
    ldmia sp!, {lr}
    bx lr
@round_up:
    add r0, r0, #1
    ldmia sp!, {lr}
    bx lr
@half_min:
    cmn r2, lr
    addne r0, r0, #1
    ldmia sp!, {lr}
    bx lr
@nan_b:
    mov r0, r1
    b @nan
@inf:
    mov r0, #0xff000000
    orr r0, lr, r0, lsr #1
    ldmia sp!, {lr}
    bx lr
@nan:
    mvn r0, #0x80000000
    ldmia sp!, {lr}
    bx lr
@nan_inf_inf:
    mvn r0, #0x80000000
    ldmia sp!, {lr}
    bx lr
@overflow:
    tst r0, #0x80000000
    mov r0, #0xff000000
    movne r0, r0, asr #1
    moveq r0, r0, lsr #1
    ldmia sp!, {lr}
    bx lr
@return_sign:
    ldmia sp!, {lr}
    bx lr
@zero:
    mov r0, lr
    ldmia sp!, {lr}
    bx lr
}

// clang-format on

// _drdiv, the reverse division b / a: swaps the operands and falls into _ddiv.
// clang-format off
asm double _drdiv(double a, double b) {
    eor r1, r1, r3
    eor r3, r1, r3
    eor r1, r1, r3
    eor r0, r0, r2
    eor r2, r0, r2
    eor r0, r0, r2
}

// clang-format on

// _ddiv: a / b. The divisor's top mantissa bits index a 256-byte reciprocal table in the middle of the routine;
// fixed-point multiply steps refine it into a quotient estimate, and the remainder decides the final
// rounding.
// clang-format off
asm double _ddiv(double a, double b) {
    stmdb sp!, {r4, r5, r6, lr}
    ldr lr, =0xffe
    eor r4, r1, r3
    ands r12, lr, r1, lsr #19
    cmpne r12, lr
    beq @a_special
    bic r1, r1, lr, lsl #20
    orr r1, r1, #0x100000
    add r12, r12, r4, lsr #31
@unpack_b:
    ands r4, lr, r3, lsr #19
    cmpne r4, lr
    beq @b_special
    bic r3, r3, lr, lsl #20
    orr r3, r3, #0x100000
@divide:
    sub r12, r12, r4
    cmp r1, r3
    cmpeq r0, r2
    bhs @reciprocal
    adds r0, r0, r0
    adc r1, r1, r1
    sub r12, r12, #2
@reciprocal:
    // r4 = the reciprocal table less 0x100 entries: the divisor's mantissa bits 19 to 12 and its integer bit index it
    sub r4, pc, #0x24
    ldrb lr, [r4, r3, lsr #12]
    rsbs r2, r2, #0
    rsc r3, r3, #0
    mov r4, #0x20000000
    mla r5, lr, r3, r4
    mov r6, r3, lsl #10
    mov r5, r5, lsr #7
    mul lr, r5, lr
    orr r6, r6, r2, lsr #22
    mov lr, lr, lsr #13
    mul r5, lr, r6
    mov r6, r1, lsl #10
    orr r6, r6, r0, lsr #22
    mov r5, r5, lsr #16
    mul r5, lr, r5
    mov lr, lr, lsl #14
    add lr, lr, r5, lsr #16
    umull r5, r6, lr, r6
    umull r4, r5, r6, r2
    mla r5, r3, r6, r5
    mov r4, r4, lsr #26
    orr r4, r4, r5, lsl #6
    add r4, r4, r0, lsl #2
    umull lr, r5, r4, lr
    mov r4, #0
    adds r5, r5, r6, lsl #24
    adc r4, r4, r6, lsr #8
    cmp r12, #0x800
    bge @overflow
    add r12, r12, #0x7f0
    adds r12, r12, #0xc
    bmi @underflow
    orr r1, r4, r12, lsl #31
    bic r12, r12, #1
    add r1, r1, r12, lsl #19
    tst lr, #0x80000000
    bne @round_up
    rsbs r2, r2, #0
    mov r4, r4, lsl #1
    add r4, r4, r5, lsr #31
    mul lr, r2, r4
    mov r6, #0
    mov r4, r5, lsl #1
    orr r4, r4, #1
    umlal r6, lr, r4, r2
    rsc r3, r3, #0
    mla lr, r4, r3, lr
    cmp lr, r0, lsl #21
    bmi @round_up
    mov r0, r5
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@round_up:
    adds r0, r5, #1
    adc r1, r1, #0
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
    // The reciprocal table again
    dcd 0xfdfeffff
    dcd 0xf9fafbfc
    dcd 0xf5f6f7f8
    dcd 0xf1f2f3f4
    dcd 0xeeeff0f0
    dcd 0xeaebeced
    dcd 0xe7e8e9ea
    dcd 0xe4e5e6e6
    dcd 0xe1e2e2e3
    dcd 0xdedfdfe0
    dcd 0xdbdcdcdd
    dcd 0xd8d9d9da
    dcd 0xd5d6d7d7
    dcd 0xd2d3d4d4
    dcd 0xd0d0d1d2
    dcd 0xcdcececf
    dcd 0xcbcbcccc
    dcd 0xc8c9c9ca
    dcd 0xc6c6c7c8
    dcd 0xc3c4c5c5
    dcd 0xc1c2c2c3
    dcd 0xbfbfc0c0
    dcd 0xbdbdbebe
    dcd 0xbabbbcbc
    dcd 0xb8b9b9ba
    dcd 0xb6b7b7b8
    dcd 0xb4b5b5b6
    dcd 0xb2b3b3b4
    dcd 0xb0b1b1b2
    dcd 0xafafafb0
    dcd 0xadadaeae
    dcd 0xababacac
    dcd 0xa9aaaaaa
    dcd 0xa7a8a8a9
    dcd 0xa6a6a7a7
    dcd 0xa4a4a5a5
    dcd 0xa2a3a3a4
    dcd 0xa1a1a2a2
    dcd 0x9fa0a0a0
    dcd 0x9e9e9e9f
    dcd 0x9c9d9d9d
    dcd 0x9b9b9b9c
    dcd 0x999a9a9a
    dcd 0x98989999
    dcd 0x96979798
    dcd 0x95959696
    dcd 0x94949495
    dcd 0x92939393
    dcd 0x91919292
    dcd 0x90909191
    dcd 0x8f8f8f90
    dcd 0x8d8e8e8e
    dcd 0x8c8c8d8d
    dcd 0x8b8b8c8c
    dcd 0x8a8a8a8b
    dcd 0x8989898a
    dcd 0x88888888
    dcd 0x86878787
    dcd 0x85868686
    dcd 0x84858585
    dcd 0x83838484
    dcd 0x82828383
    dcd 0x81818282
    dcd 0x80808181
@overflow:
    movs r1, r12, lsl #31
    orr r1, r1, #0x7f000000
    orr r1, r1, #0xf00000
    mov r0, #0
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@underflow:
    mvn r6, r12, asr #1
    cmp r6, #0x34
    bgt @zero_underflow
    beq @half_min
    cmp r6, #0x14
    bge @underflow_wide
    rsb r6, r6, #0x13
    mov lr, r0, lsl r6
    rsb r6, r6, #0x14
    mov r0, r5, lsr r6
    rsb r6, r6, #0x20
    orr r0, r0, r4, lsl r6
    rsb r6, r6, #0x20
    mov r4, r4, lsr r6
    orr r1, r4, r12, lsl #31
    mov r12, lr
    mov lr, #0
    b @underflow_round
@underflow_wide:
    rsb r6, r6, #0x33
    mov lr, r1, lsl r6
    mov r1, r12, lsl #31
    rsb r6, r6, #0x20
    orr r12, lr, r0, lsr r6
    rsb r6, r6, #0x20
    mov lr, r0, lsl r6
    mov r5, r5, lsr #21
    orr r5, r5, r4, lsl #11
    rsb r6, r6, #0x1f
    mov r0, r5, lsr r6
    mov r4, #0
@underflow_round:
    rsbs r2, r2, #0
    mul r4, r2, r4
    mov r5, #0
    umlal r5, r4, r2, r0
    rsc r3, r3, #0
    mla r4, r0, r3, r4
    cmp r4, r12
    cmpeq r5, lr
    ldmeqia sp!, {r4, r5, r6, lr}
    bxeq lr
    adds r5, r5, r2
    adc r4, r4, r3
    cmp r4, r12
    bmi @increment_check
    bne @underflow_exact
    cmp r5, lr
    beq @increment
    blo @increment_check
@underflow_exact:
    subs r5, r5, r2
    sbc r4, r4, r3
@underflow_check:
    adds r5, r5, r5
    adc r4, r4, r4
    adds r5, r5, r2
    adc r4, r4, r3
    adds lr, lr, lr
    adc r12, r12, r12
    cmp r4, r12
    bmi @increment
    ldmneia sp!, {r4, r5, r6, lr}
    bxne lr
    cmp r5, lr
    blo @increment
    ldmneia sp!, {r4, r5, r6, lr}
    bxne lr
    tst r0, #1
    ldmeqia sp!, {r4, r5, r6, lr}
    bxeq lr
@increment:
    adds r0, r0, #1
    adc r1, r1, #0
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@increment_check:
    adds r0, r0, #1
    adc r1, r1, #0
    b @underflow_check
@half_min:
    rsbs r2, r2, #0
    rsc r3, r3, #0
    cmp r1, r3
    cmpeq r0, r2
    mov r1, r12, lsl #31
    mov r0, #0
    movne r0, #1
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@zero_underflow:
    mov r1, r12, lsl #31
    mov r0, #0
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@a_special:
    orrs r5, r0, r1, lsl #1
    beq @a_zero
    cmp r12, lr
    beq @a_inf_or_nan
    movs r1, r1, lsl #12
    beq @a_denormal_low
    clz r5, r1
    movs r1, r1, lsl r5
    sub r12, r12, r5
    add r5, r12, #0x1f
    mov r1, r1, lsr #11
    orr r1, r1, r0, lsr r5
    rsb r5, r5, #0x20
    mov r0, r0, lsl r5
    mov r12, r12, lsl #1
    orr r12, r12, r4, lsr #31
    b @unpack_b
@a_denormal_low:
    mvn r12, #0x13
    clz r5, r0
    movs r0, r0, lsl r5
    sub r12, r12, r5
    mov r1, r0, lsr #11
    mov r0, r0, lsl #21
    mov r12, r12, lsl #1
    orr r12, r12, r4, lsr #31
    b @unpack_b
@a_inf_or_nan:
    orrs r5, r0, r1, lsl #12
    bne @a_nan
    bic r5, r3, #0x80000000
    cmp r5, lr, lsl #19
    bhs @a_inf_b_special
    and r5, r3, #0x80000000
    eor r1, r5, r1
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@a_inf_b_special:
    orrs r5, r2, r3, lsl #12
    bne @nan_b
    b @nan
@b_special:
    orrs r5, r2, r3, lsl #1
    beq @inf
    cmp r4, lr
    beq @b_inf_or_nan
    movs r3, r3, lsl #12
    beq @b_denormal_low
    clz r5, r3
    movs r3, r3, lsl r5
    sub r4, r4, r5
    add r5, r4, #0x1f
    mov r3, r3, lsr #11
    orr r3, r3, r2, lsr r5
    rsb r5, r5, #0x20
    mov r2, r2, lsl r5
    mov r4, r4, lsl #1
    b @divide
@b_denormal_low:
    mvn r4, #0x13
    clz r5, r2
    movs r2, r2, lsl r5
    sub r4, r4, r5
    mov r3, r2, lsr #11
    mov r2, r2, lsl #21
    mov r4, r4, lsl #1
    b @divide
@b_inf_or_nan:
    orrs r5, r2, r3, lsl #12
    bne @nan_b
    mov r1, r12, lsl #31
    mov r0, #0
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@inf:
    mov r1, r12, lsl #31
    orr r1, r1, lr, lsl #19
    mov r0, #0
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@a_zero:
    orrs r5, r2, r3, lsl #1
    beq @nan
    bic r5, r3, #0x80000000
    cmp r5, lr, lsl #19
    cmpeq r2, #0
    bhi @nan_b
    eor r1, r1, r3
    and r1, r1, #0x80000000
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@a_nan:
    tst r1, #0x80000
    beq @nan
    bic r5, r3, #0x80000000
    cmp r5, lr, lsl #19
    cmpeq r2, #0
    bhi @nan_b
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@nan_b:
    tst r3, #0x80000
    beq @nan
    mov r1, r3
    mov r0, r2
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
@nan:
    orr r1, r1, #0x7f000000
    orr r1, r1, #0xf80000
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

// clang-format on

// _fp_init, which the startup code calls before the static initializers. There is no floating-point hardware
// to set up, so it returns.
// clang-format off
asm void _fp_init(void) {
    bx lr
}

// clang-format on
