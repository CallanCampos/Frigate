# Game changes

Every change the port makes outside `port/`, other than its own new files under `.github/` (`README.md` and
`workflows/port-*.yml`). Game code stays the decomp's ([PLAN.md §4.1](PLAN.md#41-repository),
[§4.8](PLAN.md#48-64-bit-and-dual-target-rules)), so this list should stay short, and every row is something an
upstream merge can conflict with. A change outside `port/` without a row here is a mistake.

## The three allowed kinds

1. **Neutral:** a portability fix that compiles to the same bytes under MWCC, such as `uptr` for a value that carries
   an address, or an SDK constant in place of a raw VRAM address. It stays in this repository: the port is a solo
   project and proposes nothing upstream ([ADR 0002](adr/0002-solo-project-no-upstream-posts.md)).
2. **Hook:** a hook macro (`PORT_EVENT`, `PORT_SHOULD`) that compiles to nothing, or to the bare expression, in the
   ROM build.
3. **Platform:** an `#ifdef PLATFORM_PC` block, used rarely and commented in the code. `include/platform.h` is the only
   place that decides between MWCC and a host. Memory-safety fixes for documented `// BUG:`s that 64-bit layouts make
   dangerous are this kind.

A change that fits none of these doesn't go in.

## Proving a change is MWCC-neutral

Every kind needs the same proof, since the code around a hook or a platform block, and its `#else`, still compiles
under MWCC.

- In a **complete** file, `python3 configure.py && ninja` with both SHA1s matching is the proof.
- In an **incomplete** file (its `delinks.txt` entry isn't marked `complete`) the ROM still links the original object,
  so the ROM build proves nothing about its C. Run
  `.venv/bin/python tools/decomp/compiler_probe.py FILE --compilers 1.1p1 --mismatches` for both versions (add
  `--version w2_us`) before and after, and show that no function's result changed.
- A **header** change (`include/platform.h` in a common header, a retyped API, a hook macro) changes every file that
  includes it: ninja proves the complete ones, and the probe runs on every incomplete file that includes the header,
  directly or through another header, in both versions. After a build, `ninja -t deps` lists each object's headers;
  when in doubt, probe every incomplete file.
- A **hook** must also expand to nothing or to its bare argument when `PLATFORM_PC` is undefined, and a **platform**
  block must leave the code MWCC compiles as it was; `PLATFORM_PC` is defined only by the host build.
- A **guard-only** change, one that only adds `#if`/`#ifdef PLATFORM_PC` branches and leaves what MWCC compiles token
  for token as it was, is proven for every file at once, complete or incomplete, by
  `.venv/bin/python port/tools/check_mwcc_neutral.py`: MWCC's preprocessed text of every C file, in both versions, is
  identical to the last merged upstream commit's, so the compiler gets the same input. It replaces the per-file probes
  for such changes; ninja with both SHA1s still runs.

## Columns

| Column | What to write |
| --- | --- |
| File | Path from the repository root; one row per file, or per header and its users when one macro change covers many |
| Kind | Neutral, Hook or Platform |
| Guard or macro | The macro, typedef or `#ifdef` that carries the change |
| Why | What breaks on a host without it, in one line |
| MWCC-neutral proof | For every kind: "ninja, both SHA1s" for complete files; the probe result for incomplete files, every including one for a header |
| Commit | The port commit that made the change, short SHA. A row is committed with its change, so it can't name that commit: write `PORT_SHA`, and the next commit to this file replaces it with the SHA from `git log -1 --format=%h -- port/docs/game-changes.md`, run before committing |

When a sync conflicts with a row's change, resolve it by taking upstream's version and reapplying the change, at the
code's new place if upstream moved it, then update the row's commit. When upstream makes the same change on its own,
git usually merges it without a conflict, so nothing flags it and no tool drops the row. So after each of its syncs, a
session deletes every row whose files all match upstream again (`git diff upstream/main -- FILE` prints nothing), in
its next commit; on a conflict, [SYNC.md](SYNC.md#resolving-conflicts), step 3, drops it. The git history of this
file keeps what the port needed.

## Changes

| File | Kind | Guard or macro | Why | MWCC-neutral proof | Commit |
| --- | --- | --- | --- | --- | --- |
| `include/gfl/overlay.h` | Platform | `#ifdef PLATFORM_PC` around `OVERLAY_ID` | `OVERLAY_ID(n)` is the address of a linker symbol; the host links every overlay into one program, so the ID is `n` | `check_mwcc_neutral.py`, every file, both versions; ninja, both SHA1s | `577554b0b` |
| `lib/nitro/include/nitro/math.h` | Platform | `#if defined(PLATFORM_PC)` before `#elif defined(__thumb)` | `MATH_CountLeadingZeros` is an MWCC `asm` block; the host counts with `__builtin_clz` | as above | `577554b0b` |
| `lib/nitro/src/crypto/rc4.c`, `fx/fx_mtx22.c`, `fx/fx_mtx33.c`, `fx/fx_mtx43.c`, `fx/fx_mtx44.c`, `gx/g3x.c`, `math/math_pop.c` | Platform | `#ifndef PLATFORM_PC` around each MWCC `asm` function (18 in all) | MWCC `asm` functions don't compile on a host; `port/hal` provides host versions | as above | `577554b0b` |
| `include/types.h` | Platform | `PORT_DS_ALIGN_64` on the `u64`, `s64` and `f64` typedefs, empty unless `PLATFORM_PC` | On a host 64-bit values align to 8, so structs holding them grow (a bag sort entry from 12 to 16 bytes, changing `MATH_QSort`'s pivot and the order of equal keys; save blocks); the DS aligns them to 4 | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `d63819fc5` |
| `include/types.h` (again) | Neutral | `uptr`, `sptr` and `ulong32` macros: `u32`, `s32` and `unsigned long` for MWCC, pointer-wide and 32-bit on hosts | A value carrying an address needs pointer width on a host; `unsigned long` is 64-bit on macOS and Linux | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `e49c96554` |
| `lib/nnsys/src/fnd/heapcommoni.h` | Neutral | `AddrOf` returns `uptr` | NNS FND's heap arithmetic cut host addresses to 32 bits: the first GFL heap faulted | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `e49c96554` |
| `lib/nitro/include/nitro/os.h` | Neutral | `HW_DTCM` is `(uptr)SDK_AUTOLOAD_DTCM_START` | The interrupt check flags at the end of DTCM were reached through a cut address (`OS_SetIrqCheckFlag`, the first VBlank) | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `e49c96554` |
| `lib/nitro/include/nitro/gx.h`, `lib/nitro/src/gx/gx.c`, `lib/nitro/src/gx/g2.c`, `src/ov257/ctvt_talk.c`, `src/system/net_err.c` | Neutral | The register-address parameters of `G2x_SetBGyAffine_`, `GXx_Set/GetMasterBrightness_`, `G2x_SetBlend*_`, `G2x_ChangeBlendBrightness_` and the `(u32)&reg_...` casts at their callers are `uptr` | A register's address is a host address (ADR 0004); as a `u32` it was cut and the first brightness write in the boot screens faulted | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `e49c96554` |
| `lib/nnsys/src/snd/snd_heap.c`, `src/gfl/net.c`, `src/ov011/dwc_rapcommon.c` | Neutral | Alignment arithmetic on pointers through `uptr` | Rounding an address up through `u32` cut it | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `e49c96554` |
| `lib/nitro/include/nitro/rtc.h`, `include/save/misc_savedata.h` | Neutral | `RTCDate`'s fields and `metTrainerCount` are `ulong32` | With a 64-bit `unsigned long` the adventure and misc save blocks grew (0x5c to 0x78, 0xfc to 0x100), the save outgrew its 0x26000 buffer and heap 7 was overwritten | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `e49c96554` |
| `src/gfl/heapsys.c` | Platform | `PORT_HEAP_SIZE()` (the bare size for MWCC) at the root heaps' creation, and an `#ifdef PLATFORM_PC` resize of each child heap | Host structs are larger, so the DS's heap sizes run out (heap 8 at `net_err.c:190`); `port/hal/gfl/heap_policy.c` sizes them | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | `e49c96554` |
| `lib/nnsys/src/snd/snd_internal.h`, `lib/nnsys/src/snd/snd_arc.c` | Platform | `#ifdef PLATFORM_PC`: `NNSSndArcFileInfo.mem` is a `u32` handle, converted with `port_snd_handle` and `port_snd_pointer` (declared in `port/include/port_host.h`) in `NNS_SndArcInitOnMemory`, `NNS_SndArcGetFileAddress` and `NNS_SndArcSetFileAddress`; a `_Static_assert` on the entry's 16 bytes | The sound archive's file table is read from the file as it is; an 8-byte pointer moved every entry after the first, so each file's offset and size were misread | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | PORT_SHA |
| `lib/nitro/include/nitro/snd.h` | Platform | `#ifdef PLATFORM_PC`: `SNDWaveArc.topLink` and `SNDWaveArcLink`'s `waveArc` and `next` are `u32` handles (`port/hal/snd7/snd_bank_host.c` keeps them); `_Static_assert`s on the heads' DS sizes | Wave archives and banks are files the ARM7 reads too; 8-byte links moved `waveCount`, `offsetTable` and `instCount`, so NitroSystem's loader and `pm_voice.c` read the wrong words. No decomp file reads the links themselves (only `snd_bank.c`, which the host replaces) | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | PORT_SHA |
| `lib/nnsys/include/nnsys/snd.h`, `lib/nnsys/src/snd/snd_heap.c`, `snd_arc.c`, `snd_arc_loader.c`, `snd_arc_stream.c`, `snd_player.c` | Neutral | The sound heap's `data1` and `data2` are `uptr` (`NNSSndHeapDisposeCallback`, `NNS_SndHeapAlloc`, `SndHeapBlock`, `LoadFile`, every dispose callback), and the archive, stream player and wave archive passed as `data1` are cast with `(uptr)` | The dispose callbacks get their archive, player or wave archive back from `data1`; cut to 32 bits, it pointed nowhere once a heap level was freed | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | PORT_SHA |
| `lib/nnsys/src/snd/snd_heap.c` (again) | Platform | `#ifdef PLATFORM_PC`: `BLOCK_HEAD_SIZE` is `sizeof(SndHeapBlock)` rounded up to `HEAP_ALIGN`, with a `_Static_assert`; 32 for MWCC | A host's block head is 48 bytes, so with the DS's 32 the data handed out overlapped the head's callback and data | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | PORT_SHA |
| `lib/nnsys/src/snd/snd_stream.c` | Neutral | `AddU32ToPtr` adds through `uptr` | Every stream's block pointers (the game's stream player's too) were cut to 32 bits | `check_mwcc_neutral.py` (token streams), every file, both versions; ninja, both SHA1s | PORT_SHA |
