# 0021: Root heaps in the game's own memory get host memory

- **Status:** accepted
- **Date:** 2026-10-23
- **Plan:** D5, §4.5 (heaps), §4.8 (rule 7: buffers sized for 4-byte pointers are sized from the real structs)
- **Upstream:** `0a12a2bd3e` (merge of PR #480)
- **Note (2026-10-23):** accepted by the session that made the change, the field-entry track of the plan of
  2026-10-23, not by the developer in person, on the developer's standing instruction to carry the plan forward, as
  ADRs 0013 to 0016 were; the developer is told in the pull request that brings it. Numbers 0019 and 0020 are held for
  that plan's pointer-cast guard and one program for both versions, which were planned with this one.

## Context

GFL makes a root heap in one of two ways (`src/gfl/heapsys.c`). `GFL_MemInit`'s table takes its roots from the OS main
arena, and on a host `PORT_HEAP_SIZE` already sizes them at four times the DS's size, and every child heap at twice
([ADR 0005](0005-platform-boundary-as-built.md), `port/hal/gfl/heap_policy.c`). `GFL_HeapCreateRoot`, through
`GFL_HeapAddRoot`, makes a root heap in memory that the caller gives it, at the caller's size. That memory is sized
for the DS's structs, and the host build left it as it was. Three callers do this:

- **The field** (`src/ov036/fieldmap_proc.c`, `FieldmapProc_Init`): `HEAPID_FIELD_CLACT` in `g_FieldHeapMemory`, 64
  KiB of overlay 36's `.bss`, with three children: weather `0x6400`, place names `0x7400` and the scene area `0x500`.
  On the host the children double, so weather and place names alone take `0x1b000`, more than the root holds.
  `GFL_HeapCreateChild` fails, and `GFL_HeapHandleAllocResult` calls `OS_Terminate`. The game stopped itself (status
  71) at the field's first load: at frame 3,436 after CONTINUE and at frame 10,759 in New Game
  ([STATUS.md](../STATUS.md), 2026-10-22).
- **The trade screen** (`src/ov194/pokemontrade_proc.c`): `HEAPID_POKEMON_TRADE` in its own `0xc000`-byte static.
- **GFL's extra heap at boot** (`src/system/gfl_use.c`, `GFLAppInit`): heap 9, `0x3f00` bytes that the game takes from
  the OS heap on a DS (a DSi makes it a child heap instead).

A scratch build with the field's array doubled under `PLATFORM_PC` opened the field from both saves in both versions,
and ran the 28,600 frames of the Route 19 route from the starter save, through a trainer battle, with room to spare.

## Decision

- **`GFL_HeapAddRoot` gives the heap host memory in place of the caller's.** It passes
  `PORT_HEAP_ROOT_MEMORY(heapId, memory, size)` and `PORT_HEAP_SIZE(heapId, size, TRUE)` to `GFL_HeapAdd`. In the ROM
  build they are the bare `memory` and `size`, so MWCC compiles the same tokens: a Hook in
  [game-changes.md](../game-changes.md). One hook covers every root made in the game's own memory, the three above
  and any that upstream's C adds later.
- **The size is `port_heap_size`'s for a root**, four times the DS's (`--heap-scale-root` overrides it), as for the
  arena's roots. The field's root gets 256 KiB, the trade screen's 192 KiB, the extra heap `0xfc00` bytes.
- **The memory is kept for the caller's memory.** `port_heap_root_memory` (`port/hal/gfl/heap_policy.c`, declared in
  `port/include/port_host.h`) makes it with `calloc` the first time a root heap is made in a given place, zeroed as
  `.bss` is, and returns the same memory every later time a root heap is made there. Each caller's memory is a static,
  or a block that is never freed. The game makes its heap there again at every use: the field's at every map load. So
  this keeps the DS's model, one block of memory reused, with what the last heap left in it. Up to eight places are
  kept; a ninth, or a larger size than the kept memory's for the same place, stops the program with a message.
- **The caller's own memory stays as it is, unused on the host:** the static array, or the OS heap's block.

## Alternatives

- **Resize `g_FieldHeapMemory` under `PLATFORM_PC`** (the scratch build's way, with a size macro in
  `include/field/field_internal.h`): that is a header change, so every incomplete file that includes the header needs
  its own proof. It fixes one caller only, leaving the trade screen's heap at its DS size, and each caller that
  upstream adds would need another change.
- **Scale the size alone in `GFL_HeapAddRoot`:** the heap would run past the end of the caller's memory.
- **Allocate the memory at each creation and free it when the heap is deleted:** that needs a second hook, in
  `GFL_HeapDeleteCore`'s branch for external roots. It also hands each creation fresh memory where the DS reuses the
  same bytes, so it is a looser model of the DS than the kept block.
- **Take the memory from the OS main arena:** arena memory is never freed, so the block would have to be kept anyway.
  It would then come out of the 16 MiB that the arena's own roots are sized against, for no gain.

## Consequences

- The field opens at CONTINUE and in New Game. The field also needs `Field.colorPostFX` typed for the host's 16-byte
  struct. Upstream's `b62c655bfa` does that, and it reaches Frigate with the sync after PR 13. Until then the field
  crashes in `FieldSkillMapEff_Draw` a few frames after it opens, where it used to stop at the heap.
- GFL's extra heap moves from the OS heap's block into host memory at boot, on every run, and is four times as large.
  The boot, the opening, the title and the start menu are unchanged: their replays and oracle sequences pass as they
  did.
- About 500 KiB of host memory stays allocated for the program's life, once each heap has been made.
- AddressSanitizer sees each block as an allocation of its own. The FND hooks poison its free memory while its heap is
  alive and unpoison it when the heap is destroyed, as for any heap.
- The DS clears `g_FieldHeapMemory` when overlay 36 loads again (the host's overlay reset does the same to the array).
  The kept block is never cleared. A read of heap memory that the game never wrote could therefore see other bytes on
  the host than on the DS; that would be a game bug, and the sanitized build would not see it.
- Revisit if a caller's memory turns out to be shorter-lived than a static, if a caller reads its heap's memory
  through its own pointer (none does today), or if a heap here needs more than the root factor gives it.
