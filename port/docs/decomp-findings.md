# Decomp findings

Things the port found in the decomp's C that differ from the ROM, are unresolved, or behave in a way the decomp's
documentation doesn't say. The port is a solo project and posts nothing upstream ([ADR 0002](adr/0002-solo-project-no-upstream-posts.md));
the user decides whether a decomp session fixes a finding, as ordinary decomp work. The "equivalent" notes of
[PLAN.md §6](PLAN.md#6-verification), one per nonmatching row the port checks, go here too.

| Where | What | How the port handles it | Found |
| --- | --- | --- | --- |
| `src/system/resh_zek_savedata.c:16`, `include/save/resh_zek.h:27` | `ReshZekSave_UnresolvedInitTarget` is a relocation to `0x0215cd8c`, which while overlay 11 is loaded is `WCM_RssiToQuality`: it writes nothing and its result is unused, so the call has no effect on the DS | `port/hal/stubs/unresolved.c` does nothing | 2026-10-17, phase 1 integration |
| `src/gfl/heap.c:292` | `GFL_HeapDumpOnFailure` ends the program with `exit(1)` and no message when a heap runs out | the host sizes heaps larger (`port/hal/gfl/heap_policy.c`); a message on that path would help debugging | 2026-10-17, phase 1 integration |
