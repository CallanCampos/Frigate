// The script VM's own commands (0x00 to 0x32), the readers of their arguments, and the table of every field script
// command (a descriptive name). Function and data names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0)
//
// The tables' place here is inferred from MWCC's size sort: VM_CMP_LUT is read only here, the larger tables after it
// can't be script_plugin.c's, which starts with 2-byte tables, and EVCMD_MAX counts EVCMD_TABLE

#include "types.h"
#include "field/entree_scripts.h"
#include "field/event_phrase_input.h"
#include "field/event_sound.h"
#include "field/field_actor_animation.h"
#include "field/field_gimmick_gate.h"
#include "field/field_move_scripts.h"
#include "field/field_script.h"
#include "field/field_sound.h"
#include "field/field_status.h"
#include "field/fld_wipe_3dobj.h"
#include "field/funfest_scripts.h"
#include "field/hidden_hollow.h"
#include "field/medal.h"
#include "field/pair_sys.h"
#include "field/pdw_postman.h"
#include "field/player_state.h"
#include "field/pleasure_boat.h"
#include "field/scrcmd_actor_move.h"
#include "field/scrcmd_buildmodel.h"
#include "field/scrcmd_camera.h"
#include "field/scrcmd_download_data.h"
#include "field/scrcmd_encount.h"
#include "field/scrcmd_entree_forest.h"
#include "field/scrcmd_fade.h"
#include "field/scrcmd_field.h"
#include "field/scrcmd_field_actor.h"
#include "field/scrcmd_fld_battle.h"
#include "field/scrcmd_game_state.h"
#include "field/scrcmd_gym.h"
#include "field/scrcmd_hidden_hollow.h"
#include "field/scrcmd_hollow_rival.h"
#include "field/scrcmd_item.h"
#include "field/scrcmd_join_avenue_store.h"
#include "field/scrcmd_keysystem.h"
#include "field/scrcmd_mapchange.h"
#include "field/scrcmd_menu.h"
#include "field/scrcmd_misc.h"
#include "field/scrcmd_msg.h"
#include "field/scrcmd_musical.h"
#include "field/scrcmd_net.h"
#include "field/scrcmd_pedometer.h"
#include "field/scrcmd_pep_quiz.h"
#include "field/scrcmd_place_name.h"
#include "field/scrcmd_poke_disp.h"
#include "field/scrcmd_pokemon.h"
#include "field/scrcmd_pokemon_misc.h"
#include "field/scrcmd_proc.h"
#include "field/scrcmd_proc_fld.h"
#include "field/scrcmd_save.h"
#include "field/scrcmd_shop.h"
#include "field/scrcmd_sound.h"
#include "field/scrcmd_sp_poke.h"
#include "field/scrcmd_sp_poke_gimmick.h"
#include "field/scrcmd_trainer.h"
#include "field/scrcmd_trial_house.h"
#include "field/scrcmd_word.h"
#include "field/script_network.h"
#include "field/stadium_script.h"
#include "field/survey.h"
#include "field/unity_tower.h"
#include "gfl/key.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

// Whether a comparison's result meets each condition of the conditional jumps and calls, by VMCmp's result (less,
// equal, greater)
static const u8 VM_CMP_LUT[][3] = {
    { 1, 0, 0 }, // less
    { 0, 1, 0 }, // equal
    { 0, 0, 1 }, // greater
    { 1, 1, 0 }, // less or equal
    { 0, 1, 1 }, // greater or equal
    { 1, 0, 1 }, // not equal
};

// What finishes each of the script's sub events when it ends, by the IDs of FieldScriptSubEvent_Register
const FieldScriptSubEventFinishFunc FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS[SCRIPT_SUB_EVENT_MAX] = {
    FieldScriptSubEventFinish_EvCamera,   FieldScriptSubEventFinish_SystemMsg,   FieldScriptSubEventFinish_ActorMsg,
    FieldScriptSubEventFinish_InfoMsg,    FieldScriptSubEventFinish_Sign,        FieldScriptSubEventFinish_MultiMsg,
    FieldScriptSubEventFinish_CheckerMsg, FieldScriptSubEventFinish_NDemo,       FieldScriptSubEventFinish_Subscreen,
    ActorMsgWin_CleanupAutoMode,          FieldScriptSubEventFinish_MEPlayback,  GameCommPause_Finish,
    ScriptWork_RestoreBgmVolumeEvent,     FieldScriptSubEventFinish_BGMPlayback, NULL,
};

// The field script commands, by their IDs. An ID with no command is NULL: EVCMD_PERM_TABLE allows it at no level, so
// FieldScript_OpcodeGuard stops the script before the VM would call it
const FieldScriptCommand EVCMD_TABLE[] = {
    s0000_VMNop,                                      // 0x000
    s0001_VMNop2,                                     // 0x001
    s0002_VMHalt,                                     // 0x002
    s0003_VMSleep,                                    // 0x003
    s0004_VMCall,                                     // 0x004
    s0005_VMReturn,                                   // 0x005
    s0006_DebugPrint,                                 // 0x006
    s0007_DebugStack,                                 // 0x007
    s0008_VMStackPushConst,                           // 0x008
    s0009_VMStackPush,                                // 0x009
    s000A_VMStackPop,                                 // 0x00a
    s000B_VMStackDiscard,                             // 0x00b
    s000C_VMStackAdd,                                 // 0x00c
    s000D_VMStackSub,                                 // 0x00d
    s000E_VMStackMul,                                 // 0x00e
    s000F_VMStackDiv,                                 // 0x00f
    s0010_VMStackPushFlag,                            // 0x010
    s0011_VMStackCmp,                                 // 0x011
    s0012_WorkAnd,                                    // 0x012
    s0013_WorkOr,                                     // 0x013
    s0014_VMRegSet8,                                  // 0x014
    s0015_VMRegSet32,                                 // 0x015
    s0016_VMRegMov,                                   // 0x016
    s0017_VMRegCmp8,                                  // 0x017
    s0018_VMRegCmpConst8,                             // 0x018
    s0019_WorkCmpConst,                               // 0x019
    s001A_WorkCmpWork,                                // 0x01a
    s001B_RTCallGlobalAsync,                          // 0x01b
    s001C_RTCallGlobal,                               // 0x01c
    s001D_RTEndGlobal,                                // 0x01d
    s001E_VMJump,                                     // 0x01e
    s001F_VMJumpIf,                                   // 0x01f
    s0020_VMCallIf,                                   // 0x020
    s0021_RTReserveScript,                            // 0x021
    s0022_FieldGetContinueFlag,                       // 0x022
    s0023_FlagSet,                                    // 0x023
    s0024_FlagReset,                                  // 0x024
    s0025_FlagGet,                                    // 0x025
    s0026_WorkAdd,                                    // 0x026
    s0027_WorkSub,                                    // 0x027
    s0028_WorkSetConst,                               // 0x028
    s0029_WorkGet,                                    // 0x029
    s002A_WorkSet,                                    // 0x02a
    s002B_WorkMul,                                    // 0x02b
    s002C_WorkDiv,                                    // 0x02c
    s002D_WorkMod,                                    // 0x02d
    s002E_ActorsPauseAll,                             // 0x02e
    s002F_ActorsUnpauseAll,                           // 0x02f
    s0030_FinishAllEvents,                            // 0x030
    s0031_ABKeyWait,                                  // 0x031
    s0032_LastKeyWait,                                // 0x032
    s0033_MsgSetAutoscrolls,                          // 0x033
    s0034_SystemMsg,                                  // 0x034
    s0035_SystemMsgAsync,                             // 0x035
    s0036_InfoMsgClose,                               // 0x036
    s0037_MsgSetLoadingSpinner,                       // 0x037
    s0038_InfoMsg,                                    // 0x038
    s0039_InfoMsgClose,                               // 0x039
    s003A_MultiMsg,                                   // 0x03a
    s003B_MsgWinCloseNo,                              // 0x03b
    s003C_ActorMsg,                                   // 0x03c
    s003D_ParentActorMsg,                             // 0x03d
    s003E_ActorMsgClose,                              // 0x03e
    s003F_MsgWinCloseAll,                             // 0x03f
    s0040_MoneyWinDisp,                               // 0x040
    s0041_MoneyWinClose,                              // 0x041
    s0042_MoneyWinUpdate,                             // 0x042
    s0043_MsgPlaceSign,                               // 0x043
    s0044_MsgPlaceSignClose,                          // 0x044
    s0045_CheckerMsg,                                 // 0x045
    s0046_CheckerMsgClose,                            // 0x046
    s0047_YesNoWin,                                   // 0x047
    s0048_ActorMsgGendered,                           // 0x048
    s0049_ActorMsgVersioned,                          // 0x049
    s004A_ScreamMsg,                                  // 0x04a
    s004B_MsgWaitAdvance,                             // 0x04b
    s004C_WordSetPlayerName,                          // 0x04c
    s004D_WordSetItemName,                            // 0x04d
    s004E_WordSetItemNameEx,                          // 0x04e
    s004F_WordSetItemNameWithArticle,                 // 0x04f
    s0050_WordSetTMMoveName,                          // 0x050
    s0051_WordSetMoveName,                            // 0x051
    s0052_WordSetItemPocketName,                      // 0x052
    s0053_WordSetPartyPokeSpecies,                    // 0x053
    s0054_WordSetPartyPokeName,                       // 0x054
    s0055_WordSetDaycarePokeSpecies,                  // 0x055
    s0056_WordSetPokeTypeName,                        // 0x056
    s0057_WordSetPokeSpecies,                         // 0x057
    s0058_WordSetPokeSpeciesWithArticle,              // 0x058
    s0059_WordSetPlaceName,                           // 0x059
    s005A_WordSetTrendName,                           // 0x05a
    s005B_WordSetDaycarePokeName,                     // 0x05b
    s005C_WordSetNumber,                              // 0x05c
    s005D_WordSetMusicalInfo,                         // 0x05d
    s005E_WordSetCountry,                             // 0x05e
    s005F_WordSetHobbyName,                           // 0x05f
    s0060_WordSetPassPowerName,                       // 0x060
    s0061_WordSetTrainerClassName,                    // 0x061
    s0062_WordSetTrainerClassNameWithArticle,         // 0x062
    s0063_WordSetSurveyAnswer,                        // 0x063
    s0064_ActorCmdExec,                               // 0x064
    s0065_ActorCmdWait,                               // 0x065
    s0066_ActorGetMoveCode,                           // 0x066
    s0067_ActorGetGPos,                               // 0x067
    s0068_PlayerGetGPos,                              // 0x068
    s0069_ActorNew,                                   // 0x069
    s006A_ActorGetSpawnFlag,                          // 0x06a
    s006B_ActorAdd,                                   // 0x06b
    s006C_ActorDelete,                                // 0x06c
    s006D_ActorSetGPos,                               // 0x06d
    s006E_PlayerGetDir,                               // 0x06e
    s006F_PlayerGetActorInFront,                      // 0x06f
    s0070_ActorFindByGPos,                            // 0x070
    s0071_PlayerGetRailPos,                           // 0x071
    s0072_ActorGetRailPos,                            // 0x072
    s0073_ActorSetMoveCode,                           // 0x073
    s0074_ActorSetEyeToEye,                           // 0x074
    s0075_PlayerSetSpecialSequence,                   // 0x075
    s0076_PlayerMoveToYAsync,                         // 0x076
    s0077_PlayerTurnByTrigger,                        // 0x077
    s0078_PlayerGetExState,                           // 0x078
    s0079_ActorGetUserParam,                          // 0x079
    s007A_ActorPlayRailSlipdown,                      // 0x07a
    s007B_ActorJumpToGPos,                            // 0x07b
    s007C_PlayerSetRailPos,                           // 0x07c
    s007D_ActorSetRailPos,                            // 0x07d
    s007E_ActorPlayTeleportSeq,                       // 0x07e
    s007F_TrainerEyeGetTrainerID,                     // 0x07f
    s0080_TrainerEyeEventInit,                        // 0x080
    s0081_TrainerEyeEventStart,                       // 0x081
    s0082_TrainerGetActorID,                          // 0x082
    s0083_TrainerGetBattleType,                       // 0x083
    s0084_ActorGetTrainerID,                          // 0x084
    s0085_CallTrainerBattle,                          // 0x085
    s0085_CallTrainerMultiBattle,                     // 0x086, which swan's name numbers 0x085
    s0087_TrainerSayMessage,                          // 0x087
    s0088_TrainerGetMessageTypes,                     // 0x088
    NULL,                                             // 0x089
    s008A_TrainerGetBattleType,                       // 0x08a
    s008B_TrainerBGMPlayPush,                         // 0x08b
    s008C_CallTrainerLose,                            // 0x08c
    s008D_TrainerBattleIsVictory,                     // 0x08d
    s008E_CallTrainerBattleEnd,                       // 0x08e
    s008F_TrainerClassBGMPlayPush,                    // 0x08f
    s0090_TrainerBGMPlay,                             // 0x090
    NULL,                                             // 0x091
    s0092_TrainerGetFieldAction,                      // 0x092
    s0093_TrainerGetPrizeItem,                        // 0x093
    s0094_CallTradedPokemonBattle,                    // 0x094
    s0095_TrainerFlagSet,                             // 0x095
    s0096_TrainerFlagReset,                           // 0x096
    s0097_TrainerFlagGet,                             // 0x097
    s0098_BGMPlay,                                    // 0x098
    NULL,                                             // 0x099
    NULL,                                             // 0x09a
    s009B_BGMIsPlaying,                               // 0x09b
    NULL,                                             // 0x09c
    NULL,                                             // 0x09d
    s009E_BGMChangeMap,                               // 0x09e
    s009F_BGMPlayPush,                                // 0x09f
    s00A0_BGMWait,                                    // 0x0a0
    s00A1_BGMPush,                                    // 0x0a1
    s00A2_BGMPop,                                     // 0x0a2
    s00A3_ISSSwitchEnable,                            // 0x0a3
    s00A4_ISSSwitchDisable,                           // 0x0a4
    s00A5_ISSSwitchQuery,                             // 0x0a5
    s00A6_SEPlay,                                     // 0x0a6
    s00A7_SEStop,                                     // 0x0a7
    s00A8_SEWait,                                     // 0x0a8
    s00A9_MEPlay,                                     // 0x0a9
    s00AA_MEWait,                                     // 0x0aa
    s00AB_PVPlay,                                     // 0x0ab
    s00AC_PVWait,                                     // 0x0ac
    s00AD_ListMenuInitCommon,                         // 0x0ad
    s00AE_ListMenu_AnchorTopLeft,                     // 0x0ae
    s00AF_ListMenuAdd,                                // 0x0af
    s00B0_ListMenuShow,                               // 0x0b0
    s00B1_ListMenuShow2,                              // 0x0b1
    s00B2_ListMenu_AnchorTopRight,                    // 0x0b2
    s00B3_FadeEx,                                     // 0x0b3
    s00B4_FadeExWait,                                 // 0x0b4
    s00B5_ItemAdd,                                    // 0x0b5
    s00B6_ItemSub,                                    // 0x0b6
    s00B7_ItemCheckSpace,                             // 0x0b7
    s00B8_ItemCheckAmount,                            // 0x0b8
    s00B9_ItemGetCount,                               // 0x0b9
    s00BA_ItemIsTMHM,                                 // 0x0ba
    s00BB_ItemGetPocket,                              // 0x0bb
    s00BC_PhenomenonGetItemID,                        // 0x0bc
    s00BD_ItemGetClass,                               // 0x0bd
    s00BE_MapChangeFake,                              // 0x0be
    s00BF_MapChangeWarpPad,                           // 0x0bf
    s00C0_MapChangeWarpRail,                          // 0x0c0
    s00C1_MapChangeQuicksand,                         // 0x0c1
    s00C2_MapChangeWarp,                              // 0x0c2
    s00C3_MapChangeUnionRoom,                         // 0x0c3
    s00C4_MapChangeCore,                              // 0x0c4
    s00C5_CallSurf,                                   // 0x0c5
    s00C6_CallWaterfall,                              // 0x0c6
    s00C7_CallCut,                                    // 0x0c7
    s00C8_CallDiving,                                 // 0x0c8
    s00C9_ConfigToggleKanji,                          // 0x0c9
    s00CA_ConfigGetKanji,                             // 0x0ca
    s00CB_Random,                                     // 0x0cb
    s00CC_RTGetTextFile,                              // 0x0cc
    s00CD_RTCGetDayPart,                              // 0x0cd
    s00CE_TrainerCardGetStars,                        // 0x0ce
    s00CF_RTCGetWeekDay,                              // 0x0cf
    s00D0_RTCGetDate,                                 // 0x0d0
    s00D1_RTCGetTime,                                 // 0x0d1
    s00D2_RTCGetSeason,                               // 0x0d2
    s00D3_RTGetZoneID,                                // 0x0d3
    s00D4_TrainerCardGetBirthDate,                    // 0x0d4
    s00D5_TrainerCardHasBadge,                        // 0x0d5
    s00D6_TrainerCardAddBadge,                        // 0x0d6
    s00D7_TrainerCardGetBadgeCount,                   // 0x0d7
    s00D8_MapReplaceIsEventSet,                       // 0x0d8
    s00D9_FieldSetTeleportZone,                       // 0x0d9
    s00DA_MapReplaceSetEvent,                         // 0x0da
    s00DB_FieldSetNextZoneHere,                       // 0x0db
    s00DC_FieldSetNextZone,                           // 0x0dc
    s00DD_PokeDexGetCount,                            // 0x0dd
    s00DE_PokeDexRegist,                              // 0x0de
    s00DF_PokeDexIsRegist,                            // 0x0df
    s00E0_GameGetVersion,                             // 0x0e0
    s00E1_TrainerCardGetSex,                          // 0x0e1
    s00E2_SaveDataCheckRequired,                      // 0x0e2
    s00E3_GiveRunningShoes,                           // 0x0e3
    s00E4_BoxUnlockWallpapers,                        // 0x0e4
    FieldScriptEnv_CmdGetTrainerViewGroupAndMarkSeen, // 0x0e5
    s00E6_StartMenuIsItemUnlocked,                    // 0x0e6
    s00E7_StartMenuUnlockItem,                        // 0x0e7
    s00E8_StartMenuIsItemSelected,                    // 0x0e8
    NULL,                                             // 0x0e9
    s00EA_HOFCheckIntegrity,                          // 0x0ea
    s00EB_DayCareCheckSpawnFlag,                      // 0x0eb
    s00EC_DayCareBreed,                               // 0x0ec
    s00ED_DayCareResetSeed,                           // 0x0ed
    s00EE_DayCareGetPkmCount,                         // 0x0ee
    s00EF_DayCareCalcEggSpawnChance,                  // 0x0ef
    s00F0_DayCareDeposit,                             // 0x0f0
    s00F1_DayCareWithdraw,                            // 0x0f1
    s00F2_DayCareGetSpecies,                          // 0x0f2
    s00F3_DayCareGetForme,                            // 0x0f3
    s00F4_DayCareCalcNewLevel,                        // 0x0f4
    s00F5_DayCareCalcLevelGain,                       // 0x0f5
    s00F6_DayCareCalcWithdrawCost,                    // 0x0f6
    s00F7_DayCareCallPokeSelect,                      // 0x0f7
    s00F8_DayCareGetSex,                              // 0x0f8
    s00F9_MoneyAdd,                                   // 0x0f9
    s00FA_MoneySub,                                   // 0x0fa
    s00FB_MoneyCheck,                                 // 0x0fb
    s00FC_PokePartyGetHappiness,                      // 0x0fc
    s00FD_PokePartyAdjustHappiness,                   // 0x0fd
    s00FE_PokePartyGetSpecies,                        // 0x0fe
    s00FF_PokePartyGetForme,                          // 0x0ff
    s0100_PokePartyCheckPokerus,                      // 0x100
    s0101_PokePartyIsFullHP,                          // 0x101
    s0102_PokePartyIsEgg,                             // 0x102
    s0103_PokePartyGetCount,                          // 0x103
    s0104_PokePartyRecoverAll,                        // 0x104
    s0105_CallPokeNameInput,                          // 0x105
    s0106_CallEggHatch,                               // 0x106
    s0107_CallPokeSelect,                             // 0x107
    s0108_PokePartyGetMoveCount,                      // 0x108
    s0109_CallPokeMoveReplace,                        // 0x109
    s010A_PokePartyGetMove,                           // 0x10a
    s010B_PokePartyLearnMove,                         // 0x10b
    s010C_PokePartyAdd,                               // 0x10c
    s010D_PokePartyGetMemberByType,                   // 0x10d
    s010E_PokePartyAddEx,                             // 0x10e
    s010F_PokePartyAddEgg,                            // 0x10f
    s0110_PokePartyGetParam,                          // 0x110
    s0111_PokePartySetIV,                             // 0x111
    s0112_PokePartyGetEVTotal,                        // 0x112
    s0113_PokePartyIsOriginGame,                      // 0x113
    s0114_PokePartyGetCountBySpecies,                 // 0x114
    s0115_PokePartyHasMove,                           // 0x115
    s0116_PokePartyHasMoveAny,                        // 0x116
    s0117_PokePartySetForme,                          // 0x117
    s0118_PokePartyFindBySpecies,                     // 0x118
    s0119_PokePartyIsFromWhiteForest,                 // 0x119
    s011A_PokePartyGetMetDate,                        // 0x11a
    s011B_PokePartyGetTypes,                          // 0x11b
    s011C_PokePartyChangeRotomForme,                  // 0x11c
    s011D_TrainerCardSetFavePokemon,                  // 0x11d
    s011E_TrainerCardSaveGymVictoryParty,             // 0x11e
    s011F_WordSetGymVictoryParty,                     // 0x11f
    s0120_FieldTradeSavePokemon,                      // 0x120
    s0121_BoxGetCount,                                // 0x121
    s0122_BoxAdd,                                     // 0x122
    s0123_BoxAddEx,                                   // 0x123
    s0124_BMHndAnmPlay,                               // 0x124
    s0125_BMPlayHOFMachineSeq,                        // 0x125
    s0126_BMChangeMdlID,                              // 0x126
    s0127_BMCreateHandleByGPos,                       // 0x127
    s0128_BMReleaseHandle,                            // 0x128
    s0129_BMHndAudioVisualAnmPlay,                    // 0x129
    s012A_BMHndAnmWait,                               // 0x12a
    s012B_BMAnmPlayInv,                               // 0x12b
    s012C_BMHndAnmPause,                              // 0x12c
    s012D_BMSetVisible,                               // 0x12d
    s012E_BMAnmPlayLoop,                              // 0x12e
    s012F_PokecenPlayHealingSequence,                 // 0x12f
    s0130_PokecenPCOpen,                              // 0x130
    s0131_PokecenPCIdle,                              // 0x131
    s0132_PokecenPCClose,                             // 0x132
    NULL,                                             // 0x133
    s0134_CasteliaRushInit,                           // 0x134
    NULL,                                             // 0x135
    s0136_FieldSetWeather,                            // 0x136
    s0137_SaveDataWrite,                              // 0x137
    s0138_SaveDataGetStatus,                          // 0x138
    s0139_GameCommDisconnect,                         // 0x139
    s013A_GameCommGetStatus,                          // 0x13a
    s013B_GameCommCheckDSiWiFi,                       // 0x13b
    s013C_GameCommTryBoot,                            // 0x13c
    s013D_GameCommPause,                              // 0x13d
    s013E_GameCommResume,                             // 0x13e
    s013F_EvCameraInit,                               // 0x13f
    s0140_EvCameraEnd,                                // 0x140
    s0141_EvCameraUnbind,                             // 0x141
    s0142_EvCameraRebind,                             // 0x142
    s0143_EvCameraMoveTo,                             // 0x143
    s0144_EvCameraReturn,                             // 0x144
    s0145_EvCameraWait,                               // 0x145
    s0146_EvCameraMoveToCommon,                       // 0x146
    s0147_EvCameraMoveToDefault,                      // 0x147
    s0148_EvCameraShake,                              // 0x148
    s0149_CallFriendlyShopBuy,                        // 0x149
    s014A_FieldOpen,                                  // 0x14a
    s014B_FieldClose,                                 // 0x14b
    s014C_RTFreeUserHeap,                             // 0x14c
    s014D_CallRecordSystem,                           // 0x14d
    s014E_CallBag,                                    // 0x14e
    s014F_CallPC,                                     // 0x14f
    s0150_CallMailbox,                                // 0x150
    s0151_CallPokedexDiploma,                         // 0x151
    s0152_CallGeonet,                                 // 0x152
    s0153_callPoke3Select,                            // 0x153
    s0154_Call3DDemo,                                 // 0x154
    s0155_CallXTransceiver,                           // 0x155
    s0156_CallGameClear,                              // 0x156
    s0157_ActorAnimationInit,                         // 0x157
    s0158_ActorAnimationFree,                         // 0x158
    s0159_ActorAnimationPlay,                         // 0x159
    s015A_ActorAnimationWait,                         // 0x15a
    s015B_WifiListHasValidProfile,                    // 0x15b
    s015C_WifiListCountFriends,                       // 0x15c
    s015D_NetConnectWiFiClub,                         // 0x15d
    s015E_NetConnectGTS,                              // 0x15e
    s015F_Nop,                                        // 0x15f
    s0160_NetConnectWiFiBattle,                       // 0x160
    s0161_NetConnectBattleVideo,                      // 0x161
    s0162_NetConnectGTSNegotiation,                   // 0x162
    s0163_MusicalCall,                                // 0x163
    s0164_MusicalCallDressUp,                         // 0x164
    s0165_MusicalGetInfo,                             // 0x165
    s0166_MusicalGetAudienceGift,                     // 0x166
    s0167_MusicalComm,                                // 0x167
    s0168_MusicalAddProp,                             // 0x168
    s0169_MusicalClearAudienceGift,                   // 0x169
    s016A_MusicalSelectPoke,                          // 0x16a
    NULL,                                             // 0x16b
    NULL,                                             // 0x16c
    NULL,                                             // 0x16d
    s016E_CallMonolith,                               // 0x16e
    NULL,                                             // 0x16f
    func_ov033_021772f4,                              // 0x170
    NULL,                                             // 0x171
    func_ov033_021772c8,                              // 0x172
    NULL,                                             // 0x173
    s0174_CallWildBattle,                             // 0x174
    s0175_CallWildBattleEnd,                          // 0x175
    s0176_CallWildLose,                               // 0x176
    s0177_WildBattleIsVictory,                        // 0x177
    s0178_WildBattleGetResult,                        // 0x178
    s0179_CallCaptureDemo,                            // 0x179
    GimmickGateCmd_ViewBoard,                         // 0x17a
    func_ov012_02157bdc,                              // 0x17b
    s017C_RollerCoasterCallSwitch,                    // 0x17c
    s017D_RollerCoasterSetCarIntroSeen,               // 0x17d
    NULL,                                             // 0x17e
    NULL,                                             // 0x17f
    NULL,                                             // 0x180
    NULL,                                             // 0x181
    NULL,                                             // 0x182
    NULL,                                             // 0x183
    NULL,                                             // 0x184
    NULL,                                             // 0x185
    s0186_DriftveilGymPrepareFreight,                 // 0x186
    s0187_DriftveilLiftPlaceAtBottom,                 // 0x187
    NULL,                                             // 0x188
    NULL,                                             // 0x189
    NULL,                                             // 0x18a
    NULL,                                             // 0x18b
    s018C_DriftveilGymCallElevator,                   // 0x18c
    s018D_DriftveilGymCallFreight,                    // 0x18d
    s018E_DriftveilLiftCallMove,                      // 0x18e
    s018F_IcirrusGymCallSwitchAnim,                   // 0x18f
    s0190_IcirrusGymCallFlipSwitch,                   // 0x190
    s0191_IcirrusGymToggleMovementAtPlace,            // 0x191
    NULL,                                             // 0x192
    NULL,                                             // 0x193
    NULL,                                             // 0x194
    NULL,                                             // 0x195
    NULL,                                             // 0x196
    s0197_CGearPowerOn,                               // 0x197
    s0198_FieldSubscreenChange,                       // 0x198
    s0199_FieldSubscreenReturn,                       // 0x199
    s019A_CGearControlWarning,                        // 0x19a
    s019B_PlayFieldEffect,                            // 0x19b
    s019C_PlayHMCutInEffect,                          // 0x19c
    s019D_PlayAlderFlyEffect,                         // 0x19d
    NULL,                                             // 0x19e
    NULL,                                             // 0x19f
    NULL,                                             // 0x1a0
    s01A1_PleasureBoatGetRoomInfo,                    // 0x1a1
    s01A2_CallRoyalUnovaView,                         // 0x1a2
    s01A3_FadeInBlackQ,                               // 0x1a3
    s01A4_FadeOutBlackQ,                              // 0x1a4
    s01A5_FadeInWhiteQ,                               // 0x1a5
    s01A6_FadeOutWhiteQ,                              // 0x1a6
    s01A7_FadeWait,                                   // 0x1a7
    s01A8_FadeInBlackQ_,                              // 0x1a8
    NULL,                                             // 0x1a9
    s01AA_CallSeasonBanner,                           // 0x1aa
    s01AB_FadeInBlack,                                // 0x1ab
    s01AC_FadeOutBlack,                               // 0x1ac
    s01AD_FadeInWhite,                                // 0x1ad
    s01AE_FadeOutWhite,                               // 0x1ae
    Regulation_RegisterAllowedSpecies,                // 0x1af
    Regulation_CheckPartyRegulation,                  // 0x1b0
    s01B1_CallLeagueLiftWarp,                         // 0x1b1
    ScrcmdProcFld_SpawnMovePokemonIfInactive,         // 0x1b2
    FieldChunk_ScrCmdReadSaveByte,                    // 0x1b3
    s01B4_FieldTradeStart,                            // 0x1b4
    s01B5_FieldTradeCheck,                            // 0x1b5
    NULL,                                             // 0x1b6
    NULL,                                             // 0x1b7
    NULL,                                             // 0x1b8
    NULL,                                             // 0x1b9
    NULL,                                             // 0x1ba
    NULL,                                             // 0x1bb
    NULL,                                             // 0x1bc
    NULL,                                             // 0x1bd
    NULL,                                             // 0x1be
    NULL,                                             // 0x1bf
    NULL,                                             // 0x1c0
    s01C1_ElevatorSetTablePtr,                        // 0x1c1
    s01C2_ElevatorBuildListMenu,                      // 0x1c2
    s01C2_ElevatorChangeMap,                          // 0x1c3, which swan's name numbers 0x1c2
    s01C4_PokeDexIsComplete,                          // 0x1c4
    s01C5_PokeDexGetEvaluationParams,                 // 0x1c5
    s01C6_PokeDexGiveNational,                        // 0x1c6
    s01C7_PokeDexHaveNational,                        // 0x1c7
    s01C8_PokeDexEnable,                              // 0x1c8
    s01C9_NDemoStart,                                 // 0x1c9
    s01CA_NDemoEnd,                                   // 0x1ca
    s01CB_NDemoReadyTalkMotion,                       // 0x1cb
    ElevatorTable_GetMysteryEntryCount,               // 0x1cc
    ElevatorTable_RegisterBirthdayEntry,              // 0x1cd
    s01CE_UnityTowerGetTrainerClass,                  // 0x1ce
    s01CF_UnityTowerGetStateParam,                    // 0x1cf
    s01D0_UnityTowerCallFloorSelect,                  // 0x1d0
    s01D1_MoveTutorCheckParty,                        // 0x1d1
    s01D2_MoveTutorCheckPkm,                          // 0x1d2
    s01D3_MoveTutorGetMoveID,                         // 0x1d3
    s01D4_MoveTutorCallPokeSelect,                    // 0x1d4
    s01D5_MoveReminderCheckPkm,                       // 0x1d5
    s01D6_MoveReminderCallMoveSelect,                 // 0x1d6
    s01D7_ObjInitPointGPos,                           // 0x1d7
    s01D8_ObjInitWarpGPos,                            // 0x1d8
    s01D9_ObjInitNPCGPos,                             // 0x1d9
    s01DA_CallPhraseSelect,                           // 0x1da
    func_ov033_02177d78,                              // 0x1db
    s01DC_DreamWorldItems,                            // 0x1dc
    s01DD_BroadcastGameBeacon,                        // 0x1dd
    s01DE_PassPowerPopDepletedID,                     // 0x1de
    s01DF_CallPassPowerFlash,                         // 0x1df
    s01E0_StadiumSetupActorsDouble,                   // 0x1e0
    s01E1_StadiumLoadTrainerTable,                    // 0x1e1
    s01E2_StadiumFreeTrainerTable,                    // 0x1e2
    s01E3_StadiumSetupActorSingle,                    // 0x1e3
    s01E0_StadiumSetupActorsTriple,                   // 0x1e4, which swan's name numbers 0x1e0
    s01E5_StadiumResetTrainerFlags,                   // 0x1e5
    s01E6_TrialHouseWorkInit,                         // 0x1e6
    s01E7_TrialHouseWorkDelete,                       // 0x1e7
    s01E8_TrialHousePrepareParty,                     // 0x1e8
    s01E9_TrialHouseCallTeamSelect,                   // 0x1e9
    s01EA_TrialHouseSetTrainer,                       // 0x1ea
    s01EB_TrialHouseMsgDisp,                          // 0x1eb
    s01EC_TrialHouseStartBattle,                      // 0x1ec
    s01ED_TrialHouseSetDownloadMode,                  // 0x1ed
    s01EE_TrialHouseSetBattleTestRank,                // 0x1ee
    s01EF_TrialHouseGetBattleTestRank,                // 0x1ef
    s01F0_TrialHouseDownloadExamination,              // 0x1f0
    s01F1_TrialHouseCalcPointsStars,                  // 0x1f1
    s01F2_TrialHouseGetExaminationType,               // 0x1f2
    s01F3_TrialHouseSaveData,                         // 0x1f3
    s01F4_TrialHouseCallResults,                      // 0x1f4
    s01F5_TrialHouseGetRecordMask,                    // 0x1f5
    Scrcmd_CheckWifiListFlags,                        // 0x1f6
    Scrcmd_WifiListOperation,                         // 0x1f7
    Scrcmd_SetWifiFriendNameWord,                     // 0x1f8
    Scrcmd_CheckPmsWordPair,                          // 0x1f9
    s01FA_PepQuizGenerate,                            // 0x1fa
    s01FB_ItemCollectorCheckGroup,                    // 0x1fb
    s01FC_ItemCollectorGetPrice,                      // 0x1fc
    SurveyCmd_Start,                                  // 0x1fd
    SurveyCmd_Clear,                                  // 0x1fe
    s01FF_SurveyGetCurrentQuestionID,                 // 0x1ff
    s0200_SurveyGetCurrentAnswerIDs,                  // 0x200
    s0201_SurveyGetPopularOptionMsgID,                // 0x201
    SurveyCmd_GetQuestionState,                       // 0x202
    SurveyCmd_GetCurrentRemaining,                    // 0x203
    s0204_SurveyGetTime,                              // 0x204
    s0205_CallGreetingPhraseInput,                    // 0x205
    s0206_CallThanksPhraseInput,                      // 0x206
    s0207_WordSetPasserbyCount,                       // 0x207
    s0208_WordSetThankedCount,                        // 0x208
    SurveyCmd_WriteOwnAnswer,                         // 0x209
    SurveyCmd_ShowRankMessage,                        // 0x20a
    SurveyCmd_GetRank,                                // 0x20b
    SurveyCmd_RaiseRank,                              // 0x20c
    s020D_PokePartyFindEx,                            // 0x20d
    s020E_SpPokeJump,                                 // 0x20e
    s020F_SpPokeAppear,                               // 0x20f
    s0210_SpPokeWaitAnimEnd,                          // 0x210
    s0211_SpPokeWaitFrame,                            // 0x211
    s0212_ActorMoveToHeight,                          // 0x212
    s0213_ActorHoldPos,                               // 0x213
    s0214_CallPokemonPreview,                         // 0x214
    s0215_SetSpecialTransferUsed,                     // 0x215
    Scrcmd_CheckBeaconTrainerKey,                     // 0x216
    s0217_MapChangeEntreeForest,                      // 0x217
    func_ov012_02164c68,                              // 0x218
    func_ov012_02164bcc,                              // 0x219
    s021A_WordSetLoadEntreeForestPkmName,             // 0x21a
    s021B_EntreeForestSpawnAllPkm,                    // 0x21b
    s021C_EntreeForestStartBattle,                    // 0x21c
    s021D_EntreeForestRemovePkm,                      // 0x21d
    s021E_FishingChallengeGetRandomPkm,               // 0x21e
    func_ov012_02156104,                              // 0x21f
    func_ov012_02156128,                              // 0x220
    s0221_AnvilleTrainGetModelID,                     // 0x221
    s0222_AnvilleTrainGetDailyIndex,                  // 0x222
    s0223_PokePartyGetHiddenPowerType,                // 0x223
    s0224_PokePartyGetIV,                             // 0x224
    s0225_ItemGetRandomOwnedTMMove,                   // 0x225
    CountTrainersEvent_StartCountOtherOtIdsTask,      // 0x226
    s0227_RecordAdd,                                  // 0x227
    s0228_RecordGet,                                  // 0x228
    NULL,                                             // 0x229
    s022A_HollowRivalGenMsgID,                        // 0x22a
    Scrcmd_StartSwapNpcEvent,                         // 0x22b
    NULL,                                             // 0x22c
    s022D_PlayFieldEffectByTable,                     // 0x22d
    ScriptNative_SetActorZoneDeleteOff,               // 0x22e
    s022F_WordSetLoadPastTradePkmName,                // 0x22f
    s0230_MsgPlaceSignFont4,                          // 0x230
    s0231_CallGameManual,                             // 0x231
    PokeParty_ControlLegendMon,                       // 0x232
    Scrcmd_HasPlayerLanguage,                         // 0x233
    s0234_ActorFallDownToXZ,                          // 0x234
    s0235_BGMPlayEx,                                  // 0x235
    s0236_WordSetLoadItemCollectorPrice,              // 0x236
    s0237_ItemCollectorSell,                          // 0x237
    s0238_BGMChangeMapEx,                             // 0x238
    ScriptNative_CheckPlayerFormModel,                // 0x239
    ScriptWork_PlayPerapVoice,                        // 0x23a
    func_ov012_02169c1c,                              // 0x23b
    s023C_NuvemaGimmickPlay,                          // 0x23c
    s023D_NuvemaGimmickWaitAnim,                      // 0x23d
    s023E_DayCareGetSexForNamePrint,                  // 0x23e
    s023F_CallPlaceNameDisp,                          // 0x23f
    ScriptWork_FadeBgmVolume,                         // 0x240
    ScriptWork_RestoreBgmVolume,                      // 0x241
    NULL,                                             // 0x242
    NULL,                                             // 0x243
    NULL,                                             // 0x244
    s0245_BGMFadeOut,                                 // 0x245
    s0246_BGMFadeOutAll,                              // 0x246
    s0247_MapChangeRail,                              // 0x247
    s0248_PlayerMoveToYAsync_,                        // 0x248
    s0249_GetRandomDistinctSix,                       // 0x249
    s024A_FieldSubscreenIsEnabled,                    // 0x24a
    s024B_FieldSubscreenDisable,                      // 0x24b
    s024C_BGMAmbienceResume,                          // 0x24c
    s024D_RollerCoasterAmbienceStart,                 // 0x24d
    s024E_PokePartyIsFullPP,                          // 0x24e
    s024F_ActorWalkRoute,                             // 0x24f
    s0250_ActorPairSet,                               // 0x250
    s0251_ActorPairEnd,                               // 0x251
    s0252_ActorPairGetTrID,                           // 0x252
    s0253_ActorPairSetMoveEnable,                     // 0x253
    NULL,                                             // 0x254
    NULL,                                             // 0x255
    NULL,                                             // 0x256
    NULL,                                             // 0x257
    NULL,                                             // 0x258
    NULL,                                             // 0x259
    func_ov012_02157b7c,                              // 0x25a
    s025B_GymElecReapplyProgress,                     // 0x25b
    s025C_GymElecSetFollower,                         // 0x25c
    s025D_GymElecSetProgress,                         // 0x25d
    NULL,                                             // 0x25e
    s025F_GymElecSetStage,                            // 0x25f
    NULL,                                             // 0x260
    NULL,                                             // 0x261
    s0262_HollowRivalSetValue,                        // 0x262
    s0263_HollowRivalSetFlag,                         // 0x263
    func_ov012_0216a748,                              // 0x264
    s0265_CallXTransceiverSecret,                     // 0x265
    s0266_HollowRivalGetCounter,                      // 0x266
    func_ov012_0216a7cc,                              // 0x267
    s0268_GymInsectPlayObject,                        // 0x268
    s0269_GymInsectShowEffect,                        // 0x269
    NULL,                                             // 0x26a
    NULL,                                             // 0x26b
    s026C_WordSetMedalName,                           // 0x26c
    s026D_WordSetMedalRank,                           // 0x26d
    s026E_MedalGetCount,                              // 0x26e
    NULL,                                             // 0x26f
    NULL,                                             // 0x270
    s0271_MedalAcknowledge,                           // 0x271
    s0272_MedalGetSpinTradeActor,                     // 0x272
    s0273_MedalGive,                                  // 0x273
    s0274_FunfestMissionStart,                        // 0x274
    s0275_WordSetFunfestMission,                      // 0x275
    s0276_FunfestMissionBroadcast,                    // 0x276
    s0277_FunfestActorDelete,                         // 0x277
    s0278_FunfestDispSalesmanMessage,                 // 0x278
    s0279_FunfestBGMReturn,                           // 0x279
    s027A_FunfestGetGenericInfo,                      // 0x27a
    s027B_FunfestGetItemExchangeInfo,                 // 0x27b
    s027C_FunfestGetItemSaleInfo,                     // 0x27c
    s027D_FunfestGetPokemonQuizInfo,                  // 0x27d
    s027E_FunfestGetPokemonQuizSpecies,               // 0x27e
    s027F_FunfestGetPokemonQuizBogusSpecies,          // 0x27f
    NULL,                                             // 0x280
    NULL,                                             // 0x281
    NULL,                                             // 0x282
    NULL,                                             // 0x283
    s0284_CallPlaceSelect,                            // 0x284
    s0285_CallWordSetPokeNameInput,                   // 0x285
    NULL,                                             // 0x286
    NULL,                                             // 0x287
    NULL,                                             // 0x288
    NULL,                                             // 0x289
    s028A_MapChangeFlyWarp,                           // 0x28a
    s028B_CallEntralinkWarpOut,                       // 0x28b
    NULL,                                             // 0x28c
    NULL,                                             // 0x28d
    s028E_MistraltonGymFireCannon,                    // 0x28e
    s028F_CallRivalNameInput,                         // 0x28f
    s0290_WordSetLoadRivalName,                       // 0x290
    s0291_VirbankGymStopModelAnimations,              // 0x291
    s0292_VirbankGymSetActorFlag,                     // 0x292
    s0293_HiddenHollowCallWarpIn,                     // 0x293
    s0294_HiddenHollowGetParam,                       // 0x294
    s0295_HiddenHollowReset,                          // 0x295
    s0296_HiddenHollowCallWarpOut,                    // 0x296
    s0297_CallWildBattleEx,                           // 0x297
    s0298_WordSetEmptyFileMessage,                    // 0x298
    s0299_WordSetLoadAbility,                         // 0x299
    s029A_WordSetLoadNature,                          // 0x29a
    s029B_WordSetLoadJoinAvenueName,                  // 0x29b
    NULL,                                             // 0x29c
    NULL,                                             // 0x29d
    s029E_MedalGetFieldEffectID,                      // 0x29e
    s029F_MedalDiscover,                              // 0x29f
    s02A0_MedalIsObtained,                            // 0x2a0
    s02A1_MedalGetMostCompleteCategory,               // 0x2a1
    s02A2_HumilauGymPlayLeaderSplash,                 // 0x2a2
    s02A3_HumilauGymPlayLeaderWaves,                  // 0x2a3
    func_ov012_02157bb4,                              // 0x2a4
    s02A5_OpelucidGymCallMove,                        // 0x2a5
    s02A6_OpelucidGymRefreshVars,                     // 0x2a6
    s02A7_OpelucidGymCallButton,                      // 0x2a7
    s02A8_OpelucidGymFreeHeap,                        // 0x2a8
    s02A9_OpelucidGymCreateHeap,                      // 0x2a9
    NULL,                                             // 0x2aa
    NULL,                                             // 0x2ab
    s02AC_CallLeafPileStuck,                          // 0x2ac
    NULL,                                             // 0x2ad
    s02AE_GameGetMyCity,                              // 0x2ae
    s02AF_GameGetDifficulty,                          // 0x2af
    s02B0_CallUnovaLinkKeyUnlock,                     // 0x2b0
    s02B1_KeyInfoGetChamberKey,                       // 0x2b1
    s02B2_MemoryLinkCheckFlag,                        // 0x2b2
    s02B3_MemoryLinkGetVar,                           // 0x2b3
    s02B4_MemoryLinkIsPlayerLinked,                   // 0x2b4
    s02B5_WordSetMemoryLinkPlayerName,                // 0x2b5
    s02B6_MemoryLinkCheckMusical,                     // 0x2b6
    s02B7_MemoryLinkAddMusicalProps,                  // 0x2b7
    s02B8_PedometerStart,                             // 0x2b8
    s02B9_PedometerEnd,                               // 0x2b9
    s02BA_PedometerGet,                               // 0x2ba
    s02BB_Gym0601FanAmbienceStart,                    // 0x2bb
    s02BC_TVCheckCommercial,                          // 0x2bc
    s02BD_TVGenCommercialMsgID,                       // 0x2bd
    s02BE_ActorMoveLinear,                            // 0x2be
    NULL,                                             // 0x2bf
    s02C0_FieldTradeGetSpecies,                       // 0x2c0
    NULL,                                             // 0x2c1
    s02C2_RepelRearm,                                 // 0x2c2
    func_ov012_0216a7f4,                              // 0x2c3
    s02C4_HiddenHollowSetPeriod,                      // 0x2c4
    s02C5_JoinAvenueRecordActivity,                   // 0x2c5
    s02C6_JoinAvenueStoreStart,                       // 0x2c6
    s02C6_JoinAvenueStoreEnd,                         // 0x2c7, which swan's name numbers 0x2c6
    NULL,                                             // 0x2c8
    NULL,                                             // 0x2c9
    s02CA_CallHappyPhraseInput,                       // 0x2ca
    s02CB_PokewoodGetLevel,                           // 0x2cb
    NULL,                                             // 0x2cc
    NULL,                                             // 0x2cd
    NULL,                                             // 0x2ce
    s02CF_PokeDexCheckHabitatList,                    // 0x2cf
    s02D0_PokeDexEnableHabitatList,                   // 0x2d0
    s02D1_KeyInfoGetMyCityProgress,                   // 0x2d1
    s02D2_FieldOpenRestoreLCD,                        // 0x2d2
    s02D3_HollowRivalGetValue,                        // 0x2d3
    s02D4_ItemGetTMCount,                             // 0x2d4
    s02D5_WbtIsTrainerBeaten,                         // 0x2d5
    NULL,                                             // 0x2d6
    Scrcmd_GetWifiHistoryCountryCount,                // 0x2d7
    Scrcmd_MarkPartnerCandidate,                      // 0x2d8
    Scrcmd_IsPartnerCandidateMarked,                  // 0x2d9
    s02DA_UnityTowerSetRatingLevel,                   // 0x2da
    s02DB_UnityTowerSetFloor,                         // 0x2db
    s02DC_UnityTowerInitVisitorMessage,               // 0x2dc
    s02DD_UnityTowerGetVisitorCount,                  // 0x2dd
    s02DE_UnityTowerSetHobby,                         // 0x2de
    s02DF_UnityTowerGetHobby,                         // 0x2df
    NULL,                                             // 0x2e0
    s02E1_MedalDiscoverInitial,                       // 0x2e1
    NULL,                                             // 0x2e2
    s02E3_LensFlareRequest,                           // 0x2e3
    NULL,                                             // 0x2e4
    s02E5_HiddenHollowSet,                            // 0x2e5
    s02E6_HiddenHollowCreateEvents,                   // 0x2e6
    NULL,                                             // 0x2e7
    s02E8_CallFieldFadeOut,                           // 0x2e8
    s02E9_CallFieldFadeIn,                            // 0x2e9
    s02EA_PokePartyAddNPoke,                          // 0x2ea
    NULL,                                             // 0x2eb
    NULL,                                             // 0x2ec
    s02ED_SetSoftResetBlocked,                        // 0x2ed
    s02EE_MusicalIsPropOwned,                         // 0x2ee
    s02EF_MusicalGetOwnedPropCount,                   // 0x2ef
    NULL,                                             // 0x2f0
    s02F1_SetOneShotDRObtained,                       // 0x2f1
    s02F2_IsOneShotDRObtained,                        // 0x2f2
};

// MWCC lays the file's data out by size, so the count comes first in the ROM, ahead of the tables
const u32 EVCMD_MAX = NELEMS(EVCMD_TABLE);

BOOL s0000_VMNop(VM *vm, FieldScriptEnv *env) {
    return FALSE;
}

BOOL s0001_VMNop2(VM *vm, FieldScriptEnv *env) {
    return FALSE;
}

BOOL s0002_VMHalt(VM *vm, FieldScriptEnv *env) {
    VM_Halt(vm);
    return TRUE;
}

BOOL swapToScrcmdEnvirDecPauseCtr(VM *vm, void *env) {
    return FieldScriptEnv_UpdateWaitCounter(env);
}

BOOL s0003_VMSleep(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_SetWaitCounter(env, VM_Read16(vm));
    VM_SetNativeCallback(vm, swapToScrcmdEnvirDecPauseCtr);
    return TRUE;
}

BOOL s0004_VMCall(VM *vm, FieldScriptEnv *env) {
    u32 offset = VM_Read32(vm);

    VM_Call(vm, vm->pc + offset);
    return FALSE;
}

BOOL s0005_VMReturn(VM *vm, FieldScriptEnv *env) {
    VM_Return(vm);
    return FALSE;
}

BOOL s0006_DebugPrint(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetGameData(env);
    ScriptReadAny(vm, env);
    return FALSE;
}

BOOL s0007_DebugStack(VM *vm, FieldScriptEnv *env) {
    VM_StackPop(vm);
    ScriptReadAny(vm, env);
    return FALSE;
}

BOOL s0014_VMRegSet8(VM *vm, FieldScriptEnv *env) {
    u8 index = VM_Read8(vm);
    u8 value = VM_Read8(vm);

    vm->work[index] = value;
    return FALSE;
}

BOOL s0015_VMRegSet32(VM *vm, FieldScriptEnv *env) {
    u8 index = VM_Read8(vm);
    u32 value = VM_Read32(vm);

    vm->work[index] = value;
    return FALSE;
}

BOOL s0016_VMRegMov(VM *vm, FieldScriptEnv *env) {
    u8 dst = VM_Read8(vm);
    u8 src = VM_Read8(vm);

    vm->work[dst] = vm->work[src];
    return FALSE;
}

u8 VMCmp(u32 left, u32 right) {
    if (left < right) {
        return 0;
    }
    if (left == right) {
        return 1;
    }
    return 2;
}

BOOL s0017_VMRegCmp8(VM *vm, FieldScriptEnv *env) {
    u8 left = vm->work[VM_Read8(vm)];
    u8 right = vm->work[VM_Read8(vm)];

    vm->cmpResult = VMCmp(left, right);
    return FALSE;
}

BOOL s0018_VMRegCmpConst8(VM *vm, FieldScriptEnv *env) {
    u8 left = vm->work[VM_Read8(vm)];
    u8 right = VM_Read8(vm);

    vm->cmpResult = VMCmp(left, right);
    return FALSE;
}

BOOL s0019_WorkCmpConst(VM *vm, FieldScriptEnv *env) {
    u16 left = *ScriptReadVar(vm, env);
    u16 right = VM_Read16(vm);

    vm->cmpResult = VMCmp(left, right);
    return FALSE;
}

BOOL s001A_WorkCmpWork(VM *vm, FieldScriptEnv *env) {
    u16 *left = ScriptReadVar(vm, env);
    u16 *right = ScriptReadVar(vm, env);

    vm->cmpResult = VMCmp(*left, *right);
    return FALSE;
}

BOOL s0008_VMStackPushConst(VM *vm, FieldScriptEnv *env) {
    VM_StackPush(vm, VM_Read16(vm));
    return FALSE;
}

BOOL s0009_VMStackPush(VM *vm, FieldScriptEnv *env) {
    VM_StackPush(vm, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s000A_VMStackPop(VM *vm, FieldScriptEnv *env) {
    u32 value = VM_StackPop(vm);

    *ScriptReadVar(vm, env) = value;
    return FALSE;
}

BOOL s000B_VMStackDiscard(VM *vm, FieldScriptEnv *env) {
    VM_StackPop(vm);
    return FALSE;
}

BOOL s000C_VMStackAdd(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left + right);
    return FALSE;
}

BOOL s000D_VMStackSub(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left - right);
    return FALSE;
}

BOOL s000E_VMStackMul(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left * right);
    return FALSE;
}

BOOL s000F_VMStackDiv(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left / right);
    return FALSE;
}

BOOL s0010_VMStackPushFlag(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);

    VM_StackPush(vm, EventWork_FlagGet(eventWork, flag));
    return FALSE;
}

BOOL s0011_VMStackCmp(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);
    u16 comparison = VM_Read16(vm);
    BOOL result;

    switch (comparison) {
    case 0:
        result = left < right;
        break;
    case 1:
        result = left == right;
        break;
    case 2:
        result = left > right;
        break;
    case 3:
        result = left <= right;
        break;
    case 4:
        result = left >= right;
        break;
    case 5:
        result = left != right;
        break;
    case 6:
        result = left == 1 || right == 1;
        break;
    case 7:
        result = left == 1 && right == 1;
        break;
    default:
        result = FALSE;
        break;
    }
    VM_StackPush(vm, result);
    return FALSE;
}

BOOL s001B_RTCallGlobalAsync(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    u16 scriptId;
    u16 zoneId;

    FieldScriptEnv_GetGameSystem(env);
    work = FieldScriptEnv_GetScriptWork(env);
    scriptId = VM_Read16(vm);
    zoneId = FieldScriptEnv_GetZoneID(env);
    ScriptWork_AddVM(work, zoneId, scriptId);
    return TRUE;
}

BOOL ScriptNative_WaitFinishSubScript(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    ScriptWork *work;
    u8 index;

    work = FieldScriptEnv_GetScriptWork(env);
    index = FieldScriptEnv_GetVMIndex(env);
    if (!FieldScript_VMExists(work, index)) {
        FieldScriptEnv_Restore(env);
        return TRUE;
    }
    return FALSE;
}

BOOL s001C_RTCallGlobal(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    ScriptWork *work;
    u16 scriptId;
    u32 level;
    u32 index;

    gsys = FieldScriptEnv_GetGameSystem(env);
    work = FieldScriptEnv_GetScriptWork(env);
    scriptId = VM_Read16(vm);
    if (!FieldScriptEnv_IsReducedFeatureLevel(env)) {
        index = ScriptWork_AddVM(work, FieldScriptEnv_GetZoneID(env), scriptId);
        SetScrEnvVMIndex(env, index);
        FieldScriptEnv_Save(env);
        VM_SetNativeCallback(vm, ScriptNative_WaitFinishSubScript);
        return TRUE;
    }
    FieldScriptEnv_Save(env);
    level = FieldScriptEnv_GetFeatureLevel(env);
    FieldScript_Run(gsys, work, scriptId, level);
    FieldScriptEnv_Restore(env);
    return FALSE;
}

BOOL s001D_RTEndGlobal(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    VM_Halt(vm);
    return TRUE;
}

BOOL s001E_VMJump(VM *vm, FieldScriptEnv *env) {
    u32 offset = VM_Read32(vm);

    VM_Jump(vm, vm->pc + offset);
    return FALSE;
}

BOOL s001F_VMJumpIf(VM *vm, FieldScriptEnv *env) {
    u8 condition = VM_Read8(vm);
    u32 offset = VM_Read32(vm);

    if (condition == 0xff) {
        if (VM_StackPop(vm) != 1) {
            VM_Jump(vm, vm->pc + offset);
        }
    } else if (VM_CMP_LUT[condition][vm->cmpResult] == 1) {
        VM_Jump(vm, vm->pc + offset);
    }
    return FALSE;
}

BOOL s0020_VMCallIf(VM *vm, FieldScriptEnv *env) {
    u8 condition = VM_Read8(vm);
    u32 offset = VM_Read32(vm);

    if (VM_CMP_LUT[condition][vm->cmpResult] == 1) {
        VM_Call(vm, vm->pc + offset);
    }
    return FALSE;
}

BOOL s0021_RTReserveScript(VM *vm, FieldScriptEnv *env) {
    u16 scriptId = VM_Read16(vm);
    FieldStatus *status = GameData_GetFieldStatus(FieldScriptEnv_GetGameData(env));

    FieldStatus_ReserveScript(status, scriptId);
    return FALSE;
}

BOOL s0022_FieldGetContinueFlag(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    FieldStatus *status = GameData_GetFieldStatus(FieldScriptEnv_GetGameData(env));

    *value = FieldStatus_CheckContinueFlag(status);
    return FALSE;
}

BOOL s0023_FlagSet(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);

    EventWork_FlagSet(eventWork, flag);
    return FALSE;
}

BOOL s0024_FlagReset(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);

    EventWork_FlagReset(eventWork, flag);
    return FALSE;
}

BOOL s0025_FlagGet(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);
    u16 *value = ScriptReadVar(vm, env);

    *value = EventWork_FlagGet(eventWork, flag);
    return FALSE;
}

BOOL s0026_WorkAdd(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value += operand;
    return FALSE;
}

BOOL s0027_WorkSub(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value -= operand;
    return FALSE;
}

BOOL s002B_WorkMul(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value *= operand;
    return FALSE;
}

BOOL s002C_WorkDiv(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value /= operand;
    return FALSE;
}

BOOL s002D_WorkMod(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value %= operand;
    return FALSE;
}

BOOL s0012_WorkAnd(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value &= operand;
    return FALSE;
}

BOOL s0013_WorkOr(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value |= operand;
    return FALSE;
}

BOOL s0028_WorkSetConst(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);

    *value = VM_Read16(vm);
    return FALSE;
}

BOOL s0029_WorkGet(VM *vm, FieldScriptEnv *env) {
    u16 *dst = ScriptReadVar(vm, env);
    u16 *src = ScriptReadVar(vm, env);

    *dst = *src;
    return FALSE;
}

BOOL s002A_WorkSet(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);

    *value = ScriptReadAny(vm, env);
    return FALSE;
}

BOOL s002E_ActorsPauseAll(VM *vm, FieldScriptEnv *env) {
    return PauseEventMModels(vm, env);
}

BOOL s002F_ActorsUnpauseAll(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    BOOL wait = FALSE;

    EnableAllActorsMovementScr(env);
    if (FieldSnd_BGMGetStackIndex(GameData_GetFieldSoundSystem(gameData)) != 0) {
        ScriptWork_CallEvent(work, EventBGMPopAll_Create(gsys, 0));
        wait = TRUE;
    }
    return wait;
}

BOOL s0030_FinishAllEvents(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event;

    FieldScriptEnv_GetGameSystem(env);
    event = EventFinishScriptSubEvents_Create(env);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL testAB(VM *vm, void *env) {
    return (GCTX_HIDGetPressedKeys() & 3) != 0;
}

BOOL s0031_ABKeyWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, testAB);
    return TRUE;
}

BOOL ScriptNative_LastKeyWait(VM *vm, void *env) {
    return (GCTX_HIDGetPressedKeys() & 0xf3) != 0;
}

BOOL s0032_LastKeyWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_LastKeyWait);
    return TRUE;
}

u16 *ScriptReadVar(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 id = VM_Read16(vm);

    return ScriptWork_GetWkAddr(work, gameData, id);
}

u16 ScriptReadAny(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 value = VM_Read16(vm);

    return ScriptWork_ResolveHybridValue(work, gameData, value);
}

u16 FieldScriptEnv_GetZoneID(FieldScriptEnv *env) {
    return PlayerState_GetZoneID(GameData_GetPlayerState(FieldScriptEnv_GetGameData(env)));
}
