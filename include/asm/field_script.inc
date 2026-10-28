// Macros for the field scripts, written by tools/script/field_script.py inc

#include "constants/abilities.h"
#include "constants/field_script.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/trades.h"
#include "constants/trainers.h"
#include "constants/types.h"
#include "constants/vars.h"
#include "constants/zones.h"

    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs
    .data

    .macro FieldCommand id
    .2byte \id
    .endm

    // The offset of a script, at the start of a script file
    .macro ScriptEntry label
    .4byte \label - (. + 4)
    .endm

    .macro ScriptEntriesEnd
    .2byte 0xfd13
    .endm

    // Movement data for ActorCmdExec
    .macro Move action, count
    .2byte \action
    .2byte \count
    .endm

    .macro MoveEnd
    .2byte 0xfe
    .2byte 0
    .endm

    // A map script table
    .macro MapScript type, script
    .2byte \type
    .4byte \script
    .endm

    .macro MapScriptConditions label
    .2byte 1
    .4byte \label - (. + 4)
    .endm

    .macro MapScriptsEnd
    .2byte 0
    .endm

    .macro MapScriptCondition var, value, script
    .2byte \var
    .2byte \value
    .2byte \script
    .endm

    .macro MapScriptConditionsEnd
    .2byte 0
    .endm

    .macro VMNop
    FieldCommand 0
    .endm

    .macro VMNop2
    FieldCommand 1
    .endm

    .macro VMHalt
    FieldCommand 2
    .endm

    .macro VMSleep a0
    FieldCommand 3
    .2byte \a0
    .endm

    .macro VMCall a0
    FieldCommand 4
    .4byte \a0 - (. + 4)
    .endm

    .macro VMReturn
    FieldCommand 5
    .endm

    .macro DebugPrint a0
    FieldCommand 6
    .2byte \a0
    .endm

    .macro DebugStack a0
    FieldCommand 7
    .2byte \a0
    .endm

    .macro VMStackPushConst a0
    FieldCommand 8
    .2byte \a0
    .endm

    .macro VMStackPush a0
    FieldCommand 9
    .2byte \a0
    .endm

    .macro VMStackPop a0
    FieldCommand 10
    .2byte \a0
    .endm

    .macro VMStackDiscard
    FieldCommand 11
    .endm

    .macro VMStackAdd
    FieldCommand 12
    .endm

    .macro VMStackSub
    FieldCommand 13
    .endm

    .macro VMStackMul
    FieldCommand 14
    .endm

    .macro VMStackDiv
    FieldCommand 15
    .endm

    .macro VMStackPushFlag a0
    FieldCommand 16
    .2byte \a0
    .endm

    .macro VMStackCmp a0
    FieldCommand 17
    .2byte \a0
    .endm

    .macro WorkAnd a0, a1
    FieldCommand 18
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkOr a0, a1
    FieldCommand 19
    .2byte \a0
    .2byte \a1
    .endm

    .macro VMRegSet8 a0, a1
    FieldCommand 20
    .byte \a0
    .byte \a1
    .endm

    .macro VMRegSet32 a0, a1
    FieldCommand 21
    .byte \a0
    .4byte \a1
    .endm

    .macro VMRegMov a0, a1
    FieldCommand 22
    .byte \a0
    .byte \a1
    .endm

    .macro VMRegCmp8 a0, a1
    FieldCommand 23
    .byte \a0
    .byte \a1
    .endm

    .macro VMRegCmpConst8 a0, a1
    FieldCommand 24
    .byte \a0
    .byte \a1
    .endm

    .macro WorkCmpConst a0, a1
    FieldCommand 25
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkCmpWork a0, a1
    FieldCommand 26
    .2byte \a0
    .2byte \a1
    .endm

    .macro RTCallGlobalAsync a0
    FieldCommand 27
    .2byte \a0
    .endm

    .macro RTCallGlobal a0
    FieldCommand 28
    .2byte \a0
    .endm

    .macro RTEndGlobal
    FieldCommand 29
    .endm

    .macro VMJump a0
    FieldCommand 30
    .4byte \a0 - (. + 4)
    .endm

    .macro VMJumpIf a0, a1
    FieldCommand 31
    .byte \a0
    .4byte \a1 - (. + 4)
    .endm

    .macro VMCallIf a0, a1
    FieldCommand 32
    .byte \a0
    .4byte \a1 - (. + 4)
    .endm

    .macro RTReserveScript a0
    FieldCommand 33
    .2byte \a0
    .endm

    .macro FieldGetContinueFlag a0
    FieldCommand 34
    .2byte \a0
    .endm

    .macro FlagSet a0
    FieldCommand 35
    .2byte \a0
    .endm

    .macro FlagReset a0
    FieldCommand 36
    .2byte \a0
    .endm

    .macro FlagGet a0, a1
    FieldCommand 37
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkAdd a0, a1
    FieldCommand 38
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkSub a0, a1
    FieldCommand 39
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkSetConst a0, a1
    FieldCommand 40
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkGet a0, a1
    FieldCommand 41
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkSet a0, a1
    FieldCommand 42
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkMul a0, a1
    FieldCommand 43
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkDiv a0, a1
    FieldCommand 44
    .2byte \a0
    .2byte \a1
    .endm

    .macro WorkMod a0, a1
    FieldCommand 45
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorsPauseAll
    FieldCommand 46
    .endm

    .macro ActorsUnpauseAll
    FieldCommand 47
    .endm

    .macro FinishAllEvents
    FieldCommand 48
    .endm

    .macro ABKeyWait
    FieldCommand 49
    .endm

    .macro LastKeyWait
    FieldCommand 50
    .endm

    .macro MsgSetAutoscrolls a0
    FieldCommand 51
    .2byte \a0
    .endm

    .macro SystemMsg a0, a1
    FieldCommand 52
    .2byte \a0
    .2byte \a1
    .endm

    .macro SystemMsgAsync a0, a1
    FieldCommand 53
    .2byte \a0
    .2byte \a1
    .endm

    .macro InfoMsgClose
    FieldCommand 54
    .endm

    .macro MsgSetLoadingSpinner a0
    FieldCommand 55
    .2byte \a0
    .endm

    .macro InfoMsg a0, a1
    FieldCommand 56
    .2byte \a0
    .byte \a1
    .endm

    .macro InfoMsgClose_0039
    FieldCommand 57
    .endm

    .macro MultiMsg a0, a1, a2, a3
    FieldCommand 58
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro MsgWinCloseNo a0
    FieldCommand 59
    .2byte \a0
    .endm

    .macro ActorMsg a0, a1, a2, a3, a4
    FieldCommand 60
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro ParentActorMsg a0, a1, a2, a3
    FieldCommand 61
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ActorMsgClose
    FieldCommand 62
    .endm

    .macro MsgWinCloseAll
    FieldCommand 63
    .endm

    .macro MoneyWinDisp a0, a1
    FieldCommand 64
    .2byte \a0
    .2byte \a1
    .endm

    .macro MoneyWinClose
    FieldCommand 65
    .endm

    .macro MoneyWinUpdate
    FieldCommand 66
    .endm

    .macro MsgPlaceSign a0, a1
    FieldCommand 67
    .2byte \a0
    .2byte \a1
    .endm

    .macro MsgPlaceSignClose
    FieldCommand 68
    .endm

    .macro CheckerMsg a0, a1, a2, a3
    FieldCommand 69
    .2byte \a0
    .byte \a1
    .byte \a2
    .2byte \a3
    .endm

    .macro CheckerMsgClose
    FieldCommand 70
    .endm

    .macro YesNoWin a0
    FieldCommand 71
    .2byte \a0
    .endm

    .macro ActorMsgGendered a0, a1, a2, a3, a4, a5
    FieldCommand 72
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro ActorMsgVersioned a0, a1, a2, a3, a4, a5
    FieldCommand 73
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro ScreamMsg a0, a1
    FieldCommand 74
    .2byte \a0
    .byte \a1
    .endm

    .macro MsgWaitAdvance
    FieldCommand 75
    .endm

    .macro WordSetPlayerName a0
    FieldCommand 76
    .byte \a0
    .endm

    .macro WordSetItemName a0, a1
    FieldCommand 77
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetItemNameEx a0, a1, a2, a3
    FieldCommand 78
    .byte \a0
    .2byte \a1
    .2byte \a2
    .byte \a3
    .endm

    .macro WordSetItemNameWithArticle a0, a1
    FieldCommand 79
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetTMMoveName a0, a1
    FieldCommand 80
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetMoveName a0, a1
    FieldCommand 81
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetItemPocketName a0, a1
    FieldCommand 82
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetPartyPokeSpecies a0, a1
    FieldCommand 83
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetPartyPokeName a0, a1
    FieldCommand 84
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetDaycarePokeSpecies a0, a1
    FieldCommand 85
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetPokeTypeName a0, a1
    FieldCommand 86
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetPokeSpecies a0, a1
    FieldCommand 87
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetPokeSpeciesWithArticle a0, a1
    FieldCommand 88
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetPlaceName a0, a1
    FieldCommand 89
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetTrendName a0, a1
    FieldCommand 90
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetDaycarePokeName a0, a1
    FieldCommand 91
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetNumber a0, a1, a2
    FieldCommand 92
    .byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro WordSetMusicalInfo a0, a1, a2
    FieldCommand 93
    .byte \a0
    .byte \a1
    .2byte \a2
    .endm

    .macro WordSetCountry a0, a1
    FieldCommand 94
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetHobbyName a0, a1
    FieldCommand 95
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetPassPowerName a0, a1
    FieldCommand 96
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetTrainerClassName a0, a1
    FieldCommand 97
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetTrainerClassNameWithArticle a0, a1
    FieldCommand 98
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetSurveyAnswer a0, a1
    FieldCommand 99
    .byte \a0
    .2byte \a1
    .endm

    .macro ActorCmdExec a0, a1
    FieldCommand 100
    .2byte \a0
    .4byte \a1 - (. + 4)
    .endm

    .macro ActorCmdWait
    FieldCommand 101
    .endm

    .macro ActorGetMoveCode a0, a1
    FieldCommand 102
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorGetGPos a0, a1, a2
    FieldCommand 103
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PlayerGetGPos a0, a1
    FieldCommand 104
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorNew a0, a1, a2, a3, a4, a5
    FieldCommand 105
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro ActorGetSpawnFlag a0, a1
    FieldCommand 106
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorAdd a0
    FieldCommand 107
    .2byte \a0
    .endm

    .macro ActorDelete a0
    FieldCommand 108
    .2byte \a0
    .endm

    .macro ActorSetGPos a0, a1, a2, a3, a4
    FieldCommand 109
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro PlayerGetDir a0
    FieldCommand 110
    .2byte \a0
    .endm

    .macro PlayerGetActorInFront a0, a1
    FieldCommand 111
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorFindByGPos a0, a1, a2, a3, a4
    FieldCommand 112
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro PlayerGetRailPos a0, a1, a2
    FieldCommand 113
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ActorGetRailPos a0, a1, a2, a3
    FieldCommand 114
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ActorSetMoveCode a0, a1
    FieldCommand 115
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorSetEyeToEye
    FieldCommand 116
    .endm

    .macro PlayerSetSpecialSequence a0
    FieldCommand 117
    .2byte \a0
    .endm

    .macro PlayerMoveToYAsync a0, a1, a2, a3
    FieldCommand 118
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PlayerTurnByTrigger
    FieldCommand 119
    .endm

    .macro PlayerGetExState a0
    FieldCommand 120
    .2byte \a0
    .endm

    .macro ActorGetUserParam a0, a1, a2
    FieldCommand 121
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ActorPlayRailSlipdown a0
    FieldCommand 122
    .2byte \a0
    .endm

    .macro ActorJumpToGPos a0, a1, a2, a3
    FieldCommand 123
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PlayerSetRailPos a0, a1, a2
    FieldCommand 124
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ActorSetRailPos a0, a1, a2, a3
    FieldCommand 125
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ActorPlayTeleportSeq a0
    FieldCommand 126
    .2byte \a0
    .endm

    .macro TrainerEyeGetTrainerID a0, a1
    FieldCommand 127
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerEyeEventInit a0
    FieldCommand 128
    .2byte \a0
    .endm

    .macro TrainerEyeEventStart
    FieldCommand 129
    .endm

    .macro TrainerGetActorID a0, a1
    FieldCommand 130
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerGetBattleType a0
    FieldCommand 131
    .2byte \a0
    .endm

    .macro ActorGetTrainerID a0
    FieldCommand 132
    .2byte \a0
    .endm

    .macro CallTrainerBattle a0, a1, a2
    FieldCommand 133
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro CallTrainerMultiBattle a0, a1, a2, a3
    FieldCommand 134
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro TrainerSayMessage a0, a1, a2
    FieldCommand 135
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro TrainerGetMessageTypes a0, a1, a2
    FieldCommand 136
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro TrainerGetBattleType_008A a0, a1
    FieldCommand 138
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerBGMPlayPush a0
    FieldCommand 139
    .2byte \a0
    .endm

    .macro CallTrainerLose
    FieldCommand 140
    .endm

    .macro TrainerBattleIsVictory a0
    FieldCommand 141
    .2byte \a0
    .endm

    .macro CallTrainerBattleEnd
    FieldCommand 142
    .endm

    .macro TrainerClassBGMPlayPush a0
    FieldCommand 143
    .2byte \a0
    .endm

    .macro TrainerBGMPlay a0
    FieldCommand 144
    .2byte \a0
    .endm

    .macro TrainerGetFieldAction a0, a1
    FieldCommand 146
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerGetPrizeItem a0, a1
    FieldCommand 147
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallTradedPokemonBattle a0, a1, a2, a3
    FieldCommand 148
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro TrainerFlagSet a0
    FieldCommand 149
    .2byte \a0
    .endm

    .macro TrainerFlagReset a0
    FieldCommand 150
    .2byte \a0
    .endm

    .macro TrainerFlagGet a0, a1
    FieldCommand 151
    .2byte \a0
    .2byte \a1
    .endm

    .macro BGMPlay a0
    FieldCommand 152
    .2byte \a0
    .endm

    .macro BGMIsPlaying a0, a1
    FieldCommand 155
    .2byte \a0
    .2byte \a1
    .endm

    .macro BGMChangeMap
    FieldCommand 158
    .endm

    .macro BGMPlayPush a0
    FieldCommand 159
    .2byte \a0
    .endm

    .macro BGMWait
    FieldCommand 160
    .endm

    .macro BGMPush a0
    FieldCommand 161
    .2byte \a0
    .endm

    .macro BGMPop a0, a1
    FieldCommand 162
    .2byte \a0
    .2byte \a1
    .endm

    .macro ISSSwitchEnable a0
    FieldCommand 163
    .2byte \a0
    .endm

    .macro ISSSwitchDisable a0
    FieldCommand 164
    .2byte \a0
    .endm

    .macro ISSSwitchQuery a0, a1
    FieldCommand 165
    .2byte \a0
    .2byte \a1
    .endm

    .macro SEPlay a0
    FieldCommand 166
    .2byte \a0
    .endm

    .macro SEStop
    FieldCommand 167
    .endm

    .macro SEWait
    FieldCommand 168
    .endm

    .macro MEPlay a0
    FieldCommand 169
    .2byte \a0
    .endm

    .macro MEWait
    FieldCommand 170
    .endm

    .macro PVPlay a0, a1
    FieldCommand 171
    .2byte \a0
    .2byte \a1
    .endm

    .macro PVWait
    FieldCommand 172
    .endm

    .macro ListMenuInitCommon a0, a1, a2, a3, a4
    FieldCommand 173
    .byte \a0
    .byte \a1
    .2byte \a2
    .byte \a3
    .2byte \a4
    .endm

    .macro ListMenu_AnchorTopLeft a0, a1, a2, a3, a4
    FieldCommand 174
    .byte \a0
    .byte \a1
    .2byte \a2
    .byte \a3
    .2byte \a4
    .endm

    .macro ListMenuAdd a0, a1, a2
    FieldCommand 175
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ListMenuShow
    FieldCommand 176
    .endm

    .macro ListMenuShow2
    FieldCommand 177
    .endm

    .macro ListMenu_AnchorTopRight a0, a1, a2, a3, a4
    FieldCommand 178
    .byte \a0
    .byte \a1
    .2byte \a2
    .byte \a3
    .2byte \a4
    .endm

    .macro FadeEx a0, a1, a2, a3
    FieldCommand 179
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro FadeExWait
    FieldCommand 180
    .endm

    .macro ItemAdd a0, a1, a2
    FieldCommand 181
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ItemSub a0, a1, a2
    FieldCommand 182
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ItemCheckSpace a0, a1, a2
    FieldCommand 183
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ItemCheckAmount a0, a1, a2
    FieldCommand 184
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ItemGetCount a0, a1
    FieldCommand 185
    .2byte \a0
    .2byte \a1
    .endm

    .macro ItemIsTMHM a0, a1
    FieldCommand 186
    .2byte \a0
    .2byte \a1
    .endm

    .macro ItemGetPocket a0, a1
    FieldCommand 187
    .2byte \a0
    .2byte \a1
    .endm

    .macro PhenomenonGetItemID a0
    FieldCommand 188
    .2byte \a0
    .endm

    .macro ItemGetClass a0, a1
    FieldCommand 189
    .2byte \a0
    .2byte \a1
    .endm

    .macro MapChangeFake a0, a1, a2, a3
    FieldCommand 190
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro MapChangeWarpPad a0, a1, a2, a3
    FieldCommand 191
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro MapChangeWarpRail a0, a1, a2, a3, a4
    FieldCommand 192
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro MapChangeQuicksand a0, a1, a2
    FieldCommand 193
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MapChangeWarp a0, a1, a2, a3
    FieldCommand 194
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro MapChangeUnionRoom
    FieldCommand 195
    .endm

    .macro MapChangeCore a0, a1, a2, a3, a4
    FieldCommand 196
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro CallSurf
    FieldCommand 197
    .endm

    .macro CallWaterfall a0
    FieldCommand 198
    .2byte \a0
    .endm

    .macro CallCut
    FieldCommand 199
    .endm

    .macro CallDiving a0
    FieldCommand 200
    .2byte \a0
    .endm

    .macro TrainerDataCmd_00C9
    FieldCommand 201
    .endm

    .macro TrainerDataCmd_00CA a0
    FieldCommand 202
    .2byte \a0
    .endm

    .macro Random a0, a1
    FieldCommand 203
    .2byte \a0
    .2byte \a1
    .endm

    .macro RTGetTextFile a0
    FieldCommand 204
    .2byte \a0
    .endm

    .macro RTCGetDayPart a0
    FieldCommand 205
    .2byte \a0
    .endm

    // s00CE_TrainerCardGetStars
    .macro Cmd_00CE a0
    FieldCommand 206
    .2byte \a0
    .endm

    .macro RTCGetWeekDay a0
    FieldCommand 207
    .2byte \a0
    .endm

    .macro RTCGetDate a0, a1
    FieldCommand 208
    .2byte \a0
    .2byte \a1
    .endm

    .macro RTCGetTime a0, a1
    FieldCommand 209
    .2byte \a0
    .2byte \a1
    .endm

    .macro RTCGetSeason a0
    FieldCommand 210
    .2byte \a0
    .endm

    .macro RTGetZoneID a0
    FieldCommand 211
    .2byte \a0
    .endm

    .macro TrainerCardGetBirthDate a0, a1
    FieldCommand 212
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerCardHasBadge a0, a1
    FieldCommand 213
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerCardAddBadge a0
    FieldCommand 214
    .2byte \a0
    .endm

    .macro TrainerCardGetBadgeCount a0
    FieldCommand 215
    .2byte \a0
    .endm

    .macro MapReplaceIsEventSet a0, a1
    FieldCommand 216
    .2byte \a0
    .2byte \a1
    .endm

    .macro FieldSetTeleportZone a0
    FieldCommand 217
    .2byte \a0
    .endm

    .macro MapReplaceSetEvent a0, a1, a2
    FieldCommand 218
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro FieldSetNextZoneHere
    FieldCommand 219
    .endm

    .macro FieldSetNextZone a0, a1, a2, a3, a4
    FieldCommand 220
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro PokeDexGetCount a0, a1
    FieldCommand 221
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokeDexRegist a0, a1
    FieldCommand 222
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokeDexIsRegist a0, a1, a2
    FieldCommand 223
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro GameGetVersion a0
    FieldCommand 224
    .2byte \a0
    .endm

    .macro TrainerCardGetSex a0
    FieldCommand 225
    .2byte \a0
    .endm

    .macro SaveDataCheckRequired a0
    FieldCommand 226
    .2byte \a0
    .endm

    .macro GiveRunningShoes
    FieldCommand 227
    .endm

    // s00E4_BoxUnlockWallpapers
    .macro Cmd_00E4 a0
    FieldCommand 228
    .2byte \a0
    .endm

    .macro TimeSigCmd_00E5 a0, a1
    FieldCommand 229
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerCardCmd_00E6 a0, a1
    FieldCommand 230
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerCardCmd_00E7 a0
    FieldCommand 231
    .2byte \a0
    .endm

    .macro TrainerCardCmd_00E8 a0, a1
    FieldCommand 232
    .2byte \a0
    .2byte \a1
    .endm

    .macro HOFCheckIntegrity a0
    FieldCommand 234
    .2byte \a0
    .endm

    .macro DayCareCheckSpawnFlag a0
    FieldCommand 235
    .2byte \a0
    .endm

    .macro DayCareBreed
    FieldCommand 236
    .endm

    .macro DayCareResetSeed
    FieldCommand 237
    .endm

    .macro DayCareGetPkmCount a0
    FieldCommand 238
    .2byte \a0
    .endm

    .macro DayCareCalcEggSpawnChance a0
    FieldCommand 239
    .2byte \a0
    .endm

    .macro DayCareDeposit a0
    FieldCommand 240
    .2byte \a0
    .endm

    .macro DayCareWithdraw a0
    FieldCommand 241
    .2byte \a0
    .endm

    .macro DayCareGetSpecies a0, a1
    FieldCommand 242
    .2byte \a0
    .2byte \a1
    .endm

    .macro DayCareGetForme a0, a1
    FieldCommand 243
    .2byte \a0
    .2byte \a1
    .endm

    .macro DayCareCalcNewLevel a0, a1
    FieldCommand 244
    .2byte \a0
    .2byte \a1
    .endm

    .macro DayCareCalcLevelGain a0, a1
    FieldCommand 245
    .2byte \a0
    .2byte \a1
    .endm

    .macro DayCareCalcWithdrawCost a0, a1
    FieldCommand 246
    .2byte \a0
    .2byte \a1
    .endm

    .macro DayCareCallPokeSelect a0
    FieldCommand 247
    .2byte \a0
    .endm

    .macro DayCareGetSex a0, a1
    FieldCommand 248
    .2byte \a0
    .2byte \a1
    .endm

    .macro MoneyAdd a0
    FieldCommand 249
    .2byte \a0
    .endm

    .macro MoneySub a0
    FieldCommand 250
    .2byte \a0
    .endm

    .macro MoneyCheck a0, a1
    FieldCommand 251
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyGetHappiness a0, a1
    FieldCommand 252
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyAdjustHappiness a0, a1, a2
    FieldCommand 253
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyGetSpecies a0, a1
    FieldCommand 254
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyGetForme a0, a1
    FieldCommand 255
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyCheckPokerus a0
    FieldCommand 256
    .2byte \a0
    .endm

    .macro PokePartyIsFullHP a0, a1
    FieldCommand 257
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyIsEgg a0, a1
    FieldCommand 258
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyGetCount a0, a1
    FieldCommand 259
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyRecoverAll
    FieldCommand 260
    .endm

    .macro CallPokeNameInput a0, a1, a2
    FieldCommand 261
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro CallEggHatch a0
    FieldCommand 262
    .2byte \a0
    .endm

    .macro CallPokeSelect a0, a1, a2, a3
    FieldCommand 263
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PokePartyGetMoveCount a0, a1
    FieldCommand 264
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallPokeMoveReplace a0, a1, a2, a3
    FieldCommand 265
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PokePartyGetMove a0, a1, a2
    FieldCommand 266
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyLearnMove a0, a1, a2
    FieldCommand 267
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyAdd a0, a1, a2, a3
    FieldCommand 268
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PokePartyGetMemberByType a0, a1
    FieldCommand 269
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyAddEx a0, a1, a2, a3, a4, a5, a6, a7, a8
    FieldCommand 270
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .2byte \a6
    .2byte \a7
    .2byte \a8
    .endm

    .macro PokePartyAddEgg a0, a1, a2
    FieldCommand 271
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyGetParam a0, a1, a2
    FieldCommand 272
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartySetIV a0, a1, a2
    FieldCommand 273
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyGetEVTotal a0, a1
    FieldCommand 274
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyIsOriginGame a0, a1
    FieldCommand 275
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyGetCountBySpecies a0, a1
    FieldCommand 276
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyHasMove a0, a1, a2
    FieldCommand 277
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyHasMoveAny a0, a1
    FieldCommand 278
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartySetForme a0, a1
    FieldCommand 279
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyFindBySpecies a0, a1, a2
    FieldCommand 280
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyIsFromWhiteForest a0, a1, a2
    FieldCommand 281
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyGetMetDate a0, a1, a2, a3
    FieldCommand 282
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PokePartyGetTypes a0, a1, a2
    FieldCommand 283
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokePartyChangeRotomForme a0, a1, a2
    FieldCommand 284
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro TrainerCardSetFavePokemon a0
    FieldCommand 285
    .2byte \a0
    .endm

    .macro TrainerCardSaveGymVictoryParty a0
    FieldCommand 286
    .2byte \a0
    .endm

    .macro WordSetGymVictoryParty a0, a1
    FieldCommand 287
    .2byte \a0
    .2byte \a1
    .endm

    .macro FieldTradeSavePokemon a0, a1
    FieldCommand 288
    .2byte \a0
    .2byte \a1
    .endm

    .macro BoxGetCount a0, a1
    FieldCommand 289
    .2byte \a0
    .2byte \a1
    .endm

    .macro BoxAdd a0, a1, a2, a3
    FieldCommand 290
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro BoxAddEx a0, a1, a2, a3, a4, a5, a6, a7, a8
    FieldCommand 291
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .2byte \a6
    .2byte \a7
    .2byte \a8
    .endm

    .macro BMHndAnmPlay a0, a1
    FieldCommand 292
    .2byte \a0
    .2byte \a1
    .endm

    .macro BMPlayHOFMachineSeq
    FieldCommand 293
    .endm

    .macro BMChangeMdlID a0, a1, a2, a3
    FieldCommand 294
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro BMCreateHandleByGPos a0, a1, a2, a3
    FieldCommand 295
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro BMReleaseHandle a0
    FieldCommand 296
    .2byte \a0
    .endm

    .macro BMHndAudioVisualAnmPlay a0, a1
    FieldCommand 297
    .2byte \a0
    .2byte \a1
    .endm

    .macro BMHndAnmWait a0
    FieldCommand 298
    .2byte \a0
    .endm

    .macro BMAnmPlayInv a0, a1, a2
    FieldCommand 299
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro BMHndAnmPause a0
    FieldCommand 300
    .2byte \a0
    .endm

    .macro BMSetVisible a0, a1, a2, a3
    FieldCommand 301
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro BMAnmPlayLoop a0, a1, a2
    FieldCommand 302
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokecenPlayHealingSequence a0
    FieldCommand 303
    .2byte \a0
    .endm

    .macro PokecenPCOpen
    FieldCommand 304
    .endm

    .macro PokecenPCIdle
    FieldCommand 305
    .endm

    .macro PokecenPCClose a0
    FieldCommand 306
    .2byte \a0
    .endm

    .macro CasteliaRushInit
    FieldCommand 308
    .endm

    .macro FieldSetWeather a0, a1
    FieldCommand 310
    .2byte \a0
    .2byte \a1
    .endm

    .macro SaveDataWrite a0
    FieldCommand 311
    .2byte \a0
    .endm

    .macro SaveDataGetStatus a0, a1, a2
    FieldCommand 312
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro GameCommDisconnect a0
    FieldCommand 313
    .2byte \a0
    .endm

    .macro GameCommGetStatus a0
    FieldCommand 314
    .2byte \a0
    .endm

    .macro GameCommCheckDSiWiFi a0
    FieldCommand 315
    .2byte \a0
    .endm

    // s013C_GameCommTryBoot
    .macro Cmd_013C
    FieldCommand 316
    .endm

    // s013D_GameCommPause
    .macro Cmd_013D
    FieldCommand 317
    .endm

    // s013E_GameCommResume
    .macro Cmd_013E
    FieldCommand 318
    .endm

    .macro EvCameraInit
    FieldCommand 319
    .endm

    .macro EvCameraEnd
    FieldCommand 320
    .endm

    .macro EvCameraUnbind
    FieldCommand 321
    .endm

    .macro EvCameraRebind
    FieldCommand 322
    .endm

    .macro EvCameraMoveTo a0, a1, a2, a3, a4, a5, a6
    FieldCommand 323
    .2byte \a0
    .2byte \a1
    .4byte \a2
    .4byte \a3
    .4byte \a4
    .4byte \a5
    .2byte \a6
    .endm

    .macro EvCameraReturn a0
    FieldCommand 324
    .2byte \a0
    .endm

    .macro EvCameraWait
    FieldCommand 325
    .endm

    .macro EvCameraMoveToCommon a0, a1
    FieldCommand 326
    .2byte \a0
    .2byte \a1
    .endm

    .macro EvCameraMoveToDefault a0
    FieldCommand 327
    .2byte \a0
    .endm

    .macro EvCameraShake a0, a1, a2, a3, a4, a5, a6, a7
    FieldCommand 328
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .2byte \a6
    .2byte \a7
    .endm

    .macro CallFriendlyShopBuy a0, a1
    FieldCommand 329
    .2byte \a0
    .2byte \a1
    .endm

    .macro FieldOpen
    FieldCommand 330
    .endm

    .macro FieldClose
    FieldCommand 331
    .endm

    .macro RTFreeUserHeap
    FieldCommand 332
    .endm

    .macro CallRecordSystem a0, a1
    FieldCommand 333
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallBag a0, a1, a2
    FieldCommand 334
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro CallPC a0, a1
    FieldCommand 335
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallMailbox a0
    FieldCommand 336
    .2byte \a0
    .endm

    .macro CallPokedexDiploma a0, a1
    FieldCommand 337
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallGeonet
    FieldCommand 338
    .endm

    .macro callPoke3Select a0
    FieldCommand 339
    .2byte \a0
    .endm

    .macro Call3DDemo a0, a1
    FieldCommand 340
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallXTransceiver a0, a1
    FieldCommand 341
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallGameClear a0
    FieldCommand 342
    .2byte \a0
    .endm

    .macro ActorAnimationInit a0
    FieldCommand 343
    .2byte \a0
    .endm

    .macro ActorAnimationFree
    FieldCommand 344
    .endm

    .macro ActorAnimationPlay a0
    FieldCommand 345
    .2byte \a0
    .endm

    .macro ActorAnimationWait
    FieldCommand 346
    .endm

    // s015B_WifiListHasValidProfile
    .macro Cmd_015B a0
    FieldCommand 347
    .2byte \a0
    .endm

    // s015C_WifiListCountFriends
    .macro Cmd_015C a0
    FieldCommand 348
    .2byte \a0
    .endm

    .macro NetConnectWiFiClub
    FieldCommand 349
    .endm

    .macro NetConnectGTS
    FieldCommand 350
    .endm

    // s015F_Nop
    .macro Cmd_015F
    FieldCommand 351
    .endm

    .macro NetConnectWiFiBattle a0, a1
    FieldCommand 352
    .2byte \a0
    .2byte \a1
    .endm

    .macro NetConnectBattleVideo a0
    FieldCommand 353
    .2byte \a0
    .endm

    .macro NetConnectGTSNegotiation
    FieldCommand 354
    .endm

    .macro MusicalCmd_0163 a0, a1
    FieldCommand 355
    .2byte \a0
    .byte \a1
    .endm

    .macro MusicalCmd_0164 a0
    FieldCommand 356
    .2byte \a0
    .endm

    .macro MusicalCmd_0165 a0, a1, a2
    FieldCommand 357
    .byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MusicalCmd_0166 a0, a1, a2
    FieldCommand 358
    .2byte \a0
    .byte \a1
    .2byte \a2
    .endm

    // s0167_MusicalComm
    .macro Cmd_0167 a0, a1, a2, a3
    FieldCommand 359
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro MusicalCmd_0168 a0
    FieldCommand 360
    .2byte \a0
    .endm

    .macro MusicalCmd_0169 a0
    FieldCommand 361
    .2byte \a0
    .endm

    // s016A_MusicalSelectPoke
    .macro Cmd_016A a0, a1
    FieldCommand 362
    .2byte \a0
    .2byte \a1
    .endm

    .macro HighLinkCmd_016E
    FieldCommand 366
    .endm

    .macro HighLinkCmd_0170 a0
    FieldCommand 368
    .2byte \a0
    .endm

    .macro IsFestMissionAvailable a0
    FieldCommand 370
    .2byte \a0
    .endm

    .macro CallWildBattle a0, a1, a2
    FieldCommand 372
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro CallWildBattleEnd
    FieldCommand 373
    .endm

    .macro CallWildLose
    FieldCommand 374
    .endm

    .macro WildBattleIsVictory a0
    FieldCommand 375
    .2byte \a0
    .endm

    .macro WildBattleGetResult a0
    FieldCommand 376
    .2byte \a0
    .endm

    .macro CallCaptureDemo
    FieldCommand 377
    .endm

    // GimmickGateCmd_ViewBoard
    .macro Cmd_017A a0
    FieldCommand 378
    .2byte \a0
    .endm

    // func_ov012_02157bdc
    .macro Cmd_017B a0, a1
    FieldCommand 379
    .2byte \a0
    .2byte \a1
    .endm

    // s017C_RollerCoasterCallSwitch
    .macro Cmd_017C a0
    FieldCommand 380
    .2byte \a0
    .endm

    // ScrcmdFieldGimmickCartLanes_UnlockLaneFromScript
    .macro Cmd_017D a0
    FieldCommand 381
    .2byte \a0
    .endm

    // s0186_DriftveilGymPrepareFreight
    .macro Cmd_0186
    FieldCommand 390
    .endm

    // s0187_DriftveilLiftPlaceAtBottom
    .macro Cmd_0187 a0
    FieldCommand 391
    .2byte \a0
    .endm

    // ScrcmdFieldGimmickPlateDoors_StartDoorEvent
    .macro Cmd_018C a0, a1
    FieldCommand 396
    .2byte \a0
    .2byte \a1
    .endm

    // s018D_DriftveilGymCallFreight
    .macro Cmd_018D a0
    FieldCommand 397
    .2byte \a0
    .endm

    // s018E_DriftveilLiftCallMove
    .macro Cmd_018E a0
    FieldCommand 398
    .2byte \a0
    .endm

    // s018F_IcirrusGymCallSwitchAnim
    .macro Cmd_018F a0
    FieldCommand 399
    .2byte \a0
    .endm

    // s0190_IcirrusGymCallFlipSwitch
    .macro Cmd_0190 a0
    FieldCommand 400
    .2byte \a0
    .endm

    // s0191_IcirrusGymToggleMovementAtPlace
    .macro Cmd_0191 a0
    FieldCommand 401
    .2byte \a0
    .endm

    .macro CGearPowerOn a0
    FieldCommand 407
    .2byte \a0
    .endm

    .macro FieldSubscreenChange a0
    FieldCommand 408
    .2byte \a0
    .endm

    .macro FieldSubscreenReturn
    FieldCommand 409
    .endm

    .macro CGearControlWarning a0
    FieldCommand 410
    .2byte \a0
    .endm

    .macro PlayFieldEffect a0
    FieldCommand 411
    .2byte \a0
    .endm

    .macro PlayHMCutInEffect a0
    FieldCommand 412
    .2byte \a0
    .endm

    .macro PlayAlderFlyEffect a0
    FieldCommand 413
    .2byte \a0
    .endm

    // s01A1_PleasureBoatGetRoomInfo
    .macro Cmd_01A1 a0, a1, a2, a3
    FieldCommand 417
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro CallRoyalUnovaView a0
    FieldCommand 418
    .2byte \a0
    .endm

    .macro FadeInBlackQ
    FieldCommand 419
    .endm

    .macro FadeOutBlackQ
    FieldCommand 420
    .endm

    .macro FadeInWhiteQ
    FieldCommand 421
    .endm

    .macro FadeOutWhiteQ
    FieldCommand 422
    .endm

    .macro FadeWait
    FieldCommand 423
    .endm

    .macro FadeInBlackQ_
    FieldCommand 424
    .endm

    .macro CallSeasonBanner
    FieldCommand 426
    .endm

    .macro FadeInBlack
    FieldCommand 427
    .endm

    .macro FadeOutBlack
    FieldCommand 428
    .endm

    .macro FadeInWhite
    FieldCommand 429
    .endm

    .macro FadeOutWhite
    FieldCommand 430
    .endm

    // Regulation_RegisterAllowedSpecies
    .macro Cmd_01AF a0, a1
    FieldCommand 431
    .2byte \a0
    .2byte \a1
    .endm

    // Regulation_CheckPartyRegulation
    .macro Cmd_01B0 a0, a1
    FieldCommand 432
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallLeagueLiftWarp
    FieldCommand 433
    .endm

    // ScrcmdProcFld_SpawnMovePokemonIfInactive
    .macro Cmd_01B2 a0
    FieldCommand 434
    .2byte \a0
    .endm

    // FieldChunk_ScrCmdReadSaveByte
    .macro Cmd_01B3 a0, a1
    FieldCommand 435
    .2byte \a0
    .2byte \a1
    .endm

    .macro FieldTradeStart a0, a1
    FieldCommand 436
    .2byte \a0
    .2byte \a1
    .endm

    .macro FieldTradeCheck a0, a1, a2
    FieldCommand 437
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ElevatorSetTablePtr a0
    FieldCommand 449
    .4byte \a0 - (. + 4)
    .endm

    .macro ElevatorBuildListMenu
    FieldCommand 450
    .endm

    .macro ElevatorChangeMap a0
    FieldCommand 451
    .2byte \a0
    .endm

    .macro PokeDexIsComplete a0, a1
    FieldCommand 452
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokeDexGetEvaluationParams a0, a1, a2, a3
    FieldCommand 453
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PokeDexGiveNational
    FieldCommand 454
    .endm

    .macro PokeDexHaveNational a0
    FieldCommand 455
    .2byte \a0
    .endm

    .macro PokeDexEnable
    FieldCommand 456
    .endm

    .macro NDemoStart a0, a1, a2
    FieldCommand 457
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro NDemoEnd
    FieldCommand 458
    .endm

    .macro NDemoReadyTalkMotion
    FieldCommand 459
    .endm

    // ElevatorTable_GetMysteryEntryCount
    .macro Cmd_01CC a0
    FieldCommand 460
    .2byte \a0
    .endm

    // ElevatorTable_RegisterBirthdayEntry
    .macro Cmd_01CD a0
    FieldCommand 461
    .2byte \a0
    .endm

    // s01CE_UnityTowerGetTrainerClass
    .macro Cmd_01CE a0, a1
    FieldCommand 462
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerGetStateParam a0, a1
    FieldCommand 463
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerCallFloorSelect a0, a1, a2, a3
    FieldCommand 464
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro MoveTutorCheckParty a0, a1
    FieldCommand 465
    .2byte \a0
    .2byte \a1
    .endm

    .macro MoveTutorCheckPkm a0, a1, a2
    FieldCommand 466
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MoveTutorGetMoveID a0, a1, a2
    FieldCommand 467
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MoveTutorCallPokeSelect a0, a1, a2
    FieldCommand 468
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MoveReminderCheckPkm a0, a1
    FieldCommand 469
    .2byte \a0
    .2byte \a1
    .endm

    .macro MoveReminderCallMoveSelect a0, a1
    FieldCommand 470
    .2byte \a0
    .2byte \a1
    .endm

    .macro ObjInitPointGPos a0, a1, a2, a3
    FieldCommand 471
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ObjInitWarpGPos a0, a1, a2, a3
    FieldCommand 472
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ObjInitNPCGPos a0, a1, a2, a3, a4
    FieldCommand 473
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro CallPhraseSelect a0, a1, a2, a3
    FieldCommand 474
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // func_ov033_02177d78
    .macro Cmd_01DB a0, a1
    FieldCommand 475
    .2byte \a0
    .2byte \a1
    .endm

    .macro DreamWorldCmd_01DC a0, a1
    FieldCommand 476
    .2byte \a0
    .2byte \a1
    .endm

    // s01DD_BroadcastGameBeacon
    .macro Cmd_01DD a0, a1, a2
    FieldCommand 477
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PassPowerPopDepletedID a0, a1
    FieldCommand 478
    .2byte \a0
    .2byte \a1
    .endm

    // s01DF_CallPassPowerFlash
    .macro Cmd_01DF
    FieldCommand 479
    .endm

    .macro StadiumSetupActorsDouble a0, a1, a2, a3
    FieldCommand 480
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro StadiumLoadTrainerTable
    FieldCommand 481
    .endm

    .macro StadiumFreeTrainerTable
    FieldCommand 482
    .endm

    .macro StadiumSetupActorSingle a0, a1, a2
    FieldCommand 483
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro StadiumSetupActorsTriple a0, a1, a2, a3, a4, a5
    FieldCommand 484
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro StadiumResetTrainerFlags
    FieldCommand 485
    .endm

    .macro TrialHouseWorkInit
    FieldCommand 486
    .endm

    .macro TrialHouseWorkDelete
    FieldCommand 487
    .endm

    .macro TrialHousePrepareParty a0
    FieldCommand 488
    .2byte \a0
    .endm

    .macro TrialHouseCallTeamSelect a0, a1, a2
    FieldCommand 489
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro TrialHouseCmd_01EA a0, a1
    FieldCommand 490
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrialHouseMsgDisp a0, a1
    FieldCommand 491
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrialHouseStartBattle
    FieldCommand 492
    .endm

    .macro TrialHouseCmd_01ED a0
    FieldCommand 493
    .2byte \a0
    .endm

    .macro TrainerCardCmd_01EE a0
    FieldCommand 494
    .2byte \a0
    .endm

    .macro TrialHouseGetBattleTestRank a0
    FieldCommand 495
    .2byte \a0
    .endm

    .macro TrialHouseCmd_01F0 a0
    FieldCommand 496
    .2byte \a0
    .endm

    .macro TrialHouseCalcPointsStars a0, a1
    FieldCommand 497
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrialHouseCmd_01F2 a0
    FieldCommand 498
    .2byte \a0
    .endm

    .macro TrialHouseSaveData a0
    FieldCommand 499
    .2byte \a0
    .endm

    .macro TrialHouseCmd_01F4 a0, a1
    FieldCommand 500
    .2byte \a0
    .2byte \a1
    .endm

    // s01F5_TrialHouseGetRecordMask
    .macro Cmd_01F5 a0
    FieldCommand 501
    .2byte \a0
    .endm

    // Scrcmd_CheckWifiListFlags
    .macro Cmd_01F6 a0, a1, a2, a3
    FieldCommand 502
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // Scrcmd_WifiListOperation
    .macro Cmd_01F7 a0, a1, a2, a3
    FieldCommand 503
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // Scrcmd_SetWifiFriendNameWord
    .macro Cmd_01F8 a0, a1
    FieldCommand 504
    .2byte \a0
    .2byte \a1
    .endm

    // Scrcmd_CheckPmsWordPair
    .macro Cmd_01F9 a0, a1, a2, a3
    FieldCommand 505
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PepQuizGenerate a0, a1, a2, a3
    FieldCommand 506
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ItemCollectorCheckGroup a0, a1
    FieldCommand 507
    .2byte \a0
    .2byte \a1
    .endm

    .macro ItemCollectorGetPrice a0, a1, a2
    FieldCommand 508
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro TrainerGameInfoCmd_01FD a0, a1, a2, a3
    FieldCommand 509
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro TrainerGameInfoCmd_01FE
    FieldCommand 510
    .endm

    .macro SurveyGetCurrentQuestionID a0
    FieldCommand 511
    .2byte \a0
    .endm

    .macro SurveyGetCurrentAnswerIDs a0, a1, a2
    FieldCommand 512
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro SurveyGetPopularOptionMsgID a0, a1
    FieldCommand 513
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerCardCmd_0202 a0, a1
    FieldCommand 514
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrainerCardCmd_0203 a0
    FieldCommand 515
    .2byte \a0
    .endm

    .macro SurveyGetTime a0
    FieldCommand 516
    .2byte \a0
    .endm

    .macro CallGreetingPhraseInput a0
    FieldCommand 517
    .2byte \a0
    .endm

    .macro CallThanksPhraseInput a0
    FieldCommand 518
    .2byte \a0
    .endm

    .macro TrainerGameInfoCmd_0207 a0
    FieldCommand 519
    .byte \a0
    .endm

    .macro TrainerGameInfoCmd_0208 a0
    FieldCommand 520
    .byte \a0
    .endm

    // SurveyCmd_WriteOwnAnswer
    .macro Cmd_0209 a0, a1
    FieldCommand 521
    .2byte \a0
    .2byte \a1
    .endm

    // SurveyCmd_ShowRankMessage
    .macro Cmd_020A
    FieldCommand 522
    .endm

    .macro TrainerGameInfoCmd_020B a0
    FieldCommand 523
    .2byte \a0
    .endm

    .macro TrainerGameInfoCmd_020C
    FieldCommand 524
    .endm

    .macro PokePartyFindEx a0, a1, a2, a3
    FieldCommand 525
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // s020E_SpPokeJump
    .macro Cmd_020E a0, a1, a2, a3, a4, a5
    FieldCommand 526
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    // s020F_SpPokeAppear
    .macro Cmd_020F a0, a1, a2, a3
    FieldCommand 527
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // s0210_SpPokeWaitAnimEnd
    .macro Cmd_0210 a0
    FieldCommand 528
    .2byte \a0
    .endm

    // s0211_SpPokeWaitFrame
    .macro Cmd_0211 a0
    FieldCommand 529
    .2byte \a0
    .endm

    // s0212_ActorMoveToHeight
    .macro Cmd_0212 a0, a1, a2
    FieldCommand 530
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // s0213_ActorHoldPos
    .macro Cmd_0213 a0
    FieldCommand 531
    .2byte \a0
    .endm

    .macro CallPokemonPreview a0, a1, a2, a3
    FieldCommand 532
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // s0215_SetSpecialTransferUsed
    .macro Cmd_0215 a0, a1
    FieldCommand 533
    .2byte \a0
    .2byte \a1
    .endm

    .macro IsOneShotDRObtained a0, a1
    FieldCommand 534
    .2byte \a0
    .2byte \a1
    .endm

    .macro MapChangeEntreeForest a0
    FieldCommand 535
    .2byte \a0
    .endm

    .macro AreaNPCCmd_0218 a0, a1
    FieldCommand 536
    .2byte \a0
    .2byte \a1
    .endm

    .macro AreaNPCCmd_0219 a0, a1
    FieldCommand 537
    .2byte \a0
    .2byte \a1
    .endm

    .macro WordSetLoadEntreeForestPkmName a0, a1
    FieldCommand 538
    .2byte \a0
    .2byte \a1
    .endm

    .macro EntreeForestSpawnAllPkm
    FieldCommand 539
    .endm

    .macro EntreeForestStartBattle a0, a1
    FieldCommand 540
    .2byte \a0
    .2byte \a1
    .endm

    .macro AreaNPCCmd_021D a0
    FieldCommand 541
    .2byte \a0
    .endm

    .macro FishingChallengeGetRandomPkm a0
    FieldCommand 542
    .2byte \a0
    .endm

    // func_ov012_02156104
    .macro Cmd_021F a0, a1
    FieldCommand 543
    .2byte \a0
    .2byte \a1
    .endm

    // func_ov012_02156128
    .macro Cmd_0220 a0, a1
    FieldCommand 544
    .2byte \a0
    .2byte \a1
    .endm

    .macro AnvilleTrainGetModelID a0, a1
    FieldCommand 545
    .2byte \a0
    .2byte \a1
    .endm

    .macro AnvilleTrainGetDailyIndex a0
    FieldCommand 546
    .2byte \a0
    .endm

    .macro PokePartyGetHiddenPowerType a0, a1
    FieldCommand 547
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyGetIV a0, a1, a2
    FieldCommand 548
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ItemGetRandomOwnedTMMove a0
    FieldCommand 549
    .2byte \a0
    .endm

    // CountTrainersEvent_StartCountOtherOtIdsTask
    .macro Cmd_0226 a0
    FieldCommand 550
    .2byte \a0
    .endm

    .macro RecordAdd a0, a1
    FieldCommand 551
    .2byte \a0
    .2byte \a1
    .endm

    .macro RecordGet a0, a1
    FieldCommand 552
    .2byte \a0
    .2byte \a1
    .endm

    .macro HollowRivalCmd_022A a0, a1
    FieldCommand 554
    .2byte \a0
    .2byte \a1
    .endm

    // Scrcmd_StartSwapNpcEvent
    .macro Cmd_022B a0, a1
    FieldCommand 555
    .2byte \a0
    .2byte \a1
    .endm

    .macro FieldEffect a0
    FieldCommand 557
    .2byte \a0
    .endm

    // ScriptNative_SetMmdlZoneDeleteOff
    .macro Cmd_022E a0
    FieldCommand 558
    .2byte \a0
    .endm

    .macro WordSetLoadPastTradePkmName a0, a1
    FieldCommand 559
    .2byte \a0
    .2byte \a1
    .endm

    // s0230_MsgPlaceSignFont4
    .macro Cmd_0230 a0, a1
    FieldCommand 560
    .2byte \a0
    .2byte \a1
    .endm

    // s0231_CallGameManual
    .macro Cmd_0231 a0
    FieldCommand 561
    .2byte \a0
    .endm

    // PokeParty_ControlLegendMon
    .macro Cmd_0232 a0, a1
    FieldCommand 562
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerGetVisitorCountry a0
    FieldCommand 563
    .2byte \a0
    .endm

    .macro ActorFallDownToXZ a0, a1, a2, a3
    FieldCommand 564
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro BGMPlayEx a0, a1
    FieldCommand 565
    .2byte \a0
    .2byte \a1
    .endm

    .macro WordSetLoadItemCollectorPrice a0, a1, a2, a3
    FieldCommand 566
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ItemCollectorSell a0, a1
    FieldCommand 567
    .2byte \a0
    .2byte \a1
    .endm

    .macro BGMChangeMapEx a0
    FieldCommand 568
    .2byte \a0
    .endm

    // ScriptNative_CheckPlayerFormModel
    .macro Cmd_0239 a0
    FieldCommand 569
    .2byte \a0
    .endm

    .macro PokeVoicePlay a0, a1
    FieldCommand 570
    .2byte \a0
    .2byte \a1
    .endm

    // func_ov012_02169c1c
    .macro Cmd_023B
    FieldCommand 571
    .endm

    // s023C_NuvemaGimmickPlay
    .macro Cmd_023C
    FieldCommand 572
    .endm

    // s023D_NuvemaGimmickWaitAnim
    .macro Cmd_023D
    FieldCommand 573
    .endm

    .macro DayCareGetSexForNamePrint a0, a1, a2
    FieldCommand 574
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro CallPlaceNameDisp
    FieldCommand 575
    .endm

    // ScriptWork_FadeBgmVolume
    .macro Cmd_0240 a0, a1
    FieldCommand 576
    .2byte \a0
    .2byte \a1
    .endm

    // ScriptWork_RestoreBgmVolume
    .macro Cmd_0241 a0
    FieldCommand 577
    .2byte \a0
    .endm

    .macro BGMFadeOut a0
    FieldCommand 581
    .2byte \a0
    .endm

    .macro BGMFadeOutAll a0
    FieldCommand 582
    .2byte \a0
    .endm

    .macro MapChangeRail a0, a1, a2, a3, a4
    FieldCommand 583
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro PlayerMoveToYAsync_ a0, a1, a2, a3
    FieldCommand 584
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // s0249_GetRandomDistinctSix
    .macro Cmd_0249 a0, a1, a2, a3, a4, a5
    FieldCommand 585
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    // s024A_FieldSubscreenIsEnabled
    .macro Cmd_024A a0
    FieldCommand 586
    .2byte \a0
    .endm

    .macro FieldSubscreenDisable
    FieldCommand 587
    .endm

    .macro BGMAmbienceResume
    FieldCommand 588
    .endm

    // s024D_RollerCoasterAmbienceStart
    .macro Cmd_024D
    FieldCommand 589
    .endm

    .macro PokePartyIsFullPP a0, a1
    FieldCommand 590
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorWalkRoute a0, a1, a2, a3, a4, a5
    FieldCommand 591
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro ActorPairSet a0, a1, a2, a3, a4
    FieldCommand 592
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro ActorPairEnd a0, a1
    FieldCommand 593
    .2byte \a0
    .2byte \a1
    .endm

    .macro ActorPairGetTrID a0
    FieldCommand 594
    .2byte \a0
    .endm

    .macro ActorPairSetMoveEnable a0
    FieldCommand 595
    .byte \a0
    .endm

    // func_ov012_02157b7c
    .macro Cmd_025A a0
    FieldCommand 602
    .2byte \a0
    .endm

    // s025B_GymElecReapplyProgress
    .macro Cmd_025B
    FieldCommand 603
    .endm

    // s025C_GymElecSetFollower
    .macro Cmd_025C a0, a1, a2
    FieldCommand 604
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // s025D_GymElecSetProgress
    .macro Cmd_025D a0
    FieldCommand 605
    .2byte \a0
    .endm

    // s025F_GymElecSetStage
    .macro Cmd_025F a0
    FieldCommand 607
    .2byte \a0
    .endm

    .macro HollowRivalCmd_0262 a0, a1
    FieldCommand 610
    .2byte \a0
    .2byte \a1
    .endm

    .macro HollowRivalCmd_0263 a0
    FieldCommand 611
    .2byte \a0
    .endm

    .macro HollowRivalCmd_0264 a0, a1
    FieldCommand 612
    .2byte \a0
    .2byte \a1
    .endm

    .macro HollowRivalCmd_0265 a0
    FieldCommand 613
    .2byte \a0
    .endm

    .macro HollowRivalCmd_0266 a0
    FieldCommand 614
    .2byte \a0
    .endm

    .macro HollowRivalCmd_0267
    FieldCommand 615
    .endm

    // s0268_GymInsectPlayObject
    .macro Cmd_0268 a0
    FieldCommand 616
    .2byte \a0
    .endm

    // s0269_GymInsectShowEffect
    .macro Cmd_0269
    FieldCommand 617
    .endm

    .macro WordSetMedalName a0, a1
    FieldCommand 620
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetMedalRank a0, a1
    FieldCommand 621
    .byte \a0
    .2byte \a1
    .endm

    .macro MedalGetCount a0, a1
    FieldCommand 622
    .byte \a0
    .2byte \a1
    .endm

    .macro MedalAcknowledge a0, a1
    FieldCommand 625
    .2byte \a0
    .2byte \a1
    .endm

    .macro MedalGetGuruActor a0, a1
    FieldCommand 626
    .2byte \a0
    .2byte \a1
    .endm

    .macro MedalGive a0
    FieldCommand 627
    .2byte \a0
    .endm

    .macro FunfestMissionStart
    FieldCommand 628
    .endm

    // s0275_WordSetFunfestMission
    .macro Cmd_0275 a0, a1, a2
    FieldCommand 629
    .byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro FunfestMissionBroadcast a0, a1
    FieldCommand 630
    .2byte \a0
    .2byte \a1
    .endm

    .macro FunfestActorDelete
    FieldCommand 631
    .endm

    .macro FunfestDispSalesmanMessage a0, a1, a2
    FieldCommand 632
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro FunfestBGMReturn
    FieldCommand 633
    .endm

    .macro FunfestGetGenericInfo a0, a1
    FieldCommand 634
    .2byte \a0
    .2byte \a1
    .endm

    .macro FunfestGetItemExchangeInfo a0, a1, a2, a3
    FieldCommand 635
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro FunfestGetItemSaleInfo a0, a1
    FieldCommand 636
    .2byte \a0
    .2byte \a1
    .endm

    .macro FunfestGetPokemonQuizInfo a0, a1, a2
    FieldCommand 637
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro FunfestGetPokemonQuizSpecies a0, a1
    FieldCommand 638
    .2byte \a0
    .2byte \a1
    .endm

    .macro FunfestGetPokemonQuizBogusSpecies a0, a1
    FieldCommand 639
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallPlaceSelect a0, a1
    FieldCommand 644
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallWordSetPokeNameInput a0, a1, a2
    FieldCommand 645
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MapChangeFlyWarp a0, a1
    FieldCommand 650
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallEntralinkWarpOut
    FieldCommand 651
    .endm

    // s028E_MistraltonGymFireCannon
    .macro Cmd_028E a0
    FieldCommand 654
    .2byte \a0
    .endm

    // s028F_CallRivalNameInput
    .macro Cmd_028F a0
    FieldCommand 655
    .2byte \a0
    .endm

    .macro WordSetLoadRivalName a0
    FieldCommand 656
    .byte \a0
    .endm

    // s0291_VirbankGymStopModelAnimations
    .macro Cmd_0291 a0
    FieldCommand 657
    .2byte \a0
    .endm

    // s0292_VirbankGymSetActorFlag
    .macro Cmd_0292 a0
    FieldCommand 658
    .2byte \a0
    .endm

    .macro HiddenHollowCallWarpIn a0
    FieldCommand 659
    .2byte \a0
    .endm

    .macro HiddenHollowGetParam a0, a1
    FieldCommand 660
    .2byte \a0
    .2byte \a1
    .endm

    .macro HiddenHollowReset
    FieldCommand 661
    .endm

    .macro HiddenHollowCallWarpOut
    FieldCommand 662
    .endm

    .macro CallWildBattleEx a0, a1, a2, a3
    FieldCommand 663
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // s0298_WordSetEmptyFileMessage
    .macro Cmd_0298 a0, a1
    FieldCommand 664
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetLoadAbility a0, a1
    FieldCommand 665
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetLoadNature a0, a1
    FieldCommand 666
    .byte \a0
    .2byte \a1
    .endm

    .macro WordSetLoadJoinAvenueName a0
    FieldCommand 667
    .byte \a0
    .endm

    .macro MedalGetFieldEffectID a0, a1
    FieldCommand 670
    .2byte \a0
    .2byte \a1
    .endm

    .macro MedalDiscover a0
    FieldCommand 671
    .2byte \a0
    .endm

    .macro MedalIsObtained a0, a1
    FieldCommand 672
    .2byte \a0
    .2byte \a1
    .endm

    .macro MedalGetMostCompleteCategory a0
    FieldCommand 673
    .2byte \a0
    .endm

    // s02A2_HumilauGymPlayLeaderSplash
    .macro Cmd_02A2
    FieldCommand 674
    .endm

    // s02A3_HumilauGymPlayLeaderWaves
    .macro Cmd_02A3
    FieldCommand 675
    .endm

    // func_ov012_02157bb4
    .macro Cmd_02A4
    FieldCommand 676
    .endm

    // s02A5_OpelucidGymCallMove
    .macro Cmd_02A5 a0
    FieldCommand 677
    .2byte \a0
    .endm

    // s02A6_OpelucidGymRefreshVars
    .macro Cmd_02A6
    FieldCommand 678
    .endm

    // s02A7_OpelucidGymCallButton
    .macro Cmd_02A7 a0
    FieldCommand 679
    .2byte \a0
    .endm

    // s02A8_OpelucidGymFreeHeap
    .macro Cmd_02A8
    FieldCommand 680
    .endm

    // s02A9_OpelucidGymCreateHeap
    .macro Cmd_02A9
    FieldCommand 681
    .endm

    .macro CallLeafPileStuck
    FieldCommand 684
    .endm

    // s02AE_GameGetMyCity
    .macro Cmd_02AE a0
    FieldCommand 686
    .2byte \a0
    .endm

    .macro GameGetDifficulty a0
    FieldCommand 687
    .2byte \a0
    .endm

    .macro CallUnovaLinkKeyUnlock a0
    FieldCommand 688
    .2byte \a0
    .endm

    .macro KeysCmd_02B1 a0
    FieldCommand 689
    .2byte \a0
    .endm

    // s02B2_MemoryLinkCheckFlag
    .macro Cmd_02B2 a0, a1
    FieldCommand 690
    .2byte \a0
    .2byte \a1
    .endm

    // s02B3_MemoryLinkGetVar
    .macro Cmd_02B3 a0, a1
    FieldCommand 691
    .2byte \a0
    .2byte \a1
    .endm

    // s02B4_MemoryLinkIsPlayerLinked
    .macro Cmd_02B4 a0, a1
    FieldCommand 692
    .2byte \a0
    .2byte \a1
    .endm

    // s02B5_WordSetMemoryLinkPlayerName
    .macro Cmd_02B5 a0, a1
    FieldCommand 693
    .2byte \a0
    .2byte \a1
    .endm

    .macro MusicalCmd_02B6 a0, a1
    FieldCommand 694
    .2byte \a0
    .2byte \a1
    .endm

    .macro MusicalCmd_02B7 a0
    FieldCommand 695
    .2byte \a0
    .endm

    .macro PedometerStart
    FieldCommand 696
    .endm

    .macro PedometerEnd
    FieldCommand 697
    .endm

    .macro PedometerGet a0
    FieldCommand 698
    .2byte \a0
    .endm

    .macro Gym0601FanAmbienceStart
    FieldCommand 699
    .endm

    .macro TVCheckCommercial a0, a1
    FieldCommand 700
    .2byte \a0
    .2byte \a1
    .endm

    .macro TVGenCommercialMsgID a0
    FieldCommand 701
    .2byte \a0
    .endm

    .macro ActorMoveLinear a0, a1, a2, a3, a4
    FieldCommand 702
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro FieldTradeGetSpecies a0, a1
    FieldCommand 704
    .2byte \a0
    .2byte \a1
    .endm

    .macro RepelRearm a0
    FieldCommand 706
    .2byte \a0
    .endm

    .macro HollowRivalCmd_02C3 a0
    FieldCommand 707
    .2byte \a0
    .endm

    // s02C4_HiddenHollowSetPeriod
    .macro Cmd_02C4
    FieldCommand 708
    .endm

    // func_ov036_021c783c
    .macro Cmd_02C5 a0
    FieldCommand 709
    .2byte \a0
    .endm

    .macro JoinAvenueStoreStart
    FieldCommand 710
    .endm

    .macro JoinAvenueStoreEnd
    FieldCommand 711
    .endm

    .macro CallHappyPhraseInput a0
    FieldCommand 714
    .2byte \a0
    .endm

    // s02CB_PokewoodGetLevel
    .macro Cmd_02CB a0
    FieldCommand 715
    .2byte \a0
    .endm

    .macro PokeDexCheckHabitatList a0, a1, a2, a3
    FieldCommand 719
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PokeDexEnableHabitatList
    FieldCommand 720
    .endm

    .macro KeysCmd_02D1 a0
    FieldCommand 721
    .2byte \a0
    .endm

    .macro FieldOpenRestoreLCD
    FieldCommand 722
    .endm

    .macro HollowRivalCmd_02D3 a0, a1
    FieldCommand 723
    .2byte \a0
    .2byte \a1
    .endm

    .macro ItemGetTMCount a0
    FieldCommand 724
    .2byte \a0
    .endm

    // func_ov036_021c9a7c
    .macro Cmd_02D5 a0, a1
    FieldCommand 725
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerCmd_02D7 a0, a1
    FieldCommand 727
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerCmd_02D8 a0
    FieldCommand 728
    .2byte \a0
    .endm

    .macro UnityTowerGetVisitorParam a0, a1
    FieldCommand 729
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerCmd_02DA a0
    FieldCommand 730
    .2byte \a0
    .endm

    .macro UnityTowerSetFloor a0, a1
    FieldCommand 731
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerInitVisitorMessage a0, a1, a2
    FieldCommand 732
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro UnityTowerGetVisitorCount a0
    FieldCommand 733
    .2byte \a0
    .endm

    .macro UnityTowerSetHobby a0
    FieldCommand 734
    .2byte \a0
    .endm

    .macro UnityTowerGetHobby a0
    FieldCommand 735
    .2byte \a0
    .endm

    .macro MedalDiscoverInitial
    FieldCommand 737
    .endm

    .macro LensFlareRequest
    FieldCommand 739
    .endm

    .macro HiddenHollowSet a0, a1, a2, a3
    FieldCommand 741
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro HiddenHollowCreateEvents
    FieldCommand 742
    .endm

    // s02E8_CallFieldFadeOut
    .macro Cmd_02E8 a0, a1
    FieldCommand 744
    .2byte \a0
    .2byte \a1
    .endm

    // s02E9_CallFieldFadeIn
    .macro Cmd_02E9 a0, a1
    FieldCommand 745
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokePartyAddNPoke a0, a1, a2, a3, a4, a5
    FieldCommand 746
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    // s02ED_SetSoftResetBlocked
    .macro Cmd_02ED a0, a1
    FieldCommand 749
    .2byte \a0
    .2byte \a1
    .endm

    .macro MusicalIsPropOwned a0, a1
    FieldCommand 750
    .2byte \a0
    .2byte \a1
    .endm

    .macro MusicalGetOwnedPropCount a0
    FieldCommand 751
    .2byte \a0
    .endm

    .macro SetOneShotDRObtained a0
    FieldCommand 753
    .2byte \a0
    .endm

    .macro IsOneShotDRObtained_02F2 a0, a1
    FieldCommand 754
    .2byte \a0
    .2byte \a1
    .endm

    // Plugin 1

    // BSubwayCmd_ClearWork
    .macro Plugin1_Cmd1000
    FieldCommand 1000
    .endm

    // BSubwayCmd_CreateWork
    .macro Plugin1_Cmd1001 a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    // BSubwayCmd_FreeWork
    .macro Plugin1_Cmd1002
    FieldCommand 1002
    .endm

    .macro BSubwayCmd_Tool a0, a1, a2, a3
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // Plugin 2

    .macro PleasureBoatCmd_Create a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro PleasureBoatCmd_Free
    FieldCommand 1001
    .endm

    .macro PleasureBoatCmd_GetInfo a0, a1
    FieldCommand 1002
    .2byte \a0
    .2byte \a1
    .endm

    .macro PleasureBoatCmd_AdvanceClock a0, a1
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .endm

    .macro PleasureBoatCmd_SetTrainerInfo a0, a1, a2, a3
    FieldCommand 1004
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PleasureBoatCmd_StopClock
    FieldCommand 1005
    .endm

    // Plugin 3

    // PokemonLeagueCmd_DisableRailLines
    .macro Plugin3_Cmd1000 a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro PokemonLeagueCmd_SetCamera a0
    FieldCommand 1001
    .2byte \a0
    .endm

    // PokemonLeagueCmd_PlayGrimsleyGroup
    .macro Plugin3_Cmd1002 a0
    FieldCommand 1002
    .2byte \a0
    .endm

    // PokemonLeagueCmd_PlayGrimsleyModel0Animation
    .macro Plugin3_Cmd1003 a0
    FieldCommand 1003
    .2byte \a0
    .endm

    .macro PokemonLeagueCmd_PlayCaitlinAmbience
    FieldCommand 1004
    .endm

    // PokemonLeagueCmd_PlayCaitlinModels34Or5
    .macro Plugin3_Cmd1005 a0
    FieldCommand 1005
    .2byte \a0
    .endm

    // PokemonLeagueCmd_CallCaitlinModel1Event
    .macro Plugin3_Cmd1006
    FieldCommand 1006
    .endm

    // PokemonLeagueCmd_PlayCaitlinModel0
    .macro Plugin3_Cmd1007
    FieldCommand 1007
    .endm

    // PokemonLeagueCmd_DropCaitlinActor
    .macro Plugin3_Cmd1008 a0, a1
    FieldCommand 1008
    .2byte \a0
    .2byte \a1
    .endm

    // PokemonLeagueCmd_CallShauntalModel8Event
    .macro Plugin3_Cmd1009
    FieldCommand 1009
    .endm

    // PokemonLeagueCmd_PlayShauntalModel
    .macro Plugin3_Cmd1010 a0
    FieldCommand 1010
    .2byte \a0
    .endm

    // PokemonLeagueCmd_CallShauntalRoomChangeEvent
    .macro Plugin3_Cmd1011 a0
    FieldCommand 1011
    .2byte \a0
    .endm

    // PokemonLeagueCmd_CallShauntalModels6And9Event
    .macro Plugin3_Cmd1012
    FieldCommand 1012
    .endm

    // PokemonLeagueCmd_PlayMarshalModel0
    .macro Plugin3_Cmd1013
    FieldCommand 1013
    .endm

    // PokemonLeagueCmd_PlayMarshalModel3Animation
    .macro Plugin3_Cmd1014 a0
    FieldCommand 1014
    .2byte \a0
    .endm

    // PokemonLeagueCmd_PlayMarshalModel1
    .macro Plugin3_Cmd1015
    FieldCommand 1015
    .endm

    // PokemonLeagueCmd_CallMarshalPlatformEvent
    .macro Plugin3_Cmd1016
    FieldCommand 1016
    .endm

    // PokemonLeagueCmd_CallChampionChangeEvent
    .macro Plugin3_Cmd1017
    FieldCommand 1017
    .endm

    // PokemonLeagueCmd_CallChampionOpenEvent
    .macro Plugin3_Cmd1018
    FieldCommand 1018
    .endm

    // PokemonLeagueCmd_HideChampionActorShadow
    .macro Plugin3_Cmd1019 a0
    FieldCommand 1019
    .2byte \a0
    .endm

    // PokemonLeagueCmd_ShowChampionActorShadow
    .macro Plugin3_Cmd1020 a0
    FieldCommand 1020
    .2byte \a0
    .endm

    // PokemonLeagueCmd_SetChampionRoomOpen
    .macro Plugin3_Cmd1021
    FieldCommand 1021
    .endm

    // Plugin 4

    .macro PalParkCmd_CallMbParent
    FieldCommand 1000
    .endm

    .macro PalParkCmd_GetInfo a0, a1
    FieldCommand 1001
    .byte \a0
    .2byte \a1
    .endm

    // Plugin 5

    .macro AbyssalRuinsCmd_StartUnderwaterEffect
    FieldCommand 1000
    .endm

    .macro AbyssalRuinsCmd_GetStepCounter a0
    FieldCommand 1001
    .2byte \a0
    .endm

    // Plugin 6

    .macro WbtCmd_Create
    FieldCommand 1000
    .endm

    .macro WbtCmd_Free
    FieldCommand 1001
    .endm

    .macro WbtCmd_IsCreated a0
    FieldCommand 1002
    .2byte \a0
    .endm

    .macro WbtCmd_SetRound a0
    FieldCommand 1003
    .2byte \a0
    .endm

    .macro WbtCmd_GetRound a0
    FieldCommand 1004
    .2byte \a0
    .endm

    // WbtCmd_MakeMatch
    .macro Plugin6_Cmd1005
    FieldCommand 1005
    .endm

    // WbtCmd_IsTournamentAvailable
    .macro Plugin6_Cmd1006 a0, a1
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_WouldOpenTournament
    .macro Plugin6_Cmd1007 a0, a1
    FieldCommand 1007
    .2byte \a0
    .2byte \a1
    .endm

    .macro WbtCmd_CheckRegulation a0
    FieldCommand 1008
    .2byte \a0
    .endm

    .macro WbtCmd_SetStyle a0
    FieldCommand 1009
    .2byte \a0
    .endm

    .macro WbtCmd_GetStyle a0
    FieldCommand 1010
    .2byte \a0
    .endm

    .macro WbtCmd_SetTournament a0
    FieldCommand 1011
    .2byte \a0
    .endm

    .macro WbtCmd_GetTournament a0
    FieldCommand 1012
    .2byte \a0
    .endm

    .macro WbtCmd_SetType a0
    FieldCommand 1013
    .2byte \a0
    .endm

    .macro WbtCmd_GetType a0
    FieldCommand 1014
    .2byte \a0
    .endm

    // WbtCmd_SetCategory
    .macro Plugin6_Cmd1015 a0
    FieldCommand 1015
    .2byte \a0
    .endm

    // WbtCmd_GetCategory
    .macro Plugin6_Cmd1016 a0
    FieldCommand 1016
    .2byte \a0
    .endm

    .macro WbtCmd_RecordWin
    FieldCommand 1017
    .endm

    .macro WbtCmd_GetWinCount a0, a1, a2
    FieldCommand 1018
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro WbtCmd_AwardBattlePoints a0
    FieldCommand 1020
    .2byte \a0
    .endm

    // WbtCmd_GetSlot
    .macro Plugin6_Cmd1022 a0, a1
    FieldCommand 1022
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_GetSlotTrainerObjCodes
    .macro Plugin6_Cmd1023 a0, a1, a2
    FieldCommand 1023
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // WbtCmd_GetEntryCount
    .macro Plugin6_Cmd1024 a0
    FieldCommand 1024
    .2byte \a0
    .endm

    // WbtCmd_CountDownloads
    .macro Plugin6_Cmd1025 a0
    FieldCommand 1025
    .2byte \a0
    .endm

    // WbtCmd_SetDownloadSlot
    .macro Plugin6_Cmd1026 a0
    FieldCommand 1026
    .2byte \a0
    .endm

    // WbtCmd_GetDownloadSlot
    .macro Plugin6_Cmd1027 a0
    FieldCommand 1027
    .2byte \a0
    .endm

    // WbtCmd_SetUseBattleBox
    .macro Plugin6_Cmd1028 a0
    FieldCommand 1028
    .2byte \a0
    .endm

    // WbtCmd_GetSlotTrainerObjCodes
    .macro Plugin6_Cmd1031 a0, a1, a2
    FieldCommand 1031
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro WbtCmd_MakeRentalParty a0
    FieldCommand 1038
    .2byte \a0
    .endm

    // WbtCmd_SetTournamentNameWord
    .macro Plugin6_Cmd1044 a0, a1
    FieldCommand 1044
    .byte \a0
    .2byte \a1
    .endm

    // WbtCmd_CallEntryPartySelect
    .macro Plugin6_Cmd1046 a0
    FieldCommand 1046
    .2byte \a0
    .endm

    // WbtCmd_ShowTrainerList
    .macro Plugin6_Cmd1047 a0
    FieldCommand 1047
    .2byte \a0
    .endm

    // WbtCmd_CallPokeSelect
    .macro Plugin6_Cmd1048 a0
    FieldCommand 1048
    .2byte \a0
    .endm

    // WbtCmd_ShowDownloads
    .macro Plugin6_Cmd1049 a0, a1
    FieldCommand 1049
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_ShowWinRecord
    .macro Plugin6_Cmd1050 a0
    FieldCommand 1050
    .2byte \a0
    .endm

    .macro WbtCmd_Download a0, a1
    FieldCommand 1051
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_GetRegulationId
    .macro Plugin6_Cmd1057 a0
    FieldCommand 1057
    .2byte \a0
    .endm

    // func_ov056_021e77b0
    .macro Plugin6_Cmd1058 a0
    FieldCommand 1058
    .2byte \a0
    .endm

    // WbtCmd_CheckEntryParties
    .macro Plugin6_Cmd1059 a0
    FieldCommand 1059
    .2byte \a0
    .endm

    // Plugin 7

    // WbtCmd_MakeMatch
    .macro Plugin7_Cmd1005
    FieldCommand 1005
    .endm

    // WbtCmd_SetCategory
    .macro Plugin7_Cmd1015 a0
    FieldCommand 1015
    .2byte \a0
    .endm

    // WbtCmd_GetCategory
    .macro Plugin7_Cmd1016 a0
    FieldCommand 1016
    .2byte \a0
    .endm

    // WbtCmd_GetRoundRemaining
    .macro Plugin7_Cmd1019 a0
    FieldCommand 1019
    .2byte \a0
    .endm

    // WbtCmd_ResetPicks
    .macro Plugin7_Cmd1023
    FieldCommand 1023
    .endm

    // WbtCmd_GetEntryCount
    .macro Plugin7_Cmd1024 a0
    FieldCommand 1024
    .2byte \a0
    .endm

    // WbtCmd_ShowPartyList
    .macro Plugin7_Cmd1029
    FieldCommand 1029
    .endm

    // WbtCmd_ClosePartyList
    .macro Plugin7_Cmd1030
    FieldCommand 1030
    .endm

    // WbtCmd_ResetPicks
    .macro Plugin7_Cmd1031
    FieldCommand 1031
    .endm

    // WbtCmd_RandomPick
    .macro Plugin7_Cmd1032 a0
    FieldCommand 1032
    .2byte \a0
    .endm

    // WbtCmd_SetPick
    .macro Plugin7_Cmd1033 a0, a1
    FieldCommand 1033
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_WordSetPickSpecies
    .macro Plugin7_Cmd1034 a0, a1
    FieldCommand 1034
    .byte \a0
    .2byte \a1
    .endm

    // WbtCmd_ListPartyPokemon
    .macro Plugin7_Cmd1035
    FieldCommand 1035
    .endm

    // WbtCmd_Dummy
    .macro Plugin7_Cmd1036
    FieldCommand 1036
    .endm

    // WbtCmd_SwapPickedPokemon
    .macro Plugin7_Cmd1037
    FieldCommand 1037
    .endm

    .macro WbtCmd_MakeOpponentParty
    FieldCommand 1039
    .endm

    // WbtCmd_GetOpponentObjCode
    .macro Plugin7_Cmd1040 a0
    FieldCommand 1040
    .2byte \a0
    .endm

    // WbtCmd_ShowOpponentLine
    .macro Plugin7_Cmd1041 a0, a1
    FieldCommand 1041
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_CallBattle
    .macro Plugin7_Cmd1042 a0
    FieldCommand 1042
    .2byte \a0
    .endm

    // WbtCmd_ShowOpponentLine7
    .macro Plugin7_Cmd1043 a0
    FieldCommand 1043
    .2byte \a0
    .endm

    // WbtCmd_SetTournamentNameWord
    .macro Plugin7_Cmd1044 a0, a1
    FieldCommand 1044
    .byte \a0
    .2byte \a1
    .endm

    // WbtCmd_WordSetOpponentName
    .macro Plugin7_Cmd1045 a0
    FieldCommand 1045
    .byte \a0
    .endm

    // WbtCmd_ShowTrainerList
    .macro Plugin7_Cmd1047 a0
    FieldCommand 1047
    .2byte \a0
    .endm

    // WbtCmd_ShowPortraits
    .macro Plugin7_Cmd1052 a0
    FieldCommand 1052
    .2byte \a0
    .endm

    // WbtCmd_LoadPortraitScene
    .macro Plugin7_Cmd1053 a0, a1
    FieldCommand 1053
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_PlayStadiumIntro
    .macro Plugin7_Cmd1054
    FieldCommand 1054
    .endm

    // WbtCmd_SwitchLights
    .macro Plugin7_Cmd1055 a0, a1
    FieldCommand 1055
    .2byte \a0
    .2byte \a1
    .endm

    // WbtCmd_PlayCrowd
    .macro Plugin7_Cmd1056 a0
    FieldCommand 1056
    .2byte \a0
    .endm

    // WbtCmd_SetNamePlates
    .macro Plugin7_Cmd1060 a0
    FieldCommand 1060
    .2byte \a0
    .endm

    // Plugin 8

    // ResortCmd_StartTalk
    .macro Plugin8_Cmd1000
    FieldCommand 1000
    .endm

    // ResortCmd_EndTalk
    .macro Plugin8_Cmd1001
    FieldCommand 1001
    .endm

    // ResortCmd_GetValue
    .macro Plugin8_Cmd1002 a0, a1
    FieldCommand 1002
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_GetScriptValue
    .macro Plugin8_Cmd1003 a0, a1
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_ShowRecords
    .macro Plugin8_Cmd1004 a0, a1
    FieldCommand 1004
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_MakeData
    .macro Plugin8_Cmd1005 a0, a1
    FieldCommand 1005
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_RecreatePeople
    .macro Plugin8_Cmd1006 a0
    FieldCommand 1006
    .2byte \a0
    .endm

    // ResortCmd_SetWord
    .macro Plugin8_Cmd1007 a0, a1, a2, a3
    FieldCommand 1007
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // ResortCmd_RemovePerson
    .macro Plugin8_Cmd1008 a0, a1, a2
    FieldCommand 1008
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_AddShopExperience
    .macro Plugin8_Cmd1009 a0, a1, a2, a3
    FieldCommand 1009
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // ResortCmd_AddPopularity
    .macro Plugin8_Cmd1010 a0, a1, a2
    FieldCommand 1010
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_StartTalk
    .macro Plugin8_Cmd1011
    FieldCommand 1011
    .endm

    // ResortCmd_SaveShopId
    .macro Plugin8_Cmd1012
    FieldCommand 1012
    .endm

    // ResortCmd_SetOccupantsLocked
    .macro Plugin8_Cmd1013 a0
    FieldCommand 1013
    .2byte \a0
    .endm

    // ResortCmd_HideActors
    .macro Plugin8_Cmd1014 a0
    FieldCommand 1014
    .2byte \a0
    .endm

    // ResortCmd_RaffleShopItem
    .macro Plugin8_Cmd1015 a0, a1, a2
    FieldCommand 1015
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_SetFriendship
    .macro Plugin8_Cmd1016 a0, a1, a2
    FieldCommand 1016
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_RaiseLevel
    .macro Plugin8_Cmd1017 a0, a1, a2
    FieldCommand 1017
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_ChangeEffortValue
    .macro Plugin8_Cmd1018 a0, a1, a2, a3
    FieldCommand 1018
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // ResortCmd_ShortenEggSteps
    .macro Plugin8_Cmd1019 a0, a1, a2
    FieldCommand 1019
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_SetShopFlag
    .macro Plugin8_Cmd1020
    FieldCommand 1020
    .endm

    // ResortCmd_PayAndSetShopFlag
    .macro Plugin8_Cmd1021 a0, a1
    FieldCommand 1021
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_GetPersonVisit
    .macro Plugin8_Cmd1022 a0, a1, a2, a3, a4
    FieldCommand 1022
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    // ResortCmd_NameEntry
    .macro Plugin8_Cmd1023 a0, a1
    FieldCommand 1023
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_Visit
    .macro Plugin8_Cmd1024 a0, a1, a2, a3
    FieldCommand 1024
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // ResortCmd_CheckRank100Unlock
    .macro Plugin8_Cmd1025 a0
    FieldCommand 1025
    .2byte \a0
    .endm

    // ResortCmd_MarkRaffled
    .macro Plugin8_Cmd1026
    FieldCommand 1026
    .endm

    // ResortCmd_AddRecordPerson
    .macro Plugin8_Cmd1027 a0, a1
    FieldCommand 1027
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_AddEntryOrRecord
    .macro Plugin8_Cmd1028 a0, a1
    FieldCommand 1028
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_GetNPCValue
    .macro Plugin8_Cmd1029 a0, a1, a2
    FieldCommand 1029
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_SetOwnValue
    .macro Plugin8_Cmd1030 a0, a1
    FieldCommand 1030
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_GetOwnValue
    .macro Plugin8_Cmd1031 a0, a1
    FieldCommand 1031
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_SetOccupantWords
    .macro Plugin8_Cmd1032 a0, a1, a2
    FieldCommand 1032
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_SetNPCWord
    .macro Plugin8_Cmd1033 a0, a1, a2
    FieldCommand 1033
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_GetGoodsValue
    .macro Plugin8_Cmd1034 a0, a1, a2
    FieldCommand 1034
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_DescribeOccupant
    .macro Plugin8_Cmd1035 a0, a1, a2
    FieldCommand 1035
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // ResortCmd_ShowMedalInfo
    .macro Plugin8_Cmd1036
    FieldCommand 1036
    .endm

    // ResortCmd_StartSceneAnim
    .macro Plugin8_Cmd1037 a0
    FieldCommand 1037
    .2byte \a0
    .endm

    // ResortCmd_IsSceneAnimDone
    .macro Plugin8_Cmd1038 a0, a1
    FieldCommand 1038
    .2byte \a0
    .2byte \a1
    .endm

    // ResortCmd_EndSceneAnim
    .macro Plugin8_Cmd1039 a0
    FieldCommand 1039
    .2byte \a0
    .endm

    // Plugin 8, with overlay 60 loaded

    // ResortShopCmd_Open
    .macro Plugin8Ov60_Cmd1000 a0, a1, a2, a3, a4, a5, a6, a7
    FieldCommand 1000
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .2byte \a6
    .2byte \a7
    .endm

    // ResortShopCmd_Open
    .macro Plugin8Ov60_Cmd1011 a0, a1, a2, a3, a4, a5, a6, a7
    FieldCommand 1011
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .2byte \a6
    .2byte \a7
    .endm

    // Plugin 9

    // BlackTowerCmd_CreateWalls
    .macro Plugin9_Cmd1000
    FieldCommand 1000
    .endm

    // BlackTowerCmd_ChangeRoom
    .macro Plugin9_Cmd1001 a0, a1, a2
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // BlackTowerCmd_StartRun
    .macro Plugin9_Cmd1003
    FieldCommand 1003
    .endm

    // BlackTowerCmd_GetFloorInArea
    .macro Plugin9_Cmd1004 a0
    FieldCommand 1004
    .2byte \a0
    .endm

    // BlackTowerCmd_ChangeFloor
    .macro Plugin9_Cmd1005 a0
    FieldCommand 1005
    .2byte \a0
    .endm

    // BlackTowerCmd_GetArea
    .macro Plugin9_Cmd1006 a0
    FieldCommand 1006
    .2byte \a0
    .endm

    // BlackTowerCmd_CallBattleEvent
    .macro Plugin9_Cmd1007
    FieldCommand 1007
    .endm

    // func_ov061_021e5a90
    .macro Plugin9_Cmd1008 a0
    FieldCommand 1008
    .2byte \a0
    .endm

    // BlackTowerCmd_SetTrainerDefeated
    .macro Plugin9_Cmd1009 a0, a1
    FieldCommand 1009
    .2byte \a0
    .2byte \a1
    .endm

    // func_ov061_021e5bac
    .macro Plugin9_Cmd1010 a0
    FieldCommand 1010
    .2byte \a0
    .endm

    // BlackTowerCmd_IsTrainerDefeated
    .macro Plugin9_Cmd1011 a0, a1
    FieldCommand 1011
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_GetNpcScript
    .macro Plugin9_Cmd1012 a0
    FieldCommand 1012
    .2byte \a0
    .endm

    // func_ov061_021e5bc4
    .macro Plugin9_Cmd1013 a0
    FieldCommand 1013
    .2byte \a0
    .endm

    // BlackTowerCmd_GetSpecialTrainerFlag
    .macro Plugin9_Cmd1014 a0, a1
    FieldCommand 1014
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_SetSpecialTrainerFlag
    .macro Plugin9_Cmd1015 a0, a1
    FieldCommand 1015
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_CallDoorEvent
    .macro Plugin9_Cmd1016 a0, a1
    FieldCommand 1016
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_GetGoalFloor
    .macro Plugin9_Cmd1017 a0
    FieldCommand 1017
    .2byte \a0
    .endm

    // BlackTowerCmd_GetTrainerClass
    .macro Plugin9_Cmd1018 a0, a1
    FieldCommand 1018
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_SetProgress
    .macro Plugin9_Cmd1019 a0
    FieldCommand 1019
    .2byte \a0
    .endm

    // func_ov061_021e5da4
    .macro Plugin9_Cmd1020 a0
    FieldCommand 1020
    .2byte \a0
    .endm

    // func_ov061_021e5dd4
    .macro Plugin9_Cmd1021 a0
    FieldCommand 1021
    .2byte \a0
    .endm

    // BlackTowerCmd_GetTrainerId
    .macro Plugin9_Cmd1022 a0, a1
    FieldCommand 1022
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_GetVersion
    .macro Plugin9_Cmd1023 a0
    FieldCommand 1023
    .2byte \a0
    .endm

    // BlackTowerCmd_GetInfo
    .macro Plugin9_Cmd1024 a0, a1, a2
    FieldCommand 1024
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // BlackTowerCmd_CallMachineEvent
    .macro Plugin9_Cmd1025
    FieldCommand 1025
    .endm

    // BlackTowerCmd_IsTargetSlot
    .macro Plugin9_Cmd1026 a0, a1
    FieldCommand 1026
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_GetNpcScriptByResult
    .macro Plugin9_Cmd1027 a0, a1
    FieldCommand 1027
    .2byte \a0
    .2byte \a1
    .endm

    // BlackTowerCmd_GetRoomNpcCount
    .macro Plugin9_Cmd1028 a0
    FieldCommand 1028
    .2byte \a0
    .endm

    // BlackTowerCmd_HasEarnedMoney
    .macro Plugin9_Cmd1029 a0
    FieldCommand 1029
    .2byte \a0
    .endm

    // BlackTowerCmd_CallLiftEvent
    .macro Plugin9_Cmd1030
    FieldCommand 1030
    .endm

    // BlackTowerCmd_TakeEarnedMoney
    .macro Plugin9_Cmd1031 a0
    FieldCommand 1031
    .2byte \a0
    .endm

    // Plugin 10

    // PokewoodCmd_Film
    .macro Plugin10_Cmd1000 a0, a1, a2
    FieldCommand 1000
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // PokewoodCmd_WatchSlot
    .macro Plugin10_Cmd1001 a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    // PokewoodCmd_PickMovie
    .macro Plugin10_Cmd1002 a0
    FieldCommand 1002
    .2byte \a0
    .endm

    // PokewoodCmd_OpenMenu
    .macro Plugin10_Cmd1003 a0, a1
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .endm

    // PokewoodCmd_SetSaveSlot
    .macro Plugin10_Cmd1004 a0
    FieldCommand 1004
    .2byte \a0
    .endm

    // PokewoodCmd_SetMessageWord
    .macro Plugin10_Cmd1005 a0, a1
    FieldCommand 1005
    .byte \a0
    .2byte \a1
    .endm

    .macro PokewoodCmd_GetMovieFlag a0, a1, a2
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro PokewoodCmd_SetMovieFlag a0, a1
    FieldCommand 1007
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokewoodCmd_CountDownloadedMovies a0
    FieldCommand 1008
    .2byte \a0
    .endm

    .macro PokewoodCmd_CheckDownloadedMovie a0, a1
    FieldCommand 1009
    .2byte \a0
    .2byte \a1
    .endm

    // PokewoodCmd_GetDownloadedMovieResult
    .macro Plugin10_Cmd1010 a0, a1
    FieldCommand 1010
    .2byte \a0
    .2byte \a1
    .endm

    .macro PokewoodCmd_Create
    FieldCommand 1011
    .endm

    .macro PokewoodCmd_Free
    FieldCommand 1012
    .endm

    // PokewoodCmd_SelectCast
    .macro Plugin10_Cmd1013 a0
    FieldCommand 1013
    .2byte \a0
    .endm

    // PokewoodCmd_SetUsePlayerCast
    .macro Plugin10_Cmd1014 a0
    FieldCommand 1014
    .2byte \a0
    .endm

    // PokewoodCmd_GetNewUnlockKind
    .macro Plugin10_Cmd1015 a0, a1, a2
    FieldCommand 1015
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // PokewoodCmd_TestAndSetLevelBit
    .macro Plugin10_Cmd1016 a0
    FieldCommand 1016
    .2byte \a0
    .endm

    // PokewoodCmd_GetSlotFlag
    .macro Plugin10_Cmd1017 a0, a1
    FieldCommand 1017
    .2byte \a0
    .2byte \a1
    .endm

    // func_ov062_021e5f1c
    .macro Plugin10_Cmd1018 a0, a1
    FieldCommand 1018
    .2byte \a0
    .2byte \a1
    .endm

    // func_ov062_021e5f50
    .macro Plugin10_Cmd1019 a0, a1, a2
    FieldCommand 1019
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // func_ov062_021e5fec
    .macro Plugin10_Cmd1020 a0, a1
    FieldCommand 1020
    .2byte \a0
    .2byte \a1
    .endm

    // func_ov062_021e6010
    .macro Plugin10_Cmd1021 a0
    FieldCommand 1021
    .2byte \a0
    .endm

    // PokewoodCmd_SetSlotFlag
    .macro Plugin10_Cmd1022 a0
    FieldCommand 1022
    .2byte \a0
    .endm

    // PokewoodCmd_OpenRecordMonitor
    .macro Plugin10_Cmd1023 a0
    FieldCommand 1023
    .2byte \a0
    .endm

    // func_ov062_021e60a8
    .macro Plugin10_Cmd1024 a0, a1, a2, a3
    FieldCommand 1024
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro PokewoodCmd_CountMovieFlags a0, a1
    FieldCommand 1025
    .2byte \a0
    .2byte \a1
    .endm

    // PokewoodCmd_WatchBlock
    .macro Plugin10_Cmd1026
    FieldCommand 1026
    .endm

    // Plugin 11

    .macro BadgeGateCmd_PlayCheck a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro BadgeGateCmd_PlayLastGate
    FieldCommand 1001
    .endm

    // Plugin 12

    // PlasmaFrigateCmd_PressSwitch
    .macro Plugin12_Cmd1000 a0
    FieldCommand 1000
    .2byte \a0
    .endm

    // PlasmaFrigateCmd_SetLightsState
    .macro Plugin12_Cmd1001 a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    // PlasmaFrigateCmd_PlayGroupASecond
    .macro Plugin12_Cmd1002 a0
    FieldCommand 1002
    .2byte \a0
    .endm

    // PlasmaFrigateCmd_PlayGroupBSecond
    .macro Plugin12_Cmd1003 a0
    FieldCommand 1003
    .2byte \a0
    .endm

    .macro PlasmaFrigateCmd_EnterPassword a0
    FieldCommand 1004
    .2byte \a0
    .endm

    .macro PlasmaFrigateCmd_GetPasswordIndex a0
    FieldCommand 1005
    .2byte \a0
    .endm

    // PlasmaFrigateCmd_SetSwitchLightState
    .macro Plugin12_Cmd1006 a0, a1
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .endm

    // PlasmaFrigateCmd_PlaySwitchAnim
    .macro Plugin12_Cmd1007 a0
    FieldCommand 1007
    .2byte \a0
    .endm

    // PlasmaFrigateCmd_PlaySwitchLightAnim
    .macro Plugin12_Cmd1008 a0
    FieldCommand 1008
    .2byte \a0
    .endm

    // Plugin 13

    .macro PokemonCenterCmd_Medal a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro PokemonCenterCmd_FindEarnedMedal a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    // PokemonCenterCmd_GetMatchState
    .macro Plugin13_Cmd1005 a0, a1
    FieldCommand 1005
    .2byte \a0
    .2byte \a1
    .endm

    // PokemonCenterCmd_SetMatchInProgress
    .macro Plugin13_Cmd1006
    FieldCommand 1006
    .endm

    .macro PokemonCenterCmd_ClearMatchInProgress
    FieldCommand 1007
    .endm

    // PokemonCenterCmd_SetMatchState
    .macro Plugin13_Cmd1008 a0, a1
    FieldCommand 1008
    .2byte \a0
    .2byte \a1
    .endm

    // PokemonCenterCmd_AddBattlePoints
    .macro Plugin13_Cmd1009 a0
    FieldCommand 1009
    .2byte \a0
    .endm

    // Plugin 14

    // Plugin14Cmd_StartAspertiaSeasonAnimations
    .macro Plugin14_Cmd1000
    FieldCommand 1000
    .endm

    // Plugin14Cmd_ShowGC6Model
    .macro Plugin14_Cmd1001 a0
    FieldCommand 1001
    .2byte \a0
    .endm

    // Plugin14Cmd_PlayGC6Model
    .macro Plugin14_Cmd1002 a0
    FieldCommand 1002
    .2byte \a0
    .endm

    // Plugin14Cmd_StartActorHover
    .macro Plugin14_Cmd1003 a0
    FieldCommand 1003
    .2byte \a0
    .endm

    // Plugin14Cmd_PlayGC6Model23
    .macro Plugin14_Cmd1004 a0
    FieldCommand 1004
    .2byte \a0
    .endm

    // Plugin14Cmd_HideGC6Model3
    .macro Plugin14_Cmd1005
    FieldCommand 1005
    .endm

    // Plugin14Cmd_ShiftActor
    .macro Plugin14Cmd_ShiftActor a0, a1, a2
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // Plugin14Cmd_ResetLCDToBlack
    .macro Plugin14_Cmd1007
    FieldCommand 1007
    .endm

    // Plugin14Cmd_StartLeagueCenterAnimations
    .macro Plugin14_Cmd1009
    FieldCommand 1009
    .endm

    // Plugin14Cmd_FadeOutSeq
    .macro Plugin14Cmd_FadeOutSeq a0
    FieldCommand 1010
    .2byte \a0
    .endm

    // Plugin14Cmd_LoadAspertiaBG3Picture
    .macro Plugin14_Cmd1011
    FieldCommand 1011
    .endm

    // Plugin14Cmd_CallDT9ReleaseEvent
    .macro Plugin14_Cmd1012
    FieldCommand 1012
    .endm

    // Plugin14Cmd_CallDT9ThrowEvent
    .macro Plugin14_Cmd1013 a0, a1, a2, a3, a4, a5
    FieldCommand 1013
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    // Plugin14Cmd_PlayDT9Thrown
    .macro Plugin14_Cmd1014 a0, a1, a2
    FieldCommand 1014
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // Plugin14Cmd_CallDT9WaitThrownDoneEvent
    .macro Plugin14_Cmd1015
    FieldCommand 1015
    .endm

    // Plugin14Cmd_CallDT9WaitFifthFrameEvent
    .macro Plugin14_Cmd1016
    FieldCommand 1016
    .endm

    // Plugin 15

    // Plugin15Cmd_ShowSplash
    .macro Plugin15_Cmd1000 a0, a1, a2
    FieldCommand 1000
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    // Plugin15Cmd_JumpActorOut
    .macro Plugin15_Cmd1001 a0, a1, a2, a3
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // Plugin 16

    // Plugin16Cmd_ShowRuinsModel
    .macro Plugin16_Cmd1000 a0
    FieldCommand 1000
    .2byte \a0
    .endm

    // Plugin16Cmd_SetRuinsAnimFrame
    .macro Plugin16_Cmd1001 a0
    FieldCommand 1001
    .2byte \a0
    .endm

    // Plugin16Cmd_PlayRuinsAnim
    .macro Plugin16_Cmd1002 a0
    FieldCommand 1002
    .2byte \a0
    .endm

    // Plugin16Cmd_CallFormEvent
    .macro Plugin16_Cmd1003
    FieldCommand 1003
    .endm

    // Plugin16Cmd_HideActor2
    .macro Plugin16_Cmd1004
    FieldCommand 1004
    .endm

    // Plugin16Cmd_StartChase
    .macro Plugin16_Cmd1005 a0
    FieldCommand 1005
    .2byte \a0
    .endm

    // Plugin16Cmd_WaitChase
    .macro Plugin16_Cmd1006
    FieldCommand 1006
    .endm

    // Plugin16Cmd_DropActor
    .macro Plugin16_Cmd1007
    FieldCommand 1007
    .endm
