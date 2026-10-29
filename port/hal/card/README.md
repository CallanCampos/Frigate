# Cartridge boundaries

These modules serve NitroSDK CARD (`lib/nitro/src/card`): ROM/filesystem reads, saves and the genuine-card check.
The player supplies the ROM/save; no cartridge data is stored here.

| Module | Responsibility |
| --- | --- |
| [card_rom.c](card_rom.c), [port_card.h](port_card.h), [card_internal.h](card_internal.h) | Existing production reader/lifecycle, unchanged by this foundation. Requested DMA still uses the existing CARD-task fallback. |
| [backup.c](backup.c), [card_backup_host.c](card_backup_host.c), [card_backup_calls.h](card_backup_calls.h) | Backup-image service, successful WRITE initial SDK wait, and native pointer transport ([ADR0029](../../docs/adr/0029-backup-write-service-wait.md)). |
| [card_spi.c](card_spi.c), [card_spi.h](card_spi.h) | Cartridge infrared/SPI model. |
| [port_card_bus.c](port_card_bus.c), [port_card_bus.h](port_card_bus.h) | Independent caller-owned NTR read/ID wire engine; `card_bus`. |
| [port_card_hardware.c](port_card_hardware.c), [port_card_hardware.h](port_card_hardware.h) | One test-activated CPU/DMA owner, CPU latch-consume API, physical READY/COMMIT events and bounded native-mainRAM DMA; `card_hardware_*`. |

## Physical foundation, not production timing

Production `card_rom.c` and `replaced.txt` are byte-identical to reviewed main. This candidate does not activate wire
latency, a CPU page cache, SDK DMA setup, a new polling cost or a second cartridge in runtime play. CPU adapter and
actual SDK completion work remain separate preserved WIP. Future CPU and DMA commands must share **one** bus owner
and the same `PortCardBusRequest`, after lifecycle/polling review. No game/SDK source, input, seed or golden changes
are part of this foundation. [ADR0028](../../docs/adr/0028-card-wire-and-physical-dma-progress.md) records the scope.

The bus implements independently expressed rules from the pinned test oracle's **model**, not a retail-card
measurement: [melonDS1.1 NDSCart.cpp](https://github.com/melonDS-emu/melonDS/blob/1.1/src/NDSCart.cpp),
`WriteROMCnt`, `ROMPrepareData`, `AdvanceROMTransfer`, `ReadROMData`, `ROMEndTransfer`.
Archive SHA256: `61e339bcb18a68a17485973637d972ea628c5624d7e6b8adf6870f895d5e26fd`. No implementation text was imported.
For byte clock C5/8, first READY is C*(8+L1+L2+4); subsequent READY follows consumption+C*(4+L2 at a 512-byte
boundary); zero-length completion is C*(8+L1). An unread READY word holds indefinitely. Premature reads return the
old latch. Final consumption clears BUSY and optionally latches completion. Latch/ack and IRQ delivery are distinct.
The engine honors supplied L1 even for ID: original SDK `CARDi_ReadRomIDCore` masks L1 when building its command;
that policy belongs to the future adapter. Backends supply immutable bytes/ID with whole native context pointers.

Only typed ordinary NTR READ and ID commands exist. DSi, encrypted/secure phases, writes, status/refresh commands
and raw ROMCnt/slot ownership are unsupported. Invalid size/latency/address range, backward time, backend error
and deadline overflow fail explicitly. No CPU-work, FS, allocation, copy or game-AI cost is inferred.

## Test-activated physical events

The closed core event kind calls physical state transitions directly, even during old IRQ Spin. It never invokes
SDK handlers/tasks, generic timed callbacks, frame hooks or arbitrary observer functions. READY demands an armed
repeat-one-word channel; COMMIT consumes the latch, writes four bounded bytes and schedules the next READY from
actual consumption. Final consumption latches CARD_DATA IF, never an ordinary DMA IRQ.

Known native-mainRAM service occupies nine normalized cycles per word, the specific pinned IO32-to-mainRAM count 1
burst class in [the DMA guide](../dma/README.md). Timers/lines/physical events progress during that interval; ARM9,
IRQ delivery and generic host work wait until unit release. Own-work Spin excludes that stall once; absolute waits
and Halt count it toward their existing deadline. A target inside a stall can return later because ARM9 is stopped.
Equal-cycle timer/line/timed events retain their relative order; due physical transitions close before ARM9 or host
work resumes. The exact timerIF64/READY 64/COMMIT 73 case is tested, without a general hardware-parity claim.

The single armed channel owns a whole native-main-arena range. Unknown pointers, misalignment, invalid channels,
overlap and exhausted payload bounds cannot gain writes. Stopped/reset tokens cannot commit; epochs are monotonic
across reset and exhaustion stays fail-closed. Reset/cancel and OS run teardown discard pending events before their
buffers disappear. Actual MI Stop/StopAll cancel this experimental channel. Generic reuse fails visibly rather
than stealing it; production MIi_CardDmaCopy32 remains deliberately unsupported. No new production feature flag.

## Shared CPU latch boundary

`port_card_hardware_configure` accepts an immutable backend only while the controller and DMA are idle. A CPU command
uses the actual OS clock and receives a monotonic generation token. DMA cannot arm during that command, and a CPU
command cannot start while DMA owns a destination. Both use the same physical bus; no second cartridge is created.

`port_card_hardware_read_cpu` returns the old latch before READY. Consuming READY schedules the next word from the
actual consumption time; an unread word holds indefinitely. Final consumption clears BUSY and publishes CARD_DATA
IF immediately, before the caller's separate destination store. The API neither advances a CPU-work estimate nor
delivers an IRQ handler. Absolute waits and the OS's existing interrupt boundaries retain those responsibilities.

Cancel/reset invalidate old tokens, including across reinitialization. A stale read/cancel cannot consume or stop a
later command. Backend failure at READY cancels the command, retains `PORT_CARD_BUS_IO` in `state.result`, invalidates
its token and raises no completion IF. A subsequent token read returns BUSY; inspect the retained result for the
cause. Invalid output or deadline overflow does not silently consume a ready word: correct the call or cancel it.

These are controller tests, not an activated SDK CPU reader. `card_rom.c` still supplies the existing production
fallback. Cache/page/ID/task integration, original polling/store/copy work, DMA selection and authentic gameplay
timing validation remain separate. No device minimum or observed scene duration is presented as a generic CPU cost.

## Backup WRITE service

Successful nonempty WRITE bytes precede a delayed result/PXI reply by `OS_MilliSecondsToTicks(spec.writePage) * 64` cycles;
25 ms gives 837,824. This is the observed original ARM7 service policy, not flash-busy or CPU time. All other request
costs remain unmodeled. Bus reset and backup boot retire pending replies without rolling back image bytes. Path changes also retire replies,
but separately keep the existing API behavior of freeing the old image/dirty state; unflushed old-path bytes need
not persist.
An old queued callback checks current ownership and deadline before touching the command. The genuine SDK keeps
page splitting, cancellation between pages, BUSY/REQ and callback ownership; host timed callbacks never call SDK.
`card_backup_service_*` exercises actual SDK/PXI/OS; the explicit-clock `card_backup_deferred_reply` isolates stale
callback and sentinel arithmetic. Full runtime acceptance remains separate.

The actual-game `game_wrapper` and `game_error` scenarios call `writeFlashWaitSuccess` from
`src/gfl/backup_card.c` through genuine CARD tasks. They prove three page waits, sound/independent timed work,
high-buffer readback, error callback and released locks/flags. The host-only wait ignores its return: the original
finisher remains the result/error/unlock authority. No replacement CARD API or clock increment is used.
