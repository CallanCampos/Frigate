# 0005: The platform boundary as built

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D3, D5, §4.5, §4.7
- **Upstream:** `4aa041311` (merge of PR #348)

## Context

PLAN.md D3 chose a hybrid boundary: the decomp's game code, GFL, NitroSystem and the portable parts of NitroSDK
compile unchanged, the hardware-bound parts get host versions, and display state is modelled for the renderers. Phase 1
built it ([STATUS.md](../STATUS.md), 2026-10-17) and settled what the plan left open. The three spikes in
`port/spikes/` and the research in [sdk-surface.md](../research/sdk-surface.md) are the evidence.

## Decision

- **The SDK's own C stays wherever it runs on a host.** Only what can't is replaced: MWCC `asm` functions, files bound
  to the ARM9's hardware (CP15, CPSR, TCM, the exception vectors, card registers), and files that keep DS addresses
  in forms the header translation can't reach. Each module lists what it replaces in `port/hal/<module>/replaced.txt`.
- **Replaced asm is proven against the ROM.** Every host version of an SDK asm function is compared with the ROM's own
  machine code, run in a test-only CPU emulator, on thousands of seeded inputs in both versions
  (`port/tests/asm_oracle`). The emulator is a test tool only (PLAN.md §1).
- **Threads run on fibers under the SDK's own scheduler** (`port/hal/os`, from the fibers spike). One host thread runs
  the game; the decomp's `os_thread.c`, mutexes, messages and events decide which thread runs, so DS ordering holds and
  runs are deterministic. Interrupts are taken only at defined points: a thread waiting or halting, interrupts being
  re-enabled with one pending, a modelled busy wait. A virtual clock of 263 lines a frame drives VCOUNT, HBlank,
  VBlank, timers and alarms.
- **The ARM7 is replaced at the PXI protocol level** (`port/hal/pxi` and the services beside it: power, touch, firmware
  settings, reset, cartridge slot, microphone, the backup chip, and a sound driver). The ARM9's SDK C talks to them
  exactly as to the DS's ARM7, so the ARM9 side keeps its behaviour, timing contracts and error paths. A service that
  ARM9 code busy-waits on answers before the call returns. The real-time clock is the exception, replaced at its API
  because its ARM9 file has an asm helper: it reads the host's clock, or a fixed time for tests (`--fixed-time`).
- **DMA is done at once by the CPU** (`port/hal/dma`): each transfer copies unit by unit in the hardware's order, an
  asynchronous one raises its interrupt as the hardware would, and transfers to the geometry FIFO go to the geometry
  command log.
- **ROM reads come from the player's dump**, memory-mapped (`port/hal/rom`, `port/hal/card`); saves are a 512 KiB
  flash image written atomically; overlays are all resident, and loading one restores its writable statics
  (overlay-sections spike).
- **VRAM banks alias into the CPU views** by page mapping where the host allows (`port/hal/gx`, vram-alias spike), with
  a copying backend that gives the same results.
- **Heaps are sized for 64-bit structs** by a host policy (`port/hal/gfl/heap_policy.c`) that `src/gfl/heapsys.c`
  calls through a hook that is the bare size for MWCC.
- **Code that isn't C yet stays a trap**, except where the plan defers a whole subsystem: Nintendo Wi-Fi Connection
  (phase 7). DWC's initialization runs at boot and reaches four overlay 11 symbols that upstream's claim hasn't
  written yet; `port/hal/dwc/ov011_pending.c` gives the effect they have offline and is deleted symbol by symbol as
  upstream's C lands (the link fails with a duplicate symbol).

## Alternatives

- **Replace NitroSDK at its API, libntr-style:** reimplements what the decomp already has as matching C, and loses the
  SDK's own behaviour wherever the replacement differs.
- **A full hardware model with the ARM7's services behind PXI emulated register by register:** an emulator without the
  CPU (PLAN.md §1).
- **Preemptive host threads:** nondeterministic, which breaks replays and multiplayer.
- **Reimplement the ARM7 services at the ARM9 SDK's API:** more code than the protocols, and it would replace ARM9
  logic that the decomp has as C.

## Consequences

- Every replaced function has a documented reason and, for asm, a differential test. Upstream's SDK C keeps flowing
  into the port with each sync.
- Timing differs from the DS where interrupts would arrive mid-computation; ordering and determinism don't.
- The protocols of the ARM7 services are derived from the ARM9 side, since the ARM7 isn't decompiled; where the ARM9
  code doesn't show the ARM7's behaviour, the service documents its choice.
- Revisit the Wi-Fi stand-ins when overlay 11's C lands, and the ARM7 sound driver's fidelity in phase 2.
