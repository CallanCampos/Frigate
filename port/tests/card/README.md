# CARD physical-foundation tests

| Module | Actual code exercised |
| --- | --- |
| [test_card_bus.c](test_card_bus.c) | Pure caller-owned wire controller, immutable manufactured backend, high native pointer and hand-derived deadlines. |
| [test_card_hardware.c](test_card_hardware.c) | Closed physical events/mainRAM channel linked to real SDK OS objects and port_os_run, not a scheduler substitute. |

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

There are no CPU-reader tests in this candidate: production card_rom.c is unchanged and no wire timing is activated
for gameplay. Actual CARD page/cache/ID/IRQ lifecycle and polling are later work. Unit success establishes this
bounded native contract, not retail hardware accuracy, trainer-AI timing or whole-game coverage.
