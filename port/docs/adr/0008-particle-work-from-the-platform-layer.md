# 0008: Particle work from the platform layer

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D5, §4.5, §4.8 (rule 7)
- **Upstream:** `e484d5011` (merge of PR #353)

## Context

GFL's particle systems (`src/gfl/particle.c`) run NitroSystem's SPL. A system's SPL manager allocates its manager, 24
emitters, 200 particles and the resource file's resource, behavior and texture tables from one buffer, with a bump
allocator that never frees (`Particle_Alloc`). The caller allocates that buffer from its own game heap and passes it to
`func_0204f980`, sized for the DS's structs. 13 files do so (the intro, battles, trades, the field and others), each
with its own constant.

Every one of those structs holds pointers, so on a 64-bit host each is larger: an emitter takes 208 bytes against 156,
a particle 80 against 68, the manager 136, a resource 64, a behavior 16 and a texture 24. The intro's 0x4800 bytes hold
the DS's emitters and particles; on the host those alone take 20,992 bytes. New Game's intro overran the buffer into the
next heap block, the particle camera's pointer came out NULL, and freeing it crashed in `GFL_HeapFree(NULL)`
([STATUS.md](../STATUS.md), 2026-10-17, New Game's intro). PLAN.md §4.8 rule 7 says fixed-size buffers sized for 4-byte
pointers are sized from the real structs; the question is where.

## Decision

- **`func_0204f980` takes the system's work from the platform layer on a host**, in one `#ifdef PLATFORM_PC` block
  (a Platform row in [game-changes.md](../game-changes.md)): `port_particle_work_size` gives the size and
  `port_particle_work` the memory (`port/hal/gfl/particle_work.c`, declared in `port/include/port_host.h`).
  `func_0204fa84`, the one path that frees a system, gives it back with `port_particle_work_free`. The 13 callers don't
  change.
- **The size is twice the caller's, a proven bound and not a measurement.** A struct's host size is at most twice its
  DS size: each field at most doubles (a pointer from 4 bytes to 8), and so does each offset, since no alignment more
  than doubles (`s64`, `u64` and `f64` keep the DS's 4 through `PORT_DS_ALIGN_64`). Every struct SPL allocates holds a
  pointer, so its host size is a multiple of 8, and the allocator's rounding to 4 adds nothing to it; on the DS that
  rounding can only add. So each allocation takes at most twice what it takes on the DS, where they all fit the
  caller's size. A `_Static_assert` in `particle_work.c` checks the multiple of a pointer for all six structs.
- **The work comes from the host's heap (`calloc`), not a game heap.** The caller still allocates its DS-sized buffer
  from its game heap, as on the DS, so the game's heaps hold what they held there, and the heap policy's measured sizes
  ([ADR 0005](0005-platform-boundary-as-built.md)) stay valid. `workStart` stays the caller's buffer, since
  `func_020500c8` hands it back for the caller to free (`btlv_effvm.c` does).
- **The work is kept by system ID**, 0 to 15, which no two live systems share. Creating a system whose ID's work is
  still held stops the program with a message: it would be a system created twice without being freed.

## Alternatives

- **Resize the buffer at the 13 call sites:** 13 game changes, each needing its own proof and ledger row, each a merge
  conflict in waiting, and each size a guess unless it repeats the proof above; a 14th caller that upstream adds would
  overflow silently.
- **A size-only hook like `PORT_HEAP_SIZE`:** the size is the caller's to give, and the buffer is allocated before
  `func_0204f980` sees it, so a hook there can't enlarge it; a hook at the callers is the alternative above.
- **The work from the caller's game heap, inside `func_0204f980`:** each system would take three times the DS's work
  from a heap that the policy sized by measuring the game's own allocations, so the intro's and the battles' heaps
  could run out where the DS's don't. The game heap gains nothing here, since nothing else reads the work.
- **A measured factor:** a measurement covers only the effect files that ran; a battle effect with more resources
  could need more. The bound covers every file.

## Consequences

- On a host each system holds its DS-sized buffer, unused but for `workStart`, plus twice that from the host's heap,
  outside the game heaps' accounting: the callers give 0x4800 to 0x5000 bytes, so at most 40 KiB more a system.
- ASan sees the work as an allocation of its own, so an overrun of it is reported rather than landing in the next heap
  block.
- Revisit if SPL gains a struct whose size isn't a multiple of a pointer's (the `_Static_assert` fails to compile), if
  a caller reads SPL's data through its own buffer (none does: `workStart` is only handed back to be freed), or if a
  path frees a system without `func_0204fa84` (then the work would leak and the next system with that ID would stop the
  program).
