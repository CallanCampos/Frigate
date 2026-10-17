# BW2 engine, boot path and port milestones (pokebw2 decomp, Black 2 US)

Scope: what a native PC/Mac port built from pokebw2 needs from the game's own engine, the boot-to-gameplay path, the
code still missing per milestone, the overlay semantics to reproduce, the engine's lineage against Platinum, and the
dual-screen usage. All figures are for `b2_us` at commit `0c2bbee9d` (worktree
`desktop-subagent-inspection-3190ba`), computed by scripts in `research/work/engine/` (`coverage.py`, `overlays.py`,
`milestones.py`, `callers.py`, `datacov.py`) from `config/b2_us/**/delinks.txt` and `symbols.txt`; per-function data in
`work/engine/funcs.tsv`, per-file in `files.tsv`, per-module in `modules.tsv`, overlays in `overlays.tsv`.

## 0. Coverage in one paragraph

43,402 ARM9 functions (3.87 MB of code). Classification used throughout:

- **complete**: in a `delinks.txt` file marked `complete` (C links and matches): 32,571 functions, 2.76 MB.
- **C, incomplete file**: the function has a C definition in a file that isn't complete yet (the build still links
  the original asm; most such functions match individually, 444 functions are listed as nonmatching in
  `docs/nonmatching-functions.md`, almost all register allocation/scheduling): 6,328 functions, 745 KB.
- **no C**: in a registered file whose `.c` doesn't exist or lacks it, or outside every file: 4,503 functions,
  360 KB, of which 498 are linker ARM/Thumb veneers (ov167 calls into ov169, see 3.4).

So 89.6% of functions (90.7% of bytes) have C. Split by owner:

| owner | complete | C, incomplete file | no C |
| --- | --- | --- | --- |
| game code (`src/`) | 30,080 / 2.48 MB | 4,650 / 511 KB | **1,568 / 113 KB** |
| libraries (`lib/`) | 2,491 / 283 KB | 1,678 / 234 KB | 111 / 15 KB |
| outside any file | | | 2,326 / 230 KB (ov11 972, ov182 845, LTD 461, main 35, ITCM 13) |

**All game code without C is in overlay 36 (the field), 1,563 functions / 112.6 KB: 35 registered files
with no `.c` yet plus 15 functions of `shortcut_menu.c`; and 5 functions in ov257 (Xtransceiver camera).** ARM9 main's
only game code without C is the cell actor system's 4 OAM renderers in ITCM (3.0 KB, see 1.3). Everything else without C is library code: DWC/NitroWiFi Wi-Fi (ov11, ov182), TwlSDK
DSi libraries (LTD autoload), DS Protect, the boot stub and BIOS calls. ARM9 main (the engine) is entirely written in
C: 7,019 complete functions, 731 written in 34 incomplete files, 37 without C (crt0, BIOS SVC stubs, two
`os_arena.c` stubs). The ARM7 binary (164 KB, Nintendo's stock sound/wireless/touch/RTC component) is not decompiled
at all (`extract/b2_us/arm7/arm7.bin`, no config).

Data is a second, smaller gap: of 12,671 data/bss symbols in ARM9 main and the overlays, 2,046 sit in incomplete
files without a C definition and 1,406 are outside any file (mostly ov11/ov182). In the field, ~840 belong to the
no-C ov36 files (688 in `fldmmdl_acmd.c` alone) and ~95 to its written files; main has ~50 (`wipe_sub.c` 15,
NitroSystem `g2d_OamManager.c` 11, `arc_tool.c` 4).

## 1. Engine map: entry, main loop and subsystems

### 1.1 Entry and main loop

- Boot: crt0 `Entry` (0x02004800, ARM, no C) and NitroSDK init, then `main()` in `src/system/main.c` (complete;
  the file name is a guess). The entry symbol is `main`; there is no `NitroMain` symbol.
- `main()` order: `SystemInit()` (SDK init) -> `GFLAppInit()` (`src/system/gfl_use.c`) -> HBlank/VBlank IRQ handlers
  -> `GameInit()` -> `while (TRUE)`.
- `GFLAppInit()` (complete): on DS, carves an OS heap from the main arena for heap 9; `GFL_MemInit` with two root heaps
  (heap 0 "system" 0x6000; heap 1 "user" 0x1a3e00 on DS, 0x48ae00 on DSi) and 193 heap IDs; DTCM heap 0x2000; child
  heap 7 (save data, 0x27000); heap 9 (device alloc, 0x3f00); `GFL_ArcSysInit` with 308 NARC paths ("a/0/0/0" to
  "a/3/0/7", indexed by archive ID); `GFL_HIDInit`; assert handlers; `GFL_OvlManagerInit(0, 20, 4, 4)` (20 main-RAM,
  4 ITCM, 4 DTCM overlay slots); `GCTX_ProcMgrInit(0)`; HBlank TCB manager (4 tasks) and VBlank TCB manager (16);
  `GFL_FadeCreate`; `mainBackupSystemInit(7, ...)` (save backend); `GFL_TextPrintInit`; random init (LC + MT);
  lid callbacks that mute sound.
- `GameInit()`: loads overlays **12 and 27** (kept resident except while Wi-Fi is up), text RNG, download-play config,
  queues `EVENT_TITLE_FUNCS`, `SaveControl_Create(7)` (reads the save), net heap, net error system, species names
  and personal data, `GFL_SndInit`, `PokeVoice_Init`, mono/stereo from the config, key items, `GameBeaconSys_Create`,
  `PassPower_Init`.
- Frame: `func_02005430` (empty hook) -> `GFLAppUpdate()` = `GCTX_HIDUpdate` (input), `func_02011818`,
  **`GCTX_ProcMgrUpdate()` (the whole game)**, `func_02044198` -> `GameUpdate()` = brightness, `GFL_WipeMain`, sound
  update, Pokémon voice, beacons, Pass Powers -> `GFL_G3DSysCheckSwapBuffers` -> main-memory priority to ARM9 ->
  sleep the sound thread -> **`OS_WaitIrq(TRUE, OS_IE_V_BLANK)`** -> `GameUpdateVBlank()` (`SystemUpdateVBlank`,
  `GFL_VBlankUpdate`: VBlank TCBs, `GFL_BGSysUpdate` register/scroll flush, OAM flush `func_0204f864`, fade) -> wake
  the sound thread. The VBlank IRQ itself only runs a single optional callback (`GFL_VBlankSetCallback`) and waits
  for DMA 3.
- Frame rate: the loop is 60 Hz; the field switches input to 30 Hz (`GCTX_HIDChangeFPS(30)` in
  `FieldRoutine_Open`) and the game system skips event processing on odd frames in 30 FPS mode
  (`GameData_Is30FPSMode`/`Is30FPSSkipFrame` in `GSYS_Update`).
- Threads: only the sound system creates one (`g_GFLSndThread`, BGM loading during fades, `snd_sys.c`), plus
  NitroSystem's sound-archive stream thread and NitroSDK's card thread. The main loop explicitly sleeps/wakes the sound
  thread around the VBlank wait.

### 1.2 Processes, tasks, events

- **Procs** (`src/gfl/procsys.c`, complete): `GameProcFunctions {init, main, exit}`, each called once a frame with
  `(proc, &subState, param, work)` until it returns TRUE. A proc carries an overlay ID: `PROC_STATE_LOAD` calls
  `GFL_OvlLoad(id)` before init and `GFL_OvlUnload(id)` after exit returns TRUE. Managers stack procs (queue = push a
  child that runs until it ends, then the parent resumes; replace = swap). The global manager (`GCTX_ProcMgr*`) runs
  the top level; nested managers exist (`CreateGameProcManager`): the game system has one, `NewGame` has one.
- **Game system** (`src/system/game_system.c`, complete): `GAMESYSTEM_PROC_FUNCTIONS` (overlay NONE) owns
  `GameData`, the event pointer, a proc manager, comm, ISS (interactive sound) and the festival; each frame:
  event provider -> events -> comm -> its proc manager (field or apps) -> field sound, ISS, timers.
  `GameSystem_Init` loads DS Protect (ov337) and runs code-checksum checks (see 3.5).
- **Events** (`src/system/game_event.c`, complete): `GameEvent` = callback + state + work, chained
  (`GameEvent_ChainNext`, `GameEvent_Replace`); the field's event checks are `FieldEventProvider_*` in
  `src/ov036/fieldmap_event.c` (no C).
- **TCBs** (`src/gfl/tcb.c`, `tcbl.c`, complete): priority task lists; the HBlank and VBlank managers above, and
  per-module managers (the field makes one of 32).

### 1.3 Subsystem table (ARM9 main)

"C" = all functions written; "complete" = file matches and links.

| subsystem | game files (status) | SDK under it (status) | port note |
| --- | --- | --- | --- |
| heap | `gfl/heapsys.c`, `heap.c`, `heap_dtcm.c` (complete) | NitroSystem FND exp heap (`lib/nnsys/src/fnd`, complete) | fixed budgets per heap ID; 64-bit pointers will overflow some (e.g. heap 7 0x27000, heap 3 0xc000) |
| overlays | `gfl/gf_overlay.c` (complete) | NitroSDK FS overlay (`lib/nitro/src/fs`, complete) | see section 3 |
| procs/tasks/events | `gfl/procsys.c`, `tcb.c`, `tcbl.c`, `system/game_system.c`, `game_event.c` (complete) | | portable as is |
| archives | `gfl/arc_tool.c` (C, incomplete: 1 nonmatching, 4 data), `arc_util.c`, `system/file_util.c` (complete) | FS (complete) | NARC by archive ID -> path table in `gfl_use.c` |
| BG / windows | `gfl/bg_sys.c`, `gx_layers.c`, `bmp.c`, `bmp_win.c`, `fade.c` (complete); `system/wipe.c` (complete), `wipe_sub.c` (C, incomplete, 15 data) | GX/G2 (complete) | register-level 2D engine needed (or emulated) |
| OAM / cell actors | `gfl/clact.c` (complete), `vman.c`, `areaman.c` (complete) | NitroSystem G2D (20/27 files complete; renderer/OAM manager written) | **the 4 ClAct renderers live in ITCM with no C** (`mainRenderer`, `func_01ff8318/8610/8900`, 3.0 KB) |
| 3D | `gfl/g3d_system.c`, `g3d_util.c`, `g3d_camera.c`, `g3d_light.c`, `double3Ddisp.c`, `billboard*.c`, `particle.c` (complete); `system/mcss.c` (C, incomplete), `ica_anime.c` (C, incomplete) | NitroSystem G3D (17/21 complete; `sbc.c`, `nsbca.c`, `nsbta.c`, `maya.c` written), SPL particles (complete) | geometry engine + G3D model formats |
| text | `gfl/msgdata.c` (encrypted msg files), `strbuf.c`, `textprint.c`, `systemfont.c`, `str_sjis.c` (complete); `system/printsys.c` (C, incomplete, 3 nonmatching), `gf_font.c`, `wordset.c` (complete) | | software glyph blitting into BG bitmaps; portable |
| sound | `system/snd_sys.c`, `snd_strm.c`, `snd_bgm_stack.c`, `pm_voice.c`, `snd_mic.c`, `iss_*.c`, `field_sound*.c` (complete) | NitroSystem snd (12/13 complete; `snd_arc_stream.c` written), NitroSDK SND (complete) | **ARM7 sequencer not decompiled**: needs an SSEQ/SBNK/SWAR/STRM player |
| input | `gfl/ui.c` (`GCTX_HID*`, lid, soft reset L+R+Start+Select, replay hook), `key.c`, `touchpanel.c` (complete) | PAD, SPI touch (complete) | trivial to back with host input |
| RTC | `gfl/rtc_cache.c`, `system/rtc.c`, `season.c` (complete) | RTC (complete) | seasons follow the real month |
| save | `gfl/backup_card.c`, `backup_system.c`, `savedata.c`; `system/save_control.c`, `save_table.c`, `save_error.c` + ~60 `*_savedata.c` (complete) | CARD (4/5 complete) | two copies + CRCs + footer, written a block per frame; back with a file |
| network | `gfl/net*.c` (11 files), `wm_icon.c`; `system/net_define.c`, `net_err.c`, `game_comm.c`, `game_beacon*.c`, `wih_dwc.c` (complete) | WM (complete); DWC in main (mostly complete) | Wi-Fi overlays mostly no C; stub for a single-player slice |
| misc | `system/brightness.c`, `palanm.c`, `pokegra.c`, `poke_icon.c`, `pokedex.c` (C, incomplete) | MSL C library (complete but `fp_fastI.c` soft float) | |

ITCM also holds `OS_IrqHandler`, reset and DMA helpers (no C; replaced by the platform layer).

## 2. Boot-to-gameplay flow

Traced from C (all of these steps are complete C except where noted):

1. **Power-on** -> crt0 -> `main()` -> `GameInit()` queues `EVENT_TITLE_FUNCS` (`src/system/boot_proc.c`, overlay
   NONE), a never-ending proc whose tick queues, in a loop:
2. **Boot screens** `BOOT_SCREENS_PROC_FUNCTIONS`, **ov162** `boot_screens.c`: TPC and Nintendo logos, then the
   copyright notice (BG only).
3. **Game Freak logo** and 4. **opening**: `CDEMO_PROC_FUNCTIONS`, **ov264** (`cdemo_main.c`, `cdemo_control.c`,
   `cdemo_script.c`): the logo is a 151-frame movie of LZ-compressed 256x192 16-bit frames (0x18000 bytes each) from
   `ARCID_CDEMO_GFLOGO`, double-buffered into VRAM; then demo scripts.
5. **Title** `TITLE_PROC_FUNCTIONS`, ov162 `title.c`: 3D Kyurem scene under a curve-following camera, 2D layers over
   it, **display capture into VRAM D every frame for a motion blur**, stream music (`snd_strm.c`); A/Start ->
   `START_MENU_PROC_FUNCTIONS`, Up+Select+B -> `DELETE_SAVE_PROC_FUNCTIONS` (ov162 `delete_save.c`), Down+X+Y -> mic
   test (ov253); timeout -> back to step 2 with the logos skipped.
6. **Start menu** ov162 `startmenu.c` (loads ov331 `save_outside.c` for extra-save checks): Continue / New Game /
   Mystery Gift (ov197) / Battle Competition (ov290) / Game Sync settings / DWC utility (ov182) / mic test / Download
   Play (ov181) / Unova Link (ov332). Continue and New Game call `GameStart_*` (ov162 `game_start.c`).
7. **New game** `NEW_GAME_PROC_FUNCTIONS` (ov162): background save task (`save_control_intr.c`); preloads sounds by
   loading ov280 and ov294 just to copy their sound tables; a nested proc manager runs **intro ov294**
   (`intro*.c`: Professor Juniper, MCSS Pokémon, G3D, particles) -> **name entry ov280** (`namein.c`, C but incomplete:
   111 of 148 functions not yet matching/linked) -> intro (player named) -> rival name entry -> intro (done). Exit
   writes player/config/rival into the save and replaces itself with `GAMESYSTEM_PROC_FUNCTIONS` (overlay NONE) with
   `GAME_ENTRYPOINT_OPENING`.
8. **Game system** -> `CallCreateGameEntryPointEvent` (ov12 `event_game_clear.c`) loads **ov35**
   (`event_mapchange.c`, the field glue) -> `EventGameOpening`: clear LCDC VRAM, season banner, **3D demo ov293**
   (`Event3DDemo_Create(.., 5, ..)`, `demo3d*.c`) -> `EventFieldFirst`: Aspertia City spawn
   (`LoadAspertiaCitySpawnInfo`), ov279 `new_game.c` (bag and Dream Radar init), script 0x1d, BGM, **`EventFieldOpen`**
   -> `GSYS_StartFieldmapProc` queues `FIELDMAP_PROC_FUNCS` with **ov36**; `FieldRoutine_Open` loads **ov33** (field
   events: PC, Pokémon Center, fly, dive, fishing, field moves...) or ov34 in the Union Room, switches input to 30 Hz;
   entrance transition, script 0x1e.
9. **First overworld (Aspertia)**: ov12 resident (script VM `script_sys.c`, `scrcmd_*`, `fldmmdl.c` actor core,
   events, map matrix, `field_encount_st.c`), ov27 resident, ov33, ov35, ov36 (field), the subscreen app overlay (the
   C-Gear ov79 `c_gear_sw.c`, or ov80 `no_gear.c` before it is obtained), weather ov74-78, the map's gimmick overlay
   (Aspertia: ov112), script plugin overlays ov50-68 on demand.
10. **Wild battle**: ov36 `field_encount.c` (complete) -> `EncEff` (ov36 `enceff.c`, no C) + encounter effect overlays
   ov146-157 (complete) -> `EventBattleCall` (ov12 `event_battle.c`): BGM, encounter effect, **field close (ov36
   unloaded)**, `GSYS_QueueProcAsEvent(OVERLAY_ID(167), ...)`: **ov167** battle core (`btl_main.c`, server/client;
   C, 51% of bytes complete) which loads **ov168** (battle view `btlv_*`, C, 25% complete), **ov169** (move/ability
   handlers, complete, loaded into VRAM bank H after `GX_SetBankForLCDC(0x80)`), **ov170** trainer AI (complete);
   battle sub-apps ov285-289 (bag, party, scenario, choice; complete); then ov166 `event_battle_return.c`, DS Protect
   ov337 checks, field reopens. Evolution ov284, Pokédex registration ov297 (complete).
11. **Save**: field menu (`field_menu.c`, ov36, C incomplete) -> ov12 `event_save.c`/`report_event.c`, ov87
   `report.c` subscreen, `save_control.c` incremental write; Continue path: `Continue_Exit` loads the location and
   enters `GAME_ENTRYPOINT_FIELD_CONTINUE`.
12. **Menus**: each app is a proc pushed on the game system's manager by ov12 `event_field_proclink.c`; because all
   app overlays share addresses with ov36, the field closes and reopens around every app: bag ov142, party ov165
   (`pokelist.c`, with DS Protect), summary ov207, Pokédex ov296/298-304, PC boxes ov255, trainer card ov186, options
   ov140, town map ov144, UI scene/touch bar ov139.

### 2.1 Coverage of the overlays on the path

| overlay | role | funcs | complete (bytes) | C (bytes) |
| --- | --- | --- | --- | --- |
| main | engine | 7,787 | 83% | 99.6% |
| ov162 | boot screens, title, start menu, new game/continue | 148 | 100% | 100% |
| ov264 | GF logo movie, opening demo | 45 | 100% | 100% |
| ov294 | Professor intro | 132 | 100% | 100% |
| ov280 | name entry | 148 | 10% | 100% |
| ov279 | new-game save setup | 4 | 100% | 100% |
| ov293 | 3D demos (opening, ferris wheel, ships) | 112 | 100% | 100% |
| ov012 | resident field core (script VM, events, actors) | 1,668 | 99% | 100% |
| ov027 | resident (survey, Funfest mission data) | 42 | 100% | 100% |
| ov033 | field events (PC, Center, fly, dive...) | 238 | 83% | 100% |
| ov035 | map change / entry points | 109 | 100% | 100% |
| **ov036** | **field map** | 4,339 | 27% | **63%** |
| ov074-078, ov079/080, ov087, ov112 | weather, C-Gear / no gear, report, Aspertia gimmick | | 100% | 100% |
| ov146-157 | encounter effects | | 100% | 100% |
| ov167 | battle core | 4,223 (+498 veneers) | 51% | 100% |
| ov168 | battle view | 632 | 25% | 100% |
| ov169 | battle handlers (VRAM) | 1,298 | 100% | 100% |
| ov170, ov166, ov285-289, ov284, ov297 | AI, battle return, battle apps, evolution, dex entry | | 100% | 100% |
| ov142, ov207, ov255, ov296-304, ov186, ov140, ov144 | bag, summary, PC, Pokédex, card, options, map | | 100% | 100% |
| ov165 | party list (+ DS Protect) | 267 | 47% | 99.8% |
| ov139 | UI scene, touch bar | 120 | 43% | 100% |
| ov337 | DS Protect (multi) | 57 | 22% | 98% |

### 2.2 Milestone table

"No C" counts functions/bytes that must be decompiled (or otherwise supplied) for the milestone, incremental over the
previous milestones; "C, incomplete" is code already in C but not yet proven (incomplete files, with nonmatching rows
from `docs/nonmatching-functions.md`). The platform layer (section 6) is needed from M1 on and isn't counted.

| milestone | game overlays involved | no C (new) | C, incomplete (new) | main blockers |
| --- | --- | --- | --- | --- |
| **M1** first frame (logos, GF movie) | main, ITCM, ov162, ov264 | 0 game functions (crt0/BIOS/IRQ: 50 funcs, 6.9 KB, replaced by the platform) | main: 731 funcs / 93 KB in 34 files (55 nonmatching rows; game files `mcss.c`, `printsys.c`, `wipe_sub.c`, `pokedex.c`, `ica_anime.c`, `arc_tool.c` = 9 rows) | none in game code; data still in asm in `wipe_sub.c`, `arc_tool.c`, NNS `g2d_OamManager.c`; 2D renderer, NARC FS, save backend (SaveControl reads at boot) |
| **M2** title + start menu | ov162, ov331 | **4 ClAct OAM renderers in ITCM, 3.0 KB** (sprites everywhere from here on) | 0 | 3D (G3D Kyurem), display capture + VRAM display, STRM stream music, ARM7 sound |
| **M3** new game -> Aspertia overworld | ov162, ov294, ov280, ov279, ov293, ov35, ov12, ov27, ov33, ov36, ov80, ov74-78, ov112, ov337 | **456 funcs / 27.2 KB, 10 ov36 files**: `field_msgbg.c` 177/12.5 KB, `field_task_manager.c` 52/4.8 KB, `weather_task.c` 91/4.0 KB, `field_subscreen.c` 72/2.6 KB, `fld_exp_obj.c` 24/1.7 KB, `weather.c` 16/1.0 KB, `field_task.c`, `field_gimmick.c`, `field_menu_event.c`, `field_wfbc.c`; plus stubs for `sodateya.c`, `fes_gimmick.c`, `pleasure_boat.c`, `field_fmission_info.c`, `enceff.c` (called by `fieldmap.c`) | ov36 1,414 funcs / 109 KB (71 nonmatching rows; `fieldmap.c`, `field_rail*.c`, camera, mapper, buildmodel, player, `fldmmdl_blact.c`...), ~95 data symbols of written ov36 files still in asm; ov280 111 funcs (1 row); ov12 `delivery_beacon.c` 22; ov33 `bsubway_scr.c` 39; DS Protect | the field map overlay |
| **M4** walk, NPCs, talk, map transitions | ov36, ov35, ov33, ov12, ov89-137 per map | **880 funcs / 61.1 KB, 19 ov36 files**: `fldmmdl_acmd.c` 430/18.5 KB (actor action commands, + 688 data symbols), `ctalkmsgwin.c` 56/7.9 KB (speech balloons, `ActorMsgWin_*`), `fieldmap_event.c` 55/7.0 KB (`FieldEventProvider_*`: talk/step/warp/encounter checks), `sodateya.c` 65/5.3 KB, `field_fade.c` 43/4.4 KB, `scrcmd_pokemon_misc.c` 27/3.1 KB, `place_name_letter.c` 39/2.8 KB, `scrcmd_field.c` 25/2.4 KB, `field_fmission_info.c` 26/2.3 KB, field effects `fldeff_reflect/iaigiri/ochiba/hide/btrain/ripple/splash.c` 80/5.3 KB, `fes_gimmick.c`, `pleasure_boat.c`, `scrcmd_pokemon_fld.c` | (in M3) | NPC movement and dialogue are the biggest single gap |
| **M5** wild battle | ov36 encounter, ov146-157, ov167-170, ov166, ov285-289, ov284, ov297, ov337 | **21 funcs / 1.8 KB** (`enceff.c`); DS Protect detect (16 funcs, stub) | ov167 1,126 funcs / 117 KB (13 rows), ov168 479 / 66 KB (10 rows), ~90 data symbols still in asm (`btl_client.c` 33, `btlv_effvm.c` 15, `btlv_input.c` 15...) | battle is all C; mostly verification and data |
| **M6** save / load | main save stack, ov12, ov36, ov87, ov331, ov162 | 0 beyond M3 (`field_msgbg.c` for the save dialog) | 0 beyond M3 | save backend in the platform layer |
| **M7** menus (bag, party, Pokédex, PC) | ov36 `field_menu.c`, ov139, ov142, ov165, ov207, ov255, ov296-304, ov186, ov140, ov144 | **15 funcs / 1.8 KB** (`shortcut_menu.c`, Y-button registered items) | ov165 `plist_sys.c` 89 funcs / 19 KB (2 rows, 22 data), ov139 `frame_list.c` 55 / 7.6 KB, DS Protect in ov165 | party list's DS Protect hooks must be stubbed |

Deferrable ov36 files (link play, facilities): `comm_entry_menu.c` 75, `ircbattlemenu.c` 63, `party_select_list.c` 30,
`scrcmd_hidden_hollow.c` 19, `union_mine_pms.c` 4: 191 functions / 20.7 KB. The M3 + M4 + M5 + M7 + deferred rows add
up to the 1,563 ov36 functions / 112,564 bytes without C.

Out of the slice: Wi-Fi (ov11 DWC 988 functions / 143 KB without C, ov182 DWC utility 910 / 79 KB, ov29 VChat,
ov189 GTS, ov260, ov201), DSi camera/Xtransceiver (ov257, LTD autoload 461 / 14 KB).

## 3. Overlay semantics the port must reproduce

### 3.1 Numbers

- 344 overlays. 84 have no functions: ov6-7, 19, 37-49, 58, 138, 158-161, 163, 183, 188, 202, 206, 212, 217-252,
  254, 274-278, 282-283, 291-292, 329, 333-336, 339-343; all are 32-byte placeholders except ov58 (192 bytes, the
  data-only `scrcmd_resort_table.c`).
- `.data`: 167 overlays have a `.data` section (3,052 data symbols; 87.8 KB). Much of it is tables that MWCC puts in
  `.data` (non-const initializers, the `"file.c"` strings for `GFL_HeapAllocate`), but some is mutable.
- `.bss`: 34 overlays have `.bss` (726 symbols, 221 KB; ov36 64 KB, ov194 48 KB, ov11 26 KB, ov189 18 KB,
  ov257 15 KB, ov167 10.6 KB).
- `.ctor` (static initializers): **only ov165** has entries (5): DS Protect's self-decrypting routines
  (`DSProt_DecryptRun`, `DecryptStubCheck`, `DecryptDispatch`, `DecryptDetect`, `DecryptCipher`), which
  `FS_LoadOverlay` runs on every load of the party list. Every other overlay's `.ctor` is just the terminator.
- `GFL_OvlManagerInit(0, 20, 4, 4)`: up to 20 main-RAM overlays loaded at once; `GFL_OvlLoad` asserts and refuses an
  overlay whose range overlaps a loaded one (`GFL_OvlCheckMemoryCollision`); ITCM/DTCM overlays load without DMA
  (none are used in BW2).

### 3.2 Address-sharing groups

Overlapping is pervasive: taking overlap transitively, 318 overlays form one component (0x021503c0-0x021fb880). The
useful view is by load address (exclusive groups share a base; `work/engine/overlay_bases.tsv` has all 72 bases):

| base | members | role |
| --- | --- | --- |
| 0x0214f500 | ov0-9 | Wi-Fi app launch events (GTS, Game Sync, battle video...) |
| 0x0214ff00 | ov10 | `event_make.c` (`OVERLAY_BATTLE`) |
| 0x021503c0 | **ov12 (resident field core) / ov11 (DWC Wi-Fi, 270 KB, reaches 0x021922c0)** | Wi-Fi evicts the field core: `net_define.c` unloads ov12 and ov27 and loads ov11 + ov29 (+ ov189 for GTS), and reloads them after |
| 0x0216e660 | ov13-23 | field event launchers (beacon detail, Pass Power, egg demo, Xtransceiver, poke status, Funfest, PWT, Pokéstar) |
| 0x0216f900 / 0x02170340 | ov24-26 / **ov27 (resident)**, ov28 (Union) | |
| 0x02176b00 | ov33 (field events) / ov34 (Union Room) / ov261 / ov274 | chosen by `FieldRoutine_Open` |
| 0x0217c940 | ov35 (map change glue) / 14 others (mic test, Geonet, IR check...) | |
| 0x0217f600 | **ov36 (field, to 0x021e57e0)** / ov170 (trainer AI) / ov337 (DS Protect) | |
| 0x021e57e0-0x021fb880 | ov37-68 script plugins, ov71-73, ov74-78 weather, ov79-88 subscreen apps, ov89-137 map gimmicks (49 at 0x021eec80), ov146-157 encounter effects, ov210-212, ov226-252 | field-side slots |
| 0x021998c0 | **ov167 (battle, to 0x021dda60)**, ov139, ov142 bag, ov143, ov164, ov172, ov175, ov188, ov206, ov208, ov257, ov258, ov259, ov264, ov265, ov334, ov335 | app/battle world: overlaps ov36 |
| 0x0219ce80 | ov162 title/start + 36 apps | overlaps ov36 |
| 0x021dda60 | ov168 (battle view) / ov171 | |
| 0x021f4260-0x021fb880 | ov285-289 battle apps, ov297, ov290, ov339-343 | |
| 0x06898020 | **ov169 in VRAM bank H** (LCDC) | battle handlers execute from VRAM |
| 0x02400020/40 | ov218-225 | empty, DSi extended RAM |

### 3.3 Code that relies on load-time semantics

- **Field <-> app exclusivity drives the architecture**: every menu app and the battle overlap ov36, so the field is
  closed (`CreateFieldCloseEvent`) and reopened around them; its state lives in `GameData`. A port with everything
  resident doesn't need this, but must keep the close/reopen sequence because game logic (scripts, events, return
  params) is built around it.
- **Load just to read**: `NewGame_Init` loads ov280 and ov294 only to copy `NAME_ENTRY_SOUNDS`/`INTRO_SOUNDS`;
  `EventFieldFirst` and the opening load ov279 to call `InitItemBag`/`InitDreamRadarFlagSave`; ov331 is loaded around
  single calls in `game_start.c`/`startmenu.c`. Harmless when resident.
- **Zeroed `.bss` on each load**: lazily created state such as `sScriptCache` in ov170 `tr_ai.c` (created if NULL,
  freed and reset to NULL), `sWork` in ov30 `net_whpipe.c` (`.data`, initialized NULL, refuses a second init);
  counters never reset by code: `sMicTestBufferFullCount` (ov253), `sBadgeViewCounter` (ov186: `.step` is never
  written, so it relies on zero), `sStaActingPause` (ov209, never written). The code generally reinitializes its
  statics explicitly (e.g. `sSideEffects` in ov169 is cleared with `MI_CpuFill8`), so the dependence is mild.
- **`.data` re-copied on each load**: mutable templates such as `sPlayerTemplate` (ov36 `field_player_nogrid.c`),
  `sGridPlayer` (`fieldmap_ctrl_hybrid.c`), `sPasserbyTemplate` (ov121) have fields written before use, so they
  survive persistence; `sHeapId = 0xffff` (ov181 `mbp.c`) is written on init.
- **Function pointers into overlays**: proc tables (`GameProcFunctions`) and TCB/VBlank callbacks registered by
  overlay code are dropped before the overlay unloads by design; on the DS a stale pointer would crash, on a resident
  port it would silently run, so this direction is safe. ov12/ov27 being unloaded during Wi-Fi means field code is
  unreachable then.
- **Overlay in VRAM**: `btlv_core.c` maps VRAM bank H to LCDC and loads ov169 into it; ov167 reaches it through 478
  linker veneers. On a port: no-op, but bank H can't hold its usual sub-engine data during battles (the renderer
  must honor the LCDC mapping).
- Recommendation: emulate `GFL_OvlLoad` as "reset this overlay's `.data` from a pristine copy and zero its `.bss`"
  (cheap if each overlay's objects are linked into their own sections), keep `GFL_OvlUnload` a no-op, and turn the
  collision assert off. That keeps the zero/reinit assumptions without auditing 167 overlays.

### 3.4 DS Protect (must be neutralized)

Four integration points compute an XOR-rotate checksum over the **machine code** of a DS Protect function and call
a "tamper" handler if it doesn't match `DSPROT_CHECKSUM` (macros `DSPROT_CHECKED_CALL`/`DSPROT_CHECKED_RUN` in
`lib/dsprot/include/dsprot/dsprot.h`):

- `GameSystem_Init` (`src/system/game_system.c`): tamper = add a VBlank task that advances the MT RNG twice per frame
  (cumulative per failure) and perturbs the seed; then once per badge if `seed % 0x12af != 0`.
- `event_mapchange.c` (ov35): tamper = leak 0x1000 bytes of the event heap per map change.
- `event_battle.c` (ov12): checks after each battle.
- `pokelist.c` (ov165): `DSProt_CallCrash` on failure, plus the `.ctor` decryptors.

Compiled natively, every checksum fails, so a port silently gets RNG drift and heap leaks unless these are stubbed
(define the checks to take the "nop" path and drop ov337/ov165's DS Protect objects).

## 4. Engine lineage vs Platinum

Compared against pret's `pokeplatinum` (`pret/main`) in the research clone. That clone also carries PC-port branches
(`origin/pc_port`, `elyro1/pc_port` and `pc_single_screen_mod`, `evoluketion/pc_port` and `single-screen`). The port
puts most of its work in an external reimplementation of the Nintendo SDK/NitroSystem (`subprojects/libntr*.wrap`)
plus `src/port/` (imgui tools, charcode conversion) and ~137 `#ifdef SDK_PORT` patches in game code, most of them
content-specific crash fixes; the engine-level ones are u32->u64 address arithmetic in `savedata.c`, a NARC
file-handle leak, a larger VBlank task limit, and SPL particle fixes.

Verdict: **same Game Freak lineage at the core, rewritten at the presentation layers.**

| subsystem | Platinum | BW2 | verdict | evidence | Platinum port fixes |
| --- | --- | --- | --- | --- | --- |
| overlays | `game_overlay.c` | `gfl/gf_overlay.c` | near-identical | same main/ITCM/DTCM regions, slot scan, collision check, `FS_SetDefaultDMA(FS_DMA_NOT_USE)` for TCM; BW2 sizes lists at init and drops async loads | direct |
| tasks | `sys_task_manager.c` | `gfl/tcb.c` (+ new `tcbl.c`) | near-identical | `SysTask` fields = `TCB` fields; `SysTaskManager_ExecuteTasks` vs `GFL_TCBMgrUpdate` line for line | direct (VBlank limit = `VBLANK_TCB_COUNT`) |
| archives | `narc.c` | `gfl/arc_tool.c`, `arc_util.c` | near-identical | same NARC seek sequence (0xc, FATB, FIMG); `NARC` vs `ArcTool`; BW2 adds in-memory archives | direct; the handle leak is already fixed in BW2 |
| strings | `string_gf.c` | `gfl/strbuf.c` | near-identical | same struct and magic 0xb6f8d2ec | direct |
| heap | `heap.c` | `gfl/heap.c`, `heapsys.c`, `heap_dtcm.c` | same lineage, restructured | NNS FND expanded heaps by ID, child heaps, header before each block; BW2 has a 0x1c header with file/line and a tail-allocation bit, DTCM and DSi layouts | logic maps 1:1; 64-bit alignment of the 0x1c header needs checking |
| procs | `overlay_manager.c` | `gfl/procsys.c` | same lineage, restructured | same load -> init -> main -> exit machine with overlay load/unload; BW2 adds the parent/child stack and queue/replace | by analogy |
| field events | `field_task.c` | `system/game_event.c` | same lineage, restructured | `FieldTask` call stack vs `GameEvent` (`ChainNext`, `GSYS_QueueProcAsEvent` vs `FieldTask_RunApplication`) | by analogy |
| BG / windows | `bg_window.c` | `gfl/bg_sys.c`, `bmp_win.c`, `bmp.c` | same lineage, restructured | `Background` fields map onto `BGSysBG`; deferred scroll/tilemap transfers flushed at VBlank; BW2 is a singleton with an `AreaMan` VRAM allocator and `BmpWin` over `GFLBitmap` | rendering lives in the SDK emulation, so it carries over |
| sprites/OAM | `sprite*.c`, `render_oam.c`, char/pltt transfer | `gfl/clact.c`, `vman.c` | rewritten on the same NitroSystem G2D | same cell/multi-cell animation types, but a new global `ClActSys` with units, OAM managers and its own resource/VRAM manager | little at code level |
| 3D | `easy3d.c`, `camera.c` | `gfl/g3d_*.c`, `double3Ddisp.c`, `billboard*.c` | rewritten on the same NitroSystem G3D | new resource/object/actor/scene managers, simpler camera, dual-screen 3D through capture | library layer reusable |
| sound | `sound*.c` | `system/snd_sys.c`, `snd_bgm_stack.c`, `snd_strm.c`, `pm_voice.c`, `iss_*.c` | rewritten on the same NitroSystem snd | BGM stack, stream player, interactive sound system, BGM-loading OS thread | no; threads needed |
| messages/fonts | `message.c`, `text.c`, `font*.c`, charcode | `gfl/msgdata.c`, `system/printsys.c`, `gf_font.c` | rewritten, different formats | BW2 msg files are multi-language with a per-message key `(id+3)*0x2983` rotated left 3 per character; text is UTF-16 with 0xF000 commands; fonts are Nitro NFTR | charcode conversion doesn't apply; both read little-endian files directly |
| save | `savedata.c`, `save_table.c` | `gfl/savedata.c`, `backup_system.c`, `backup_card.c`, `system/save_control.c`, `save_table.c` | same concept, rewritten | table of blocks, two copies, CRC16-CCITT; BW2: per-block CRCs, footer magic 0x31053527, copies at 0 and 0x26000, writes only changed blocks | the u64 address fix has no counterpart (BW2 uses `u8 *` + offsets); struct layouts still need auditing |
| main loop | `main.c`, `system.c` | `system/main.c`, `gfl_use.c` | same lineage, restructured | `gSystem` vs `GFLAppInit`/`GFLAppUpdate`, HBlank/VBlank TCBs, DSi branch, memory priority | re-add port hooks by hand |

Implication: what transfers from the Platinum PC port is mainly its SDK/NitroSystem reimplementation (libntr: GX/G2D
emulation, NNS G3D, sound, FS) and the patterns of its fixes. BW2 additionally needs DSi (TWL) SDK entry points
(`OS_IsRunOnTwl`, SCFG, the TWL heap layout), OS threads for sound, display capture (title motion blur, dual 3D,
transitions), VRAM-bank-as-code tolerance (ov169), and its own message/font/charset handling. Game-side Platinum fixes
are almost all Platinum-content specific and do not apply.

## 5. Dual-screen and display usage

The only display-select wrapper is `GFL_BGSysSetDisplayLayout` (`src/gfl/gx_layers.c:118`; 1 = main engine on top,
0 = main on the bottom; `GX_Init` defaults to 1). About 85 direct calls (45 x 1, 40 x 0) plus ~42 `*Graphic_Create(layout,
...)` helpers; two raw register writes (`ov000/event_gsync.c:201`, `ov141/bsubway_records.c:355`). Nothing resets it
between apps: each app sets its own.

| mode | top | bottom | main engine | 3D on | capture |
| --- | --- | --- | --- | --- | --- |
| boot logos, start menu | main | sub | top (`boot_screens.c:131`, `startmenu.c:424`) | none | no |
| title ov162 | logo (sub) | 3D Kyurem | bottom (`title.c:503`); swaps to top after the idle timeout (`title.c:328`), back on input | bottom, then top | every frame: blend with VRAM D, display VRAM D (motion blur) |
| intro ov294, 3D demos ov293 | 3D | sub | top (`intro.c:45`, `demo3d.c:85`) | top; both screens in 12 of 26 demos (IDs 6-9, 13, 16-22) | demo motion-blur command; dual 3D |
| field ov36/ov12 | 3D map | subscreen app (C-Gear etc.) on sub BG 4-7 + sub OBJ | top (`fieldmap.c:896`, `:928`) | top | building entry (`field_transition.c:436`, capture to bank D shown as zoomed affine BG2), field-move cut-in (`fld3d_ci.c:893`, frozen 3D backdrop) |
| battle ov167/168 | 3D scene | command UI (sub BG, bank C) | top (`btlv_core.c:231`), never swaps | top | encounter transitions: ov148 captures 3D to bank D and remaps it as a 256x256 texture (ov150-154); ov149 `GFL_G3DScreenTexCapture` |
| bag ov142, PC ov255 | sub | main (touch) | bottom | none | no |
| party ov165, summary ov207, Pokédex ov298-304 | sub | main (touch) | bottom | bottom (3D Pokémon sprites) | no |
| Hall of Fame ov265 | both | both | alternates every frame | both | dual 3D |

- **Dual 3D** (`src/gfl/double3Ddisp.c`, NitroSDK's dual-3D technique): the main engine renders the top on one frame
  and the bottom on the next, capturing each into bank C or D; the sub engine shows the previous capture on the other
  screen (bank C as a direct-color bitmap BG, bank D as 12 64x64 bitmap OBJs). Used only by ov293 3D demos and ov265
  Hall of Fame; field and battle never use it. A port can render both screens natively and drop the 30 Hz alternation,
  keeping the per-screen camera offsets (the cameras model the physical gap: ~48 px in `ddemo_main.c:383-395`).
- **Runtime swaps**: title (idle), GTS's "exchange screens" slide (`worldtrade_search.c:1221`,
  `worldtrade_partner.c:123`/`:485`), Xtransceiver (`ctvt_draw.c:190`/`:346`), `comm_btl_demo.c:417`, and every frame
  in dual 3D. Battle never swaps.
- **C-Gear**: ov79 `c_gear_sw.c` draws only on the sub engine (BG 4-7, sub OBJ), so it's the bottom screen in the field.
  Subscreen apps ov79-83, ov86-88 share 0x021ea860 (ov84/85 stack on ov83), one at a time. The manager is ov36
  `field_subscreen.c` (no C): a table at 0x021cf9dc of 13 entries x 7 words (init, update, alternate-frame update,
  event check, terminate...), e.g. `FieldSubscreen_CGear_Init` loads ov79 and calls its create function; switching via
  `FieldSubscreen_ReqChange`/`ChangeImm`, driven from `fieldmap.c:441/547/592/632`. Before the C-Gear is obtained the
  subscreen is ov80 `no_gear.c`.
- **Touch**: read globally in physical bottom-screen pixels (`src/gfl/touchpanel.c`); apps hit-test rectangles; no
  remapping by engine. The field's top screen ignores touch.
- **LCDC banks**: capture targets (`double3Ddisp`, `demo3d_capture`, `encounter_effect_capture`, `system/screentex.c`,
  `field_transition`, `fld3d_ci`), bulk VRAM clears (`gx_layers.c:15`, `event_mapchange.c:156`, ...), sprite extended
  palette loads (`clact.c:2246-2298`), and ov169's code in bank H.
- **Geometry**: all 23 `G3_ViewPort` calls are 0,0,255,191 and the 4:3 constant 0x1555 appears 28 times. Sprite surfaces
  are 256x192; most apps put the sub surface at y=512 (no sprite spans screens), a few at y=192 so sprites can cross
  (`ov197/mystery_graphic.c:69`, ov199 via `data_02093f24`; `ov311/un_select_graphic.c:62` puts the main surface at 192).
- **Per-scanline effects** a renderer must support: HBlank TCBs for window-edge wipes (`system/wipe_sub.c`, most
  transitions), the battle BG3 wave (`ov168/btlv_bg.c:263`), wave scrolls (ov190, ov199), per-line blend gradients
  (Pokédex lists, ov311). **VCount gating**: `mcss.c:1083` and `btlv_effvm.c:4393` only upload during lines 192-200,
  and `field_g3d_mapper.c:923` checks `GX_GetVCount` too, so a fake line counter must report VBlank-range values or
  those loads never happen.
- Not checked: the 35 asm-only ov36 files (e.g. `field_fade.c`, `weather.c`) could write display-select or capture
  registers inline, which leaves no relocation to search.

Implications: lay out by the top/bottom setting, not by engine, since it changes per app, mid-scene and per frame.
Widescreen matters for field, battle and intro (3D on top); in menus the touch screen is the main engine and the title
has 3D on the bottom. A single-screen layout needs a touch overlay for the C-Gear and battle UI, mapped back to
bottom-screen pixels. Capture must be emulated as render-to-VRAM (A-D) readable as texture, bitmap BG, bitmap OBJ or
direct display.

## 6. Recommended vertical-slice order

**Platform layer first (before M1).** Replace NitroSDK's hardware modules (OS IRQ/alarms/threads/arena, GX/G2/G3
registers and VRAM banks, FS/CARD ROM and backup, PAD/touch/RTC/SPI, SND/PXI, WM, MI DMA) and the 35 crt0/BIOS
functions (LZ77, Huffman, RL, CRC16, Div, Sqrt) with host code; the Platinum port's libntr is the closest starting
point. Needed beyond it: TWL entry points (`OS_IsRunOnTwl`, SCFG; the TWL heap layout is the bigger one), an
SSEQ/SBNK/SWAR/STRM player in place of the undecompiled ARM7, display capture, LCDC mapping, HBlank callbacks and a
VCount that passes through 192-200. Also: overlays resident with `.data` reset and `.bss` zeroing on `GFL_OvlLoad`
(3.3), DS Protect stubbed (3.4), `OVERLAY_n_ID` linker symbols defined as constants, heap budgets reviewed for 64-bit
pointers (fixed sizes per heap ID; the 0x1c block header's alignment).

1. **M1 (logos, GF movie)**: no game decompilation needed. Exercises the 2D engines, the direct-color bitmap BG (the
   GF logo movie writes 0x18000-byte frames to VRAM), LZ decompression, NARC access, input, the save read at boot.
   Starts exercising the written-but-unproven main code (731 functions, most of it SDK; game files have 9 nonmatching rows).
2. **M2 (title, start menu)**: decompile the 4 ITCM ClAct renderers (3.0 KB; every later screen uses sprites);
   needs G3D, capture blending, the runtime display swap, STRM music.
3. **M3 (new game to Aspertia)**: decompile the 10 core ov36 files (456 functions / 27.2 KB; `field_msgbg.c` first),
   stub `sodateya`/`fes_gimmick`/`pleasure_boat`/`field_fmission_info`/`enceff`, write the ~95 data symbols of the
   written ov36 files, and debug the 1,414 written ov36 functions (71 nonmatching). Intro needs MCSS + SPL particles;
   name entry needs touch; the 3D demo is single-screen; the field needs fog, edge marking, billboards, 30 Hz input.
4. **M4 (walk, NPCs, talk, warps)**: the largest decompilation block, 880 functions / 61.1 KB in 19 ov36 files:
   `fldmmdl_acmd.c` (430 functions + 688 data symbols), `fieldmap_event.c`, `ctalkmsgwin.c`, `field_fade.c`,
   `place_name_letter.c`, the field scrcmd files and field effects. Building entries need capture-to-affine-BG.
5. **M5 (wild battle)**: all battle code is C; decompile `enceff.c` (21 functions), write ~90 battle data symbols
   still in asm (`btl_client.c` 33, `btlv_effvm.c` 15, `btlv_input.c` 15...), support capture-as-texture
   transitions, the battle HBlank wave, MCSS VCount gating; ov169's bank-H placement is a no-op.
6. **M6 (save/load)**: nothing new to decompile; the save backend (two copies at 0 and 0x26000, per-block CRCs, footer
   magic 0x31053527) maps to a file.
7. **M7 (menus)**: `shortcut_menu.c` (15 functions); stub the party list's DS Protect; menus put the main engine on
   the bottom, which tests the layout/touch logic.

Out of the slice: Wi-Fi (ov11, ov182, ov29, ov189, ov201, ov260: 1,898 functions without C in ov11 and ov182, all library code),
DSi camera (ov257, LTD), wireless/IR (ov30, ov31 are complete C but need WM/IR backends), the 191 deferred ov36
functions (link menus, facility party select).
