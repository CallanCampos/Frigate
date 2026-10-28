<!-- Native representation of NitroSDK's caller-owned FAT/FNT cache, used during boot and DWC file access. -->
# 0026: Keep archive offsets and resolve cached tables through the native pointer

- **Status:** accepted
- **Date:** 2026-10-28
- **Plan:** §4.5 (platform layer), §4.8 (64-bit boundaries), §6 (verification)

## Context

NitroSDK loads FAT/FNT bytes into caller-owned memory, then stores their addresses in 32-bit table positions.
Those fields cannot carry a native pointer. Frigate previously reserved the same allocation but left the cache
unloaded, reading metadata from storage. Returned data agree, but the request stream differs: a controlled Hugh
trace found 58 extra global FNT reads and two FAT reads during the first AI damage calculation.

`FSArchiveTables.load_mem` already retains a full native pointer. The directory/table positions can remain their
original archive offsets. The real SDK unload path already restores those offsets and returns the caller's pointer.

## Decision

The host loader copies both tables through the SDK's file commands into the caller's allocation, aligned as the
original loader specifies. It retains `load_mem` as the native pointer and keeps FAT/FNT positions as offsets.
A small typed HAL resolver maps complete table spans to that allocation. Only `FSi_ReadTable`'s cached branch calls
it under `PLATFORM_PC`; the original DS copy remains under `#else` and preprocesses identically in both versions.

Keep the SDK's query/reload/unload contracts, including a null query preserving a loaded cache and an insufficient
non-null reload unloading it first. Payload files still use the storage callback. Invalid or ambiguous nonempty
ranges fail; valid zero-length table spans succeed without copying.

## Alternatives

- Widen all SDK directory/table positions: a much larger ABI/layout change where positions also represent ROM
  offsets and are retained in several structures.
- Maintain another cache registry: duplicates ownership already represented by `load_mem`.
- Retain uncached metadata: preserves returned bytes but introduces extra card requests, obscuring timing work.

## Consequences and validation

One SDK source seam is recorded in [game-changes.md](../game-changes.md). The real archive/file/default command
regression tests native pointers, storage traffic, lifecycle and bounds under ordinary builds and ASan/UBSan.
Both original ROM hashes and both-version MWCC preprocessing must remain unchanged. Game replays validate the native integration separately. A controlled Black 2 run preserves all 8,500 frame
records and 34,000 state snapshots; its 18 ordered CARD source/length requests now match the bounded DS trace
exactly. This request equality does not establish equal transfer timing, DMA/cache behavior or AI scheduling.

This decision adds no CARD latency or CPU work model. Timing observations use the pinned melonDS 1.1 model,
not retail-card hardware. Its observed script loads exceed the AI deadline, but their
ideal card-wire time alone is below it; SDK/FS/copy/IRQ work also matters. Correct cache traffic is a prerequisite
to that investigation, not a claim that trainer-AI parity is solved.
