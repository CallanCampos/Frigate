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
| `event_make.c` | `0x0214ff00`–`0x0215039c` | 6 | partial | descriptive |

### Overlay 12

1668 of 1668 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_ircbattle.c` | `0x021503c0`–`0x02150cf8` | 13 | complete | string at `0x0216dfa0` |
| `musical_event.c` | `0x02150cf8`–`0x02151e68` | 33 | partial | string at `0x0216dfb4` |
| `musical_dressup_sys.c` | `0x02151e68`–`0x02151f90` | 5 | complete | string at `0x0216dfd0` |
| `musical_stage_sys.c` | `0x02151f90`–`0x021522d8` | 9 | complete | string at `0x0216dff4` |
| `musical_program.c` | `0x021522d8`–`0x0215264c` | 15 | partial | string at `0x0216e008` |
| `event_colosseum_battle.c` | `0x0215264c`–`0x0215291c` | 3 | complete | descriptive |
| `delivery_beacon.c` | `0x0215291c`–`0x02152c0c` | 22 | partial | string at `0x0216e0a8` |
| `delivery_irc.c` | `0x02152c0c`–`0x02153160` | 27 | complete | string at `0x0216e180` |
| `mystery_gift_pokemon.c` | `0x02153160`–`0x021535dc` | 1 | partial | descriptive |
| `intrude_work.c` | `0x021535dc`–`0x0215366c` | 12 | complete | descriptive |
| `script_sys.c` | `0x0215366c`–`0x02153da4` | 41 | partial | string at `0x0216e190` |
| `script_work.c` | `0x02153da4`–`0x02154070` | 31 | complete | string at `0x0216e1b4` |
| `script_sub_event.c` | `0x02154070`–`0x02154180` | 7 | partial | descriptive |
| `scrcmd_vm.c` | `0x02154180`–`0x02154948` | 59 | complete | descriptive |
| `script_plugin.c` | `0x02154948`–`0x02154a64` | 5 | complete | descriptive |
| `map_matrix.c` | `0x02154a64`–`0x02154c94` | 15 | partial | string at `0x0216e1c4` |
| `map_replace.c` | `0x02154c94`–`0x02154ea0` | 11 | complete | string at `0x0216e1d4` |
| `respawn_zone.c` | `0x02154ea0`–`0x02154f98` | 7 | complete | descriptive |
| `trainer_script.c` | `0x02154f98`–`0x021550a4` | 11 | complete | descriptive |
| `scrcmd_work.c` | `0x021550a4`–`0x021555f8` | 47 | complete | string at `0x0216e1e4` |
| `scrcmd_game_state.c` | `0x021555f8`–`0x02156174` | 55 | complete | descriptive |
| `scrcmd_pokemon.c` | `0x02156174`–`0x021574a4` | 43 | complete | string at `0x0216e1f4` |
| `scrcmd_proc.c` | `0x021574a4`–`0x02157c20` | 21 | partial | string at `0x0216e208` |
| `scrcmd_sodateya.c` | `0x02157c20`–`0x021580c4` | 18 | complete | string at `0x0216e218` |
| `scrcmd_musical.c` | `0x021580c4`–`0x021590ec` | 27 | partial | string at `0x0216e22c` |
| `field_encount_st.c` | `0x021590ec`–`0x021593fc` | 15 | complete | string at `0x0216e240` |
| `hiden_event.c` | `0x021593fc`–`0x02159bc0` | 43 | complete | descriptive |
| `scrcmd_network.c` | `0x02159bc0`–`0x02159d54` | 9 | complete | string at `0x0216e254` |
| `scrcmd_stadium.c` | `0x02159d54`–`0x0215a0b0` | 9 | partial | descriptive |
| `event_battle_lose.c` | `0x0215a0b0`–`0x0215a19c` | 2 | complete | descriptive |
| `event_game_clear.c` | `0x0215a19c`–`0x0215a7a4` | 13 | complete | descriptive |
| `event_field_menu.c` | `0x0215a7a4`–`0x0215aa68` | 3 | complete | descriptive |
| `event_shortcut_menu.c` | `0x0215aa68`–`0x0215b488` | 17 | complete | string at `0x0216e268` |
| `event_field_proclink.c` | `0x0215b488`–`0x0215c4f8` | 37 | partial | string at `0x0216e280` |
| `event_save.c` | `0x0215c4f8`–`0x0215c6b0` | 5 | partial | descriptive |
| `event_action_call.c` | `0x0215c6b0`–`0x0215cb48` | 13 | complete | descriptive |
| `event_3d_demo.c` | `0x0215cb48`–`0x0215cd58` | 5 | partial | descriptive |
| `city_state.c` | `0x0215cd58`–`0x0215cda4` | 3 | complete | descriptive |
| `eventdata_system.c` | `0x0215cda4`–`0x0215dabc` | 61 | partial | string at `0x0216e298` |
| `field_actor_tool.c` | `0x0215dabc`–`0x0215ee10` | 69 | partial | descriptive |
| `zone_change.c` | `0x0215ee10`–`0x0215ef60` | 9 | complete | descriptive |
| `itemuse_event.c` | `0x0215ef60`–`0x0215f23c` | 13 | complete | descriptive |
| `hidden_item.c` | `0x0215f23c`–`0x0215f2d0` | 4 | complete | descriptive |
| `calender.c` | `0x0215f2d0`–`0x0215f440` | 11 | complete | string at `0x0216e2ac` |
| `scrcmd_ndemo.c` | `0x0215f440`–`0x0215f55c` | 5 | complete | descriptive |
| `game_beacon_search.c` | `0x0215f55c`–`0x0215f958` | 17 | partial | string at `0x0216e2b8` |
| `game_beacon_set.c` | `0x0215f958`–`0x02160668` | 86 | complete | descriptive |
| `symbol_map.c` | `0x02160668`–`0x021609b4` | 12 | complete | string at `0x0216e2d0` |
| `event_royal_unova.c` | `0x021609b4`–`0x02160b80` | 8 | complete | descriptive |
| `scrcmd_unity_tower.c` | `0x02160b80`–`0x02160dc8` | 7 | complete | descriptive |
| `building_enter_effect.c` | `0x02160dc8`–`0x02160eb4` | 3 | partial | descriptive |
| `townmap_util.c` | `0x02160eb4`–`0x02160ff4` | 2 | complete | descriptive |
| `scrcmd_phrase_select.c` | `0x02160ff4`–`0x021611e0` | 2 | complete | descriptive |
| `scrcmd_weather.c` | `0x021611e0`–`0x02161260` | 1 | complete | descriptive |
| `symbol_save_field.c` | `0x02161260`–`0x021613d0` | 8 | complete | descriptive |
| `comm_player.c` | `0x021613d0`–`0x02161844` | 12 | complete | string at `0x0216e2e4` |
| `bsubway_comm.c` | `0x02161844`–`0x02161c88` | 27 | complete | descriptive |
| `fld_btl_inst_event.c` | `0x02161c88`–`0x02161f6c` | 4 | complete | descriptive |
| `fld_btl_inst_tool.c` | `0x02161f6c`–`0x02162b64` | 22 | partial | string at `0x0216e36c` |
| `event_cgear_poweron.c` | `0x02162b64`–`0x02162c48` | 3 | complete | descriptive |
| `event_trial_house.c` | `0x02162c48`–`0x02162f44` | 6 | partial | descriptive |
| `ev_time.c` | `0x02162f44`–`0x021631c8` | 13 | complete | descriptive |
| `field_g3d_map.c` | `0x021631c8`–`0x02163b38` | 47 | partial | string at `0x0216e380` |
| `report_event.c` | `0x02163b38`–`0x02164330` | 20 | complete | string at `0x0216e390` |
| `shaymin_form.c` | `0x02164330`–`0x02164490` | 4 | complete | descriptive |
| `scrcmd_trial_house.c` | `0x02164490`–`0x02164838` | 16 | complete | descriptive |
| `scrcmd_actor_move.c` | `0x02164838`–`0x021649ec` | 4 | complete | descriptive |
| `season_form.c` | `0x021649ec`–`0x02164ae0` | 1 | partial | descriptive |
| `scrcmd_entree_forest.c` | `0x02164ae0`–`0x021652cc` | 16 | complete | descriptive |
| `survey.c` | `0x021652cc`–`0x02165598` | 14 | complete | descriptive |
| `scrcmd_sp_poke_gimmick.c` | `0x02165598`–`0x0216598c` | 8 | complete | descriptive |
| `oneshot_dr.c` | `0x0216598c`–`0x021659ec` | 2 | complete | descriptive |
| `namein_setup.c` | `0x021659ec`–`0x02165b1c` | 7 | complete | string at `0x0216e3a0` |
| `event_league_lift.c` | `0x02165b1c`–`0x02165eb8` | 7 | partial | descriptive |
| `event_bsubway.c` | `0x02165eb8`–`0x02166664` | 17 | partial | descriptive |
| `fldmmdl.c` | `0x02166664`–`0x02168320` | 270 | complete | string at `0x0216e5c0` |
| `fest_mission_field.c` | `0x02168320`–`0x02168468` | 5 | complete | descriptive |
| `burmy_form.c` | `0x02168468`–`0x021684bc` | 1 | complete | descriptive |
| `event_battle.c` | `0x021684bc`–`0x0216919c` | 26 | complete | string at `0x0216e5dc` |
| `trcard_sys.c` | `0x0216919c`–`0x02169c1c` | 23 | complete | string at `0x0216e5ec` |
| `scrcmd_sp_poke.c` | `0x02169c1c`–`0x02169c7c` | 4 | complete | descriptive |
| `waza_oshie.c` | `0x02169c7c`–`0x02169e18` | 4 | partial | string at `0x0216e5fc` |
| `g3d_text_draw.c` | `0x02169e18`–`0x0216a190` | 7 | partial | descriptive |
| `pass_power_check.c` | `0x0216a190`–`0x0216a23c` | 2 | partial | descriptive |
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

### Overlay 21

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_festival.c` | `0x0216e660`–`0x0216e884` | 3 | complete | descriptive |

### Overlay 27

42 of 42 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `scrcmd_survey.c` | `0x02170340`–`0x02170ac4` | 31 | complete | descriptive |
| `fest_mission_data.c` | `0x02170ac4`–`0x02170e40` | 11 | complete | string at `0x021711e0` |

### Overlay 33

238 of 238 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `entree_forest.c` | `0x02176b00`–`0x02176d88` | 5 | partial | descriptive |
| `event_funfest_mission.c` | `0x02176d88`–`0x02176ec4` | 2 | complete | descriptive |
| `entree_forest_first_warp.c` | `0x02176ec4`–`0x02176f90` | 1 | complete | descriptive |
| `scrcmd_funfest.c` | `0x02176f90`–`0x02177370` | 13 | complete | descriptive |
| `entralink_warp.c` | `0x02177370`–`0x02177574` | 3 | complete | descriptive |
| `event_cgear_shutdown.c` | `0x02177574`–`0x021775e8` | 2 | complete | descriptive |
| `event_phrase_input.c` | `0x021775e8`–`0x02177998` | 6 | complete | descriptive |
| `pdw_postman.c` | `0x02177998`–`0x02178448` | 47 | partial | string at `0x0217c600` |
| `scrcmd_field_move.c` | `0x02178448`–`0x021785c4` | 6 | complete | descriptive |
| `event_sweet_scent.c` | `0x021785c4`–`0x02178908` | 8 | complete | descriptive |
| `event_fly.c` | `0x02178908`–`0x02178ca8` | 3 | partial | descriptive |
| `event_chatot.c` | `0x02178ca8`–`0x021791c0` | 10 | partial | descriptive |
| `event_fishing.c` | `0x021791c0`–`0x02179664` | 9 | partial | descriptive |
| `event_dendou_machine.c` | `0x02179664`–`0x02179868` | 8 | complete | descriptive |
| `event_pc.c` | `0x02179868`–`0x02179b54` | 8 | complete | descriptive |
| `event_pokemon_center.c` | `0x02179b54`–`0x02179dd4` | 9 | complete | descriptive |
| `event_game_manual.c` | `0x02179dd4`–`0x02179e98` | 3 | complete | string at `0x0217c610` |
| `event_abyssal_ruins.c` | `0x02179e98`–`0x02179f04` | 1 | complete | descriptive |
| `event_dive.c` | `0x02179f04`–`0x0217a1b0` | 7 | complete | descriptive |
| `field_actor_animation.c` | `0x0217a1b0`–`0x0217a4a0` | 12 | complete | descriptive |
| `fld_trade.c` | `0x0217a4a0`–`0x0217aa1c` | 11 | partial | string at `0x0217c624` |
| `unity_visitor.c` | `0x0217aa1c`–`0x0217ac70` | 6 | complete | descriptive |
| `trial_house.c` | `0x0217ac70`–`0x0217b468` | 19 | partial | string at `0x0217c630` |
| `bsubway_scr.c` | `0x0217b468`–`0x0217c340` | 39 | partial | string at `0x0217c640` |

### Overlay 35

109 of 109 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `event_mapchange.c` | `0x0217c940`–`0x0217ed70` | 78 | complete | string at `0x0217f5c0` |
| `el_scoreboard.c` | `0x0217ed70`–`0x0217ee5c` | 4 | complete | string at `0x0217f5e4` |
| `event_season_banner.c` | `0x0217ee5c`–`0x0217f52c` | 27 | complete | descriptive |

### Overlay 36

364 of 4339 functions are in source files. Embedded names without a file yet: `comm_entry_menu.c`, `ctalkmsgwin.c`, `effect_encount.c`, `enceff.c`, `entrance_camera.c`, `event_trainer_eye.c`, `fes_gimmick.c`, `field_3dbg.c`, `field_bbd_color.c`, `field_camera.c`, `field_camera_anime.c`, `field_comm_actor.c`, `field_effect.c`, `field_encount.c`, `field_fade.c`, `field_flash.c`, `field_fmission_info.c`, `field_fog.c`, `field_g3dobj.c`, `field_ground_anime.c`, `field_light.c`, `field_menu.c`, `field_msgbg.c`, `field_nogrid_mapper.c`, `field_place_name.c`, `field_player.c`, `field_player_core.c`, `field_player_grid.c`, `field_player_grid_event.c`, `field_player_nogrid.c`, `field_rail.c`, `field_rail_loader.c`, `field_saveanime.c`, `field_subscreen.c`, `field_task.c`, `field_task_manager.c`, `field_wfbc.c`, `field_zonefog.c`, `fieldmap_ctrl_grid.c`, `fieldmap_ctrl_nogrid_work.c`, `fieldmap_func.c`, `fieldmap_tcb_camera_zoom.c`, `fieldskill_mapeff.c`, `fld3d_ci.c`, `fld_exp_obj.c`, `fld_particle.c`, `fld_scenearea.c`, `fld_scenearea_loader.c`, `fld_wipe_3dobj.c`, `fldeff_btrain.c`, `fldeff_bubble.c`, `fldeff_d06denki.c`, `fldeff_encount.c`, `fldeff_fes_kira.c`, `fldeff_fishing.c`, `fldeff_footmark.c`, `fldeff_grass.c`, `fldeff_gyoe.c`, `fldeff_hide.c`, `fldeff_iaigiri.c`, `fldeff_iwakudaki.c`, `fldeff_kemuri.c`, `fldeff_namipoke.c`, `fldeff_ochiba.c`, `fldeff_reflect.c`, `fldeff_ripple.c`, `fldeff_shadow.c`, `fldeff_splash.c`, `fldmmdl_acmd.c`, `fldmmdl_blact.c`, `fldmmdl_g3dobj.c`, `height_ex.c`, `ircbattlemenu.c`, `land_data_patch.c`, `party_select_list.c`, `place_name_letter.c`, `pleasure_boat.c`, `rail_attr.c`, `scrcmd_encount.c`, `scrcmd_fld_battle.c`, `scrcmd_fldmmdl.c`, `scrcmd_pokemon_fld.c`, `scrcmd_proc_fld.c`, `scrcmd_shop.c`, `shortcut_menu.c`, `sodateya.c`, `sound_obj.c`, `weather.c`, `weather_task.c`.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `fieldmap.c` | `0x0217f600`–`0x021814dc` | 128 | partial | string at `0x021d4b20` |
| `field_buildmodel.c` | `0x0218304c`–`0x02184244` | 85 | partial | string at `0x021d4b2c` |
| `field_g3d_mapper.c` | `0x02184244`–`0x021856a0` | 57 | partial | string at `0x021d4b4c` |
| `fieldmap_ctrl_hybrid.c` | `0x0219e430`–`0x0219e9d0` | 13 | complete | string at `0x021d4f44` |
| `scrcmd_medal.c` | `0x021c7b38`–`0x021c7fd8` | 13 | partial | descriptive |
| `fld_vreq.c` | `0x021c7fd8`–`0x021c81f4` | 14 | complete | string at `0x021d56f0` |
| `field_palace_sys.c` | `0x021c81f4`–`0x021c83e8` | 6 | complete | string at `0x021d56fc` |
| `field_goout_effect.c` | `0x021c83e8`–`0x021c8808` | 11 | partial | string at `0x021d5710` |
| `field_goout_effect_data.c` | `0x021c8808`–`0x021c8954` | 9 | complete | string at `0x021d5728` |
| `resort_mapcreate.c` | `0x021c8954`–`0x021c8c34` | 6 | partial | string at `0x021d5744` |
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
| `scrcmd_resort.c` | `0x021e58c0`–`0x021e7c70` | 43 | partial | string at `0x021e7de0` |
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

### Overlay 95

3 of 3 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gym_striaton.c` | `0x021eec80`–`0x021eecbc` | 3 | complete | descriptive |

### Overlay 103

17 of 17 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `gimmick_badge_gate.c` | `0x021eec80`–`0x021ef794` | 17 | partial | descriptive |

### Overlay 104

67 of 67 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `field_gimmick_gate.c` | `0x021eec80`–`0x021efbd8` | 48 | partial | string at `0x021f078c` |
| `gimmick_obj_elboard.c` | `0x021efbd8`–`0x021f039c` | 19 | partial | string at `0x021f07a4` |

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

### Overlay 126

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `underwater_effect.c` | `0x021eec80`–`0x021eee3c` | 4 | complete | descriptive |

### Overlay 137

200 of 261 functions are in source files. Embedded names without a file yet: `resort_sys.c`.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `resort_field.c` | `0x021eec80`–`0x021f0e98` | 113 | complete | string at `0x021f58a0` |
| `resort_people.c` | `0x021f0e98`–`0x021f1710` | 46 | complete | string at `0x021f58b0` |
| `resort_data_manager.c` | `0x021f1710`–`0x021f1c24` | 30 | complete | string at `0x021f58c0` |
| `resort_npc.c` | `0x021f1c24`–`0x021f1f1c` | 11 | complete | string at `0x021f58d8` |

### Overlay 144

4 of 107 functions are in source files. Embedded names without a file yet: `townmap.c`, `townmap_grh.c`.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `townmap_data.c` | `0x0219f718`–`0x0219f76c` | 4 | complete | descriptive |

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

### Overlay 167

2504 of 4223 functions are in source files. Embedded names without a file yet: `btl_adapter.c`, `btl_client.c`, `btl_field.c`, `btl_net.c`, `btl_rec.c`, `btlv_scu.c`, `pokewood_cutin.c`.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `btl_main.c` | `0x021998c0`–`0x0219e3cc` | 231 | partial | string at `0x021dae80` |
| `btl_server.c` | `0x0219e3cc`–`0x0219f390` | 55 | complete | string at `0x021dae8c` |
| `btl_server_flow.c` | `0x0219f390`–`0x021ae32c` | 997 | partial | string at `0x021dae9c` |
| `btl_server_flow_sub.c` | `0x021ae32c`–`0x021b083c` | 151 | partial | descriptive |
| `btl_handler_work.c` | `0x021b083c`–`0x021b0a1c` | 11 | partial | descriptive |
| `btl_server_cmd.c` | `0x021b0a1c`–`0x021b1674` | 19 | partial | descriptive |
| `btl_pokeparam.c` | `0x021ba584`–`0x021bc6bc` | 154 | partial | string at `0x021daf7c` |
| `battle_event.c` | `0x021bc6bc`–`0x021bd054` | 46 | partial | descriptive |
| `btl_calc.c` | `0x021bd054`–`0x021bdaf8` | 58 | complete | string at `0x021daf94` |
| `battle_action.c` | `0x021bdaf8`–`0x021bdcac` | 17 | complete | descriptive |
| `ability_handlers.c` | `0x021bdcac`–`0x021c26ec` | 442 | partial | descriptive |
| `battle_condition.c` | `0x021ce158`–`0x021ce520` | 20 | complete | descriptive |
| `poke_type_pair.c` | `0x021ce520`–`0x021ce604` | 8 | complete | descriptive |
| `btlv_core.c` | `0x021ce604`–`0x021d0c24` | 295 | partial | string at `0x021dafa0` |

### Overlay 170

161 of 161 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `tr_ai.c` | `0x0217f600`–`0x02181c74` | 161 | complete | string at `0x02181ea0` |

### Overlay 279

4 of 4 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `new_game.c` | `0x021e8be0`–`0x021e8c74` | 4 | complete | descriptive |

### Overlay 284

151 of 151 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `shinka_demo.c` | `0x021e30e0`–`0x021e49b0` | 44 | complete | string at `0x021e80a0` |
| `shinka_demo_graphic.c` | `0x021e49b0`–`0x021e4dd0` | 23 | complete | string at `0x021e80b0` |
| `shinka_demo_view.c` | `0x021e4dd0`–`0x021e7824` | 56 | partial | string at `0x021e80c8` |
| `shinka_demo_effect.c` | `0x021e7824`–`0x021e7d9c` | 28 | complete | string at `0x021e80dc` |

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

### Overlay 307

97 of 97 functions are in source files.

| File | `.text` (Black 2) | Functions | Status | Name |
| --- | --- | --- | --- | --- |
| `egg_demo.c` | `0x021ddbc0`–`0x021de730` | 38 | complete | descriptive |
| `egg_demo_graphic.c` | `0x021de730`–`0x021dead0` | 21 | complete | string at `0x021df7e0` |
| `egg_demo_view.c` | `0x021dead0`–`0x021df528` | 38 | complete | string at `0x021df7f4` |
