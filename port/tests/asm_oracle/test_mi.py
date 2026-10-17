"""Differential tests of port/hal/mi, the host versions of NitroSDK's MI routines that the SDK wrote in assembly
(lib/nitro/src/mi/mi_memory.c, mi_swap.c and mi_uncompress.c), against the ROM's original code in the emulator.

Every function gets thousands of seeded random cases: sizes from 0 to 512 and beyond, with the sizes around each
unrolled block (multiples of 32, and 0 to 40) drawn more often; every alignment of the source and the destination
that the function accepts; overlapping buffers, with the destination before and after the source; and edge values.
The byte copies and fills also get every combination of small sizes and alignments. MI_UncompressLZ8 is checked on
random streams of both formats, on data compressed by a reference encoder written here, on compressed members of the
ROM's own NARC archives, and decompressing in place.

The halfword and word functions get pointers aligned to their unit only: the ARM9's handling of an unaligned address
isn't something the emulator reproduces (harness.UnalignedAccess), nor do the host versions.

MIi_CpuSend32 and MI_Copy*B also get a geometry command port, as the game gives them GXFIFO: every word the original
writes to the port is compared with the log that the host's port/hal/hw keeps of the port's writes.

A failure names the case to run again: pass only=N to the run() or run_all() call of that test.
"""

from __future__ import annotations

import ctypes
import random
import struct

from build import build_library
from compare import Case, Mismatch, Ptr, compare, main
from harness import REPO, Emulator, default_version

SOURCES = ["port/hal/mi/mi_memory.c", "port/hal/mi/mi_swap.c", "port/hal/mi/mi_uncompress.c"]
COUNT = 2000


class Lazy:
    """The emulator and the library, made on first use, so that a missing ROM skips before either is needed."""

    def __getattr__(self, name):
        if name == "emu":
            self.emu = Emulator()
            return self.emu
        if name == "lib":
            self.lib = build_library("mi", SOURCES)
            return self.lib
        raise AttributeError(name)


shared = Lazy()


def run(function, signature, generate, *, count=COUNT, seed=0, only=None) -> int:
    return compare(shared.emu, shared.lib, function, signature, generate, count=count, seed=seed, only=only)


def enumerated(cases: list[Case], start: int = 0):
    """A generator for compare() that returns cases[0], cases[1], ... in turn, for tests that go through every
    combination; start is compare()'s only, when one case is run again."""
    position = [start]

    def generate(_rng):
        case = cases[position[0]]
        position[0] += 1
        return case

    return generate


def run_all(function, signature, cases: list[Case], *, only=None) -> int:
    return compare(shared.emu, shared.lib, function, signature, enumerated(cases, only or 0), count=len(cases),
                   seed="every", only=only)


# Inputs


def size_for(rng, limit=512) -> int:
    """A size: small (every unrolled path), near a multiple of 32 (the block loops' edges), any up to limit, or
    larger."""
    choice = rng.random()
    if choice < 0.3:
        return rng.randrange(41)
    if choice < 0.55:
        return max(0, 32 * rng.randrange(limit // 32 + 1) + rng.randrange(-4, 5))
    if choice < 0.93:
        return rng.randrange(limit + 1)
    return rng.randrange(limit, 8 * limit + 1)


def value_for(rng) -> int:
    """A fill value: any word (the fills mix the bits above the low byte into the other bytes), a byte, or an edge."""
    choice = rng.random()
    if choice < 0.45:
        return rng.randrange(1 << 32)
    if choice < 0.8:
        return rng.randrange(256)
    return rng.choice([0, 0xFF, 0x100, 0x1FF, 0x8000, 0xFFFF, 0x10000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF,
                       0x12345678])


def copy_pointers(rng, size: int, align: int, overlap: bool):
    """The regions and the source and destination of a copy of size bytes, at offsets that are multiples of align:
    separate buffers, or one buffer with the destination some bytes before or after the source, which overlap when
    they are fewer than size apart."""
    offsets = range(0, 4, align)
    if not overlap:
        regions = {"src": rng.randbytes(size + 8), "dst": rng.randbytes(size + 8)}
        return regions, Ptr("src", rng.choice(offsets)), Ptr("dst", rng.choice(offsets))
    if rng.random() < 0.6:
        delta = align * rng.randrange(-40 // align, 40 // align + 1)
    else:
        delta = align * (rng.randrange(-(size + 8), size + 9) // align)
    src = rng.choice(offsets) + max(0, -delta)
    dst = src + delta
    return {"buf": rng.randbytes(max(src, dst) + size + 8)}, Ptr("buf", src), Ptr("buf", dst)


def copy_generator(align: int, overlap: bool, limit=512):
    def generate(rng):
        size = size_for(rng, limit)
        regions, src, dst = copy_pointers(rng, size, align, overlap)
        return Case(regions, [src, dst, size])

    return generate


def every_copy(sizes, deltas) -> list[Case]:
    """Every source and destination alignment and every size of sizes, in separate buffers; then every source
    alignment, every distance of deltas and every size, in one buffer."""
    rng = random.Random("every copy")
    cases = []
    for size in sizes:
        for src in range(4):
            for dst in range(4):
                regions = {"src": rng.randbytes(size + 8), "dst": rng.randbytes(size + 8)}
                cases.append(Case(regions, [Ptr("src", src), Ptr("dst", dst), size]))
    for size in sizes:
        for delta in deltas:
            for offset in range(4):
                src = offset + max(0, -delta)
                cases.append(Case({"buf": rng.randbytes(max(src, src + delta) + size + 8)},
                                  [Ptr("buf", src), Ptr("buf", src + delta), size]))
    return cases


def huge_sizes(rng) -> int:
    """A size of 2 GiB or more, a negative s32, for which the halfword and word loops do nothing."""
    return rng.choice([0x80000000, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFE0,
                       rng.randrange(0x80000000, 1 << 32)])


# The halfword and word functions


def check_units(function: str, unit: int, fill: bool) -> str:
    """A halfword or word copy or fill: separate and overlapping buffers, and sizes of 2 GiB or more."""
    total = 0
    if fill:
        def generate(rng):
            size = size_for(rng)
            dst = unit * rng.randrange(4 // unit)
            return Case({"dst": rng.randbytes(size + 8)}, [value_for(rng), Ptr("dst", dst), size])

        def generate_huge(rng):
            return Case({"dst": rng.randbytes(64)}, [value_for(rng), Ptr("dst"), huge_sizes(rng)])

        signature = f"void({'u16' if unit == 2 else 'u32'}, ptr, u32)"
        total += run(function, signature, generate)
        total += run(function, signature, generate_huge, count=50, seed=1)
    else:
        def generate_huge(rng):
            return Case({"src": rng.randbytes(64), "dst": rng.randbytes(64)}, [Ptr("src"), Ptr("dst"), huge_sizes(rng)])

        total += run(function, "void(ptr, ptr, u32)", copy_generator(unit, False))
        total += run(function, "void(ptr, ptr, u32)", copy_generator(unit, True), seed=1)
        total += run(function, "void(ptr, ptr, u32)", generate_huge, count=50, seed=2)
    return f"{total} cases"


def test_cpu_clear16():
    return check_units("MIi_CpuClear16", 2, True)


def test_cpu_copy16():
    return check_units("MIi_CpuCopy16", 2, False)


def test_cpu_clear32():
    return check_units("MIi_CpuClear32", 4, True)


def test_cpu_copy32():
    return check_units("MIi_CpuCopy32", 4, False)


def test_cpu_clear_fast():
    return check_units("MIi_CpuClearFast", 4, True)


def test_cpu_copy_fast():
    return check_units("MIi_CpuCopyFast", 4, False)


def test_cpu_send32():
    """Every word goes to the same address. The comparison sees only the memory each side leaves, so it shows the last
    word sent; a port inside the source shows more of the order of the loads and stores, but at the source's last word
    only that the last two words are sent in order (the word before is stored there before the last is loaded).
    test_geometry_port compares every word sent, in order."""
    total = 0

    def generate(rng):
        size = size_for(rng)
        return Case({"src": rng.randbytes(size + 8), "port": rng.randbytes(16)},
                    [Ptr("src"), Ptr("port", 4 * rng.randrange(4)), size])

    def generate_inside(rng):
        size = size_for(rng, 128)
        words = (size + 3) // 4
        port = 32 + 4 * (words - 1) if words and rng.random() < 0.4 else 4 * rng.randrange(words + 16)
        return Case({"src": rng.randbytes(size + 64)}, [Ptr("src", 32), Ptr("src", port), size])

    def generate_huge(rng):
        return Case({"src": rng.randbytes(64), "port": rng.randbytes(16)}, [Ptr("src"), Ptr("port"), huge_sizes(rng)])

    total += run("MIi_CpuSend32", "void(ptr, ptr, u32)", generate)
    total += run("MIi_CpuSend32", "void(ptr, ptr, u32)", generate_inside, seed=1)
    total += run("MIi_CpuSend32", "void(ptr, ptr, u32)", generate_huge, count=50, seed=2)
    return f"{total} cases"


def test_copy_fixed():
    """MI_Copy32B, 36B, 48B and 64B: word-aligned, separate and overlapping."""
    total = 0
    for name, size in (("MI_Copy32B", 32), ("MI_Copy36B", 36), ("MI_Copy48B", 48), ("MI_Copy64B", 64)):
        def generate(rng, size=size):
            regions, src, dst = copy_pointers(rng, size, 4, rng.random() < 0.5)
            return Case(regions, [src, dst])

        total += run(name, "void(ptr, ptr)", generate, count=1000)
    return f"{total} cases"


class PortG3Write(ctypes.Structure):
    """A write that port/hal/hw logged to a geometry command port: the port's DS address and the word."""

    _fields_ = [("port", ctypes.c_uint32), ("value", ctypes.c_uint32)]


GXFIFO = 0x04000400
# GXFIFO's mirrors, which take every write as GXFIFO does; hw logs them as GXFIFO
GXFIFO_END = 0x04000440
# The command ports, from the first after GXFIFO's mirrors to GXSTAT
COMMAND_PORTS = (0x04000440, 0x04000600)


def test_geometry_port():
    """MIi_CpuSend32 and MI_Copy*B to a geometry port: GXFIFO, as NNS's command buffer and G3_MultMtx33 give it to
    them, and the command ports. Every write the original makes to the port and the ones after it is compared with
    the log that the host's port/hal/hw keeps, given the port as the game gives it: the entry that evaluating
    reg_G3X_GXFIFO or a reg_G3_* register logs. A send writes each word to the port, none for a size of 0 or of 2 GiB
    or more; a copy's STM writes to the ports that follow it, which from GXFIFO are its mirrors."""
    emu, lib = shared.emu, shared.lib
    take = lib.function("port_hw_g3_take", ctypes.POINTER(PortG3Write), ctypes.POINTER(ctypes.c_size_t))
    gxfifo = lib.function("port_hw_gxfifo", ctypes.c_void_p)
    g3_port = lib.function("port_hw_g3_port", ctypes.c_void_p, ctypes.c_uint32)
    send = lib.function("MIi_CpuSend32", None, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32)
    copies = {name: lib.function(name, None, ctypes.c_void_p, ctypes.c_void_p)
              for name in ("MI_Copy32B", "MI_Copy36B", "MI_Copy48B", "MI_Copy64B")}

    def logged() -> list[tuple[int, int, int]]:
        count = ctypes.c_size_t()
        log = take(ctypes.byref(count))
        return [(log[i].port, 4, log[i].value) for i in range(count.value)]

    def check(function: str, port: int, data: bytes, size: int | None = None):
        sizes = [] if size is None else [size]
        emu.reset_scratch()
        src = emu.alloc(len(data), data=data)
        with emu.record_writes(port, port + 64) as writes:
            emu.call(function, src, port, *sizes)
        original = [(GXFIFO if GXFIFO <= w.address < GXFIFO_END else w.address, w.size, w.value) for w in writes]
        logged()
        buffer = ctypes.create_string_buffer(data, len(data))
        slot = gxfifo() if port == GXFIFO else g3_port(port)
        if size is None:
            copies[function](buffer, slot)
        else:
            send(buffer, slot, size)
        host = logged()
        if host != original:
            first = next((i for i, (a, b) in enumerate(zip(original, host)) if a != b), min(len(original), len(host)))
            raise Mismatch(
                f"{function} to {port:#010x} ({len(data)} bytes of source{'' if size is None else f', size {size}'}): "
                f"the original writes {len(original)} words and the host logs {len(host)}; from write {first}, the "
                f"original {[(hex(p), hex(v)) for p, _, v in original[first:first + 4]]}, "
                f"the host {[(hex(p), hex(v)) for p, _, v in host[first:first + 4]]}")

    def port_for(rng, span: int) -> int:
        """GXFIFO, or a command port that the span of bytes written from it doesn't take past the last."""
        if rng.random() < 0.6:
            return GXFIFO
        return COMMAND_PORTS[0] + 4 * rng.randrange((COMMAND_PORTS[1] - COMMAND_PORTS[0] - span) // 4 + 1)

    rng = random.Random("geometry port")
    total = 0
    for _ in range(1000):
        size = size_for(rng) if rng.random() < 0.95 else huge_sizes(rng)
        data = rng.randbytes(min(size, 8 * 512) + 4)
        check("MIi_CpuSend32", port_for(rng, 4), data, size)
        total += 1
    for name, size in (("MI_Copy32B", 32), ("MI_Copy36B", 36), ("MI_Copy48B", 48), ("MI_Copy64B", 64)):
        for _ in range(200):
            check(name, port_for(rng, size), rng.randbytes(size))
            total += 1
    return f"{total} cases"


def test_zero36b():
    """Thumb."""

    def generate(rng):
        return Case({"dst": rng.randbytes(48)}, [Ptr("dst", 4 * rng.randrange(3))])

    return f"{run('MI_Zero36B', 'void(ptr)', generate, count=200)} cases"


def test_swap_word():
    def generate(rng):
        return Case({"word": rng.randbytes(16)}, [value_for(rng), Ptr("word", 4 * rng.randrange(4))])

    return f"{run('MI_SwapWord', 'u32(u32, ptr)', generate, count=1000)} cases"


# The byte functions


def test_cpu_fill8():
    total = 0

    def generate(rng):
        size = size_for(rng)
        return Case({"dst": rng.randbytes(size + 16)}, [Ptr("dst", rng.randrange(8)), value_for(rng), size])

    cases = []
    rng = random.Random("every fill8")
    for size in range(73):
        for dst in range(8):
            for value in (0x5A, 0x1234ABCD):
                cases.append(Case({"dst": rng.randbytes(size + 16)}, [Ptr("dst", dst), value, size]))
    total += run("MI_CpuFill8", "void(ptr, u32, u32)", generate, count=4000)
    total += run_all("MI_CpuFill8", "void(ptr, u32, u32)", cases)
    return f"{total} cases"


def test_cpu_fill():
    total = 0

    def generate(rng):
        size = size_for(rng)
        return Case({"dst": rng.randbytes(size + 16)}, [Ptr("dst", rng.randrange(8)), value_for(rng), size])

    cases = []
    rng = random.Random("every fill")
    for size in range(81):
        for dst in range(8):
            for value in (0xA5, 0x89ABCDEF):
                cases.append(Case({"dst": rng.randbytes(size + 16)}, [Ptr("dst", dst), value, size]))
    total += run("MI_CpuFill", "void(ptr, u32, u32)", generate, count=4000)
    total += run_all("MI_CpuFill", "void(ptr, u32, u32)", cases)
    return f"{total} cases"


def check_byte_copy(function: str) -> str:
    total = 0
    total += run(function, "void(ptr, ptr, u32)", copy_generator(1, False), count=3000)
    total += run(function, "void(ptr, ptr, u32)", copy_generator(1, True), count=4000, seed=1)
    total += run_all(function, "void(ptr, ptr, u32)", every_copy(range(73), range(-9, 10)))
    return f"{total} cases"


def test_cpu_copy8():
    return check_byte_copy("MI_CpuCopy8")


def test_cpu_copy():
    return check_byte_copy("MI_CpuCopy")


def test_cpu_move():
    return check_byte_copy("MI_CpuMove")


def test_cpu_comp8():
    """Equal and unequal buffers, the first difference anywhere, any alignment, and a buffer against itself."""
    total = 0

    def generate(rng):
        size = size_for(rng)
        data = rng.randbytes(size)
        other = bytearray(data)
        if size and rng.random() < 0.75:
            index = rng.randrange(size) if rng.random() < 0.7 else size - 1
            other[index] = (other[index] + rng.randrange(1, 256)) & 0xFF
            for later in range(index + 1, size):
                if rng.random() < 0.3:
                    other[later] = rng.randrange(256)
        a, b = rng.randrange(4), rng.randrange(4)
        return Case({"a": rng.randbytes(a) + data + rng.randbytes(4), "b": rng.randbytes(b) + bytes(other) +
                     rng.randbytes(4)}, [Ptr("a", a), Ptr("b", b), size])

    def generate_self(rng):
        size = size_for(rng)
        data = bytes(rng.choice([0, 0xFF, rng.randrange(256)]) for _ in range(size + 48))
        return Case({"buf": data}, [Ptr("buf", rng.randrange(4)), Ptr("buf", rng.randrange(48)), size])

    total += run("MI_CpuComp8", "s32(ptr, ptr, u32)", generate, count=3000)
    total += run("MI_CpuComp8", "s32(ptr, ptr, u32)", generate_self, count=1000, seed=1)
    return f"{total} cases"


# MI_UncompressLZ8


def lz_decode(stream: bytes, *, base: int = 0):
    """The reference decoder, as the format is documented: the output, the number of bytes of stream read, and the
    size from the header; or None when the stream is cut short or refers to bytes before the output's start."""
    if len(stream) < 4:
        return None
    header = struct.unpack_from("<I", stream)[0]
    extended = header & 0xF != 0
    remaining = header >> 8
    out = bytearray()
    s = 4
    try:
        while remaining > 0:
            flags = stream[s]
            s += 1
            for _ in range(8):
                if remaining <= 0:
                    break
                if not flags & 0x80:
                    out.append(stream[s])
                    s += 1
                    remaining -= 1
                else:
                    first = stream[s]
                    if not extended:
                        length = 3
                    elif first & 0xE0:
                        length = 1
                    elif not first & 0x10:
                        length = 0x11 + ((first & 0xF) << 4)
                        s += 1
                    else:
                        length = 0x111 + ((first & 0xF) << 12) + (stream[s + 1] << 4)
                        s += 2
                    length += stream[s] >> 4
                    distance = ((stream[s] & 0xF) << 8 | stream[s + 1]) + 1
                    s += 2
                    if distance > len(out) + base:
                        return None
                    for _ in range(length):
                        out.append(out[-distance])
                    remaining -= length
                flags <<= 1
    except IndexError:
        return None
    return bytes(out), s, header >> 8


def lz_encode(data: bytes, extended: bool, *, window: int = 0x1000, tries: int = 24) -> bytes:
    """A reference LZ77 encoder: greedy, with hash chains over three-byte prefixes. extended chooses type 0x11, whose
    copies run to 0x10110 bytes, over type 0x10, whose copies are 3 to 18 bytes."""
    longest = 0x10110 if extended else 18
    out = bytearray(struct.pack("<I", len(data) << 8 | (0x11 if extended else 0x10)))
    chains: dict[bytes, list[int]] = {}
    i = 0

    def remember(position):
        if position + 3 <= len(data):
            chains.setdefault(data[position:position + 3], []).append(position)

    while i < len(data):
        flag_at = len(out)
        out.append(0)
        for bit in range(8):
            if i >= len(data):
                break
            best, best_from = 0, 0
            for start in reversed(chains.get(data[i:i + 3], [])[-tries:]):
                if i - start > window:
                    break
                length = 0
                while length < longest and i + length < len(data) and data[start + length] == data[i + length]:
                    length += 1
                if length > best:
                    best, best_from = length, start
            if best >= 3:
                distance = i - best_from - 1
                out[flag_at] |= 0x80 >> bit
                if not extended:
                    out += bytes([(best - 3) << 4 | distance >> 8, distance & 0xFF])
                elif best <= 0x10:
                    out += bytes([(best - 1) << 4 | distance >> 8, distance & 0xFF])
                elif best <= 0x110:
                    rest = best - 0x11
                    out += bytes([rest >> 4, (rest & 0xF) << 4 | distance >> 8, distance & 0xFF])
                else:
                    rest = best - 0x111
                    out += bytes([0x10 | rest >> 12, (rest >> 4) & 0xFF, (rest & 0xF) << 4 | distance >> 8,
                                  distance & 0xFF])
                for position in range(i, i + best):
                    remember(position)
                i += best
            else:
                out.append(data[i])
                remember(i)
                i += 1
    return bytes(out)


def random_stream(rng, *, longest=2000) -> tuple[bytes, int]:
    """A random valid stream of a random format, and how many bytes decompressing it writes: every copy refers to
    bytes already written, and the last block may run past the size, as the original allows."""
    extended = rng.random() < 0.6
    if extended:
        kind = 0x11 if rng.random() < 0.8 else rng.randrange(256) | rng.randrange(1, 16)
    else:
        kind = 0x10 if rng.random() < 0.8 else rng.randrange(256) & 0xF0
    size = rng.choice([0, 1, 2, 3, 4, rng.randrange(64), rng.randrange(2048), rng.randrange(2048)])
    out = bytearray(struct.pack("<I", size << 8 | kind))
    written, remaining = 0, size
    literal_chance = rng.random()
    while remaining > 0:
        flag_at = len(out)
        out.append(rng.randrange(256))
        for bit in range(8):
            if remaining <= 0:
                break
            if written == 0 or rng.random() < literal_chance:
                out[flag_at] &= ~(0x80 >> bit) & 0xFF
                out.append(rng.randrange(256))
                written += 1
                remaining -= 1
                continue
            out[flag_at] |= 0x80 >> bit
            limit = min(written, 0x1000)
            distance = rng.randrange(1, limit + 1) if rng.random() < 0.5 else rng.randrange(1, min(limit, 8) + 1)
            form = rng.random()
            if not extended:
                length = rng.randrange(3, 19)
                out += bytes([(length - 3) << 4 | (distance - 1) >> 8, (distance - 1) & 0xFF])
            elif form < 0.6:
                length = rng.randrange(3, 17)
                out += bytes([(length - 1) << 4 | (distance - 1) >> 8, (distance - 1) & 0xFF])
            elif form < 0.9:
                length = rng.randrange(0x11, 0x111)
                rest = length - 0x11
                out += bytes([rest >> 4, (rest & 0xF) << 4 | (distance - 1) >> 8, (distance - 1) & 0xFF])
            else:
                length = rng.randrange(0x111, max(0x112, min(0x10111, longest)))
                rest = length - 0x111
                out += bytes([0x10 | rest >> 12, (rest >> 4) & 0xFF, (rest & 0xF) << 4 | (distance - 1) >> 8,
                              (distance - 1) & 0xFF])
            written += length
            remaining -= length
    return bytes(out), written


def lz_case(stream: bytes, written: int, rng, note="") -> Case:
    """A stream at a word boundary (the header is loaded as a word) and an output buffer at any alignment."""
    before = 4 * rng.randrange(2)
    return Case({"src": rng.randbytes(before) + stream, "dst": rng.randbytes(written + 8)},
                [Ptr("src", before), Ptr("dst", rng.randrange(4))], note)


def lz_in_place(rng, stream: bytes, written: int, spare: int, note="") -> Case | None:
    """A case that decompresses a stream to the start of the buffer it ends: the stream at a word boundary, ending
    spare bytes after the output's end (fewer than none when spare is negative). None when the output overwrites
    stream bytes before they are read and the stream that results is no longer valid (a copy from before the output,
    or more output than the buffer holds), which neither side can be judged on."""
    start = max(0, written + spare - len(stream)) & ~3
    buffer = bytearray(rng.randbytes(max(start + len(stream), written) + 8))
    buffer[start:start + len(stream)] = stream
    if not lz_model_in_place(bytearray(buffer), start):
        return None
    return Case({"buf": bytes(buffer)}, [Ptr("buf", start), Ptr("buf", 0)], note)


def lz_model_in_place(buffer: bytearray, start: int) -> bool:
    """Decompresses buffer[start:] to buffer[0:] in place, reading each stream byte when the format says to and writing
    each output byte in turn, and tells whether every read and write stayed inside the buffer and every copy inside
    the output."""
    s, d = start, 0
    try:
        header = struct.unpack_from("<I", buffer, s)[0]
        s += 4
        extended, remaining = header & 0xF != 0, header >> 8
        while remaining > 0:
            flags = buffer[s]
            s += 1
            for _ in range(8):
                if remaining <= 0:
                    break
                if not flags & 0x80:
                    buffer[d] = buffer[s]
                    s, d, remaining = s + 1, d + 1, remaining - 1
                else:
                    first = buffer[s]
                    if not extended:
                        length = 3
                    elif first & 0xE0:
                        length = 1
                    elif not first & 0x10:
                        length = 0x11 + ((first & 0xF) << 4)
                        s += 1
                    else:
                        length = 0x111 + ((first & 0xF) << 12) + (buffer[s + 1] << 4)
                        s += 2
                    length += buffer[s] >> 4
                    distance = ((buffer[s] & 0xF) << 8 | buffer[s + 1]) + 1
                    s += 2
                    if distance > d or d + length > len(buffer):
                        return False
                    for _ in range(length):
                        buffer[d] = buffer[d - distance]
                        d += 1
                    remaining -= length
                flags <<= 1
    except (IndexError, struct.error):
        return False
    return True


def run_lz(generate, *, count, seed=0, fast=False) -> int:
    """compare() for MI_UncompressLZ8; fast turns the alignment check off for long outputs (the code's only access
    wider than a byte is the header's, at a word boundary in every case here)."""
    emu = shared.emu
    emu.strict_alignment = not fast
    try:
        return compare(emu, shared.lib, "MI_UncompressLZ8", "void(ptr, ptr)", generate, count=count, seed=seed)
    finally:
        emu.strict_alignment = True


def test_uncompress_lz8_random():
    """Random valid streams of both formats, and of the other format types the original treats as one of them; the
    longest copies of the extended format once in a while."""
    total = 0

    def generate(rng):
        stream, written = random_stream(rng)
        return lz_case(stream, written, rng)

    def generate_longest(rng):
        stream, written = random_stream(rng, longest=0x10111)
        return lz_case(stream, written, rng)

    total += run_lz(generate, count=3000)
    total += run_lz(generate_longest, count=40, seed=1, fast=True)
    return f"{total} cases"


def test_uncompress_lz8_encoded():
    """Data compressed by the reference encoder, in both formats: random bytes, runs, repeats near and far, and the
    longest copies."""
    total = 0

    def data_for(rng) -> bytes:
        size = rng.choice([0, 1, 2, 3, rng.randrange(40), rng.randrange(600), rng.randrange(6000)])
        data = bytearray()
        while len(data) < size:
            choice = rng.random()
            if choice < 0.3 or not data:
                data += rng.randbytes(rng.randrange(1, 40))
            elif choice < 0.5:
                data += bytes([rng.randrange(256)]) * rng.randrange(1, 400)
            else:
                start = rng.randrange(max(0, len(data) - 0x1100), len(data))
                for _ in range(rng.randrange(1, 300)):
                    data.append(data[start] if rng.random() < 0.97 else rng.randrange(256))
                    start += 1
        return bytes(data[:size])

    def generate(rng):
        data = data_for(rng)
        stream = lz_encode(data, rng.random() < 0.6)
        decoded = lz_decode(stream)
        if decoded is None or decoded[0] != data:
            raise AssertionError("the reference encoder and decoder disagree")
        return lz_case(stream, len(data), rng)

    def generate_longest(rng):
        data = bytes([rng.randrange(256)]) * rng.randrange(0x10000, 0x12000) + rng.randbytes(rng.randrange(100))
        return lz_case(lz_encode(data, True), len(data), rng)

    total += run_lz(generate, count=600, fast=True)
    total += run_lz(generate_longest, count=4, seed=1, fast=True)
    return f"{total} cases"


def narc_members(data: bytes) -> list[bytes]:
    """The members of a NARC archive: its file allocation table (BTAF) gives each one's start and end in its file
    image (GMIF)."""
    if data[:4] != b"NARC":
        return []
    offset = struct.unpack_from("<H", data, 0x0C)[0]
    entries, image = [], None
    for _ in range(struct.unpack_from("<H", data, 0x0E)[0]):
        magic, size = data[offset:offset + 4], struct.unpack_from("<I", data, offset + 4)[0]
        if magic == b"BTAF":
            count = struct.unpack_from("<H", data, offset + 8)[0]
            entries = [struct.unpack_from("<II", data, offset + 12 + 8 * i) for i in range(count)]
        elif magic == b"GMIF":
            image = offset + 8
        offset += size
    return [data[image + start:image + end] for start, end in entries] if image is not None else []


def rom_lz_members(count: int, *, largest=0x20000) -> list[tuple[str, bytes, int]]:
    """A seeded sample of the ROM's NARC members that are LZ77 streams, a third of them of type 0x10 and the rest of
    type 0x11, each one that the reference decoder reads without running off the member: its name, stream and the
    number of bytes decompressing it writes."""
    files = REPO / "extract" / default_version() / "files"
    candidates = {0x10: [], 0x11: []}
    for path in sorted(files.rglob("*")):
        if not path.is_file():
            continue
        with open(path, "rb") as file:
            if file.read(4) != b"NARC":
                continue
        for index, member in enumerate(narc_members(path.read_bytes())):
            if len(member) >= 8 and member[0] in candidates and 0 < struct.unpack_from("<I", member)[0] >> 8 <= largest:
                candidates[member[0]].append((f"{path.relative_to(files)}[{index}]", member))
    rng = random.Random("rom lz members")
    sample = []
    for kind, wanted in ((0x10, count // 3), (0x11, count - count // 3)):
        pool = candidates[kind][:]
        rng.shuffle(pool)
        found = 0
        for name, member in pool:
            if found == wanted:
                break
            decoded = lz_decode(member)
            if decoded is not None:
                sample.append((name, member, len(decoded[0])))
                found += 1
    return sample


def test_uncompress_lz8_rom():
    """Compressed members of the ROM's own NARC archives, the whole of each decompressed and compared."""
    members = rom_lz_members(300)
    if not members:
        raise AssertionError("no LZ77 members found in the ROM's NARC archives")
    rng = random.Random("rom lz cases")
    cases = [lz_case(stream, written, rng, note=name) for name, stream, written in members]
    total = run_lz(enumerated(cases), count=len(cases), seed="rom", fast=True)
    largest = max(written for _, _, written in members)
    plain = sum(1 for _, stream, _ in members if stream[0] == 0x10)
    return f"{total} members ({plain} of type 0x10, the largest {largest} bytes)"


def test_uncompress_lz8_in_place():
    """Decompressing in place: the stream ending a little after the output's end, or a little before it, so that the
    output may overwrite stream bytes before they are read. Both sides must read and write in the same order to agree;
    a case whose stream that makes invalid is drawn again."""
    total = 0
    tight = 0

    def generate(rng):
        nonlocal tight
        while True:
            stream, written = random_stream(rng)
            spare = rng.choice([0, 4, 8, 16, 64, -4, -8, -16, -rng.randrange(1, 200)])
            case = lz_in_place(rng, stream, written, spare)
            if case is not None:
                tight += spare < 0
                return case

    total += run_lz(generate, count=2000)
    rng = random.Random("rom lz in place")
    cases = []
    for name, stream, written in rom_lz_members(60, largest=0x8000):
        for spare in (64, 8, 0, -8, -64):
            case = lz_in_place(rng, stream, written, spare, note=f"{name}, {spare} bytes spare")
            if case is not None:
                cases.append(case)
                tight += spare < 0
    total += run_lz(enumerated(cases), count=len(cases), seed="rom in place", fast=True)
    return f"{total} cases ({tight} with the stream ending before the output)"


if __name__ == "__main__":
    main([
        test_cpu_clear16,
        test_cpu_copy16,
        test_cpu_clear32,
        test_cpu_copy32,
        test_cpu_send32,
        test_cpu_clear_fast,
        test_cpu_copy_fast,
        test_copy_fixed,
        test_geometry_port,
        test_cpu_fill8,
        test_cpu_fill,
        test_cpu_copy8,
        test_cpu_comp8,
        test_cpu_copy,
        test_cpu_move,
        test_zero36b,
        test_swap_word,
        test_uncompress_lz8_random,
        test_uncompress_lz8_encoded,
        test_uncompress_lz8_rom,
        test_uncompress_lz8_in_place,
    ])
