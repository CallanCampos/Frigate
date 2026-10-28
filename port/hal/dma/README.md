# DMA boundary

| Module | SDK/game owner and purpose |
| --- | --- |
| [dma.c](dma.c) | NitroSDK MI DMA (`lib/nitro/src/mi/mi_dma*.c`): existing generic copies/fills/GX sends remain immediate native operations and retain callback IRQ behavior. |
| [card_dma.c](card_dma.c), [card_dma.h](card_dma.h) | Independent physical-foundation channel: native main-arena bounds, advancing cursor, demand token and cancellation. Test activation only; production CARD reader unchanged. |

Original mi_dma_card.c configures fixed MCD1 source, advancing output, repeat count 1 and CARD timing. The new
[physical controller](../card/README.md) uses typed READY/COMMIT events; no SDK callback runs during service.
MIi_CardDmaCopy32 setup, actual CARD page-end handler and production DMA selection remain separate unfinished work.
Future CPU and DMA adapters must share one bus owner and PortCardBusRequest.

Nine normalized cycles per word are the specific pinned model IO32-to-mainRAM class, not a gameplay adjustment:
[DMA.cpp](https://github.com/melonDS-emu/melonDS/blob/1.1/src/DMA.cpp) Start/UnitTimings9_32/Run9;
[NDS.cpp](https://github.com/melonDS-emu/melonDS/blob/1.1/src/NDS.cpp) default IO DMA timings;
[DMA_Timings.cpp](https://github.com/melonDS-emu/melonDS/blob/1.1/src/DMA_Timings.cpp) MRAMWrite32Bursts[1].
Each repeated count 1 demand begins a fresh burst and uses first entry 9, not later entries 3. ARM9 timestamp shifting
is converted back at the system boundary; do not multiply that service class by another CPU-clock ratio. No third-
party implementation code was copied. This is emulator-model evidence, not a retail benchmark or CPU/FS cost model.

Arm accepts only a bounded 32-aligned range proven by port_os_main_arena_holds, channel 0..3 and positive whole-word
length. It never truncates a native pointer or assumes a host stack/heap/static is DS mainRAM. Only one channel owns
the cartridge. Exhaustion holds READY without another write. Stop invalidates token/pointer. Reset advances the
epoch instead of reusing it; UINT64_MAX exhaustion stays fail-closed. Generic operations cannot steal an armed
channel; harmless zero-byte copies remain no-ops. MI Wait rejects unsupported experimental ownership, while actual
MI Stop/StopAll cancel it. No new cost is assigned to SDK stop's register reads.

Unknown/TCM destinations and multichannel arbitration are unsupported, not silently assigned this timing class.
LP64 alignment is not DS mode parity. Polling, SDK lifecycle, CPU instructions and gameplay timing still need review.
