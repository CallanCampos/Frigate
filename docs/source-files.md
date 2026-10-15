# Source files

Each source file is one of the game's original files, as [Code organization](code-organization.md) describes. This lists the
files of every overlay that has some, with the evidence for their names. `tools/decomp/source_files.py --markdown`
prints the tables below from the configs and the ROM:

- **Boundaries.** The linker placed one object per original file, so a file's `.text`, `.rodata`, `.data` and `.bss`
  are each one range, in the same order in every section. `tools/decomp/source_files.py OVERLAY` lists the file names
  embedded in the overlay and where they are used, and `--profile START END` scores each function boundary by whether
  the data references on both sides stay in file order and how many calls cross it. A boundary between two embedded
  names is where both scores are lowest; a boundary without a name nearby comes from the data order alone.
- **Names.** "string" means the overlay embeds the file's name (the address is that of the string, in the file's
  `.data`), which some function passes to `GFL_HeapAllocate` or an assert. "descriptive" means the ROM gives no name,
  and the file is named for what it does, never for an overlay number.
- **Status.** "complete" files link their compiled C. "partial" files link the original code until every function
  matches and the data they use is written; `docs/nonmatching-functions.md` lists the functions in C that do not match.
- The functions outside every file are in the gaps that dsd leaves between them, in original assembly.

### Overlay 0

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_gsync.c` | `0x0214f500`–`0x0214f904` | 3 | complete | descriptive |

### Overlay 1

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_gtsnego.c` | `0x0214f500`–`0x0214f8dc` | 4 | complete | string at `0x0214f8e0` |

### Overlay 2

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_wifibattlematch.c` | `0x0214f500`–`0x0214f6a0` | 3 | complete | string at `0x0214f6c0` |

### Overlay 3

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_battle_video.c` | `0x0214f500`–`0x0214f608` | 3 | complete | descriptive |

### Overlay 4

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_worldtrade.c` | `0x0214f500`–`0x0214f6c8` | 3 | complete | descriptive |

### Overlay 5

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_wbt_wifi.c` | `0x0214f500`–`0x0214f5c8` | 2 | complete | string at `0x0214f5e0` |

### Overlay 8

9 of 9 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_wificlub.c` | `0x0214f500`–`0x0214fe88` | 9 | complete | string at `0x0214fee0` |

### Overlay 9

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_wifi_bsubway.c` | `0x0214f500`–`0x0214f614` | 3 | complete | descriptive |

### Overlay 10

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_make.c` | `0x0214ff00`–`0x0215039c` | 6 | complete | descriptive |

### Overlay 11

230 of 1688 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `net_devwifi.c` | `0x021503c0`–`0x021505d4` | 26 | complete | string at `0x021852ac` |
| `dwc_rap.c` | `0x021505d4`–`0x02152078` | 77 | partial | string at `0x021852d0` |
| `dwc_rapcommon.c` | `0x02152078`–`0x0215251c` | 14 | complete | string at `0x021853f0` |
| `cps.c` | `0x0215251c`–`0x02155e34` | 113 | partial | descriptive |

### Overlay 12

1668 of 1668 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_ircbattle.c` | `0x021503c0`–`0x02150cf8` | 13 | complete | string at `0x0216dfa0` |
| `musical_event.c` | `0x02150cf8`–`0x02151e68` | 33 | complete | string at `0x0216dfb4` |
| `musical_dressup_sys.c` | `0x02151e68`–`0x02151f90` | 5 | complete | string at `0x0216dfd0` |
| `musical_stage_sys.c` | `0x02151f90`–`0x021522d8` | 9 | complete | string at `0x0216dff4` |
| `musical_program.c` | `0x021522d8`–`0x0215264c` | 15 | complete | string at `0x0216e008` |
| `event_colosseum_battle.c` | `0x0215264c`–`0x0215291c` | 3 | complete | descriptive |
| `delivery_beacon.c` | `0x0215291c`–`0x02152c0c` | 22 | partial | string at `0x0216e0a8` |
| `delivery_irc.c` | `0x02152c0c`–`0x02153160` | 27 | complete | string at `0x0216e180` |
| `mystery_gift_pokemon.c` | `0x02153160`–`0x021535dc` | 1 | complete | descriptive |
| `intrude_work.c` | `0x021535dc`–`0x0215366c` | 12 | complete | descriptive |
| `script_sys.c` | `0x0215366c`–`0x02153da4` | 41 | complete | string at `0x0216e190` |
| `script_work.c` | `0x02153da4`–`0x02154070` | 31 | complete | string at `0x0216e1b4` |
| `script_sub_event.c` | `0x02154070`–`0x02154180` | 7 | complete | descriptive |
| `scrcmd_vm.c` | `0x02154180`–`0x02154948` | 59 | complete | descriptive |
| `script_plugin.c` | `0x02154948`–`0x02154a64` | 5 | complete | descriptive |
| `map_matrix.c` | `0x02154a64`–`0x02154c94` | 15 | complete | string at `0x0216e1c4` |
| `map_replace.c` | `0x02154c94`–`0x02154ea0` | 11 | complete | string at `0x0216e1d4` |
| `respawn_zone.c` | `0x02154ea0`–`0x02154f98` | 7 | complete | descriptive |
| `trainer_script.c` | `0x02154f98`–`0x021550a4` | 11 | complete | descriptive |
| `scrcmd_work.c` | `0x021550a4`–`0x021555f8` | 47 | complete | string at `0x0216e1e4` |
| `scrcmd_game_state.c` | `0x021555f8`–`0x02156174` | 55 | complete | descriptive |
| `scrcmd_pokemon.c` | `0x02156174`–`0x021574a4` | 43 | complete | string at `0x0216e1f4` |
| `scrcmd_proc.c` | `0x021574a4`–`0x02157c20` | 21 | complete | string at `0x0216e208` |
| `scrcmd_sodateya.c` | `0x02157c20`–`0x021580c4` | 18 | complete | string at `0x0216e218` |
| `scrcmd_musical.c` | `0x021580c4`–`0x021590ec` | 27 | complete | string at `0x0216e22c` |
| `field_encount_st.c` | `0x021590ec`–`0x021593fc` | 15 | complete | string at `0x0216e240` |
| `hiden_event.c` | `0x021593fc`–`0x02159bc0` | 43 | complete | descriptive |
| `scrcmd_network.c` | `0x02159bc0`–`0x02159d54` | 9 | complete | string at `0x0216e254` |
| `scrcmd_stadium.c` | `0x02159d54`–`0x0215a0b0` | 9 | complete | descriptive |
| `event_battle_lose.c` | `0x0215a0b0`–`0x0215a19c` | 2 | complete | descriptive |
| `event_game_clear.c` | `0x0215a19c`–`0x0215a7a4` | 13 | complete | descriptive |
| `event_field_menu.c` | `0x0215a7a4`–`0x0215aa68` | 3 | complete | descriptive |
| `event_shortcut_menu.c` | `0x0215aa68`–`0x0215b488` | 17 | complete | string at `0x0216e268` |
| `event_field_proclink.c` | `0x0215b488`–`0x0215c4f8` | 37 | complete | string at `0x0216e280` |
| `event_save.c` | `0x0215c4f8`–`0x0215c6b0` | 5 | complete | descriptive |
| `event_action_call.c` | `0x0215c6b0`–`0x0215cb48` | 13 | complete | descriptive |
| `event_3d_demo.c` | `0x0215cb48`–`0x0215cd58` | 5 | complete | descriptive |
| `city_state.c` | `0x0215cd58`–`0x0215cda4` | 3 | complete | descriptive |
| `eventdata_system.c` | `0x0215cda4`–`0x0215dabc` | 61 | partial | string at `0x0216e298` |
| `field_actor_tool.c` | `0x0215dabc`–`0x0215ee10` | 69 | complete | descriptive |
| `zone_change.c` | `0x0215ee10`–`0x0215ef60` | 9 | complete | descriptive |
| `itemuse_event.c` | `0x0215ef60`–`0x0215f23c` | 13 | complete | descriptive |
| `hidden_item.c` | `0x0215f23c`–`0x0215f2d0` | 4 | complete | descriptive |
| `calender.c` | `0x0215f2d0`–`0x0215f440` | 11 | complete | string at `0x0216e2ac` |
| `scrcmd_ndemo.c` | `0x0215f440`–`0x0215f55c` | 5 | complete | descriptive |
| `game_beacon_search.c` | `0x0215f55c`–`0x0215f958` | 17 | complete | string at `0x0216e2b8` |
| `game_beacon_set.c` | `0x0215f958`–`0x02160668` | 86 | complete | descriptive |
| `symbol_map.c` | `0x02160668`–`0x021609b4` | 12 | complete | string at `0x0216e2d0` |
| `event_royal_unova.c` | `0x021609b4`–`0x02160b80` | 8 | complete | descriptive |
| `scrcmd_unity_tower.c` | `0x02160b80`–`0x02160dc8` | 7 | complete | descriptive |
| `building_enter_effect.c` | `0x02160dc8`–`0x02160eb4` | 3 | complete | descriptive |
| `townmap_util.c` | `0x02160eb4`–`0x02160ff4` | 2 | complete | descriptive |
| `scrcmd_phrase_select.c` | `0x02160ff4`–`0x021611e0` | 2 | complete | descriptive |
| `scrcmd_weather.c` | `0x021611e0`–`0x02161260` | 1 | complete | descriptive |
| `symbol_save_field.c` | `0x02161260`–`0x021613d0` | 8 | complete | descriptive |
| `comm_player.c` | `0x021613d0`–`0x02161844` | 12 | complete | string at `0x0216e2e4` |
| `bsubway_comm.c` | `0x02161844`–`0x02161c88` | 27 | complete | descriptive |
| `fld_btl_inst_event.c` | `0x02161c88`–`0x02161f6c` | 4 | complete | descriptive |
| `fld_btl_inst_tool.c` | `0x02161f6c`–`0x02162b64` | 22 | complete | string at `0x0216e36c` |
| `event_cgear_poweron.c` | `0x02162b64`–`0x02162c48` | 3 | complete | descriptive |
| `event_trial_house.c` | `0x02162c48`–`0x02162f44` | 6 | complete | descriptive |
| `ev_time.c` | `0x02162f44`–`0x021631c8` | 13 | complete | descriptive |
| `field_g3d_map.c` | `0x021631c8`–`0x02163b38` | 47 | complete | string at `0x0216e380` |
| `report_event.c` | `0x02163b38`–`0x02164330` | 20 | complete | string at `0x0216e390` |
| `shaymin_form.c` | `0x02164330`–`0x02164490` | 4 | complete | descriptive |
| `scrcmd_trial_house.c` | `0x02164490`–`0x02164838` | 16 | complete | descriptive |
| `scrcmd_actor_move.c` | `0x02164838`–`0x021649ec` | 4 | complete | descriptive |
| `season_form.c` | `0x021649ec`–`0x02164ae0` | 1 | complete | descriptive |
| `scrcmd_entree_forest.c` | `0x02164ae0`–`0x021652cc` | 16 | complete | descriptive |
| `survey.c` | `0x021652cc`–`0x02165598` | 14 | complete | descriptive |
| `scrcmd_sp_poke_gimmick.c` | `0x02165598`–`0x0216598c` | 8 | complete | descriptive |
| `oneshot_dr.c` | `0x0216598c`–`0x021659ec` | 2 | complete | descriptive |
| `namein_setup.c` | `0x021659ec`–`0x02165b1c` | 7 | complete | string at `0x0216e3a0` |
| `event_league_lift.c` | `0x02165b1c`–`0x02165eb8` | 7 | complete | descriptive |
| `event_bsubway.c` | `0x02165eb8`–`0x02166664` | 17 | complete | descriptive |
| `fldmmdl.c` | `0x02166664`–`0x02168320` | 270 | complete | string at `0x0216e5c0` |
| `fest_mission_field.c` | `0x02168320`–`0x02168468` | 5 | complete | descriptive |
| `burmy_form.c` | `0x02168468`–`0x021684bc` | 1 | complete | descriptive |
| `event_battle.c` | `0x021684bc`–`0x0216919c` | 26 | complete | string at `0x0216e5dc` |
| `trcard_sys.c` | `0x0216919c`–`0x02169c1c` | 23 | complete | string at `0x0216e5ec` |
| `scrcmd_sp_poke.c` | `0x02169c1c`–`0x02169c7c` | 4 | complete | descriptive |
| `waza_oshie.c` | `0x02169c7c`–`0x02169e18` | 4 | complete | string at `0x0216e5fc` |
| `g3d_text_draw.c` | `0x02169e18`–`0x0216a190` | 7 | complete | descriptive |
| `pass_power_check.c` | `0x0216a190`–`0x0216a23c` | 2 | complete | descriptive |
| `pair_sys.c` | `0x0216a23c`–`0x0216a6a4` | 16 | complete | string at `0x0216e60c` |
| `scrcmd_hollow_rival.c` | `0x0216a6a4`–`0x0216a82c` | 7 | complete | descriptive |
| `scrcmd_keysystem.c` | `0x0216a82c`–`0x0216a950` | 5 | complete | string at `0x0216e618` |
| `scrcmd_download_data.c` | `0x0216a950`–`0x0216abc0` | 6 | complete | descriptive |
| `scrcmd_pedometer.c` | `0x0216abc0`–`0x0216ac28` | 3 | complete | descriptive |
| `hidden_hollow.c` | `0x0216ac28`–`0x0216acc4` | 2 | complete | descriptive |
| `scrcmd_join_avenue_store.c` | `0x0216acc4`–`0x0216ad3c` | 4 | complete | descriptive |
| `event_field_open_lcd.c` | `0x0216ad3c`–`0x0216adcc` | 3 | complete | descriptive |

### Overlay 13

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_beacon_detail.c` | `0x0216e660`–`0x0216e7a8` | 2 | complete | descriptive |

### Overlay 14

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_pass_power.c` | `0x0216e660`–`0x0216e8d8` | 3 | complete | descriptive |

### Overlay 15

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_medal_info_beacon.c` | `0x0216e660`–`0x0216e7a8` | 2 | complete | descriptive |

### Overlay 16

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_egg_demo.c` | `0x0216e660`–`0x0216e788` | 2 | complete | descriptive |

### Overlay 17

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_comm_tvt.c` | `0x0216e660`–`0x0216e7cc` | 2 | complete | descriptive |

### Overlay 18

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_research_radar.c` | `0x0216e660`–`0x0216e744` | 2 | complete | string at `0x0216e760` |

### Overlay 20

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_poke_status.c` | `0x0216e660`–`0x0216e9d4` | 6 | complete | string at `0x0216e9e0` |

### Overlay 21

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_festival.c` | `0x0216e660`–`0x0216e884` | 3 | complete | descriptive |

### Overlay 22

10 of 10 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_wbt.c` | `0x0216e660`–`0x0216e978` | 10 | complete | descriptive |

### Overlay 23

41 of 41 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_pokewood.c` | `0x0216e660`–`0x0216f838` | 41 | complete | string at `0x0216f8e0` |

### Overlay 24

5 of 5 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fest_mission_sparkle_place.c` | `0x0216f900`–`0x0216fb98` | 5 | complete | descriptive |

### Overlay 25

17 of 17 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fest_mission_people.c` | `0x0216f900`–`0x0216fffc` | 17 | complete | descriptive |

### Overlay 26

5 of 5 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fest_mission_hollow.c` | `0x0216f900`–`0x0216fa50` | 5 | complete | descriptive |

### Overlay 27

42 of 42 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_survey.c` | `0x02170340`–`0x02170ac4` | 31 | complete | descriptive |
| `fest_mission_data.c` | `0x02170ac4`–`0x02170e40` | 11 | complete | string at `0x021711e0` |

### Overlay 28

155 of 155 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `union_comm.c` | `0x02170340`–`0x02170ec8` | 41 | complete | string at `0x02172f20` |
| `union_main.c` | `0x02170ec8`–`0x0217110c` | 10 | complete | string at `0x02172f6c` |
| `union_receive.c` | `0x0217110c`–`0x021719dc` | 44 | complete | descriptive |
| `union_subproc.c` | `0x021719dc`–`0x021723ec` | 13 | complete | descriptive |
| `colosseum_comm.c` | `0x021723ec`–`0x02172830` | 29 | complete | descriptive |
| `colosseum.c` | `0x02172830`–`0x02172c94` | 18 | complete | string at `0x02173084` |

### Overlay 29

115 of 115 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `dwc_rap_vchat.c` | `0x021922c0`–`0x02192ae8` | 19 | complete | descriptive |
| `vct_adpcm.c` | `0x02192ae8`–`0x02193cb0` | 8 | partial | descriptive |
| `vct_audio.c` | `0x02193cb0`–`0x021956d4` | 39 | partial | descriptive |
| `vct_session.c` | `0x021956d4`–`0x02196680` | 30 | partial | descriptive |
| `vct_dsp.c` | `0x02196680`–`0x02196eb4` | 14 | partial | descriptive |
| `vct.c` | `0x02196eb4`–`0x021971a8` | 5 | partial | descriptive |

### Overlay 30

169 of 169 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `net_devwireless.c` | `0x021730e0`–`0x0217333c` | 39 | complete | string at `0x0217576c` |
| `net_whpipe.c` | `0x0217333c`–`0x02174140` | 60 | complete | string at `0x02175780` |
| `wih.c` | `0x02174140`–`0x02175680` | 70 | complete | string at `0x021757b4` |

### Overlay 31

74 of 74 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `net_devirc.c` | `0x02175800`–`0x0217591c` | 24 | complete | descriptive |
| `net_irc.c` | `0x0217591c`–`0x02175f30` | 24 | complete | descriptive |
| `irc_link.c` | `0x02175f30`–`0x02176384` | 19 | complete | descriptive |
| `irc_spi.c` | `0x02176384`–`0x02176628` | 7 | complete | descriptive |

### Overlay 32

5 of 5 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wfc_user_info_warning.c` | `0x021998c0`–`0x02199c5c` | 5 | complete | descriptive |

### Overlay 33

238 of 238 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `entree_forest.c` | `0x02176b00`–`0x02176d88` | 5 | complete | descriptive |
| `event_funfest_mission.c` | `0x02176d88`–`0x02176ec4` | 2 | complete | descriptive |
| `entree_forest_first_warp.c` | `0x02176ec4`–`0x02176f90` | 1 | complete | descriptive |
| `scrcmd_funfest.c` | `0x02176f90`–`0x02177370` | 13 | complete | descriptive |
| `entralink_warp.c` | `0x02177370`–`0x02177574` | 3 | complete | descriptive |
| `event_cgear_shutdown.c` | `0x02177574`–`0x021775e8` | 2 | complete | descriptive |
| `event_phrase_input.c` | `0x021775e8`–`0x02177998` | 6 | complete | descriptive |
| `pdw_postman.c` | `0x02177998`–`0x02178448` | 47 | complete | string at `0x0217c600` |
| `scrcmd_field_move.c` | `0x02178448`–`0x021785c4` | 6 | complete | descriptive |
| `event_sweet_scent.c` | `0x021785c4`–`0x02178908` | 8 | complete | descriptive |
| `event_fly.c` | `0x02178908`–`0x02178ca8` | 3 | complete | descriptive |
| `event_chatot.c` | `0x02178ca8`–`0x021791c0` | 10 | complete | descriptive |
| `event_fishing.c` | `0x021791c0`–`0x02179664` | 9 | complete | descriptive |
| `event_dendou_machine.c` | `0x02179664`–`0x02179868` | 8 | complete | descriptive |
| `event_pc.c` | `0x02179868`–`0x02179b54` | 8 | complete | descriptive |
| `event_pokemon_center.c` | `0x02179b54`–`0x02179dd4` | 9 | complete | descriptive |
| `event_game_manual.c` | `0x02179dd4`–`0x02179e98` | 3 | complete | string at `0x0217c610` |
| `event_abyssal_ruins.c` | `0x02179e98`–`0x02179f04` | 1 | complete | descriptive |
| `event_dive.c` | `0x02179f04`–`0x0217a1b0` | 7 | complete | descriptive |
| `field_actor_animation.c` | `0x0217a1b0`–`0x0217a4a0` | 12 | complete | descriptive |
| `fld_trade.c` | `0x0217a4a0`–`0x0217aa1c` | 11 | complete | string at `0x0217c624` |
| `unity_visitor.c` | `0x0217aa1c`–`0x0217ac70` | 6 | complete | descriptive |
| `trial_house.c` | `0x0217ac70`–`0x0217b468` | 19 | complete | string at `0x0217c630` |
| `bsubway_scr.c` | `0x0217b468`–`0x0217c340` | 39 | partial | string at `0x0217c640` |

### Overlay 34

195 of 195 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `union_actor.c` | `0x02176b00`–`0x02177444` | 23 | complete | descriptive |
| `union_oneself.c` | `0x02177444`–`0x0217aa7c` | 64 | complete | string at `0x0217c6ec` |
| `union_msg.c` | `0x0217aa7c`–`0x0217b68c` | 66 | complete | string at `0x0217c85c` |
| `union_eff.c` | `0x0217b68c`–`0x0217bc84` | 32 | complete | string at `0x0217c8b0` |
| `colosseum_tool.c` | `0x0217bc84`–`0x0217becc` | 10 | complete | descriptive |

### Overlay 35

109 of 109 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_mapchange.c` | `0x0217c940`–`0x0217ed70` | 78 | complete | string at `0x0217f5c0` |
| `el_scoreboard.c` | `0x0217ed70`–`0x0217ee5c` | 4 | complete | string at `0x0217f5e4` |
| `event_season_banner.c` | `0x0217ee5c`–`0x0217f52c` | 27 | complete | descriptive |

### Overlay 36

4339 of 4339 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fieldmap.c` | `0x0217f600`–`0x021814dc` | 128 | partial | string at `0x021d4b20` |
| `fieldmap_event.c` | `0x021814dc`–`0x0218304c` | 55 | partial | descriptive |
| `field_buildmodel.c` | `0x0218304c`–`0x02184244` | 85 | partial | string at `0x021d4b2c` |
| `field_g3d_mapper.c` | `0x02184244`–`0x021856a0` | 57 | partial | string at `0x021d4b4c` |
| `field_camera.c` | `0x021856a0`–`0x02186ed0` | 84 | partial | string at `0x021d4b88` |
| `field_camera_anime.c` | `0x02186ed0`–`0x021876d0` | 34 | partial | string at `0x021d4b98` |
| `field_msgbg.c` | `0x021876d0`–`0x0218a7f8` | 177 | partial | string at `0x021d4bb0` |
| `field_map_chunk_wb.c` | `0x0218a7f8`–`0x0218a8f8` | 2 | complete | descriptive |
| `field_map_chunk_gc.c` | `0x0218a8f8`–`0x0218aa7c` | 3 | complete | descriptive |
| `field_map_chunk_ng.c` | `0x0218aa7c`–`0x0218ab60` | 2 | complete | descriptive |
| `field_map_chunk_rd.c` | `0x0218ab60`–`0x0218adac` | 2 | complete | descriptive |
| `field_wfbc.c` | `0x0218adac`–`0x0218ae38` | 7 | partial | string at `0x021d4bc0` |
| `field_comm_actor.c` | `0x0218ae38`–`0x0218b12c` | 12 | partial | string at `0x021d4bd0` |
| `field_menu_event.c` | `0x0218b12c`–`0x0218b1f8` | 2 | partial | descriptive |
| `ctalkmsgwin.c` | `0x0218b1f8`–`0x0218d0f4` | 56 | partial | string at `0x021d4c23` |
| `ircbattlemenu.c` | `0x0218d0f4`–`0x0218ed18` | 63 | partial | string at `0x021d4d24` |
| `fldmmdl_acmd.c` | `0x0218ed18`–`0x02193680` | 430 | partial | string at `0x021d4d40` |
| `fldmmdl_blact.c` | `0x02193680`–`0x021946b4` | 58 | partial | string at `0x021d4d50` |
| `fldmmdl_g3dobj.c` | `0x021946b4`–`0x02195378` | 91 | partial | string at `0x021d4d60` |
| `fld_act_rail.c` | `0x02195378`–`0x02196238` | 74 | partial | descriptive |
| `field_rail_loader.c` | `0x02196238`–`0x02197534` | 62 | partial | string at `0x021d4de8` |
| `field_fog.c` | `0x02197534`–`0x02197710` | 22 | partial | string at `0x021d4dfc` |
| `field_zonefog.c` | `0x02197710`–`0x02197820` | 11 | partial | string at `0x021d4e08` |
| `field_light.c` | `0x02197820`–`0x02198198` | 31 | partial | string at `0x021d4e18` |
| `field_subscreen.c` | `0x02198198`–`0x02198bc0` | 72 | partial | string at `0x021d4e28` |
| `field_3dbg.c` | `0x02198bc0`–`0x02199004` | 20 | partial | string at `0x021d4e3c` |
| `weather.c` | `0x02199004`–`0x021993e0` | 16 | partial | string at `0x021d4e54` |
| `weather_task.c` | `0x021993e0`–`0x0219a3b4` | 91 | partial | string at `0x021d4e60` |
| `fieldmap_ctrl_grid.c` | `0x0219a3b4`–`0x0219a47c` | 5 | partial | string at `0x021d4e70` |
| `field_player.c` | `0x0219a47c`–`0x0219ad4c` | 68 | complete | string at `0x021d4e88` |
| `field_player_core.c` | `0x0219ad4c`–`0x0219b7a8` | 65 | partial | string at `0x021d4e98` |
| `field_player_grid.c` | `0x0219b7a8`–`0x0219cea0` | 99 | complete | string at `0x021d4eac` |
| `field_player_nogrid.c` | `0x0219cea0`–`0x0219e328` | 91 | partial | string at `0x021d4ee4` |
| `fieldmap_ctrl_nogrid_work.c` | `0x0219e328`–`0x0219e430` | 7 | partial | string at `0x021d4efc` |
| `fieldmap_ctrl_hybrid.c` | `0x0219e430`–`0x0219e9d0` | 13 | complete | string at `0x021d4f44` |
| `field_ground_anime.c` | `0x0219e9d0`–`0x0219f178` | 32 | partial | string at `0x021d4f5c` |
| `event_entrance_effect.c` | `0x0219f178`–`0x0219fa10` | 25 | complete | descriptive |
| `field_menu.c` | `0x0219fa10`–`0x021a0a1c` | 27 | partial | string at `0x021d4f74` |
| `enc_pokeset.c` | `0x021a0a1c`–`0x021a1640` | 27 | partial | descriptive |
| `field_encount.c` | `0x021a1640`–`0x021a1fdc` | 20 | complete | string at `0x021d4f84` |
| `effect_encount.c` | `0x021a1fdc`–`0x021a29d0` | 30 | partial | string at `0x021d4f94` |
| `map_attr.c` | `0x021a29d0`–`0x021a2f08` | 69 | complete | descriptive |
| `fld_scenearea.c` | `0x021a2f08`–`0x021a30f8` | 12 | complete | string at `0x021d4fa8` |
| `fld_scenearea_loader.c` | `0x021a30f8`–`0x021a3664` | 20 | partial | string at `0x021d4fe0` |
| `field_effect.c` | `0x021a3664`–`0x021a3b34` | 38 | complete | string at `0x021d4ff8` |
| `fldeff_shadow.c` | `0x021a3b34`–`0x021a3de0` | 9 | complete | string at `0x021d5008` |
| `fldeff_kemuri.c` | `0x021a3de0`–`0x021a3fd8` | 10 | complete | string at `0x021d5018` |
| `fldeff_grass.c` | `0x021a3fd8`–`0x021a4384` | 10 | partial | string at `0x021d5050` |
| `fldeff_namipoke.c` | `0x021a4384`–`0x021a50a4` | 27 | complete | string at `0x021d5060` |
| `fldeff_encount.c` | `0x021a50a4`–`0x021a5604` | 22 | complete | string at `0x021d50c8` |
| `fldeff_iwakudaki.c` | `0x021a5604`–`0x021a5828` | 10 | complete | string at `0x021d50ec` |
| `fldeff_fishing.c` | `0x021a5828`–`0x021a5a18` | 10 | complete | string at `0x021d5104` |
| `fldeff_fes_kira.c` | `0x021a5a18`–`0x021a5d2c` | 18 | complete | string at `0x021d5124` |
| `event_trainer_eye.c` | `0x021a5d2c`–`0x021a6668` | 39 | complete | string at `0x021d5138` |
| `scrcmd_trainer.c` | `0x021a6668`–`0x021a6b80` | 18 | complete | descriptive |
| `scrcmd_save.c` | `0x021a6b80`–`0x021a6d14` | 3 | complete | descriptive |
| `scrcmd_sound.c` | `0x021a6d14`–`0x021a74ac` | 37 | complete | descriptive |
| `scrcmd_item.c` | `0x021a74ac`–`0x021a7724` | 10 | complete | descriptive |
| `scrcmd_word.c` | `0x021a7724`–`0x021a7eb8` | 36 | complete | descriptive |
| `scrcmd_mapchange.c` | `0x021a7eb8`–`0x021a82bc` | 9 | complete | descriptive |
| `scrcmd_menu.c` | `0x021a82bc`–`0x021a861c` | 11 | complete | descriptive |
| `scrcmd_msg.c` | `0x021a861c`–`0x021a98c4` | 65 | partial | descriptive |
| `scrcmd_fldmmdl.c` | `0x021a98c4`–`0x021aad44` | 50 | complete | string at `0x021d514c` |
| `scrcmd_gym.c` | `0x021aad44`–`0x021ab218` | 28 | complete | descriptive |
| `scrcmd_fade.c` | `0x021ab218`–`0x021ab73c` | 18 | partial | descriptive |
| `scrcmd_buildmodel.c` | `0x021ab73c`–`0x021abb48` | 18 | complete | descriptive |
| `scrcmd_place_name.c` | `0x021abb48`–`0x021abb70` | 1 | complete | descriptive |
| `scrcmd_shop.c` | `0x021abb70`–`0x021adc6c` | 53 | partial | string at `0x021d51cc` |
| `scrcmd_net.c` | `0x021adc6c`–`0x021add94` | 6 | complete | descriptive |
| `scrcmd_proc_fld.c` | `0x021add94`–`0x021ae394` | 17 | complete | string at `0x021d51dc` |
| `scrcmd_encount.c` | `0x021ae394`–`0x021ae6d4` | 9 | complete | string at `0x021d51f0` |
| `scrcmd_camera.c` | `0x021ae6d4`–`0x021aea40` | 14 | complete | descriptive |
| `scrcmd_fld_battle.c` | `0x021aea40`–`0x021aedf4` | 9 | complete | string at `0x021d5204` |
| `scrcmd_pokemon_misc.c` | `0x021aedf4`–`0x021afa2c` | 27 | partial | descriptive |
| `scrcmd_pokemon_fld.c` | `0x021afa2c`–`0x021afcd4` | 4 | partial | string at `0x021d5218` |
| `scrcmd_misc.c` | `0x021afcd4`–`0x021b0228` | 16 | partial | descriptive |
| `field_rail.c` | `0x021b0228`–`0x021b3a3c` | 137 | partial | string at `0x021d5238` |
| `rail_attr.c` | `0x021b3a3c`–`0x021b3bac` | 13 | complete | string at `0x021d525c` |
| `field_nogrid_mapper.c` | `0x021b3bac`–`0x021b3e50` | 18 | complete | string at `0x021d5268` |
| `fldeff_gyoe.c` | `0x021b3e50`–`0x021b427c` | 15 | complete | string at `0x021d5280` |
| `fieldmap_func.c` | `0x021b427c`–`0x021b445c` | 9 | complete | string at `0x021d5290` |
| `union_mine_pms.c` | `0x021b445c`–`0x021b46b0` | 4 | partial | descriptive |
| `fldeff_footmark.c` | `0x021b46b0`–`0x021b496c` | 10 | complete | string at `0x021d5324` |
| `fldeff_reflect.c` | `0x021b496c`–`0x021b4eb4` | 15 | partial | string at `0x021d5338` |
| `field_place_name.c` | `0x021b4eb4`–`0x021b5b7c` | 96 | partial | string at `0x021d534c` |
| `field_fmission_info.c` | `0x021b5b7c`–`0x021b64b0` | 26 | partial | string at `0x021d5360` |
| `fes_gimmick.c` | `0x021b64b0`–`0x021b6824` | 18 | partial | string at `0x021d5378` |
| `place_name_letter.c` | `0x021b6824`–`0x021b733c` | 39 | partial | string at `0x021d53b0` |
| `entrance_camera.c` | `0x021b733c`–`0x021b7f7c` | 53 | partial | string at `0x021d53c4` |
| `field_gimmick.c` | `0x021b7f7c`–`0x021b804c` | 4 | partial | descriptive |
| `fld_exp_obj.c` | `0x021b804c`–`0x021b86d4` | 24 | partial | string at `0x021d53d8` |
| `field_transition.c` | `0x021b86d4`–`0x021b9494` | 45 | partial | descriptive |
| `field_fade.c` | `0x021b9494`–`0x021ba5d0` | 43 | partial | string at `0x021d53e8` |
| `height_ex.c` | `0x021ba5d0`–`0x021ba81c` | 14 | partial | string at `0x021d53f8` |
| `field_player_grid_event.c` | `0x021ba81c`–`0x021bb590` | 23 | partial | string at `0x021d5404` |
| `fld_particle.c` | `0x021bb590`–`0x021bb67c` | 8 | partial | string at `0x021d5420` |
| `fld3d_ci.c` | `0x021bb67c`–`0x021bd2ac` | 54 | partial | string at `0x021d5430` |
| `sodateya.c` | `0x021bd2ac`–`0x021be790` | 65 | partial | string at `0x021d54a4` |
| `fldeff_ripple.c` | `0x021be790`–`0x021be988` | 9 | partial | string at `0x021d54b4` |
| `fldeff_splash.c` | `0x021be988`–`0x021beb70` | 9 | partial | string at `0x021d54c4` |
| `sound_obj.c` | `0x021beb70`–`0x021bedd8` | 9 | complete | string at `0x021d54d4` |
| `shortcut_menu.c` | `0x021bedd8`–`0x021bf914` | 24 | partial | string at `0x021d54e0` |
| `event_name_entry.c` | `0x021bf914`–`0x021bfe88` | 8 | partial | descriptive |
| `field_g3dobj.c` | `0x021bfe88`–`0x021c05d4` | 30 | partial | string at `0x021d54f0` |
| `fieldmap_tcb_camera_zoom.c` | `0x021c05d4`–`0x021c0870` | 6 | partial | string at `0x021d5500` |
| `field_task.c` | `0x021c0870`–`0x021c0914` | 11 | partial | string at `0x021d551c` |
| `field_task_manager.c` | `0x021c0914`–`0x021c1bc4` | 52 | partial | string at `0x021d552c` |
| `field_flash.c` | `0x021c1bc4`–`0x021c1d88` | 18 | partial | string at `0x021d5544` |
| `fieldskill_mapeff.c` | `0x021c1d88`–`0x021c1e30` | 9 | partial | string at `0x021d5554` |
| `fld_wipe_3dobj.c` | `0x021c1e30`–`0x021c2080` | 7 | partial | string at `0x021d5568` |
| `pleasure_boat.c` | `0x021c2080`–`0x021c229c` | 12 | partial | string at `0x021d557c` |
| `field_slope_slip.c` | `0x021c229c`–`0x021c24b4` | 14 | partial | descriptive |
| `field_water_current.c` | `0x021c24b4`–`0x021c2788` | 18 | partial | descriptive |
| `fldeff_d06denki.c` | `0x021c2788`–`0x021c2cbc` | 23 | partial | string at `0x021d558c` |
| `land_data_patch.c` | `0x021c2cbc`–`0x021c2ddc` | 6 | partial | string at `0x021d55a0` |
| `fldeff_iaigiri.c` | `0x021c2ddc`–`0x021c3180` | 13 | partial | string at `0x021d55c0` |
| `party_select_list.c` | `0x021c3180`–`0x021c3c34` | 30 | partial | string at `0x021d55f8` |
| `field_bbd_color.c` | `0x021c3c34`–`0x021c3d9c` | 6 | partial | string at `0x021d5620` |
| `comm_entry_menu.c` | `0x021c3d9c`–`0x021c5d7c` | 75 | partial | string at `0x021d5634` |
| `enceff.c` | `0x021c5d7c`–`0x021c649c` | 21 | partial | string at `0x021d5648` |
| `fldeff_btrain.c` | `0x021c649c`–`0x021c66e8` | 11 | partial | string at `0x021d5654` |
| `fldeff_hide.c` | `0x021c66e8`–`0x021c6934` | 11 | partial | string at `0x021d5664` |
| `fldeff_bubble.c` | `0x021c6934`–`0x021c6cc8` | 15 | partial | string at `0x021d56b0` |
| `field_saveanime.c` | `0x021c6cc8`–`0x021c7028` | 12 | partial | string at `0x021d56c0` |
| `field_map_chunk_height.c` | `0x021c7028`–`0x021c71d4` | 1 | complete | descriptive |
| `scrcmd_field.c` | `0x021c71d4`–`0x021c7b38` | 25 | partial | descriptive |
| `scrcmd_medal.c` | `0x021c7b38`–`0x021c7fd8` | 13 | partial | descriptive |
| `fld_vreq.c` | `0x021c7fd8`–`0x021c81f4` | 14 | complete | string at `0x021d56f0` |
| `field_palace_sys.c` | `0x021c81f4`–`0x021c83e8` | 6 | complete | string at `0x021d56fc` |
| `field_goout_effect.c` | `0x021c83e8`–`0x021c8808` | 11 | complete | string at `0x021d5710` |
| `field_goout_effect_data.c` | `0x021c8808`–`0x021c8954` | 9 | complete | string at `0x021d5728` |
| `resort_mapcreate.c` | `0x021c8954`–`0x021c8c34` | 6 | partial | string at `0x021d5744` |
| `scrcmd_hidden_hollow.c` | `0x021c8c34`–`0x021c944c` | 19 | partial | descriptive |
| `fldeff_ochiba.c` | `0x021c944c`–`0x021c97b8` | 12 | partial | string at `0x021d5758` |
| `scrcmd_ochiba.c` | `0x021c97b8`–`0x021c9eb8` | 22 | complete | string at `0x021d5768` |

### Overlay 50

21 of 21 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_bsubway.c` | `0x021e5800`–`0x021e7008` | 21 | complete | descriptive |

### Overlay 51

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_pleasure_boat.c` | `0x021e5800`–`0x021e5984` | 6 | complete | descriptive |

### Overlay 52

24 of 24 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_pokemon_league.c` | `0x021e5800`–`0x021e5b44` | 24 | complete | descriptive |

### Overlay 53

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_palpark.c` | `0x021e5800`–`0x021e58a4` | 2 | complete | string at `0x021e58c0` |

### Overlay 54

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_abyssal_ruins.c` | `0x021e5800`–`0x021e5868` | 2 | complete | descriptive |

### Overlay 55

122 of 122 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_wbt.c` | `0x021e5800`–`0x021e5b98` | 22 | complete | descriptive |
| `wbt_system.c` | `0x021e5b98`–`0x021e6388` | 45 | complete | string at `0x021e7560` |
| `wbt_tool.c` | `0x021e6388`–`0x021e67f4` | 25 | complete | string at `0x021e7570` |
| `wbt_setup.c` | `0x021e67f4`–`0x021e6b04` | 11 | complete | string at `0x021e757c` |
| `wbt_party.c` | `0x021e6b04`–`0x021e7264` | 19 | complete | string at `0x021e7598` |

### Overlay 56

20 of 20 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_wbt_entrance.c` | `0x021e75c0`–`0x021e7ba4` | 20 | complete | descriptive |

### Overlay 57

24 of 24 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_wbt_stadium.c` | `0x021e75c0`–`0x021e7c14` | 24 | complete | descriptive |

### Overlay 59

46 of 46 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_resort.c` | `0x021e58c0`–`0x021e7c70` | 43 | complete | string at `0x021e7de0` |
| `scrcmd_medalinfo.c` | `0x021e7c70`–`0x021e7db4` | 3 | complete | string at `0x021e7df0` |

### Overlay 60

76 of 76 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_resort_shop.c` | `0x021e58c0`–`0x021e8a4c` | 76 | complete | string at `0x021e8bbc` |

### Overlay 61

32 of 32 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_black_tower.c` | `0x021e5800`–`0x021e6180` | 32 | complete | descriptive |

### Overlay 62

70 of 70 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_pokewood.c` | `0x021e5800`–`0x021e614c` | 36 | complete | descriptive |
| `pokewood_system.c` | `0x021e614c`–`0x021e6680` | 25 | complete | string at `0x021e6a58` |
| `pokewood_setup.c` | `0x021e6680`–`0x021e6868` | 9 | complete | string at `0x021e6a6c` |

### Overlay 63

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_badge_gate.c` | `0x021e5800`–`0x021e5864` | 2 | complete | descriptive |

### Overlay 64

13 of 13 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_plasma_frigate.c` | `0x021e5800`–`0x021e5acc` | 13 | complete | descriptive |

### Overlay 65

66 of 66 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_pokemon_center.c` | `0x021e5800`–`0x021e6684` | 66 | complete | descriptive |

### Overlay 66

17 of 17 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_plugin14.c` | `0x021e5800`–`0x021e5ba0` | 17 | complete | descriptive |

### Overlay 67

2 of 2 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_plugin15.c` | `0x021e5800`–`0x021e58a4` | 2 | complete | descriptive |

### Overlay 68

9 of 9 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_plugin16.c` | `0x021e5800`–`0x021e595c` | 9 | complete | descriptive |

### Overlay 69

36 of 36 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `union_app.c` | `0x0217c940`–`0x0217cf90` | 36 | complete | string at `0x0217cfa0` |

### Overlay 70

101 of 101 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `oekaki_comm.c` | `0x0217cfc0`–`0x0217d34c` | 12 | complete | descriptive |
| `oekaki.c` | `0x0217d34c`–`0x0217f84c` | 89 | complete | string at `0x0217fd38` |

### Overlay 71

10 of 10 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fest_mission_sparkle.c` | `0x021e8be0`–`0x021e8f98` | 10 | complete | descriptive |

### Overlay 72

16 of 16 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fest_mission_gimmick.c` | `0x021e8be0`–`0x021e90b8` | 16 | complete | descriptive |

### Overlay 73

7 of 7 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `rival_select.c` | `0x021e8be0`–`0x021e8d24` | 7 | complete | descriptive |

### Overlay 74

7 of 7 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `weather_sunny.c` | `0x021e90c0`–`0x021e9220` | 7 | complete | descriptive |

### Overlay 75

37 of 37 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `weather_particles.c` | `0x021e90c0`–`0x021ea0dc` | 37 | complete | descriptive |

### Overlay 76

47 of 47 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `weather_rain.c` | `0x021e90c0`–`0x021ea674` | 47 | complete | descriptive |

### Overlay 77

10 of 10 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `weather_sandstorm.c` | `0x021e90c0`–`0x021e94c8` | 10 | complete | descriptive |

### Overlay 78

19 of 19 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `weather_tint.c` | `0x021e90c0`–`0x021e9400` | 19 | complete | descriptive |

### Overlay 79

130 of 130 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `c_gear_sw.c` | `0x021ea860`–`0x021ed7f0` | 130 | complete | string at `0x021eda48` |

### Overlay 80

11 of 11 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `no_gear.c` | `0x021ea860`–`0x021eaad4` | 11 | complete | string at `0x021eab40` |

### Overlay 81

15 of 15 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wbt_subscreen.c` | `0x021ea860`–`0x021eabac` | 15 | complete | string at `0x021eac80` |

### Overlay 82

37 of 37 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `union_subdisp.c` | `0x021ea860`–`0x021eb9fc` | 37 | complete | string at `0x021eba80` |

### Overlay 83

25 of 25 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `beacon_view_common.c` | `0x021ea860`–`0x021eae24` | 25 | complete | string at `0x021eae40` |

### Overlay 84

138 of 138 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `beacon_view_setup.c` | `0x021eae60`–`0x021ec1ac` | 34 | complete | string at `0x021eec60` |
| `beacon_view.c` | `0x021ec1ac`–`0x021ecdec` | 28 | complete | descriptive |
| `beacon_view_sub.c` | `0x021ecdec`–`0x021ee9a4` | 69 | complete | descriptive |
| `beacon_view_accessor.c` | `0x021ee9a4`–`0x021eeb24` | 7 | complete | descriptive |

### Overlay 85

113 of 113 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fmission_view.c` | `0x021eae60`–`0x021ec5cc` | 61 | complete | descriptive |
| `fmission_view_setup.c` | `0x021ec5cc`–`0x021ed688` | 34 | complete | string at `0x021eda40` |
| `fmission_log_accessor_fv.c` | `0x021ed688`–`0x021ed950` | 18 | complete | string at `0x021eda58` |

### Overlay 86

22 of 22 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `dowsing.c` | `0x021ea860`–`0x021eb6ac` | 22 | partial | string at `0x021eb760` |

### Overlay 87

20 of 20 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `report.c` | `0x021ea860`–`0x021eb4c8` | 20 | complete | string at `0x021eb580` |

### Overlay 88

21 of 21 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `c_gear_power_onoff.c` | `0x021ea860`–`0x021eaf94` | 21 | complete | string at `0x021eb038` |

### Overlay 89

7 of 7 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pleasure_boat_rooms.c` | `0x021eec80`–`0x021ef054` | 7 | complete | descriptive |

### Overlay 90

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_state.c` | `0x021eec80`–`0x021eece8` | 3 | complete | descriptive |

### Overlay 91

21 of 21 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_insect.c` | `0x021eec80`–`0x021ef7c8` | 21 | complete | string at `0x021ef9c0` |

### Overlay 92

77 of 77 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_elec.c` | `0x021eec80`–`0x021eff74` | 77 | complete | string at `0x021f0440` |

### Overlay 93

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_nacrene.c` | `0x021eec80`–`0x021eecc4` | 3 | complete | descriptive |

### Overlay 94

34 of 34 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_mistralton.c` | `0x021eec80`–`0x021f0234` | 34 | complete | descriptive |

### Overlay 95

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_striaton.c` | `0x021eec80`–`0x021eecbc` | 3 | complete | descriptive |

### Overlay 96

30 of 30 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_driftveil.c` | `0x021eec80`–`0x021efc70` | 30 | complete | descriptive |

### Overlay 97

8 of 8 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_driftveil_lift.c` | `0x021eec80`–`0x021ef110` | 8 | complete | descriptive |

### Overlay 98

11 of 11 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_icirrus.c` | `0x021eec80`–`0x021ef0f8` | 11 | complete | descriptive |

### Overlay 99

35 of 35 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_opelucid.c` | `0x021eec80`–`0x021efbac` | 35 | complete | descriptive |

### Overlay 100

11 of 11 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_virbank.c` | `0x021eec80`–`0x021ef08c` | 11 | complete | descriptive |

### Overlay 101

43 of 43 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_humilau.c` | `0x021eec80`–`0x021f042c` | 43 | complete | descriptive |

### Overlay 102

27 of 27 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_roller_coaster.c` | `0x021eec80`–`0x021f0194` | 27 | complete | descriptive |

### Overlay 103

17 of 17 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_badge_gate.c` | `0x021eec80`–`0x021ef794` | 17 | complete | descriptive |

### Overlay 104

67 of 67 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `field_gimmick_gate.c` | `0x021eec80`–`0x021efbd8` | 48 | complete | string at `0x021f078c` |
| `gimmick_obj_elboard.c` | `0x021efbd8`–`0x021f039c` | 19 | complete | string at `0x021f07a4` |

### Overlay 105

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_statue.c` | `0x021eec80`–`0x021eee2c` | 6 | complete | descriptive |

### Overlay 106

12 of 12 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_lift.c` | `0x021eec80`–`0x021eee58` | 12 | complete | descriptive |

### Overlay 107

23 of 23 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `field_gimmick_d06.c` | `0x021eec80`–`0x021ef358` | 23 | complete | string at `0x021ef660` |

### Overlay 108

12 of 12 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `field_gimmick_bsubway.c` | `0x021eec80`–`0x021eef8c` | 12 | complete | string at `0x021ef080` |

### Overlay 109

17 of 17 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_skyarrow_bridge.c` | `0x021eec80`–`0x021ef27c` | 17 | complete | descriptive |

### Overlay 110

9 of 9 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_tubeline_bridge.c` | `0x021eec80`–`0x021eef0c` | 9 | complete | descriptive |

### Overlay 111

11 of 11 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_marine_tube.c` | `0x021eec80`–`0x021ef11c` | 11 | complete | descriptive |

### Overlay 112

11 of 11 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_aspertia_city.c` | `0x021eec80`–`0x021eef58` | 11 | complete | descriptive |

### Overlay 113

10 of 10 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_humilau.c` | `0x021eec80`–`0x021eefd8` | 10 | complete | descriptive |

### Overlay 114

9 of 9 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_underground_ruins.c` | `0x021eec80`–`0x021eeef4` | 9 | complete | descriptive |

### Overlay 115

10 of 10 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_center.c` | `0x021eec80`–`0x021eeed0` | 10 | complete | descriptive |

### Overlay 116

14 of 14 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_giant_chasm_6.c` | `0x021eec80`–`0x021ef1bc` | 14 | complete | descriptive |

### Overlay 117

55 of 55 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_plasma_frigate.c` | `0x021eec80`–`0x021efd00` | 55 | complete | descriptive |

### Overlay 118

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_plasma_frigate_deck.c` | `0x021eec80`–`0x021eed98` | 6 | complete | descriptive |

### Overlay 119

18 of 18 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_plasma_frigate_lights.c` | `0x021eec80`–`0x021ef058` | 18 | complete | descriptive |

### Overlay 120

28 of 28 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_champion.c` | `0x021eec80`–`0x021ef36c` | 28 | complete | descriptive |

### Overlay 121

35 of 35 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `castelia_rush.c` | `0x021eef80`–`0x021efb80` | 35 | complete | descriptive |

### Overlay 122

48 of 48 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_shauntal.c` | `0x021eec80`–`0x021ef8e0` | 48 | complete | descriptive |

### Overlay 123

18 of 18 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_grimsley.c` | `0x021eec80`–`0x021ef154` | 18 | complete | descriptive |

### Overlay 124

28 of 28 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_marshal.c` | `0x021eec80`–`0x021ef464` | 28 | complete | descriptive |

### Overlay 125

32 of 32 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_league_caitlin.c` | `0x021eec80`–`0x021ef234` | 32 | complete | descriptive |

### Overlay 126

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `underwater_effect.c` | `0x021eec80`–`0x021eee3c` | 4 | complete | descriptive |

### Overlay 127

137 of 137 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `infinity_dungeon_gimmick.c` | `0x021eec80`–`0x021efaf0` | 40 | complete | string at `0x021f4e20` |
| `infinity_dungeon.c` | `0x021efaf0`–`0x021f0a64` | 34 | complete | string at `0x021f4f04` |
| `infinity_dungeon_rooms.c` | `0x021f0a64`–`0x021f1684` | 38 | complete | descriptive |
| `infinity_dungeon_creator.c` | `0x021f1684`–`0x021f1c80` | 6 | complete | string at `0x021f4f18` |
| `inf_dungeon_retire.c` | `0x021f1c80`–`0x021f2128` | 19 | complete | string at `0x021f4f60` |

### Overlay 128

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_passing_vehicles.c` | `0x021eec80`–`0x021eefe8` | 6 | complete | descriptive |

### Overlay 129

40 of 40 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `sp_poke_gimmick.c` | `0x021eec80`–`0x021ef834` | 31 | complete | string at `0x021f0120` |
| `sp_poke_event.c` | `0x021ef834`–`0x021efccc` | 9 | complete | descriptive |

### Overlay 130

20 of 20 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_funfest_levels.c` | `0x021eec80`–`0x021ef180` | 20 | complete | descriptive |

### Overlay 131

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `rail_slipdown.c` | `0x021eec80`–`0x021ef334` | 4 | complete | string at `0x021ef340` |

### Overlay 132

12 of 12 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_dragonspiral_tower_9.c` | `0x021eec80`–`0x021ef1c4` | 12 | complete | descriptive |

### Overlay 133

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_nuvema.c` | `0x021eec80`–`0x021eeeb0` | 6 | complete | descriptive |

### Overlay 134

67 of 67 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wbt_stadium_gimmick.c` | `0x021eec80`–`0x021efc60` | 35 | complete | descriptive |
| `wbt_battle.c` | `0x021efc60`–`0x021f0994` | 32 | complete | string at `0x021f0edc` |

### Overlay 135

39 of 39 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wbt_makematch.c` | `0x021eec80`–`0x021efbf4` | 39 | complete | string at `0x021efc20` |

### Overlay 136

13 of 13 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_pokewood_poster.c` | `0x021eec80`–`0x021eef5c` | 13 | complete | descriptive |

### Overlay 137

261 of 261 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `resort_field.c` | `0x021eec80`–`0x021f0e98` | 113 | complete | string at `0x021f58a0` |
| `resort_people.c` | `0x021f0e98`–`0x021f1710` | 46 | complete | string at `0x021f58b0` |
| `resort_data_manager.c` | `0x021f1710`–`0x021f1c24` | 30 | complete | string at `0x021f58c0` |
| `resort_npc.c` | `0x021f1c24`–`0x021f1f1c` | 11 | complete | string at `0x021f58d8` |
| `resort_sys.c` | `0x021f1f1c`–`0x021f52ac` | 61 | complete | string at `0x021f58e8` |

### Overlay 139

120 of 120 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `ui_scene.c` | `0x021998c0`–`0x02199aa0` | 9 | complete | string at `0x0219ce20` |
| `touchbar.c` | `0x02199aa0`–`0x0219a2a4` | 41 | complete | string at `0x0219ce2c` |
| `print_msg.c` | `0x0219a2a4`–`0x0219a438` | 3 | complete | descriptive |
| `msgsearch.c` | `0x0219a438`–`0x0219a584` | 4 | complete | string at `0x0219ce38` |
| `yesno_menu.c` | `0x0219a584`–`0x0219af1c` | 8 | complete | string at `0x0219ce44` |
| `frame_list.c` | `0x0219af1c`–`0x0219cd18` | 55 | partial | string at `0x0219ce54` |

### Overlay 140

89 of 89 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `config_panel.c` | `0x0219ce80`–`0x0219ee3c` | 89 | complete | descriptive |

### Overlay 141

42 of 42 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `bsubway_records.c` | `0x0219ce80`–`0x0219de68` | 42 | complete | descriptive |

### Overlay 142

196 of 196 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `itemmenu.c` | `0x021998c0`–`0x0219d4c0` | 125 | complete | string at `0x021a1640` |
| `itemmenu_disp.c` | `0x0219d4c0`–`0x021a03d4` | 58 | complete | descriptive |
| `bag_item.c` | `0x021a03d4`–`0x021a08e0` | 13 | complete | string at `0x021a1694` |

### Overlay 143

247 of 247 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `monolith_main.c` | `0x021998c0`–`0x0219a170` | 21 | complete | descriptive |
| `monolith_tool.c` | `0x0219a170`–`0x0219aa70` | 35 | complete | string at `0x021a043c` |
| `monolith_top.c` | `0x0219aa70`–`0x0219afac` | 17 | complete | descriptive |
| `monolith_record.c` | `0x0219afac`–`0x0219b6d4` | 13 | complete | descriptive |
| `monolith_status.c` | `0x0219b6d4`–`0x0219bb80` | 9 | complete | descriptive |
| `monolith_power_select.c` | `0x0219bb80`–`0x0219d360` | 53 | complete | descriptive |
| `monolith_power_info.c` | `0x0219d360`–`0x0219d8f8` | 12 | complete | descriptive |
| `monolith_mission_select.c` | `0x0219d8f8`–`0x0219f64c` | 69 | complete | descriptive |
| `monolith_mission_info.c` | `0x0219f64c`–`0x0219fe70` | 18 | partial | descriptive |

### Overlay 144

107 of 107 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `townmap.c` | `0x0219ce80`–`0x0219f2dc` | 84 | complete | string at `0x0219f940` |
| `townmap_grh.c` | `0x0219f2dc`–`0x0219f718` | 19 | complete | string at `0x0219f94c` |
| `townmap_data.c` | `0x0219f718`–`0x0219f76c` | 4 | complete | descriptive |

### Overlay 145

83 of 83 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `medalbox.c` | `0x0219ce80`–`0x021a0610` | 74 | complete | descriptive |
| `medalbox_data_sys.c` | `0x021a0610`–`0x021a0cc0` | 9 | complete | string at `0x021a13e8` |

### Overlay 146

30 of 30 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_encounter_cutin.c` | `0x021f59e0`–`0x021f5c50` | 30 | complete | descriptive |

### Overlay 147

1 of 1 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `battle_cutin_db.c` | `0x021f5c60`–`0x021f5c70` | 1 | complete | descriptive |

### Overlay 148

8 of 8 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `encounter_effect_capture.c` | `0x021f59e0`–`0x021f5d78` | 8 | complete | descriptive |

### Overlay 149

9 of 9 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `encounter_effect_model.c` | `0x021f59e0`–`0x021f5c78` | 9 | complete | descriptive |

### Overlay 150

5 of 5 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `encounter_effect_grid.c` | `0x021f5da0`–`0x021f61d8` | 5 | complete | descriptive |

### Overlay 151

8 of 8 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `encounter_effect_ripple.c` | `0x021f5da0`–`0x021f6330` | 8 | partial | descriptive |

### Overlay 152

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `encounter_effect_cells.c` | `0x021f6200`–`0x021f62f0` | 4 | complete | descriptive |

### Overlay 153

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `encounter_effect_columns.c` | `0x021f6200`–`0x021f62e4` | 4 | complete | descriptive |

### Overlay 154

9 of 9 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `encounter_effect_spread.c` | `0x021f6200`–`0x021f6480` | 9 | complete | descriptive |

### Overlay 155

17 of 17 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fld_faceup.c` | `0x021f59e0`–`0x021f6050` | 17 | complete | string at `0x021f60a0` |

### Overlay 156

11 of 11 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `field_pass_power.c` | `0x021f59e0`–`0x021f5e7c` | 11 | complete | descriptive |

### Overlay 157

20 of 20 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_fest_mission.c` | `0x021f59e0`–`0x021f6268` | 20 | complete | descriptive |

### Overlay 162

148 of 148 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `title.c` | `0x0219ce80`–`0x0219d914` | 24 | complete | descriptive |
| `startmenu.c` | `0x0219d914`–`0x021a000c` | 62 | complete | string at `0x021a1ae0` |
| `game_start.c` | `0x021a000c`–`0x021a0a1c` | 22 | complete | descriptive |
| `boot_screens.c` | `0x021a0a1c`–`0x021a0d28` | 3 | complete | descriptive |
| `delete_save.c` | `0x021a0d28`–`0x021a12c0` | 27 | complete | descriptive |
| `save_control_intr.c` | `0x021a12c0`–`0x021a1468` | 10 | complete | string at `0x021a1aec` |

### Overlay 164

7 of 7 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `net_sync.c` | `0x021998c0`–`0x021999e8` | 7 | complete | descriptive |

### Overlay 165

267 of 267 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pokelist.c` | `0x02199a00`–`0x02199b88` | 4 | complete | descriptive |
| `plist_sys.c` | `0x02199b88`–`0x0219e678` | 89 | partial | descriptive |
| `plist_plate.c` | `0x0219e678`–`0x0219f5ec` | 32 | complete | string at `0x021a4cec` |
| `plist_message.c` | `0x0219f5ec`–`0x0219fae8` | 23 | complete | string at `0x021a4cfc` |
| `plist_menu.c` | `0x0219fae8`–`0x0219fef0` | 14 | complete | string at `0x021a4d0c` |
| `plist_item.c` | `0x0219fef0`–`0x021a0d38` | 21 | complete | descriptive |
| `plist_battle.c` | `0x021a0d38`–`0x021a1944` | 18 | complete | string at `0x021a4d1c` |
| `plist_demo.c` | `0x021a1944`–`0x021a23e8` | 21 | complete | string at `0x021a4d2c` |
| `status_rcv.c` | `0x021a23e8`–`0x021a30f8` | 7 | complete | descriptive |
| `dsprot_run.c` | `0x021a30f8`–`0x021a33fc` | 6 | partial | descriptive |
| `dsprot_stub_check.c` | `0x021a33fc`–`0x021a37b0` | 5 | complete | descriptive |
| `dsprot_decrypt.c` | `0x021a37b0`–`0x021a3ab0` | 5 | complete | descriptive |
| `dsprot_detect.c` | `0x021a3ab0`–`0x021a4484` | 14 | partial | descriptive |
| `dsprot_cipher.c` | `0x021a4484`–`0x021a49dc` | 8 | partial | descriptive |

### Overlay 166

10 of 10 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_battle_return.c` | `0x0219ce80`–`0x0219d650` | 10 | complete | string at `0x0219d6c0` |

### Overlay 167

4223 of 4223 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `btl_main.c` | `0x021998c0`–`0x0219e3cc` | 231 | partial | string at `0x021dae80` |
| `btl_server.c` | `0x0219e3cc`–`0x0219f390` | 55 | complete | string at `0x021dae8c` |
| `btl_server_flow.c` | `0x0219f390`–`0x021ae32c` | 997 | partial | string at `0x021dae9c` |
| `btl_server_flow_sub.c` | `0x021ae32c`–`0x021b083c` | 151 | complete | descriptive |
| `btl_handler_work.c` | `0x021b083c`–`0x021b0a1c` | 11 | complete | descriptive |
| `btl_server_cmd.c` | `0x021b0a1c`–`0x021b1674` | 19 | partial | descriptive |
| `btl_client.c` | `0x021b1674`–`0x021b9950` | 311 | partial | string at `0x021daf60` |
| `btl_net.c` | `0x021b9950`–`0x021ba584` | 63 | complete | string at `0x021daf70` |
| `btl_pokeparam.c` | `0x021ba584`–`0x021bc6bc` | 154 | complete | string at `0x021daf7c` |
| `battle_event.c` | `0x021bc6bc`–`0x021bd054` | 46 | complete | descriptive |
| `btl_calc.c` | `0x021bd054`–`0x021bdaf8` | 58 | complete | string at `0x021daf94` |
| `battle_action.c` | `0x021bdaf8`–`0x021bdcac` | 17 | complete | descriptive |
| `ability_handlers.c` | `0x021bdcac`–`0x021c26ec` | 442 | complete | descriptive |
| `item_handlers.c` | `0x021c26ec`–`0x021c5b04` | 431 | complete | descriptive |
| `move_handlers.c` | `0x021c5b04`–`0x021cde38` | 620 | complete | descriptive |
| `handler_common.c` | `0x021cde38`–`0x021ce10c` | 16 | complete | descriptive |
| `battle_overlay.c` | `0x021ce10c`–`0x021ce158` | 2 | complete | descriptive |
| `battle_condition.c` | `0x021ce158`–`0x021ce520` | 20 | complete | descriptive |
| `poke_type_pair.c` | `0x021ce520`–`0x021ce604` | 8 | complete | descriptive |
| `btlv_core.c` | `0x021ce604`–`0x021d0c24` | 295 | complete | string at `0x021dafa0` |
| `btlv_scu.c` | `0x021d0c24`–`0x021d45b0` | 124 | complete | string at `0x021dafac` |
| `btl_rec.c` | `0x021d45b0`–`0x021d4a1c` | 18 | partial | string at `0x021dafb8` |
| `btl_adapter.c` | `0x021d4a1c`–`0x021d4c64` | 17 | complete | string at `0x021dafc4` |
| `btl_string.c` | `0x021d4c64`–`0x021d5968` | 33 | complete | descriptive |
| `btl_field.c` | `0x021d5968`–`0x021d5e1c` | 36 | complete | string at `0x021dafd4` |
| `pokewood_cutin.c` | `0x021d5e1c`–`0x021d6c24` | 48 | partial | string at `0x021dafe0` |

### Overlay 168

632 of 632 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `btlv_stage.c` | `0x021dda60`–`0x021de0f0` | 11 | complete | string at `0x021f3f20` |
| `btlv_field.c` | `0x021de0f0`–`0x021de6b4` | 11 | complete | string at `0x021f3f30` |
| `btlv_camera.c` | `0x021de6b4`–`0x021dec68` | 13 | complete | string at `0x021f3f40` |
| `btlv_effect.c` | `0x021dec68`–`0x021e0de4` | 119 | partial | string at `0x021f3f9c` |
| `btlv_effvm.c` | `0x021e0de4`–`0x021e67b8` | 138 | partial | string at `0x021f4174` |
| `btlv_mcss.c` | `0x021e67b8`–`0x021e982c` | 95 | partial | string at `0x021f4188` |
| `btlv_clact.c` | `0x021e982c`–`0x021ea2a8` | 23 | complete | string at `0x021f4194` |
| `btlv_input.c` | `0x021ea2a8`–`0x021ef794` | 127 | partial | string at `0x021f41c8` |
| `btlv_gauge.c` | `0x021ef794`–`0x021f1c84` | 58 | complete | string at `0x021f41d8` |
| `btlv_b_gauge.c` | `0x021f1c84`–`0x021f2174` | 7 | complete | string at `0x021f41e8` |
| `btlv_timer.c` | `0x021f2174`–`0x021f26b8` | 8 | partial | string at `0x021f41f8` |
| `btlv_bg.c` | `0x021f26b8`–`0x021f2cdc` | 15 | complete | string at `0x021f4208` |
| `btlv_finger_cursor.c` | `0x021f2cdc`–`0x021f2f54` | 7 | complete | string at `0x021f4214` |

### Overlay 169

1298 of 1298 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `field_handlers.c` | `0x06898020`–`0x068981f0` | 36 | complete | descriptive |
| `pos_handlers.c` | `0x068981f0`–`0x06898bfc` | 175 | complete | descriptive |
| `side_handlers.c` | `0x06898bfc`–`0x06899af0` | 261 | complete | descriptive |
| `btlv_scd.c` | `0x06899af0`–`0x0689b938` | 428 | complete | string at `0x0689e938` |
| `condition_effects.c` | `0x0689b938`–`0x0689ca10` | 263 | complete | descriptive |
| `btl_tables.c` | `0x0689ca10`–`0x0689cc20` | 27 | complete | descriptive |
| `btl_poke_select.c` | `0x0689cc20`–`0x0689ccc4` | 10 | complete | descriptive |
| `btl_monset.c` | `0x0689ccc4`–`0x0689d178` | 38 | partial | descriptive |
| `btl_wazarec.c` | `0x0689d178`–`0x0689d284` | 7 | complete | descriptive |
| `btl_faintrec.c` | `0x0689d284`–`0x0689d384` | 9 | complete | descriptive |
| `btl_pospoke.c` | `0x0689d384`–`0x0689d790` | 44 | complete | descriptive |

### Overlay 170

161 of 161 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `tr_ai.c` | `0x0217f600`–`0x02181c74` | 161 | complete | string at `0x02181ea0` |

### Overlay 171

40 of 40 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `vs_multi_list.c` | `0x021dda60`–`0x021deaec` | 40 | complete | string at `0x021dedc0` |

### Overlay 172

39 of 39 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pdc.c` | `0x021998c0`–`0x021999a8` | 12 | complete | string at `0x0219a620` |
| `pdc_main.c` | `0x021999a8`–`0x02199f94` | 11 | complete | string at `0x0219a628` |
| `pdc_msg.c` | `0x02199f94`–`0x0219a53c` | 16 | complete | string at `0x0219a634` |

### Overlay 173

442 of 442 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifi_p2pmatchroom.c` | `0x0219ce80`–`0x0219e0ac` | 88 | complete | descriptive |
| `wifi_p2pmatch_net.c` | `0x0219e0ac`–`0x0219e0f0` | 7 | complete | descriptive |
| `wifi_p2pmatchroom_map.c` | `0x0219e0f0`–`0x0219e108` | 2 | complete | descriptive |
| `wifi_p2pmatch.c` | `0x0219e108`–`0x021a61c0` | 332 | partial | descriptive |
| `wifi_status.c` | `0x021a61c0`–`0x021a6268` | 13 | complete | descriptive |

### Overlay 174

86 of 86 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `ircbattlematch.c` | `0x0219ce80`–`0x0219f0b4` | 86 | complete | string at `0x0219f30c` |

### Overlay 175

90 of 90 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `compatible_irc_sys.c` | `0x021998c0`–`0x0219a0f8` | 48 | complete | string at `0x0219ace4` |
| `irc_compatible.c` | `0x0219a0f8`–`0x0219a89c` | 28 | complete | string at `0x0219acfc` |
| `irc_appbar.c` | `0x0219a89c`–`0x0219abc4` | 14 | complete | string at `0x0219ad10` |

### Overlay 176

60 of 60 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `irc_compat_menu.c` | `0x0219ad20`–`0x0219c278` | 60 | complete | descriptive |

### Overlay 177

79 of 79 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `irc_sync_check.c` | `0x0219ad20`–`0x0219c9d8` | 79 | complete | descriptive |

### Overlay 178

73 of 73 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `irc_rhythm_check.c` | `0x0219ad20`–`0x0219c6e4` | 73 | complete | descriptive |

### Overlay 179

53 of 53 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `irc_compat_result.c` | `0x0219ad20`–`0x0219c144` | 53 | complete | descriptive |

### Overlay 180

70 of 70 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `irc_ranking.c` | `0x0219ad20`–`0x0219cc4c` | 70 | complete | string at `0x0219ce40` |

### Overlay 181

220 of 220 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `mb_parent_sys.c` | `0x0219ce80`–`0x0219eec8` | 28 | partial | string at `0x021a4354` |
| `mbp.c` | `0x0219eec8`–`0x0219f580` | 16 | complete | string at `0x021a4650` |
| `mb_comm_sys.c` | `0x0219f580`–`0x0219fc0c` | 47 | complete | string at `0x021a46a8` |
| `mb_util_msg.c` | `0x0219fc0c`–`0x021a039c` | 26 | partial | string at `0x021a46b8` |
| `mb_parent_dataconv_sys.c` | `0x021a039c`–`0x021a092c` | 15 | partial | string at `0x021a46c8` |
| `mb_comm.c` | `0x021a092c`–`0x021a1698` | 18 | partial | descriptive |
| `mb_segment.c` | `0x021a1698`–`0x021a2040` | 11 | partial | descriptive |
| `mb_gameinfo.c` | `0x021a2040`–`0x021a2638` | 14 | partial | descriptive |
| `mb_wm.c` | `0x021a2638`–`0x021a3b28` | 28 | partial | descriptive |
| `mb_cache.c` | `0x021a3b28`–`0x021a3bd4` | 3 | partial | descriptive |
| `mb_task.c` | `0x021a3bd4`–`0x021a3e44` | 7 | complete | descriptive |
| `mb_frame.c` | `0x021a3e44`–`0x021a4040` | 7 | complete | descriptive |

### Overlay 182

103 of 1113 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `dwc_raputil.c` | `0x021a4920`–`0x021a49d0` | 5 | complete | string at `0x021c05c0` |
| `dwc_aoss.c` | `0x021a49d0`–`0x021a6c1c` | 71 | partial | descriptive |
| `dwc_aoss_ap.c` | `0x021a6c1c`–`0x021a71fc` | 12 | partial | descriptive |
| `dwc_wcm.c` | `0x021a71fc`–`0x021a7858` | 9 | complete | descriptive |
| `dwc_util_ui_effect.c` | `0x021ab040`–`0x021ab17c` | 3 | partial | descriptive |
| `dwc_util_screen.c` | `0x021ab588`–`0x021ab63c` | 3 | partial | descriptive |

### Overlay 184

1 of 1 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `error_window_screen.c` | `0x0219f340`–`0x0219f4ac` | 1 | complete | descriptive |

### Overlay 185

395 of 395 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pms_input_data.c` | `0x0219ce80`–`0x0219d330` | 18 | complete | string at `0x021a7460` |
| `pms_input_view.c` | `0x0219d330`–`0x0219e7bc` | 67 | complete | string at `0x021a7500` |
| `pmsi_search.c` | `0x0219e7bc`–`0x0219ead4` | 11 | complete | string at `0x021a7514` |
| `pmsi_initial_data.c` | `0x0219ead4`–`0x0219eb70` | 9 | complete | descriptive |
| `pmsiv_category.c` | `0x0219eb70`–`0x0219f53c` | 31 | complete | string at `0x021a7524` |
| `pmsiv_edit.c` | `0x0219f53c`–`0x021a006c` | 32 | complete | string at `0x021a7538` |
| `pmsiv_tool.c` | `0x021a006c`–`0x021a0270` | 9 | complete | descriptive |
| `pmsiv_menu.c` | `0x021a0270`–`0x021a0d64` | 41 | complete | string at `0x021a7548` |
| `pms_input.c` | `0x021a0d64`–`0x021a3124` | 96 | complete | string at `0x021a7558` |
| `pmsiv_wordwin.c` | `0x021a3124`–`0x021a3a14` | 23 | complete | string at `0x021a7564` |
| `pms_select.c` | `0x021a3a14`–`0x021a4ef0` | 44 | complete | descriptive |
| `pms_select_graphic.c` | `0x021a4ef0`–`0x021a512c` | 14 | complete | string at `0x021a7578` |

### Overlay 186

162 of 162 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `trainer_card.c` | `0x021a75a0`–`0x021a9f80` | 63 | complete | descriptive |
| `trcard_bmp.c` | `0x021a9f80`–`0x021aaf00` | 22 | complete | descriptive |
| `trcard_obj.c` | `0x021aaf00`–`0x021ab178` | 6 | complete | descriptive |
| `badge_view.c` | `0x021ab178`–`0x021acd20` | 71 | complete | string at `0x021ada4c` |

### Overlay 187

37 of 37 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `medal_info_beacon.c` | `0x021e8be0`–`0x021e8ec8` | 7 | complete | string at `0x021ea078` |
| `medal_info_viewer.c` | `0x021e8ec8`–`0x021e9f0c` | 30 | complete | string at `0x021ea08c` |

### Overlay 189

497 of 497 functions are in source files. Embedded names without a file yet: `ncryption.c`.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `nhttp_rap.c` | `0x0219ce80`–`0x0219d410` | 25 | partial | string at `0x021acffc` |
| `nhttp_rap_evilcheck.c` | `0x0219d410`–`0x0219d504` | 6 | complete | string at `0x021ad0ac` |
| `dpw_profile.c` | `0x0219d504`–`0x0219d550` | 1 | complete | descriptive |
| `nhttp_os.c` | `0x0219d550`–`0x0219d60c` | 8 | partial | descriptive |
| `nhttp_socket.c` | `0x0219d60c`–`0x0219d954` | 14 | partial | descriptive |
| `nhttp_stdlib.c` | `0x0219d954`–`0x0219dd94` | 16 | partial | descriptive |
| `nhttp_bgnend.c` | `0x0219dd94`–`0x0219df00` | 9 | partial | descriptive |
| `nhttp_control.c` | `0x0219df00`–`0x0219e0d8` | 6 | partial | descriptive |
| `nhttp_list.c` | `0x0219e0d8`–`0x0219e1f8` | 6 | partial | descriptive |
| `nhttp_recvbuf.c` | `0x0219e1f8`–`0x0219e538` | 9 | partial | descriptive |
| `nhttp_request.c` | `0x0219e538`–`0x0219ea14` | 8 | partial | descriptive |
| `nhttp_response.c` | `0x0219ea14`–`0x0219ec24` | 4 | partial | descriptive |
| `nhttp_thread.c` | `0x0219ec24`–`0x021a076c` | 30 | partial | descriptive |
| `d_nhttp.c` | `0x021a076c`–`0x021a0a38` | 15 | partial | descriptive |
| `d_nhttp_common.c` | `0x021a0a38`–`0x021a0e1c` | 35 | partial | descriptive |
| `d_nhttp_private.c` | `0x021a0e1c`–`0x021a1160` | 11 | partial | descriptive |
| `dwc_ghttp.c` | `0x021a1160`–`0x021a154c` | 15 | partial | descriptive |
| `ghttpBuffer.c` | `0x021a154c`–`0x021a1980` | 11 | partial | descriptive |
| `ghttpCallbacks.c` | `0x021a1980`–`0x021a1a44` | 3 | partial | descriptive |
| `ghttpCommon.c` | `0x021a1a44`–`0x021a1e0c` | 8 | partial | descriptive |
| `ghttpConnection.c` | `0x021a1e0c`–`0x021a23d8` | 10 | partial | descriptive |
| `ghttpEncryption.c` | `0x021a23d8`–`0x021a2c50` | 17 | partial | descriptive |
| `ghttpMain.c` | `0x021a2c50`–`0x021a3074` | 16 | partial | descriptive |
| `ghttpPost.c` | `0x021a3074`–`0x021a4114` | 21 | partial | descriptive |
| `ghttpProcess.c` | `0x021a4114`–`0x021a54b4` | 18 | partial | descriptive |
| `nd.c` | `0x021a54b4`–`0x021a6c84` | 61 | partial | descriptive |
| `dpw_tr.c` | `0x021a6c84`–`0x021a8010` | 19 | partial | descriptive |
| `dpw_battle_tower.c` | `0x021a8010`–`0x021a893c` | 12 | partial | descriptive |
| `gamestats.c` | `0x021a893c`–`0x021a90f0` | 16 | partial | descriptive |
| `crypt.c` | `0x021a90f0`–`0x021ac838` | 67 | partial | descriptive |

### Overlay 190

94 of 94 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifilogin.c` | `0x021b2f80`–`0x021b3a80` | 39 | complete | descriptive |
| `wifilogin_disp.c` | `0x021b3a80`–`0x021b46cc` | 17 | complete | string at `0x021b53b8` |
| `wifilogin_message.c` | `0x021b46cc`–`0x021b4da4` | 21 | complete | string at `0x021b53cc` |
| `wifilogout.c` | `0x021b4da4`–`0x021b50a0` | 17 | complete | descriptive |

### Overlay 191

80 of 80 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifi_bsubway.c` | `0x021b5420`–`0x021b6fe0` | 74 | complete | descriptive |
| `wifi_bsubway_graphic.c` | `0x021b6fe0`–`0x021b7168` | 6 | complete | string at `0x021b734c` |

### Overlay 192

20 of 20 functions are in source files. Embedded names without a file yet: `pokemontrade_demo_common.c`.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pokemontrade_demo_common_irc.c` | `0x021b2f80`–`0x021b3938` | 18 | complete | descriptive |
| `pokemontrade_ircdemo.c` | `0x021b3938`–`0x021b3f20` | 2 | complete | string at `0x021b430c` |

### Overlay 193

71 of 71 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pokemontrade_demo_common.c` | `0x021b2f80`–`0x021b3858` | 20 | complete | string at `0x021b6d18` |
| `pokemontrade_demo_up.c` | `0x021b3858`–`0x021b4234` | 4 | complete | string at `0x021b6d34` |
| `pokemontrade_demo_down.c` | `0x021b4234`–`0x021b523c` | 20 | complete | string at `0x021b7004` |
| `pokemontrade_demo_middle.c` | `0x021b523c`–`0x021b69d0` | 27 | complete | string at `0x021b73cc` |

### Overlay 194

521 of 521 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pokemontrade_proc.c` | `0x021b76e0`–`0x021bbe14` | 179 | complete | string at `0x021c6eb0` |
| `pokemontrade_nego.c` | `0x021bbe14`–`0x021be808` | 99 | complete | string at `0x021c6ee0` |
| `pokemontrade_mcss.c` | `0x021be808`–`0x021bead8` | 11 | complete | string at `0x021c6ef4` |
| `pokemontrade_save.c` | `0x021bead8`–`0x021bfba4` | 32 | complete | string at `0x021c6f08` |
| `pokemontrade_message.c` | `0x021bfba4`–`0x021c1918` | 50 | complete | descriptive |
| `pokemontrade_3d.c` | `0x021c1918`–`0x021c2a24` | 41 | complete | string at `0x021c6fc0` |
| `pokemontrade_2d.c` | `0x021c2a24`–`0x021c6380` | 109 | partial | string at `0x021c7330` |

### Overlay 195

175 of 175 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gtsnego_proc.c` | `0x021b2f80`–`0x021b4d24` | 67 | complete | descriptive |
| `gtsnego_disp.c` | `0x021b4d24`–`0x021b66b8` | 61 | complete | string at `0x021b7b90` |
| `gtsnego_message.c` | `0x021b66b8`–`0x021b7870` | 47 | complete | string at `0x021b7bb4` |

### Overlay 196

24 of 24 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `net_common_download.c` | `0x021b5420`–`0x021b5cec` | 24 | complete | string at `0x021b5d20` |

### Overlay 197

237 of 237 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `mystery.c` | `0x021b5420`–`0x021b8348` | 57 | complete | string at `0x021be980` |
| `mystery_graphic.c` | `0x021b8348`–`0x021b876c` | 23 | complete | string at `0x021be98c` |
| `mystery_util.c` | `0x021b876c`–`0x021b99e0` | 56 | complete | string at `0x021be9a0` |
| `mystery_album.c` | `0x021b99e0`–`0x021bd068` | 59 | partial | string at `0x021be9b0` |
| `mystery_net.c` | `0x021bd068`–`0x021bdd34` | 40 | complete | string at `0x021be9c0` |
| `mystery_check.c` | `0x021bdd34`–`0x021be2dc` | 2 | complete | descriptive |

### Overlay 198

35 of 35 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `comm_tvt_field.c` | `0x021b2f80`–`0x021b4444` | 35 | complete | descriptive |

### Overlay 199

256 of 256 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gsync_menu.c` | `0x021b2f80`–`0x021b4224` | 41 | complete | string at `0x021b99e0` |
| `gsync_message.c` | `0x021b4224`–`0x021b4870` | 17 | complete | string at `0x021b99f0` |
| `gsync_download.c` | `0x021b4870`–`0x021b4b48` | 16 | complete | string at `0x021b9a00` |
| `gsync_disp.c` | `0x021b4b48`–`0x021b5bfc` | 36 | complete | string at `0x021b9b40` |
| `gsync_state.c` | `0x021b5bfc`–`0x021b8238` | 98 | complete | string at `0x021b9b50` |
| `pdwacc_message.c` | `0x021b8238`–`0x021b88d0` | 13 | complete | string at `0x021b9b98` |
| `pdwacc_disp.c` | `0x021b88d0`–`0x021b8f4c` | 8 | complete | string at `0x021b9bec` |
| `pdwacc_state.c` | `0x021b8f4c`–`0x021b95ac` | 27 | complete | descriptive |

### Overlay 200

17 of 17 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `cg_help.c` | `0x021b2f80`–`0x021b382c` | 17 | complete | string at `0x021b396c` |

### Overlay 201

97 of 97 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gds_rap.c` | `0x021b5420`–`0x021b6104` | 24 | complete | string at `0x021b7ae0` |
| `gds_rap_response.c` | `0x021b6104`–`0x021b631c` | 10 | complete | descriptive |
| `battle_rec_ov.c` | `0x021b631c`–`0x021b64e8` | 7 | complete | descriptive |
| `poke_net.c` | `0x021b64e8`–`0x021b7374` | 31 | partial | descriptive |
| `poke_net_gds.c` | `0x021b7374`–`0x021b7918` | 18 | complete | descriptive |
| `poke_net_ssl.c` | `0x021b7918`–`0x021b7a5c` | 7 | complete | descriptive |

### Overlay 203

170 of 170 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifi_2dchar.c` | `0x021a78c0`–`0x021a8180` | 61 | complete | string at `0x021a9ae0` |
| `wf2dmap_map.c` | `0x021a8180`–`0x021a8260` | 8 | complete | string at `0x021a9af0` |
| `wf2dmap_obj.c` | `0x021a8260`–`0x021a8d50` | 46 | complete | string at `0x021a9b00` |
| `wf2dmap_judge.c` | `0x021a8d50`–`0x021a8f70` | 12 | complete | descriptive |
| `wf2dmap_objdraw.c` | `0x021a8f70`–`0x021a9374` | 25 | complete | string at `0x021a9b10` |
| `wf2dmap_scroll.c` | `0x021a9374`–`0x021a93d4` | 5 | complete | descriptive |
| `wf2dmap_scrdraw.c` | `0x021a93d4`–`0x021a98cc` | 9 | complete | string at `0x021a9b24` |
| `wf2dmap_cmdq.c` | `0x021a98cc`–`0x021a9990` | 4 | complete | string at `0x021a9b38` |

### Overlay 204

170 of 170 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifi_note.c` | `0x021a9b60`–`0x021aded8` | 170 | complete | descriptive |

### Overlay 205

51 of 51 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `codein.c` | `0x021b7380`–`0x021b7710` | 5 | complete | descriptive |
| `codein_pv.c` | `0x021b7710`–`0x021b85f8` | 21 | complete | descriptive |
| `codein_disp.c` | `0x021b85f8`–`0x021b93ec` | 25 | complete | descriptive |

### Overlay 207

164 of 164 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `p_sta_sys.c` | `0x021b2f80`–`0x021b50b0` | 36 | complete | descriptive |
| `p_status.c` | `0x021b50b0`–`0x021b5364` | 3 | complete | string at `0x021bb6ac` |
| `p_sta_sub.c` | `0x021b5364`–`0x021b6924` | 29 | complete | string at `0x021bb6b8` |
| `p_sta_info.c` | `0x021b6924`–`0x021b7644` | 12 | complete | string at `0x021bb6c4` |
| `p_sta_ribbon.c` | `0x021b7644`–`0x021b8510` | 29 | complete | string at `0x021bb6d4` |
| `p_sta_skill.c` | `0x021b8510`–`0x021ba900` | 43 | complete | string at `0x021bb6e4` |
| `p_sta_oam.c` | `0x021ba900`–`0x021bad9c` | 10 | complete | string at `0x021bb6f4` |
| `ribbon.c` | `0x021bad9c`–`0x021bae10` | 2 | complete | descriptive |

### Overlay 208

88 of 88 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `dup_fitting.c` | `0x021998c0`–`0x0219d88c` | 58 | complete | string at `0x0219ddc0` |
| `dup_fitting_item.c` | `0x0219d88c`–`0x0219db98` | 30 | complete | string at `0x0219ddd0` |

### Overlay 209

360 of 360 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `musical_shot_sys.c` | `0x021ba140`–`0x021ba598` | 6 | complete | string at `0x021c300c` |
| `mus_shot_photo.c` | `0x021ba598`–`0x021bad2c` | 10 | complete | string at `0x021c3020` |
| `mus_shot_info.c` | `0x021bad2c`–`0x021bb368` | 11 | complete | string at `0x021c3034` |
| `mus_item_draw.c` | `0x021bb368`–`0x021bb9bc` | 32 | complete | string at `0x021c3044` |
| `mus_poke_draw.c` | `0x021bb9bc`–`0x021bbe50` | 24 | complete | string at `0x021c3054` |
| `musical_mcss.c` | `0x021bbe50`–`0x021bd238` | 35 | complete | string at `0x021c3064` |
| `sta_act_obj.c` | `0x021bd238`–`0x021bd42c` | 10 | complete | string at `0x021c3074` |
| `sta_act_light.c` | `0x021bd42c`–`0x021bd778` | 12 | complete | string at `0x021c3084` |
| `sta_act_bg.c` | `0x021bd778`–`0x021bd8f0` | 7 | complete | string at `0x021c30d0` |
| `sta_act_poke.c` | `0x021bd8f0`–`0x021be9b0` | 36 | complete | string at `0x021c30e0` |
| `sta_acting.c` | `0x021be9b0`–`0x021c0380` | 68 | complete | string at `0x021c30f0` |
| `sta_act_effect.c` | `0x021c0380`–`0x021c051c` | 9 | complete | string at `0x021c3100` |
| `sta_act_audience.c` | `0x021c051c`–`0x021c0ae0` | 15 | complete | string at `0x021c3114` |
| `sta_act_button.c` | `0x021c0ae0`–`0x021c1024` | 6 | complete | string at `0x021c3128` |
| `sta_act_script.c` | `0x021c1024`–`0x021c134c` | 9 | complete | string at `0x021c313c` |
| `script_command.c` | `0x021c134c`–`0x021c2bec` | 70 | complete | string at `0x021c3150` |

### Overlay 210

20 of 20 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `musical_system.c` | `0x021eec80`–`0x021eef38` | 6 | complete | string at `0x021ef1a0` |
| `mus_item_data.c` | `0x021eef38`–`0x021ef188` | 14 | complete | string at `0x021ef1b4` |

### Overlay 211

77 of 77 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `mus_comm_func.c` | `0x021ef1e0`–`0x021f0614` | 77 | complete | string at `0x021f06d0` |

### Overlay 213

7 of 7 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `battle_select.c` | `0x021bb700`–`0x021bbb30` | 7 | complete | descriptive |

### Overlay 214

531 of 531 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `worldtrade.c` | `0x021d3380`–`0x021d3f9c` | 36 | complete | string at `0x021e2a18` |
| `worldtrade_box.c` | `0x021d3f9c`–`0x021d5c48` | 58 | complete | string at `0x021e2bc0` |
| `worldtrade_demo.c` | `0x021d5c48`–`0x021d61dc` | 6 | complete | string at `0x021e2bec` |
| `worldtrade_deposit.c` | `0x021d61dc`–`0x021d77c8` | 59 | partial | string at `0x021e2ca0` |
| `worldtrade_enter.c` | `0x021d77c8`–`0x021d8074` | 30 | complete | string at `0x021e2d34` |
| `worldtrade_input.c` | `0x021d8074`–`0x021daeb8` | 74 | complete | string at `0x021e2dd8` |
| `worldtrade_mypoke.c` | `0x021daeb8`–`0x021dba8c` | 27 | complete | descriptive |
| `worldtrade_partner.c` | `0x021dba8c`–`0x021dc784` | 29 | complete | descriptive |
| `worldtrade_search.c` | `0x021dc784`–`0x021de3f8` | 61 | complete | string at `0x021e2f38` |
| `worldtrade_status.c` | `0x021de3f8`–`0x021de510` | 3 | complete | string at `0x021e2f58` |
| `worldtrade_sublcd.c` | `0x021de510`–`0x021dec14` | 17 | complete | string at `0x021e2f6c` |
| `worldtrade_title.c` | `0x021dec14`–`0x021dfa78` | 39 | complete | descriptive |
| `worldtrade_upload.c` | `0x021dfa78`–`0x021e14e0` | 71 | complete | descriptive |
| `worldtrade_adapter.c` | `0x021e14e0`–`0x021e1ab8` | 21 | complete | string at `0x021e308c` |

### Overlay 215

169 of 169 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `mailtool.c` | `0x021a75a0`–`0x021a771c` | 6 | complete | string at `0x021ab780` |
| `mailsys.c` | `0x021a771c`–`0x021a84e4` | 29 | complete | string at `0x021ab78c` |
| `mailview.c` | `0x021a84e4`–`0x021a8d68` | 16 | complete | string at `0x021ab798` |
| `mailbox.c` | `0x021a8d68`–`0x021a8de4` | 3 | complete | descriptive |
| `mb_main.c` | `0x021a8de4`–`0x021a9740` | 23 | complete | descriptive |
| `mb_bmp.c` | `0x021a9740`–`0x021a9d90` | 15 | complete | descriptive |
| `mb_obj.c` | `0x021a9d90`–`0x021aa188` | 17 | complete | descriptive |
| `mb_seq.c` | `0x021aa188`–`0x021aaefc` | 50 | complete | string at `0x021ab7a4` |
| `mb_ui.c` | `0x021aaefc`–`0x021ab004` | 10 | complete | descriptive |

### Overlay 216

292 of 292 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `guru2.c` | `0x021bb700`–`0x021bb984` | 13 | complete | string at `0x021c1100` |
| `guru2_comm.c` | `0x021bb984`–`0x021bbc98` | 25 | complete | string at `0x021c1108` |
| `guru2_receipt.c` | `0x021bbc98`–`0x021bd6a0` | 69 | complete | descriptive |
| `guru2_main.c` | `0x021bd6a0`–`0x021c08d0` | 185 | complete | string at `0x021c1158` |

### Overlay 253

46 of 46 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `mictest.c` | `0x0217c940`–`0x0217d648` | 46 | complete | string at `0x0217d800` |

### Overlay 255

786 of 786 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `box2.c` | `0x021bb700`–`0x021bb7e0` | 3 | complete | descriptive |
| `box2_main.c` | `0x021bb7e0`–`0x021c28a8` | 163 | complete | string at `0x021d9640` |
| `box2_seq.c` | `0x021c28a8`–`0x021cdf18` | 192 | complete | string at `0x021d964c` |
| `box2_bmp.c` | `0x021cdf18`–`0x021cf3c0` | 58 | complete | descriptive |
| `box2_obj.c` | `0x021cf3c0`–`0x021d23d8` | 130 | complete | descriptive |
| `box2_ui.c` | `0x021d23d8`–`0x021d364c` | 71 | complete | string at `0x021d9658` |
| `box2_bgwfrm.c` | `0x021d364c`–`0x021d3b64` | 35 | complete | descriptive |
| `box_search.c` | `0x021d3b64`–`0x021d6d28` | 118 | complete | descriptive |
| `box_search_graphic.c` | `0x021d6d28`–`0x021d6f6c` | 16 | complete | string at `0x021d96c0` |

### Overlay 256

77 of 77 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `dendou_pc.c` | `0x021bb700`–`0x021bb758` | 3 | complete | descriptive |
| `dpc_main.c` | `0x021bb758`–`0x021bbd10` | 17 | complete | descriptive |
| `dpc_seq.c` | `0x021bbd10`–`0x021bc714` | 21 | complete | descriptive |
| `dpc_bmp.c` | `0x021bc714`–`0x021bce88` | 8 | complete | descriptive |
| `dpc_obj.c` | `0x021bce88`–`0x021bd748` | 25 | complete | descriptive |
| `dpc_ui.c` | `0x021bd748`–`0x021bd8e4` | 3 | complete | descriptive |

### Overlay 257

630 of 630 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `camera_system.c` | `0x021998c0`–`0x0219a350` | 108 | complete | string at `0x021b25a0` |
| `ima_adpcm.c` | `0x0219a350`–`0x0219a550` | 8 | complete | descriptive |
| `draw_system.c` | `0x0219a550`–`0x0219a850` | 12 | partial | string at `0x021b25b0` |
| `ctvt_camera.c` | `0x0219a850`–`0x0219b5b8` | 29 | partial | string at `0x021b25c0` |
| `ctvt_comm.c` | `0x0219b5b8`–`0x0219cd74` | 69 | complete | string at `0x021b25f8` |
| `ctvt_talk.c` | `0x0219cd74`–`0x0219ef4c` | 40 | partial | string at `0x021b2604` |
| `ctvt_mic.c` | `0x0219ef4c`–`0x0219f234` | 24 | complete | string at `0x021b261c` |
| `ctvt_draw.c` | `0x0219f234`–`0x021a0ac4` | 15 | partial | string at `0x021b264c` |
| `ctvt_call.c` | `0x021a0ac4`–`0x021a22bc` | 16 | partial | string at `0x021b2658` |
| `ctvt_game.c` | `0x021a22bc`–`0x021a7620` | 93 | partial | string at `0x021b2674` |
| `ctvt_game_target.c` | `0x021a7620`–`0x021a78d0` | 6 | partial | descriptive |
| `ctvt_game_balloon.c` | `0x021a78d0`–`0x021a8090` | 12 | complete | string at `0x021b2680` |
| `ctvt_game_cam.c` | `0x021a8090`–`0x021a9704` | 67 | partial | string at `0x021b2694` |
| `ctvt_game_cam_graphic.c` | `0x021a9704`–`0x021a98d4` | 8 | complete | string at `0x021b26a4` |
| `comm_tvt_sys.c` | `0x021a98d4`–`0x021aafdc` | 75 | partial | string at `0x021b26c8` |
| `ssp_jpegdec.c` | `0x021aafdc`–`0x021ac664` | 12 | partial | descriptive |
| `ssp_exifdec.c` | `0x021ac664`–`0x021acc64` | 4 | complete | descriptive |
| `ssp_jpegenc.c` | `0x021acc64`–`0x021ae378` | 21 | partial | descriptive |
| `ssp_exifenc.c` | `0x021ae378`–`0x021af740` | 11 | complete | descriptive |

### Overlay 258

84 of 84 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `waza_oshie_main.c` | `0x021998c0`–`0x0219b9a8` | 84 | complete | descriptive |

### Overlay 259

318 of 318 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `local_tvt_sys.c` | `0x021998c0`–`0x02199b3c` | 11 | complete | string at `0x021a0940` |
| `local_tvt_chara.c` | `0x02199b3c`–`0x0219a8a4` | 32 | complete | string at `0x021a0950` |
| `local_tvt_scriptmode.c` | `0x0219a8a4`–`0x0219ac00` | 15 | complete | string at `0x021a0964` |
| `local_tvt_callmode.c` | `0x0219ac00`–`0x0219b3f4` | 16 | complete | string at `0x021a097c` |
| `local_tvt_graphic.c` | `0x0219b3f4`–`0x0219b630` | 14 | complete | string at `0x021a0994` |
| `local_tvt_message.c` | `0x0219b630`–`0x0219be0c` | 29 | complete | string at `0x021a09a8` |
| `local_tvt_util.c` | `0x0219be0c`–`0x0219d558` | 51 | complete | string at `0x021a09bc` |
| `local_tvt_callselect.c` | `0x0219d558`–`0x0219e3f0` | 50 | complete | string at `0x021a09d0` |
| `local_tvt_calldata.c` | `0x0219e3f0`–`0x0219e70c` | 23 | complete | string at `0x021a09e8` |
| `local_tvt_person.c` | `0x0219e70c`–`0x0219e908` | 8 | complete | string at `0x021a0a00` |
| `local_tvt_person_support.c` | `0x0219e908`–`0x0219ee18` | 8 | complete | string at `0x021a0a14` |
| `local_tvt_person_mother.c` | `0x0219ee18`–`0x0219f04c` | 7 | complete | string at `0x021a0a30` |
| `local_tvt_person_doctor.c` | `0x0219f04c`–`0x0219f63c` | 11 | complete | string at `0x021a0a4c` |
| `local_tvt_person_chelen.c` | `0x0219f63c`–`0x0219faac` | 9 | complete | string at `0x021a0a68` |
| `local_tvt_person_rival.c` | `0x0219faac`–`0x0219fcac` | 7 | complete | string at `0x021a0a84` |
| `local_tvt_person_secret.c` | `0x0219fcac`–`0x021a039c` | 27 | complete | string at `0x021a0aa0` |

### Overlay 260

376 of 376 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifibattlematch_link.c` | `0x021b2f80`–`0x021b3064` | 3 | complete | descriptive |
| `wifibattlematch_link_random.c` | `0x021b3064`–`0x021b57dc` | 44 | complete | descriptive |
| `wifibattlematch_link_competition.c` | `0x021b57dc`–`0x021b8cc8` | 46 | complete | descriptive |
| `wifibattlematch_net.c` | `0x021b8cc8`–`0x021bc058` | 100 | partial | string at `0x021bff6c` |
| `wifibattlematch_net_fields.c` | `0x021bc058`–`0x021bc218` | 5 | partial | descriptive |
| `sake.c` | `0x021bc218`–`0x021bc4fc` | 19 | complete | descriptive |
| `sake_error.c` | `0x021bc4fc`–`0x021bc7c4` | 3 | complete | descriptive |
| `sake_post.c` | `0x021bc7c4`–`0x021bc82c` | 2 | complete | descriptive |
| `sake_storage.c` | `0x021bc82c`–`0x021bcc04` | 11 | complete | descriptive |
| `sake_records.c` | `0x021bcc04`–`0x021bd43c` | 25 | complete | descriptive |
| `dwc_sc.c` | `0x021bd43c`–`0x021bda1c` | 29 | complete | descriptive |
| `dwc_competition.c` | `0x021bda1c`–`0x021beaa0` | 55 | partial | descriptive |
| `gdb.c` | `0x021beaa0`–`0x021bf63c` | 26 | partial | descriptive |
| `dwc_auth.c` | `0x021bf63c`–`0x021bfcd8` | 8 | partial | descriptive |

### Overlay 261

127 of 127 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `battle_championship_flow.c` | `0x02176b00`–`0x02177954` | 41 | complete | string at `0x0217a868` |
| `livebattlematch_flow.c` | `0x02177954`–`0x021795c8` | 36 | complete | string at `0x0217a884` |
| `digitalcardcheck.c` | `0x021795c8`–`0x0217a27c` | 24 | complete | string at `0x0217a8bc` |
| `livebattlematch_irc.c` | `0x0217a27c`–`0x0217a7b4` | 26 | complete | string at `0x0217a8d4` |

### Overlay 262

90 of 90 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifibattlematch_graphic.c` | `0x021c2560`–`0x021c279c` | 14 | complete | string at `0x021c5fa0` |
| `wifibattlematch_view.c` | `0x021c279c`–`0x021c568c` | 52 | complete | string at `0x021c5fbc` |
| `wifibattlematch_util.c` | `0x021c568c`–`0x021c5cf8` | 24 | complete | string at `0x021c5fd4` |

### Overlay 263

14 of 14 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifibattlematch_subproc.c` | `0x021c2560`–`0x021c2c50` | 14 | complete | descriptive |

### Overlay 264

45 of 45 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `cdemo_main.c` | `0x021998c0`–`0x02199b40` | 16 | complete | string at `0x0219a2c0` |
| `cdemo_control.c` | `0x02199b40`–`0x02199e9c` | 8 | complete | descriptive |
| `cdemo_script.c` | `0x02199e9c`–`0x0219a098` | 21 | complete | descriptive |

### Overlay 265

99 of 99 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `dendou_demo.c` | `0x021998c0`–`0x0219991c` | 3 | complete | descriptive |
| `ddemo_main.c` | `0x0219991c`–`0x0219a1d4` | 44 | complete | descriptive |
| `ddemo_seq.c` | `0x0219a1d4`–`0x0219aadc` | 17 | complete | descriptive |
| `ddemo_obj.c` | `0x0219aadc`–`0x0219b7d0` | 35 | complete | descriptive |

### Overlay 266

57 of 57 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `staff_roll_main.c` | `0x0219ce80`–`0x0219e518` | 57 | complete | string at `0x0219e660` |

### Overlay 267

22 of 22 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `the_end.c` | `0x0219ce80`–`0x0219d110` | 7 | complete | descriptive |
| `the_end_graphic.c` | `0x0219d110`–`0x0219d46c` | 15 | complete | string at `0x0219d5c0` |

### Overlay 268

191 of 191 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `br_start_proc.c` | `0x021ba140`–`0x021ba4f8` | 7 | complete | descriptive |
| `br_menu_proc.c` | `0x021ba4f8`–`0x021ba7a0` | 4 | complete | descriptive |
| `br_record_proc.c` | `0x021ba7a0`–`0x021bc1e0` | 30 | complete | descriptive |
| `br_btlsubway_proc.c` | `0x021bc1e0`–`0x021bd02c` | 18 | complete | descriptive |
| `br_rndmatch_proc.c` | `0x021bd02c`–`0x021bd804` | 11 | complete | descriptive |
| `br_bvrank_proc.c` | `0x021bd804`–`0x021be790` | 20 | complete | descriptive |
| `br_bvsearch_proc.c` | `0x021be790`–`0x021bf37c` | 19 | complete | descriptive |
| `br_codein_proc.c` | `0x021bf37c`–`0x021bf540` | 3 | complete | descriptive |
| `br_codein.c` | `0x021bf540`–`0x021bf6b8` | 8 | complete | string at `0x021c284c` |
| `br_codein_disp.c` | `0x021bf6b8`–`0x021c004c` | 18 | complete | descriptive |
| `br_codein_pv.c` | `0x021c004c`–`0x021c0afc` | 16 | complete | descriptive |
| `br_bvsend_proc.c` | `0x021c0afc`–`0x021c0ea4` | 6 | complete | descriptive |
| `br_bvdelete_proc.c` | `0x021c0ea4`–`0x021c13f8` | 11 | complete | descriptive |
| `br_bvsave_proc.c` | `0x021c13f8`–`0x021c20e8` | 20 | complete | descriptive |

### Overlay 269

11 of 11 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `br_musical_send_proc.c` | `0x021ef1e0`–`0x021ef77c` | 11 | complete | descriptive |

### Overlay 270

27 of 27 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `br_musical_look_proc.c` | `0x021ef1e0`–`0x021efe00` | 27 | complete | descriptive |

### Overlay 271

293 of 293 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `br_core.c` | `0x021efe40`–`0x021f0770` | 30 | complete | descriptive |
| `br_proc_sys.c` | `0x021f0770`–`0x021f0a6c` | 11 | complete | string at `0x021f7b84` |
| `br_btn.c` | `0x021f0a6c`–`0x021f23b8` | 56 | complete | string at `0x021f7c20` |
| `br_btn_data.c` | `0x021f23b8`–`0x021f266c` | 8 | complete | string at `0x021f7cd8` |
| `br_res.c` | `0x021f266c`–`0x021f31b8` | 20 | complete | string at `0x021f7d50` |
| `br_graphic.c` | `0x021f31b8`–`0x021f34a0` | 16 | complete | string at `0x021f7e10` |
| `br_util.c` | `0x021f34a0`–`0x021f54d8` | 63 | partial | string at `0x021f7e20` |
| `br_fade.c` | `0x021f54d8`–`0x021f5b68` | 17 | complete | string at `0x021f7eb0` |
| `br_sidebar.c` | `0x021f5b68`–`0x021f6224` | 18 | complete | string at `0x021f7ebc` |
| `br_net.c` | `0x021f6224`–`0x021f6a1c` | 37 | complete | string at `0x021f7ed4` |
| `br_pokesearch.c` | `0x021f6a1c`–`0x021f70a0` | 17 | complete | string at `0x021f7f08` |

### Overlay 272

12 of 12 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `br_main.c` | `0x021f7f80`–`0x021f82b8` | 12 | complete | string at `0x021f8320` |

### Overlay 273

28 of 28 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `battle_rec_tool.c` | `0x021e92a0`–`0x021ea074` | 28 | complete | string at `0x021ea080` |

### Overlay 279

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `new_game.c` | `0x021e8be0`–`0x021e8c74` | 4 | complete | descriptive |

### Overlay 280

148 of 148 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `namein.c` | `0x021d96e0`–`0x021dd28c` | 111 | partial | descriptive |
| `namein_graphic.c` | `0x021dd28c`–`0x021dd4c8` | 14 | complete | string at `0x021ddb84` |
| `namein_data.c` | `0x021dd4c8`–`0x021dd8e4` | 23 | complete | string at `0x021ddb98` |

### Overlay 281

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `prof_word.c` | `0x021f59e0`–`0x021f5c00` | 6 | complete | string at `0x021f5c20` |

### Overlay 284

151 of 151 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `shinka_demo.c` | `0x021e30e0`–`0x021e49b0` | 44 | complete | string at `0x021e80a0` |
| `shinka_demo_graphic.c` | `0x021e49b0`–`0x021e4dd0` | 23 | complete | string at `0x021e80b0` |
| `shinka_demo_view.c` | `0x021e4dd0`–`0x021e7824` | 56 | complete | string at `0x021e80c8` |
| `shinka_demo_effect.c` | `0x021e7824`–`0x021e7d9c` | 28 | complete | string at `0x021e80dc` |

### Overlay 285

9 of 9 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `b_app_tool.c` | `0x021f4260`–`0x021f4414` | 9 | complete | string at `0x021f4420` |

### Overlay 286

99 of 99 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `b_bag_bmp.c` | `0x021f4440`–`0x021f4cc4` | 21 | complete | descriptive |
| `b_bag_item.c` | `0x021f4cc4`–`0x021f4f04` | 7 | complete | descriptive |
| `b_bag_anm.c` | `0x021f4f04`–`0x021f5354` | 5 | complete | descriptive |
| `b_bag_obj.c` | `0x021f5354`–`0x021f5b64` | 22 | complete | descriptive |
| `b_bag_main.c` | `0x021f5b64`–`0x021f6c10` | 34 | complete | string at `0x021f7420` |
| `b_bag_ui.c` | `0x021f6c10`–`0x021f6d54` | 10 | complete | descriptive |

### Overlay 287

190 of 190 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `b_plist_anm.c` | `0x021f4440`–`0x021f4f44` | 12 | complete | string at `0x021fb860` |
| `b_plist_bmp.c` | `0x021f4f44`–`0x021f75dc` | 56 | complete | descriptive |
| `b_plist_obj.c` | `0x021f75dc`–`0x021f8714` | 34 | complete | descriptive |
| `b_plist_main.c` | `0x021f8714`–`0x021facd0` | 79 | complete | string at `0x021fb870` |
| `b_plist_cursor.c` | `0x021facd0`–`0x021fadac` | 9 | complete | descriptive |

### Overlay 288

74 of 74 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `b_scenario_main.c` | `0x021f4440`–`0x021f61a8` | 42 | complete | string at `0x021f7230` |
| `b_scenario_graphic.c` | `0x021f61a8`–`0x021f6db0` | 32 | complete | descriptive |

### Overlay 289

33 of 33 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `b_choice_main.c` | `0x021f4440`–`0x021f51a0` | 33 | complete | string at `0x021f53fc` |

### Overlay 290

36 of 36 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wifibattlematch_sys.c` | `0x021fb880`–`0x021fcb4c` | 33 | complete | string at `0x021fd048` |
| `wifibattlematch_battle.c` | `0x021fcb4c`–`0x021fcf84` | 3 | complete | descriptive |

### Overlay 293

112 of 112 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `demo3d.c` | `0x021a1b20`–`0x021a1ea4` | 10 | complete | string at `0x021ab360` |
| `demo3d_graphic.c` | `0x021a1ea4`–`0x021a22b4` | 24 | complete | string at `0x021ab36c` |
| `demo3d_engine.c` | `0x021a22b4`–`0x021a298c` | 11 | complete | string at `0x021ab380` |
| `demo3d_data.c` | `0x021a298c`–`0x021a2ba8` | 13 | complete | string at `0x021ab390` |
| `demo3d_capture.c` | `0x021a2ba8`–`0x021a2db8` | 7 | complete | descriptive |
| `demo3d_cmd.c` | `0x021a2db8`–`0x021a39e4` | 36 | complete | descriptive |
| `demo3d_cmdsys.c` | `0x021a39e4`–`0x021a3c00` | 7 | complete | string at `0x021ab3f0` |
| `demo3d_exp_c_cruiser.c` | `0x021a3c00`–`0x021a3d6c` | 4 | complete | string at `0x021ab400` |

### Overlay 294

132 of 132 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `intro.c` | `0x021a1b20`–`0x021a1c50` | 3 | complete | descriptive |
| `intro_graphic.c` | `0x021a1c50`–`0x021a21ec` | 23 | complete | string at `0x021a4ba0` |
| `intro_cmd.c` | `0x021a21ec`–`0x021a305c` | 61 | complete | string at `0x021a4c90` |
| `intro_script.c` | `0x021a305c`–`0x021a3068` | 1 | complete | descriptive |
| `intro_msg.c` | `0x021a3068`–`0x021a355c` | 16 | complete | string at `0x021a4cd4` |
| `intro_mcss.c` | `0x021a355c`–`0x021a38a8` | 15 | complete | string at `0x021a4ce0` |
| `intro_g3d.c` | `0x021a38a8`–`0x021a3bf8` | 9 | complete | string at `0x021a4cf0` |
| `intro_particle.c` | `0x021a3bf8`–`0x021a3cf4` | 4 | complete | string at `0x021a4d20` |

### Overlay 295

19 of 19 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `game_clear_save.c` | `0x0219ce80`–`0x0219d460` | 13 | complete | descriptive |
| `battle_lose_message.c` | `0x0219d460`–`0x0219d704` | 6 | complete | descriptive |

### Overlay 296

61 of 61 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukan_info.c` | `0x0219d6e0`–`0x0219fa7c` | 61 | complete | string at `0x0219fba0` |

### Overlay 297

31 of 31 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukan_toroku.c` | `0x021f4260`–`0x021f4638` | 6 | complete | descriptive |
| `zukan_toroku_graphic.c` | `0x021f4638`–`0x021f487c` | 16 | complete | string at `0x021f4fa0` |
| `zukan_nickname.c` | `0x021f487c`–`0x021f4e38` | 9 | complete | string at `0x021f4fb8` |

### Overlay 298

380 of 380 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukan_detail_procsys.c` | `0x0219fbc0`–`0x0219fcdc` | 7 | complete | string at `0x021acde0` |
| `zukan_detail_common.c` | `0x0219fcdc`–`0x021a03e8` | 39 | complete | string at `0x021acdf8` |
| `zukan_detail.c` | `0x021a03e8`–`0x021a0700` | 5 | complete | string at `0x021ace10` |
| `zukan_detail_graphic.c` | `0x021a0700`–`0x021a0ae0` | 23 | complete | string at `0x021ace20` |
| `zknd_tbar.c` | `0x021a0ae0`–`0x021a1268` | 39 | complete | string at `0x021ace38` |
| `zukan_detail_touchbar.c` | `0x021a1268`–`0x021a21f0` | 44 | complete | string at `0x021ace44` |
| `zukan_detail_headbar.c` | `0x021a21f0`–`0x021a24e0` | 13 | complete | string at `0x021ace5c` |
| `zukan_detail_info.c` | `0x021a24e0`–`0x021a328c` | 20 | complete | descriptive |
| `zukan_detail_map.c` | `0x021a328c`–`0x021a63d4` | 76 | complete | descriptive |
| `zukan_detail_voice.c` | `0x021a63d4`–`0x021a79e0` | 39 | complete | string at `0x021ace88` |
| `zukan_detail_form.c` | `0x021a79e0`–`0x021abf74` | 75 | complete | string at `0x021acec4` |

### Overlay 299

94 of 94 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukanlist.c` | `0x0219fbc0`–`0x0219fc14` | 3 | complete | descriptive |
| `zknlist_main.c` | `0x0219fc14`–`0x021a05dc` | 29 | complete | descriptive |
| `zknlist_seq.c` | `0x021a05dc`–`0x021a0ec0` | 16 | complete | descriptive |
| `zknlist_bmp.c` | `0x021a0ec0`–`0x021a13f0` | 12 | complete | descriptive |
| `zknlist_obj.c` | `0x021a13f0`–`0x021a23a4` | 33 | complete | descriptive |
| `zknlist_ui.c` | `0x021a23a4`–`0x021a25c8` | 1 | complete | descriptive |

### Overlay 300

190 of 190 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukansearch.c` | `0x0219fbc0`–`0x0219fc14` | 3 | complete | descriptive |
| `zknsearch_main.c` | `0x0219fc14`–`0x021a08d0` | 46 | complete | string at `0x021a4800` |
| `zknsearch_seq.c` | `0x021a08d0`–`0x021a1ce8` | 41 | complete | descriptive |
| `zknsearch_bmp.c` | `0x021a1ce8`–`0x021a26ac` | 25 | complete | descriptive |
| `zknsearch_obj.c` | `0x021a26ac`–`0x021a382c` | 42 | complete | descriptive |
| `zknsearch_ui.c` | `0x021a382c`–`0x021a3bb4` | 12 | complete | descriptive |
| `zknsearch_list.c` | `0x021a3bb4`–`0x021a4094` | 21 | complete | descriptive |

### Overlay 301

19 of 19 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukan_open.c` | `0x0219fbc0`–`0x021a023c` | 19 | complete | descriptive |

### Overlay 302

46 of 46 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukan.c` | `0x021acee0`–`0x021ad624` | 19 | complete | string at `0x021ae5a0` |
| `zukan_common.c` | `0x021ad624`–`0x021adecc` | 20 | complete | string at `0x021ae5a8` |
| `zukan_search_engine.c` | `0x021adecc`–`0x021ae3dc` | 7 | complete | string at `0x021ae5b8` |

### Overlay 303

114 of 114 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukan_habitat.c` | `0x0219fbc0`–`0x021a1a80` | 56 | complete | descriptive |
| `zukan_habitat_graphic.c` | `0x021a1a80`–`0x021a32d8` | 58 | complete | descriptive |

### Overlay 304

92 of 92 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zukan_area_list.c` | `0x0219fbc0`–`0x021a073c` | 34 | complete | descriptive |
| `zukan_area_list_view.c` | `0x021a073c`–`0x021a1dac` | 58 | complete | descriptive |

### Overlay 305

82 of 82 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `comm_btl_demo.c` | `0x0219ce80`–`0x0219e39c` | 61 | complete | descriptive |
| `comm_btl_demo_graphic.c` | `0x0219e39c`–`0x0219e74c` | 21 | complete | string at `0x0219ec1c` |

### Overlay 306

73 of 73 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `btl_rec_sel.c` | `0x0219ce80`–`0x0219e9a0` | 52 | complete | string at `0x0219ef20` |
| `btl_rec_sel_graphic.c` | `0x0219e9a0`–`0x0219ed40` | 21 | complete | string at `0x0219ef30` |

### Overlay 307

97 of 97 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `egg_demo.c` | `0x021ddbc0`–`0x021de730` | 38 | complete | descriptive |
| `egg_demo_graphic.c` | `0x021de730`–`0x021dead0` | 21 | complete | string at `0x021df7e0` |
| `egg_demo_view.c` | `0x021dead0`–`0x021df528` | 38 | complete | string at `0x021df7f4` |

### Overlay 308

79 of 79 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `beacon_detail.c` | `0x0219f960`–`0x021a055c` | 30 | complete | descriptive |
| `beacon_detail_gra.c` | `0x021a055c`–`0x021a07d0` | 14 | complete | string at `0x021a1a60` |
| `beacon_detail_sub.c` | `0x021a07d0`–`0x021a17c0` | 35 | complete | descriptive |

### Overlay 309

87 of 87 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fmission_app.c` | `0x0219ce80`–`0x0219e938` | 34 | complete | descriptive |
| `fmission_app_gra.c` | `0x0219e938`–`0x0219ecd8` | 21 | complete | string at `0x021a06f0` |
| `fmission_app_draw.c` | `0x0219ecd8`–`0x0219fbc0` | 26 | complete | descriptive |
| `fmission_log_accessor_fapp.c` | `0x0219fbc0`–`0x021a014c` | 6 | complete | string at `0x021a0720` |

### Overlay 310

734 of 734 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `research_radar.c` | `0x0219ce80`–`0x0219d3c0` | 23 | complete | descriptive |
| `research_top.c` | `0x0219d3c0`–`0x0219e470` | 96 | complete | string at `0x021a77ec` |
| `research_list.c` | `0x0219e470`–`0x021a0f60` | 186 | complete | string at `0x021a77fc` |
| `research_graph.c` | `0x021a0f60`–`0x021a4394` | 204 | complete | string at `0x021a780c` |
| `research_common.c` | `0x021a4394`–`0x021a47dc` | 49 | complete | string at `0x021a7820` |
| `research_list_recovery.c` | `0x021a47dc`–`0x021a4820` | 9 | complete | string at `0x021a7834` |
| `queue.c` | `0x021a4820`–`0x021a490c` | 13 | complete | string at `0x021a7850` |
| `bg_font.c` | `0x021a490c`–`0x021a4ae0` | 7 | complete | string at `0x021a7858` |
| `circle_graph.c` | `0x021a4ae0`–`0x021a5a88` | 64 | complete | string at `0x021a7864` |
| `arrow.c` | `0x021a5a88`–`0x021a605c` | 25 | complete | string at `0x021a7874` |
| `research_data.c` | `0x021a605c`–`0x021a6100` | 6 | complete | descriptive |
| `percentage.c` | `0x021a6100`–`0x021a63e0` | 24 | complete | string at `0x021a787c` |
| `palette_anime.c` | `0x021a63e0`–`0x021a6848` | 28 | complete | string at `0x021a788c` |

### Overlay 311

70 of 70 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `un_select.c` | `0x0219ce80`–`0x0219e088` | 53 | complete | string at `0x0219e988` |
| `un_select_graphic.c` | `0x0219e088`–`0x0219e3f4` | 17 | complete | string at `0x0219e994` |

### Overlay 312

49 of 49 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `geonet.c` | `0x0217c940`–`0x0217eb9c` | 49 | complete | descriptive |

### Overlay 313

28 of 28 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `th_award.c` | `0x0219ce80`–`0x0219d9a8` | 14 | complete | descriptive |
| `th_award_graphic.c` | `0x0219d9a8`–`0x0219dbe4` | 14 | complete | string at `0x0219ddc0` |

### Overlay 314

33 of 33 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `chihou_zukan_award.c` | `0x0219ce80`–`0x0219d6a4` | 16 | complete | string at `0x0219db80` |
| `chihou_zukan_award_graphic.c` | `0x0219d6a4`–`0x0219da10` | 17 | complete | string at `0x0219db98` |

### Overlay 315

33 of 33 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `zenkoku_zukan_award.c` | `0x0219ce80`–`0x0219d73c` | 16 | complete | string at `0x0219dc40` |
| `zenkoku_zukan_award_graphic.c` | `0x0219d73c`–`0x0219daa8` | 17 | complete | string at `0x0219dc58` |

### Overlay 316

103 of 103 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `psel.c` | `0x0219ce80`–`0x0219f6d0` | 80 | complete | string at `0x0219fe00` |
| `psel_graphic.c` | `0x0219f6d0`–`0x0219fa84` | 23 | complete | string at `0x0219fe08` |

### Overlay 317

21 of 21 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `subway_map.c` | `0x0219ce80`–`0x0219d16c` | 6 | complete | descriptive |
| `subway_map_graphic.c` | `0x0219d16c`–`0x0219d4c8` | 15 | complete | string at `0x0219d600` |

### Overlay 318

22 of 22 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `marine_tube_board.c` | `0x0219ce80`–`0x0219d16c` | 4 | complete | descriptive |
| `marine_tube_board_message.c` | `0x0219d16c`–`0x0219d324` | 6 | complete | descriptive |
| `marine_tube_board_graphic.c` | `0x0219d324`–`0x0219d550` | 12 | complete | string at `0x0219d640` |

### Overlay 319

115 of 115 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `manual.c` | `0x0219ce80`–`0x0219d1e0` | 4 | complete | descriptive |
| `manual_graphic.c` | `0x0219d1e0`–`0x0219d41c` | 14 | complete | string at `0x0219f8e0` |
| `manual_data.c` | `0x0219d41c`–`0x0219d62c` | 22 | complete | string at `0x0219f8f4` |
| `manual_common.c` | `0x0219d62c`–`0x0219d838` | 6 | complete | string at `0x0219f904` |
| `manual_touchbar.c` | `0x0219d838`–`0x0219df70` | 21 | complete | string at `0x0219f914` |
| `manual_list.c` | `0x0219df70`–`0x0219ebc8` | 19 | complete | string at `0x0219f928` |
| `manual_top.c` | `0x0219ebc8`–`0x0219eff4` | 10 | complete | string at `0x0219f938` |
| `manual_category.c` | `0x0219eff4`–`0x0219f168` | 4 | complete | string at `0x0219f948` |
| `manual_title.c` | `0x0219f168`–`0x0219f388` | 4 | complete | string at `0x0219f95c` |
| `manual_explain.c` | `0x0219f388`–`0x0219f6f8` | 11 | complete | string at `0x0219f96c` |

### Overlay 320

66 of 66 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wbt_list.c` | `0x0219ce80`–`0x0219eb70` | 50 | complete | descriptive |
| `wbt_list_graphic.c` | `0x0219eb70`–`0x0219edb4` | 16 | complete | string at `0x0219f140` |

### Overlay 321

51 of 51 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pkwd_count.c` | `0x0219ce80`–`0x0219dc04` | 34 | complete | descriptive |
| `pkwd_count_graphic.c` | `0x0219dc04`–`0x0219de5c` | 17 | complete | string at `0x0219e2a0` |

### Overlay 322

117 of 117 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pkwd_book.c` | `0x0219ce80`–`0x021a02a8` | 101 | complete | descriptive |
| `pkwd_book_graphic.c` | `0x021a02a8`–`0x021a04fc` | 16 | complete | string at `0x021a0c8c` |

### Overlay 323

77 of 77 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pkwd_selectscreen.c` | `0x0219ce80`–`0x0219e6cc` | 63 | complete | string at `0x0219ee44` |
| `pkwd_selectscreen_graphic.c` | `0x0219e6cc`–`0x0219e908` | 14 | complete | string at `0x0219ee58` |

### Overlay 324

74 of 74 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pkwd_briefing.c` | `0x0219ce80`–`0x0219efec` | 74 | complete | string at `0x0219fc4c` |

### Overlay 325

81 of 81 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `pkwd_record_monitor.c` | `0x0219ce80`–`0x0219f098` | 65 | complete | descriptive |
| `pkwd_record_monitor_graphic.c` | `0x0219f098`–`0x0219f2dc` | 16 | complete | string at `0x0219f7d8` |

### Overlay 326

198 of 198 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `win_record.c` | `0x0219ce80`–`0x0219e234` | 47 | complete | descriptive |
| `win_record_graphic.c` | `0x0219e234`–`0x0219e478` | 16 | complete | string at `0x021a36c0` |
| `t_download.c` | `0x0219e478`–`0x021a0a84` | 34 | complete | descriptive |
| `t_download_util.c` | `0x021a0a84`–`0x021a24ac` | 66 | complete | descriptive |
| `t_download_save.c` | `0x021a24ac`–`0x021a2afc` | 19 | complete | string at `0x021a3748` |
| `t_download_graphic.c` | `0x021a2afc`–`0x021a2d40` | 16 | complete | string at `0x021a375c` |

### Overlay 327

60 of 60 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `wbt_download_sys.c` | `0x021b5d80`–`0x021b60a4` | 15 | complete | string at `0x021b7320` |
| `wbt_download_net_graphic.c` | `0x021b60a4`–`0x021b62f0` | 12 | complete | string at `0x021b7354` |
| `wbt_download_net_util.c` | `0x021b62f0`–`0x021b70d4` | 33 | complete | string at `0x021b7370` |

### Overlay 328

66 of 66 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gpower_app.c` | `0x0219ce80`–`0x0219e9d8` | 50 | complete | descriptive |
| `gpower_app_graphic.c` | `0x0219e9d8`–`0x0219ec1c` | 16 | complete | string at `0x0219efa0` |

### Overlay 330

5 of 5 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_pdc_return.c` | `0x0219ce80`–`0x0219d1b4` | 5 | complete | string at `0x0219d1e0` |

### Overlay 331

14 of 14 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `save_outside.c` | `0x021bea20`–`0x021beea4` | 14 | complete | string at `0x021beec0` |

### Overlay 332

384 of 384 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `key_system_main.c` | `0x021beee0`–`0x021c0120` | 53 | complete | string at `0x021c92c0` |
| `key_system_graphic.c` | `0x021c0120`–`0x021c054c` | 22 | complete | string at `0x021c92d8` |
| `key_system_util.c` | `0x021c054c`–`0x021c1dd0` | 82 | complete | string at `0x021c92f0` |
| `key_system_net.c` | `0x021c1dd0`–`0x021c2b18` | 35 | complete | string at `0x021c9394` |
| `key_system_flow.c` | `0x021c2b18`–`0x021c45bc` | 48 | complete | descriptive |
| `key_system_flow_sub.c` | `0x021c45bc`–`0x021c53dc` | 33 | complete | descriptive |
| `data_convert_flow.c` | `0x021c53dc`–`0x021c6e14` | 38 | complete | string at `0x021c944c` |
| `wb_save_block.c` | `0x021c6e14`–`0x021c6e80` | 4 | complete | descriptive |
| `wb_save_convert.c` | `0x021c6e80`–`0x021c7028` | 3 | complete | string at `0x021c9468` |
| `cygnus_flow.c` | `0x021c7028`–`0x021c8b34` | 66 | partial | string at `0x021c947c` |

### Overlay 338

6 of 6 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `ir_check.c` | `0x0217c940`–`0x0217cb10` | 6 | partial | descriptive |

