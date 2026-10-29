# CARD physical-foundation tests

| Module | Actual code exercised |
| --- | --- |
| [test_card_bus.c](test_card_bus.c) | Pure caller-owned wire controller, immutable manufactured backend, high native pointer and hand-derived deadlines. |
| [test_backup_service.c](test_backup_service.c) | Genuine CARD common/task/stream/request, PXI and OS against synthetic flash; only ROM initialization is a typed test seam. |
| [test_card_hardware.c](test_card_hardware.c) | Closed physical events/mainRAM channel and shared CPU latch ownership linked to real SDK OS objects and port_os_run, not a scheduler substitute. |

`card_bus` covers C5/8, L1/L2, every hardware payload size, delayed/early consumption,512-byte gaps, ID timing policy,
completion latch/ack, cancel/reset, invalid requests, overflow and backend failure. Data is synthetic, never a ROM.

`card_hardware_*` covers pacing/visibility/bounds; absolute versus own-work timing; I/IME/IE masking; timer IF during
service; old IRQ Spin physical progress while generic host work defers; Halt; cancellation/reset and no stale writes;
generic ownership conflicts and inactive immediate DMA behavior. Exact equal-cycle case: start 4, READY 64 and timer/
host-event64; timer IF is observed 64 but handler/host work starts 73 only after commit. Exact ABA case: arm/demand,
reset, arm/demand-new, commit-old must fail without touching the new range; commit-new remains valid. Monotonic
epochs do not reset to zero or wrap on exhaustion.

Independent arithmetic: C5/L1=3/L2=2 gives first READY 85/COMMIT 94; 1024 bytes end 7499, including 256 nine-cycle units
and one extra 512-byte boundary gap. A 512-byte request ends 3777; own 4000 cycles ends 5152 after 1152 stalled cycles,
while absolute 4000 remains 4000. Source/compile/binary hashes and plain/fatal sanitizer evidence are retained locally.

The six `card_hardware_cpu_*` scenarios cover held/early reads, all 128 words of a page, next-READY pacing, final-consume
IF before a separate destination store, masked IRQ delivery, bidirectional CPU/DMA exclusion, stale command tokens,
cancel/reset ABA, zero-data completion, READ/ID backend failure and recovery, and an actual IRQ handler's timed ID read
without nested IRQ delivery. Ordinary CPU consumption incurs no DMA stall or invented CPU-work charge.

These exercise the controller boundary; production card_rom.c is unchanged and no wire timing is activated for
gameplay. Actual SDK page/cache/task integration and polling/store/copy costs are later work. Unit success establishes
this bounded native contract, not retail hardware accuracy, trainer-AI timing or whole-game coverage.

## Backup service tests

`card_backup_service_stream` splits 768 bytes across the real 256-byte page buffer, writes/verifies each page and
checks three exact 837,824-cycle waits, while a real sound alarm and independent host timed callback interleave.
`sync` covers the synchronous SDK task path, including zero-length requests with no invented wait. `cancel` uses
actual `CARD_CancelBackupAsync` from another thread and checks exactly-once completion/BUSY+REQ release, first-page
retention and untouched later pages. `stop`, `bus_reset` and `path_reset` check that bytes precede the transition
and expect no task resumption/callback after whole-run termination; a new OS/PXI/service boot produces no stale
reply; it does not reinitialize CARD task state or promise old-task cleanup. Path switching still frees the old
image and dirty state, so that case does not claim unflushed old-path persistence. `invalid` rejects inaccessible
memory, unsupported raw request IDs 31/32 without unsafe capability shifts, and an invalid chip size shift;
`io_error` makes atomic file replacement fail after the image has loaded, and checks the real SDK still completes
while retaining bytes. Disk persistence failure is reported/retried, not invented as cartridge failure.

The test target filters exactly `card_rom.c.o`/`.obj` from native HAL objects and links that actual source
separately with only `CARDi_InitRom` renamed. The fixture provides that typed initialization entry to boot backup
without a ROM; all other SDK/HAL owners remain real. Normal/fatal sanitizer builds use the same `port_decomp_env`.
The older `tests/rom/test_backup.c` byte/file fixture uses writePage=0; its explicit-clock `deferred_reply` case
covers early and exact deadlines, stale queued callbacks after reset/new owner, exactly-once reply and UINT64_MAX
sentinel. It does not substitute for actual scheduler coverage. See
[ADR0029](../../docs/adr/0029-backup-write-service-wait.md).

The actual-game `game_wrapper` and `game_error` scenarios call `writeFlashWaitSuccess` from
`src/gfl/backup_card.c` through genuine CARD tasks. They prove three page waits, sound/independent timed work,
high-buffer readback, error callback and released locks/flags. The host-only wait ignores its return: the original
finisher remains the result/error/unlock authority. No replacement CARD API or clock increment is used.
