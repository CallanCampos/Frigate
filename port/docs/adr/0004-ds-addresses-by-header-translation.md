# 0004: DS addresses by build-time header translation

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D3, §4.5 (display model)
- **Upstream:** `4aa041311` (merge of PR #348)
- **Note (2026-10-24):** not every access to a geometry command port gets a fresh slot now: one to POLYGON_ATTR,
  TEXIMAGE_PARAM or PLTT_BASE right after one to the same port shares that access's slot, since those commands only
  store their value and the macro's access can't tell SPL's read of POLYGON_ATTR (`lib/spl/src/spl_draw.c`) from a
  write, which sent the engine a POLYGON_ATTR of 0; a send to a shared slot keeps it (`port/hal/hw/hw.c`, STATUS.md's
  entry of 2026-10-24).

## Context

NitroSDK reaches the hardware through macros with literal DS addresses: 134 register macros such as
`#define reg_GX_DISPCNT (*(vu32 *)0x04000000)` and about 40 memory-map constants such as
`#define HW_BG_VRAM 0x06000000`, in eleven headers of `lib/nitro/include` and `lib/twl/include`. The game's C and
the SDK's C use them at thousands of sites, including 79 inline functions used at 1,569 game sites
([sdk-surface.md](../research/sdk-surface.md)). On a host these addresses must lead to host memory.

The DS addresses can't simply be mapped at the same host addresses: a 64-bit macOS process on Apple Silicon reserves
its lowest 4 GiB (`__PAGEZERO`). Editing each macro in the decomp headers would be about 175 changes to upstream's
files, each a conflict risk on every sync. Some registers act when read or written (the geometry command ports, the
divider, VCOUNT), which plain memory can't do.

## Decision

- `port/tools/gen_host_tree.py` writes host variants of every header that names a DS address into
  `build/port/gen/hwinclude/`, at every configure, from upstream's current headers. The host build puts that
  directory first on the include path. Nothing in the repository's headers changes.
- The translation is narrow and checked: a literal in the DS address range (`0x01000000`–`0x0affffff`) is wrapped as
  `PORT_HW_ADDR(literal)` only right after a pointer cast, or as the value of an `HW_*` or `REG_*_ADDR` constant.
  Masks and flags in the same range are untouched. No unwrapped address remains in the variants (checked when the
  variants were introduced; a test should keep checking it).
- `PORT_HW_ADDR()` (`port/include/port_host.h`) maps a DS address to host memory: a 16 MiB window for each region of
  the memory map (`0x01` ITCM to `0x0a`), each a static, page-aligned, zero-filled array of `port/hal/hw/hw.c`. An
  address stays an address constant, so static tables of DS addresses compile and register accesses are a single
  address computation the compiler folds.
- Registers whose accesses act are mapped to accessors by `HW_REGISTER_POLICY` in the generator: each returns the
  memory that one access uses. A write to a geometry command port or `GXFIFO` gets a fresh slot in a log the geometry
  engine reads, so every store is seen in order; a read of the divider or square-root result computes it from the
  operand registers, bit-exact to GBATEK's description (division by zero, `-MAX/-1`, the 32-bit mode's inverted upper
  word); `VCOUNT`, the timers, `GXSTAT` and the matrix readbacks come from the platform layer's state.
- SDK functions that keep DS addresses in other forms (`gx_load3d.c` shifts them right by 12 into a table) are
  replaced by the platform layer (`port/hal/replaced.txt`). The few raw addresses in game or SDK C files are fixed
  where the program reaches them, as game changes.
- Registers that only another SDK function writes, and whose writes must act at once (`VRAMCNT`, only in
  `gx_vramcnt.c` and `mi_wram.c`), are handled by wrapping those functions: the file is compiled with its exported
  functions renamed, and the platform layer's function of the original name calls the renamed one and then applies
  the change.

## Alternatives

- **Edit the decomp headers** (`#ifdef PLATFORM_PC` around each macro, or a translation macro in each definition): about
  175 changes to upstream files and their conflicts, for the same result.
- **Map the DS addresses at the same host addresses:** impossible below 4 GiB on Apple Silicon macOS, and it would put
  the port's correctness on platform-specific address-space tricks.
- **A runtime region table** (`port_hw_region[address >> 24]`): works, but an address is no longer a constant, so
  static tables of DS addresses (`screentex.c`, overlay 310's palette animations) don't compile, and each access loads
  the table.
- **Trap register accesses with page protection and emulate the faulting instruction:** that is an emulator.

## Consequences

- Upstream's header changes reach the host build at the next configure with no merge work.
- A DS address used as an integer is now a 64-bit host address: code that keeps one in a `u32` truncates it. Those
  sites are among the 64-bit fixes the compile probe counts, and are fixed as the program reaches them.
- The geometry engine (phase 2) consumes the command log; until then the status reads idle and the readbacks are
  zero.
- Generated variants are build artefacts, never committed and never edited by hand; a change to the translation is a
  change to the generator.
- Revisit if a header form appears that the translation can't express, or if per-access accessors cost measurable time.
