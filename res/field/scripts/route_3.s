#include "asm/field_script.inc"
#include "text/script/route_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_4:
    ActorsPauseAll
    ActorCmdExec 0, Movement_0090
    ActorCmdWait
    WordSetPlayerName 0
    TrainerCardGetSex 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0073
    // "Heeey, [f000]Ā\u0001\u0000!\nCome here![f000]븁\u0000"
    InfoMsg Route3_Text_HeeeyComeHere, 1
    VMJump L_0078

L_0073:
    // "Heeey, [f000]Ā\u0001\u0000!\nCome here![f000]븁\u0000"
    InfoMsg Route3_Text_HeeeyComeHere_2, 1

L_0078:
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0098
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x417f, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0090:
    Move 1, 1
    MoveEnd

Movement_0098:
    Move 32, 1
    MoveEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 3"
    MsgPlaceSign Route3_Text_Route3, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Pokémon Day Care\nWe Take Care of Your Precious Pokémon"
    MsgPlaceSign Route3_Text_PokemonDayCareWe, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nTap the yellow button at the top of a\nPC Box to switch to Group Move mode.[f000]븁\u0000\nIt lets you move groups\nof Pokémon in your PC Boxes."
    MsgPlaceSign Route3_Text_TrainerTipsTapYellow, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a6a
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon have been with me\nsince I was little![f000]븁\u0000\nThey are always just\nraring to battle!"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_PokemonHaveBeenSince, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You know how many people use\nProtect or Detect in Double Battles?[f000]븀\u0000\nThat's the time to use Feint!"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_KnowHowManyPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon Trainer!\nDo you have a Pokémon Egg?"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_PokemonTrainerHavePokemon, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0194
    PokePartyGetCount 0x8023, 3
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0180
    // "Wow! It's a Pokémon Egg!\nIt feels sort of warm![f000]븀\u0000\nA Pokémon will hatch from this, right?"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_WowItsPokemonEgg, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_018E

L_0180:
    // "Teacher says that\nlies lead to a life of crime!"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_TeacherSaysLiesLead, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_018E:
    VMJump L_01A2

L_0194:
    // "Aww...[f000]븁\u0000\nThe old guy next door said he didn't\nknow when they'd find another Egg...[f000]븁\u0000\nBut I really want to see one!"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_AwwOldGuyNext, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01A2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes they let my big brother\nplay together with Pokémon, too!"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_SometimesTheyLetBig, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Go! Go! People who ride Bicycles\nare so cool!"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_GoGoPeopleWho, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_020B

L_01F7:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Don't you have a Bicycle?\nCan you even ride one?"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_DontHaveBicycleCan, 0, 0
    LastKeyWait
    ActorMsgClose

L_020B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01d7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey! Come on!\nShow me your Habitat List![f000]븁\u0000\nI want to see all of Route 3's Pokémon!\nThe ones in the tall grass, the ones[f000]븀\u0000\nyou fish for, and the ones on the water![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_HeyComeShowHabitat, 0, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    PokeDexCheckHabitatList 321, 0, 0, 0x8024
    PokeDexCheckHabitatList 321, 1, 0, 0x8025
    PokeDexCheckHabitatList 321, 2, 0, 0x8026
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02DD
    // "Oh... Really?[f000]븁\u0000\nI never knew there were this\nmany Pokémon on Route 3![f000]븁\u0000\nPokémon Trainer, that's amazing!\nI'm so moved, I'll give you this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_OhReallyNeverKnew, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 7
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01d7
    // "Pokémon Trainers sure\nare good at meeting Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_PokemonTrainersSureGood, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02EB

L_02DD:
    // "If you fill up the Habitat List,\nI'd like you to tell me![f000]븁\u0000\nI want to see all of Route 3's Pokémon!\nThe ones in the tall grass, the ones[f000]븀\u0000\nyou fish for, and the ones on the water.[f000]븀\u0000\nGot it?"
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_IfFillUpHabitat, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02EB:
    VMJump L_0305

L_02F1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon Trainers sure\nare good at meeting Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, Route3_Text_PokemonTrainersSureGood, 0, 0
    LastKeyWait
    ActorMsgClose

L_0305:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
