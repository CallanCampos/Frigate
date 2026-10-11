#include "asm/field_script.inc"
#include "text/script/plasma_frigate_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

L_002A:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    ObjInitWarpGPos 1, 0, 0, 0
    VMJump L_005B

L_0051:
    ObjInitWarpGPos 0, 0, 0, 0

L_005B:
    VMReturn

Script_1:
    VMCall L_002A
    VMHalt

Script_2:
    VMHalt

Script_3:
    VMCall L_002A
    VMHalt

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    FlagReset EVENT_FLAG_0x0346
    FlagReset EVENT_FLAG_0x0347
    PlayerGetGPos 0x8022, 0x8023
    BGMPlayPush SEQ_BGM_E_7_SAGE
    ActorCmdExec 4, Movement_0488
    ActorCmdWait
    // "Zinzolin: You're an impressive\nTrainer to have made it this far.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ZinzolinYoureImpressiveTrainer, 1, 3, 0
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C2
    ActorCmdExec 255, Movement_039C
    VMJump L_00DD

L_00C2:
    VMStackPush 0x8022
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DD
    ActorCmdExec 255, Movement_03A8

L_00DD:
    ActorCmdWait
    ActorCmdExec 1, Movement_0488
    ActorCmdWait
    // "Since you went to such trouble\nto come here, I'll show you something.[f000]븁\u0000\nThis is...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_SinceWentSuchTrouble, 1, 3, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_03C0
    ActorCmdWait
    // "The legendary Pokémon of ice!\nIts name is Kyurem![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_LegendaryPokemonIceIts, 1, 3, 1
    ActorMsgClose
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0x9d000, 0xc8000, 0, 0xfc000, 24
    EvCameraWait
    VMSleep 12
    PVPlay 646, 0
    // "Haaahraa..."
    ScreamMsg PlasmaFrigate5_Text_Haaahraa, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    EvCameraMoveToDefault 32
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    // "The ice missiles we fired\ninto Opelucid City were[f000]븀\u0000\ncreated with Kyurem's power[f000]븀\u0000\nand Team Plasma's technology![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_IceMissilesWeFired, 1, 3, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0488
    ActorCmdWait
    // "Well...[f000]븁\u0000\nYou could become a\nthreat to Team Plasma,[f000]븀\u0000\nso we will eliminate you here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_WellCouldBecomeThreat, 1, 3, 0
    MsgWinCloseAll
    VMSleep 12
    ActorNew 12, 20, 0, 251, 291, 0
    ActorCmdExec 251, Movement_03F8
    ActorCmdWait
    // "Not with me around, you won't![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_NotAroundWont, 251, 6, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0490
    VMSleep 4
    ActorCmdExec 255, Movement_0498
    ActorCmdWait
    // "Thanks for removing the barrier!\nThat was a big help![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ThanksRemovingBarrierBig, 251, 6, 0
    MsgWinCloseAll
    // "Zinzolin: Hmph!\nWe'll simply eliminate both of you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ZinzolinHmphWellSimply, 1, 3, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0480
    ActorCmdExec 255, Movement_0480
    ActorCmdExec 4, Movement_03E0
    ActorCmdWait
    ActorCmdExec 255, Movement_03A8
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0215
    CallTrainerMultiBattle 794, 797, 347, 0
    VMJump L_0242

L_0215:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0238
    CallTrainerMultiBattle 795, 797, 347, 0
    VMJump L_0242

L_0238:
    CallTrainerMultiBattle 796, 797, 347, 0

L_0242:
    VMCall L_0358
    // "[f000]Ā\u0001\u0001: I'm not going\nto lose to Team Plasma![f000]븁\u0000\nBy the way, what is this place?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ImNotGoingLose, 251, 6, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 251, Movement_04A0
    ActorCmdWait
    // "That Pokémon...[f000]븁\u0000\nIt's so icy...[f000]븁\u0000\nCould that be the source of\nthe attack on Opelucid City?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_PokemonItsIcyCould, 251, 6, 0
    MsgWinCloseAll
    // "Zinzolin: Hmph. You're a smarter\nTrainer than I expected.[f000]븁\u0000\nIf you've got that much sense,\nwhy did you do something as[f000]븀\u0000\ndangerous as sneaking into our base?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ZinzolinHmphYoureSmarter, 1, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: That should be obvious![f000]븁\u0000\nI'll do whatever it takes to get\nmy sister's Pokémon back![f000]븁\u0000\nAre YOU the one who stole a\nPurrloin in Aspertia five years ago?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ShouldObviousIllWhatever, 251, 6, 0
    MsgWinCloseAll
    // "Zinzolin: If it's just a Purrloin,\nsomeone probably stole it[f000]븀\u0000\nand is using it.[f000]븁\u0000\nWhy can't you understand?[f000]븁\u0000\nThere are other Purrloin.\nWhy are you so fixated on this one?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ZinzolinIfItsJust, 1, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: That's the ONLY Purrloin\nin the world that my late grandpa[f000]븀\u0000\ncaught for my little sister![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ThatsOnlyPurrloinWorld, 251, 6, 0
    MsgWinCloseAll
    // "Zinzolin: An individual's feelings...[f000]븁\u0000\nTo you, that's probably a\nmatter of great importance.[f000]븁\u0000\nBut from the perspective of other\npeople, it is a trifling matter indeed.[f000]븁\u0000\nCompare those feelings against\nthe majesty of this ship![f000]븁\u0000\nThis ship itself is a device that\nuses the Pokémon Kyurem's power![f000]븁\u0000\nWith this ship, this time we will\nconquer Unova![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_ZinzolinIndividualsFeelingsThats, 1, 3, 0
    // "It looks like Kyurem\nhas fully recovered.[f000]븁\u0000\nWe'll put the DNA Splicers\nto good use.[f000]븁\u0000\nI'll let you take care of them,\nShadow Triad![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_LooksLikeKyuremHas, 1, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Don't mess with me!\nYou were the one who lost![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate5_Text_DontMessWereOne, 251, 6, 0
    MsgWinCloseAll
    ActorAdd 2
    ActorAdd 3
    ActorCmdExec 2, Movement_042C
    ActorCmdExec 3, Movement_042C
    VMSleep 8
    ActorCmdExec 255, Movement_0490
    ActorCmdExec 251, Movement_0498
    ActorCmdWait
    FlagSet EVENT_FLAG_0x0346
    FlagSet EVENT_FLAG_0x0347
    WorkSetConst EVENT_WORK_0x4100, 2
    FlagReset EVENT_FLAG_0x0357
    WorkSetConst EVENT_WORK_0x4106, 4
    WorkSetConst EVENT_WORK_PLASMA_FRIGATE_LOCATION, 2
    MapReplaceSetEvent 5, 0, 0
    MapReplaceSetEvent 6, 0, 0
    FadeOutBlackQ
    BGMFadeOut 6
    FadeWait
    FieldClose
    Call3DDemo 13, 0
    FieldOpen
    RTReserveScript 5
    MapChangeCore ZONE_ROUTE_21, 798, 65531, 242, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0358:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0392
    PokePartyGetCount 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_038A
    PokePartyRecoverAll

L_038A:
    CallTrainerBattleEnd
    VMJump L_0398

L_0392:
    FlagSet EVENT_FLAG_0x0347
    CallTrainerLose

L_0398:
    VMReturn
    .balign 4, 0

Movement_039C:
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_03A8:
    Move 14, 1
    Move 32, 1
    MoveEnd
    Move 14, 1
    Move 32, 1
    MoveEnd

Movement_03C0:
    Move 3, 1
    Move 71, 1
    Move 10, 1
    Move 72, 1
    MoveEnd
    Move 11, 1
    Move 33, 1
    MoveEnd

Movement_03E0:
    Move 14, 3
    Move 33, 1
    MoveEnd
    MsgPlaceSignClose
    VMNop2
    VMStackAdd
    VMHalt
    .byte 0xfe
    .balign 4, 0

Movement_03F8:
    Move 68, 1
    Move 19, 1
    Move 16, 2
    MoveEnd
    WorkOr 1, 16
    VMHalt
    WorkOr 1, 16
    VMHalt
    Move 34, 1
    MoveEnd
    Move 11, 1
    Move 32, 1
    MoveEnd

Movement_042C:
    Move 184, 1
    MoveEnd
    Move 185, 1
    Move 69, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_0480:
    Move 32, 1
    MoveEnd

Movement_0488:
    Move 33, 1
    MoveEnd

Movement_0490:
    Move 34, 1
    MoveEnd

Movement_0498:
    Move 35, 1
    MoveEnd

Movement_04A0:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 100, 1
    MoveEnd
