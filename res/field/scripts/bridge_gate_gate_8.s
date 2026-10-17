#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_8.h"

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
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x411e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0061
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Marine Tube is just ahead.[f000]븁\u0000\nBut please wait for a little bit longer.\nJust a little bit..."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate8_Text_MarineTubeJustAhead, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0075

L_0061:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Marine Tube is ahead!\nPlease enjoy the stunning scenery!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate8_Text_MarineTubeAheadPlease, 0, 0
    LastKeyWait
    ActorMsgClose

L_0075:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I moved here just to be the first person\nto go through the Marine Tube."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate8_Text_MovedHereJustFirst, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Marine Tube.[f000]븁\u0000\nIt's an undersea tunnel, so to speak.\nDo you know how such tunnels are made?[f000]븁\u0000\nIt's quite simple!\nThey're built on land[f000]븀\u0000\nand then sunk into the sea![f000]븁\u0000\nWithout Pokémon, the construction\nwould've been impossible."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate8_Text_MarineTubeItsUndersea, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorCmdExec 1, Movement_0114
    ActorCmdWait
    WorkAdd 0x8021, 2
    ActorWalkRoute 1, 0x8021, 0x8022, 1, 4, 1
    ActorCmdWait
    // "Sorry, I'll be finished with the cleaning\nsoon. Please wait until then![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BridgeGateGate8_Text_SorryIllFinishedCleaning, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_017C
    ActorWalkRoute 1, 18, 8, 1, 4, 0
    ActorCmdWait
    ActorCmdExec 1, Movement_01AC
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0114:
    Move 17, 1
    Move 16, 2
    Move 17, 2
    Move 16, 2
    Move 17, 1
    Move 34, 1
    Move 75, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a sign that explains\nthe Marine Tube.[f000]븁\u0000"
    InfoMsg BridgeGateGate8_Text_ItsSignExplainsMarine, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 3, 0
    FieldOpen
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_017C:
    Move 14, 1
    MoveEnd
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
    Move 32, 1
    MoveEnd

Movement_01AC:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
