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

    .macro SystemMsgClose
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

    .macro InfoMsgClose
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

    .macro TrainerEyeGetBattleType a0
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

    .macro TrainerGetBattleType a0, a1
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

    .macro ListMenuAnchorTopLeft a0, a1, a2, a3, a4
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

    .macro ListMenuAnchorTopRight a0, a1, a2, a3, a4
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

    .macro ConfigToggleKanji
    FieldCommand 201
    .endm

    .macro ConfigGetKanji a0
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

    .macro TrainerCardGetStars a0
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

    .macro BoxUnlockWallpapers a0
    FieldCommand 228
    .2byte \a0
    .endm

    .macro GetTrainerViewGroupAndMarkSeen a0, a1
    FieldCommand 229
    .2byte \a0
    .2byte \a1
    .endm

    .macro StartMenuIsItemUnlocked a0, a1
    FieldCommand 230
    .2byte \a0
    .2byte \a1
    .endm

    .macro StartMenuUnlockItem a0
    FieldCommand 231
    .2byte \a0
    .endm

    .macro StartMenuIsItemSelected a0, a1
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

    .macro GameCommTryBoot
    FieldCommand 316
    .endm

    .macro GameCommPause
    FieldCommand 317
    .endm

    .macro GameCommResume
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

    .macro WifiListHasValidProfile a0
    FieldCommand 347
    .2byte \a0
    .endm

    .macro WifiListCountFriends a0
    FieldCommand 348
    .2byte \a0
    .endm

    .macro NetConnectWiFiClub
    FieldCommand 349
    .endm

    .macro NetConnectGTS
    FieldCommand 350
    .endm

    .macro Nop
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

    .macro MusicalCall a0, a1
    FieldCommand 355
    .2byte \a0
    .byte \a1
    .endm

    .macro MusicalCallDressUp a0
    FieldCommand 356
    .2byte \a0
    .endm

    .macro MusicalGetInfo a0, a1, a2
    FieldCommand 357
    .byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MusicalGetAudienceGift a0, a1, a2
    FieldCommand 358
    .2byte \a0
    .byte \a1
    .2byte \a2
    .endm

    .macro MusicalComm a0, a1, a2, a3
    FieldCommand 359
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro MusicalAddProp a0
    FieldCommand 360
    .2byte \a0
    .endm

    .macro MusicalClearAudienceGift a0
    FieldCommand 361
    .2byte \a0
    .endm

    .macro MusicalSelectPoke a0, a1
    FieldCommand 362
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallMonolith
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

    .macro GimmickGateViewBoard a0
    FieldCommand 378
    .2byte \a0
    .endm

    // func_ov012_02157bdc
    .macro Cmd_017B a0, a1
    FieldCommand 379
    .2byte \a0
    .2byte \a1
    .endm

    .macro RollerCoasterCallSwitch a0
    FieldCommand 380
    .2byte \a0
    .endm

    .macro RollerCoasterSetCarIntroSeen a0
    FieldCommand 381
    .2byte \a0
    .endm

    .macro DriftveilGymPrepareFreight
    FieldCommand 390
    .endm

    .macro DriftveilLiftPlaceAtBottom a0
    FieldCommand 391
    .2byte \a0
    .endm

    .macro DriftveilGymCallElevator a0, a1
    FieldCommand 396
    .2byte \a0
    .2byte \a1
    .endm

    .macro DriftveilGymCallFreight a0
    FieldCommand 397
    .2byte \a0
    .endm

    .macro DriftveilLiftCallMove a0
    FieldCommand 398
    .2byte \a0
    .endm

    .macro IcirrusGymCallSwitchAnim a0
    FieldCommand 399
    .2byte \a0
    .endm

    .macro IcirrusGymCallFlipSwitch a0
    FieldCommand 400
    .2byte \a0
    .endm

    .macro IcirrusGymToggleMovementAtPlace a0
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

    .macro PleasureBoatGetRoomInfo a0, a1, a2, a3
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

    .macro FadeInBlackQ_01A8
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

    .macro RegisterAllowedSpecies a0, a1
    FieldCommand 431
    .2byte \a0
    .2byte \a1
    .endm

    .macro CheckPartyRegulation a0, a1
    FieldCommand 432
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallLeagueLiftWarp
    FieldCommand 433
    .endm

    .macro SpawnMovePokemonIfInactive a0
    FieldCommand 434
    .2byte \a0
    .endm

    .macro ReadSaveByte a0, a1
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

    .macro ElevatorGetMysteryEntryCount a0
    FieldCommand 460
    .2byte \a0
    .endm

    .macro ElevatorRegisterBirthdayEntry a0
    FieldCommand 461
    .2byte \a0
    .endm

    .macro UnityTowerGetTrainerClass a0, a1
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

    .macro DreamWorldItems a0, a1
    FieldCommand 476
    .2byte \a0
    .2byte \a1
    .endm

    .macro BroadcastGameBeacon a0, a1, a2
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

    .macro CallPassPowerFlash
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

    .macro TrialHouseSetTrainer a0, a1
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

    .macro TrialHouseSetDownloadMode a0
    FieldCommand 493
    .2byte \a0
    .endm

    .macro TrialHouseSetBattleTestRank a0
    FieldCommand 494
    .2byte \a0
    .endm

    .macro TrialHouseGetBattleTestRank a0
    FieldCommand 495
    .2byte \a0
    .endm

    .macro TrialHouseDownloadExamination a0
    FieldCommand 496
    .2byte \a0
    .endm

    .macro TrialHouseCalcPointsStars a0, a1
    FieldCommand 497
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrialHouseGetExaminationType a0
    FieldCommand 498
    .2byte \a0
    .endm

    .macro TrialHouseSaveData a0
    FieldCommand 499
    .2byte \a0
    .endm

    .macro TrialHouseCallResults a0, a1
    FieldCommand 500
    .2byte \a0
    .2byte \a1
    .endm

    .macro TrialHouseGetRecordMask a0
    FieldCommand 501
    .2byte \a0
    .endm

    .macro CheckWifiListFlags a0, a1, a2, a3
    FieldCommand 502
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro WifiListOperation a0, a1, a2, a3
    FieldCommand 503
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro SetWifiFriendNameWord a0, a1
    FieldCommand 504
    .2byte \a0
    .2byte \a1
    .endm

    .macro CheckPmsWordPair a0, a1, a2, a3
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

    .macro SurveyStart a0, a1, a2, a3
    FieldCommand 509
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro SurveyClear
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

    .macro SurveyGetQuestionState a0, a1
    FieldCommand 514
    .2byte \a0
    .2byte \a1
    .endm

    .macro SurveyGetCurrentRemaining a0
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

    .macro WordSetPasserbyCount a0
    FieldCommand 519
    .byte \a0
    .endm

    .macro WordSetThankedCount a0
    FieldCommand 520
    .byte \a0
    .endm

    .macro SurveyWriteOwnAnswer a0, a1
    FieldCommand 521
    .2byte \a0
    .2byte \a1
    .endm

    .macro SurveyShowRankMessage
    FieldCommand 522
    .endm

    .macro SurveyGetRank a0
    FieldCommand 523
    .2byte \a0
    .endm

    .macro SurveyRaiseRank
    FieldCommand 524
    .endm

    .macro PokePartyFindEx a0, a1, a2, a3
    FieldCommand 525
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro SpPokeJump a0, a1, a2, a3, a4, a5
    FieldCommand 526
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro SpPokeAppear a0, a1, a2, a3
    FieldCommand 527
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro SpPokeWaitAnimEnd a0
    FieldCommand 528
    .2byte \a0
    .endm

    .macro SpPokeWaitFrame a0
    FieldCommand 529
    .2byte \a0
    .endm

    .macro ActorMoveToHeight a0, a1, a2
    FieldCommand 530
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ActorHoldPos a0
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

    .macro SetSpecialTransferUsed a0, a1
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

    .macro EntreeForestRemovePkm a0
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

    .macro StartCountOtherOtIdsTask a0
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

    .macro TVGenProgramMsgID a0, a1
    FieldCommand 554
    .2byte \a0
    .2byte \a1
    .endm

    .macro StartSwapNpcEvent a0, a1
    FieldCommand 555
    .2byte \a0
    .2byte \a1
    .endm

    .macro FieldEffect a0
    FieldCommand 557
    .2byte \a0
    .endm

    .macro SetActorZoneDeleteOff a0
    FieldCommand 558
    .2byte \a0
    .endm

    .macro WordSetLoadPastTradePkmName a0, a1
    FieldCommand 559
    .2byte \a0
    .2byte \a1
    .endm

    .macro MsgPlaceSignFont4 a0, a1
    FieldCommand 560
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallGameManual a0
    FieldCommand 561
    .2byte \a0
    .endm

    .macro ControlLegendMon a0, a1
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

    .macro CheckPlayerFormModel a0
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

    .macro NuvemaGimmickPlay
    FieldCommand 572
    .endm

    .macro NuvemaGimmickWaitAnim
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

    .macro FadeBgmVolume a0, a1
    FieldCommand 576
    .2byte \a0
    .2byte \a1
    .endm

    .macro RestoreBgmVolume a0
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

    .macro PlayerMoveToYAsync_0248 a0, a1, a2, a3
    FieldCommand 584
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro GetRandomDistinctSix a0, a1, a2, a3, a4, a5
    FieldCommand 585
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro FieldSubscreenIsEnabled a0
    FieldCommand 586
    .2byte \a0
    .endm

    .macro FieldSubscreenDisable
    FieldCommand 587
    .endm

    .macro BGMAmbienceResume
    FieldCommand 588
    .endm

    .macro RollerCoasterAmbienceStart
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

    .macro GymElecReapplyProgress
    FieldCommand 603
    .endm

    .macro GymElecSetFollower a0, a1, a2
    FieldCommand 604
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro GymElecSetProgress a0
    FieldCommand 605
    .2byte \a0
    .endm

    .macro GymElecSetStage a0
    FieldCommand 607
    .2byte \a0
    .endm

    .macro HollowRivalSetValue a0, a1
    FieldCommand 610
    .2byte \a0
    .2byte \a1
    .endm

    .macro HollowRivalSetFlag a0
    FieldCommand 611
    .2byte \a0
    .endm

    .macro HollowRivalCmd_0264 a0, a1
    FieldCommand 612
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallXTransceiverSecret a0
    FieldCommand 613
    .2byte \a0
    .endm

    .macro HollowRivalGetCounter a0
    FieldCommand 614
    .2byte \a0
    .endm

    .macro HollowRivalCmd_0267
    FieldCommand 615
    .endm

    .macro GymInsectPlayObject a0
    FieldCommand 616
    .2byte \a0
    .endm

    .macro GymInsectShowEffect
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

    .macro MedalGetSpinTradeActor a0, a1
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

    .macro WordSetFunfestMission a0, a1, a2
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

    .macro MistraltonGymFireCannon a0
    FieldCommand 654
    .2byte \a0
    .endm

    .macro CallRivalNameInput a0
    FieldCommand 655
    .2byte \a0
    .endm

    .macro WordSetLoadRivalName a0
    FieldCommand 656
    .byte \a0
    .endm

    .macro VirbankGymStopModelAnimations a0
    FieldCommand 657
    .2byte \a0
    .endm

    .macro VirbankGymSetActorFlag a0
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

    .macro WordSetEmptyFileMessage a0, a1
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

    .macro HumilauGymPlayLeaderSplash
    FieldCommand 674
    .endm

    .macro HumilauGymPlayLeaderWaves
    FieldCommand 675
    .endm

    // func_ov012_02157bb4
    .macro Cmd_02A4
    FieldCommand 676
    .endm

    .macro OpelucidGymCallMove a0
    FieldCommand 677
    .2byte \a0
    .endm

    .macro OpelucidGymRefreshVars
    FieldCommand 678
    .endm

    .macro OpelucidGymCallButton a0
    FieldCommand 679
    .2byte \a0
    .endm

    .macro OpelucidGymFreeHeap
    FieldCommand 680
    .endm

    .macro OpelucidGymCreateHeap
    FieldCommand 681
    .endm

    .macro CallLeafPileStuck
    FieldCommand 684
    .endm

    .macro GameGetMyCity a0
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

    .macro KeyInfoGetChamberKey a0
    FieldCommand 689
    .2byte \a0
    .endm

    .macro MemoryLinkCheckFlag a0, a1
    FieldCommand 690
    .2byte \a0
    .2byte \a1
    .endm

    .macro MemoryLinkGetVar a0, a1
    FieldCommand 691
    .2byte \a0
    .2byte \a1
    .endm

    .macro MemoryLinkIsPlayerLinked a0, a1
    FieldCommand 692
    .2byte \a0
    .2byte \a1
    .endm

    .macro WordSetMemoryLinkPlayerName a0, a1
    FieldCommand 693
    .2byte \a0
    .2byte \a1
    .endm

    .macro MemoryLinkCheckMusical a0, a1
    FieldCommand 694
    .2byte \a0
    .2byte \a1
    .endm

    .macro MemoryLinkAddMusicalProps a0
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

    .macro HiddenHollowSetPeriod
    FieldCommand 708
    .endm

    .macro JoinAvenueRecordActivity a0
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

    .macro PokewoodGetLevel a0
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

    .macro KeyInfoGetMyCityProgress a0
    FieldCommand 721
    .2byte \a0
    .endm

    .macro FieldOpenRestoreLCD
    FieldCommand 722
    .endm

    .macro HollowRivalGetValue a0, a1
    FieldCommand 723
    .2byte \a0
    .2byte \a1
    .endm

    .macro ItemGetTMCount a0
    FieldCommand 724
    .2byte \a0
    .endm

    .macro WbtIsTrainerBeaten a0, a1
    FieldCommand 725
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetWifiHistoryCountryCount a0, a1
    FieldCommand 727
    .2byte \a0
    .2byte \a1
    .endm

    .macro MarkPartnerCandidate a0
    FieldCommand 728
    .2byte \a0
    .endm

    .macro UnityTowerGetVisitorParam a0, a1
    FieldCommand 729
    .2byte \a0
    .2byte \a1
    .endm

    .macro UnityTowerSetRatingLevel a0
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

    .macro CallFieldFadeOut a0, a1
    FieldCommand 744
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallFieldFadeIn a0, a1
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

    .macro SetSoftResetBlocked a0, a1
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

    .macro ClearWork
    FieldCommand 1000
    .endm

    .macro CreateWork a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    .macro FreeWork
    FieldCommand 1002
    .endm

    .macro Tool a0, a1, a2, a3
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // Plugin 2

    .macro PleasureBoat_Create a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro PleasureBoat_Free
    FieldCommand 1001
    .endm

    .macro PleasureBoat_GetInfo a0, a1
    FieldCommand 1002
    .2byte \a0
    .2byte \a1
    .endm

    .macro AdvanceClock a0, a1
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .endm

    .macro SetTrainerInfo a0, a1, a2, a3
    FieldCommand 1004
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro StopClock
    FieldCommand 1005
    .endm

    // Plugin 3

    .macro DisableRailLines a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro SetCamera a0
    FieldCommand 1001
    .2byte \a0
    .endm

    .macro PlayGrimsleyGroup a0
    FieldCommand 1002
    .2byte \a0
    .endm

    .macro PlayGrimsleyModel0Animation a0
    FieldCommand 1003
    .2byte \a0
    .endm

    .macro PlayCaitlinAmbience
    FieldCommand 1004
    .endm

    .macro PlayCaitlinModels34Or5 a0
    FieldCommand 1005
    .2byte \a0
    .endm

    .macro CallCaitlinModel1Event
    FieldCommand 1006
    .endm

    .macro PlayCaitlinModel0
    FieldCommand 1007
    .endm

    .macro DropCaitlinActor a0, a1
    FieldCommand 1008
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallShauntalModel8Event
    FieldCommand 1009
    .endm

    .macro PlayShauntalModel a0
    FieldCommand 1010
    .2byte \a0
    .endm

    .macro CallShauntalRoomChangeEvent a0
    FieldCommand 1011
    .2byte \a0
    .endm

    .macro CallShauntalModels6And9Event
    FieldCommand 1012
    .endm

    .macro PlayMarshalModel0
    FieldCommand 1013
    .endm

    .macro PlayMarshalModel3Animation a0
    FieldCommand 1014
    .2byte \a0
    .endm

    .macro PlayMarshalModel1
    FieldCommand 1015
    .endm

    .macro CallMarshalPlatformEvent
    FieldCommand 1016
    .endm

    .macro CallChampionChangeEvent
    FieldCommand 1017
    .endm

    .macro CallChampionOpenEvent
    FieldCommand 1018
    .endm

    .macro HideChampionActorShadow a0
    FieldCommand 1019
    .2byte \a0
    .endm

    .macro ShowChampionActorShadow a0
    FieldCommand 1020
    .2byte \a0
    .endm

    .macro SetChampionRoomOpen
    FieldCommand 1021
    .endm

    // Plugin 4

    .macro CallMbParent
    FieldCommand 1000
    .endm

    .macro Palpark_GetInfo a0, a1
    FieldCommand 1001
    .byte \a0
    .2byte \a1
    .endm

    // Plugin 5

    .macro StartUnderwaterEffect
    FieldCommand 1000
    .endm

    .macro GetStepCounter a0
    FieldCommand 1001
    .2byte \a0
    .endm

    // Plugin 6

    .macro Wbt_Create
    FieldCommand 1000
    .endm

    .macro Wbt_Free
    FieldCommand 1001
    .endm

    .macro IsCreated a0
    FieldCommand 1002
    .2byte \a0
    .endm

    .macro SetRound a0
    FieldCommand 1003
    .2byte \a0
    .endm

    .macro GetRound a0
    FieldCommand 1004
    .2byte \a0
    .endm

    .macro MakeMatch
    FieldCommand 1005
    .endm

    .macro IsTournamentAvailable a0, a1
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .endm

    .macro WouldOpenTournament a0, a1
    FieldCommand 1007
    .2byte \a0
    .2byte \a1
    .endm

    .macro CheckRegulation a0
    FieldCommand 1008
    .2byte \a0
    .endm

    .macro SetStyle a0
    FieldCommand 1009
    .2byte \a0
    .endm

    .macro GetStyle a0
    FieldCommand 1010
    .2byte \a0
    .endm

    .macro SetTournament a0
    FieldCommand 1011
    .2byte \a0
    .endm

    .macro GetTournament a0
    FieldCommand 1012
    .2byte \a0
    .endm

    .macro SetType a0
    FieldCommand 1013
    .2byte \a0
    .endm

    .macro GetType a0
    FieldCommand 1014
    .2byte \a0
    .endm

    .macro SetCategory a0
    FieldCommand 1015
    .2byte \a0
    .endm

    .macro GetCategory a0
    FieldCommand 1016
    .2byte \a0
    .endm

    .macro RecordWin
    FieldCommand 1017
    .endm

    .macro GetWinCount a0, a1, a2
    FieldCommand 1018
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro AwardBattlePoints a0
    FieldCommand 1020
    .2byte \a0
    .endm

    .macro GetSlot a0, a1
    FieldCommand 1022
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetSlotTrainerObjCodes_1023 a0, a1, a2
    FieldCommand 1023
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro GetEntryCount a0
    FieldCommand 1024
    .2byte \a0
    .endm

    .macro CountDownloads a0
    FieldCommand 1025
    .2byte \a0
    .endm

    .macro SetDownloadSlot a0
    FieldCommand 1026
    .2byte \a0
    .endm

    .macro GetDownloadSlot a0
    FieldCommand 1027
    .2byte \a0
    .endm

    .macro SetUseBattleBox a0
    FieldCommand 1028
    .2byte \a0
    .endm

    .macro GetSlotTrainerObjCodes_1031 a0, a1, a2
    FieldCommand 1031
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro MakeRentalParty a0
    FieldCommand 1038
    .2byte \a0
    .endm

    .macro SetTournamentNameWord a0, a1
    FieldCommand 1044
    .byte \a0
    .2byte \a1
    .endm

    .macro CallEntryPartySelect a0
    FieldCommand 1046
    .2byte \a0
    .endm

    .macro ShowTrainerList a0
    FieldCommand 1047
    .2byte \a0
    .endm

    .macro WbtEntrance_CallPokeSelect a0
    FieldCommand 1048
    .2byte \a0
    .endm

    .macro ShowDownloads a0, a1
    FieldCommand 1049
    .2byte \a0
    .2byte \a1
    .endm

    .macro ShowWinRecord a0
    FieldCommand 1050
    .2byte \a0
    .endm

    .macro Download a0, a1
    FieldCommand 1051
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetRegulationId a0
    FieldCommand 1057
    .2byte \a0
    .endm

    // func_ov056_021e77b0
    .macro Plugin6_Cmd1058 a0
    FieldCommand 1058
    .2byte \a0
    .endm

    .macro CheckEntryParties a0
    FieldCommand 1059
    .2byte \a0
    .endm

    // Plugin 7

    .macro GetRoundRemaining a0
    FieldCommand 1019
    .2byte \a0
    .endm

    .macro ResetPicks_1023
    FieldCommand 1023
    .endm

    .macro ShowPartyList
    FieldCommand 1029
    .endm

    .macro ClosePartyList
    FieldCommand 1030
    .endm

    .macro ResetPicks_1031
    FieldCommand 1031
    .endm

    .macro RandomPick a0
    FieldCommand 1032
    .2byte \a0
    .endm

    .macro SetPick a0, a1
    FieldCommand 1033
    .2byte \a0
    .2byte \a1
    .endm

    .macro WordSetPickSpecies a0, a1
    FieldCommand 1034
    .byte \a0
    .2byte \a1
    .endm

    .macro ListPartyPokemon
    FieldCommand 1035
    .endm

    .macro Dummy
    FieldCommand 1036
    .endm

    .macro SwapPickedPokemon
    FieldCommand 1037
    .endm

    .macro MakeOpponentParty
    FieldCommand 1039
    .endm

    .macro GetOpponentObjCode a0
    FieldCommand 1040
    .2byte \a0
    .endm

    .macro ShowOpponentLine a0, a1
    FieldCommand 1041
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallBattle a0
    FieldCommand 1042
    .2byte \a0
    .endm

    .macro ShowOpponentLine7 a0
    FieldCommand 1043
    .2byte \a0
    .endm

    .macro WordSetOpponentName a0
    FieldCommand 1045
    .byte \a0
    .endm

    .macro ShowPortraits a0
    FieldCommand 1052
    .2byte \a0
    .endm

    .macro LoadPortraitScene a0, a1
    FieldCommand 1053
    .2byte \a0
    .2byte \a1
    .endm

    .macro PlayStadiumIntro
    FieldCommand 1054
    .endm

    .macro SwitchLights a0, a1
    FieldCommand 1055
    .2byte \a0
    .2byte \a1
    .endm

    .macro PlayCrowd a0
    FieldCommand 1056
    .2byte \a0
    .endm

    .macro SetNamePlates a0
    FieldCommand 1060
    .2byte \a0
    .endm

    // Plugin 8

    .macro StartTalk_1000
    FieldCommand 1000
    .endm

    .macro EndTalk
    FieldCommand 1001
    .endm

    .macro GetValue a0, a1
    FieldCommand 1002
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetScriptValue a0, a1
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .endm

    .macro ShowRecords a0, a1
    FieldCommand 1004
    .2byte \a0
    .2byte \a1
    .endm

    .macro MakeData a0, a1
    FieldCommand 1005
    .2byte \a0
    .2byte \a1
    .endm

    .macro RecreatePeople a0
    FieldCommand 1006
    .2byte \a0
    .endm

    .macro SetWord a0, a1, a2, a3
    FieldCommand 1007
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro RemovePerson a0, a1, a2
    FieldCommand 1008
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro AddShopExperience a0, a1, a2, a3
    FieldCommand 1009
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro AddPopularity a0, a1, a2
    FieldCommand 1010
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro StartTalk_1011
    FieldCommand 1011
    .endm

    .macro SaveShopId
    FieldCommand 1012
    .endm

    .macro SetOccupantsLocked a0
    FieldCommand 1013
    .2byte \a0
    .endm

    .macro HideActors a0
    FieldCommand 1014
    .2byte \a0
    .endm

    .macro RaffleShopItem a0, a1, a2
    FieldCommand 1015
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro SetFriendship a0, a1, a2
    FieldCommand 1016
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro RaiseLevel a0, a1, a2
    FieldCommand 1017
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ChangeEffortValue a0, a1, a2, a3
    FieldCommand 1018
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro ShortenEggSteps a0, a1, a2
    FieldCommand 1019
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro SetShopFlag
    FieldCommand 1020
    .endm

    .macro PayAndSetShopFlag a0, a1
    FieldCommand 1021
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetPersonVisit a0, a1, a2, a3, a4
    FieldCommand 1022
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .endm

    .macro NameEntry a0, a1
    FieldCommand 1023
    .2byte \a0
    .2byte \a1
    .endm

    .macro Visit a0, a1, a2, a3
    FieldCommand 1024
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    .macro CheckRank100Unlock a0
    FieldCommand 1025
    .2byte \a0
    .endm

    .macro MarkRaffled
    FieldCommand 1026
    .endm

    .macro AddRecordPerson a0, a1
    FieldCommand 1027
    .2byte \a0
    .2byte \a1
    .endm

    .macro AddEntryOrRecord a0, a1
    FieldCommand 1028
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetNPCValue a0, a1, a2
    FieldCommand 1029
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro SetOwnValue a0, a1
    FieldCommand 1030
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetOwnValue a0, a1
    FieldCommand 1031
    .2byte \a0
    .2byte \a1
    .endm

    .macro SetOccupantWords a0, a1, a2
    FieldCommand 1032
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro SetNPCWord a0, a1, a2
    FieldCommand 1033
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro GetGoodsValue a0, a1, a2
    FieldCommand 1034
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro DescribeOccupant a0, a1, a2
    FieldCommand 1035
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ShowMedalInfo
    FieldCommand 1036
    .endm

    .macro StartSceneAnim a0
    FieldCommand 1037
    .2byte \a0
    .endm

    .macro IsSceneAnimDone a0, a1
    FieldCommand 1038
    .2byte \a0
    .2byte \a1
    .endm

    .macro EndSceneAnim a0
    FieldCommand 1039
    .2byte \a0
    .endm

    // Plugin 8, with overlay 60 loaded

    .macro Open_1000 a0, a1, a2, a3, a4, a5, a6, a7
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

    .macro Open_1011 a0, a1, a2, a3, a4, a5, a6, a7
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

    .macro CreateWalls
    FieldCommand 1000
    .endm

    .macro ChangeRoom a0, a1, a2
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro StartRun
    FieldCommand 1003
    .endm

    .macro GetFloorInArea a0
    FieldCommand 1004
    .2byte \a0
    .endm

    .macro ChangeFloor a0
    FieldCommand 1005
    .2byte \a0
    .endm

    .macro GetArea a0
    FieldCommand 1006
    .2byte \a0
    .endm

    .macro CallBattleEvent
    FieldCommand 1007
    .endm

    // func_ov061_021e5a90
    .macro Plugin9_Cmd1008 a0
    FieldCommand 1008
    .2byte \a0
    .endm

    .macro SetTrainerDefeated a0, a1
    FieldCommand 1009
    .2byte \a0
    .2byte \a1
    .endm

    // func_ov061_021e5bac
    .macro Plugin9_Cmd1010 a0
    FieldCommand 1010
    .2byte \a0
    .endm

    .macro IsTrainerDefeated a0, a1
    FieldCommand 1011
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetNpcScript a0
    FieldCommand 1012
    .2byte \a0
    .endm

    // func_ov061_021e5bc4
    .macro Plugin9_Cmd1013 a0
    FieldCommand 1013
    .2byte \a0
    .endm

    .macro GetSpecialTrainerFlag a0, a1
    FieldCommand 1014
    .2byte \a0
    .2byte \a1
    .endm

    .macro SetSpecialTrainerFlag a0, a1
    FieldCommand 1015
    .2byte \a0
    .2byte \a1
    .endm

    .macro CallDoorEvent a0, a1
    FieldCommand 1016
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetGoalFloor a0
    FieldCommand 1017
    .2byte \a0
    .endm

    .macro GetTrainerClass a0, a1
    FieldCommand 1018
    .2byte \a0
    .2byte \a1
    .endm

    .macro SetProgress a0
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

    .macro GetTrainerId a0, a1
    FieldCommand 1022
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetVersion a0
    FieldCommand 1023
    .2byte \a0
    .endm

    .macro BlackTower_GetInfo a0, a1, a2
    FieldCommand 1024
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro CallMachineEvent
    FieldCommand 1025
    .endm

    .macro IsTargetSlot a0, a1
    FieldCommand 1026
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetNpcScriptByResult a0, a1
    FieldCommand 1027
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetRoomNpcCount a0
    FieldCommand 1028
    .2byte \a0
    .endm

    .macro HasEarnedMoney a0
    FieldCommand 1029
    .2byte \a0
    .endm

    .macro CallLiftEvent
    FieldCommand 1030
    .endm

    .macro TakeEarnedMoney a0
    FieldCommand 1031
    .2byte \a0
    .endm

    // Plugin 10

    .macro Film a0, a1, a2
    FieldCommand 1000
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro WatchSlot a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    .macro PickMovie a0
    FieldCommand 1002
    .2byte \a0
    .endm

    .macro OpenMenu a0, a1
    FieldCommand 1003
    .2byte \a0
    .2byte \a1
    .endm

    .macro SetSaveSlot a0
    FieldCommand 1004
    .2byte \a0
    .endm

    .macro SetMessageWord a0, a1
    FieldCommand 1005
    .byte \a0
    .2byte \a1
    .endm

    .macro GetMovieFlag a0, a1, a2
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro SetMovieFlag a0, a1
    FieldCommand 1007
    .2byte \a0
    .2byte \a1
    .endm

    .macro CountDownloadedMovies a0
    FieldCommand 1008
    .2byte \a0
    .endm

    .macro CheckDownloadedMovie a0, a1
    FieldCommand 1009
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetDownloadedMovieResult a0, a1
    FieldCommand 1010
    .2byte \a0
    .2byte \a1
    .endm

    .macro Pokewood_Create
    FieldCommand 1011
    .endm

    .macro Pokewood_Free
    FieldCommand 1012
    .endm

    .macro SelectCast a0
    FieldCommand 1013
    .2byte \a0
    .endm

    .macro SetUsePlayerCast a0
    FieldCommand 1014
    .2byte \a0
    .endm

    .macro GetNewUnlockKind a0, a1, a2
    FieldCommand 1015
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro TestAndSetLevelBit a0
    FieldCommand 1016
    .2byte \a0
    .endm

    .macro GetSlotFlag a0, a1
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

    .macro SetSlotFlag a0
    FieldCommand 1022
    .2byte \a0
    .endm

    .macro OpenRecordMonitor a0
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

    .macro CountMovieFlags a0, a1
    FieldCommand 1025
    .2byte \a0
    .2byte \a1
    .endm

    .macro WatchBlock
    FieldCommand 1026
    .endm

    // Plugin 11

    .macro PlayCheck a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro PlayLastGate
    FieldCommand 1001
    .endm

    // Plugin 12

    .macro PressSwitch a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro SetLightsState a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    .macro PlayGroupASecond a0
    FieldCommand 1002
    .2byte \a0
    .endm

    .macro PlayGroupBSecond a0
    FieldCommand 1003
    .2byte \a0
    .endm

    .macro EnterPassword a0
    FieldCommand 1004
    .2byte \a0
    .endm

    .macro GetPasswordIndex a0
    FieldCommand 1005
    .2byte \a0
    .endm

    .macro SetSwitchLightState a0, a1
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .endm

    .macro PlaySwitchAnim a0
    FieldCommand 1007
    .2byte \a0
    .endm

    .macro PlaySwitchLightAnim a0
    FieldCommand 1008
    .2byte \a0
    .endm

    // Plugin 13

    .macro Medal a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro FindEarnedMedal a0, a1
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .endm

    .macro GetMatchState a0, a1
    FieldCommand 1005
    .2byte \a0
    .2byte \a1
    .endm

    .macro SetMatchInProgress
    FieldCommand 1006
    .endm

    .macro ClearMatchInProgress
    FieldCommand 1007
    .endm

    .macro SetMatchState a0, a1
    FieldCommand 1008
    .2byte \a0
    .2byte \a1
    .endm

    .macro AddBattlePoints a0
    FieldCommand 1009
    .2byte \a0
    .endm

    // Plugin 14

    .macro StartAspertiaSeasonAnimations
    FieldCommand 1000
    .endm

    .macro ShowGC6Model a0
    FieldCommand 1001
    .2byte \a0
    .endm

    .macro PlayGC6Model a0
    FieldCommand 1002
    .2byte \a0
    .endm

    .macro StartActorHover a0
    FieldCommand 1003
    .2byte \a0
    .endm

    .macro PlayGC6Model23 a0
    FieldCommand 1004
    .2byte \a0
    .endm

    .macro HideGC6Model3
    FieldCommand 1005
    .endm

    .macro ShiftActor a0, a1, a2
    FieldCommand 1006
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro ResetLCDToBlack
    FieldCommand 1007
    .endm

    .macro StartLeagueCenterAnimations
    FieldCommand 1009
    .endm

    .macro FadeOutSeq a0
    FieldCommand 1010
    .2byte \a0
    .endm

    .macro LoadAspertiaBG3Picture
    FieldCommand 1011
    .endm

    .macro CallDT9ReleaseEvent
    FieldCommand 1012
    .endm

    .macro CallDT9ThrowEvent a0, a1, a2, a3, a4, a5
    FieldCommand 1013
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .2byte \a4
    .2byte \a5
    .endm

    .macro PlayDT9Thrown a0, a1, a2
    FieldCommand 1014
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro CallDT9WaitThrownDoneEvent
    FieldCommand 1015
    .endm

    .macro CallDT9WaitFifthFrameEvent
    FieldCommand 1016
    .endm

    // Plugin 15

    .macro ShowSplash a0, a1, a2
    FieldCommand 1000
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .endm

    .macro JumpActorOut a0, a1, a2, a3
    FieldCommand 1001
    .2byte \a0
    .2byte \a1
    .2byte \a2
    .2byte \a3
    .endm

    // Plugin 16

    .macro ShowRuinsModel a0
    FieldCommand 1000
    .2byte \a0
    .endm

    .macro SetRuinsAnimFrame a0
    FieldCommand 1001
    .2byte \a0
    .endm

    .macro PlayRuinsAnim a0
    FieldCommand 1002
    .2byte \a0
    .endm

    .macro CallFormEvent
    FieldCommand 1003
    .endm

    .macro HideActor2
    FieldCommand 1004
    .endm

    .macro StartChase a0
    FieldCommand 1005
    .2byte \a0
    .endm

    .macro WaitChase
    FieldCommand 1006
    .endm

    .macro DropActor
    FieldCommand 1007
    .endm
