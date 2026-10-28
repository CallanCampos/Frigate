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
    Scrcmd_Vm_VMNop,                                    // 0x000
    Scrcmd_Vm_VMNop2,                                   // 0x001
    Scrcmd_Vm_VMHalt,                                   // 0x002
    Scrcmd_Vm_VMSleep,                                  // 0x003
    Scrcmd_Vm_VMCall,                                   // 0x004
    Scrcmd_Vm_VMReturn,                                 // 0x005
    Scrcmd_Vm_DebugPrint,                               // 0x006
    Scrcmd_Vm_DebugStack,                               // 0x007
    Scrcmd_Vm_VMStackPushConst,                         // 0x008
    Scrcmd_Vm_VMStackPush,                              // 0x009
    Scrcmd_Vm_VMStackPop,                               // 0x00a
    Scrcmd_Vm_VMStackDiscard,                           // 0x00b
    Scrcmd_Vm_VMStackAdd,                               // 0x00c
    Scrcmd_Vm_VMStackSub,                               // 0x00d
    Scrcmd_Vm_VMStackMul,                               // 0x00e
    Scrcmd_Vm_VMStackDiv,                               // 0x00f
    Scrcmd_Vm_VMStackPushFlag,                          // 0x010
    Scrcmd_Vm_VMStackCmp,                               // 0x011
    Scrcmd_Vm_WorkAnd,                                  // 0x012
    Scrcmd_Vm_WorkOr,                                   // 0x013
    Scrcmd_Vm_VMRegSet8,                                // 0x014
    Scrcmd_Vm_VMRegSet32,                               // 0x015
    Scrcmd_Vm_VMRegMov,                                 // 0x016
    Scrcmd_Vm_VMRegCmp8,                                // 0x017
    Scrcmd_Vm_VMRegCmpConst8,                           // 0x018
    Scrcmd_Vm_WorkCmpConst,                             // 0x019
    Scrcmd_Vm_WorkCmpWork,                              // 0x01a
    Scrcmd_Vm_RTCallGlobalAsync,                        // 0x01b
    Scrcmd_Vm_RTCallGlobal,                             // 0x01c
    Scrcmd_Vm_RTEndGlobal,                              // 0x01d
    Scrcmd_Vm_VMJump,                                   // 0x01e
    Scrcmd_Vm_VMJumpIf,                                 // 0x01f
    Scrcmd_Vm_VMCallIf,                                 // 0x020
    Scrcmd_Vm_RTReserveScript,                          // 0x021
    Scrcmd_Vm_FieldGetContinueFlag,                     // 0x022
    Scrcmd_Vm_FlagSet,                                  // 0x023
    Scrcmd_Vm_FlagReset,                                // 0x024
    Scrcmd_Vm_FlagGet,                                  // 0x025
    Scrcmd_Vm_WorkAdd,                                  // 0x026
    Scrcmd_Vm_WorkSub,                                  // 0x027
    Scrcmd_Vm_WorkSetConst,                             // 0x028
    Scrcmd_Vm_WorkGet,                                  // 0x029
    Scrcmd_Vm_WorkSet,                                  // 0x02a
    Scrcmd_Vm_WorkMul,                                  // 0x02b
    Scrcmd_Vm_WorkDiv,                                  // 0x02c
    Scrcmd_Vm_WorkMod,                                  // 0x02d
    Scrcmd_Vm_ActorsPauseAll,                           // 0x02e
    Scrcmd_Vm_ActorsUnpauseAll,                         // 0x02f
    Scrcmd_Vm_FinishAllEvents,                          // 0x030
    Scrcmd_Vm_ABKeyWait,                                // 0x031
    Scrcmd_Vm_LastKeyWait,                              // 0x032
    Scrcmd_Msg_MsgSetAutoscrolls,                       // 0x033
    Scrcmd_Msg_SystemMsg,                               // 0x034
    Scrcmd_Msg_SystemMsgAsync,                          // 0x035
    Scrcmd_Msg_SystemMsgClose,                          // 0x036
    Scrcmd_Msg_MsgSetLoadingSpinner,                    // 0x037
    Scrcmd_Msg_InfoMsg,                                 // 0x038
    Scrcmd_Msg_InfoMsgClose,                            // 0x039
    Scrcmd_Msg_MultiMsg,                                // 0x03a
    Scrcmd_Msg_MsgWinCloseNo,                           // 0x03b
    Scrcmd_Msg_ActorMsg,                                // 0x03c
    Scrcmd_Msg_ParentActorMsg,                          // 0x03d
    Scrcmd_Msg_ActorMsgClose,                           // 0x03e
    Scrcmd_Msg_MsgWinCloseAll,                          // 0x03f
    Scrcmd_Msg_MoneyWinDisp,                            // 0x040
    Scrcmd_Msg_MoneyWinClose,                           // 0x041
    Scrcmd_Msg_MoneyWinUpdate,                          // 0x042
    Scrcmd_Msg_MsgPlaceSign,                            // 0x043
    Scrcmd_Msg_MsgPlaceSignClose,                       // 0x044
    Scrcmd_Msg_CheckerMsg,                              // 0x045
    Scrcmd_Msg_CheckerMsgClose,                         // 0x046
    Scrcmd_Menu_YesNoWin,                               // 0x047
    Scrcmd_Msg_ActorMsgGendered,                        // 0x048
    Scrcmd_Msg_ActorMsgVersioned,                       // 0x049
    Scrcmd_Msg_ScreamMsg,                               // 0x04a
    Scrcmd_Msg_MsgWaitAdvance,                          // 0x04b
    Scrcmd_Word_WordSetPlayerName,                      // 0x04c
    Scrcmd_Word_WordSetItemName,                        // 0x04d
    Scrcmd_Word_WordSetItemNameEx,                      // 0x04e
    Scrcmd_Word_WordSetItemNameWithArticle,             // 0x04f
    Scrcmd_Word_WordSetTMMoveName,                      // 0x050
    Scrcmd_Word_WordSetMoveName,                        // 0x051
    Scrcmd_Word_WordSetItemPocketName,                  // 0x052
    Scrcmd_Word_WordSetPartyPokeSpecies,                // 0x053
    Scrcmd_Word_WordSetPartyPokeName,                   // 0x054
    Scrcmd_Word_WordSetDaycarePokeSpecies,              // 0x055
    Scrcmd_Word_WordSetPokeTypeName,                    // 0x056
    Scrcmd_Word_WordSetPokeSpecies,                     // 0x057
    Scrcmd_Word_WordSetPokeSpeciesWithArticle,          // 0x058
    Scrcmd_Word_WordSetPlaceName,                       // 0x059
    Scrcmd_Word_WordSetTrendName,                       // 0x05a
    Scrcmd_Word_WordSetDaycarePokeName,                 // 0x05b
    Scrcmd_Word_WordSetNumber,                          // 0x05c
    Scrcmd_Musical_WordSetMusicalInfo,                  // 0x05d
    Scrcmd_Word_WordSetCountry,                         // 0x05e
    Scrcmd_Word_WordSetHobbyName,                       // 0x05f
    Scrcmd_Word_WordSetPassPowerName,                   // 0x060
    Scrcmd_Word_WordSetTrainerClassName,                // 0x061
    Scrcmd_Word_WordSetTrainerClassNameWithArticle,     // 0x062
    Scrcmd_Word_WordSetSurveyAnswer,                    // 0x063
    Scrcmd_FieldActor_ActorCmdExec,                     // 0x064
    Scrcmd_FieldActor_ActorCmdWait,                     // 0x065
    Scrcmd_FieldActor_ActorGetMoveCode,                 // 0x066
    Scrcmd_FieldActor_ActorGetGPos,                     // 0x067
    Scrcmd_FieldActor_PlayerGetGPos,                    // 0x068
    Scrcmd_FieldActor_ActorNew,                         // 0x069
    Scrcmd_FieldActor_ActorGetSpawnFlag,                // 0x06a
    Scrcmd_FieldActor_ActorAdd,                         // 0x06b
    Scrcmd_FieldActor_ActorDelete,                      // 0x06c
    Scrcmd_FieldActor_ActorSetGPos,                     // 0x06d
    Scrcmd_FieldActor_PlayerGetDir,                     // 0x06e
    Scrcmd_FieldActor_PlayerGetActorInFront,            // 0x06f
    Scrcmd_FieldActor_ActorFindByGPos,                  // 0x070
    Scrcmd_FieldActor_PlayerGetRailPos,                 // 0x071
    Scrcmd_FieldActor_ActorGetRailPos,                  // 0x072
    Scrcmd_FieldActor_ActorSetMoveCode,                 // 0x073
    Scrcmd_FieldActor_ActorSetEyeToEye,                 // 0x074
    Scrcmd_FieldActor_PlayerSetSpecialSequence,         // 0x075
    Scrcmd_FieldActor_PlayerMoveToYAsync,               // 0x076
    Scrcmd_FieldActor_PlayerTurnByTrigger,              // 0x077
    Scrcmd_FieldActor_PlayerGetExState,                 // 0x078
    Scrcmd_FieldActor_ActorGetUserParam,                // 0x079
    Scrcmd_FieldActor_ActorPlayRailSlipdown,            // 0x07a
    Scrcmd_FieldActor_ActorJumpToGPos,                  // 0x07b
    Scrcmd_FieldActor_PlayerSetRailPos,                 // 0x07c
    Scrcmd_FieldActor_ActorSetRailPos,                  // 0x07d
    Scrcmd_FieldActor_ActorPlayTeleportSeq,             // 0x07e
    Scrcmd_Trainer_TrainerEyeGetTrainerID,              // 0x07f
    Scrcmd_Trainer_TrainerEyeEventInit,                 // 0x080
    Scrcmd_Trainer_TrainerEyeEventStart,                // 0x081
    Scrcmd_Trainer_TrainerGetActorID,                   // 0x082
    Scrcmd_Trainer_TrainerEyeGetBattleType,             // 0x083
    Scrcmd_Trainer_ActorGetTrainerID,                   // 0x084
    Scrcmd_Trainer_CallTrainerBattle,                   // 0x085
    Scrcmd_Trainer_CallTrainerMultiBattle,              // 0x086, which swan's name numbers 0x085
    Scrcmd_Msg_TrainerSayMessage,                       // 0x087
    Scrcmd_Trainer_TrainerGetMessageTypes,              // 0x088
    NULL,                                               // 0x089
    Scrcmd_Trainer_TrainerGetBattleType,                // 0x08a
    Scrcmd_Trainer_TrainerBGMPlayPush,                  // 0x08b
    Scrcmd_GameState_CallTrainerLose,                   // 0x08c
    Scrcmd_GameState_TrainerBattleIsVictory,            // 0x08d
    Scrcmd_Trainer_CallTrainerBattleEnd,                // 0x08e
    Scrcmd_Trainer_TrainerClassBGMPlayPush,             // 0x08f
    Scrcmd_Trainer_TrainerBGMPlay,                      // 0x090
    NULL,                                               // 0x091
    Scrcmd_Trainer_TrainerGetFieldAction,               // 0x092
    Scrcmd_Trainer_TrainerGetPrizeItem,                 // 0x093
    Scrcmd_Trainer_CallTradedPokemonBattle,             // 0x094
    Scrcmd_GameState_TrainerFlagSet,                    // 0x095
    Scrcmd_GameState_TrainerFlagReset,                  // 0x096
    Scrcmd_GameState_TrainerFlagGet,                    // 0x097
    Scrcmd_Sound_BGMPlay,                               // 0x098
    NULL,                                               // 0x099
    NULL,                                               // 0x09a
    Scrcmd_Sound_BGMIsPlaying,                          // 0x09b
    NULL,                                               // 0x09c
    NULL,                                               // 0x09d
    Scrcmd_Sound_BGMChangeMap,                          // 0x09e
    Scrcmd_Sound_BGMPlayPush,                           // 0x09f
    Scrcmd_Sound_BGMWait,                               // 0x0a0
    Scrcmd_Sound_BGMPush,                               // 0x0a1
    Scrcmd_Sound_BGMPop,                                // 0x0a2
    Scrcmd_Sound_ISSSwitchEnable,                       // 0x0a3
    Scrcmd_Sound_ISSSwitchDisable,                      // 0x0a4
    Scrcmd_Sound_ISSSwitchQuery,                        // 0x0a5
    Scrcmd_Sound_SEPlay,                                // 0x0a6
    Scrcmd_Sound_SEStop,                                // 0x0a7
    Scrcmd_Sound_SEWait,                                // 0x0a8
    Scrcmd_Sound_MEPlay,                                // 0x0a9
    Scrcmd_Sound_MEWait,                                // 0x0aa
    Scrcmd_Sound_PVPlay,                                // 0x0ab
    Scrcmd_Sound_PVWait,                                // 0x0ac
    Scrcmd_Menu_ListMenuInitCommon,                     // 0x0ad
    Scrcmd_Menu_ListMenuAnchorTopLeft,                  // 0x0ae
    Scrcmd_Menu_ListMenuAdd,                            // 0x0af
    Scrcmd_Menu_ListMenuShow,                           // 0x0b0
    Scrcmd_Menu_ListMenuShow2,                          // 0x0b1
    Scrcmd_Menu_ListMenuAnchorTopRight,                 // 0x0b2
    Scrcmd_Fade_FadeEx,                                 // 0x0b3
    Scrcmd_Fade_FadeExWait,                             // 0x0b4
    Scrcmd_Item_ItemAdd,                                // 0x0b5
    Scrcmd_Item_ItemSub,                                // 0x0b6
    Scrcmd_Item_ItemCheckSpace,                         // 0x0b7
    Scrcmd_Item_ItemCheckAmount,                        // 0x0b8
    Scrcmd_Item_ItemGetCount,                           // 0x0b9
    Scrcmd_Item_ItemIsTMHM,                             // 0x0ba
    Scrcmd_Item_ItemGetPocket,                          // 0x0bb
    Scrcmd_Encount_PhenomenonGetItemID,                 // 0x0bc
    Scrcmd_Item_ItemGetClass,                           // 0x0bd
    Scrcmd_Mapchange_MapChangeFake,                     // 0x0be
    Scrcmd_Mapchange_MapChangeWarpPad,                  // 0x0bf
    Scrcmd_Mapchange_MapChangeWarpRail,                 // 0x0c0
    Scrcmd_Mapchange_MapChangeQuicksand,                // 0x0c1
    Scrcmd_Mapchange_MapChangeWarp,                     // 0x0c2
    Scrcmd_Mapchange_MapChangeUnionRoom,                // 0x0c3
    Scrcmd_Mapchange_MapChangeCore,                     // 0x0c4
    Scrcmd_FieldMove_CallSurf,                          // 0x0c5
    Scrcmd_FieldMove_CallWaterfall,                     // 0x0c6
    Scrcmd_FieldMove_CallCut,                           // 0x0c7
    Scrcmd_FieldMove_CallDiving,                        // 0x0c8
    Scrcmd_GameState_ConfigToggleKanji,                 // 0x0c9
    Scrcmd_GameState_ConfigGetKanji,                    // 0x0ca
    Scrcmd_GameState_Random,                            // 0x0cb
    Scrcmd_GameState_RTGetTextFile,                     // 0x0cc
    Scrcmd_GameState_RTCGetDayPart,                     // 0x0cd
    Scrcmd_GameState_TrainerCardGetStars,               // 0x0ce
    Scrcmd_GameState_RTCGetWeekDay,                     // 0x0cf
    Scrcmd_GameState_RTCGetDate,                        // 0x0d0
    Scrcmd_GameState_RTCGetTime,                        // 0x0d1
    Scrcmd_GameState_RTCGetSeason,                      // 0x0d2
    Scrcmd_GameState_RTGetZoneID,                       // 0x0d3
    Scrcmd_GameState_TrainerCardGetBirthDate,           // 0x0d4
    Scrcmd_GameState_TrainerCardHasBadge,               // 0x0d5
    Scrcmd_GameState_TrainerCardAddBadge,               // 0x0d6
    Scrcmd_GameState_TrainerCardGetBadgeCount,          // 0x0d7
    Scrcmd_GameState_MapReplaceIsEventSet,              // 0x0d8
    Scrcmd_GameState_FieldSetTeleportZone,              // 0x0d9
    Scrcmd_GameState_MapReplaceSetEvent,                // 0x0da
    Scrcmd_GameState_FieldSetNextZoneHere,              // 0x0db
    Scrcmd_GameState_FieldSetNextZone,                  // 0x0dc
    Scrcmd_GameState_PokeDexGetCount,                   // 0x0dd
    Scrcmd_GameState_PokeDexRegist,                     // 0x0de
    Scrcmd_GameState_PokeDexIsRegist,                   // 0x0df
    Scrcmd_GameState_GameGetVersion,                    // 0x0e0
    Scrcmd_GameState_TrainerCardGetSex,                 // 0x0e1
    Scrcmd_GameState_SaveDataCheckRequired,             // 0x0e2
    Scrcmd_GameState_GiveRunningShoes,                  // 0x0e3
    Scrcmd_GameState_BoxUnlockWallpapers,               // 0x0e4
    Scrcmd_GameState_GetTrainerViewGroupAndMarkSeen,    // 0x0e5
    Scrcmd_GameState_StartMenuIsItemUnlocked,           // 0x0e6
    Scrcmd_GameState_StartMenuUnlockItem,               // 0x0e7
    Scrcmd_GameState_StartMenuIsItemSelected,           // 0x0e8
    NULL,                                               // 0x0e9
    Scrcmd_GameState_HOFCheckIntegrity,                 // 0x0ea
    Scrcmd_DayCare_DayCareCheckSpawnFlag,               // 0x0eb
    Scrcmd_DayCare_DayCareBreed,                        // 0x0ec
    Scrcmd_DayCare_DayCareResetSeed,                    // 0x0ed
    Scrcmd_DayCare_DayCareGetPkmCount,                  // 0x0ee
    Scrcmd_DayCare_DayCareCalcEggSpawnChance,           // 0x0ef
    Scrcmd_DayCare_DayCareDeposit,                      // 0x0f0
    Scrcmd_DayCare_DayCareWithdraw,                     // 0x0f1
    Scrcmd_DayCare_DayCareGetSpecies,                   // 0x0f2
    Scrcmd_DayCare_DayCareGetForme,                     // 0x0f3
    Scrcmd_DayCare_DayCareCalcNewLevel,                 // 0x0f4
    Scrcmd_DayCare_DayCareCalcLevelGain,                // 0x0f5
    Scrcmd_DayCare_DayCareCalcWithdrawCost,             // 0x0f6
    Scrcmd_DayCare_DayCareCallPokeSelect,               // 0x0f7
    Scrcmd_DayCare_DayCareGetSex,                       // 0x0f8
    Scrcmd_Pokemon_MoneyAdd,                            // 0x0f9
    Scrcmd_Pokemon_MoneySub,                            // 0x0fa
    Scrcmd_Pokemon_MoneyCheck,                          // 0x0fb
    Scrcmd_Pokemon_PokePartyGetHappiness,               // 0x0fc
    Scrcmd_Pokemon_PokePartyAdjustHappiness,            // 0x0fd
    Scrcmd_Pokemon_PokePartyGetSpecies,                 // 0x0fe
    Scrcmd_Pokemon_PokePartyGetForme,                   // 0x0ff
    Scrcmd_PokemonMisc_PokePartyCheckPokerus,           // 0x100
    Scrcmd_Pokemon_PokePartyIsFullHP,                   // 0x101
    Scrcmd_Pokemon_PokePartyIsEgg,                      // 0x102
    Scrcmd_Pokemon_PokePartyGetCount,                   // 0x103
    Scrcmd_Pokemon_PokePartyRecoverAll,                 // 0x104
    Scrcmd_PokemonMisc_CallPokeNameInput,               // 0x105
    Scrcmd_PokemonMisc_CallEggHatch,                    // 0x106
    Scrcmd_PokemonMisc_CallPokeSelect,                  // 0x107
    Scrcmd_Pokemon_PokePartyGetMoveCount,               // 0x108
    Scrcmd_PokemonMisc_CallPokeMoveReplace,             // 0x109
    Scrcmd_Pokemon_PokePartyGetMove,                    // 0x10a
    Scrcmd_Pokemon_PokePartyLearnMove,                  // 0x10b
    Scrcmd_Pokemon_PokePartyAdd,                        // 0x10c
    Scrcmd_Pokemon_PokePartyGetMemberByType,            // 0x10d
    Scrcmd_Pokemon_PokePartyAddEx,                      // 0x10e
    Scrcmd_Pokemon_PokePartyAddEgg,                     // 0x10f
    Scrcmd_Pokemon_PokePartyGetParam,                   // 0x110
    Scrcmd_Pokemon_PokePartySetIV,                      // 0x111
    Scrcmd_Pokemon_PokePartyGetEVTotal,                 // 0x112
    Scrcmd_Pokemon_PokePartyIsOriginGame,               // 0x113
    Scrcmd_Pokemon_PokePartyGetCountBySpecies,          // 0x114
    Scrcmd_Pokemon_PokePartyHasMove,                    // 0x115
    Scrcmd_Pokemon_PokePartyHasMoveAny,                 // 0x116
    Scrcmd_Pokemon_PokePartySetForme,                   // 0x117
    Scrcmd_Pokemon_PokePartyFindBySpecies,              // 0x118
    Scrcmd_Pokemon_PokePartyIsFromWhiteForest,          // 0x119
    Scrcmd_Pokemon_PokePartyGetMetDate,                 // 0x11a
    Scrcmd_Pokemon_PokePartyGetTypes,                   // 0x11b
    Scrcmd_Pokemon_PokePartyChangeRotomForme,           // 0x11c
    Scrcmd_PokemonMisc_TrainerCardSetFavePokemon,       // 0x11d
    Scrcmd_PokemonMisc_TrainerCardSaveGymVictoryParty,  // 0x11e
    Scrcmd_PokemonMisc_WordSetGymVictoryParty,          // 0x11f
    Scrcmd_PokemonMisc_FieldTradeSavePokemon,           // 0x120
    Scrcmd_Pokemon_BoxGetCount,                         // 0x121
    Scrcmd_Pokemon_BoxAdd,                              // 0x122
    Scrcmd_Pokemon_BoxAddEx,                            // 0x123
    Scrcmd_Buildmodel_BMHndAnmPlay,                     // 0x124
    Scrcmd_Buildmodel_BMPlayHOFMachineSeq,              // 0x125
    Scrcmd_Buildmodel_BMChangeMdlID,                    // 0x126
    Scrcmd_Buildmodel_BMCreateHandleByGPos,             // 0x127
    Scrcmd_Buildmodel_BMReleaseHandle,                  // 0x128
    Scrcmd_Buildmodel_BMHndAudioVisualAnmPlay,          // 0x129
    Scrcmd_Buildmodel_BMHndAnmWait,                     // 0x12a
    Scrcmd_Buildmodel_BMAnmPlayInv,                     // 0x12b
    Scrcmd_Buildmodel_BMHndAnmPause,                    // 0x12c
    Scrcmd_Buildmodel_BMSetVisible,                     // 0x12d
    Scrcmd_Buildmodel_BMAnmPlayLoop,                    // 0x12e
    Scrcmd_Buildmodel_PokecenPlayHealingSequence,       // 0x12f
    Scrcmd_Buildmodel_PokecenPCOpen,                    // 0x130
    Scrcmd_Buildmodel_PokecenPCIdle,                    // 0x131
    Scrcmd_Buildmodel_PokecenPCClose,                   // 0x132
    NULL,                                               // 0x133
    Scrcmd_Misc_CasteliaRushInit,                       // 0x134
    NULL,                                               // 0x135
    Scrcmd_Weather_FieldSetWeather,                     // 0x136
    Scrcmd_Save_SaveDataWrite,                          // 0x137
    Scrcmd_GameState_SaveDataGetStatus,                 // 0x138
    Scrcmd_Network_GameCommDisconnect,                  // 0x139
    Scrcmd_Net_GameCommGetStatus,                       // 0x13a
    Scrcmd_Network_GameCommCheckDSiWiFi,                // 0x13b
    Scrcmd_Network_GameCommTryBoot,                     // 0x13c
    Scrcmd_Network_GameCommPause,                       // 0x13d
    Scrcmd_Network_GameCommResume,                      // 0x13e
    Scrcmd_Camera_EvCameraInit,                         // 0x13f
    Scrcmd_Camera_EvCameraEnd,                          // 0x140
    Scrcmd_Camera_EvCameraUnbind,                       // 0x141
    Scrcmd_Camera_EvCameraRebind,                       // 0x142
    Scrcmd_Camera_EvCameraMoveTo,                       // 0x143
    Scrcmd_Camera_EvCameraReturn,                       // 0x144
    Scrcmd_Camera_EvCameraWait,                         // 0x145
    Scrcmd_Camera_EvCameraMoveToCommon,                 // 0x146
    Scrcmd_Camera_EvCameraMoveToDefault,                // 0x147
    Scrcmd_Camera_EvCameraShake,                        // 0x148
    Scrcmd_Shop_CallFriendlyShopBuy,                    // 0x149
    Scrcmd_Proc_FieldOpen,                              // 0x14a
    Scrcmd_Proc_FieldClose,                             // 0x14b
    Scrcmd_Proc_RTFreeUserHeap,                         // 0x14c
    Scrcmd_ProcFld_CallRecordSystem,                    // 0x14d
    Scrcmd_Proc_CallBag,                                // 0x14e
    Scrcmd_ProcFld_CallPC,                              // 0x14f
    Scrcmd_Proc_CallMailbox,                            // 0x150
    Scrcmd_Proc_CallPokedexDiploma,                     // 0x151
    Scrcmd_ProcFld_CallGeonet,                          // 0x152
    Scrcmd_Proc_callPoke3Select,                        // 0x153
    Scrcmd_Proc_Call3DDemo,                             // 0x154
    Scrcmd_ProcFld_CallXTransceiver,                    // 0x155
    Scrcmd_ProcFld_CallGameClear,                       // 0x156
    Scrcmd_FieldActorAnimation_ActorAnimationInit,      // 0x157
    Scrcmd_FieldActorAnimation_ActorAnimationFree,      // 0x158
    Scrcmd_FieldActorAnimation_ActorAnimationPlay,      // 0x159
    Scrcmd_FieldActorAnimation_ActorAnimationWait,      // 0x15a
    Scrcmd_Net_WifiListHasValidProfile,                 // 0x15b
    Scrcmd_Net_WifiListCountFriends,                    // 0x15c
    Scrcmd_Net_NetConnectWiFiClub,                      // 0x15d
    Scrcmd_Net_NetConnectGTS,                           // 0x15e
    Scrcmd_Proc_Nop,                                    // 0x15f
    Scrcmd_Proc_NetConnectWiFiBattle,                   // 0x160
    Scrcmd_Proc_NetConnectBattleVideo,                  // 0x161
    Scrcmd_Net_NetConnectGTSNegotiation,                // 0x162
    Scrcmd_Musical_MusicalCall,                         // 0x163
    Scrcmd_Musical_MusicalCallDressUp,                  // 0x164
    Scrcmd_Musical_MusicalGetInfo,                      // 0x165
    Scrcmd_Musical_MusicalGetAudienceGift,              // 0x166
    Scrcmd_Musical_MusicalComm,                         // 0x167
    Scrcmd_Musical_MusicalAddProp,                      // 0x168
    Scrcmd_Musical_MusicalClearAudienceGift,            // 0x169
    Scrcmd_Musical_MusicalSelectPoke,                   // 0x16a
    NULL,                                               // 0x16b
    NULL,                                               // 0x16c
    NULL,                                               // 0x16d
    Scrcmd_Proc_CallMonolith,                           // 0x16e
    NULL,                                               // 0x16f
    func_ov033_021772f4,                                // 0x170
    NULL,                                               // 0x171
    func_ov033_021772c8,                                // 0x172
    NULL,                                               // 0x173
    Scrcmd_Encount_CallWildBattle,                      // 0x174
    Scrcmd_Encount_CallWildBattleEnd,                   // 0x175
    Scrcmd_GameState_CallWildLose,                      // 0x176
    Scrcmd_GameState_WildBattleIsVictory,               // 0x177
    Scrcmd_GameState_WildBattleGetResult,               // 0x178
    Scrcmd_Encount_CallCaptureDemo,                     // 0x179
    Scrcmd_FieldGimmickGate_GimmickGateViewBoard,       // 0x17a
    func_ov012_02157bdc,                                // 0x17b
    Scrcmd_Gym_RollerCoasterCallSwitch,                 // 0x17c
    Scrcmd_Gym_RollerCoasterSetCarIntroSeen,            // 0x17d
    NULL,                                               // 0x17e
    NULL,                                               // 0x17f
    NULL,                                               // 0x180
    NULL,                                               // 0x181
    NULL,                                               // 0x182
    NULL,                                               // 0x183
    NULL,                                               // 0x184
    NULL,                                               // 0x185
    Scrcmd_Gym_DriftveilGymPrepareFreight,              // 0x186
    Scrcmd_Gym_DriftveilLiftPlaceAtBottom,              // 0x187
    NULL,                                               // 0x188
    NULL,                                               // 0x189
    NULL,                                               // 0x18a
    NULL,                                               // 0x18b
    Scrcmd_Gym_DriftveilGymCallElevator,                // 0x18c
    Scrcmd_Gym_DriftveilGymCallFreight,                 // 0x18d
    Scrcmd_Gym_DriftveilLiftCallMove,                   // 0x18e
    Scrcmd_Gym_IcirrusGymCallSwitchAnim,                // 0x18f
    Scrcmd_Gym_IcirrusGymCallFlipSwitch,                // 0x190
    Scrcmd_Gym_IcirrusGymToggleMovementAtPlace,         // 0x191
    NULL,                                               // 0x192
    NULL,                                               // 0x193
    NULL,                                               // 0x194
    NULL,                                               // 0x195
    NULL,                                               // 0x196
    Scrcmd_ProcFld_CGearPowerOn,                        // 0x197
    Scrcmd_Field_FieldSubscreenChange,                  // 0x198
    Scrcmd_Field_FieldSubscreenReturn,                  // 0x199
    Scrcmd_PokeDisp_CGearControlWarning,                // 0x19a
    Scrcmd_FldWipe3dobj_PlayFieldEffect,                // 0x19b
    Scrcmd_FldWipe3dobj_PlayHMCutInEffect,              // 0x19c
    Scrcmd_FldWipe3dobj_PlayAlderFlyEffect,             // 0x19d
    NULL,                                               // 0x19e
    NULL,                                               // 0x19f
    NULL,                                               // 0x1a0
    Scrcmd_RoyalUnova_PleasureBoatGetRoomInfo,          // 0x1a1
    Scrcmd_RoyalUnova_CallRoyalUnovaView,               // 0x1a2
    Scrcmd_Fade_FadeInBlackQ,                           // 0x1a3
    Scrcmd_Fade_FadeOutBlackQ,                          // 0x1a4
    Scrcmd_Fade_FadeInWhiteQ,                           // 0x1a5
    Scrcmd_Fade_FadeOutWhiteQ,                          // 0x1a6
    Scrcmd_Fade_FadeWait,                               // 0x1a7
    Scrcmd_Fade_FadeInBlackQ_01A8,                      // 0x1a8
    NULL,                                               // 0x1a9
    Scrcmd_Fade_CallSeasonBanner,                       // 0x1aa
    Scrcmd_Fade_FadeInBlack,                            // 0x1ab
    Scrcmd_Fade_FadeOutBlack,                           // 0x1ac
    Scrcmd_Fade_FadeInWhite,                            // 0x1ad
    Scrcmd_Fade_FadeOutWhite,                           // 0x1ae
    Scrcmd_FldBattle_RegisterAllowedSpecies,            // 0x1af
    Scrcmd_FldBattle_CheckPartyRegulation,              // 0x1b0
    Scrcmd_FldeffCut_CallLeagueLiftWarp,                // 0x1b1
    Scrcmd_Encount_SpawnMovePokemonIfInactive,          // 0x1b2
    Scrcmd_SpPokeGimmick_ReadSaveByte,                  // 0x1b3
    Scrcmd_PokemonMisc_FieldTradeStart,                 // 0x1b4
    Scrcmd_PokemonMisc_FieldTradeCheck,                 // 0x1b5
    NULL,                                               // 0x1b6
    NULL,                                               // 0x1b7
    NULL,                                               // 0x1b8
    NULL,                                               // 0x1b9
    NULL,                                               // 0x1ba
    NULL,                                               // 0x1bb
    NULL,                                               // 0x1bc
    NULL,                                               // 0x1bd
    NULL,                                               // 0x1be
    NULL,                                               // 0x1bf
    NULL,                                               // 0x1c0
    Scrcmd_Misc_ElevatorSetTablePtr,                    // 0x1c1
    Scrcmd_Misc_ElevatorBuildListMenu,                  // 0x1c2
    Scrcmd_Misc_ElevatorChangeMap,                      // 0x1c3, which swan's name numbers 0x1c2
    Scrcmd_PokemonMisc_PokeDexIsComplete,               // 0x1c4
    Scrcmd_PokemonMisc_PokeDexGetEvaluationParams,      // 0x1c5
    Scrcmd_GameState_PokeDexGiveNational,               // 0x1c6
    Scrcmd_GameState_PokeDexHaveNational,               // 0x1c7
    Scrcmd_GameState_PokeDexEnable,                     // 0x1c8
    Scrcmd_Ndemo_NDemoStart,                            // 0x1c9
    Scrcmd_Ndemo_NDemoEnd,                              // 0x1ca
    Scrcmd_Ndemo_NDemoReadyTalkMotion,                  // 0x1cb
    Scrcmd_Misc_ElevatorGetMysteryEntryCount,           // 0x1cc
    Scrcmd_Misc_ElevatorRegisterBirthdayEntry,          // 0x1cd
    Scrcmd_UnityTower_UnityTowerGetTrainerClass,        // 0x1ce
    Scrcmd_UnityTower_UnityTowerGetStateParam,          // 0x1cf
    Scrcmd_UnityTower_UnityTowerCallFloorSelect,        // 0x1d0
    Scrcmd_PokemonMisc_MoveTutorCheckParty,             // 0x1d1
    Scrcmd_PokemonMisc_MoveTutorCheckPkm,               // 0x1d2
    Scrcmd_PokemonMisc_MoveTutorGetMoveID,              // 0x1d3
    Scrcmd_PokemonMisc_MoveTutorCallPokeSelect,         // 0x1d4
    Scrcmd_Pokemon_MoveReminderCheckPkm,                // 0x1d5
    Scrcmd_Proc_MoveReminderCallMoveSelect,             // 0x1d6
    Scrcmd_GameState_ObjInitPointGPos,                  // 0x1d7
    Scrcmd_GameState_ObjInitWarpGPos,                   // 0x1d8
    Scrcmd_GameState_ObjInitNPCGPos,                    // 0x1d9
    Scrcmd_PhraseSelect_CallPhraseSelect,               // 0x1da
    func_ov033_02177d78,                                // 0x1db
    Scrcmd_Musical_DreamWorldItems,                     // 0x1dc
    Scrcmd_FldeffCut_BroadcastGameBeacon,               // 0x1dd
    Scrcmd_FldeffCut_PassPowerPopDepletedID,            // 0x1de
    Scrcmd_FldeffCut_CallPassPowerFlash,                // 0x1df
    Scrcmd_Stadium_StadiumSetupActorsDouble,            // 0x1e0
    Scrcmd_Stadium_StadiumLoadTrainerTable,             // 0x1e1
    Scrcmd_Stadium_StadiumFreeTrainerTable,             // 0x1e2
    Scrcmd_Stadium_StadiumSetupActorSingle,             // 0x1e3
    Scrcmd_Stadium_StadiumSetupActorsTriple,            // 0x1e4, which swan's name numbers 0x1e0
    Scrcmd_Stadium_StadiumResetTrainerFlags,            // 0x1e5
    Scrcmd_TrialHouse_TrialHouseWorkInit,               // 0x1e6
    Scrcmd_TrialHouse_TrialHouseWorkDelete,             // 0x1e7
    Scrcmd_TrialHouse_TrialHousePrepareParty,           // 0x1e8
    Scrcmd_TrialHouse_TrialHouseCallTeamSelect,         // 0x1e9
    Scrcmd_TrialHouse_TrialHouseSetTrainer,             // 0x1ea
    Scrcmd_TrialHouse_TrialHouseMsgDisp,                // 0x1eb
    Scrcmd_TrialHouse_TrialHouseStartBattle,            // 0x1ec
    Scrcmd_TrialHouse_TrialHouseSetDownloadMode,        // 0x1ed
    Scrcmd_TrialHouse_TrialHouseSetBattleTestRank,      // 0x1ee
    Scrcmd_TrialHouse_TrialHouseGetBattleTestRank,      // 0x1ef
    Scrcmd_TrialHouse_TrialHouseDownloadExamination,    // 0x1f0
    Scrcmd_TrialHouse_TrialHouseCalcPointsStars,        // 0x1f1
    Scrcmd_TrialHouse_TrialHouseGetExaminationType,     // 0x1f2
    Scrcmd_TrialHouse_TrialHouseSaveData,               // 0x1f3
    Scrcmd_TrialHouse_TrialHouseCallResults,            // 0x1f4
    Scrcmd_TrialHouse_TrialHouseGetRecordMask,          // 0x1f5
    Scrcmd_PhraseInput_CheckWifiListFlags,              // 0x1f6
    Scrcmd_PhraseInput_WifiListOperation,               // 0x1f7
    Scrcmd_PhraseInput_SetWifiFriendNameWord,           // 0x1f8
    Scrcmd_Field_CheckPmsWordPair,                      // 0x1f9
    Scrcmd_PepQuiz_PepQuizGenerate,                     // 0x1fa
    Scrcmd_Misc_ItemCollectorCheckGroup,                // 0x1fb
    Scrcmd_Misc_ItemCollectorGetPrice,                  // 0x1fc
    Scrcmd_Survey_SurveyStart,                          // 0x1fd
    Scrcmd_Survey_SurveyClear,                          // 0x1fe
    Scrcmd_Survey_SurveyGetCurrentQuestionID,           // 0x1ff
    Scrcmd_Survey_SurveyGetCurrentAnswerIDs,            // 0x200
    Scrcmd_Survey_SurveyGetPopularOptionMsgID,          // 0x201
    Scrcmd_Survey_SurveyGetQuestionState,               // 0x202
    Scrcmd_Survey_SurveyGetCurrentRemaining,            // 0x203
    Scrcmd_Survey_SurveyGetTime,                        // 0x204
    Scrcmd_ProcFld_CallGreetingPhraseInput,             // 0x205
    Scrcmd_ProcFld_CallThanksPhraseInput,               // 0x206
    Scrcmd_Word_WordSetPasserbyCount,                   // 0x207
    Scrcmd_Word_WordSetThankedCount,                    // 0x208
    Scrcmd_Survey_SurveyWriteOwnAnswer,                 // 0x209
    Scrcmd_Survey_SurveyShowRankMessage,                // 0x20a
    Scrcmd_Survey_SurveyGetRank,                        // 0x20b
    Scrcmd_Survey_SurveyRaiseRank,                      // 0x20c
    Scrcmd_SpPokeGimmick_PokePartyFindEx,               // 0x20d
    Scrcmd_SpPokeGimmick_SpPokeJump,                    // 0x20e
    Scrcmd_SpPokeGimmick_SpPokeAppear,                  // 0x20f
    Scrcmd_SpPokeGimmick_SpPokeWaitAnimEnd,             // 0x210
    Scrcmd_SpPokeGimmick_SpPokeWaitFrame,               // 0x211
    Scrcmd_ActorMove_ActorMoveToHeight,                 // 0x212
    Scrcmd_ActorMove_ActorHoldPos,                      // 0x213
    Scrcmd_PokeDisp_CallPokemonPreview,                 // 0x214
    Scrcmd_SpPokeGimmick_SetSpecialTransferUsed,        // 0x215
    Scrcmd_PokeDisp_IsOneShotDRObtained,                // 0x216
    Scrcmd_EntreeForest_MapChangeEntreeForest,          // 0x217
    func_ov012_02164c68,                                // 0x218
    func_ov012_02164bcc,                                // 0x219
    Scrcmd_EntreeForest_WordSetLoadEntreeForestPkmName, // 0x21a
    Scrcmd_EntreeForest_EntreeForestSpawnAllPkm,        // 0x21b
    Scrcmd_EntreeForest_EntreeForestStartBattle,        // 0x21c
    Scrcmd_EntreeForest_EntreeForestRemovePkm,          // 0x21d
    Scrcmd_Encount_FishingChallengeGetRandomPkm,        // 0x21e
    func_ov012_02156104,                                // 0x21f
    func_ov012_02156128,                                // 0x220
    Scrcmd_Misc_AnvilleTrainGetModelID,                 // 0x221
    Scrcmd_Misc_AnvilleTrainGetDailyIndex,              // 0x222
    Scrcmd_PokemonMisc_PokePartyGetHiddenPowerType,     // 0x223
    Scrcmd_PokemonMisc_PokePartyGetIV,                  // 0x224
    Scrcmd_PokemonMisc_ItemGetRandomOwnedTMMove,        // 0x225
    Scrcmd_Enceff_StartCountOtherOtIdsTask,             // 0x226
    Scrcmd_PokeDisp_RecordAdd,                          // 0x227
    Scrcmd_PokeDisp_RecordGet,                          // 0x228
    NULL,                                               // 0x229
    Scrcmd_PokeDisp_TVGenProgramMsgID,                  // 0x22a
    Scrcmd_PokeDisp_StartSwapNpcEvent,                  // 0x22b
    NULL,                                               // 0x22c
    Scrcmd_FldWipe3dobj_FieldEffect,                    // 0x22d
    Scrcmd_FieldActor_SetActorZoneDeleteOff,            // 0x22e
    Scrcmd_PokemonMisc_WordSetLoadPastTradePkmName,     // 0x22f
    Scrcmd_Msg_MsgPlaceSignFont4,                       // 0x230
    Scrcmd_ProcFld_CallGameManual,                      // 0x231
    Scrcmd_PokemonMisc_ControlLegendMon,                // 0x232
    Scrcmd_Leaves_UnityTowerGetVisitorCountry,          // 0x233
    Scrcmd_FieldActor_ActorFallDownToXZ,                // 0x234
    Scrcmd_Sound_BGMPlayEx,                             // 0x235
    Scrcmd_Misc_WordSetLoadItemCollectorPrice,          // 0x236
    Scrcmd_Misc_ItemCollectorSell,                      // 0x237
    Scrcmd_Sound_BGMChangeMapEx,                        // 0x238
    Scrcmd_FieldActor_CheckPlayerFormModel,             // 0x239
    Scrcmd_Sound_PokeVoicePlay,                         // 0x23a
    func_ov012_02169c1c,                                // 0x23b
    Scrcmd_SpPoke_NuvemaGimmickPlay,                    // 0x23c
    Scrcmd_SpPoke_NuvemaGimmickWaitAnim,                // 0x23d
    Scrcmd_DayCare_DayCareGetSexForNamePrint,           // 0x23e
    Scrcmd_PlaceName_CallPlaceNameDisp,                 // 0x23f
    Scrcmd_Sound_FadeBgmVolume,                         // 0x240
    Scrcmd_Sound_RestoreBgmVolume,                      // 0x241
    NULL,                                               // 0x242
    NULL,                                               // 0x243
    NULL,                                               // 0x244
    Scrcmd_Sound_BGMFadeOut,                            // 0x245
    Scrcmd_Sound_BGMFadeOutAll,                         // 0x246
    Scrcmd_Mapchange_MapChangeRail,                     // 0x247
    Scrcmd_FieldActor_PlayerMoveToYAsync_0248,          // 0x248
    Scrcmd_Stadium_GetRandomDistinctSix,                // 0x249
    Scrcmd_Network_FieldSubscreenIsEnabled,             // 0x24a
    Scrcmd_Network_FieldSubscreenDisable,               // 0x24b
    Scrcmd_Sound_BGMAmbienceResume,                     // 0x24c
    Scrcmd_Gym_RollerCoasterAmbienceStart,              // 0x24d
    Scrcmd_Pokemon_PokePartyIsFullPP,                   // 0x24e
    Scrcmd_FieldActor_ActorWalkRoute,                   // 0x24f
    Scrcmd_PairSys_ActorPairSet,                        // 0x250
    Scrcmd_PairSys_ActorPairEnd,                        // 0x251
    Scrcmd_PairSys_ActorPairGetTrID,                    // 0x252
    Scrcmd_PairSys_ActorPairSetMoveEnable,              // 0x253
    NULL,                                               // 0x254
    NULL,                                               // 0x255
    NULL,                                               // 0x256
    NULL,                                               // 0x257
    NULL,                                               // 0x258
    NULL,                                               // 0x259
    func_ov012_02157b7c,                                // 0x25a
    Scrcmd_Gym_GymElecReapplyProgress,                  // 0x25b
    Scrcmd_Gym_GymElecSetFollower,                      // 0x25c
    Scrcmd_Gym_GymElecSetProgress,                      // 0x25d
    NULL,                                               // 0x25e
    Scrcmd_Gym_GymElecSetStage,                         // 0x25f
    NULL,                                               // 0x260
    NULL,                                               // 0x261
    Scrcmd_HollowRival_HollowRivalSetValue,             // 0x262
    Scrcmd_HollowRival_HollowRivalSetFlag,              // 0x263
    func_ov012_0216a748,                                // 0x264
    Scrcmd_ProcFld_CallXTransceiverSecret,              // 0x265
    Scrcmd_HollowRival_HollowRivalGetCounter,           // 0x266
    func_ov012_0216a7cc,                                // 0x267
    Scrcmd_Gym_GymInsectPlayObject,                     // 0x268
    Scrcmd_Gym_GymInsectShowEffect,                     // 0x269
    NULL,                                               // 0x26a
    NULL,                                               // 0x26b
    Scrcmd_Word_WordSetMedalName,                       // 0x26c
    Scrcmd_Word_WordSetMedalRank,                       // 0x26d
    Scrcmd_Medal_MedalGetCount,                         // 0x26e
    NULL,                                               // 0x26f
    NULL,                                               // 0x270
    Scrcmd_Medal_MedalAcknowledge,                      // 0x271
    Scrcmd_Medal_MedalGetSpinTradeActor,                // 0x272
    Scrcmd_Medal_MedalGive,                             // 0x273
    Scrcmd_Funfest_FunfestMissionStart,                 // 0x274
    Scrcmd_Word_WordSetFunfestMission,                  // 0x275
    Scrcmd_Funfest_FunfestMissionBroadcast,             // 0x276
    Scrcmd_Funfest_FunfestActorDelete,                  // 0x277
    Scrcmd_Msg_FunfestDispSalesmanMessage,              // 0x278
    Scrcmd_Funfest_FunfestBGMReturn,                    // 0x279
    Scrcmd_Funfest_FunfestGetGenericInfo,               // 0x27a
    Scrcmd_Funfest_FunfestGetItemExchangeInfo,          // 0x27b
    Scrcmd_Funfest_FunfestGetItemSaleInfo,              // 0x27c
    Scrcmd_Funfest_FunfestGetPokemonQuizInfo,           // 0x27d
    Scrcmd_Funfest_FunfestGetPokemonQuizSpecies,        // 0x27e
    Scrcmd_Funfest_FunfestGetPokemonQuizBogusSpecies,   // 0x27f
    NULL,                                               // 0x280
    NULL,                                               // 0x281
    NULL,                                               // 0x282
    NULL,                                               // 0x283
    Scrcmd_ProcFld_CallPlaceSelect,                     // 0x284
    Scrcmd_ProcFld_CallWordSetPokeNameInput,            // 0x285
    NULL,                                               // 0x286
    NULL,                                               // 0x287
    NULL,                                               // 0x288
    NULL,                                               // 0x289
    Scrcmd_Mapchange_MapChangeFlyWarp,                  // 0x28a
    Scrcmd_Funfest_CallEntralinkWarpOut,                // 0x28b
    NULL,                                               // 0x28c
    NULL,                                               // 0x28d
    Scrcmd_Gym_MistraltonGymFireCannon,                 // 0x28e
    Scrcmd_ProcFld_CallRivalNameInput,                  // 0x28f
    Scrcmd_Word_WordSetLoadRivalName,                   // 0x290
    Scrcmd_Gym_VirbankGymStopModelAnimations,           // 0x291
    Scrcmd_Gym_VirbankGymSetActorFlag,                  // 0x292
    Scrcmd_HiddenHollow_HiddenHollowCallWarpIn,         // 0x293
    Scrcmd_HiddenHollow_HiddenHollowGetParam,           // 0x294
    Scrcmd_HiddenHollow_HiddenHollowReset,              // 0x295
    Scrcmd_HiddenHollow_HiddenHollowCallWarpOut,        // 0x296
    Scrcmd_Encount_CallWildBattleEx,                    // 0x297
    Scrcmd_Word_WordSetEmptyFileMessage,                // 0x298
    Scrcmd_Word_WordSetLoadAbility,                     // 0x299
    Scrcmd_Word_WordSetLoadNature,                      // 0x29a
    Scrcmd_Word_WordSetLoadJoinAvenueName,              // 0x29b
    NULL,                                               // 0x29c
    NULL,                                               // 0x29d
    Scrcmd_Medal_MedalGetFieldEffectID,                 // 0x29e
    Scrcmd_Medal_MedalDiscover,                         // 0x29f
    Scrcmd_Medal_MedalIsObtained,                       // 0x2a0
    Scrcmd_Medal_MedalGetMostCompleteCategory,          // 0x2a1
    Scrcmd_Gym_HumilauGymPlayLeaderSplash,              // 0x2a2
    Scrcmd_Gym_HumilauGymPlayLeaderWaves,               // 0x2a3
    func_ov012_02157bb4,                                // 0x2a4
    Scrcmd_Gym_OpelucidGymCallMove,                     // 0x2a5
    Scrcmd_Gym_OpelucidGymRefreshVars,                  // 0x2a6
    Scrcmd_Gym_OpelucidGymCallButton,                   // 0x2a7
    Scrcmd_Gym_OpelucidGymFreeHeap,                     // 0x2a8
    Scrcmd_Gym_OpelucidGymCreateHeap,                   // 0x2a9
    NULL,                                               // 0x2aa
    NULL,                                               // 0x2ab
    Scrcmd_FldeffLeaves_CallLeafPileStuck,              // 0x2ac
    NULL,                                               // 0x2ad
    Scrcmd_Keysystem_GameGetMyCity,                     // 0x2ae
    Scrcmd_Keysystem_GameGetDifficulty,                 // 0x2af
    Scrcmd_Keysystem_CallUnovaLinkKeyUnlock,            // 0x2b0
    Scrcmd_Keysystem_KeyInfoGetChamberKey,              // 0x2b1
    Scrcmd_DownloadData_MemoryLinkCheckFlag,            // 0x2b2
    Scrcmd_DownloadData_MemoryLinkGetVar,               // 0x2b3
    Scrcmd_DownloadData_MemoryLinkIsPlayerLinked,       // 0x2b4
    Scrcmd_DownloadData_WordSetMemoryLinkPlayerName,    // 0x2b5
    Scrcmd_DownloadData_MemoryLinkCheckMusical,         // 0x2b6
    Scrcmd_DownloadData_MemoryLinkAddMusicalProps,      // 0x2b7
    Scrcmd_Pedometer_PedometerStart,                    // 0x2b8
    Scrcmd_Pedometer_PedometerEnd,                      // 0x2b9
    Scrcmd_Pedometer_PedometerGet,                      // 0x2ba
    Scrcmd_Gym_Gym0601FanAmbienceStart,                 // 0x2bb
    Scrcmd_PokeDisp_TVCheckCommercial,                  // 0x2bc
    Scrcmd_PokeDisp_TVGenCommercialMsgID,               // 0x2bd
    Scrcmd_FieldActor_ActorMoveLinear,                  // 0x2be
    NULL,                                               // 0x2bf
    Scrcmd_PokemonMisc_FieldTradeGetSpecies,            // 0x2c0
    NULL,                                               // 0x2c1
    Scrcmd_Encount_RepelRearm,                          // 0x2c2
    func_ov012_0216a7f4,                                // 0x2c3
    Scrcmd_HiddenHollow_HiddenHollowSetPeriod,          // 0x2c4
    Scrcmd_PokeDisp_JoinAvenueRecordActivity,           // 0x2c5
    Scrcmd_JoinAvenueStore_JoinAvenueStoreStart,        // 0x2c6
    Scrcmd_JoinAvenueStore_JoinAvenueStoreEnd,          // 0x2c7, which swan's name numbers 0x2c6
    NULL,                                               // 0x2c8
    NULL,                                               // 0x2c9
    Scrcmd_ProcFld_CallHappyPhraseInput,                // 0x2ca
    Scrcmd_JoinAvenueStore_PokewoodGetLevel,            // 0x2cb
    NULL,                                               // 0x2cc
    NULL,                                               // 0x2cd
    NULL,                                               // 0x2ce
    Scrcmd_Leaves_PokeDexCheckHabitatList,              // 0x2cf
    Scrcmd_GameState_PokeDexEnableHabitatList,          // 0x2d0
    Scrcmd_Leaves_KeyInfoGetMyCityProgress,             // 0x2d1
    Scrcmd_GameState_FieldOpenRestoreLCD,               // 0x2d2
    Scrcmd_HollowRival_HollowRivalGetValue,             // 0x2d3
    Scrcmd_Item_ItemGetTMCount,                         // 0x2d4
    Scrcmd_Leaves_WbtIsTrainerBeaten,                   // 0x2d5
    NULL,                                               // 0x2d6
    Scrcmd_Leaves_GetWifiHistoryCountryCount,           // 0x2d7
    Scrcmd_Leaves_MarkPartnerCandidate,                 // 0x2d8
    Scrcmd_Leaves_UnityTowerGetVisitorParam,            // 0x2d9
    Scrcmd_Leaves_UnityTowerSetRatingLevel,             // 0x2da
    Scrcmd_Leaves_UnityTowerSetFloor,                   // 0x2db
    Scrcmd_Leaves_UnityTowerInitVisitorMessage,         // 0x2dc
    Scrcmd_Leaves_UnityTowerGetVisitorCount,            // 0x2dd
    Scrcmd_Leaves_UnityTowerSetHobby,                   // 0x2de
    Scrcmd_Leaves_UnityTowerGetHobby,                   // 0x2df
    NULL,                                               // 0x2e0
    Scrcmd_Medal_MedalDiscoverInitial,                  // 0x2e1
    NULL,                                               // 0x2e2
    Scrcmd_JoinAvenueStore_LensFlareRequest,            // 0x2e3
    NULL,                                               // 0x2e4
    Scrcmd_HiddenHollow_HiddenHollowSet,                // 0x2e5
    Scrcmd_HiddenHollow_HiddenHollowCreateEvents,       // 0x2e6
    NULL,                                               // 0x2e7
    Scrcmd_Fade_CallFieldFadeOut,                       // 0x2e8
    Scrcmd_Fade_CallFieldFadeIn,                        // 0x2e9
    Scrcmd_Pokemon_PokePartyAddNPoke,                   // 0x2ea
    NULL,                                               // 0x2eb
    NULL,                                               // 0x2ec
    Scrcmd_GameState_SetSoftResetBlocked,               // 0x2ed
    Scrcmd_Musical_MusicalIsPropOwned,                  // 0x2ee
    Scrcmd_Musical_MusicalGetOwnedPropCount,            // 0x2ef
    NULL,                                               // 0x2f0
    Scrcmd_GameState_SetOneShotDRObtained,              // 0x2f1
    Scrcmd_GameState_IsOneShotDRObtained_02F2,          // 0x2f2
};

// MWCC lays the file's data out by size, so the count comes first in the ROM, ahead of the tables
const u32 EVCMD_MAX = NELEMS(EVCMD_TABLE);

BOOL Scrcmd_Vm_VMNop(VM *vm, FieldScriptEnv *env) {
    return FALSE;
}

BOOL Scrcmd_Vm_VMNop2(VM *vm, FieldScriptEnv *env) {
    return FALSE;
}

BOOL Scrcmd_Vm_VMHalt(VM *vm, FieldScriptEnv *env) {
    VM_Halt(vm);
    return TRUE;
}

BOOL swapToScrcmdEnvirDecPauseCtr(VM *vm, void *env) {
    return FieldScriptEnv_UpdateWaitCounter(env);
}

BOOL Scrcmd_Vm_VMSleep(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_SetWaitCounter(env, VM_Read16(vm));
    VM_SetNativeCallback(vm, swapToScrcmdEnvirDecPauseCtr);
    return TRUE;
}

BOOL Scrcmd_Vm_VMCall(VM *vm, FieldScriptEnv *env) {
    u32 offset = VM_Read32(vm);

    VM_Call(vm, vm->pc + offset);
    return FALSE;
}

BOOL Scrcmd_Vm_VMReturn(VM *vm, FieldScriptEnv *env) {
    VM_Return(vm);
    return FALSE;
}

BOOL Scrcmd_Vm_DebugPrint(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetGameData(env);
    ScriptReadAny(vm, env);
    return FALSE;
}

BOOL Scrcmd_Vm_DebugStack(VM *vm, FieldScriptEnv *env) {
    VM_StackPop(vm);
    ScriptReadAny(vm, env);
    return FALSE;
}

BOOL Scrcmd_Vm_VMRegSet8(VM *vm, FieldScriptEnv *env) {
    u8 index = VM_Read8(vm);
    u8 value = VM_Read8(vm);

    vm->work[index] = value;
    return FALSE;
}

BOOL Scrcmd_Vm_VMRegSet32(VM *vm, FieldScriptEnv *env) {
    u8 index = VM_Read8(vm);
    u32 value = VM_Read32(vm);

    vm->work[index] = value;
    return FALSE;
}

BOOL Scrcmd_Vm_VMRegMov(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Vm_VMRegCmp8(VM *vm, FieldScriptEnv *env) {
    u8 left = vm->work[VM_Read8(vm)];
    u8 right = vm->work[VM_Read8(vm)];

    vm->cmpResult = VMCmp(left, right);
    return FALSE;
}

BOOL Scrcmd_Vm_VMRegCmpConst8(VM *vm, FieldScriptEnv *env) {
    u8 left = vm->work[VM_Read8(vm)];
    u8 right = VM_Read8(vm);

    vm->cmpResult = VMCmp(left, right);
    return FALSE;
}

BOOL Scrcmd_Vm_WorkCmpConst(VM *vm, FieldScriptEnv *env) {
    u16 left = *ScriptReadVar(vm, env);
    u16 right = VM_Read16(vm);

    vm->cmpResult = VMCmp(left, right);
    return FALSE;
}

BOOL Scrcmd_Vm_WorkCmpWork(VM *vm, FieldScriptEnv *env) {
    u16 *left = ScriptReadVar(vm, env);
    u16 *right = ScriptReadVar(vm, env);

    vm->cmpResult = VMCmp(*left, *right);
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackPushConst(VM *vm, FieldScriptEnv *env) {
    VM_StackPush(vm, VM_Read16(vm));
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackPush(VM *vm, FieldScriptEnv *env) {
    VM_StackPush(vm, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackPop(VM *vm, FieldScriptEnv *env) {
    u32 value = VM_StackPop(vm);

    *ScriptReadVar(vm, env) = value;
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackDiscard(VM *vm, FieldScriptEnv *env) {
    VM_StackPop(vm);
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackAdd(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left + right);
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackSub(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left - right);
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackMul(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left * right);
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackDiv(VM *vm, FieldScriptEnv *env) {
    u32 right = VM_StackPop(vm);
    u32 left = VM_StackPop(vm);

    VM_StackPush(vm, left / right);
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackPushFlag(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);

    VM_StackPush(vm, EventWork_FlagGet(eventWork, flag));
    return FALSE;
}

BOOL Scrcmd_Vm_VMStackCmp(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Vm_RTCallGlobalAsync(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Vm_RTCallGlobal(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Vm_RTEndGlobal(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    VM_Halt(vm);
    return TRUE;
}

BOOL Scrcmd_Vm_VMJump(VM *vm, FieldScriptEnv *env) {
    u32 offset = VM_Read32(vm);

    VM_Jump(vm, vm->pc + offset);
    return FALSE;
}

BOOL Scrcmd_Vm_VMJumpIf(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Vm_VMCallIf(VM *vm, FieldScriptEnv *env) {
    u8 condition = VM_Read8(vm);
    u32 offset = VM_Read32(vm);

    if (VM_CMP_LUT[condition][vm->cmpResult] == 1) {
        VM_Call(vm, vm->pc + offset);
    }
    return FALSE;
}

BOOL Scrcmd_Vm_RTReserveScript(VM *vm, FieldScriptEnv *env) {
    u16 scriptId = VM_Read16(vm);
    FieldStatus *status = GameData_GetFieldStatus(FieldScriptEnv_GetGameData(env));

    FieldStatus_ReserveScript(status, scriptId);
    return FALSE;
}

BOOL Scrcmd_Vm_FieldGetContinueFlag(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    FieldStatus *status = GameData_GetFieldStatus(FieldScriptEnv_GetGameData(env));

    *value = FieldStatus_CheckContinueFlag(status);
    return FALSE;
}

BOOL Scrcmd_Vm_FlagSet(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);

    EventWork_FlagSet(eventWork, flag);
    return FALSE;
}

BOOL Scrcmd_Vm_FlagReset(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);

    EventWork_FlagReset(eventWork, flag);
    return FALSE;
}

BOOL Scrcmd_Vm_FlagGet(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 flag = ScriptReadAny(vm, env);
    u16 *value = ScriptReadVar(vm, env);

    *value = EventWork_FlagGet(eventWork, flag);
    return FALSE;
}

BOOL Scrcmd_Vm_WorkAdd(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value += operand;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkSub(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value -= operand;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkMul(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value *= operand;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkDiv(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value /= operand;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkMod(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value %= operand;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkAnd(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value &= operand;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkOr(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 operand = ScriptReadAny(vm, env);

    *value |= operand;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkSetConst(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);

    *value = VM_Read16(vm);
    return FALSE;
}

BOOL Scrcmd_Vm_WorkGet(VM *vm, FieldScriptEnv *env) {
    u16 *dst = ScriptReadVar(vm, env);
    u16 *src = ScriptReadVar(vm, env);

    *dst = *src;
    return FALSE;
}

BOOL Scrcmd_Vm_WorkSet(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);

    *value = ScriptReadAny(vm, env);
    return FALSE;
}

BOOL Scrcmd_Vm_ActorsPauseAll(VM *vm, FieldScriptEnv *env) {
    return PauseEventMModels(vm, env);
}

BOOL Scrcmd_Vm_ActorsUnpauseAll(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Vm_FinishAllEvents(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Vm_ABKeyWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, testAB);
    return TRUE;
}

BOOL ScriptNative_LastKeyWait(VM *vm, void *env) {
    return (GCTX_HIDGetPressedKeys() & 0xf3) != 0;
}

BOOL Scrcmd_Vm_LastKeyWait(VM *vm, FieldScriptEnv *env) {
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
