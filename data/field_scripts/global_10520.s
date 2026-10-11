#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 0x8011, Movement_0128
    ActorCmdWait
    ActorGetUserParam 0x8011, 0, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0043
    WorkSetConst 0x8020, 5

L_0043:
    CallWildBattle 590, 0x8020, 1024
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    ActorGetSpawnFlag 0x8011, 0x8010
    FlagSet 0x8010
    ActorDelete 0x8011
    CallWildBattleEnd
    VMCall L_010E
    VMJump L_0080

L_007E:
    CallWildLose

L_0080:
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 0x8011, Movement_0128
    ActorCmdWait
    ActorGetUserParam 0x8011, 0, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C5
    WorkSetConst 0x8021, 5

L_00C5:
    CallWildBattle 591, 0x8021, 1024
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0100
    ActorGetSpawnFlag 0x8011, 0x8010
    FlagSet 0x8010
    ActorDelete 0x8011
    CallWildBattleEnd
    VMCall L_010E
    VMJump L_0102

L_0100:
    CallWildLose

L_0102:
    WorkSetConst 0x8021, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_010E:
    VMStackPushFlag EVENT_FLAG_0x0998
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0125
    FlagSet EVENT_FLAG_0x0998

L_0125:
    VMReturn
    .balign 4, 0

Movement_0128:
    Move 49, 2
    MoveEnd
