"""MSL's runtime division on the host (_u32_div_f, port/hal/msl/fp_fastI.c) against the ROM's routine
(lib/msl/src/fp/fp_fastI.s), which the game's C calls by name: the field's lens flare data at every change of map and
the Unity Tower's visitors.

The ARM routine returns the quotient in r0 and the remainder in r1, which the decomp's declaration (include/system/
aeabi.h) reads as one u64, the quotient in its low half; the harness reads the original's r0 and r1 the same way. A
divisor of 0, which C leaves undefined, gives what the DS's gives: the routine returns at once with the dividend and 0.
The inputs are every divisor of 0 and 1, powers of two and the values around them, dividends smaller than, equal to and
just above the divisor, the edges of 32 bits, and random pairs of every bit length.
"""

from build import build_library
from compare import Case, compare, main
from harness import Emulator

SOURCES = ["port/hal/msl/fp_fastI.c"]
NAME = "_u32_div_f"
SIGNATURE = "u64(u32, u32)"
EDGES = [0, 1, 2, 3, 4, 5, 7, 8, 9, 0x7FFFFFFF, 0x80000000, 0x80000001, 0xFFFFFFFE, 0xFFFFFFFF]


class Lazy:
    """The emulator and the build, made on first use, so that a missing ROM skips before either is needed."""

    def __getattr__(self, name):
        if name == "emu":
            self.emu = Emulator()
            return self.emu
        if name == "lib":
            self.lib = build_library("msl_hal", SOURCES)
            return self.lib
        raise AttributeError(name)


shared = Lazy()


def word(rng) -> int:
    """A value of a random bit length, or one around a power of two."""
    bits = rng.randrange(33)
    value = rng.getrandbits(bits) if bits else 0
    if rng.randrange(4) == 0 and bits:
        value = ((1 << (bits - 1)) + rng.choice([-1, 0, 1])) & 0xFFFFFFFF
    return value


def test_edges():
    """Every pair of the edge values, divisor 0 among them, each a case of its own."""
    count = 0
    for dividend in EDGES:
        for divisor in EDGES:
            def generate(rng, pair=(dividend, divisor)):
                return Case({}, list(pair))

            count += compare(shared.emu, shared.lib, NAME, SIGNATURE, generate, count=1,
                             seed=f"edges {dividend:#x} {divisor:#x}")
    return f"{count} cases"


def test_divisor_zero():
    """A divisor of 0 with random dividends: the dividend comes back, with a remainder of 0."""

    def generate(rng):
        return Case({}, [word(rng), 0])

    return f"{compare(shared.emu, shared.lib, NAME, SIGNATURE, generate, count=500, seed='zero')} cases"


def test_near_the_divisor():
    """Dividends just below, at and just above a multiple of the divisor, and smaller than it."""

    def generate(rng):
        divisor = word(rng) or 1
        multiple = rng.randrange(0, 0xFFFFFFFF // divisor + 1)
        dividend = (multiple * divisor + rng.choice([-1, 0, 1])) & 0xFFFFFFFF
        if rng.randrange(4) == 0:
            dividend = rng.randrange(divisor)
        return Case({}, [dividend, divisor])

    return f"{compare(shared.emu, shared.lib, NAME, SIGNATURE, generate, count=4000, seed='near')} cases"


def test_random():
    """Random pairs of every bit length."""

    def generate(rng):
        return Case({}, [word(rng), word(rng)])

    return f"{compare(shared.emu, shared.lib, NAME, SIGNATURE, generate, count=4000, seed='random')} cases"


if __name__ == "__main__":
    main([test_edges, test_divisor_zero, test_near_the_divisor, test_random])
