#include "asm/field_script.inc"
#include "text/script/castelia_sewers_5.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ade
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0035
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'll keep experimenting every day!\nIt's important to keep trying.[f000]븀\u0000\nCome back and see how it's going!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers5_Text_IllKeepExperimentingEvery, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_003B

L_0035:
    VMCall L_0041

L_003B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0041:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Lots of toxins build up in the sewer\nsystem. I think I can use them to make[f000]븀\u0000\nmedicines. So I'm running an experiment![f000]븁\u0000\nIf this works, I might be able to use the\nvenom of Poison-type Pokémon to make[f000]븀\u0000\ndifferent medicines. How exciting![f000]븁\u0000\nWell, well...\nThe result of today's experiment was...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers5_Text_LotsToxinsBuildUp, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0168
    ActorCmdWait
    VMSleep 30
    Random 0x8010, 100
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPushConst 89
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00C8
    ActorCmdExec 0, Movement_0158
    ActorCmdWait
    VMSleep 15
    ActorCmdExec 0, Movement_0170
    ActorCmdWait
    // "Wow! Today's experiment\nwas super successful![f000]븁\u0000\nHere! Please accept\nthe Revive I made![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers5_Text_WowTodaysExperimentSuper, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 28
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0143

L_00C8:
    VMStackPush 0x8010
    VMStackPushConst 59
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0117
    ActorCmdExec 0, Movement_0158
    ActorCmdWait
    // "Today's experiment was a success![f000]븁\u0000\nHere! Please accept the\nSuper Potion I made.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers5_Text_TodaysExperimentSuccessHere, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 26
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0143

L_0117:
    // "Well, today's experiment could have\ngone better.[f000]븁\u0000\nBut I did manage to make a Potion.\nHere, you can have it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers5_Text_WellTodaysExperimentCould, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_0143:
    // "I'll keep experimenting every day!\nIt's important to keep trying.[f000]븀\u0000\nCome back and see how it's going!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers5_Text_IllKeepExperimentingEvery, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ade
    VMReturn
    .balign 4, 0

Movement_0158:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_0168:
    Move 161, 1
    MoveEnd

Movement_0170:
    Move 160, 1
    MoveEnd
