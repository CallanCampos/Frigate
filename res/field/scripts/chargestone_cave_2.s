#include "asm/field_script.inc"
#include "text/script/chargestone_cave_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPush EVENT_WORK_0x40d2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0070
    VMStackPushFlag EVENT_FLAG_0x0314
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006A
    FlagSet EVENT_FLAG_0x0314

L_006A:
    VMJump L_0089

L_0070:
    VMStackPush EVENT_WORK_0x40d2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0089
    HollowRivalCmd_0262 3, 2

L_0089:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40d2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AA
    ActorSetGPos 13, 11, 0, 48, 3

L_00AA:
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 13, Movement_0270
    ActorCmdWait
    // "Heeey![f000]븁\u0000"
    // "Hi there![f000]븁\u0000"
    ActorMsgGendered 1024, ChargestoneCave2_Text_Heeey, ChargestoneCave2_Text_HiThere, 13, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FD
    WorkSub 0x8022, 1
    ActorWalkRoute 13, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    VMJump L_011D

L_00FD:
    WorkSub 0x8022, 1
    ActorWalkRoute 13, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 13, Movement_024C
    ActorCmdWait

L_011D:
    WordSetPlayerName 0
    // "Bianca: Did you know this?[f000]븁\u0000\nIf you push the floating stones,\nthey move![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_BiancaDidKnowIf, 13, 0, 0
    MsgWinCloseAll
    ActorCmdExec 13, Movement_0254
    ActorCmdWait
    VMSleep 8
    ActorCmdExec 13, Movement_024C
    ActorCmdWait
    // "As always, this place is charged with\nlots of electricity that Pokémon like![f000]븁\u0000\nThe electric charges react from one\nstone to another, so that's why[f000]븀\u0000\nthere are floating stones![f000]븁\u0000\nYou can't push all of them, though.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_AlwaysPlaceChargedLots, 13, 0, 0
    MsgWinCloseAll
    ActorCmdExec 13, Movement_0268
    ActorCmdWait
    // "Oh, that's right!\nI came here to research something![f000]븁\u0000\nBe seeing you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_OhThatsRightCame, 13, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 13, 14, 45, 1, 8, 1
    ActorCmdWait
    ActorWalkRoute 13, 15, 40, 1, 8, 0
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40d2, 1
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B3
    ActorDelete 13
    FlagSet EVENT_FLAG_0x0314
    VMJump L_01BF

L_01B3:
    ActorSetGPos 13, 37, 0, 37, 3

L_01BF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    // "[f000]븉\u0001\u0001Chargestone Cave--\nI really like it here.[f000]븁\u0000\nFormulas express the\nforces behind electricity,[f000]븀\u0000\nits connection to Pokémon,[f000]븀\u0000\nand humans and Pokémon themselves.[f000]븁\u0000\nThis--this is my ideal place.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_ChargestoneCaveReallyLike, 11, 0, 0
    // "[f000]븉\u0001\u0001I have to go...[f000]븁\u0000\nI have to go in order to save\nPokémon and protect the very[f000]븀\u0000\nfriend that I have to stop![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_HaveGoHaveGo, 11, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 11
    SEWait
    FlagSet EVENT_FLAG_0x0315
    WorkSetConst EVENT_WORK_0x40d3, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca: The bridge fell apart,\nbut it's being fixed right now!"
    ParentActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_BiancaBridgeFellApart, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What could have happened?\nMaybe wild Pokémon ran into it.[f000]븁\u0000\nAt any rate, it's going to take some\ntime to fix. Go wait around Driftveil!"
    ParentActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_WhatCouldHaveHappened, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_024C:
    Move 33, 1
    MoveEnd

Movement_0254:
    Move 61, 1
    Move 35, 1
    Move 34, 1
    Move 61, 1
    MoveEnd

Movement_0268:
    Move 75, 1
    MoveEnd

Movement_0270:
    Move 33, 1
    Move 75, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0990
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02FC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What beautiful stones![f000]븁\u0000\nWouldn't it be lovely if I could\nhave such pretty gems on the[f000]븀\u0000\nwalls of my room?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_WhatBeautifulStonesWouldnt, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_FLD_133
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D1
    Cmd_0275 0, 18, 0
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg ChargestoneCave2_Text_FunfestMissionHasBeen_2, 0
    VMJump L_02DE

L_02D1:
    Cmd_0275 0, 17, 0
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg ChargestoneCave2_Text_FunfestMissionHasBeen, 0

L_02DE:
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "I'll live here![f000]븁\u0000\nFrom today on, my home will be here,\namong the beautiful stones!"
    ParentActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_IllLiveHereFrom, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0990
    VMJump L_0310

L_02FC:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'll live here![f000]븁\u0000\nFrom today on, my home will be here,\namong the beautiful stones!"
    ParentActorMsg MSGFILE_SCRIPT, ChargestoneCave2_Text_IllLiveHereFrom, 0, 0
    LastKeyWait
    ActorMsgClose

L_0310:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
