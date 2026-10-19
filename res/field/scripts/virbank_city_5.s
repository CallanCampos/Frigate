#include "asm/field_script.inc"
#include "text/script/virbank_city_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_5:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The ship to Castelia City\nleaves from here![f000]븁\u0000\nIt can even cross seas that are too\nrough for Pokémon to get through!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_ShipCasteliaCityLeaves, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x02d3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006B
    // "If you want the captain,\nhe's in Pokéstar Studios![f000]븁\u0000\nHis daughter, Roxie, is both a\nGym Leader and a band leader, you know.[f000]븁\u0000\nNot wanting to be outdone, he said he\nneeds to be a captain and an actor!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_IfWantCaptainHes, 0, 0
    VMJump L_0075

L_006B:
    // "Your life is your own.[f000]븁\u0000\nYou're free to work hard at one thing\nor try many different things!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_LifeOwnYoureFree, 0, 0

L_0075:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What can I do for you?\nShall we set sail for Castelia City?"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_WhatCanShallWe, 2, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E0
    // "Of course!\nPlease, step this way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_CoursePleaseStepWay, 2, 2, 0
    MsgWinCloseAll
    FadeOutBlackQ
    BGMFadeOut 30
    FadeWait
    FieldClose
    Call3DDemo 26, 0
    FieldOpen
    RTReserveScript 3
    MapChangeCore ZONE_CASTELIA_CITY_10, 12, 0, 10, 2
    VMJump L_00F0

L_00E0:
    // "OK then! Please come talk to me\nwhenever you'd like to board!"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_OkThenPleaseCome, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_00F0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: You're going\nto Castelia City, right?"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_YoureGoingCasteliaCity, 3, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C6
    // "I'm going to look for Team Plasma!\nI can't forgive those guys![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_ImGoingLookTeam, 3, 2, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_01FC
    ActorCmdWait
    // "Hey, captain!\nShow us that ship you're so proud of![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_HeyCaptainShowUs, 3, 2, 0
    MsgWinCloseAll
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_016E
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_016E
    VMJump L_017E

L_016E:
    ActorCmdExec 255, Movement_01FC
    ActorCmdWait
    VMJump L_017E

L_017E:
    // "Of course!\nPlease, step this way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_CoursePleaseStepWay, 2, 2, 0
    MsgWinCloseAll
    FadeOutBlackQ
    BGMFadeOut 30
    FadeWait
    FieldClose
    Call3DDemo 23, 0
    FieldOpen
    ActorDelete 3
    MapReplaceSetEvent 3, 1, 1
    MapChangeCore ZONE_CASTELIA_CITY_10, 12, 0, 10, 2
    FlagSet EVENT_FLAG_0x02d2
    WorkSetConst EVENT_WORK_0x40ae, 1
    VMJump L_01D6

L_01C6:
    // "Really?[f000]븁\u0000\nSomething you still need to take care of?\nGo deal with it, then.[f000]븁\u0000\nYou know that even after we go to\nCastelia City, you can come[f000]븀\u0000\nback to Virbank anytime, right?"
    ActorMsg MSGFILE_SCRIPT, VirbankCity5_Text_ReallySomethingStillNeed, 3, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_01D6:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
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

Movement_01FC:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
