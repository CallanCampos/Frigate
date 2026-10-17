# pokebw2: SDK surface and where the platform layer of a native port should sit

Scope: Black 2 (b2_us) ARM9 code as configured in `config/b2_us/arm9/**` of worktree
`desktop-subagent-inspection-3190ba` (all of main, ITCM/DTCM, autoloads, LTD autoload and 344 overlays). White 2 shares
the source, so the numbers carry over.

## How the numbers were made (and their limits)

Everything is reproducible from the scripts in `scratchpad/research/work/sdk-surface/`:

| Script | What it does |
| --- | --- |
| `db.py` | Parses every module's `symbols.txt`, `delinks.txt` and `relocs.txt`. Each function is assigned to its delinked source file, or for the ~2,600 functions in undelinked gaps, to the library of its neighbours. Each call/branch/pointer relocation becomes a caller→callee edge (199,455 edges; 43,384 functions). |
| `binscan.py` | Heuristic per-function scan of the extracted binaries (`extract/b2_us/...`). It decodes ARM/Thumb PC-relative literal loads, ARM `mov #imm` and Thumb `movs`+`lsls` constants, `swi`, CP15 `mcr/mrc` and `msr/mrs`, then buckets constants into hardware regions (2D-A/B, DMA, timers, IRQ, VRAMCNT, DIV/SQRT, POWCNT, 3D ctrl, G3 FIFO/commands, 3D status, IPC/PXI, card, TWL SCFG/NDMA/camera/DSP, PLTT, VRAM, OAM, DTCM, ITCM, shared main memory). Base+offset register addressing is not resolved, so the counts are lower bounds. Small constants such as 0x03000000 can be false positives. The VRAM range 0x06898000–0x068A0000 is excluded because overlay 169 runs there. |
| `hwscan.py` | Scans all C/H for the project's register macros (`reg_*`, `REG_*`, `HW_*`; 231 hardware macros), raw `(vuN *)0x...` casts, `asm` functions/blocks, CP15, SWI. |
| `surface.py`, `boundary.py`, `classify.py`, `gfl.py`, `dossier.py`, `callers.py` | Aggregations quoted below. JSON outputs: `surface.json`, `sdk_used.json`, `b_used.json`, `dossier.json`, `inlines.json`, `hwscan.json`. |

Because relocations cover code that is still assembly, the call graph is complete even where the C is not. Game code is
36,796 functions, and 30,080 of them (82%) sit in files marked `complete`.

**The ARM7 is not in the decomp.** The ARM7 side (`extract/b2_us/arm7/arm7.bin` 164 KB and `dsi/arm7i.bin` 284 KB)
has no config. Everything the ARM9 SDK asks of the ARM7 over PXI has no C to reuse. This covers the sound sequencer and
mixer, touch panel, RTC, power, mic, firmware NVRAM, card backup, wireless, and the DSi camera, NWM, FATFS and SNDEX
services. A port must implement these behind some boundary, whichever option is picked below.

## ARM9 main layout (address order)

| Range | Contents |
| --- | --- |
| 0x02004000–0x02005124 | crt0 (`Entry`, `boot_InitTCM`, `boot_BootstrapExec`, `MIi_UncompressBackward`, autoload callback) and the TwlSDK SVC wrappers (`Div`, `Mod`, `Sqrt`, `CpuSet`, `CpuFastSet`, `LZ77UnComp*`, `RLUnComp*`, `HuffUnComp*`, `BitUnPack`, `SVC_GetCRC16`, `SVC_WaitByLoop`, `IntrWait`, `VBlankIntrWait`, `Halt`, `SoftReset`, `IsDebugger`). There are 34 functions and none are in C. |
| 0x02005124–0x02039988 | Game system code (`src/system`, starting with `main.c`) |
| 0x02039988 | NitroSDK RC4 |
| 0x02039a80–0x02050a40 | GFL (`src/gfl`) |
| 0x02050a40– | SPL particle library |
| …–0x0205ea3c | DWC (account, backup, AC, WDS) in ARM9 main |
| 0x0205ea3c–0x0206ed94 | NitroSystem: FND, GFD, G2D, G3D, SND |
| 0x0206ed94–0x020827f8 | NitroSDK (card, cp, fs, fx, gx, math, mi, nvram, os, pxi, rtc, snd, spi, std, wm, ctrdg) plus TwlSDK SCFG and syscall |
| 0x020828c8–0x0208ee28 | MSL C library, fdlibm math, soft-float/division runtime (`fp_fastI.c`) |

Other places code runs:

- **ITCM** (0x01FF8000–0x01FF939C, 5 KB) holds:
  - four **GFL cell-actor renderers** (`mainRenderer`, `func_01ff8318/8610/8900`), reached through a table in `src/gfl/clact.c:419`;
  - `MIi_DmaSetParameters`;
  - `OS_IrqHandler` and the IRQ thread-switch tail `func_01ff8ce0` (calls `CP_SaveContext`/`CPi_RestoreContext`);
  - `OSi_DoResetSystem` and helpers, which use DSi BIOS SWIs 0x24–0x28;
  - `SCFGi_SwitchCpuSpeed`.
- **DTCM** (0x02FE0000) holds `OS_IRQTable`, `OSi_IrqThreadQueue`, the IRQ/SVC stacks, and `HW_INTR_CHECK_BUF` at DTCM+0x3FF8, which the inline `OS_SetIrqCheckFlag` writes. It also holds the **GFL DTCM heap** (`GFL_HeapDTCMInit(0x2000)` in `src/system/gfl_use.c:123`, `src/gfl/heap_dtcm.c`).
- **Overlay 169 runs from LCDC-mapped VRAM** at 0x06898020 (`docs/configs.md:103`), and autoload 3 sits at 0x06898000.
- **Autoload 2 and overlays 218–225** sit at 0x02400000+, which is DSi-only main RAM.
- **LTD autoload** (0x02700000) holds DSi libraries. Its `.ltdmain` (461 functions) is not delinked and contains NDMA, WRAM slots, PM exit callbacks and the TWL card secure-ROM read.

---

## 1. Library layering

Classes:

- **A**: portable logic that compiles natively, possibly after mechanical C rewrites of `asm` helpers.
- **B**: touches hardware and must be reimplemented or backed by a host model.
- **B-lite**: incidental hardware only (the divider, arena bounds, owner info).
- **C**: irrelevant on PC, so stub it.

"n" is the function count in the ROM and "cmpl" is how many are in complete files. HW evidence comes from the binary
scan (functions hitting each region) and the C scan (macros).

### NitroSDK (`lib/nitro`, 1,142 functions in ARM9 main and overlay 181; 122 C files)

| Module | n / cmpl | Class | Evidence (registers, memory, asm, PXI) |
| --- | --- | --- | --- |
| os/thread, context, system, interrupt, irqHandler, irqTable | part of os 246/187 | **B** | `os_context.c` `OS_SaveContext/LoadContext/InitContext` are asm. `os_system.c` has 9 asm functions: `OS_Enable/Disable/RestoreInterrupts` (CPSR via `msr/mrs`), `OSi_SpinWaitCpuCycles`, `OS_GetProcMode`. `reg_OS_IME/IE/IF` (0x04000208/210/214). `OS_IrqHandler` and the thread switch live in ITCM, `OS_IRQTable` in DTCM. 12 os functions use `msr/mrs`. |
| os/tick, timer, alarm, valarm | ″ | **B** | `reg_OS_TM0CNT_L/H`, `TM1CNT_L/H` (0x04000100–0x04000106). `OSi_AlarmHandler` is asm. VAlarm uses `GX_SetVCountEqVal` and `reg_GX_DISPSTAT`. |
| os/cache, protectionRegion/Unit, tcm, terminate_proc (`OS_Halt`), exception | ″ | **B→no-op / C** | CP15 asm: `DC_*`/`IC_*` (9 CP15 ops), `OSi_Set/GetPRParam0–7` (16), `OS_Enable/DisableProtectionUnit`, `OS_GetDTCMAddress`, `OS_Halt`. 27 os functions contain CP15 instructions. |
| os/arena, alloc, ownerInfo(Ex), entropy | ″ | A / **B-lite** | `os_arena.c` uses `HW_*` linker bounds (10 hardware references). `os_ownerInfo*` reads `HW_NVRAM_USER_INFO` and `TWL_SETTINGS` in shared memory (0x02FFFC80…). |
| os/mutex, message, event, printf, address_check, vramExclusive | ″ | **A** | No hardware. Needs the thread core. |
| os/spinLock, reset, reset_pxi, deliverArg, codecmode, emulator, application_jump | ″ | **C** | `HW_INIT_LOCK_BUF` and `HW_CARD_LOCK_BUF` (21 references in spinLock), the ARM7 reset protocol over PXI tag OS, emulator detection. |
| mi/memory, swap, uncompress | 20 | **A** (asm→C) | 20 asm functions: `MI_CpuFill8/Copy8/Copy/Move/Comp8`, `MIi_CpuClear/Copy16/32/Fast`, `MIi_CpuSend32`, `MI_Copy32B…64B`, `MI_Zero36B`, `MI_SwapWord`, `MI_UncompressLZ8` (LZ77 types 0x10 and 0x11, SWPB writes for VRAM). |
| mi/dma, dma_itcm, dma_gxcommand, dma_card, wram, sharedWram, init | 23 | **B** | `MI_DMA_REGS` (0x040000B0–0x040000DF), `REG_GXFIFO_ADDR`, `reg_G3X_GXSTAT`, `reg_MI_WRAMCNT`. `MI_SendGXCommandAsync` installs a DMA-end IRQ. |
| gx (g2, g3imm, g3_util, g3x, gx, gx_bgcnt, gx_load2d/3d, gx_state, gx_vramcnt, gxasm) | 166/166 | **B** | Fully in C. 87/166 functions hit hardware in the binary scan. Hits include `reg_GX_DISPCNT` (31), `reg_GXS_DB_DISPCNT`, `reg_GX_VRAMCNT_A…I` (≈110), `reg_G3X_DISP3DCNT`, `reg_G3X_GXFIFO`, `reg_G3X_GXSTAT`, `reg_G3_MTX_MULT_3x3`, `HW_BG_VRAM`/`HW_DB_BG_VRAM`, `CLIPMTX_RESULT`/`VECMTX_RESULT`, and the clear/fog/edge/toon tables. `GX_SendFifo48B/64B` and `GXi_NopClearFifo128_` are asm. Plus **86 static inlines in `gx.h`**. |
| fx (fx, atanidx, trig, sincos, mtx22/33/43) | | **A** (asm→C) | 15 asm matrix helpers (`MTX_Identity*_`, `MTX_Rot*`, `MAT3_*`, `MTX_Copy43To44_`, …). |
| fx/cp, fx/vec, fx/mtx44; cp/cp_context | 63/23 | **B-lite** | Hardware divider and square root: `reg_CP_DIVCNT`, `DIV_NUMER/DENOM/RESULT`, `SQRTCNT/PARAM/RESULT` (0x04000280–0x040002BF), 19 functions in the binary scan. `CP_SaveContext`/`CPi_RestoreContext` (asm) save divider state across IRQs. Replace with software. |
| math, std, crypto (rc4) | 29/16, 17/6, 2/2 | **A** | asm: `MATH_CountLeadingZerosFunc`, `MATH_QSort`, `sha1_block`, `CRYPTO_RC4Encrypt`. `math.h` has an inline `asm{}` CLZ. |
| fs (api, archive, command_default, file, memfile) | 117/117 | **A** | No hardware. Uses OS threads and messages. |
| fs/rom, fs/overlay | ″ | **B / C** | `FSi_ReadRom` → `CARD_ReadRom` is the single ROM-read hook (`lib/nitro/src/fs/fs_rom.c:42`). `fs_overlay.c` handles overlay load, digest (`SVC_CalcHMACSHA1`) and `MIi_UncompressBackward`. |
| card (common, rom, backup, request, pull_out) | 71/42 | **B** | `reg_MI_MCCNT0/1`, `MCCMD0/1`, `MCD0/1` (0x040001A0–0x040001AF, 0x04100010), `HW_CARD_ROM_HEADER`, `HW_ROM_HEADER_BUF`. Backup goes through PXI tag CARD (`CARDi_Request`). |
| pxi (fifo, init) | 6/6 | **B** | `reg_PXI_FIFO_CNT`, `SEND_FIFO`, `RECV_FIFO` (0x04000184/188, 0x04100000), `reg_PXI_MAINP_INTF`. 17 tags are used (NVRAM 4, RTC 5, TP 6, SOUND 7, PM 8, MIC 9, WM 10, FS 11, OS 12, CTRDG 13, CARD 14, FATFS 20, CAMERA 21, NWM 22, SCFG 23, SNDEX 24, plus the photo-sign channel). |
| snd (main, interface, bank, work, alarm, util) | 73/73 | **A** (ARM9 side) | No hardware: builds `SNDCommand` lists and reads `SNDSharedWork`. |
| snd/command | ″ | **B** | `SND_FlushCommand` sends packets over PXI tag SOUND to the **ARM7 driver** (not in the decomp). |
| spi: tp, pm, mic | 82/82 | **B** | PXI request/reply protocols. `pm.c` also writes `reg_GX_POWCNT`, `DISPCNT` and uses the divider. `MicWaitBusy` is asm. |
| rtc | 25/25 | **B** (api) / A (convert) | PXI tag RTC. `RtcWaitBusy` is asm. `rtc_convert.c` is portable. |
| nvram | 21/21 | **B** | PXI tag NVRAM. Only DWC uses it. |
| wm (9 files) | 80/80 | **B / C** | DS local wireless runs on the ARM7 (PXI tag WM, 47 cache operations). |
| ctrdg | 10/10 | **C** | `reg_MI_EXMEMCNT`, GBA slot. The game only calls `CTRDG_DummyInit`. |
| mb (overlay 181) | 88/28 | **C** | DS Download Play parent. |

### NitroSystem (`lib/nnsys`, 680 functions; 508 in complete files)

| Module | n / cmpl | Class | Evidence |
| --- | --- | --- | --- |
| fnd | 58/58 | **A** | No hardware. Needs only `MIi_CpuClear32` and `OS_Alloc/FreeToHeap`. |
| gfd (VRAM managers, transfer queue) | 81/81 | **A over GX** | No hardware. Calls 47 SDK functions, all `GX(S)_Load*`, `GX_Begin/EndLoad*` and `DC_FlushRange`. |
| g2d (cell, animation, multicell, renderer, OAM manager, loaders) | 182/98 | **A over GX/G3** | Produces OAM attributes and VRAM image proxies. `HW_OAM`/`HW_DB_OAM` are the only direct writes. The "3D surface" renderer path (`DrawOamToSurface3D_`) uses `G3_LoadMtx43` and G3 immediate commands. Calls 30 SDK functions. |
| g3d (kernel, sbc, glbstate, gecom, anm, util, …) | 166/117 | **A producing a G3 command stream** | `reg_G3X_GXFIFO` (31), `reg_G3_MTX_MODE/STORE/RESTORE/PUSH/POP/IDENTITY`. Packed commands go through `NNS_G3dGeBufferOP_N` → `NNS_G3dGeFlushBuffer` → `NNS_G3dGeSendDL` → `MI_SendGXCommandAsync(Fast)` / `MI_SendNDmaGXCommandAsync(Fast)`. It reads back `G3X_GetClipMtx` and `G3X_GetVectorMtx`. Calls 46 SDK functions. |
| snd (sound archive, players, heap, stream, wave out, capture) | 193/154 | **A over SND+FS+OS** | No hardware. Calls 80 SDK functions: 51 `SND_*`, `FS_*`, `OS_CreateThread`/mutex/message, `PM_*SleepCallback`, cache. |

Portability caveat for all of NitroSystem: the loaders turn file offsets into pointers **in place in 32-bit fields**
(`NNSi_G2dUnpackOffset` in `g2d_NCL_load.c:36` and `g2d_NCE_load.c:47`, and likewise in the G3D and sound-archive
binders). That is fine on a 32-bit build or with 32-bit-addressable allocations, but it is a hazard on 64-bit hosts.

### TwlSDK (`lib/twl`, about 742 functions in ARM9 main, ITCM, the LTD autoload and overlay 257)

| Module | n / cmpl | Class | Evidence |
| --- | --- | --- | --- |
| scfg | 4/4 | C | `reg_SCFG_CLK`. `SCFGi_SwitchCpuSpeed` is asm in ITCM. |
| syscall (ARM9 main and LTD) | 11/11 | B→host / C | `swi 0x24–0x27` (`syscall_twl_ltd.c`): DSi SHA1/RSA BIOS. |
| camera | 58/58 | C (or webcam) | `reg_CAMERA_CNT/MCNT`, `reg_SCFG_CLK`, PXI CAMERA. |
| dsp | 62/39 | C | `reg_DSP_PCFG/PSTS/PDATA/PMASK/SEM/PADR`, `reg_MBK_B/C`, `reg_SCFG_RST`. Used for the shutter sound, G711 and scaling. |
| mic (MICEX), snd (SNDEX), nwm, fatfs, rtc (ex), photo_sign, card | | C | PXI tags MIC, SNDEX, NWM, FATFS. |
| `.ltdmain` (undelinked, 461) | 0 | B/C | `MI_NDma*`, `MI_SendNDmaGXCommandAsync*`, `MI_AllocWramSlot`/`FreeWram`, `PM_ReadyToExit`/`*PreExitCallback`/`SetAutoExit`, `CARDi_ReadSecureRom`/`InitTwlRom`. Hits TWL-SCFG (15 functions) and TWL-NDMA. |
| SSP JPEG codec and EXIF (`lib/twl/src`, overlay 257) | in twl/- 70/37 | **A** | Pure C codec. |

Setting `OS_IsRunOnTwl()` to FALSE takes the NTR path almost everywhere. The game calls it 67 times, and it gates heap
setup, memory size, the Xtransceiver camera and NDMA.

### Other libraries

| Library | n | Class | Evidence and notes |
| --- | --- | --- | --- |
| SPL (`lib/spl`) | 54/54 | **A over G3 immediate** | `reg_G3_POLYGON_ATTR` (8). 11 functions write G3 command ports. Calls `G3_LoadMtx43`/`G3_MultMtx43` and `GX_LoadTex/TexPltt`. Only GFL calls SPL (18 functions). |
| MSL C library and math (`lib/msl`) | 95+13+9 | A, or use the host libc | No hardware. |
| MSL runtime `fp_fastI.c` | 47 | **C (delete)** | 51 asm soft-float and division helpers (`_fadd`, `_s32_div_f`, `__aeabi_*`, …). The game reaches them 4,550 times, but these are compiler-emitted calls that disappear with a host compiler. |
| DWC, NitroWiFi (CPS, SOC, SSL, WCM), dpw, poke_net, dwcutil(ui) | ~3,000 | A over sockets / B (WCM→WM/NWM) | Mostly no hardware. `dwc/vct` uses DIV/SQRT registers. `dwc/crypt` has 6 asm big-number helpers. The game's network glue (`src/gfl/net*.c`, `src/ov011/dwc_rap.c`) sits on top. The Nintendo WFC servers are gone, so stub these for milestone 1. |
| irc (overlay 31) | 26/26 | **B / C** | IR goes through the card SPI: `reg_MI_MCCNT0` (11) and `reg_MI_MCD0`. `src/ov338/ir_check.c` pokes them too. |
| DS Protect (overlays 165 and 337) | 77/18 | **C** | 31 asm functions, CP15, `REG_ROMCTRL`, `REG_SCFG_A9ROM`, `REG_AUXSPICNT_H`. Stub the stubs to report "untampered". The game calls `DSProt_*` at 18 sites, including `GameSystem_DSProtTamper1`. |

---

## 2. Game-side surface (`src/`, including code that is still asm)

Totals: **950 distinct library functions referenced by game code, at 11,070 call/pointer sites.** Here "game code"
means `src/` plus GFL's ITCM renderers and the tail of overlay 182.

| Callee layer | Distinct functions | Sites | Of which called from GFL |
| --- | --- | --- | --- |
| NitroSDK | **384** | 5,072 | 177 |
| NitroSystem | **238** | 788 | 111 |
| MSL (mostly compiler runtime) | 37 | 4,550 | 4 |
| Network libs (DWC 156, dpw 29, poke_net 16, irc 9, dwcutil 5, NitroWiFi 3) | 218 | 544 | 0 |
| TwlSDK | 48 | 78 | 0 |
| SPL | 18 | 18 | 18 |
| DS Protect | 7 | 18 | 0 |

### By family

In the table below, "funcs" is the distinct callees. "Calls" is branch sites. "ptr" counts pointer references, and calls
into the LTD module go through veneers, so they show up as pointer references.

| Family | Funcs | Calls | Ptr | Top functions (references) |
| --- | --- | --- | --- | --- |
| MI_ | 18 | 2,354 | 140 | `MI_CpuFill8` 1637, `MI_CpuCopy8` 503, `MIi_CpuCopy16` 94, `MIi_CpuClear32` 76, `MIi_CpuCopy32` 49, `MIi_CpuClearFast` 41, `MI_Copy36B` 12, `MI_StopNDma` 6, `MI_UncompressLZ8` 5, `MI_CancelWram` 4, `MI_WaitDma` 1 |
| G2x_/GXx_ (register-address helpers behind the inline macros) | 7 | 422 | 2 | `G2x_SetBlendAlpha_` 171, `GXx_SetMasterBrightness_` 126, `G2x_SetBlendBrightness_` 75, `G2x_SetBGyAffine_` 23, `GXx_GetMasterBrightness_` 20 |
| OS_ | 47 | 336 | 4 | `OS_IsRunOnTwl` 67, `OS_GetTick` 38, `OS_Terminate` 35, `OS_RestoreInterrupts` 31, `OS_GetMacAddress` 30, `OS_DisableInterrupts` 29, `OS_WaitIrq` 13, `OS_ResetSystem` 9, `OS_GetOwnerInfo` 7, `OS_WaitVBlankIntr` 6 |
| DC_ (cache) | 3 | 80 | 0 | `DC_FlushRange` 77, `DC_FlushAll` 2, `DC_InvalidateRange` 1 |
| VEC_ / FX_ / MTX_ / MATH_ | 13 / 10 / 19 / 7 | 307 / 256 / 142 / 29 | | `VEC_Subtract` 80, `FX_Div` 175, `VEC_Normalize` 67, `MTX_Identity33_` 47, `MATH_CountPopulation` 12 |
| GX_ | 59 | 252 | 2 | `GX_SetBankForLCDC` 22, `GX_SetGraphicsMode` 20, `GX_LoadBGPltt` 20, `GX_DisableBankForLCDC` 13, `GX_LoadOBJPltt` 11, `GX_SetBankForBG` 10, `GX_LoadOBJ` 10, `GX_SetBankForTex` 9, `GX_DispOn` 8 |
| GXS_ | 19 | 91 | 2 | `GXS_LoadBGPltt` 29, `GXS_LoadOBJPltt` 18, `GXS_SetGraphicsMode` 10 |
| G2_ / G2S_ | 8 / 5 | 41 / 9 | | `G2_GetBG1CharPtr` 17, `G2_GetBG1ScrPtr` 9. These return VRAM pointers that the game then writes. |
| G3_ / G3X_ | 10 / 9 | 33 / 82 | | `G3X_SetClearColor` 33, `G3X_SetFog` 27, `G3X_SetEdgeColorTable` 10, `G3_RotZ` 9, `G3i_LookAt_` 8, `G3X_GetBoxTestResult` 2 |
| NNS_G2d | 53 | 203 | 15 | `NNS_G2dGetUnpackedScreenData` 30, `…PaletteData` 23, `…BGCharacterData` 19, `…CharacterData` 12 |
| NNS_G3d / NNS_G3D* | 36 / 20 | 136 / 48 | | `NNS_G3dGetTex` 25, `NNS_G3dGeBufferOP_N` 19, `NNS_G3dGeFlushBuffer` 15, `NNS_G3dScrPosToWorldLine` 10, `NNS_G3DFlushRenderState` 8 |
| NNS_Gfd | 16 | 94 | 1 | `NNS_GfdRegisterNewVramTransferTask` 78 |
| NNS_Snd | 73 | 160 | 13 | `NNS_SndPlayerSetVolume` 11, `NNS_SndHeapSaveState` 10, `NNS_SndArcPlayerStartSeqEx` 5, `NNS_SndWaveOut*`. 139 of the 173 references come from `src/system`. |
| NNS_Fnd | 15 | 30 | | expanded heap and list |
| FS_ | 13 | 85 | 2 | `FS_ReadFile` 25, `FS_SeekFile` 22, `FS_CloseFile` 11, `FS_InitFile` 8, `FS_OpenFile` 7, `FS_LoadOverlay`/`UnloadOverlay`/`LoadOverlayInfo`, `FS_LoadTable` |
| CARD_ | 9 | 22 | | `CARD_UnlockBackup` 6, `CARD_LockBackup` 4, `CARDi_RequestStreamCommand` 3, `CARD_WaitBackupAsync` 2, `CARD_IdentifyBackup` 1. The save path is in `gfl/backup_system.c`. |
| TP_ / PAD | 11 | 15 | | `TP_Init`, `TP_GetUserInfo`, `TP_SetCalibrateParam`, `TP_RequestSamplingAsync`, `TP_WaitRawResult`, `TP_GetCalibratedPoint`, auto-sampling. PAD is read directly: see the list of direct accesses below. |
| RTC_ | 7 | 42 | 1 | `RTC_ConvertDateToDay` 12, `RTC_ConvertDateTimeToSecond` 11, `RTC_GetDate` 8, `RTC_GetTime` 2, `RTC_Init` 2 |
| PM_ | 14 | 19 | 4 | `PM_SetAmp` 7, `PM_GetBackLight` 2, `PM_GetBattery(Level)`, `PM_GoSleepMode`, exit callbacks |
| MIC_ | 7 | 15 | | limited and auto sampling |
| WM_ / MB_ | 33 / 12 | 68 / 41 | | local wireless (`MP`, `DataSharing`, `KeySharing`, parent/child) and Download Play parent |
| SND_ / SNDEX_ | 3 / 2 | 3 / 0+3 | | `SND_Get/SetWaveDataAddress`, `SNDEX_SetIirFilter` |
| CTRDG_, SCFG_ | 1, 1 | | | `CTRDG_DummyInit`, `SCFG_SetCpuSpeed` |
| CAMERA_ / DSP_ / SSP_ (TWL, overlay 257) | 22 / 10 / 3 | via veneers 30 / 20 / 3 | | Xtransceiver camera, shutter sound, JPEG |
| STD_ | 5 | 15 | | `STD_GetStringLength`, `STD_TSNPrintf` |
| DWC_ / dpw / POKE_NET / IrcLink | 36 / 29 / 16 / 9 | 102 / 66 / 23 / 21 | | `DWC_Nd*`, friend keys; `DpwTr_*` (GTS); GDS; IR link |

Not used by the game at all: SVC/BIOS wrappers, alarms (`OS_SetAlarm` and friends are only used by libraries), PXI,
`MI_DmaCopy*` directly, and `G3B_`/`G3C_` (display-list builders). The game creates one thread of its own:
`g_GFLSndThread` in `src/system/snd_sys.c:582`.

### SDK inline functions compiled into game code

The SDK headers define 235 `static inline` functions, 125 of them NitroSDK's (86 in `gx.h`). **79 of them write or read
hardware**, and game C calls them at **1,569 sites** (lib C: 161). The top ones:

| Inline | Sites | Register |
| --- | --- | --- |
| `G2S_BlendNone` / `G2_BlendNone` | 171 / 167 | `BLDCNT` |
| `GXS_SetVisibleWnd` / `GX_SetVisibleWnd` | 144 / 146 | `DISPCNT` |
| `G3_Vtx` | 49 | `VTX_16` |
| `G3_TexCoord` | 38 | |
| `G3X_EdgeMarking` / `AntiAlias` / `AlphaBlend` / `AlphaTest` / `SetShading` | 37 / 33 / 33 / 25 / 23 | `DISP3DCNT` |
| `OS_GetVBlankCount` | 37 | `HW_VBLANK_COUNT_BUF` (0x02FFFC3C) |
| `GX_SetDispSelect` | 34 | `POWCNT` |
| `G2S_ChangeBlendAlpha` | 33 | |
| `GX(S)_SetVisiblePlane` | 31 / 24 | |
| `G2(S)_SetWnd*` | ≈150 | |
| `G3_MtxMode` / `ViewPort` / `Translate` / `Scale` / `PushMtx` / `PopMtx` / `Begin` / `End` / `PolygonAttr` | | |
| `MI_SetMainMemoryPriority` | 19 | `EXMEMCNT` |
| `GX_IsHBlank` / `GX_GetVCount` | 12 / 11 | |
| `PAD_DetectFold` | 10 | `HW_BUTTON_XY_BUF` |
| `OS_SetIrqCheckFlag` | 16 | DTCM |

**33 game files issue immediate-mode G3 commands.** Among them: `system/mcss.c` (battle Pokémon sprites drawn as
textured quads), `gfl/billboard.c`, `gfl/g3d_system.c`, the overlay 36 field files, the encounter effects in overlays
148/150/151, demo and award graphics, and `pokemontrade_3d`.

### Direct hardware access from `src/` itself (bypassing SDK functions and inlines)

These come from the C scan, so they are exact for the C that exists:

- **Key input:**
  - `src/gfl/key.c`: `reg_PAD_KEYINPUT`, plus `*(vu16 *)HW_BUTTON_XY_BUF` for X, Y and the lid
  - `src/ov032/wfc_user_info_warning.c`: `reg_PAD_KEYINPUT`
  - `src/system/net_err.c`: `reg_PAD_KEYINPUT`, `DISPCNT`, `DB_DISPCNT`, `WINOUT`, `BLDCNT`
- **DMA and IRQ:**
  - `src/gfl/dma_vblank.c`: DMA control registers `0x040000B8 + ch*12`. This is dead code, because the table is never set up.
  - `src/system/snd_sys.c`: reads `reg_OS_IE`.
- **System and power:**
  - `src/system/machine_use.c`: `reg_SCFG_EXT` (0x04004008)
  - `src/ov000/event_gsync.c`, `src/ov141/bsubway_records.c`: `reg_GX_POWCNT`
- **3D status:**
  - `src/gfl/double3Ddisp.c`: `reg_G3X_VTXRAM_COUNT`
  - `src/ov168/btlv_effect.c`: `reg_G3X_RAM_COUNT`
- **Display timing (VCOUNT/DISPSTAT):**
  - `src/ov012/building_enter_effect.c`: `reg_GX_DISPSTAT`
  - `src/ov168/btlv_bg.c`: `reg_GX_VCOUNT`
  - `src/ov190/wifilogin_disp.c`: `VCOUNT`, `DISPSTAT`, `BG1OFS`, `DB_BG2OFS`
- **Capture, VRAM and BG2:**
  - `src/ov036/field_transition.c`: `reg_GX_DISPCAPCNT = 0x80330010`, `BG2CNT`, `BLDALPHA`, raw 0x06800000 (LCDC VRAM)
  - `src/ov036/fld3d_ci.c`: `DISPCAPCNT`, `BG2CNT`, raw 0x06860000
  - `src/ov035/event_mapchange.c`: raw 0x06800000
- **Windows and blending (overlays 168, 70, 171, 192–199, 257, 264, 280, 294, 295, 332):**
  - `src/ov168/btlv_effvm.c`: `WIN0/1H/V`, `BLDALPHA`, `VCOUNT`
  - `src/ov036/shortcut_menu.c`: raw `BLDCNT`
  - the rest, mostly `BLDCNT`/`BLDALPHA` read-modify-writes: `ov070/oekaki.c`, `ov171/vs_multi_list.c`, `ov192`/`ov193` trade demos, `ov197/mystery*.c`, `ov199/gsync_state.c`, `ov257/ctvt_game.c` and `ctvt_talk.c`, `ov264/cdemo_*.c`, `ov280/namein.c`, `ov294/intro_cmd.c`, `ov295/*.c`, `ov332/cygnus_flow.c`
- **Palette RAM:**
  - `src/ov200/cg_help.c`: raw 0x050005E2
- **3D command ports:**
  - `src/ov194/pokemontrade_3d.c`: `reg_G3_NORMAL`, `TEXCOORD`
- **Card SPI (IR):**
  - `src/ov338/ir_check.c`: `reg_MI_MCCNT0`, `MCD0`, with tick busy-waits

The binary scan, which includes asm, finds **894 game functions in 360 files** with hardware constants (759 of them in
complete files). By function count: 2D-A 420, 2D-B 375, IO base 105, VRAM 59, G3 FIFO/command 53, shared memory 44, PLTT
42, POWCNT 36, OAM 27, DTCM 18. Most are the `BLDCNT` and `MASTER_BRIGHT` addresses passed to `G2x_*_`/`GXx_*_`.

---

## 3. Where the platform boundary should sit

### Facts that constrain every option

1. **The game addresses display memory directly.**
   - It writes VRAM through pointers from `G2_GetBG*CharPtr`/`ScrPtr`, `HW_BG_VRAM`, `HW_LCDC_VRAM_*` and raw 0x068xxxxx.
   - It writes palettes and OAM through `HW_*_PLTT`/`HW_OAM`, and does read-modify-writes on 2D registers.
   - It maps VRAM banks itself (`GX_SetBankFor*`, 22× `GX_SetBankForLCDC`).
   - It uses display capture (`GX_SetCapture` in 7 files, plus raw `DISPCAPCNT`), dual-screen 3D (`gfl/double3Ddisp.c`: capture into VRAM C/D, shown as a direct-color bitmap BG or bitmap OBJ), and HBlank raster effects (`GFL_HBlankTCBAdd` in 10 places; the handlers read `VCOUNT`).

   So **any** option needs host VRAM, PLTT and OAM buffers with the DS's bank mapping, plus a DS 2D-engine renderer for both engines. That renderer must handle text, affine, extended and bitmap BGs, normal, affine and bitmap OBJ, windows, blending, mosaic, master brightness and capture. Nothing above this level removes that.
2. **3D is a command stream.** NNS G3D, SPL, MCSS and the field code all end in G3 geometry-engine commands. The game also *reads back* GE state: clip and vector matrices (`G3X_GetClipMtx`/`GetVectorMtx`), box tests (`NNS_G3dGeBufferOP_N` box test → `G3X_GetBoxTestResult`, used in `gfl/g3d_system.c` and `ov012/field_g3d_map.c`), and RAM counts. A host geometry engine is therefore required in every option.
3. **ARM7 services have no source.** Sound sequencing and mixing, touch, RTC, power, mic, NVRAM, backup and wireless must be written for the host at the ARM9 API or at the PXI protocol.
4. **188 unique `asm` functions in `lib/*/src` need C or host replacements** in any option: NitroSDK 95, MSL 51, DS Protect 31, DWC 6, TwlSDK 5. `src/` has none.

### Option (i): API-level replacement of NitroSDK (libntr-style), NitroSystem, SPL and GFL compiled natively

- **Raw surface:** 602 NitroSDK and TwlSDK functions are referenced from outside the SDK (by the game, NNS, SPL, DWC, NitroWiFi and others). 492 of them are referenced by non-network code. Add the 125 header inlines (79 hardware-touching).
- **What changes for pokebw2:** unlike Platinum, the SDK is now C. The class-A SDK modules can stay native: FS core, FX/MTX with the asm rewritten, MATH, STD, RC4, MI memory and LZ in C, ARM9 SND, OS mutex/message/event, RTC convert, and SSP. That is 341 SDK functions kept. With those kept, **the HAL shrinks to 356 B/B-lite entry points**:

  | Module | Entry points |
  | --- | --- |
  | gx | 139 |
  | os | 64 |
  | wm | 48 (stub) |
  | spi (TP/PM/MIC) | 37 |
  | fx | 30 (software divider) |
  | card | 12 |
  | snd/command | 8 |
  | mi | 5 |
  | rtc | 5 |
  | fs/rom | 4 |
  | nvram | 4 |

  Drop WM and the divider work, and about 280 real implementations remain.
- **GX in this option** amounts to a register file. The 139 gx entry points and 79 inlines are mostly setters of 2D/3D state and VRAM-bank routing, which feed the renderer. Implementing them is mechanical once the display model exists. Retarget the `reg_*` macros to a host struct, or keep the functions and give them host bodies.

### Option (ii): keep almost all SDK C and implement a host "hardware model"

- **Keeps:** all 166 gx functions and the 86 inlines unchanged, plus card ROM, DMA, the divider and PXI.
- **What the model has to implement:**
  - **Registers:** about 150–171 distinct I/O registers (171 register macros used in C, 150 distinct addresses; the binary literals show 146 addresses). They fall into these blocks:

    | Block | Registers |
    | --- | --- |
    | 2D-A | 26 |
    | 2D-B | 23 |
    | 3D command ports | 27 |
    | 3D control | 7 |
    | 3D status | 6 |
    | VRAMCNT/WRAMCNT | 10 |
    | DIV/SQRT | 10 |
    | Card | 7 |
    | TWL SCFG | 7 |
    | TWL DSP | 8 |
    | Timers | 4 |
    | IPC | 4 |
    | IRQ | 3 |
    | DMA | 4 channels |
    | NDMA | 4 channels |
    | KEY, POWCNT, EXMEMCNT | 1 each |

  - **Memory regions:** VRAM banks A–I and LCDC, palette RAM, OAM, extended palettes, texture slots.
  - **Engines:** the geometry engine (FIFO and command ports), DMA, timers, IME/IE/IF and VCOUNT.
  - **ARM7 high-level emulation (HLE):** about 16 PXI tags.
- **Where the line is:**
  - The code accesses absolute addresses (`*(vu16 *)0x04000000`), so the macros must be retargeted or hardware memory mapped at those addresses. Mapping below 4 GB on 64-bit macOS fights `__PAGEZERO`.
  - Side-effecting registers need trapping: G3 ports, GXFIFO, DMA control, IF write-1-clear, divider/sqrt busy bits, IPC FIFO, timer reloads. That is an emulator without the CPU.
  - It still does not avoid rewriting the OS core: context switch, CPSR masking, CP15 cache and MPU, and the asm memory helpers are ARM-only.
- **Verdict:** it buys exactness for the 2D/3D register semantics, but costs a device-emulation layer for blocks with no gameplay value, such as card ROM protocol, PXI framing, DMA and WRAM control.

### Option (iii): replace GFL graphics wrappers with modern renderer calls

- **GFL's surface:** GFL exports 748 functions to the rest of the game, used at 40,468 sites. The graphics files account for roughly 405 of them:

  | File | Functions |
  | --- | --- |
  | `clact.c` | 91 |
  | `bg_sys.c` | 50 |
  | `g3d_system.c` | 53 |
  | `particle.c` | 48 |
  | `billboard.c` + `billboard_act.c` | 38 + 25 |
  | `bmp_win.c` + `bmp.c` | 23 + 16 |
  | `g3d_camera.c` | 20 |
  | `gx_layers.c` | 13 |
  | `g3d_util.c` | 11 |
  | `fade.c` | 6 |
  | `g3d_light.c` | 5 |
  | `double3Ddisp.c` | 5 |
- **The bypass is pervasive:**
  - `src/system` and the overlays call 136 graphics SDK/NNS functions directly, at 1,177 sites from 577 functions in 252 files. By family: GX 320, G2x 303, NNS_G2d 107, NNS_G3d 93+30, NNS_Gfd 81, G3X 77, GXS 71, DC 64, G3 30.
  - Hardware inlines appear at 1,569 sites, and 894 functions in 360 files carry hardware constants.
  - The battle sprite renderer (MCSS), the field renderer and every demo issue raw G3 commands.
- **Verdict:** a GFL-level renderer cannot replace the (i)/(ii) layer. It can only *add* semantic hooks on top of it, for HD/widescreen work:
  - widen the projection in `g3d_camera`/`g3d_system`;
  - extend BG/OBJ layers in `bg_sys`/`clact`;
  - upscale 3D rendering.

  Done game-wide, it means editing on the order of 250–360 game files.

### The 3D path in detail, with the narrowest entry points

There are three producers:

- **(a) Immediate commands:** `reg_G3_*` port writes (0x04000440–0x040005CC) from the `G3_*` inlines and from `g3imm.c`/`g3_util.c`. Writers are the game (33 files), SPL, NNS G2D surface rendering, and NNS G3D (`MTX_MODE`/`PUSH`/`POP`/`STORE`/`RESTORE`/`IDENTITY`).
- **(b) Packed FIFO words** to `reg_G3X_GXFIFO` (0x04000400):
  - `GX_SendFifo48B/64B` (asm, from `G3_LoadMtx43`/`MultMtx43`/`LoadMtx44`/`MultMtx44`);
  - `MI_SendGXCommand(Async)(Fast)` and the TWL `MI_SendNDmaGXCommandAsync(Fast)`. These are the DMA path that `NNS_G3dGeSendDL` uses, and it is reached from `NNS_G3dGeFlushBuffer`/`NNS_G3dGeBufferOP_N` and from the model renderer `sbc.c`;
  - `G3X_ClearFifo`/`GXi_NopClearFifo128_`.
- **(c) Display lists inside NSBMD resources**, sent whole through (b).

The narrowest host interface, under any option:

1. `ge_port_write(cmd, u32)` for the `reg_G3_*` macros.
2. `ge_fifo_write(const u32 *words, n)`, an unpacker of packed command bytes, used by `MI_SendGXCommand*`, `MI_SendNDmaGXCommand*` and `GX_SendFifo*`.
3. Readbacks: `GXSTAT` (box test result, matrix stack levels, busy), `RAM_COUNT`/`VTXRAM_COUNT`, `CLIPMTX_RESULT`, `VECMTX_RESULT`.
4. 3D control state: `DISP3DCNT`, clear color, depth and image, fog color, offset and table, edge table, toon table, 1-dot depth, alpha test, and `SWAP_BUFFERS` parameters.
5. Texture and texture-palette memory: whichever VRAM banks `GX_SetBankForTex`/`TexPltt` route there, filled through `GX_LoadTex`/`TexPltt` and NNS GFD.

Behind these sits a faithful DS geometry engine (matrix stacks, about 37 commands, lighting, clipping, box and position
tests) that feeds a GPU rasterizer. That renderer must also cover DS quirks: polygon IDs and edge marking, fog,
translucency sorting, w-buffering, toon, and capture of 2D+3D into VRAM.

### Recommendation

Use a **hybrid of (i) and (ii), split by subsystem**:

- **Keep native C:** all class-A SDK modules, NitroSystem, SPL, GFL and the game.
- **Display: a thin hardware model.** Retarget `reg_*`, `HW_*` and the few raw casts in game code to a host register file. Plain storage is enough for 2D state. Use accessor functions only for side-effecting registers: G3 ports, GXFIFO, `GXSTAT`/`VCOUNT`/`DISPSTAT` reads, IF/IE/IME, divider and DMA control. This keeps all 166 gx functions, the 79 inlines and the 1,569 game inline sites untouched. Add the 2D renderer, the geometry engine and capture. This is effectively what libntr's display side must do as well.
- **Everything else: replace at the API level.** That covers OS threads, IRQs, alarms, ticks and cache (a host scheduler), FS ROM through the single `FSi_ReadRom` hook, the CARD backup API, TP, RTC, PM and MIC, MI DMA as memcpy, and the soft divider. This avoids emulating the card protocol, PXI framing and DMA engines.
- **Sound:** keep NNS_Snd and the ARM9 SND (C) and write a host "ARM7 sound driver" that consumes the `SND_FlushCommand` packets and maintains `SNDSharedWork`. That driver is an SSEQ/SBNK/SWAR sequencer and mixer. It preserves exact music behaviour, and NNS's 80 `SND_*` dependencies stay as they are.
- **Stub:** WM/MB/NWM, DWC networking at first, DS Protect, TwlSDK (with `OS_IsRunOnTwl() = FALSE`), CTRDG and IR.
- **Later:** add GFL-level hooks (iii) for widescreen and HD.

---

## 4. OS, threading and interrupts

### The main loop (`src/system/main.c:44`)

1. `SystemInit()` (`machine_use.c`) calls `OS_Init`, `OS_InitTick`, `OS_InitAlarm`, `RTC_Init`, `GX_Init`, then `reg_SCFG_EXT` on dev consoles, `FS_Init(1)`, `FS_LoadTable` into `OS_AllocFromArenaLo`, PM exit callbacks and `PM_Get/SetAmp`.
2. `GFLAppInit()` (`gfl_use.c:107`) sets up the OS heap on NTR, `GFL_MemInit` (NTR vs TWL layout), the DTCM heap, `GFL_ArcSysInit`, `GFL_HIDInit` (TP and keys), and the rest.
3. `OS_SetIrqFunction(OS_IE_H_BLANK, GFLAppIRQHBlank)` and `OS_SetIrqFunction(OS_IE_V_BLANK, GFLAppIRQVBlank)`, then `OS_EnableIrqMask`, `OS_EnableIrq`, `GX_HBlankIntr(TRUE)`, `GX_VBlankIntr(TRUE)`, `OS_EnableInterrupts`.
4. `GameInit()`: `GFL_OvlLoad(12)`, `GFL_OvlLoad(27)`, the text RNG, queuing `EVENT_TITLE_FUNCS`, `SaveControl_Create` (backup), net heap and errors, `PML_*` init, `GFL_SndInit`, `PokeVoice_Init`, the beacon, PassPower.
5. Each frame:
   1. `func_02005430` (PM exit and soft-reset check), `GFLAppUpdate` (HID, proc manager), `GameUpdate` (brightness, wipe, sound, voice, beacon), `GFL_G3DSysCheckSwapBuffers`.
   2. Optionally `MI_SetMainMemoryPriority(ARM9)`, then `OS_SleepThreadDirect(&g_GFLSndThread)` and **`OS_WaitIrq(TRUE, OS_IE_V_BLANK)`**.
   3. `GameUpdateVBlank`: `SystemUpdateVBlank`, then `GFL_VBlankUpdate`, which runs the VBlank TCBs, `GFL_BGSysUpdate`, VRAM-transfer flushes and `GFL_FadeUpdate`, and counts frames.
   4. Wake the sound thread and give memory priority back to the ARM7.

### IRQ context

- **VBlank** (`GFLAppIRQVBlank`): `GFL_VBlankExecCallback`, `OS_SetIrqCheckFlag(VBLANK)` (a DTCM write) and `MI_WaitDma(3)`.
- **HBlank:** `GFL_HBlankTCBUpdate`. There are 10 users: `system/wipe.c`, `game_beacon.c`, `ov168/btlv_bg.c` (wave), `ov185/pmsiv_edit.c`, `ov190/wifilogin_disp.c`, `ov199/gsync_disp.c`, `ov299`/`ov300`/`ov304` Pokédex lists, and `ov311/un_select.c`.
- The handlers read `VCOUNT` to pick the line. `wipe_sub.c`, `mcss.c`, `field_g3d_mapper.c` and `zknsearch_main.c` sample `GX_GetVCount` and `GX_IsHBlank` as well.
- A host must either render per scanline and run the HBlank TCBs between lines with `VCOUNT` set, or replace these effects.

### Other IRQ sources

| Source | Used by |
| --- | --- |
| Timer 0 | OS tick |
| Timer 1 | OS alarms |
| VCount | OS VAlarm (only `CARD_LockRom`) |
| DMA end | `MI_SendGXCommandAsync` callback, card ROM DMA |
| PXI receive | every ARM7 service |
| Card data | checked in `snd_sys.c` |
| GXFIFO condition | G3X |
| Camera, DSP | TwlSDK |

### Threads and synchronisation

`OS_CreateThread` has 19 call sites:

| Owner | Threads |
| --- | --- |
| Game | 1: `StartLoadThread` in `system/snd_sys.c`, a sound-data loading thread whose procedure depends on the mode, with `SND_THREAD_PRIORITY` |
| NNS | the sound stream thread (`snd_arc_stream.c`) |
| SDK | `CARD_Init` task thread, OS idle thread, MB task |
| Network | CPS ×3, SOC command pipe, WCM/DWC ×3, NHTTP, ND, POKE_NET ×2, dwcutil AC |
| TwlSDK | camera, DSP pipe |

- **Scheduling:** DS semantics are strict-priority and single-core. Switches happen at blocking calls and on IRQ return (the ITCM handler calls `CP_SaveContext`). On the host, a fiber/coroutine scheduler that runs IRQ handlers at defined points (inside `OS_WaitIrq`/`OS_WaitVBlankIntr`/`OS_WaitInterrupt`, and after a simulated DMA or PXI completion) and reschedules afterwards reproduces this. Host OS threads with a global lock are the alternative.
- **Alarms** (`OS_SetAlarm`/`CreateAlarm`): libraries only (NVRAM, `OS_Sleep`, CPS, WCM, AOSS/dwcutil, DSP audio).
- **Mutex, message and event:** libraries only. `OS_Sleep` has 48 call sites, including `system/snd_sys.c` and `pm_voice.c`.
- **Blocking waits outside the main loop:**
  - `OS_WaitVBlankIntr`: `system/screentex.c`, `ov036/field_transition.c`, `ov186/trainer_card.c`, `ov182` (DWC utility), `gt2`.
  - `OS_WaitInterrupt`: `ov012/fldmmdl.c` `FldActSys_ForceFullSync`, `ov194/pokemontrade_3d.c`.
  - `OS_WaitIrq`: `net_err.c` ×3, `GFL_HIDDoSoftReset`, `battle_examination.c`, the soft-reset callbacks in overlay 181, `ov184/error_window_screen.c`.
  - Busy-waits on `OS_GetTick`: `ov338/ir_check.c`.

### Cache, ticks and arenas

- **Cache:** the game makes 80 `DC_*` calls, all no-ops on the host. The libraries make many more (WM 47 by itself).
- **Ticks:** `OS_GetTick` has 198 call sites, 38 of them in game code (`playtime_ctrl.c`, `printsys.c`, …). `OS_GetVBlankCount` is used as a seed or timer in 37 places, through `HW_VBLANK_COUNT_BUF`. The host supplies a 33.514 MHz / 64 tick and a frame counter.
- **Arenas:** `OS_GetArenaLo/Hi(MAIN)`, the DTCM arena (GFL DTCM heap), and the 16 MB DSi layout when `OS_IsRunOnTwl()` is TRUE.

---

## 5. asm, interworking, TCM, SWI and decompression

### `asm`

- There are **188 unique `asm` functions**, all in `lib/*/src`. **None are in `src/`.**

  | Library | Functions |
  | --- | --- |
  | NitroSDK | 95 |
  | MSL `fp_fastI.c` (soft-float and division runtime) | 51 |
  | DS Protect | 31 |
  | DWC `crypt.c` (big-number) | 6 |
  | TwlSDK | 5 |

- NitroSDK's 95 by file:

  | File(s) | Count |
  | --- | --- |
  | `mi_memory.c` | 18 |
  | `os_protectionRegion.c` | 16 |
  | FX matrices | 15 |
  | `os_system.c` | 7 |
  | `os_cache.c` | 7 |
  | `os_context.c` | 3 |
  | `os_spinLock.c` | 4 |
  | `os_exception.c` | 3 |
  | `math` | 4 |
  | `gxasm.c` + `g3x.c` | 3 |
  | `cp_context.c` | 2 |
  | `os_protectionUnit.c` | 2 |
  | `os_alarm.c` | 1 |
  | `os_init.c` | 1 |
  | `os_tcm.c` | 1 |
  | `os_terminate_proc.c` | 1 |
  | `os_vramExclusive.c` | 1 |
  | `mi_swap.c`, `mi_uncompress.c` | 1 each |
  | `rc4` | 1 |
  | `RtcWaitBusy` | 1 |
  | `MicWaitBusy` | 1 |

- TwlSDK's 5 are 4 SWI wrappers and `SCFGi_SwitchCpuSpeed`.
- There are 3 inline `asm {}` blocks: the DS Protect cache flush (`dsprot_internal.h:18`), the CLZ in `nitro/math.h:102`, and `os_exception.c:154`.
- The crt0 and the 18 SVC stubs at 0x02004000 are still assembly.

### Interworking

The code is built with `-thumb -interworking` (the game is Thumb, much of the SDK ARM). Natively, ARM/Thumb distinctions
disappear: bit 0 of function pointers, `bx` veneers, the MSL soft-float calls the compiler emits.

The ABI assumptions that matter to a host build:

- `-char signed`: pass `-fsigned-char` on AArch64 Linux.
- `-enum int`.
- Soft-float IEEE: host FPU results should match for the basic operations.
- 32-bit pointers in data: the NNS unpackers mentioned in section 1.
- Little-endian structure layouts.

### TCM and special placement

ITCM holds the GFL CLACT renderers ×4, `OS_IrqHandler` and the thread switch, `MIi_DmaSetParameters`, the reset code and
`SCFGi_SwitchCpuSpeed`. DTCM holds the IRQ table, stacks, the IRQ check flag and the GFL DTCM heap. Overlay 169 executes
from VRAM, and overlays 218–225 live in DSi RAM.

On the host all of this becomes normal memory, except for two things. `HW_INTR_CHECK_BUF` and the other `HW_*`
shared-memory addresses must resolve to host storage: `HW_BUTTON_XY_BUF` 0x02FFFFA8, `HW_VBLANK_COUNT_BUF` 0x02FFFC3C,
`HW_NVRAM_USER_INFO` 0x02FFFC80, `SCFG_EXT_COPY` 0x02FFFDF0, and the ROM header buffers. And overlay load and unload
must re-initialise each overlay's `.data`, `.bss` and `.sinit`, because natively all overlays are resident at once.

### BIOS SWI

The game calls none directly. The library users are:

| Caller | SWI |
| --- | --- |
| OS spin locks, `OS_WaitVBlankIntr`, `PM_Init`, PXI senders (12 sites) | `SVC_WaitByLoop` |
| `OS_En/DecodeDeliverArg`, NWM | `SVC_GetCRC16` |
| CTRDG | `SVC_CpuSet` |
| FS overlay digest | `SVC_CalcHMACSHA1` |
| TWL `.ltdmain` | `SVC_SHA1*` |
| `OSi_DoResetSystem` | DSi BIOS SWIs 0x24–0x28 |

`Div`, `Sqrt`, `CpuFastSet` and the LZ77/RL/Huffman/BitUnPack SVC stubs exist but have no callers.

### Hardware math

`FX_Div` (175 game sites), `FX_Inv`, `FX_Sqrt` and `VEC_Normalize`/`VEC_Mag` use the DIV/SQRT registers. So do DWC VCT
and `pm.c`. All need software equivalents that give the same rounding.

### Asset decompression

The only decompressor the game relies on is **`MI_UncompressLZ8`** (LZ77 types 0x10 and 0x11; an asm function to
rewrite in C). It is called from `GFL_ArcSysReadHeapNewLZ(GetLen)`, `GFL_ArcToolReadHeapNewLZGetLen`,
`GFL_BGSysTransferUncomp` and `clact.c`, plus one site in overlay 182. `MIi_UncompressBackward` (boot and
`FS_StartOverlay`) handles compressed code images only, so it is irrelevant natively.

---

## 6. HAL surface table

Difficulty: S = days, M = weeks, L = months.

| Family | Functions to implement on the host | Difficulty | Notes |
| --- | --- | --- | --- |
| Host startup / crt0 / autoload | `main` wrapper; arena setup (`OS_GetArenaLo/Hi`, DTCM arena) | S | Replaces `Entry`, `boot_*`, the autoloads and the SVC stubs (34 functions, class C) |
| OS threads | `OS_CreateThread`/`DestroyThread`/`Sleep`/`Wakeup(Direct)`/`Join`/`Yield`/`SetThreadPriority`/`IsThreadTerminated`, `Disable`/`EnableScheduler`, context switch (16 used + core) | M | Fiber scheduler with DS priority semantics. The mutex, message and event code stays as C. |
| OS IRQ | `OS_SetIrqFunction`, `Enable`/`DisableIrqMask`, `OS_EnableIrq`, `OS_WaitIrq`/`WaitVBlankIntr`/`WaitInterrupt`, `OS_Disable`/`Restore`/`EnableInterrupts`, `OS_SetIrqCheckFlag` storage (≈15) | M | Frame clock produces VBlank and per-line HBlank. Handlers run inside the waits. |
| OS time | `OS_InitTick`/`GetTick`/`GetTickLo`, alarms (5), VAlarm, `OS_GetVBlankCount` storage | S | Host monotonic clock |
| OS misc | cache (`DC_*`/`IC_*`), `OS_Halt`, `OS_Terminate`/`ResetSystem` (soft reset re-runs `main` or exits), owner info, MAC, EULA/parental (≈20), `OS_IsRunOnTwl` = FALSE, `OS_GetConsoleType` | S | Owner info comes from a config file |
| MI | `MI_CpuFill`/`Copy`/`Move`/`Clear` (C rewrites of 20 asm functions), DMA as memcpy (`MI_DmaCopy`/`Fill*`, `MI_WaitDma`, `MI_StopNDma`), `MI_SetMainMemoryPriority` no-op, WRAM no-ops, `MI_UncompressLZ8` in C | S | |
| FX/MTX/VEC | software DIV/SQRT for `fx_cp.c`/`fx_vec.c`/`fx_mtx44.c` (30), and C for 15 matrix asm functions; `CP_Save/RestoreContext` no-ops | S | Bit-exact fixed-point rounding |
| FS | `FSi_ReadRom` backend (NitroFS from the .nds or an extracted tree), `FS_LoadOverlay`/`UnloadOverlay`/`StartOverlay` → re-initialise `.data`/`.bss`/`.sinit` | S–M | The FS archive core (117 functions, all C) stays |
| CARD | backup: `CARD_Lock/UnlockBackup`, `IdentifyBackup`, `RequestStreamCommand`, `WaitBackupAsync`, `TryWait`, `Cancel`, `GetResultCode` (≈12); `CARD_GetRomHeader`, pulled-out callback no-op | S | File-backed flash image |
| Input | `reg_PAD_KEYINPUT`, `HW_BUTTON_XY_BUF` (X, Y, lid) storage; `TP_*` (12) | S | Mouse or touch to touch panel, with calibration passthrough |
| RTC / PM / MIC | `RTC_GetDate`/`Time`/`DateTime(Async)` (5); `PM_*` (18: amp, backlight, battery, sleep, exit callbacks); `MIC_*` (7) | S | Host clock; PM stubs; MIC via host audio input or silence |
| **Display model: 2D** | register file for `DISPCNT`/BG/WIN/BLD/MOSAIC/`MASTER_BRIGHT`/`DISPCAPCNT` ×2 engines (≈50 registers), VRAMCNT bank mapper (A–I, LCDC, BG/OBJ/ext-palette/texture slots), PLTT, OAM; the 139 gx functions and 79 inlines stay as C against it | M | Retarget `reg_*`, `HW_*` and 7 raw-cast sites |
| **2D renderer** | engine A and B rasterizer: text, affine, extended and bitmap BGs; normal, affine and bitmap OBJ; windows; blending; mosaic; brightness; capture; per-scanline HBlank hooks | **L** | Needed by every option |
| **3D geometry engine** | `ge_port_write`, `ge_fifo_write` (feeds `MI_SendGXCommand*`, `GX_SendFifo*`), readbacks (`GXSTAT`, `RAM_COUNT`, clip and vector matrices, box test), about 37 commands | **L** | Faithful matrix stacks, lighting, clipping |
| **3D rasterizer** | polygons → GPU with DS semantics (textures from VRAM slots, fog, edge, toon, alpha, polygon IDs, sorting, w-buffer), capture to VRAM, dual-screen 3D | **L** | HD/widescreen can come later through GFL hooks |
| **Sound driver (ARM7 replacement)** | consume `SND_FlushCommand` packets (`START`/`PREPARE`/`STOP` sequence, channel PCM/PSG/noise, volume, pan, tempo, mute, alarms), update `SNDSharedWork`; SSEQ interpreter, SBNK/SWAR instruments, ADPCM, mixer, capture (the reverb in NNS `snd_capture`) | **L** | NNS_Snd (193) and ARM9 SND (73) stay as C |
| Wireless / DS Download Play / IR | WM (47), MB (12), NWM, irc | C → stub (or later netplay at the `GFL_Net*` level) | |
| Network (WFC/GTS/GDS) | SOC/CPS → host sockets, or stub DWC | M (or stub) | Servers are gone |
| TwlSDK features | camera, DSP, SSP, photo sign, FATFS, SNDEX, MICEX, SCFG, NDMA | C → stub (NTR mode) | |
| DS Protect | 7 functions called by the game → return "OK" | S | |
| MSL runtime | delete `fp_fastI.c`; use the host libc for the 95 MSL functions or keep them | S | |

### Minimal first milestone: boot to the first frame (overlay 162 `BOOT_SCREENS_PROC`)

The boot path in `main.c`/`machine_use.c`/`gfl_use.c` and the first proc, `src/ov162/boot_screens.c`, need:

1. **A native build of the game, GFL, NitroSystem, SPL and the A-class SDK.** Run with `OS_IsRunOnTwl()` = FALSE (the NTR heap layout). Rewrite the 20 MI and 15 FX asm functions in C. Drop MSL `fp_fastI`. Stub DS Protect and the TwlSDK.
2. **Host startup and memory:** main arena and DTCM arena bounds, and storage for the `HW_*` shared-memory words (VBlank counter, XY buttons and lid, IRQ check flag, owner info, ROM header).
3. **OS:**
   - `OS_Init`, `OS_InitTick`, `OS_InitAlarm` and `RTC_Init` (stubs or host clock);
   - `PM_*` stubs (amp, exit callbacks, auto-exit);
   - IRQ registration plus **a 60 Hz VBlank delivered inside `OS_WaitIrq`** (HBlank can wait);
   - a minimal cooperative thread layer, enough for `g_GFLSndThread` and the sleep/wake pairs in the main loop;
   - interrupt enable/disable as no-ops or a lock, and cache no-ops.
4. **FS and overlays:** `FS_Init`, `FS_LoadTable`, the `FSi_ReadRom` backend reading NitroFS (NARCs through `GFL_ArcSys*`, `MI_UncompressLZ8`), and `FS_LoadOverlay` as re-initialisation of overlays 12, 27 and 162.
5. **Save:** CARD backup on a file, for `SaveControl_Create(7)`. An absent file must take the "no save" path.
6. **Input:** keys from `reg_PAD_KEYINPUT` and `HW_BUTTON_XY_BUF`; `TP_Init`, `TP_GetUserInfo` and `TP_SetCalibrateParam` returning "no touch".
7. **Sound:** `SND_Init`/`NNS_Snd*` running against a **null ARM7 driver** that acknowledges command packets and reports idle players. `GFL_SndInit` and `PokeVoice_Init` must not stall.
8. **Display:**
   - `GX_Init`, `GX_SetBankFor*`, `GX_SetGraphicsMode`, `GX_DispOn`;
   - text-mode BG rendering for both engines (`GFL_BGSysCreateBG`, NCGR/NCLR/NSCR loads into VRAM and palettes);
   - `GXx_SetMasterBrightness_` fades (`GFL_FadeSet`);
   - `G2(S)_BlendNone` and `GX(S)_SetVisibleWnd` as register-file writes;
   - presenting two 256×192 framebuffers at VBlank.

   3D, OBJ, capture, HBlank, wireless and the mic are not needed for this milestone. The next targets are `CDEMO` in overlay 264 (the opening; its `cdemo_*.c` files write `BLDCNT` directly, and it needs the other demo graphics) and the title in overlay 162, which uses 3D and `GX_SetCapture` in `title.c:492`.
