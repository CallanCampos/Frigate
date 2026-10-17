#include "asm/field_script.inc"
#include "text/script/castelia_city_30.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    // "I'm working as Fennel's assistant.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_ImWorkingFennelsAssistant, 2, 0

L_003A:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00EF
    // "Is there something you would\nlike to know about researching dreams?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_ThereSomethingWouldLike, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32801
    ListMenuAdd 26, 65535, 1
    ListMenuAdd 27, 65535, 2
    ListMenuAdd 28, 65535, 3
    ListMenuShow
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_008D
    VMJump L_009D

L_008D:
    // "Game Sync is a system to collect save\nfiles from Trainers all over the world[f000]븀\u0000\nthrough Nintendo Wi-Fi Connection[f000]븀\u0000\nby making Pokémon sleep and retrieving[f000]븀\u0000\ntheir dreams.[f000]븁\u0000\nTouch the Online button on the C-Gear\nscreen, and a button called[f000]븀\u0000\n[f000][ff00]\u0001\u0001Game Sync [f000][ff00]\u0001\u0000appears.[f000]븁\u0000\nChoose a Pokémon to tuck in, then\nyou can send your save file through[f000]븀\u0000\nNintendo Wi-Fi Connection.[f000]븁\u0000\nYou should be able to make more Pokémon\nsleep as a result of the research.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_GameSyncSystemCollect, 2, 0
    VMJump L_00E9

L_009D:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_00B0
    VMJump L_00C0

L_00B0:
    // "Pokémon Dreams...[f000]븁\u0000\nIf you use Game Sync to make a\nPokémon sleep, it will have dreams.[f000]븁\u0000\nWhen you wake up the Pokémon with\nGame Sync, its dream becomes the[f000]븀\u0000\nreality in a space called the [f000][ff00]\u0001\u0001Entralink[f000][ff00]\u0001\u0000[f000]븀\u0000\nin the middle of the Unova region...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_PokemonDreamsIfUse, 2, 0
    VMJump L_00E9

L_00C0:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_00D3
    VMJump L_00E1

L_00D3:
    MsgWinCloseAll
    WorkSetConst 0x8020, 1
    VMJump L_00E9

L_00E1:
    MsgWinCloseAll
    WorkSetConst 0x8020, 1

L_00E9:
    VMJump L_003A

L_00EF:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "C-Gear Help[f000]븁\u0000\nIf you have trouble using it, touch the\n“?\" icon on the C-Gear screen!"
    SystemMsg CasteliaCity30_Text_CGearHelpIf, 2
    LastKeyWait
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "What is the Pokémon Storage System?[f000]븁\u0000\nThe person who developed the\nPokémon Storage System for the PC[f000]븀\u0000\nconnection is Bill in the Kanto region."
    SystemMsg CasteliaCity30_Text_WhatPokemonStorageSystem, 2
    LastKeyWait
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0189
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0208
    // "Hi there, Trainer!\nMy name is Fennel.[f000]븁\u0000\nI'm researching\nPokémon Trainers![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_HiThereTrainerName, 0, 0
    WorkSetConst 0x8022, 0
    PlayerGetDir 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_017D
    ActorCmdExec 0, Movement_0264
    VMJump L_01DA

L_017D:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019E
    ActorCmdExec 0, Movement_025C
    VMJump L_01DA

L_019E:
    VMStackPush 0x8022
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BF
    ActorCmdExec 0, Movement_0274
    VMJump L_01DA

L_01BF:
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DA
    ActorCmdExec 0, Movement_026C

L_01DA:
    ActorCmdWait
    // "The [f000][ff00]\u0001\u0002Game Sync[f000][ff00]\u0001\u0000 is a vital\npart of that research![f000]븁\u0000\nLet me explain the system\nfor collecting Trainers' save files.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_GameSyncVitalPart, 0, 0
    // "[f000][ff00]\u0001\u0002Game Sync [f000][ff00]\u0001\u0000is a system that retrieves\nthe memories of sleeping Pokémon![f000]븁\u0000\nThat's right! We can collect save files of\nTrainers from all over the world![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_GameSyncSystemRetrieves, 0, 0
    // "What's more, we learned that if you use\n[f000][ff00]\u0001\u0002Game Sync[f000][ff00]\u0001\u0000 to make a Pokémon sleep,[f000]븀\u0000\nit will have dreams.[f000]븁\u0000\nThen, when you wake up that Pokémon,\nits dream becomes the reality in a[f000]븀\u0000\nspace called the [f000][ff00]\u0001\u0001Entralink [f000][ff00]\u0001\u0000in the middle[f000]븀\u0000\nof the Unova region.[f000]븁\u0000\nAmazing, right?!\nIf you like, please send your save file.[f000]븁\u0000\nMy assistant can explain the details,\nso if you're interested,[f000]븀\u0000\nplease talk to her."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_WhatsMoreWeLearned, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0189
    VMJump L_0249

L_0208:
    WorkSetConst 0x8023, 0
    RecordGet 119, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023B
    // "Fennel: [f000][ff00]\u0001\u0002Game Sync[f000][ff00]\u0001\u0000 is amazing, right?!\nIf you like, please send your save file.[f000]븁\u0000\nMy assistant can explain the details,\nso if you're interested,[f000]븀\u0000\nplease talk to her."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_FennelGameSyncAmazing, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0249

L_023B:
    // "Fennel: Oh, thank you![f000]븁\u0000\nYou also sent your save file. I saw it\nwhile I was researching Trainers![f000]븁\u0000\n[f000][ff00]\u0001\u0002Game Sync[f000][ff00]\u0001\u0000 is amazing, right?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_FennelOhThankAlso, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0249:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_025C:
    Move 48, 1
    MoveEnd

Movement_0264:
    Move 49, 1
    MoveEnd

Movement_026C:
    Move 50, 1
    MoveEnd

Movement_0274:
    Move 51, 1
    MoveEnd

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 0
    VMStackPushFlag EVENT_FLAG_0x096b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C5
    // "Are you a Trainer?[f000]븁\u0000\nDo you use the PC at Pokémon Centers?[f000]븁\u0000\nI am Amanita. I maintain the Box system.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_TrainerUsePcPokemon, 1, 2, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0540
    ActorCmdWait
    // "Oh!\nYou have a Pal Pad, I see![f000]븁\u0000\nYou can register your friends in your\nPal Pad.[f000]븁\u0000\nAfter you register, you can link with\nthose friends over Nintendo Wi-Fi[f000]븀\u0000\nConnection to do all kinds of fun things![f000]븁\u0000\nYou can trade Pokémon, challenge\nyour friends to a battle, and so on.[f000]븁\u0000\nLet me give you a quick how-to on\nregistering your friends.[f000]븁\u0000\nYou can either input your friend's code\ndirectly by using your Pal Pad...[f000]븁\u0000\nOr you can use the IR Connection\nfeature of the C-Gear.[f000]븁\u0000\nThen, you can register your friend![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_OhHavePalPad, 1, 2, 0
    FlagSet EVENT_FLAG_0x096b

L_02C5:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0414
    VMStackPushFlag EVENT_FLAG_0x018a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_031E
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0312
    // "Eevee is an amazing Pokémon\nthat has many potential evolutions!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_EeveeAmazingPokemonHas, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0318

L_0312:
    VMCall L_0420

L_0318:
    VMJump L_040E

L_031E:
    // "Oh! How are the Boxes working?[f000]븁\u0000\nHey, that's right!\nI have a bunch of Eevee![f000]븀\u0000\nWould you take one for me?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_OhHowBoxesWorking, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FC
    WorkSetConst 0x8025, 0
    PokePartyGetCount 0x8025, 0
    VMStackPush 0x8025
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0372
    // "Oh! Your party is full![f000]븁\u0000\nThe PC Boxes were designed for\nsituations just like this!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_OhPartyFullPc, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03F6

L_0372:
    // "This is an Eevee I received from\nmy friend in the Kanto region![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_EeveeReceivedFromFriend, 0, 0
    MsgWinCloseAll
    PokePartyAddEx 0x8010, 133, 0, 10, 3, 0, 0, 0, 4
    WordSetPlayerName 0
    MEPlay SEQ_ME_POKEGET
    // "[f000]Ā\u0001\u0000 received\nan Eevee!"
    SystemMsg CasteliaCity30_Text_ReceivedEevee, 0
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    // "Would you like to give a\nnickname to this Eevee?"
    SystemMsg CasteliaCity30_Text_WouldLikeGiveNickname, 0
    WorkSetConst 0x8026, 0
    YesNoWin 0x8026
    InfoMsgClose
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DE
    WorkSetConst 0x8027, 0
    CallPokeNameInput 0x8027, 0x8025, 1
    VMJump L_03DE

L_03DE:
    // "Eevee is an amazing Pokémon\nthat has many potential evolutions!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_EeveeAmazingPokemonHas, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x018a
    WorkSetConst EVENT_WORK_0x400a, 1

L_03F6:
    VMJump L_040E

L_03FC:
    DebugPrint 0x8010
    // "How disappointing.[f000]븁\u0000\nWell, if you change your mind,\nplease come talk to me again!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_HowDisappointingWellIf, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_040E:
    VMJump L_041A

L_0414:
    VMCall L_0420

L_041A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0420:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0

L_0432:
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_053C
    // "Amanita: Is there something you'd like\nto ask me?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_AmanitaThereSomethingYoud, 1, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32809
    ListMenuAdd 9, 65535, 1
    ListMenuAdd 10, 65535, 2
    ListMenuAdd 11, 65535, 3
    ListMenuAdd 12, 65535, 4
    ListMenuShow
    WorkCmpConst 0x8029, 1
    VMJumpIf CMP_EQ, L_048F
    VMJump L_04A3

L_048F:
    // "You can store up to 30 Pokémon\nyou caught in one Box.[f000]븁\u0000\nAt first, there are only eight Boxes,\nbut as you store more Pokémon,[f000]븀\u0000\nthe number of Boxes increases!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_CanStoreUp30, 1, 2, 0
    MsgWaitAdvance
    VMJump L_0536

L_04A3:
    WorkCmpConst 0x8029, 2
    VMJumpIf CMP_EQ, L_04B6
    VMJump L_04CA

L_04B6:
    // "In the Battle Box, you can register one\nto six Pokémon that you often use[f000]븀\u0000\nin battles.[f000]븁\u0000\nWhen you battle using Infrared\nConnection, you can also battle with[f000]븀\u0000\nthe Pokémon in the Battle Box![f000]븁\u0000\nIt's convenient because you don't\nhave to move Pokémon around!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_BattleBoxCanRegister, 1, 2, 0
    MsgWaitAdvance
    VMJump L_0536

L_04CA:
    WorkCmpConst 0x8029, 3
    VMJumpIf CMP_EQ, L_04DD
    VMJump L_04F1

L_04DD:
    // "In order to register your friend\nin your Pal Pad...[f000]븁\u0000\nYou can either input your friend's code\ndirectly by using your Pal Pad in the[f000]븀\u0000\nKey Item case...[f000]븁\u0000\nOr you can use the IR Connection\nfeature of the C-Gear.[f000]븁\u0000\nThen, you can register your friend!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_OrderRegisterFriendPal, 1, 2, 0
    MsgWaitAdvance
    VMJump L_0536

L_04F1:
    WorkCmpConst 0x8029, 4
    VMJumpIf CMP_EQ, L_0504
    VMJump L_0520

L_0504:
    // "Catch a lot of Pokémon,\nand use the Boxes a lot!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_CatchLotPokemonUse, 1, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8028, 1
    VMJump L_0536

L_0520:
    // "Catch a lot of Pokémon,\nand use the Boxes a lot!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity30_Text_CatchLotPokemonUse, 1, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8028, 1

L_0536:
    VMJump L_0432

L_053C:
    VMReturn
    .balign 4, 0

Movement_0540:
    Move 75, 1
    MoveEnd
