#include "asm/field_script.inc"
#include "text/script/dragonspiral_tower_9.h"

// Script plugin 14, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    GameGetVersion 0x8020
    VMStackPushFlag EVENT_FLAG_0x029b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005C
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0056
    WorkSetConst EVENT_WORK_0x4020, 365
    VMJump L_005C

L_0056:
    WorkSetConst EVENT_WORK_0x4020, 364

L_005C:
    VMHalt

Script_2:
    ActorsPauseAll
    GameGetVersion 0x8020
    SEPlay SEQ_SE_END_02
    EvCameraShake 4, 0, 4, 5, 1, 0, 1, 3
    SEWait
    VMSleep 30
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0160
    // "The Dark Stone...\nIt's trembling inside the Bag![f000]븁\u0000\nTake the Dark Stone\nout of the Bag?"
    SystemMsg DragonspiralTower9_Text_DarkStoneItsTrembling, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013D
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 16
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00E5
    ActorWalkRoute 255, 16, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_0670
    ActorCmdWait

L_00E5:
    ItemSub ITEM_DARK_STONE, 1, 0x8010
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x108000, 0, 0xa8000, 30
    EvCameraWait
    VMSleep 8
    Plugin14_Cmd1012
    BGMPlayEx 1002, 6
    FlagReset EVENT_FLAG_0x029b
    WorkSetConst EVENT_WORK_0x411b, 2
    WorkSetConst EVENT_WORK_0x4020, 365
    // "The Dark Stone draws in the aura of\nthe surroundings and converts it into[f000]븀\u0000\na powerful force, which is...now...[f000]븁\u0000\nBeing released!"
    SystemMsg DragonspiralTower9_Text_DarkStoneDrawsAura, 2
    MsgWaitAdvance
    MsgWinCloseAll
    VMCall L_022D
    VMJump L_015A

L_013D:
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0698
    ActorCmdWait
    VMSleep 8
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 was pushed back\nby a mysterious force..."
    SystemMsg DragonspiralTower9_Text_PushedBackByMysterious, 2
    LastKeyWait
    MsgWinCloseAll

L_015A:
    VMJump L_0227

L_0160:
    // "The Light Stone...\nIt's trembling inside the Bag![f000]븁\u0000\nTake the Light Stone\nout of the Bag?"
    SystemMsg DragonspiralTower9_Text_LightStoneItsTrembling, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020A
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 16
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01B2
    ActorWalkRoute 255, 16, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_0670
    ActorCmdWait

L_01B2:
    ItemSub ITEM_LIGHT_STONE, 1, 0x8010
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x108000, 0, 0xa8000, 30
    EvCameraWait
    VMSleep 8
    Plugin14_Cmd1012
    BGMPlayEx 1002, 6
    FlagReset EVENT_FLAG_0x029b
    WorkSetConst EVENT_WORK_0x411b, 2
    WorkSetConst EVENT_WORK_0x4020, 364
    // "The Light Stone draws in the aura of\nthe surroundings and converts it into[f000]븀\u0000\na powerful force, which is...now...[f000]븁\u0000\nBeing released!"
    SystemMsg DragonspiralTower9_Text_LightStoneDrawsAura, 2
    MsgWaitAdvance
    MsgWinCloseAll
    VMCall L_022D
    VMJump L_0227

L_020A:
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0698
    ActorCmdWait
    VMSleep 8
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 was pushed back\nby a mysterious force..."
    SystemMsg DragonspiralTower9_Text_PushedBackByMysterious_2, 2
    LastKeyWait
    MsgWinCloseAll

L_0227:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_022D:
    FadeOutBlackQ
    FadeWait
    GameGetVersion 0x8020
    DebugPrint 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025C
    FieldClose
    Call3DDemo 9, 0
    FieldOpen
    VMJump L_0266

L_025C:
    FieldClose
    Call3DDemo 8, 0
    FieldOpen

L_0266:
    ActorAdd 0
    FadeInBlackQ
    FadeWait
    BGMChangeMapEx 6
    EvCameraRebind
    EvCameraEnd
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8023, 0
    PlayerGetDir 0x8023
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_029B
    VMJump L_02A9

L_029B:
    ActorCmdExec 255, Movement_0648
    VMJump L_02EB

L_02A9:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_02BC
    VMJump L_02CA

L_02BC:
    ActorCmdExec 255, Movement_0628
    VMJump L_02EB

L_02CA:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_02DD
    VMJump L_02EB

L_02DD:
    ActorCmdExec 255, Movement_0638
    VMJump L_02EB

L_02EB:
    ActorCmdWait
    WorkSetConst 0x8023, 0
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xe5000, 0x108000, 0, 0xb9000, 6
    EvCameraWait
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0336
    ActorCmdExec 0, Movement_0620
    VMJump L_033E

L_0336:
    ActorCmdExec 0, Movement_0618

L_033E:
    ActorCmdWait
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 1
    WorkOr 0x8024, 16
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0388
    PVPlay 644, 0
    // "Bazzazzazzash!"
    ScreamMsg DragonspiralTower9_Text_Bazzazzazzash, 0
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 644, 70, 0x8024
    VMJump L_03A1

L_0388:
    PVPlay 643, 0
    // "Preeeeaah!"
    ScreamMsg DragonspiralTower9_Text_Preeeeaah, 0
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 643, 70, 0x8024

L_03A1:
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D6
    ActorDelete 0
    FlagSet EVENT_FLAG_0x029b
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    CallWildBattleEnd
    VMJump L_03D8

L_03D6:
    CallWildLose

L_03D8:
    WorkSetConst 0x8024, 0
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_03F5
    VMJump L_03FF

L_03F5:
    FlagSet EVENT_FLAG_0x00ea
    VMJump L_0456

L_03FF:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_041F
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_041F
    VMJump L_0456

L_041F:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0446
    // "Zekrom went flying off somewhere..."
    SystemMsg DragonspiralTower9_Text_ZekromWentFlyingOff, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0450

L_0446:
    // "Reshiram went flying off somewhere..."
    SystemMsg DragonspiralTower9_Text_ReshiramWentFlyingOff, 2
    LastKeyWait
    InfoMsgClose

L_0450:
    VMJump L_0456

L_0456:
    VMStackPushFlag EVENT_FLAG_0x01e0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046F
    VMCall L_0475

L_046F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0475:
    ActorNew 16, 20, 0, 251, 103, 0
    ActorWalkRoute 251, 16, 14, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0680
    ActorCmdWait
    GameGetVersion 0x8020
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_04B4
    VMJump L_04EB

L_04B4:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04D9
    // "N: [f000]븉\u0001\u0001Zekrom has recognized\nyou as the new hero.[f000]븁\u0000\nThat's right! As a Trainer with a will\nstrong enough to change the world![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_NZekromHasRecognized, 251, 0, 0
    VMJump L_04E5

L_04D9:
    // "N: [f000]븉\u0001\u0001Reshiram has recognized\nyou as the new hero.[f000]븁\u0000\nThat's right! As a Trainer with a will\nstrong enough to change the world![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_NReshiramHasRecognized, 251, 0, 0

L_04E5:
    VMJump L_057F

L_04EB:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_04FE
    VMJump L_0535

L_04FE:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0523
    // "N: [f000]븉\u0001\u0001You showed Zekrom\nthe strength of your Pokémon.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_NShowedZekromStrength, 251, 0, 0
    VMJump L_052F

L_0523:
    // "N: [f000]븉\u0001\u0001You showed Reshiram\nthe strength of your Pokémon.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_NShowedReshiramStrength, 251, 0, 0

L_052F:
    VMJump L_057F

L_0535:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0548
    VMJump L_057F

L_0548:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_056D
    // "N: [f000]븉\u0001\u0001You're walking away from\na battle with Zekrom?[f000]븀\u0000\nWhat an amusing Trainer.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_NYoureWalkingAway, 251, 0, 0
    VMJump L_0579

L_056D:
    // "N: [f000]븉\u0001\u0001You're walking away from\na battle with Reshiram?[f000]븀\u0000\nWhat an amusing Trainer.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_NYoureWalkingAway_2, 251, 0, 0

L_0579:
    VMJump L_057F

L_057F:
    // "[f000]븉\u0001\u0001I have something to tell you.[f000]븁\u0000\nGo to the Giant Chasm!\nKyurem has returned.[f000]븁\u0000\nThat's what my friend\nhas told me.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_HaveSomethingTellGo, 251, 0, 0
    MsgWinCloseAll
    VMSleep 20
    WordSetPlayerName 0
    // "[f000]븉\u0001\u0001[f000]Ā\u0001\u0000![f000]븁\u0000\nThe world that you desire\nfor Pokémon and humans...[f000]븁\u0000\nI look forward to seeing how\nfull of love that world is.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_WorldDesirePokemonHumans, 251, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x108000, 0, 0x118000, 32
    ActorCmdExec 251, Movement_06AC
    EvCameraWait
    ActorCmdWait
    ActorCmdExec 251, Movement_0690
    ActorCmdWait
    // "[f000]븉\u0001\u0001Someday...[f000]븁\u0000\nPokémon and humans will be bound\ntogether without Poké Balls.[f000]븁\u0000\nThey will simply trust\nand help one another.[f000]븀\u0000\nMake that kind of world.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower9_Text_SomedayPokemonHumansWill, 251, 0, 0
    MsgWinCloseAll
    EvCameraReturn 40
    ActorWalkRoute 251, 16, 23, 0, 8, 1
    EvCameraWait
    ActorCmdWait
    ActorDelete 251
    FlagSet EVENT_FLAG_0x01e0
    FlagSet EVENT_FLAG_0x0390
    FlagReset EVENT_FLAG_0x03ec
    EvCameraRebind
    EvCameraEnd
    VMReturn
    .balign 4, 0
    Move 172, 1
    MoveEnd

Movement_0618:
    Move 191, 1
    MoveEnd

Movement_0620:
    Move 191, 1
    MoveEnd

Movement_0628:
    Move 13, 1
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_0638:
    Move 13, 1
    Move 14, 1
    Move 32, 1
    MoveEnd

Movement_0648:
    Move 14, 1
    Move 13, 2
    Move 15, 1
    Move 32, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 0, 1
    Move 75, 1
    MoveEnd

Movement_0670:
    Move 32, 1
    MoveEnd
    Move 1, 1
    MoveEnd

Movement_0680:
    Move 29, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0690:
    Move 34, 1
    MoveEnd

Movement_0698:
    Move 36, 2
    Move 71, 1
    Move 17, 1
    Move 72, 1
    MoveEnd

Movement_06AC:
    Move 13, 2
    Move 9, 1
    Move 63, 3
    Move 182, 1
    MoveEnd
