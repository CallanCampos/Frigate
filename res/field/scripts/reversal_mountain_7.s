#include "asm/field_script.inc"
#include "text/script/reversal_mountain_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    WordSetPlayerName 0
    VMCall L_0138
    // "Oh! Undella Town is right through here!\nI want to keep looking around a bit more.[f000]븁\u0000\nWhat do you want to do?\nShould we say bye for now?"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain7_Text_OhUndellaTownRight, 254, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F8
    VMStackPush EVENT_WORK_0x4121
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006F
    // "OK, then! I want to do a little more\nresearch about Heatran anyway![f000]븁\u0000\nThank you for coming with me!\nBe careful on the rest of your journey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain7_Text_OkThenWantLittle, 254, 0, 0
    VMJump L_007B

L_006F:
    // "OK, then![f000]븁\u0000\nI want to do a little more research\nabout where Heatran might be![f000]븁\u0000\nThank you for coming with me!\nBe careful on the rest of your journey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain7_Text_OkThenWantLittle_2, 254, 0, 0

L_007B:
    MsgWinCloseAll
    ActorWalkRoute 254, 25, 15, 1, 8, 0
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00AE
    VMSleep 16
    ActorCmdExec 255, Movement_0224

L_00AE:
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 2
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst EVENT_WORK_0x4120, 3
    WorkSetConst EVENT_WORK_0x4121, 3
    FlagSet EVENT_FLAG_0x03bf
    FlagReset EVENT_FLAG_0x03c0
    HollowRivalCmd_0262 3, 0
    ActorAdd 0
    ActorDelete 254
    VMJump L_0116

L_00F8:
    // "OK! Then, let's look around a bit more![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain7_Text_OkThenLetsLook, 254, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_01FC
    ActorCmdWait
    ActorPairSetMoveEnable 0

L_0116:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Reversal Mountain... I wonder...\nCould a Magma Stone be in there?[f000]븁\u0000\nHave you heard of it?\nThey say a Magma Stone was found[f000]븀\u0000\nin a volcano in the distant Sinnoh region.[f000]븁\u0000\nApparently, it had something\nto do with Heatran!"
    ParentActorMsg MSGFILE_SCRIPT, ReversalMountain7_Text_ReversalMountainWonderCould, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0138:
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_014F
    VMJump L_0165

L_014F:
    ActorCmdExec 255, Movement_023C
    ActorCmdExec 254, Movement_0214
    VMJump L_01E0

L_0165:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_0178
    VMJump L_018E

L_0178:
    ActorCmdExec 255, Movement_0234
    ActorCmdExec 254, Movement_021C
    VMJump L_01E0

L_018E:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_01A1
    VMJump L_01B7

L_01A1:
    ActorCmdExec 255, Movement_022C
    ActorCmdExec 254, Movement_0204
    VMJump L_01E0

L_01B7:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_01CA
    VMJump L_01E0

L_01CA:
    ActorCmdExec 255, Movement_0224
    ActorCmdExec 254, Movement_020C
    VMJump L_01E0

L_01E0:
    ActorCmdWait
    VMReturn
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_01FC:
    Move 14, 1
    MoveEnd

Movement_0204:
    Move 0, 1
    MoveEnd

Movement_020C:
    Move 1, 1
    MoveEnd

Movement_0214:
    Move 2, 1
    MoveEnd

Movement_021C:
    Move 3, 1
    MoveEnd

Movement_0224:
    Move 32, 1
    MoveEnd

Movement_022C:
    Move 33, 1
    MoveEnd

Movement_0234:
    Move 34, 1
    MoveEnd

Movement_023C:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
