#include "asm/field_script.inc"
#include "text/script/route_4_14.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm very particular about a\nPokémon's Speed!"
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_ImVeryParticularAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad4
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02FC
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CC

L_008A:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C8
    Random 0x8020, 201
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00C2
    WorkGet EVENT_WORK_0x4187, 0x8020
    WorkSetConst 0x8021, 1

L_00C2:
    VMJump L_008A

L_00C8:
    FlagSet EVENT_FLAG_DAILY_0x0ad3

L_00CC:
    // "I'm very particular about a\nPokémon's Attack!"
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_ImVeryParticularAbout_2, 0, 0
    MsgWaitAdvance
    WordSetNumber 1, EVENT_WORK_0x4187, 3
    // "That's why... You![f000]븁\u0000\nDo you have a Pokémon whose Attack\nstat is the same as or higher than [f000]Ȃ\u0001\u0001?"
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_ThatsWhyHavePokemon, 0, 0
    MsgWaitAdvance
    PokePartyGetCount 0x8022, 0

L_00F1:
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0177
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016B
    PokePartyGetParam 0x8024, 0x8023, 162
    PokePartyIsEgg 0x8027, 0x8023
    PokePartyGetParam 0x8028, 0x8023, 160
    VMStackPush 0x8024
    VMStackPush EVENT_WORK_0x4187
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_016B
    WordSetPartyPokeSpecies 0, 0x8023
    WorkSetConst 0x8026, 1

L_016B:
    WorkAdd 0x8023, 1
    VMJump L_00F1

L_0177:
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0216
    WorkSetConst 0x8023, 0

L_0190:
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0216
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020A
    PokePartyGetParam 0x8024, 0x8023, 162
    PokePartyIsEgg 0x8027, 0x8023
    PokePartyGetParam 0x8028, 0x8023, 160
    VMStackPush 0x8024
    VMStackPush EVENT_WORK_0x4187
    VMStackCmp CMP_GE
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_020A
    WordSetPartyPokeSpecies 0, 0x8023
    WorkSetConst 0x8025, 1

L_020A:
    WorkAdd 0x8023, 1
    VMJump L_0190

L_0216:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0279
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0310
    ActorCmdWait
    // "Your [f000]ā\u0001\u0000!\nIt truly has the Attack stat I like![f000]븁\u0000\nI'm very happy!\nI'll give you lots of these![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_TrulyHasAttackStat, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 570
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Various Attacks. ♪\nVarious Pokémon. ♪[f000]븁\u0000\nI don't know why, but I feel happy!\nCome back again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_VariousAttacksVariousPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ad4
    VMJump L_02F6

L_0279:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DC
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0310
    ActorCmdWait
    // "Your [f000]ā\u0001\u0000!\nI like its Attack stat![f000]븁\u0000\nI'm so happy, so I'll give you this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_LikeItsAttackStat, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 570
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Various Attacks. ♪\nVarious Pokémon. ♪[f000]븁\u0000\nI don't know why, but I feel happy!\nCome back again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_VariousAttacksVariousPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ad4
    VMJump L_02F6

L_02DC:
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0310
    ActorCmdWait
    // "That's quite all right! Good things about\nPokémon are not just Attack stats.[f000]븁\u0000\nBut, today I just feel like meeting\na Pokémon whose Attack stat is[f000]븀\u0000\nhigher than [f000]Ȃ\u0001\u0001."
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_ThatsQuiteAllRight, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02F6:
    VMJump L_030A

L_02FC:
    // "Various Attacks. ♪\nVarious Pokémon. ♪[f000]븁\u0000\nI don't know why, but I feel happy!\nCome back again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, Route414_Text_VariousAttacksVariousPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_030A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0310:
    Move 161, 1
    MoveEnd
