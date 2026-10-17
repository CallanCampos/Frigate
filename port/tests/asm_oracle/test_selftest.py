"""The harness's own test: ROM functions that the decomp already has as matching C agree, run as the ROM's code in
the emulator and as that C compiled for the host, on 1000 seeded random inputs each. Between them they cover ARM and
Thumb functions, return values and output buffers, alignment, overlapping and empty buffers, a 64-bit argument, a
stack argument, ROM functions that call other ROM functions, and function pointers called from both sides. The rest
checks what the emulator reports: unaligned accesses, faults, BIOS calls and runaway loops.
"""

import random
import struct

from build import build_library
from compare import Callback, Case, Mismatch, Ptr, compare, main
from harness import Emulator, Fault, InstructionLimit, UnalignedAccess

COUNT = 1000
SOURCES = [
    "lib/nitro/src/fx/fx.c",
    "lib/nitro/src/fx/fx_mtx33.c",
    "lib/nitro/src/fx/fx_mtx43.c",
    "lib/nitro/src/fx/fx_vec.c",
    "lib/nitro/src/math/math_crc.c",
    "lib/nitro/src/std/std_string.c",
]
# The self-test's own host functions: a function pointer called from host C, for the round trip of a Callback; a
# 64-bit result; and functions that are wrong on purpose, which the comparison must catch
WRAPPER = """\
#include "nitro/fx.h"
#include "nitro/math.h"
#include "nitro/std.h"

s32 asm_oracle_selftest_call(MATHCompareFunc compare, void *a, void *b) {
    return compare(a, b);
}

s64 asm_oracle_selftest_multiply(s32 a, s32 b) {
    return (s64)a * b;
}

// Wrong: the result is off by one when y is odd
fx32 asm_oracle_selftest_wrong_dot(const VecFx32 *a, const VecFx32 *b) {
    return VEC_DotProduct(a, b) + (a->y & 1);
}

// Wrong: one bit of z flips for some inputs
void asm_oracle_selftest_wrong_cross(const VecFx32 *a, const VecFx32 *b, VecFx32 *axb) {
    VEC_CrossProduct(a, b, axb);
    if (a->x & 0x100) {
        axb->z ^= 1;
    }
}

// Wrong: writes the byte before the destination when the size is 5
int asm_oracle_selftest_wrong_copy(char *dst, const char *src, int size) {
    int length = STD_CopyLString(dst, src, size);
    if (size == 5) {
        dst[-1] ^= 0x80;
    }
    return length;
}

// Wrong: the same result for an antisymmetric comparison, but the arguments swapped
s32 asm_oracle_selftest_wrong_call(MATHCompareFunc compare, void *a, void *b) {
    return -compare(b, a);
}
"""
# asm_oracle_selftest_call in ARM code: push {r4, lr}; mov r3, r0; mov r0, r1; mov r1, r2; blx r3; pop {r4, pc}
CALL_STUB = struct.pack("<6I", 0xE92D4010, 0xE1A03000, 0xE1A00001, 0xE1A01002, 0xE12FFF33, 0xE8BD8010)
# asm_oracle_selftest_multiply in ARM code: smull r2, r3, r0, r1; mov r0, r2; mov r1, r3; bx lr
MULTIPLY_STUB = struct.pack("<4I", 0xE0C32190, 0xE1A00002, 0xE1A01003, 0xE12FFF1E)
# A u32 comparator in ARM code: ldr r2, [r0]; ldr r3, [r1]; cmp r2, r3; mvnlo r0, #0; moveq r0, #0; movhi r0, #1;
# bx lr
COMPARE_U32_STUB = struct.pack("<7I", 0xE5902000, 0xE5913000, 0xE1520003, 0x33E00000, 0x03A00000, 0x83A00001,
                               0xE12FFF1E)


class Lazy:
    """The emulator and the library, made on first use, so that a missing ROM skips before either is needed."""

    def __getattr__(self, name):
        if name == "emu":
            self.emu = Emulator()
            return self.emu
        if name == "lib":
            self.lib = build_library("selftest", SOURCES, wrapper=WRAPPER)
            return self.lib
        raise AttributeError(name)


shared = Lazy()


def fx32(rng) -> int:
    """A fixed point value: any 32-bit value, or a small one (most of the game's), or an extreme."""
    choice = rng.random()
    if choice < 0.4:
        return rng.randrange(-(1 << 31), 1 << 31)
    if choice < 0.9:
        return rng.randrange(-0x40000, 0x40000)
    return rng.choice([0, 1, -1, 0x7FFFFFFF, -0x80000000, 0x1000, -0x1000])


def words(*values) -> bytes:
    return struct.pack(f"<{len(values)}i", *values)


def test_vec_cross_product():
    """ARM; three structures, the result possibly over an input."""

    def generate(rng):
        a, b, out = words(*(fx32(rng) for _ in range(3))), words(*(fx32(rng) for _ in range(3))), rng.randbytes(12)
        target = rng.choice(["out", "out", "out", "a", "b"])
        return Case({"a": a, "b": b, "out": out}, [Ptr("a", 4 * rng.randrange(3)), Ptr("b"), Ptr(target)])

    return f"{compare(shared.emu, shared.lib, 'VEC_CrossProduct', 'void(ptr, ptr, ptr)', generate, count=COUNT)} cases"


def test_vec_dot_product():
    """ARM; a return value."""

    def generate(rng):
        return Case({"a": words(*(fx32(rng) for _ in range(3))), "b": words(*(fx32(rng) for _ in range(3)))},
                    [Ptr("a"), Ptr("b")])

    return f"{compare(shared.emu, shared.lib, 'VEC_DotProduct', 's32(ptr, ptr)', generate, count=COUNT)} cases"


def test_vec_fx16_add():
    """Thumb; 16-bit values at 2-byte alignment."""

    def generate(rng):
        def vec():
            return struct.pack("<3h", *(rng.randrange(-0x8000, 0x8000) for _ in range(3)))

        return Case({"a": vec(), "b": vec(), "out": rng.randbytes(8)},
                    [Ptr("a"), Ptr("b"), Ptr("out", 2 * rng.randrange(2))])

    return f"{compare(shared.emu, shared.lib, 'VEC_Fx16Add', 'void(ptr, ptr, ptr)', generate, count=COUNT)} cases"


def test_fx_mul32x64c():
    """ARM; a 64-bit argument, which MWCC passes in r1 and r2."""

    def generate(rng):
        return Case({}, [fx32(rng), rng.randrange(-(1 << 63), 1 << 63)])

    return f"{compare(shared.emu, shared.lib, 'FX_Mul32x64cFunc', 's32(s32, s64)', generate, count=COUNT)} cases"


def test_mtx_scale_apply43():
    """ARM; a fifth argument on the stack, a call to another ROM function (MTX_ScaleApply33), in place or not."""

    def generate(rng):
        source = words(*(fx32(rng) for _ in range(12)))
        target = rng.choice(["dst", "dst", "src"])
        return Case({"src": source, "dst": rng.randbytes(48)},
                    [Ptr("src"), Ptr(target), fx32(rng), fx32(rng), fx32(rng)])

    signature = "void(ptr, ptr, s32, s32, s32)"
    return f"{compare(shared.emu, shared.lib, 'MTX_ScaleApply43', signature, generate, count=COUNT)} cases"


def test_crc32():
    """Thumb; the table built by one function, then used by another that calls a third; empty and unaligned
    data."""

    def generate_table(rng):
        return Case({"table": rng.randbytes(1024)}, [Ptr("table"), rng.choice([0xEDB88320, rng.getrandbits(32)])])

    def generate(rng):
        size = rng.choice([0, 1, 2, 3, rng.randrange(300)])
        return Case({"table": rng.randbytes(1024), "data": rng.randbytes(size + 3)},
                    [Ptr("table"), Ptr("data", rng.randrange(4)), size])

    tables = compare(shared.emu, shared.lib, "MATHi_CRC32InitTableRev", "void(ptr, u32)", generate_table, count=COUNT)
    crcs = compare(shared.emu, shared.lib, "MATH_CalcCRC32", "u32(ptr, ptr, u32)", generate, count=COUNT)
    return f"{tables} + {crcs} cases"


def test_std_copy_lstring():
    """Thumb; a word-at-a-time copy that depends on the two pointers' alignment, odd and non-positive sizes, and a
    call to STD_GetStringLength for what doesn't fit."""

    def generate(rng):
        return make(rng, rng.randrange(4), rng.randrange(4))

    def generate_aligned(rng):
        # The same alignment for both pointers, which takes the word loop
        offset = rng.randrange(4)
        return make(rng, offset, offset)

    def make(rng, dst_offset, src_offset):
        length = rng.choice([0, 1, 2, 3, 4, 5, rng.randrange(80)])
        text = bytes(rng.randrange(1, 256) for _ in range(src_offset + length)) + b"\0"
        size = rng.choice([-1, 0, 1, 2, 3, 4, 5, rng.randrange(-4, length + 8)])
        return Case({"src": text + rng.randbytes(rng.randrange(8)), "dst": rng.randbytes(length + 16)},
                    [Ptr("dst", dst_offset), Ptr("src", src_offset), size])

    signature = "s32(ptr, ptr, s32)"
    mixed = compare(shared.emu, shared.lib, "STD_CopyLString", signature, generate, count=COUNT, seed="mixed")
    aligned = compare(shared.emu, shared.lib, "STD_CopyLString", signature, generate_aligned, count=COUNT,
                      seed="aligned")
    return f"{mixed} + {aligned} cases"


def test_callback_round_trip():
    """A Callback given to ARM code in the emulator and to host C, both calling it the same way."""

    def compare_s32(memory, a, b):
        x, y = memory.s32(a), memory.s32(b)
        return (x > y) - (x < y)

    emu = shared.emu
    stub = emu.stub(CALL_STUB)
    callback = Callback(compare_s32, "s32(ptr, ptr)")

    def generate(rng):
        return Case({"a": words(fx32(rng)), "b": words(fx32(rng))}, [callback, Ptr("a"), Ptr("b")])

    count = compare(shared.emu, shared.lib, stub, "s32(fn, ptr, ptr)", generate, count=COUNT,
                    host_function="asm_oracle_selftest_call")
    return f"{count} cases"


def test_64_bit_result():
    """A 64-bit result, in r0 and r1."""
    stub = shared.emu.stub(MULTIPLY_STUB)

    def generate(rng):
        return Case({}, [fx32(rng), fx32(rng)])

    count = compare(shared.emu, shared.lib, stub, "s64(s32, s32)", generate, count=COUNT,
                    host_function="asm_oracle_selftest_multiply")
    return f"{count} cases"


def test_differences_are_caught():
    """Host functions that are wrong on purpose (a return value, an output byte, a write before a buffer, a
    callback called differently) each fail the comparison, with the difference named."""
    stub = shared.emu.stub(CALL_STUB)

    def compare_s32(memory, a, b):
        x, y = memory.s32(a), memory.s32(b)
        return (x > y) - (x < y)

    callback = Callback(compare_s32, "s32(ptr, ptr)")

    def vectors(rng):
        return Case({"a": words(*(fx32(rng) for _ in range(3))), "b": words(*(fx32(rng) for _ in range(3))),
                     "out": bytes(12)}, [Ptr("a"), Ptr("b"), Ptr("out")])

    def dot(rng):
        case = vectors(rng)
        return Case(case.regions, case.args[:2])

    def strings(rng):
        text = bytes(rng.randrange(1, 256) for _ in range(rng.randrange(12))) + b"\0"
        return Case({"src": text, "dst": bytes(16)}, [Ptr("dst"), Ptr("src"), rng.randrange(8)])

    def pair(rng):
        return Case({"a": words(fx32(rng)), "b": words(fx32(rng))}, [callback, Ptr("a"), Ptr("b")])

    expected = [
        ("VEC_DotProduct", "s32(ptr, ptr)", dot, "asm_oracle_selftest_wrong_dot", "return value"),
        ("VEC_CrossProduct", "void(ptr, ptr, ptr)", vectors, "asm_oracle_selftest_wrong_cross", "memory: out[8]"),
        ("STD_CopyLString", "s32(ptr, ptr, s32)", strings, "asm_oracle_selftest_wrong_copy",
         "memory: dst[-1] in the pad before it"),
        (stub, "s32(fn, ptr, ptr)", pair, "asm_oracle_selftest_wrong_call", "callback calls"),
    ]
    for function, signature, generate, wrong, difference in expected:
        try:
            compare(shared.emu, shared.lib, function, signature, generate, count=COUNT, host_function=wrong)
        except Mismatch as mismatch:
            if difference not in str(mismatch):
                raise AssertionError(f"{wrong}: the report doesn't name {difference!r}:\n{mismatch}") from None
            continue
        raise AssertionError(f"{wrong} wasn't caught")
    return f"{len(expected)} deliberate differences caught"


def test_qsort_calls_back():
    """MATH_QSort's original (ARM asm) sorting with a Python callback and with an ARM code comparator alike."""
    python_calls = []

    def compare_u32(emu, a, b, *_):
        x, y = emu.read_u32(a), emu.read_u32(b)
        python_calls.append((x, y))
        return (x > y) - (x < y)

    emu = shared.emu
    python_comparator = emu.callback(compare_u32)
    code_comparator = emu.stub(COMPARE_U32_STUB)
    rng = random.Random("qsort")
    for _ in range(200):
        values = [rng.getrandbits(32) if rng.random() < 0.5 else rng.randrange(8) for _ in range(rng.randrange(40))]
        data = struct.pack(f"<{len(values)}I", *values)
        results = []
        for comparator in (python_comparator, code_comparator):
            emu.reset_scratch()
            array = emu.alloc(max(len(data), 4), data=data)
            emu.call("MATH_QSort", array, len(values), 4, comparator, 0)
            results.append(emu.read(array, len(data)))
        if results[0] != results[1] or list(struct.unpack(f"<{len(values)}I", results[0])) != sorted(values):
            raise AssertionError(f"MATH_QSort didn't sort {values}")
    return f"200 arrays, {len(python_calls)} comparisons through the Python callback"


def test_harness_reports():
    """Unaligned accesses, faults, BIOS calls and runaway loops stop a call with the matching error, and recorded
    writes, replaced functions and stack arguments read back as expected. It replaces a ROM function for good, so
    it runs last."""
    emu = shared.emu
    emu.reset_scratch()
    a = emu.alloc(16, data=words(1, 2, 3))
    out = emu.alloc(16)
    checks = [
        (UnalignedAccess, lambda: emu.call("VEC_DotProduct", a + 2, a)),
        (Fault, lambda: emu.call("VEC_DotProduct", 0x10, a)),
        (Fault, lambda: emu.call("BitUnPack", a, out, a)),
        (InstructionLimit, lambda: emu.call(emu.stub(struct.pack("<I", 0xEAFFFFFE)), max_instructions=1000)),
    ]
    for expected, call in checks:
        try:
            call()
        except expected:
            continue
        raise AssertionError(f"expected {expected.__name__}")

    with emu.record_writes(out, out + 16) as writes:
        emu.call("VEC_Add", a, a, out)
    if [(w.address - out, w.size, w.value) for w in writes] != [(0, 4, 2), (4, 4, 4), (8, 4, 6)]:
        raise AssertionError(f"recorded writes {writes}")

    seen = []
    emu.replace("MTX_ScaleApply33", lambda emu, *_: seen.append(emu.args([4] * 5)))
    emu.call("MTX_ScaleApply43", a, out, 7, 8, 9)
    if seen != [[a, out, 7, 8, 9]]:
        raise AssertionError(f"the replaced function saw {seen}")
    return "errors, recorded writes, replaced functions and stack arguments"


if __name__ == "__main__":
    main([
        test_vec_cross_product,
        test_vec_dot_product,
        test_vec_fx16_add,
        test_fx_mul32x64c,
        test_mtx_scale_apply43,
        test_crc32,
        test_std_copy_lstring,
        test_callback_round_trip,
        test_64_bit_result,
        test_differences_are_caught,
        test_qsort_calls_back,
        test_harness_reports,
    ])
