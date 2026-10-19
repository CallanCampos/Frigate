# 0014: The sanitized build: the game's heaps poisoned in place, the DS-isms ignorelisted

- **Status:** accepted
- **Date:** 2026-10-18
- **Plan:** §4.5 (heaps), §6 (T5), §7 phase 5 ("sanitizer-clean replays")
- **Upstream:** `993e6c49b` (as merged into the port's `main` by `1f0c184f6`)
- **Note (2026-10-19):** accepted on its branch, `port-sanitizers`, and kept accepted at the branch's merge into
  `port-integrate` by the port's developer, as every earlier ADR was. The branch's commits numbered it 0013, which the
  Windows track's ADR took; it is 0014 everywhere in the tree.

## Context

T5 runs the replays under AddressSanitizer and UndefinedBehaviorSanitizer. Until now a sanitized build was a build
directory with hand-set `CMAKE_C_FLAGS`, which the header of `port/cmake/os.cmake` gave, and nothing ran it but
sessions by hand. A survey ([STATUS.md](../STATUS.md), 2026-10-18), every route of both versions under Apple clang
21 and Black 2's tests and routes under upstream clang 23, found no ASan error, four UBSan reports, and gaps in what the
sanitizers could see. Two of the gaps decide how the build works:

- **The game's heaps are invisible to ASan.** Every GFL heap and the sound heap are NitroSystem FND heaps inside one
  16 MiB `calloc`'d arena, which ASan sees as one block: an overrun from one of the game's allocations into the next,
  or a use after free, was never reported. PLAN.md §4.5 planned to map the GFL allocator to `malloc` in an ASan build.
- **Three reports are code that is right on every host but undefined by C's rules:** NitroSystem's 2-aligned reads
  of an animation's cell index through a 4-aligned type (R1, R2), and the save data's calls to 56 typed initializers
  through `void (*)(void *)` (R3). With every report fatal, they stop every route.

## Decision

- **One option, `PORT_SANITIZE`**, builds every target but SDL3 with `-fsanitize=address,undefined
  -fno-sanitize-recover=all`, so that a report fails the test that reaches it, and `PORT_SANITIZE_RECOVER` lets a
  survey run go on after each report. The tests carry labels by what they need (`route`, `oracle`), and CI's
  `sanitize` job builds Black 2 this way on macOS and Linux.
- **The game's heaps stay the game's, poisoned in place.** FND's heap files are compiled without ASan, and
  `port/hal/nnsys/fnd_hooks.c`, which already wraps FND's creation, allocation and frees, poisons a new heap's memory,
  unpoisons a block for the bytes the DS gives it when it is allocated, and poisons it when it is freed.
- **A DS-ism that is right on every host is left out by an ignorelist** (`port/cmake/sanitizer_ignorelist.txt`), by
  check and function, each with its reason; a defect is never listed, it is fixed: in Frigate's code directly, in the
  game's code under `PLATFORM_PC` with its row in game-changes.md. A read the DS makes past a heap block whose bytes
  the game then shows is neither: the read stays the DS's, and `fnd_hooks.c`'s `port_heap_ds_read_begin` and
  `port_heap_ds_read_end`, called around it under `PLATFORM_PC`, let that one read through.

## Alternatives

- **The GFL allocator on `malloc`** (PLAN.md §4.5's plan) gives every allocation redzones and a quarantine, but
  changes what the game sees: FND's fit order and alignment, the free sizes the game reads
  (`GFL_HeapGetFreeSizeCore`), the heap a block belongs to, the sound heap's frame states. The run would no longer be
  the plain build's, and the oracle's comparisons and the replays' frame hashes, which the sanitized routes check too,
  would test another program. Poisoning keeps every byte the game sees the plain build's.
- **Fixing the DS-isms in the game's code**: R1 and R2 are two reads in NitroSystem that would each take a
  `PLATFORM_PC` block to say the same load; R3 would take a wrapper for each of 56 initializers in
  `src/system/save_table.c`. Each is a game change with an upstream conflict to keep, for no change in what runs.
- **Recoverable checks with runtime suppressions** (`UBSAN_OPTIONS=suppressions=`): they need the environment set for
  every run and the checks recoverable, so a report would no longer fail a test by itself.

## Consequences

- An access outside one of the game's expanded or frame heap blocks is a report naming the game's code. The first
  survey found three such places this way: two memory bugs of the game (a font cell too short for its copy, a write
  after free in the print stream), fixed under `PLATFORM_PC`, and the copyright screen's read past a file's block,
  whose tiles the title screen's bottom screen shows as it fades in, kept and let through.
- The `OS_Alloc` heaps, unit heaps, VRAM and the DS's windows are still plain memory to ASan, and an overrun within the
  DS's 4-byte rounding of a block is not seen ([sanitizers.md](../sanitizers.md)).
- A new FND function that frees or allocates needs its wrapper to poison or unpoison, as it already needs one to
  release relocated trees.
- An entry of the ignorelist hides its check in the whole function: one is added only for code proven right on every
  host, and removed when the code changes.
