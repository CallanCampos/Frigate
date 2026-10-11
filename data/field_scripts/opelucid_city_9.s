#include "asm/field_script.inc"
#include "text/script/opelucid_city_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0152
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0142
    // "I love Tympole more than anyone!\nI'm a Tympole fanatic![f000]븁\u0000\nIf I just had one more Tympole,\nthey would sing together![f000]븁\u0000\nIf you have a Tympole with you,\nwould you show it to me?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_LoveTympoleMoreThan, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012E
    // "Thanks!\nCould you show me a Tympole, then?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_ThanksCouldShowTympole, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011A
    PokePartyGetSpecies 0x8022, 0x8020
    PokePartyIsEgg 0x8023, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 535
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0106
    // "I'm Tympole-xcited!!![f000]븁\u0000\nFinally!\nI have six Tympole in one place![f000]븁\u0000\nOK, Tympole!\nShow us those cute,[f000]븀\u0000\nlovely voices of yours!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_ImTympoleXcitedFinally, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMCall L_0225
    // "Aaaah! That was a wonderful choir![f000]븁\u0000\nMy love for Tympole\njust gets deeper and deeper![f000]븁\u0000\nIf it wasn't for you,\nI never would've heard that song![f000]븀\u0000\nReally, seriously, thanks![f000]븁\u0000\nIf you want to hear the Tympole's\nTympole song again sometime,[f000]븀\u0000\nbring a Tympole back to me, OK?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_AaaahWonderfulChoirLove, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0152
    VMJump L_0114

L_0106:
    // "That's not a Tympole, is it?\nToo bad!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_ThatsNotTympoleToo, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0114:
    VMJump L_0128

L_011A:
    // "OK... Can't do anything about that![f000]븁\u0000\nIf you catch a Tympole,\nshow it to me![f000]븀\u0000\nI'll let you hear a great song!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_OkCantAnythingAbout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0128:
    VMJump L_013C

L_012E:
    // "OK... Can't do anything about that![f000]븁\u0000\nIf you catch a Tympole,\nshow it to me![f000]븀\u0000\nI'll let you hear a great song!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_OkCantAnythingAbout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_013C:
    VMJump L_021F

L_0142:
    // "Hmm? Would you like to hear\nthe cute, lovely voices of Tympole?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_HmmWouldLikeHear, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0211
    // "Thanks!\nCould you show me a Tympole, then?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_ThanksCouldShowTympole, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FD
    PokePartyGetSpecies 0x8022, 0x8020
    PokePartyIsEgg 0x8023, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 535
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01E9
    // "Come on, Tympole!\nLet's hear that harmony!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_ComeTympoleLetsHear, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMCall L_0225
    // "Oh my! Aren't Tympole just the cutest?[f000]븁\u0000\nNo matter how many times I hear\nthat song, it gets me right here![f000]븁\u0000\nMy love for Tympole gets\ndeeper and deeper!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_OhArentTympoleJust, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0152
    VMJump L_01F7

L_01E9:
    // "That's not a Tympole, is it?\nToo bad!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_ThatsNotTympoleToo, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01F7:
    VMJump L_020B

L_01FD:
    // "OK... Can't do anything about that![f000]븁\u0000\nIf you catch a Tympole,\nshow it to me![f000]븀\u0000\nI'll let you hear a great song!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_OkCantAnythingAbout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_020B:
    VMJump L_021F

L_0211:
    // "OK... Can't do anything about that![f000]븁\u0000\nIf you catch a Tympole,\nshow it to me![f000]븀\u0000\nI'll let you hear a great song!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_OkCantAnythingAbout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_021F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0225:
    PlayerGetGPos 0x8025, 0x8026
    ActorCmdExec 0, Movement_0428
    VMSleep 3
    VMStackPush 0x8025
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0270
    ActorWalkRoute 255, 10, 12, 1, 8, 0
    ActorCmdWait
    VMJump L_027A

L_0270:
    ActorCmdExec 255, Movement_0428
    ActorCmdWait

L_027A:
    VMSleep 8
    FadeEx 3, 0, 16, 4
    FadeExWait
    ActorSetGPos 1, 7, 0, 9, 1
    ActorSetGPos 2, 8, 0, 9, 1
    ActorSetGPos 3, 9, 0, 9, 1
    ActorSetGPos 5, 10, 0, 9, 1
    ActorSetGPos 4, 11, 0, 9, 1
    ActorNew 12, 9, 1, 251, 313, 0
    FadeEx 3, 16, 0, 4
    FadeExWait
    VMSleep 16
    MEPlay SEQ_ME_OTAMARO
    MEWait
    VMSleep 8
    FadeEx 3, 0, 16, 4
    FadeExWait
    ActorSetGPos 1, 13, 0, 10, 2
    ActorSetGPos 2, 14, 0, 5, 2
    ActorSetGPos 3, 8, 0, 7, 1
    ActorSetGPos 5, 8, 0, 12, 0
    ActorSetGPos 4, 6, 0, 11, 0
    ActorDelete 251
    FadeEx 3, 16, 0, 4
    FadeExWait
    VMSleep 32
    ActorCmdExec 0, Movement_0418
    VMSleep 3
    ActorCmdExec 255, Movement_0420
    ActorCmdWait
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 535, 0
    // "Pi pi kiii!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_PiPiKiii, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 535, 0
    // "Pun purin?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_PunPurin, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 535, 0
    // "Waah weeeen!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_WaahWeeeen, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 535, 0
    // "Riiiibbbit. ♪"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_Riiiibbbit, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 535, 0
    // "Croooak!!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity9_Text_Croooak, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0418:
    Move 35, 1
    MoveEnd

Movement_0420:
    Move 34, 1
    MoveEnd

Movement_0428:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
