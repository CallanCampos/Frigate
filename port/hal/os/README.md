# Native OS boundary

The [parent map](../README.md) describes these modules. [core.c](core.c)/[port_os.h](port_os.h) run the launcher,
SDK threads, IRQs and virtual clock; the scheduler is genuine NitroSDK os_thread.c through os_thread_host.c.
[fiber.c](fiber.c) owns native contexts/sanitizer switching. Game scheduling decisions remain in original C.

## Absolute device deadline

port_os_wait_until is additive: IRQ and preempting-thread time spends its absolute target rather than extending it.
Past/exact targets never rewind. Boot/thread fibers and IRQ handlers may call it; IRQ handlers actually run on the
dispatcher host stack. A guarded path publishes host work there without nested ARM9 IRQ delivery. Ordinary thread
waits yield to the dispatcher. I/IME/IE mask delivery, not physical/timed progress. Host callbacks cannot invoke SDK
blocking operations or recursively wait. Outside-run/callback calls return 0 without advancing. Success returns 1;
stop disposes a suspended thread, while an IRQ wait returns 0 and its handler must unwind. The guard and pending
work reset each run. Existing GetTick step is unchanged; no production reader calls this new API in this candidate.

## Closed physical CARD events

A tagged READY/COMMIT event, not a generic callback, progresses during Spin/IRQ/masked/Halt paths. MainRAM DMA
service drains intervening timer/line/device events and latches IF while ARM9 is stalled. Generic host work and
IRQ delivery wait for unit release. PortOsStats.deviceStallCycles counts each interval once: own-work Spin extends
only for actual stalls; an absolute deadline never moves. Inactive old behavior and pinned determinism are tested.

Before returning to ARM9/IRQ/host work, the event loop closes all physical transitions already due. Equal-cycle
existing events retain their relative order and latch/queue normally; READY then prevents CPU execution until its
commit. OS run entry/exit cancel hardware/channel requests before output buffers/fibers can vanish. Activation is
test-only; production CARD reader and DMA selection remain unchanged. No global timing knob or hardware-parity claim.
