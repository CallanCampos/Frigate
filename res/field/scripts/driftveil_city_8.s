#include "asm/field_script.inc"
#include "text/script/driftveil_city_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm very sorry.[f000]븁\u0000\nThe Pokémon World Tournament\nwill commence shortly,[f000]븀\u0000\nbut we're still preparing the area.[f000]븁\u0000\nOh, you don't have the Driftveil City\nGym Badge yet?[f000]븁\u0000\nIn that case, how about taking\non the Pokémon Gym first?"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity8_Text_ImVerySorryPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0077
    ActorCmdExec 0, Movement_0154
    VMSleep 16
    ActorCmdExec 255, Movement_01E8
    ActorCmdWait
    VMJump L_008D

L_0077:
    ActorCmdExec 0, Movement_0148
    VMSleep 16
    ActorCmdExec 255, Movement_01F0
    ActorCmdWait

L_008D:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00BC
    WorkSub 0x8021, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait
    VMJump L_00E5

L_00BC:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_00E5
    WorkAdd 0x8021, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait

L_00E5:
    // "I'm very sorry.[f000]븁\u0000\nThe Pokémon World Tournament\nwill commence shortly,[f000]븀\u0000\nbut we're still preparing the area.[f000]븁\u0000\nOh, you don't have the Driftveil City\nGym Badge yet?[f000]븁\u0000\nIn that case, how about taking\non the Pokémon Gym first?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity8_Text_ImVerySorryPokemon, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 255, Movement_01A0
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_GE
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_LE
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0134
    ActorWalkRoute 0, 5, 8, 1, 8, 0

L_0134:
    ActorCmdWait
    ActorCmdExec 0, Movement_01D8
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0148:
    Move 34, 1
    Move 75, 1
    MoveEnd

Movement_0154:
    Move 35, 1
    Move 75, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you keep winning, you get BP!\nWhich is to say, you win Battle Points![f000]븁\u0000\nSo save up lots of BP, and exchange\nthem for great items!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity8_Text_IfKeepWinningGet, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokémon World Tournament\nis ahead!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity8_Text_PokemonWorldTournamentAhead, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 13, 1
    MoveEnd

Movement_01A0:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
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

Movement_01D8:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_01E8:
    Move 34, 1
    MoveEnd

Movement_01F0:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
