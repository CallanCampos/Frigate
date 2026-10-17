#include "asm/field_script.inc"
#include "text/script/pledge_grove.h"

// Script plugin 16, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    PokePartyFindEx 647, 0, 0x8020, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0074
    // "Three sharp marks are deeply\ncut into the rock."
    InfoMsg PledgeGrove_Text_ThreeSharpMarksDeeply, 2
    MsgWaitAdvance
    WordSetPartyPokeName 0, 0x8020
    // "[f000]Ă\u0001\u0000 seems to want to get out of\nthe Poké Ball![f000]븁\u0000\nWill you let it out?"
    InfoMsg PledgeGrove_Text_SeemsWantGetOut, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0065
    InfoMsgClose_0039
    VMCall L_0083
    VMJump L_006E

L_0065:
    // "The Poké Ball has stopped moving..."
    InfoMsg PledgeGrove_Text_PokeBallHasStopped, 2
    LastKeyWait
    InfoMsgClose_0039

L_006E:
    VMJump L_007D

L_0074:
    // "Three sharp marks are deeply\ncut into the rock."
    InfoMsg PledgeGrove_Text_ThreeSharpMarksDeeply, 2
    LastKeyWait
    InfoMsgClose_0039

L_007D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0083:
    ActorCmdExec 255, Movement_0498
    ActorCmdWait
    VMCall L_01A1
    PVPlay 647, 0
    PVWait
    ActorCmdExec 251, Movement_04F0
    ActorCmdWait
    ActorCmdExec 251, Movement_0510
    VMSleep 16
    ActorCmdExec 255, Movement_04A0
    ActorCmdWait
    ActorCmdExec 251, Movement_052C
    VMSleep 16
    ActorCmdExec 255, Movement_0498
    ActorCmdWait
    ActorCmdExec 251, Movement_0538
    ActorCmdExec 255, Movement_04A8
    ActorCmdWait
    ActorCmdExec 251, Movement_055C
    VMSleep 60
    ActorCmdExec 255, Movement_04A0
    ActorCmdWait
    ActorCmdExec 255, Movement_05BC
    VMSleep 8
    ActorCmdExec 251, Movement_0498
    ActorCmdWait
    VMSleep 8
    ActorCmdExec 251, Movement_059C
    ActorCmdWait
    PVPlay 647, 0
    PVWait
    ActorCmdExec 251, Movement_05A8
    ActorCmdWait
    ActorCmdExec 251, Movement_0498
    ActorCmdWait
    PokePartyHasMove 0x8010, 548, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0160
    VMCall L_0227
    VMJump L_0199

L_0160:
    // "The determination carved into the rock\nby Cobalion, Virizion, and Terrakion[f000]븀\u0000\nreminded [f000]Ă\u0001\u0000 of Secret Sword!"
    SystemMsg PledgeGrove_Text_DeterminationCarvedIntoRock, 2
    MsgWaitAdvance
    WorkGet 0x8000, 0x8020
    WorkSetConst 0x8001, 548
    VMCall L_02C5
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0199
    VMCall L_0227
    VMJump L_0199

L_0199:
    VMCall L_01E9
    VMReturn

L_01A1:
    Cmd_020E 0, 15, 3, 15, 3, 8
    Cmd_020F 0, 15, 3, 15
    Cmd_0211 0
    FadeEx 12, 0, 16, 2
    FadeExWait
    ActorNew 15, 15, 1, 251, 122, 0
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 0
    VMReturn

L_01E9:
    Cmd_020F 1, 15, 4, 13
    Cmd_0211 1
    FadeEx 12, 0, 16, 2
    FadeExWait
    ActorDelete 251
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 1
    Cmd_020E 1, 15, 0, 15, 3, 8
    VMReturn

L_0227:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 7768, 0, 0xed000, 0xf8000, 45056, 0xbb000, 40
    EvCameraWait
    // "What?\n[f000]Ă\u0001\u0000 is...[f000]븁\u0000"
    InfoMsg PledgeGrove_Text_What, 2
    InfoMsgClose_0039
    Plugin16_Cmd1003
    FadeEx 12, 0, 16, 4
    FadeExWait
    Plugin16_Cmd1004
    ActorDelete 251
    ActorNew 15, 13, 1, 251, 373, 0
    PlayFieldEffect 115
    PVPlay 647, 1
    FadeEx 12, 16, 0, 4
    FadeExWait
    PVWait
    PokePartySetForme 0x8020, 1
    // "[f000]Ă\u0001\u0000 has resolved to battle\nand has changed into Resolute Form!"
    InfoMsg PledgeGrove_Text_HasResolvedBattleHas, 2
    LastKeyWait
    InfoMsgClose_0039
    EvCameraReturn 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

L_02C5:
    WorkGet 0x8021, 0x8000
    WorkGet 0x8022, 0x8001
    PokePartyGetMoveCount 0x8010, 0x8021
    VMStackPush 0x8010
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_030F
    WordSetPartyPokeName 0, 0x8021
    // "[f000]Ă\u0001\u0000 remembered the move\nSecret Sword!"
    SystemMsg PledgeGrove_Text_RememberedMoveSecretSword, 0
    MEPlay SEQ_ME_LVUP
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    PokePartyLearnMove 0x8021, 0x8010, 0x8022
    WorkSetConst 0x8010, 1
    VMReturn

L_030F:
    WorkSetConst 0x8025, 1

L_0315:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_037E
    VMCall L_03BA
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0353
    WorkSetConst 0x8010, 1
    WorkSetConst 0x8025, 0
    VMJump L_0378

L_0353:
    VMCall L_0380
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0378
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8025, 0

L_0378:
    VMJump L_0315

L_037E:
    VMReturn

L_0380:
    // "Give up on remembering the\nmove Secret Sword?"
    SystemMsg PledgeGrove_Text_GiveUpRememberingMove, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B2
    WordSetPartyPokeName 0, 0x8021
    // "[f000]Ă\u0001\u0000 did not remember the\nmove Secret Sword.[f000]븁\u0000"
    SystemMsg PledgeGrove_Text_DidNotRememberMove, 0
    InfoMsgClose
    WorkSetConst 0x8026, 1
    VMReturn

L_03B2:
    WorkSetConst 0x8026, 0
    VMReturn

L_03BA:
    WorkSetConst 0x8026, 0
    WordSetPartyPokeName 0, 0x8021
    // "But [f000]Ă\u0001\u0000 can't know more\nthan four moves.[f000]븁\u0000\nDelete an existing move to make\nroom for Secret Sword?[f000]븁\u0000"
    SystemMsg PledgeGrove_Text_ButCantKnowMore, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E4
    VMReturn

L_03E4:
    InfoMsgClose
    CallPokeMoveReplace 0x8010, 0x8023, 0x8021, 0x8022
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0405
    VMReturn

L_0405:
    PokePartyGetMove 0x8024, 0x8021, 0x8023
    WordSetMoveName 1, 0x8024
    // "Is it OK to forget the\nmove [f000]ć\u0001\u0001?"
    SystemMsg PledgeGrove_Text_OkForgetMove, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0431
    VMReturn

L_0431:
    PokePartyGetMove 0x8024, 0x8021, 0x8023
    WordSetPartyPokeName 0, 0x8021
    WordSetMoveName 1, 0x8024
    // "1, [f000]븂\u0001\u00142, and[f000]븂\u0001\u0014... [f000]븂\u0001\u0014... [f000]븂\u0001\u0014... Ta-da![f000]븅\u0001\u0003[f000]븆\u0001\u0002[f000]븇\u0000[f000]븄\u0000[f000]븁\u0000\n[f000]Ă\u0001\u0000 forgot how to\nuse [f000]ć\u0001\u0001.[f000]븁\u0000\nAnd...[f000]븁\u0000"
    SystemMsg PledgeGrove_Text_12TaDa, 0
    // "[f000]Ă\u0001\u0000 remembered the move\nSecret Sword!"
    SystemMsg PledgeGrove_Text_RememberedMoveSecretSword, 0
    MEPlay SEQ_ME_LVUP
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    PokePartyLearnMove 0x8021, 0x8023, 0x8022
    WorkSetConst 0x8026, 1
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x26
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x25
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x24
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x23
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x22
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x21
    .byte 0x80
    .balign 4, 0
    Move 0, 0
    Move 32, 1
    MoveEnd

Movement_0498:
    Move 33, 1
    MoveEnd

Movement_04A0:
    Move 34, 1
    MoveEnd

Movement_04A8:
    Move 35, 1
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

Movement_04F0:
    Move 34, 1
    Move 63, 1
    Move 35, 1
    Move 63, 1
    Move 32, 1
    Move 63, 1
    Move 75, 1
    MoveEnd

Movement_0510:
    Move 18, 3
    Move 16, 3
    Move 63, 2
    Move 35, 1
    Move 51, 1
    Move 63, 2
    MoveEnd

Movement_052C:
    Move 17, 2
    Move 19, 6
    MoveEnd

Movement_0538:
    Move 16, 2
    Move 63, 2
    Move 34, 1
    Move 63, 1
    Move 35, 1
    Move 63, 1
    Move 32, 1
    Move 63, 2
    MoveEnd

Movement_055C:
    Move 17, 1
    Move 18, 2
    Move 32, 1
    Move 63, 2
    Move 34, 1
    Move 63, 1
    Move 17, 1
    Move 18, 2
    Move 16, 1
    Move 35, 1
    Move 63, 1
    Move 32, 1
    Move 63, 1
    Move 35, 1
    Move 63, 3
    MoveEnd

Movement_059C:
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_05A8:
    Move 63, 3
    Move 161, 1
    Move 63, 5
    Move 75, 1
    MoveEnd

Movement_05BC:
    Move 13, 2
    Move 32, 1
    MoveEnd
