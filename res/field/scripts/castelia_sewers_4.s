#include "asm/field_script.inc"
#include "text/script/castelia_sewers_4.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_DAILY_0x0add
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0035
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Tomorrow is another day!\nI run my experiments every day.[f000]븁\u0000\nYou're welcome to stop by again\nand see the result!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers4_Text_TomorrowAnotherDayRun, 0, 0
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
    // "I'm working on experiments to create\nmedicines from the toxins found in[f000]븀\u0000\nthe sewer system.[f000]븁\u0000\nIf I truly succeed in these experiments,\nI can create a lot of medicines from[f000]븀\u0000\nthe venom of Poison-type Pokémon.[f000]븁\u0000\nWell...\nToday's experiment was...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers4_Text_ImWorkingExperimentsCreate, 0, 0
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
    ActorCmdExec 0, Movement_0160
    ActorCmdWait
    VMSleep 15
    ActorCmdExec 0, Movement_0158
    ActorCmdWait
    // "This experiment was\nvery successful![f000]븁\u0000\nI created a Full Restore. Here, take it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers4_Text_ExperimentVerySuccessfulCreated, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 23
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
    // "This experiment was successful![f000]븁\u0000\nI created a Full Heal. Here, take it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers4_Text_ExperimentSuccessfulCreatedFull, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 27
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0143

L_0117:
    // "This experiment was OK.[f000]븁\u0000\nI created an Antidote. Here, take it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers4_Text_ExperimentOkCreatedAntidote, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 18
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_0143:
    // "Tomorrow is another day!\nI run my experiments every day.[f000]븁\u0000\nYou're welcome to stop by again\nand see the result!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers4_Text_TomorrowAnotherDayRun, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0add
    VMReturn
    .balign 4, 0

Movement_0158:
    Move 75, 1
    MoveEnd

Movement_0160:
    Move 159, 1
    MoveEnd

Movement_0168:
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd
