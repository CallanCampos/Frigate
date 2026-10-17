#include "asm/field_script.inc"
#include "text/script/undella_town_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    WordSetPlayerName 0
    BGMPlay SEQ_BGM_E_SHIRONA
    ActorCmdExec 1, Movement_00D4
    ActorCmdWait
    // "???: What's this?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_Whats, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_00DC
    ActorCmdWait
    // "What's your name?[f000]븁\u0000\n...[f000]븁\u0000\nOK. I'll remember that!\n[f000]Ā\u0001\u0000, nice to meet you.[f000]븁\u0000\nI'm Cynthia.\nI'm a Pokémon Trainer, too, like you.[f000]븁\u0000\nI have an insatiable curiosity for\nresearching Pokémon myths.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_WhatsNameOkIll, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_00E4
    ActorCmdWait
    // "I'm sure you know about Undella Bay's\nAbyssal Ruins, right?[f000]븁\u0000\nI'm staying here at my friend's villa\nso I can investigate them.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_ImSureKnowAbout, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_00EC
    ActorCmdWait
    // "In order to get to know each other\nbetter as Pokémon Trainers,[f000]븀\u0000\nI would like our Pokémon to have a match.[f000]븁\u0000\nWould you care to be my opponent?"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_OrderGetKnowEach, 1, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B4
    VMCall L_00F4
    BGMChangeMap
    VMJump L_00CC

L_00B4:
    // "Ha ha. You prefer to take things slowly\nand rationally, am I right?[f000]븁\u0000\nWhen you're ready, come and talk to me.\nI'll be happy to see you."
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_HaHaPreferTake, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x4098, 2

L_00CC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_00D4:
    Move 75, 1
    MoveEnd

Movement_00DC:
    Move 13, 2
    MoveEnd

Movement_00E4:
    Move 35, 1
    MoveEnd

Movement_00EC:
    Move 33, 1
    MoveEnd

L_00F4:
    WordSetPlayerName 0
    // "Before I send out my Pokémon,\nmy heart always begins to race...[f000]븁\u0000\nInteresting...\nMy Pokémon in their Poké Balls are[f000]븀\u0000\nradiating a happy feeling.[f000]븁\u0000\nAre you the reason?\nWhat are you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_BeforeSendOutPokemon, 1, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_CYNTHIA, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012C
    CallTrainerBattleEnd
    VMJump L_0134

L_012C:
    WorkSetConst EVENT_WORK_0x4098, 2
    CallTrainerLose

L_0134:
    // "Cynthia: That was beyond my expectation!\nWhat an exceptional battle![f000]븁\u0000\nI love being here in spring and summer.[f000]븁\u0000\nI can't stay all year, because there's\nso much to investigate in Sinnoh, as well.[f000]븁\u0000\nYou're a great Trainer, and it would make\nme happy to see you again sometime."
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaBeyondExpectationWhat, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4098, 3
    WorkSetConst EVENT_WORK_0x400f, 1
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    WorkSetConst 0x8021, 0
    RTCGetSeason 0x8021
    VMStackPush EVENT_WORK_0x4098
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BF
    // "Cynthia: Oh.\nYou've had a chance to get ready?[f000]븁\u0000\nI do want our Pokémon to have a match...\nAre you prepared to be my opponent?"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaOhYouveHad, 1, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A9
    VMCall L_00F4
    VMJump L_01B9

L_01A9:
    // "Ha ha. You prefer to take things slowly\nand rationally, am I right?[f000]븁\u0000\nWhen you're ready, come and talk to me.\nI'll be happy to see you."
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_HaHaPreferTake, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01B9:
    VMJump L_044C

L_01BF:
    VMStackPush EVENT_WORK_0x4098
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01F8
    // "Cynthia: I come here in spring and summer\nbecause there are a lot of things[f000]븀\u0000\nto investigate in Sinnoh, as well.[f000]븁\u0000\nI'd be delighted to see you again.\nYou're an awesome Pokémon Trainer!"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaComeHereSpring, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_044C

L_01F8:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03BB
    VMStackPush EVENT_WORK_0x4166
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0abc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0254
    // "Cynthia: How terrific to see you again![f000]븁\u0000\nI've got to tell you...\nMy Pokémon are excited to battle yours.[f000]븁\u0000\nWould you care to be my opponent?"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaHowTerrificSee, 1, 0, 0
    FlagSet EVENT_FLAG_DAILY_0x0abc
    VMJump L_02CE

L_0254:
    VMStackPush EVENT_WORK_0x4166
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0abc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0289
    // "Cynthia: I can tell that my Pokémon are\nexcited about battling your Pokémon...[f000]븁\u0000\nWould you care to be my opponent?"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaCanTellPokemon, 1, 0, 0
    VMJump L_02CE

L_0289:
    VMStackPush EVENT_WORK_0x4166
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0abc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02C2
    // "Cynthia: To live their lives to the\nfullest, people and Pokémon need[f000]븀\u0000\nthe chance to throw themselves into[f000]븀\u0000\nbattle against the fiercest opposition.[f000]븁\u0000\nThat's why I want to battle you.\nHow about it?"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaLiveTheirLives, 1, 0, 0
    FlagSet EVENT_FLAG_DAILY_0x0abc
    VMJump L_02CE

L_02C2:
    // "Cynthia: Are you prepared?[f000]븁\u0000\nLet's battle at full strength and see\nhow bright our lights can shine!"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaPreparedLetsBattle, 1, 0, 0

L_02CE:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0380
    VMStackPush EVENT_WORK_0x4166
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030A
    // "Cynthia: This will be such fun!\nNo holds barred![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaWillSuchFun, 1, 0, 0
    VMJump L_0316

L_030A:
    // "Cynthia: As our Pokémon meet in battle,\nI'll learn more about you[f000]븀\u0000\nand how you've taken care of them.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaOurPokemonMeet, 1, 0, 0

L_0316:
    MsgWinCloseAll
    CallTrainerBattle TRAINER_CYNTHIA, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0343
    FlagSet EVENT_FLAG_DAILY_0x0ab9
    CallTrainerBattleEnd
    VMJump L_0345

L_0343:
    CallTrainerLose

L_0345:
    VMStackPush EVENT_WORK_0x4166
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_036A
    // "Cynthia: For me, it has really been\nworthwhile to come all the way[f000]븀\u0000\nto far Unova.[f000]븁\u0000\nWhy? Because...\nI met you, and my world got wider!"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaHasReallyBeen, 1, 0, 0
    VMJump L_0376

L_036A:
    // "Cynthia: When you meet Trainers, battle\nthem to learn about the kind of people[f000]븀\u0000\nthey are. Observe the Pokémon they[f000]븀\u0000\nchoose, which moves they taught them,[f000]븀\u0000\nand which items the Pokémon hold.[f000]븁\u0000\nYou don't need words at such times...[f000]븁\u0000\nIf you want to know more about me...\nCome to Sinnoh!"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaWhenMeetTrainers, 1, 0, 0

L_0376:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03B5

L_0380:
    VMStackPush EVENT_WORK_0x4166
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03A5
    // "Cynthia: Oh, what a pity.[f000]븁\u0000\nSummers in Undella Town make me feel like\nI'm on holiday. I forget about battling![f000]븁\u0000\nBut in spring, I feel like getting\nworked up with a good battle."
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaOhWhatPity, 1, 0, 0
    VMJump L_03B1

L_03A5:
    // "Cynthia: I'm a little disappointed.[f000]븁\u0000\nI know you can battle on bigger\nstages than you've done so far![f000]븁\u0000\nIt's so plain to me that your light can\nshine brighter than this..."
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaImLittleDisappointed, 1, 0, 0

L_03B1:
    LastKeyWait
    MsgWinCloseAll

L_03B5:
    VMJump L_044C

L_03BB:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab9
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0419
    VMStackPush EVENT_WORK_0x4166
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0403
    // "Cynthia: For me, it has really been\nworthwhile to come all the way[f000]븀\u0000\nto far Unova.[f000]븁\u0000\nWhy? Because...\nI met you, and my world got wider!"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaHasReallyBeen, 1, 0, 0
    VMJump L_040F

L_0403:
    // "Cynthia: When you meet Trainers, battle\nthem to learn about the kind of people[f000]븀\u0000\nthey are. Observe the Pokémon they[f000]븀\u0000\nchoose, which moves they taught them,[f000]븀\u0000\nand which items the Pokémon hold.[f000]븁\u0000\nYou don't need words at such times...[f000]븁\u0000\nIf you want to know more about me...\nCome to Sinnoh!"
    ActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_CynthiaWhenMeetTrainers, 1, 0, 0

L_040F:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_044C

L_0419:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044C
    WorkSetConst 0x8020, 29
    WorkAdd 0x8020, EVENT_WORK_0x4166
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose

L_044C:
    WorkSetConst 0x8021, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 19
    WorkAdd 0x8020, EVENT_WORK_0x4167
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 24
    WorkAdd 0x8020, EVENT_WORK_0x4168
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 39
    WorkAdd 0x8020, EVENT_WORK_0x416a
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 44
    WorkAdd 0x8020, EVENT_WORK_0x416b
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 34
    WorkAdd 0x8020, EVENT_WORK_0x416c
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This villa belongs to Caitlin, one of the\nPokémon League's Elite Four."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown3_Text_VillaBelongsCaitlinOne, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
