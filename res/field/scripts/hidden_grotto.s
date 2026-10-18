#include "asm/field_script.inc"
#include "text/script/hidden_grotto.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    HiddenHollowGetParam 0, 0x8021
    HiddenHollowGetParam 1, 0x8020
    HiddenHollowGetParam 2, 0x8022
    HiddenHollowGetParam 3, 0x8023
    WorkSetConst 0x8024, 0
    WorkOr 0x8024, 8
    WorkOr 0x8024, 16
    WorkOr 0x8024, 128
    WorkOr 0x8024, 512
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0099
    WorkOr 0x8024, 64
    VMJump L_009F

L_0099:
    WorkOr 0x8024, 32

L_009F:
    FunfestMissionBroadcast 29, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 590
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 591
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00DE
    ActorCmdExec 0x8011, Movement_0228
    ActorCmdWait
    WorkOr 0x8024, 1024
    VMJump L_00E0

L_00DE:
    ActorSetEyeToEye

L_00E0:
    CallWildBattleEx 0x8021, 0x8020, 0x8022, 0x8024
    WildBattleIsVictory 0x8025
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010D
    ActorDelete 224
    CallWildBattleEnd
    VMJump L_010F

L_010D:
    CallWildLose

L_010F:
    HiddenHollowReset
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8026, 0
    HiddenHollowGetParam 4, 0x8026
    ActorDelete 224
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8026
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    HiddenHollowReset
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8027, 0
    HiddenHollowGetParam 5, 0x8027
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8027
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    HiddenHollowReset
    FlagReset EVENT_FLAG_0x097f
    ObjInitPointGPos 0, 15, 0, 31
    RecordAdd 44, 1
    WorkSetConst 0x8027, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    HiddenHollowCreateEvents
    VMCall L_01DD
    VMHalt

Script_7:
    VMCall L_01DD
    VMHalt

L_01DD:
    WorkSetConst 0x8028, 0
    HiddenHollowGetParam 6, 0x8028
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0206
    ObjInitPointGPos 0, 15, 0, 12

L_0206:
    WorkSetConst 0x8028, 0
    VMReturn

Script_5:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0230
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    HiddenHollowCallWarpOut
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0228:
    Move 49, 2
    MoveEnd

Movement_0230:
    Move 33, 1
    Move 1, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    FlagSet EVENT_FLAG_0x0a51
    Cmd_02C5 27
    ActorNew 15, 26, 0, 251, 249, 0
    ActorWalkRoute 251, 14, 14, 1, 8, 1
    VMSleep 10
    ActorWalkRoute 255, 15, 14, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 251, Movement_02D8
    ActorCmdWait
    // "Hey, a Pokémon![f000]븁\u0000\nA Pokémon that hides in a place\nlike this might be pretty amazing![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HiddenGrotto_Text_HeyPokemonPokemonHides, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_02F0
    VMSleep 8
    ActorCmdExec 255, Movement_02E8
    ActorCmdWait
    // "Amazing! This is a huge discovery!\nAn incredible find![f000]븁\u0000\nI'll go check a lot of other trees\nto see if there are more Hidden Grottoes![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HiddenGrotto_Text_AmazingHugeDiscoveryIncredible, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 15, 23, 1, 8, 1
    VMSleep 25
    ActorCmdExec 255, Movement_02E0
    ActorCmdWait
    ActorDelete 251
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_02D8:
    Move 32, 1
    MoveEnd

Movement_02E0:
    Move 33, 1
    MoveEnd

Movement_02E8:
    Move 34, 1
    MoveEnd

Movement_02F0:
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
    Move 1, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 3, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd
