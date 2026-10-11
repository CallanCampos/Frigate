#include "asm/field_script.inc"
#include "text/script/castelia_city_19.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PokeDexIsComplete 0x8020, 3
    PokeDexIsComplete 0x8021, 1
    DebugPrint 0x8020
    DebugPrint 0x8021
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0075
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokédex.\nHow did you obtain so many Pokémon?[f000]븁\u0000\nObviously, you caught some by yourself,\nbut you can't complete the Pokédex[f000]븀\u0000\nby just catching them, right?[f000]븁\u0000\nYou probably traded Pokémon\nwith your friends and people[f000]븀\u0000\nall over the world to complete it...[f000]븁\u0000\nIf that's the case, the Pokédex is\nnot only a wealth of Pokémon information[f000]븀\u0000\nbut also a compilation of[f000]븀\u0000\nyour communication with others."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_PokedexHowDidObtain, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_019D

L_0075:
    VMStackPushFlag EVENT_FLAG_0x00dd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FF
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello!\nI am the Game Director.[f000]븁\u0000\nOh! You've caught every kind of\nPokémon in Unova![f000]븁\u0000\nIt's truly amazing!\nNow, we'll give you an award!![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_HelloAmGameDirector_2, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 0, 0
    FieldOpen
    FadeInBlackQ
    FadeWait
    // "I will send this award certificate\nto your house!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_WillSendAwardCertificate, 0, 0
    LastKeyWait
    MsgWinCloseAll
    MedalGive 44
    FlagReset EVENT_FLAG_0x02e0
    FlagSet EVENT_FLAG_0x00dd
    WorkSetConst EVENT_WORK_0x400a, 1
    VMJump L_00F9

L_00E5:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello!\nI am the Game Director.[f000]븁\u0000\nAh! You are working on your Pokédex!\nIf you fill it up a lot, please let me see!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_HelloAmGameDirector, 0, 0
    LastKeyWait
    ActorMsgClose

L_00F9:
    VMJump L_019D

L_00FF:
    VMStackPushFlag EVENT_FLAG_0x00de
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0189
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello!\nI am the Game Director.[f000]븁\u0000\nOh?[f000]븁\u0000\nBy any chance, did you...\nobtain all the Pokémon and[f000]븀\u0000\ncomplete your Pokédex?[f000]븁\u0000\nGreat.[f000]븁\u0000\nI am very happy\nthat you made great efforts[f000]븀\u0000\nto obtain so many Pokémon.[f000]븁\u0000\nPlease, please, please\nallow me to present you with this award![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_HelloAmGameDirector_3, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 1, 0
    FieldOpen
    FadeInBlackQ
    FadeWait
    // "I will send this award certificate\nto your house, too!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_WillSendAwardCertificate_2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    MedalGive 45
    FlagReset EVENT_FLAG_0x02e1
    FlagSet EVENT_FLAG_0x00de
    WorkSetConst EVENT_WORK_0x400a, 1
    VMJump L_0183

L_016F:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello!\nI am the Game Director.[f000]븁\u0000\nAh! You are working on your Pokédex!\nIf you fill it up a lot, please let me see!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_HelloAmGameDirector, 0, 0
    LastKeyWait
    ActorMsgClose

L_0183:
    VMJump L_019D

L_0189:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokédex.\nHow did you obtain so many Pokémon?[f000]븁\u0000\nObviously, you caught some by yourself,\nbut you can't complete the Pokédex[f000]븀\u0000\nby just catching them, right?[f000]븁\u0000\nYou probably traded Pokémon\nwith your friends and people[f000]븀\u0000\nall over the world to complete it...[f000]븁\u0000\nIf that's the case, the Pokédex is\nnot only a wealth of Pokémon information[f000]븀\u0000\nbut also a compilation of[f000]븀\u0000\nyour communication with others."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_PokedexHowDidObtain, 0, 0
    LastKeyWait
    ActorMsgClose

L_019D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A game is something\nto think about, program,[f000]븀\u0000\nand, at the end, hope for![f000]븁\u0000\nWork! Work!\nWork! Please work!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_GameSomethingThinkAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am the Graphic Designer.[f000]븁\u0000\nTo draw something I've never seen,\nI need to observe a lot of objects.[f000]븁\u0000\nNot only do I have to look at them,\nbut also I need to analyze them[f000]븀\u0000\nand truly absorb them."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_AmGraphicDesignerDraw, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028A
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_021D
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You think about battles very thoroughly.[f000]븁\u0000\nI lost, but I learned a lot from you.\nBesides, it was fun![f000]븁\u0000\nCome back again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_ThinkAboutBattlesVery, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0284

L_021D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A tip for getting strong\nin Pokémon battles...[f000]븁\u0000\nLet me see.\nI guess the most important thing is...[f000]븀\u0000\nhaving a lot of battles![f000]븁\u0000\nDo you want to battle?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_TipGettingStrongPokemon, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0276
    // "Well, let's begin![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_WellLetsBegin, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_GAME_FREAK_MORIMOTO_2, 0, 0
    VMCall L_046D
    // "You think about battles very thoroughly.[f000]븁\u0000\nI lost, but I learned a lot from you.\nBesides, it was fun![f000]븁\u0000\nCome back again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_ThinkAboutBattlesVery, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ab3
    VMJump L_0284

L_0276:
    // "OK. I hope we can battle next time."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_OkHopeWeCan, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0284:
    VMJump L_031E

L_028A:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You think about battles very thoroughly.[f000]븁\u0000\nI lost, but I learned a lot from you.\nBesides, it was fun![f000]븁\u0000\nCome back again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_ThinkAboutBattlesVery, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_031E

L_02B7:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh! You've become strong! I can tell.\nDo you want to have a battle with me?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_OhYouveBecomeStrong, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0310
    // "Well, let's begin![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_WellLetsBegin_2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_GAME_FREAK_MORIMOTO, 0, 0
    VMCall L_046D
    // "You think about battles very thoroughly.[f000]븁\u0000\nI lost, but I learned a lot from you.\nBesides, it was fun![f000]븁\u0000\nCome back again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_ThinkAboutBattlesVery, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ab3
    VMJump L_031E

L_0310:
    // "No?[f000]븁\u0000\nI've been raising Pokémon, thinking about\ntheir Abilities and just the right[f000]븀\u0000\ncombination of held items.[f000]븁\u0000\nWhat a pity..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_NoIveBeenRaising, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_031E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D3
    VMStackPushFlag EVENT_FLAG_DAILY_0x0aca
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0366
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Awww! What great Pokémon![f000]븁\u0000\nThe great number of steps seems to have\nincreased their trust in you...[f000]븁\u0000\nI hope we can battle again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_AwwwWhatGreatPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_03CD

L_0366:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm Snorlax.[f000]븁\u0000\nNo, no. I'm the Planner![f000]븁\u0000\nI don't mean to butt in, but the\nitem Leftovers is important, isn't it?[f000]븁\u0000\nIt's pretty useful in battle.\nDo you want to battle and test it?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_ImSnorlaxNoNo, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03BF
    // "I like to win using my favorite Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_LikeWinUsingFavorite, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_GAME_FREAK_NISHINO, 0, 0
    VMCall L_046D
    // "Awww! What great Pokémon![f000]븁\u0000\nThe great number of steps seems to have\nincreased their trust in you...[f000]븁\u0000\nI hope we can battle again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_AwwwWhatGreatPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0aca
    VMJump L_03CD

L_03BF:
    // "No?! Really?\nMy Pokémon are pretty, though..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_NoReallyPokemonPretty, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03CD:
    VMJump L_0467

L_03D3:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0aca
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0400
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Awww! What great Pokémon![f000]븁\u0000\nThe great number of steps seems to have\nincreased their trust in you...[f000]븁\u0000\nI hope we can battle again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_AwwwWhatGreatPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0467

L_0400:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm Snorlax.[f000]븁\u0000\nNo, no. I'm the Planner![f000]븁\u0000\nI don't mean to butt in, but the\nitem Leftovers is important, isn't it?[f000]븁\u0000\nIt's pretty useful in battle.\nDo you want to battle and test it?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_ImSnorlaxNoNo, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0459
    // "I like to win using my favorite Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_LikeWinUsingFavorite, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_GAME_FREAK_NISHINO_2, 0, 0
    VMCall L_046D
    // "Awww! What great Pokémon![f000]븁\u0000\nThe great number of steps seems to have\nincreased their trust in you...[f000]븁\u0000\nI hope we can battle again tomorrow."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_AwwwWhatGreatPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0aca
    VMJump L_0467

L_0459:
    // "No?! Really?\nMy Pokémon are pretty, though..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_NoReallyPokemonPretty, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0467:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_046D:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_048C
    CallTrainerBattleEnd
    VMJump L_048E

L_048C:
    CallTrainerLose

L_048E:
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm the Sound Designer. I just woke up.\nI wonder what kind of music people like."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_ImSoundDesignerJust, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello!\nThis is GAME FREAK."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_HelloGameFreak, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Now, er, I'm, er...[f000]븁\u0000\nI'm thinking, er, a new plan of, er...[f000]븁\u0000\n...Of a game."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_NowErImEr, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This place is cold because we have to\nkeep the server cool."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity19_Text_PlaceColdBecauseWe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
