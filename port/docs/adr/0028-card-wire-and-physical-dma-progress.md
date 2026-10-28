<!-- Native physical CARD/MI foundation, below future original SDK integration; serves every ROM/file read. -->
# 0028: Keep the CARD physical model separate from production SDK integration

- **Status:** accepted for the independent physical foundation only; production SDK integration deferred
- **Date:** 2026-10-28
- **Plan:** §4.5 (SDK boundary), §6 (verification), phase 5

## Context

Equal ROM bytes or request streams do not establish equal wire time, SDK work or AI scheduling. Original CARD reads
pages and IDs; its repeating DMA consumes individual ready words and completes through CARD_DATA IRQ. A single copy
plus delay cannot reproduce that state machine. Native pointers also cannot fit the SDK's 32-bit destination fields.

A separately preserved CPU-wire adapter changed New Game's exact animated name-confirmation screenshot during
validation. That diagnostic remains open; this foundation must not import that runtime change or silently adjust
goldens, seeds, inputs or masks. Production `hal/card/card_rom.c` and `replaced.txt` remain byte-identical to the
reviewed main base. No CPU-reader test or production CARD timing/selection is included here.

## Decision scope

Build independent native modules and exercise them through manufactured immutable data and the actual SDK OS
scheduler before wiring them into production CARD. There is no emulator, copied SDK variant, game replacement,
new feature flag or general timing knob.

- Caller-owned wire state accepts absolute system cycles and typed ordinary NTR READ/ID requests. Command bytes,
  L1/L2/clock, word consumption and 512-byte gaps determine deadlines. An unread word holds READY. Completion
  latching, acknowledgement and IRQ delivery are distinct. Original SDK policy, including ID's L1 masking, remains
  the future adapter's responsibility.
- An additive absolute device wait spends IRQ/preempting-thread time toward its target. The existing own-work
  meaning of Spin and GetTick step remain. Real IRQ handlers run on the dispatcher host stack; a guarded wait can
  publish host work there while nested ARM9 IRQ delivery remains masked. Callbacks cannot block in the SDK or reenter.
- Closed physical READY/COMMIT events run in the clock itself, including old IRQ Spin. They may update device state,
  store one bounded word and latch IF; they never call SDK handlers/tasks or arbitrary host callbacks. Generic host
  work remains separate. Equal-cycle existing events latch/queue in their old relative order, then due physical
  transitions close before ARM9/IRQ/host work resumes.
- A known native-mainRAM repeat-one-word service class stalls ARM9 for nine normalized cycles while other events
  progress. Own CPU work excludes the stall once; absolute deadlines do not extend. No CPU/FS/allocator work is
  inferred. Unknown destination provenance and multichannel arbitration are unsupported, not assigned fake costs.
- Single-channel ownership carries whole native bounds and a monotonically advancing token epoch. Stop/reset
  invalidates old tokens without ABA reuse; exhaustion stays fail-closed. Run entry/exit cancel pending stores before
  output storage/fibers disappear. Unsupported generic reuse fails visibly; inactive generic DMA is unchanged.

Activation is explicitly test-only. Future CPU and DMA commands must share **one bus owner** and PortCardBusRequest;
these modules do not imply a second physical cartridge. Actual SDK page/cache/ID sequencing, CARD_DATA completion,
asynchronous callback context, native DMA eligibility and polling liveness remain separate unfinished work.
MIi_CardDmaCopy32 setup remains deliberately unsupported; CARDi_ReadRom still uses its existing task fallback.

## Evidence and alternatives

Primary policy is original `lib/nitro/src/card/{card_rom,card_common,card_task}.c` and `mi/mi_dma_card.c`. The physical
rules are checked against pinned [melonDS1.1 cartridge](https://github.com/melonDS-emu/melonDS/blob/1.1/src/NDSCart.cpp),
[DMA](https://github.com/melonDS-emu/melonDS/blob/1.1/src/DMA.cpp),
[burst timings](https://github.com/melonDS-emu/melonDS/blob/1.1/src/DMA_Timings.cpp) and
[NDS event loop](https://github.com/melonDS-emu/melonDS/blob/1.1/src/NDS.cpp). No implementation text was copied.
This is an emulator model, not a retail-hardware measurement. IO32-to-mainRAM count 1 uses each fresh burst's first
entry 9, not later entries 3; timestamp units are normalized once. That evidence is not a battle-specific constant.

Rejected alternatives: whole-page memcpy plus total delay; generic callbacks deferred until an IRQ returns; advancing
GetTick globally; unbounded/native-pointer truncation; resetting epochs to zero; delivering a same-cycle IRQ before
READY stops ARM9; silently enabling incomplete SDK DMA or turning a nonblocking getter into completion waiting.

Pinned CP15.cpp also approximates cached instruction/data timing and has instruction-cache lookup disabled. Getter
traces cannot prove authentic warm/cold cache behavior. No poll quantum is chosen here, and future polling must stay
nonblocking with snapshot semantics. Wire/service time alone cannot establish trainer-AI deadline fidelity.

## Validation and remaining work

The isolated foundation passes 22/22 focused tests in ordinary and fatal LLVM ASan/UBSan builds, with zero skips;
the manifest-tool's 14 ROM-free tests pass. These local results accept only the foundation boundary, not production
activation. Source/control hashes are fixed through each run. Canonical full gates and owner review remain below.

Foundation tests cover exact wire times, delayed consumption, native bounds, generation/reset ABA, hardware stalls,
masking, old IRQ Spin, Halt, stop/reset and same-cycle timerIF64/READY 64/COMMIT 73 behavior. Actual scheduler tests
cover absolute waits, preemption, callbacks and stop. Plain and fatal ASan/UBSan builds are required. The manifest
configure probe mocks only target-registration commands and retains all schema/output assertions.

Before publication: owner review, both ROM hashes and canonical full native gates remain required. Before runtime
integration: actual SDK lifecycle/polling review, both versions' full native/strict suites, source/traffic/oracle
comparisons and save/reload checks, with every mismatch diagnosed. ADR0023 remains proposed; accepting this limited
foundation does not accept a CPU adapter, production DMA, AI parity, whole-game completion or the overall goal.


## 2026-10-28: shared CPU command boundary, still inactive in gameplay

The follow-up gives the same physical controller explicit CPU ownership and monotonic command tokens. CPU and DMA
cannot overlap. A CPU consume schedules the next READY or publishes final transfer IF before its caller's separate
memory store; it does not deliver an IRQ or invent instruction costs. Cancel/reset invalidate old tokens, and backend
failure retains an explicit IO result without completion. Both ordinary and fatal ASan/UBSan builds pass the six new
CPU scenarios and every existing physical-controller case in both games. The unchanged production reader, original
SDK cache/task integration and unresolved authentic New Game/AI timing remain outside this increment's acceptance.
