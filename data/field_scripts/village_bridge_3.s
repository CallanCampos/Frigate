#include "asm/field_script.inc"
#include "text/script/village_bridge_3.h"

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
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01c9
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0077
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This bread will have tons of Honey.\nI'll call it Honey Bread![f000]븁\u0000\nI want to bake it soon, but I'm busy\nmaking Village Sandwiches."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_BreadWillHaveTons, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0289

L_0077:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerFlagGet TRAINER_BAKER_CHRIS, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DD
    VMStackPushFlag EVENT_FLAG_0x01c8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BD
    // "Oh!\nYou're a newcomer, aren't you?[f000]븁\u0000\nThen, let's battle without saying a word!\nAre you ready?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_OhYoureNewcomerArent, 0, 0
    FlagSet EVENT_FLAG_0x01c8
    VMJump L_00C7

L_00BD:
    // "Let's battle without saying a word!\nAre you ready?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_LetsBattleWithoutSaying, 0, 0

L_00C7:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C9
    // "All right![f000]븁\u0000\nI want to bake tasty bread\nfor strong Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_AllRightWantBake, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_BAKER_CHRIS, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0115
    CallTrainerBattleEnd
    TrainerFlagSet TRAINER_BAKER_CHRIS
    VMJump L_0117

L_0115:
    CallTrainerLose

L_0117:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B5
    // "Wow, what a Trainer![f000]븁\u0000\nYou understand and trust your Pokémon.\nThat's why you got this result!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_WowWhatTrainerUnderstand, 0, 0
    MsgWaitAdvance
    // "I have a favor to ask\nof such a wonderful Trainer![f000]븁\u0000\nI'm planning to bake bread\nwith tons of Honey![f000]븁\u0000\nWould you show me a Pokémon\nwith the Honey Gather Ability?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_HaveFavorAskSuch, 0, 0
    VMCall L_028F
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AB
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_03C8
    ActorCmdWait
    // "This [f000]ā\u0001\u0000\nwill gather Honey![f000]븁\u0000\nThank you!\nThis gift is a token of my gratitude![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_WillGatherHoneyThank, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 157
    WorkSet 0x8001, 3
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "This bread will have tons of Honey.\nI'll call it Honey Bread![f000]븁\u0000\nI want to bake it soon, but I'm busy\nmaking Village Sandwiches."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_BreadWillHaveTons, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01c9
    VMJump L_01AF

L_01AB:
    LastKeyWait
    MsgWinCloseAll

L_01AF:
    VMJump L_01C3

L_01B5:
    // "Wow, what a Trainer![f000]븁\u0000\nYou understand and trust your Pokémon.\nThat's why you got this result!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_WowWhatTrainerUnderstand, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01C3:
    VMJump L_01D7

L_01C9:
    // "Uh-oh... That's fine.\nWhen you change your mind, come back!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_UhOhThatsFine, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01D7:
    VMJump L_0289

L_01DD:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027B
    // "Wow, what a Trainer![f000]븁\u0000\nYou understand and trust your Pokémon.\nThat's why you got this result!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_WowWhatTrainerUnderstand, 0, 0
    MsgWaitAdvance
    // "I have a favor to ask\nof such a wonderful Trainer![f000]븁\u0000\nI'm planning to bake bread\nwith tons of Honey![f000]븁\u0000\nWould you show me a Pokémon\nwith the Honey Gather Ability?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_HaveFavorAskSuch, 0, 0
    VMCall L_028F
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0271
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_03C8
    ActorCmdWait
    // "This [f000]ā\u0001\u0000\nwill gather Honey![f000]븁\u0000\nThank you!\nThis gift is a token of my gratitude![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_WillGatherHoneyThank_2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 157
    WorkSet 0x8001, 3
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "This bread will have tons of Honey.\nI'll call it Honey Bread![f000]븁\u0000\nI want to bake it soon, but I'm busy\nmaking Village Sandwiches."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_BreadWillHaveTons, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01c9
    VMJump L_0275

L_0271:
    LastKeyWait
    MsgWinCloseAll

L_0275:
    VMJump L_0289

L_027B:
    // "Wow, what a Trainer![f000]븁\u0000\nYou understand and trust your Pokémon.\nThat's why you got this result!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_WowWhatTrainerUnderstand, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0289:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_028F:
    PokePartyGetCount 0x8020, 0

L_0295:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0306
    PokePartyGetParam 0x8022, 0x8021, 10
    PokePartyIsEgg 0x8024, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 118
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02FA
    PokePartyGetSpecies 0x8026, 0x8021
    WordSetPokeSpecies 0, 0x8026
    WorkSetConst 0x8023, 1

L_02FA:
    WorkAdd 0x8021, 1
    VMJump L_0295

L_0306:
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm totally hooked on Village Sandwiches![f000]븁\u0000\nI come here every day to eat them!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_ImTotallyHookedVillage, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon make the most adorable\nfaces when they bite into sandwiches!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_PokemonMakeMostAdorable, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm so happy to be able to eat\nwith my Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_ImHappyAbleEat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 572, 0
    // "Myu myuweee."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_MyuMyuweee, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Skwee weep..."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_SkweeWeep, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Kroooko!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge3_Text_Kroooko, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_03C8:
    Move 75, 1
    MoveEnd
