#include "asm/field_script.inc"
#include "text/script/route_18.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x2704000, 0x5004f, 0x2c78000, 30
    EvCameraWait
    // "The ring is my roiling sea. ♪\nThe towering waves shaped me.[f000]븁\u0000\nCrash! Crash! Crasher Wake!\nCrash! Crash! Crasher Wake![f000]븁\u0000\nI'm the tidal wave of power to wash\nyou away![f000]븁\u0000\nPut out the fire, Crasher Wake!\nRun from electricity, Crasher Wake![f000]븁\u0000\nAh, ah, aaaah!\nThe ring is my sea. ♪[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route18_Text_RingRoilingSeaTowering, 2, 1, 1
    ActorMsgClose
    EvCameraReturn 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0075
    VMJump L_0083

L_0075:
    ActorCmdExec 2, Movement_0224
    VMJump L_00A4

L_0083:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0096
    VMJump L_00A4

L_0096:
    ActorCmdExec 2, Movement_0234
    VMJump L_00A4

L_00A4:
    ActorCmdWait
    Cmd_02D5 49, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D3
    // "Oh! It's you! Hey!\nI had a good time battling you![f000]븁\u0000\nYes! I'm in the mood for a\nbattle both the winner and[f000]븀\u0000\nloser will say is fun![f000]븁\u0000\nOK! Time to take off for\nDriftveil City! Yeah!"
    ParentActorMsg MSGFILE_SCRIPT, Route18_Text_OhItsHeyHad, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_011A

L_00D3:
    // "Do you know what the Wake is singing?[f000]븁\u0000\nIt's the theme song of Crasher Wake,\npro wrestler and Gym Leader[f000]븀\u0000\nof Pastoria City in the Sinnoh region![f000]븁\u0000\nYou know it?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route18_Text_KnowWhatWakeSinging, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0104
    // "Yeeeeah![f000]븁\u0000\nThat's right! I'm the Gym Leader\nwho's got it all--as a Pokémon[f000]븀\u0000\npro wrestler and as a singer![f000]븀\u0000\nI'm Pastoria Gym's Crasher Wake![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route18_Text_YeeeeahThatsRightIm, 0, 0
    VMJump L_010E

L_0104:
    // "You've got to be kidding![f000]븁\u0000\nI'm the Gym Leader\nwho's got it all--as a Pokémon[f000]븀\u0000\npro wrestler and as a singer![f000]븀\u0000\nI'm Pastoria Gym's Crasher Wake![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route18_Text_YouveGotKiddingIm, 0, 0

L_010E:
    // "Here's a little bit of\nCrasher Wake trivia for ya![f000]븁\u0000\nEveryone says I'm a wrestler\nfrom abroad, but the truth is...[f000]븁\u0000\nI was born and raised\nin the Sinnoh region![f000]븁\u0000\nIt's what they call my gimmick!\nHey, but that's a secret, OK?[f000]븁\u0000\nOK! Time to take off for\nDriftveil City! Yeah![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route18_Text_HeresLittleBitCrasher, 0, 0
    MsgWinCloseAll

L_011A:
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0131
    VMJump L_014D

L_0131:
    ActorCmdExec 2, Movement_01D4
    VMSleep 10
    ActorCmdExec 255, Movement_023C
    ActorCmdWait
    VMJump L_014D

L_014D:
    ActorWalkRoute 2, 632, 711, 1, 8, 0
    VMSleep 10
    ActorCmdExec 255, Movement_01D4
    VMSleep 15
    FadeEx 3, 0, 16, 4
    FadeExWait
    ActorCmdWait
    ActorDelete 2
    FlagSet EVENT_FLAG_0x0351
    VMSleep 30
    FadeEx 3, 16, 0, 4
    FadeExWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorDelete 10
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 458
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x03af
    WorkSetConst EVENT_WORK_0x4074, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01D4:
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 154, 1
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

Movement_0224:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0

Movement_0234:
    Move 3, 1
    MoveEnd

Movement_023C:
    Move 0, 1
    Move 71, 1
    Move 13, 1
    Move 72, 1
    MoveEnd
