"""The host versions in port/hal of the SDK's assembly in libfx's matrix files and of its products whose decomp C reads an
argument after storing over it (port/hal/fx), libmath
(MATH_CountLeadingZerosFunc, MATH_QSort and sha1_block, port/hal/math) and libcrypto (CRYPTO_RC4Encrypt,
port/hal/crypto), against the ROM's code.

Every function runs on thousands of seeded random inputs, built twice: with the game's flags, as build.py gives a port/
file, and again with strict aliasing on, signed overflow undefined, the undefined behaviour sanitizer trapping and
uninitialized locals filled with a pattern, so that the C is shown not to lean on the game's -fwrapv,
-fno-strict-aliasing and -ftrivial-auto-var-init=zero. The inputs are the ones the originals accept: matrices on 4-byte
boundaries (the ARM9 ignores the low bits of an unaligned word store), and for MATH_QSort elements on 4-byte boundaries
when the width is a multiple of 4 (it swaps words then), a comparison function that doesn't put an element after itself
(the original then scans on before the array) and a width other than 0 unless the comparison never asks for a swap (the
original's word swap then runs through memory).
"""

import random
import struct
from dataclasses import dataclass

from build import build_library
from compare import (Callback, Case, Mismatch, Outcome, Ptr, Signature, _describe_case, _first_difference, _pad_bytes,
                     main, run_host, run_original)
from harness import Emulator

SOURCES = [
    "port/hal/fx/fx_mtx22.c",
    "port/hal/fx/fx_mtx33.c",
    "port/hal/fx/fx_mtx43.c",
    "port/hal/fx/fx_mtx44.c",
    "port/hal/math/math_pop.c",
    "port/hal/math/math.c",
    "port/hal/math/sha1_block.c",
    "port/hal/crypto/rc4.c",
]
# Native comparison functions for the large sorts, the same as the ARM stubs below
WRAPPER = """\
#include "nitro/math.h"

s32 asm_oracle_fx_compare_byte(void *a, void *b) {
    return *(u8 *)a - *(u8 *)b;
}

s32 asm_oracle_fx_compare_word(void *a, void *b) {
    u32 x = *(u32 *)a;
    u32 y = *(u32 *)b;

    return x < y ? -1 : x > y;
}
"""
# The flags of the second build: aliasing rules and signed overflow as C11 has them, undefined behaviour trapping, and
# uninitialized locals filled with a pattern where the game's flags zero them, so that a read of one before it is
# written gives different results in the two builds (it comes after the game's =zero, which it overrides)
STRICT_FLAGS = ["-fstrict-aliasing", "-fno-wrapv", "-fsanitize=undefined", "-fno-sanitize=function",
                "-fsanitize-trap=undefined", "-ftrivial-auto-var-init=pattern"]
# ldrb r2, [r0]; ldrb r3, [r1]; sub r0, r2, r3; bx lr
COMPARE_BYTE_STUB = struct.pack("<4I", 0xE5D02000, 0xE5D13000, 0xE0420003, 0xE12FFF1E)
# ldr r2, [r0]; ldr r3, [r1]; cmp r2, r3; mvnlo r0, #0; moveq r0, #0; movhi r0, #1; bx lr
COMPARE_WORD_STUB = struct.pack("<7I", 0xE5902000, 0xE5913000, 0xE1520003, 0x33E00000, 0x03A00000, 0x83A00001,
                                0xE12FFF1E)
PAD = 64


class Lazy:
    """The emulator and the two builds, made on first use, so that a missing ROM skips before either is needed."""

    def __getattr__(self, name):
        if name == "emu":
            self.emu = Emulator(max_instructions=200_000_000)
            return self.emu
        if name == "libs":
            self.libs = [("game flags", build_library("fx_hal", SOURCES, wrapper=WRAPPER)),
                         ("strict C", build_library("fx_hal_strict", SOURCES, wrapper=WRAPPER,
                                                    extra_flags=STRICT_FLAGS))]
            return self.libs
        raise AttributeError(name)


shared = Lazy()


@dataclass
class SplitCase:
    """A case whose two sides differ: their own comparison function each, and a scratch region whose words the
    original fills with addresses and the host with offsets from the array's head."""

    original: Case
    host: Case
    scratch: str | None = None
    head: Ptr | None = None


def check_scratch(name, head, initial, original, host):
    """The original stores elements' addresses in the stack buffer where the host stores their offsets from head, so
    every word either side wrote must be the host's plus one same number, the head's address, whose low 12 bits are
    the head's offset in its region (regions start at multiples of 0x1000)."""
    deltas = set()
    for index in range(0, len(initial), 4):
        before = int.from_bytes(initial[index:index + 4], "little")
        ours = int.from_bytes(host[index:index + 4], "little")
        theirs = int.from_bytes(original[index:index + 4], "little")
        if ours == before and theirs == before:
            continue
        deltas.add((theirs - ours) & 0xFFFFFFFF)
    if len(deltas) > 1 or any(delta & 0xFFF != head.offset & 0xFFF for delta in deltas):
        return [f"  memory: {name} holds other ranges (original minus host per written word: "
                f"{', '.join(hex(d) for d in sorted(deltas))})",
                f"    original {original.hex(' ')}",
                f"    host     {host.hex(' ')}"]
    return []


def run(function, signature, generate, *, count, seed):
    """compare() for both builds, running the original once per case: generate(rng, lib) returns a Case, or a
    SplitCase for sides that differ. Raises Mismatch at the first difference; returns the number of cases. A trap of
    the sanitizer kills the process, which run_all.py reports as the file failing."""
    # Parsed once: run_original and run_host take the text, the description of a failing case only the parsed form
    signature = Signature.parse(signature)
    for index in range(count):
        def make(lib):
            made = generate(random.Random(f"{seed}:{index}"), lib)
            return made if isinstance(made, SplitCase) else SplitCase(made, made)

        first = make(shared.libs[0][1])
        pads = {name: _pad_bytes(seed, index, name, PAD) for name in first.original.regions}
        header = f"{function}: case {index} of seed {seed!r}, drawn from random.Random('{seed}:{index}')"
        original = run_original(shared.emu, function, signature, first.original, pad=PAD, pads=pads)
        if original.error:
            raise Mismatch("\n".join([header] + _describe_case(first.original, signature) +
                                     [f"  the original can't run it: {original.error}"]))
        for build, lib in shared.libs:
            split = make(lib)
            host = run_host(getattr(lib.cdll, function), signature, split.host, pad=PAD, pads=pads)
            expected = Outcome(original.ret, dict(original.memory), original.calls)
            difference = []
            if split.scratch:
                before, after = pads[split.scratch]
                initial = before + split.original.regions[split.scratch] + after
                difference = check_scratch(split.scratch, split.head, initial, expected.memory.pop(split.scratch),
                                           host.memory.pop(split.scratch))
            difference = difference or _first_difference(expected, host, PAD)
            if difference:
                raise Mismatch("\n".join([f"{header}, {build} build"] +
                                         _describe_case(split.host, signature) + difference))
    return count


def fx32(rng) -> int:
    """A fixed point value: any 32-bit value, a small one (most of the game's), or an extreme."""
    choice = rng.random()
    if choice < 0.4:
        return rng.randrange(-(1 << 31), 1 << 31)
    if choice < 0.85:
        return rng.randrange(-0x2000, 0x2001)
    return rng.choice([0, 1, -1, 0x1000, -0x1000, 0x7FFFFFFF, -0x80000000, 0x80000000 - 0x1000])


# The matrix functions

# Each function that only stores: its signature and the size of its matrix
STORES = [
    ("MTX_Identity22_", "void(ptr)", 16),
    ("MAT2_Scaling", "void(ptr, s32, s32)", 16),
    ("MTX_Rot22_", "void(ptr, s32, s32)", 16),
    ("MTX_Identity33_", "void(ptr)", 36),
    ("MAT3_Scaling", "void(ptr, s32, s32, s32)", 36),
    ("MTX_RotX33_", "void(ptr, s32, s32)", 36),
    ("MTX_RotY33_", "void(ptr, s32, s32)", 36),
    ("MTX_RotZ33_", "void(ptr, s32, s32)", 36),
    ("MTX_Identity43_", "void(ptr)", 48),
    ("MTX_Scale43_", "void(ptr, s32, s32, s32)", 48),
    ("MTX_RotY43_", "void(ptr, s32, s32)", 48),
    ("MTX_Identity44_", "void(ptr)", 64),
]
# Each copy: the sizes of its source and destination
COPIES = [
    ("MAT3_To4x4", 36, 64),
    ("MTX_Copy43To44_", 48, 64),
    ("MTX_Copy44To43_", 64, 48),
]


def test_matrix_stores():
    """The identities, scalings and rotations, on random old contents at every word offset, with any value."""
    total = 0
    for function, signature, size in STORES:
        arguments = signature.count(",")

        def generate(rng, lib, size=size, arguments=arguments):
            offset = 4 * rng.randrange(8)
            return Case({"mtx": rng.randbytes(offset + size + 8)}, [Ptr("mtx", offset)] +
                        [fx32(rng) for _ in range(arguments)])

        total += run(function, signature, generate, count=2000, seed=function)
    return f"{len(STORES)} functions, {total} cases each build"


def test_matrix_copies():
    """The conversions between matrix sizes, apart and overlapping by any number of words, the same matrix
    included."""
    total = 0
    for function, source, destination in COPIES:
        def separate(rng, lib, source=source, destination=destination):
            src_offset, dst_offset = 4 * rng.randrange(8), 4 * rng.randrange(8)
            return Case({"src": rng.randbytes(src_offset + source + 8),
                         "dst": rng.randbytes(dst_offset + destination + 8)},
                        [Ptr("src", src_offset), Ptr("dst", dst_offset)])

        def overlapping(rng, lib):
            # The destination from 20 words before the source to 20 after it, which covers every overlap
            src_offset = 4 * rng.randrange(20, 24)
            dst_offset = src_offset + 4 * rng.randrange(-20, 21)
            return Case({"buf": rng.randbytes(256)}, [Ptr("buf", src_offset), Ptr("buf", dst_offset)])

        def identical(rng, lib):
            offset = 4 * rng.randrange(8)
            return Case({"buf": rng.randbytes(offset + 80)}, [Ptr("buf", offset), Ptr("buf", offset)])

        total += run(function, "void(ptr, ptr)", separate, count=2000, seed=f"{function}:separate")
        total += run(function, "void(ptr, ptr)", overlapping, count=2000, seed=f"{function}:overlapping")
        total += run(function, "void(ptr, ptr)", identical, count=200, seed=f"{function}:identical")
    return f"{len(COPIES)} functions, {total} cases each build"


# Each product whose decomp C reads an argument after storing over it: the sizes of its two arguments and its result
PRODUCTS = [
    ("MTX_Concat33", 36, 36, 36),
    ("MTX_Concat43", 48, 48, 48),
    ("MTX_Concat44", 64, 64, 64),
    ("MTX_MultVec43", 12, 48, 12),
]


def fx32_words(rng, size: int) -> bytes:
    return b"".join(struct.pack("<i", fx32(rng)) for _ in range(size // 4))


def test_matrix_products():
    """The products, apart; with the result over its first argument, its second or both, and with the two arguments
    the same; and the three overlapping by any number of words."""
    total = 0
    for function, first, second, result in PRODUCTS:
        def separate(rng, lib, first=first, second=second, result=result):
            offsets = [4 * rng.randrange(8) for _ in range(3)]
            return Case({"a": fx32_words(rng, offsets[0] + first + 8), "b": fx32_words(rng, offsets[1] + second + 8),
                         "ab": rng.randbytes(offsets[2] + result + 8)},
                        [Ptr("a", offsets[0]), Ptr("b", offsets[1]), Ptr("ab", offsets[2])])

        def identical(rng, lib, first=first, second=second):
            # Which of the three share one place: the result and the first argument, the result and the second, the
            # two arguments, or all three
            same = rng.choice(["ab=a", "ab=b", "a=b", "all"])
            offset = 4 * rng.randrange(8)
            regions = {"x": fx32_words(rng, offset + max(first, second) + 8),
                       "y": fx32_words(rng, offset + max(first, second) + 8)}
            places = {"ab=a": ("x", "y", "x"), "ab=b": ("x", "y", "y"), "a=b": ("x", "x", "y"),
                      "all": ("x", "x", "x")}[same]
            return Case(regions, [Ptr(place, offset) for place in places])

        def overlapping(rng, lib):
            # Each from 20 words before the first to 20 after it, which covers every overlap of the three
            base = 4 * rng.randrange(20, 24)
            offsets = [base] + [base + 4 * rng.randrange(-20, 21) for _ in range(2)]
            return Case({"buf": fx32_words(rng, 320)}, [Ptr("buf", offset) for offset in offsets])

        signature = "void(ptr, ptr, ptr)"
        total += run(function, signature, separate, count=2000, seed=f"{function}:separate")
        total += run(function, signature, identical, count=2000, seed=f"{function}:identical")
        total += run(function, signature, overlapping, count=4000, seed=f"{function}:overlapping")
    return f"{len(PRODUCTS)} functions, {total} cases each build"


# libmath

def test_count_leading_zeros():
    """Zero, every power of two and the values on either side of one, and random values of every bit length."""

    def generate(rng, lib):
        bits = rng.randrange(33)
        kind = rng.randrange(4)
        if bits == 0:
            value = 0
        elif kind == 0:
            value = 1 << (bits - 1)
        elif kind == 1:
            value = (1 << bits) - 1
        elif kind == 2:
            value = ((1 << (bits - 1)) + rng.choice([-1, 1])) & 0xFFFFFFFF
        else:
            value = (1 << (bits - 1)) | rng.getrandbits(bits - 1)
        return Case({}, [value])

    return f"{run('MATH_CountLeadingZerosFunc', 'u32(u32)', generate, count=4000, seed=0)} cases each build"


def test_sha1_block():
    """Random states and blocks, an all-zero and an all-one block, at every word offset; the trailing counters
    must stay as they were."""

    def generate(rng, lib):
        offset = 4 * rng.randrange(4)
        context = rng.randbytes(96)
        fill = rng.random()
        if fill < 0.1:
            context = context[:20] + bytes(64) + context[84:]
        elif fill < 0.2:
            context = context[:20] + b"\xff" * 64 + context[84:]
        return Case({"ctx": rng.randbytes(offset) + context + rng.randbytes(8)}, [Ptr("ctx", offset)])

    return f"{run('sha1_block', 'void(ptr)', generate, count=3000, seed=0)} cases each build"


def element_layout(rng, width_choices):
    """A width and an offset for the array's head that the original accepts: on a 4-byte boundary when it swaps
    words."""
    width = rng.choice(width_choices)
    offset = 4 * rng.randrange(4) if width % 4 == 0 else rng.randrange(8)
    return width, offset


def make_array(rng, count, width, key_size):
    """count elements of width bytes: a key of key_size bytes from a small range, so that many compare equal, then
    random bytes that show where each element ended up."""
    keys = rng.choice([1, 2, 3, 8, 64, 256])
    order = rng.random()
    values = [rng.randrange(keys) for _ in range(count)]
    if order < 0.1:
        values.sort()
    elif order < 0.2:
        values.sort(reverse=True)
    data = bytearray()
    for value in values:
        data += value.to_bytes(key_size, "little") + rng.randbytes(width - key_size)
    return bytes(data)


def key_comparator(key_size, descending, scale):
    """Compares little-endian unsigned keys at the start of each element; the result's size varies, its sign is the
    comparison's. A key that is equal, the same element included, compares as 0."""

    def compare_keys(memory, a, b):
        x = int.from_bytes(memory.read(a, key_size), "little")
        y = int.from_bytes(memory.read(b, key_size), "little")
        result = (x > y) - (x < y)
        return -result * scale if descending else result * scale

    return Callback(compare_keys, "s32(ptr, ptr)")


def mix(value: int) -> int:
    """A 64-bit hash of a 64-bit value (SplitMix64's finalizer), for answers that are random but cheap to repeat."""
    value = (value + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    return value ^ (value >> 31)


ANSWERS = [-1, 0, 1, -0x80000000, 0x7FFFFFFF]


def random_comparator(seed):
    """Answers each call at random, from the seed and the number of calls before it, so that both sides get the same
    answers when they make the same calls; an element compared with itself is 0, as the original needs."""
    holder = []

    def compare_randomly(memory, a, b):
        if a == b:
            return 0
        value = mix(seed ^ len(holder[0].log))
        return ANSWERS[value % 6] if value % 6 < 5 else value >> 32

    holder.append(Callback(compare_randomly, "s32(ptr, ptr)"))
    return holder[0]


WIDTHS = [1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 20, 24, 36, 64]


def element_count(rng):
    """Mostly short arrays, since each comparison is a Python call on both sides; test_qsort_large takes long ones."""
    choice = rng.random()
    if choice < 0.55:
        return rng.randrange(41)
    if choice < 0.95:
        return rng.randrange(41, 257)
    return rng.randrange(257, 601)


def test_qsort_keys():
    """Sorting by a key with many ties, so that the order of equal elements shows, of 0 to 600 elements of 1 to 64
    bytes, at every head alignment the original accepts; the comparison calls must be the same, in the same order."""

    def generate(rng, lib):
        width, offset = element_layout(rng, WIDTHS)
        key_size = min(width, rng.choice([1, 2, 4]))
        count = element_count(rng)
        comparator = key_comparator(key_size, rng.random() < 0.3, rng.choice([1, 7, 0x10000]))
        data = make_array(rng, count, width, key_size)
        return Case({"array": rng.randbytes(offset) + data + rng.randbytes(8)},
                    [Ptr("array", offset), count, width, comparator, 0])

    return f"{run('MATH_QSort', 'void(ptr, u32, u32, fn, ptr)', generate, count=1000, seed='keys')} cases each build"


def test_qsort_random_comparisons():
    """A comparison function that answers at random, which takes every branch of the partition in every order and
    shows any difference in the calls at once."""

    def generate(rng, lib):
        width, offset = element_layout(rng, WIDTHS)
        count = element_count(rng)
        comparator = random_comparator(rng.getrandbits(64))
        return Case({"array": rng.randbytes(offset + count * width + 8)},
                    [Ptr("array", offset), count, width, comparator, 0])

    return f"{run('MATH_QSort', 'void(ptr, u32, u32, fn, ptr)', generate, count=1000, seed='random')} cases each build"


def test_qsort_edges():
    """Counts of 0 to 4, counts the original compares as negative (it sorts nothing), and a width of 0 with a
    comparison that never asks for a swap (one call)."""

    def generate(rng, lib):
        kind = rng.randrange(3)
        width, offset = element_layout(rng, WIDTHS)
        if kind == 0:
            count = rng.randrange(5)
            comparator = random_comparator(rng.getrandbits(64)) if rng.random() < 0.5 else key_comparator(1, False, 1)
            return Case({"array": rng.randbytes(offset + count * width + 8)},
                        [Ptr("array", offset), count, width, comparator, 0])
        if kind == 1:
            count = rng.choice([0x80000000, 0x80000001, 0xC0000000, 0xFFFFFFFE, 0xFFFFFFFF])
            return Case({"array": rng.randbytes(offset + 64)},
                        [Ptr("array", offset), count, width, random_comparator(rng.getrandbits(64)), 0])
        count = rng.choice([2, 3, rng.randrange(2, 1000), 0x7FFFFFFF])
        answer = rng.choice([0, -1, -0x80000000])
        comparator = Callback(lambda memory, a, b: answer, "s32(ptr, ptr)")
        return Case({"array": rng.randbytes(offset + 8)}, [Ptr("array", offset), count, 0, comparator, 0])

    return f"{run('MATH_QSort', 'void(ptr, u32, u32, fn, ptr)', generate, count=1000, seed='edges')} cases each build"


def test_qsort_stack_buffer():
    """A caller's stack buffer, of the size the original needs (8 bytes per bit of the count) or more: the ranges
    must be pushed into the same words in the same order, and nothing written past them."""

    def generate(rng, lib):
        width, offset = element_layout(rng, WIDTHS)
        key_size = min(width, rng.choice([1, 2, 4]))
        count = element_count(rng)
        if rng.random() < 0.5:
            comparator = key_comparator(key_size, rng.random() < 0.3, 1)
        else:
            comparator = random_comparator(rng.getrandbits(64))
        stack_offset = 4 * rng.randrange(4)
        stack = rng.randbytes(stack_offset + 8 * count.bit_length() + 4 * rng.randrange(4))
        head = Ptr("array", offset)
        case = Case({"array": rng.randbytes(offset) + make_array(rng, count, width, key_size) + rng.randbytes(8),
                     "stack": stack}, [head, count, width, comparator, Ptr("stack", stack_offset)])
        return SplitCase(case, case, "stack", head)

    return f"{run('MATH_QSort', 'void(ptr, u32, u32, fn, ptr)', generate, count=300, seed='stack')} cases each build"


def test_qsort_large():
    """Up to 4096 elements, with an ARM comparison function in the emulator and the same one in C on the host
    (no call log then, but the payloads show the order)."""
    stubs = {}

    def generate(rng, lib):
        if not stubs:
            stubs["byte"] = shared.emu.stub(COMPARE_BYTE_STUB)
            stubs["word"] = shared.emu.stub(COMPARE_WORD_STUB)
        kind = rng.choice(["byte", "word"])
        width, offset = element_layout(rng, [4, 8, 12, 16] if kind == "word" else WIDTHS)
        count = rng.randrange(601, 4097)
        key_size = 4 if kind == "word" else 1
        regions = {"array": rng.randbytes(offset) + make_array(rng, count, width, key_size) + rng.randbytes(8)}
        host = lib.address(f"asm_oracle_fx_compare_{kind}")
        return SplitCase(Case(regions, [Ptr("array", offset), count, width, stubs[kind], 0]),
                         Case(regions, [Ptr("array", offset), count, width, host, 0]))

    return f"{run('MATH_QSort', 'void(ptr, u32, u32, fn, ptr)', generate, count=60, seed='large')} cases each build"


# libcrypto

def rc4_context(rng) -> bytes:
    """A context: i, j, two bytes of padding and the state, a permutation as RC4 keeps it or any bytes."""
    state = list(range(256))
    rng.shuffle(state)
    s = bytes(state) if rng.random() < 0.7 else rng.randbytes(256)
    return bytes([rng.randrange(256), rng.randrange(256)]) + rng.randbytes(2) + s


def rc4_length(rng) -> int:
    choice = rng.random()
    if choice < 0.3:
        return rng.randrange(17)
    if choice < 0.8:
        return rng.randrange(513)
    if choice < 0.95:
        return rng.randrange(513, 4097)
    return rng.choice([255, 256, 257, 511, 512, 513])


def test_rc4_apart():
    """Lengths of 0 to 4096 with the context, input and output each at any byte offset."""

    def generate(rng, lib):
        length = rc4_length(rng)
        ctx, src, dst = rng.randrange(8), rng.randrange(8), rng.randrange(8)
        return Case({"ctx": rng.randbytes(ctx) + rc4_context(rng) + rng.randbytes(4),
                     "in": rng.randbytes(src + length + 4), "out": rng.randbytes(dst + length + 4)},
                    [Ptr("ctx", ctx), Ptr("in", src), length, Ptr("out", dst)])

    signature = "void(ptr, ptr, u32, ptr)"
    return f"{run('CRYPTO_RC4Encrypt', signature, generate, count=2000, seed='apart')} cases each build"


def test_rc4_overlapping():
    """The output over the input (in place, or shifted by up to 9 bytes either way), or the input or the output in
    the context itself, which the original's order of loads and stores decides."""

    def generate(rng, lib):
        length = rc4_length(rng)
        kind = rng.randrange(3)
        if kind == 0:
            src = rng.randrange(16, 24)
            dst = src + rng.choice([0, 0, rng.randrange(-9, 10)])
            return Case({"ctx": rc4_context(rng), "buf": rng.randbytes(length + 48)},
                        [Ptr("ctx"), Ptr("buf", src), length, Ptr("buf", dst)])
        # The context somewhere in a buffer that also holds the input or the output, or both
        length = min(length, 600)
        ctx = rng.randrange(8)
        size = ctx + 260 + length + 16
        buffer = bytearray(rng.randbytes(size))
        buffer[ctx:ctx + 260] = rc4_context(rng)
        inside = rng.randrange(-4, size - length + 4)
        other = rng.randrange(8)
        regions = {"buf": bytes(buffer), "other": rng.randbytes(other + length + 4)}
        if kind == 1:
            return Case(regions, [Ptr("buf", ctx), Ptr("other", other), length, Ptr("buf", inside)])
        return Case(regions, [Ptr("buf", ctx), Ptr("buf", inside), length,
                              Ptr("buf", rng.randrange(-4, size - length + 4))])

    signature = "void(ptr, ptr, u32, ptr)"
    return f"{run('CRYPTO_RC4Encrypt', signature, generate, count=2000, seed='overlapping')} cases each build"


def test_rc4_lengths_that_do_nothing():
    """Lengths above 0x80000000, which the original counts as negative: the context is stored back as it was and
    nothing else is touched."""

    def generate(rng, lib):
        length = rng.choice([0x80000001, 0x80000002, 0xC0000000, 0xFFFFFFFE, 0xFFFFFFFF, rng.randrange(0x80000001,
                                                                                                       1 << 32)])
        return Case({"ctx": rc4_context(rng), "in": rng.randbytes(16), "out": rng.randbytes(16)},
                    [Ptr("ctx"), Ptr("in"), length, Ptr("out")])

    signature = "void(ptr, ptr, u32, ptr)"
    return f"{run('CRYPTO_RC4Encrypt', signature, generate, count=500, seed='nothing')} cases each build"


if __name__ == "__main__":
    main([
        test_matrix_stores,
        test_matrix_copies,
        test_matrix_products,
        test_count_leading_zeros,
        test_sha1_block,
        test_qsort_keys,
        test_qsort_random_comparisons,
        test_qsort_edges,
        test_qsort_stack_buffer,
        test_qsort_large,
        test_rc4_apart,
        test_rc4_overlapping,
        test_rc4_lengths_that_do_nothing,
    ])
