#include "asm/field_script.inc"
#include "text/script/victory_road_22.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x4123
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004F
    ActorSetGPos 9, 35, 2, 32, 1
    VMJump L_006E

L_004F:
    VMStackPush EVENT_WORK_0x4123
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006E
    ActorSetGPos 9, 30, 2, 33, 2

L_006E:
    VMHalt

Script_2:
    ActorsPauseAll
    ActorCmdExec 255, Movement_012C
    ActorCmdWait
    PVPlay 571, 0
    // "Kwaaaaan!"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad22_Text_Kwaaaaan, 9, 0, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 9, 44, 19, 1, 6, 0
    ActorCmdWait
    ActorCmdExec 9, Movement_0114
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 9
    SEWait
    WorkSetConst EVENT_WORK_0x4123, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    PVPlay 571, 0
    // "Kwaaaaan!"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad22_Text_Kwaaaaan, 9, 0, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 9, 35, 32, 1, 6, 0
    ActorCmdWait
    ActorCmdExec 9, Movement_0114
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 9
    SEWait
    WorkSetConst EVENT_WORK_0x4123, 3
    FlagSet EVENT_FLAG_0x03c1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0114:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_012C:
    Move 35, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 571, 0
    // "Kwaaan!"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad22_Text_Kwaaan, 9, 0, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "The Pokémon won't move.\nIt might be protecting something..."
    SystemMsg VictoryRoad22_Text_PokemonWontMoveMight, 2
    LastKeyWait
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
