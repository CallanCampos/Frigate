#include "asm/field_script.inc"
#include "text/script/chargestone_cave_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca: Know what?\nI'm here at Professor Juniper's request![f000]븁\u0000\nI'm researching a Pokémon\ncalled Tynamo![f000]븁\u0000\nBut there aren't very many,\nand they don't seem very strong..."
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_BiancaKnowWhatIm, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0128
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    VMCall Script_4
    VMJump L_0067

L_0051:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That's a Nugget![f000]븁\u0000\nHow'd it get so golden without\ndeep-frying? Trade secret!"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_ThatsNuggetHowdGet, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0067:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0128
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008E
    VMCall Script_4
    VMJump L_00A4

L_008E:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nuggetaboutit!"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_Nuggetaboutit, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00A4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    SEPlay SEQ_SE_MESSAGE
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_00C5
    VMJump L_00D3

L_00C5:
    ActorCmdExec 2, Movement_01D8
    VMJump L_00F4

L_00D3:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_00E6
    VMJump L_00F4

L_00E6:
    ActorCmdExec 2, Movement_01E0
    VMJump L_00F4

L_00F4:
    ActorCmdWait
    // "Hi! I'm the Nugget man.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_HiImNuggetMan, 2, 5, 0
    MsgWinCloseAll
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0117
    VMJump L_0125

L_0117:
    ActorCmdExec 1, Movement_01D8
    VMJump L_0146

L_0125:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0138
    VMJump L_0146

L_0138:
    ActorCmdExec 1, Movement_01E0
    VMJump L_0146

L_0146:
    ActorCmdWait
    // "And I'm the Nugget boy![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_ImNuggetBoy, 1, 3, 0
    MsgWinCloseAll
    // "Glad you showed up!\nI want to give you this.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_GladShowedUpWant, 2, 5, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 92
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "I want to give you this, too.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_WantGiveToo, 1, 3, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 581
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "That's a Nugget![f000]븁\u0000\nHow'd it get so golden without\ndeep-frying? Trade secret!"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_ThatsNuggetHowdGet, 2, 5, 0
    ABKeyWait
    MsgWinCloseAll
    // "Nuggetaboutit!"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave3_Text_Nuggetaboutit, 1, 3, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0128
    VMReturn

Movement_01D8:
    Move 35, 1
    MoveEnd

Movement_01E0:
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
