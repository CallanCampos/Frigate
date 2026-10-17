"""The decomp's C of libfx's matrix products, matrix scalings and vector arithmetic (lib/nitro/src/fx), which the host
compiles and the ROM doesn't link yet (the files aren't complete), against the ROM's code.

Each function runs on seeded random inputs with its result written apart from its inputs, over each input, and over
all of them when the inputs are the same, since the game makes such calls: MCSSSys_Draw multiplies its view by the
projection into the view, GFL's MAT3_RotationEulerZYX multiplies rotations into its result, and NitroSystem's SBC and
SPL concatenate and transform in place. The ROM's code loads each row (or the vector) of its first input before it
stores that row of the result, so these calls give the same result as calls into a matrix of their own; the host must
too. Results that overlap their inputs at other offsets, which no caller makes, aren't tried.

The files are built with the game's flags only (build.py gives a decomp file its own build statement's): their
products wrap in 64 bits as the ROM's SMULL and SMLAL do, which the game's -fwrapv allows and a strict build would
trap on.
"""

from build import build_library
from compare import Case, Ptr, compare, main
from harness import Emulator
from test_fx import fx32

SOURCES = [
    "lib/nitro/src/fx/fx_mtx22.c",
    "lib/nitro/src/fx/fx_mtx33.c",
    "lib/nitro/src/fx/fx_mtx43.c",
    "lib/nitro/src/fx/fx_mtx44.c",
    "lib/nitro/src/fx/fx_vec.c",
]
COUNT = 1500


class Lazy:
    """The emulator and the library, made on first use, so that a missing ROM skips before either is needed."""

    def __getattr__(self, name):
        if name == "emu":
            self.emu = Emulator(max_instructions=50_000_000)
            return self.emu
        if name == "lib":
            self.lib = build_library("fx_decomp", SOURCES)
            return self.lib
        raise AttributeError(name)


shared = Lazy()


def values(rng, count: int, width: int = 4) -> bytes:
    """count fixed point values of width bytes."""
    return b"".join((fx32(rng) & ((1 << (8 * width)) - 1)).to_bytes(width, "little") for _ in range(count))


def placements(first: int, second: int, result: int, width: int = 4):
    """Generators of (regions, first, second, result pointers) for a function of two inputs of first and second words
    and a result of result words: apart, the result over either input, and the inputs the same, with the result apart
    or over them both (only when the two inputs have one size). Each region starts at a random word offset."""

    def offset(rng):
        return width * rng.randrange(4)

    def apart(rng):
        a, b, r = offset(rng), offset(rng), offset(rng)
        return ({"a": rng.randbytes(a) + values(rng, first, width) + rng.randbytes(8),
                 "b": rng.randbytes(b) + values(rng, second, width) + rng.randbytes(8),
                 "r": rng.randbytes(r + result * width + 8)},
                Ptr("a", a), Ptr("b", b), Ptr("r", r))

    def over_first(rng):
        a, b = offset(rng), offset(rng)
        return ({"a": rng.randbytes(a) + values(rng, max(first, result), width) + rng.randbytes(8),
                 "b": rng.randbytes(b) + values(rng, second, width) + rng.randbytes(8)},
                Ptr("a", a), Ptr("b", b), Ptr("a", a))

    def over_second(rng):
        a, b = offset(rng), offset(rng)
        return ({"a": rng.randbytes(a) + values(rng, first, width) + rng.randbytes(8),
                 "b": rng.randbytes(b) + values(rng, max(second, result), width) + rng.randbytes(8)},
                Ptr("a", a), Ptr("b", b), Ptr("b", b))

    def same_inputs(rng):
        a, r = offset(rng), offset(rng)
        return ({"a": rng.randbytes(a) + values(rng, first, width) + rng.randbytes(8),
                 "r": rng.randbytes(r + result * width + 8)},
                Ptr("a", a), Ptr("a", a), Ptr("r", r))

    def all_same(rng):
        a = offset(rng)
        return ({"a": rng.randbytes(a) + values(rng, max(first, result), width) + rng.randbytes(8)},
                Ptr("a", a), Ptr("a", a), Ptr("a", a))

    found = {"apart": apart, "over the first input": over_first, "over the second input": over_second}
    if first == second:
        found["inputs the same"] = same_inputs
        if first == result:
            found["all the same"] = all_same
    return found


def check(function, first, second, result, *, width=4):
    """Runs function(first, second, result) on every placement."""
    total = 0
    for name, place in placements(first, second, result, width).items():
        def generate(rng, place=place):
            regions, a, b, r = place(rng)
            return Case(regions, [a, b, r], note=name)

        total += compare(shared.emu, shared.lib, function, "void(ptr, ptr, ptr)", generate, count=COUNT,
                         seed=f"{function}:{name}")
    return total


def test_products():
    """MTX_Concat33, MTX_Concat43 and MTX_Concat44: a times b into ab."""
    total = 0
    for function, words in (("MTX_Concat33", 9), ("MTX_Concat43", 12), ("MTX_Concat44", 16)):
        total += check(function, words, words, words)
    return f"3 functions, {total} cases"


def test_transforms():
    """MTX_MultVec33 and MTX_MultVec43: a vector through a matrix, into a vector of its own and in place."""
    total = 0
    for function, words in (("MTX_MultVec33", 9), ("MTX_MultVec43", 12)):
        for name, in_place in (("apart", False), ("in place", True)):
            def generate(rng, words=words, in_place=in_place):
                v, m, d = (4 * rng.randrange(4) for _ in range(3))
                regions = {"vec": rng.randbytes(v) + values(rng, 3) + rng.randbytes(8),
                           "mtx": rng.randbytes(m) + values(rng, words) + rng.randbytes(8)}
                if in_place:
                    return Case(regions, [Ptr("vec", v), Ptr("mtx", m), Ptr("vec", v)])
                regions["dest"] = rng.randbytes(d + 20)
                return Case(regions, [Ptr("vec", v), Ptr("mtx", m), Ptr("dest", d)])

            total += compare(shared.emu, shared.lib, function, "void(ptr, ptr, ptr)", generate, count=COUNT,
                             seed=f"{function}:{name}")
    return f"2 functions, {total} cases"


def test_scalings():
    """MTX_ScaleApply22, MTX_ScaleApply33 and MTX_ScaleApply43: a matrix's rows scaled, apart and in place."""
    total = 0
    for function, words, factors in (("MTX_ScaleApply22", 4, 2), ("MTX_ScaleApply33", 9, 3),
                                     ("MTX_ScaleApply43", 12, 3)):
        signature = "void(ptr, ptr" + ", s32" * factors + ")"
        for name, in_place in (("apart", False), ("in place", True)):
            def generate(rng, words=words, factors=factors, in_place=in_place):
                s, d = 4 * rng.randrange(4), 4 * rng.randrange(4)
                regions = {"src": rng.randbytes(s) + values(rng, words) + rng.randbytes(8)}
                scale = [fx32(rng) for _ in range(factors)]
                if in_place:
                    return Case(regions, [Ptr("src", s), Ptr("src", s)] + scale)
                regions["dest"] = rng.randbytes(d + 4 * words + 8)
                return Case(regions, [Ptr("src", s), Ptr("dest", d)] + scale)

            total += compare(shared.emu, shared.lib, function, signature, generate, count=COUNT,
                             seed=f"{function}:{name}")
    return f"3 functions, {total} cases"


def test_vectors():
    """VEC_Add, VEC_Subtract and VEC_CrossProduct on VecFx32, VEC_Fx16Add and VEC_Fx16CrossProduct on VecFx16, and
    VEC_MultAdd's scale times a plus b."""
    total = 0
    for function in ("VEC_Add", "VEC_Subtract", "VEC_CrossProduct"):
        total += check(function, 3, 3, 3)
    for function in ("VEC_Fx16Add", "VEC_Fx16CrossProduct"):
        total += check(function, 3, 3, 3, width=2)
    for name, place in placements(3, 3, 3).items():
        def generate(rng, place=place):
            regions, a, b, r = place(rng)
            return Case(regions, [fx32(rng), a, b, r], note=name)

        total += compare(shared.emu, shared.lib, "VEC_MultAdd", "void(s32, ptr, ptr, ptr)", generate, count=COUNT,
                         seed=f"VEC_MultAdd:{name}")
    return f"6 functions, {total} cases"


def test_dot_products():
    """VEC_DotProduct and VEC_Fx16DotProduct, of two vectors and of a vector with itself."""
    total = 0
    for function, width in (("VEC_DotProduct", 4), ("VEC_Fx16DotProduct", 2)):
        for name, same in (("apart", False), ("the same", True)):
            def generate(rng, width=width, same=same):
                a, b = width * rng.randrange(4), width * rng.randrange(4)
                regions = {"a": rng.randbytes(a) + values(rng, 3, width) + rng.randbytes(8)}
                if same:
                    return Case(regions, [Ptr("a", a), Ptr("a", a)])
                regions["b"] = rng.randbytes(b) + values(rng, 3, width) + rng.randbytes(8)
                return Case(regions, [Ptr("a", a), Ptr("b", b)])

            total += compare(shared.emu, shared.lib, function, "s32(ptr, ptr)", generate, count=COUNT,
                             seed=f"{function}:{name}")
    return f"2 functions, {total} cases"


if __name__ == "__main__":
    main([
        test_products,
        test_transforms,
        test_scalings,
        test_vectors,
        test_dot_products,
    ])
