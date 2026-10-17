# The asm oracle

Differential tests that compare a host C function with the ROM's original machine code. The port replaces the
SDK's assembly routines (`MI_CpuCopy*`, `MI_UncompressLZ8`, `MATH_QSort`, the `MTX_*` asm helpers and the rest of
[PLAN.md §4.5](../../docs/PLAN.md)), and the few SDK functions whose decomp C a host compiles to something else
(`MTX_Concat33` and its kin, [decomp-findings.md](../../docs/decomp-findings.md)), with C in `port/hal`, and each must
behave exactly as the original does on every input the original accepts: the same return value, the same bytes written, the same handling of odd sizes, alignment,
overlap and zero lengths. These tests run both on the same seeded random inputs and fail at the first difference.

## What a passing test shows

For every case it ran, the original (run in an emulator) and the host version:

- returned the same value, as the C type holds it (a returned pointer as the same region and offset; pointers
  outside the case's regions, to the stack or a static table, have no counterpart on the other side and aren't
  compared);
- left the same bytes in every memory region of the case, and in a margin of seeded bytes around each region, so a
  write past either end, or a result that depends on bytes read there, fails;
- made the same calls, with the same arguments and in the same order, to each function pointer it was given.

Each region starts at a multiple of 0x1000 on both sides, so a pointer's offset into its region gives it the same low
address bits for both, and alignment-dependent paths are compared like for like. This is testing, not proof: a test
proves what its generators reach, so they must reach the edges (sizes 0 to a few words and around every unrolled
block, every alignment of every pointer, overlapping and identical pointers) as well as random bulk.

## Unicorn is a test tool

The original code runs in [Unicorn](https://www.unicorn-engine.org), a CPU emulator library, set up as the DS's ARM9
(an ARM946E-S, ARMv5TE). PLAN.md §1 rules out an emulator anywhere in the port; this one is a test oracle only: it
produces the expected results, it is installed in a separate virtual environment, and nothing in the port's build,
program or CI uses it. Its version is pinned in `requirements.txt` (2.1.4). The engine is GPLv2 (the Python package's
metadata says BSD, the binding's licence); it is used unmodified and nothing of it is committed or shipped.

## Setup

1. The ROM build, which extracts the ROM into `extract/<version>` (`python3 configure.py && ninja`, with the ROM in
   `orig/`, as the [README](../../../README.md) says).
2. The host build, configured: `cmake -S port -B build/port -G Ninja`. The tests compile host C with the compiler,
   flags, include paths and defines that `build/port/build.ninja` gives the game, so they follow
   `port/CMakeLists.txt` without a copy.
3. The virtual environment:

   ```sh
   python3 -m venv build/port-venv
   build/port-venv/bin/pip install -r port/tests/asm_oracle/requirements.txt
   ```

## Running

```sh
build/port-venv/bin/python port/tests/asm_oracle/run_all.py               # every test_*.py, Black 2
build/port-venv/bin/python port/tests/asm_oracle/run_all.py --version w2_us -k mi
build/port-venv/bin/python port/tests/asm_oracle/test_selftest.py         # one file
```

`run_all.py` runs each `test_*.py` in its own process (a crash in host C fails that file only) and exits 0 when all
pass, 1 when one fails, and 77 (skipped) when `extract/<version>` is missing, as on CI, or when it runs without
Unicorn, outside the virtual environment. Libraries are built in `build/port-tests/<name>_<version>/`.

A failure prints the case and the first difference, and how to run that case alone:

```
STD_CopyLString: case 3 of seed 0 (rerun it with only=3)
  arguments: (&dst[0], &src[0], 5)
  src (6 bytes, starts at a multiple of 0x1000): a6 6a 8a 43 2c 00
  dst (16 bytes, starts at a multiple of 0x1000): 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  memory: dst[-1] in the pad before it differs (original 0xd3, host 0x53); from dst[-9]:
    original db 1c 9c d3 ac 3e e7 ab d3 a6 6a 8a 43 00 00 00 00 00 00 00 00 00 00 00 00 dc f8 7c f5 d1 31 7c
    host     db 1c 9c d3 ac 3e e7 ab 53 a6 6a 8a 43 00 00 00 00 00 00 00 00 00 00 00 00 dc f8 7c f5 d1 31 7c
```

A case the original itself can't run (an unaligned access, a fault) fails too, with "the original can't run it": the
generator made an input the original doesn't accept.

## Writing a test

A test file is `test_<module>.py` in this directory: test functions that raise on failure, and `main()` from
`compare.py` to run them. Two examples, for host versions in `port/hal/mi/mi_memory.c` and `port/hal/math/math.c`
(their paths are the module writers' choice):

```python
import struct

from build import build_library
from compare import Callback, Case, Ptr, compare, main
from harness import Emulator

emu = None
lib = None


def setup():
    global emu, lib
    if emu is None:
        emu = Emulator()
        lib = build_library("mi_memory", ["port/hal/mi/mi_memory.c"])


def test_cpu_copy8():
    setup()

    def generate(rng):
        size = rng.choice([0, 1, 2, 3, 4, 5, 31, 32, 33, rng.randrange(600)])
        src = rng.randbytes(size + 8)
        overlap = rng.random() < 0.2
        dst_region = "src" if overlap else "dst"
        return Case({"src": src, "dst": rng.randbytes(size + 8)},
                    [Ptr("src", rng.randrange(4)), Ptr(dst_region, rng.randrange(4)), size])

    return f"{compare(emu, lib, 'MI_CpuCopy8', 'void(ptr, ptr, u32)', generate)} cases"


def test_qsort():
    setup()
    lib_qsort = build_library("math", ["port/hal/math/math.c"])

    def by_value(memory, a, b):
        x, y = memory.u32(a), memory.u32(b)
        return (x > y) - (x < y)

    comparator = Callback(by_value, "s32(ptr, ptr)")

    def generate(rng):
        count = rng.randrange(40)
        data = struct.pack(f"<{count}I", *(rng.randrange(8) for _ in range(count)))
        return Case({"array": data}, [Ptr("array"), count, 4, comparator, 0])

    signature = "void(ptr, u32, u32, fn, ptr)"
    return f"{compare(emu, lib_qsort, 'MATH_QSort', signature, generate)} cases"


if __name__ == "__main__":
    main([test_cpu_copy8, test_qsort])
```

`compare()` draws case `i` from `random.Random(f"{seed}:{i}")`, so cases are the same on every run and machine, and
`only=i` reruns one. Pass `count` (default 1000) and a different `seed` for each generator of the same function.

### `compare.py`

- `compare(emu, lib, function, signature, generate, *, count=1000, seed=0, host_function=None, pad=64, only=None)
  -> int`: runs the cases and raises `Mismatch` at the first difference. `function` is the ROM's symbol name, a
  `Symbol` or an address (bit 0 set for Thumb); the host function is `lib`'s of the same name unless `host_function`
  names another (or is a ctypes function). Returns the number of cases.
- `signature`: a string such as `"s32(ptr, ptr, u32)"` or a `Signature`. Types: `void`, `u8`, `s8`, `u16`, `s16`,
  `u32`, `s32`, `u64`, `s64`, `ptr`, `fn`. Integers are truncated and extended as the C type would be, on both sides.
- `Case(regions, args, note="")`: `regions` maps a name to its initial bytes; `args` holds ints, `Ptr(region,
  offset=0)` (the offset may reach into the margin) and `Callback`s.
- `Callback(function, signature="s32(ptr, ptr)")`: a function pointer written once in Python for both sides.
  `function(memory, *args)` returns the result; `memory` reads and writes the calling side's memory
  (`read(address, size)`, `write(address, data)`, `u8` ... `s32(address)`), and pointer arguments are that side's
  addresses. Every call is logged and the two sides' logs must be equal.
- `run_original(emu, function, signature, case)` and `run_host(function, signature, case)` run one side and return an
  `Outcome(ret, memory, calls, error)`, for tests that compare something else themselves.
- `main(tests)`: runs a file's test functions; exits 0, 1, or 77 when the ROM isn't extracted.

### `build.py`

- `build_library(name, sources, *, wrapper=None, version=None, extra_flags=(), base=True) -> HostLibrary`: compiles
  the files (repository-relative paths) and `wrapper`, C source text for whatever a test needs exported (a static
  inline from a header, an adapter), into a shared library and loads it with ctypes. A decomp file gets the flags of
  its own statement in `build/port/build.ninja`; any other file gets the game's flags (`pokebw2_game`'s, with
  `-D PLATFORM_PC` and `-D BLACK2` or `-D WHITE2`), and a file in `port/` or the wrapper also gets `port_hal`'s
  include paths and options after them. `base` adds `port/hal/hw/hw.c`, the host memory behind the DS addresses
  that `port_host.h` declares. A symbol the files reference but neither define nor find in the host's C library
  gets a stub (`HostLibrary.stubbed` lists them): a function that reports its name and aborts, or a zeroed data
  placeholder. Add the files that define what the tested code really calls.
- `HostLibrary.function(name, restype, *argtypes)`: a ctypes function; `.address(name)`; `.cdll`; `.path`.

### `harness.py`

- `Emulator(version=None, *, strict_alignment=True, max_instructions=20_000_000)`: the ARM9 with main, the ITCM,
  the DTCM and the autoloads loaded at their addresses (`version` defaults to `ASM_ORACLE_VERSION`, else `b2_us`).
- `emu.call(function, *args, max_instructions=None) -> CallResult`: calls a name, `Symbol` or address (bit 0 for
  Thumb) with any number of arguments, as MWCC passes them: words in r0 to r3, then on the stack; an `I64(value)`
  takes the next two words, low first, with no even-register alignment (`FX_Mul32x64cFunc` takes its `fx64` in r1
  and r2). LR is a sentinel address, so the call ends when the function returns. `CallResult` has `r0`, `r1`, `s32`,
  `u64` and `s64`. A call checks that r4 to r11 and SP come back intact.
- `emu.alloc(size, *, align=16, data=None, fill=0) -> int` and `emu.reset_scratch()`: scratch memory at 0x08000000.
  `emu.read`, `emu.write`, `emu.read_u8/u16/u32/s32`, `emu.write_u8/u16/u32`.
- `emu.callback(function) -> int`: a function pointer for the ROM's code to call, run in Python:
  `function(emu, r0, r1, r2, r3)` returns an int for r0, an `I64` or a pair for r0 and r1, or None. Inside it,
  `emu.args([4, 4, 8, ...])` decodes the arguments by size, stack ones included, and `emu.stack_arg(i)` reads one.
  Each callback is a `bx lr` in a reserved region with a code hook on it, which runs the Python function before the
  `bx lr` returns to the caller; the address is even, so callers switch to ARM state for it.
- `emu.stub(code, *, thumb=False) -> int`: machine code (ending with its own return) written to a reserved region,
  to call or pass as a function pointer. Unicorn caches translated code, so code must be written through `stub()`,
  which discards the cache for it.
- `emu.replace(name, function)`: calls of a ROM function run `function` instead (as for `callback`), for example to
  make `DC_FlushRange` a no-op.
- `emu.record_writes(begin, end, *, reads=False)`: a context manager that yields the list of `Access(address, size,
  value, pc)` made to `[begin, end)` in the block, for what a function sends to a register or FIFO.
- `emu.load_overlay(id)`: loads an overlay at its address (calling an overlay's function needs it).
- `emu.rom.symbol(name, module=None) -> Symbol` (`address`, `entry`, `thumb`, `kind`, `size`, `module`).
- Errors (all `EmulatorError`): `Fault` (unmapped access, undefined instruction, an SVC, which is a BIOS call, a
  breakpoint), `UnalignedAccess`, `InstructionLimit`, each naming the function and offset where it happened.

## What the emulator doesn't model

- **Unaligned accesses.** The ARM946E-S rotates an unaligned word load and ignores the low address bits of word
  stores and LDM/STM; Unicorn performs them as a later ARM would. So the harness stops with `UnalignedAccess` rather
  than give a wrong answer (`strict_alignment=False` turns that off). An input that makes the original do this is
  one it doesn't accept, or one whose ARMv5 result a test must check by other means.
- **Hardware.** The I/O registers, palettes, VRAM and OAM are plain memory, the VRAM views don't alias each other, and
  the divider, square root, DMA, timers, cache operations (CP15) and the BIOS (an SVC stops the call) don't exist.
  Compare register traffic with `record_writes`, and replace what the tested function calls with `replace`.
- **The boot.** `.data` holds the ROM's initial values and `.bss` is zero; no static initializer or SDK init ran, and
  memory persists from one call to the next.

## The self-test

`test_selftest.py` proves the harness on functions the decomp already has as matching C (`fx.c`, `fx_vec.c`,
`fx_mtx33.c`, `fx_mtx43.c`, `math_crc.c`, `std_string.c`): the ROM's code in the emulator and that C compiled for
the host agree on 1000 seeded random inputs for each of `VEC_CrossProduct`, `VEC_DotProduct`, `VEC_Fx16Add`,
`FX_Mul32x64cFunc`, `MTX_ScaleApply43`, `MATHi_CRC32InitTableRev`, `MATH_CalcCRC32` and `STD_CopyLString` (twice),
a Callback round trip and a 64-bit result. Between them they cover ARM and Thumb, results and output buffers,
alignment, overlapping and empty buffers, a 64-bit and a stack argument, and ROM functions calling other ROM
functions. It also checks that four deliberately wrong host functions are caught, that `MATH_QSort`'s original sorts
through a Python callback and an ARM comparator alike, and that faults, BIOS calls, unaligned accesses and runaway
loops are reported.
