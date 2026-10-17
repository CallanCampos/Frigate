#include "asm/field_script.inc"
#include "text/script/route_4_2.h"

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
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad4
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E0
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B0

L_006E:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AC
    Random 0x8020, 201
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00A6
    WorkGet EVENT_WORK_0x4187, 0x8020
    WorkSetConst 0x8021, 1

L_00A6:
    VMJump L_006E

L_00AC:
    FlagSet EVENT_FLAG_DAILY_0x0ad3

L_00B0:
    // "I'm very particular about\nthe Speed of Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_ImVeryParticularAbout, 0, 0
    MsgWaitAdvance
    WordSetNumber 1, EVENT_WORK_0x4187, 3
    // "That's why I'm wondering if you\nhave any Pokémon with a Speed[f000]븀\u0000\nof [f000]Ȃ\u0001\u0001 or greater with you!"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_ThatsWhyImWondering, 0, 0
    MsgWaitAdvance
    PokePartyGetCount 0x8022, 0

L_00D5:
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_015B
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014F
    PokePartyGetParam 0x8024, 0x8023, 164
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
    VMJumpIf CMP_STACK, L_014F
    WordSetPartyPokeSpecies 0, 0x8023
    WorkSetConst 0x8026, 1

L_014F:
    WorkAdd 0x8023, 1
    VMJump L_00D5

L_015B:
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FA
    WorkSetConst 0x8023, 0

L_0174:
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_01FA
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01EE
    PokePartyGetParam 0x8024, 0x8023, 164
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
    VMJumpIf CMP_STACK, L_01EE
    WordSetPartyPokeSpecies 0, 0x8023
    WorkSetConst 0x8025, 1

L_01EE:
    WorkAdd 0x8023, 1
    VMJump L_0174

L_01FA:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025D
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0310
    ActorCmdWait
    // "Your [f000]ā\u0001\u0000 definitely\nhas the Speed that I like![f000]븁\u0000\nI'm really really happy,\nso I'll give you these![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_DefinitelyHasSpeedLike, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 565
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "There are many different Pokémon\nwith many different Speeds! ♪[f000]븁\u0000\nIf you'd like, please come\nvisit me again tomorrow, OK?"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_ThereManyDifferentPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ad4
    VMJump L_02DA

L_025D:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0310
    ActorCmdWait
    // "Your [f000]ā\u0001\u0000 is fast!\nJust like I like them![f000]븁\u0000\nThat makes me really happy,\nso I'll give you this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_FastJustLikeLike, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 565
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "There are many different Pokémon\nwith many different Speeds! ♪[f000]븁\u0000\nIf you'd like, please come\nvisit me again tomorrow, OK?"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_ThereManyDifferentPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ad4
    VMJump L_02DA

L_02C0:
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0310
    ActorCmdWait
    // "Don't worry! There's more to\nPokémon than Speed.[f000]븁\u0000\nBut, today, I'd sure like to see\na Pokémon with Speed greater than [f000]Ȃ\u0001\u0001!"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_DontWorryTheresMore, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02DA:
    VMJump L_02EE

L_02E0:
    // "There are many different Pokémon\nwith many different Speeds! ♪[f000]븁\u0000\nIf you'd like, please come\nvisit me again tomorrow, OK?"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_ThereManyDifferentPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02EE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm very particular about\nPokémon's Attack stat!"
    ParentActorMsg MSGFILE_SCRIPT, Route42_Text_ImVeryParticularAbout_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0310:
    Move 161, 1
    MoveEnd
