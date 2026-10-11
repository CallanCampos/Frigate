#include "asm/field_script.inc"
#include "text/script/pokestar_studios_3.h"

// Script plugin 10, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8021, 0

L_003C:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01C7
    // "Hey! Want to know anything\nabout Pokéstar Studios?"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_HeyWantKnowAnything, 3, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 23, 65535, 0
    ListMenuAdd 24, 65535, 1
    ListMenuAdd 29, 65535, 2
    ListMenuAdd 25, 65535, 3
    ListMenuAdd 26, 65535, 4
    ListMenuAdd 27, 65535, 5
    ListMenuAdd 28, 65535, 6
    ListMenuShow
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_00B1
    VMJump L_00C3

L_00B1:
    // "Pokéstar Studios films are shot\nwhile following a script![f000]븁\u0000\nIn other words, the script\nis the foundation the rest[f000]븀\u0000\nof the movie is built on![f000]븁\u0000\nDirections for making\nfilming work and information[f000]븀\u0000\nabout the other actors is[f000]븀\u0000\nalso written in the script![f000]븁\u0000\nYou can look at it while shooting,\nso check it if you're in trouble![f000]븁\u0000\nThe scripts you can shoot increase\ndepending on things like the[f000]븀\u0000\ncontent of the movies you've made[f000]븀\u0000\nor how much they've grossed![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_PokestarStudiosFilmsShot, 3, 4, 0
    VMJump L_01C1

L_00C3:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_00D6
    VMJump L_00E8

L_00D6:
    // "Pokémon are an indispensable part\nof moviemaking at Pokéstar Studios![f000]븁\u0000\nThe actor and the Pokémon's acting\nare what make a shoot proceed.[f000]븁\u0000\nAt first we have you rent Pokémon\nfor each different script.[f000]븁\u0000\nOnce you can make a good film,\nyou'll be able to shoot it again[f000]븀\u0000\nwith your own Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_PokemonIndispensablePartMoviemaking, 3, 4, 0
    VMJump L_01C1

L_00E8:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_00FB
    VMJump L_010D

L_00FB:
    // "At Pokéstar Studios, any Pokémon\ncan participate in the filming![f000]븁\u0000\nBut you need to pay attention\nto the moves it knows.[f000]븁\u0000\nIf a Pokémon knows any of these moves,\nit can't be in a movie:[f000]븀\u0000\nTransform,[f000]븀\u0000\nTorment,[f000]븀\u0000\nor Metronome.[f000]븁\u0000\nThese moves get in your costars' way,\nso they're banned at Pokéstar Studios.[f000]븁\u0000\nKeep this in mind when filming\nwith your own Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_PokestarStudiosAnyPokemon, 3, 4, 0
    VMJump L_01C1

L_010D:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_0120
    VMJump L_0132

L_0120:
    // "The basics of Pokéstar Studios film\nacting resemble Pokémon battling.[f000]븁\u0000\nBut it's not a battle, so winning\nisn't always important![f000]븁\u0000\nHints about what to do are written\nin the script, so check it carefully.[f000]븁\u0000\nThe director's comments can also\nbe helpful![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_BasicsPokestarStudiosFilm, 3, 4, 0
    VMJump L_01C1

L_0132:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_0145
    VMJump L_0157

L_0145:
    // "As the shoot goes on,\nyou can pick lines![f000]븁\u0000\nChoose the line that makes you\nthink, “This is great!\"[f000]븁\u0000\nThe lines you choose will change\nyour costars' acting and the story[f000]븀\u0000\nof the final film itself![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_ShootGoesCanPick, 3, 4, 0
    VMJump L_01C1

L_0157:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_016A
    VMJump L_017C

L_016A:
    // "As you make movies and they\nare played in the theater,[f000]븀\u0000\nyou'll become popular![f000]븁\u0000\nIf you become popular,\nI'm sure some good things will happen![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_MakeMoviesTheyPlayed, 3, 4, 0
    VMJump L_01C1

L_017C:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_018F
    VMJump L_01AB

L_018F:
    // "OK! Do your best!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_OkBest, 3, 4, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8021, 1
    VMJump L_01C1

L_01AB:
    // "OK! Do your best!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_OkBest, 3, 4, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8021, 1

L_01C1:
    VMJump L_003C

L_01C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WordSetPlayerName 0
    VMStackPushFlag EVENT_FLAG_0x01ca
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FF
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Good day!\nMr. Deeoh told me about it![f000]븁\u0000\n[f000]Ā\u0001\u0000, please,\nenter the dressing room,[f000]븀\u0000\nand relax to your heart's content."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_GoodDayMrDeeoh, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0213

L_01FF:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Only special stars approved\nby our boss are allowed into[f000]븀\u0000\nthis special dressing room.[f000]븁\u0000\nA brat like you has no business here.\nScram! Get out of here!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_OnlySpecialStarsApproved, 0, 0
    LastKeyWait
    ActorMsgClose

L_0213:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    Cmd_02CB EVENT_WORK_0x400f
    WordSetPlayerName 0
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Whaugh! [f000]Ā\u0001\u0000![f000]븁\u0000\nThanks as always for your hard work!\nI'm putting my whole heart and soul[f000]븀\u0000\ninto my guard duty and making sure[f000]븀\u0000\nnot even a single Joltik will get through."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_WhaughThanksAlwaysHard, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0263

L_024F:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Heh heh heh...[f000]븁\u0000\nYou see, this is the dressing room\nfor an amazing beauty scouted[f000]븀\u0000\nfrom the something-or-other region.[f000]븁\u0000\nI wonder what's going on inside..."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_HehHehHehSee, 0, 0
    LastKeyWait
    ActorMsgClose

L_0263:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Pokéstar Studios movies are shot\nusing rental Pokémon, right?[f000]븁\u0000\nDoes this mean my little\nLillipup can't be in the movies?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_PokestarStudiosMoviesShot, 5, 5, 0
    MsgWinCloseAll
    // "If you shoot a movie with\nrental Pokémon once successfully,[f000]븀\u0000\nthen you can use your own Pokémon![f000]븁\u0000\nThat's why me and my Stunfisk\nare shooting for the stars![f000]븁\u0000\nRight, Stunfisk?"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_IfShootMovieRental, 6, 4, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 618, 0
    // "..."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_Empty, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WordSetPlayerName 0
    Cmd_02CB EVENT_WORK_0x400f
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8022, 7
    WorkAdd 0x8022, EVENT_WORK_0x400f
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8022, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WordSetPlayerName 0
    Cmd_02CB EVENT_WORK_0x400f
    WorkSetConst 0x8023, 0
    WorkCmpConst EVENT_WORK_0x400f, 0
    VMJumpIf CMP_EQ, L_0314
    VMJump L_0320

L_0314:
    WorkSetConst 0x8023, 12
    VMJump L_0378

L_0320:
    WorkCmpConst EVENT_WORK_0x400f, 1
    VMJumpIf CMP_EQ, L_034D
    WorkCmpConst EVENT_WORK_0x400f, 2
    VMJumpIf CMP_EQ, L_034D
    WorkCmpConst EVENT_WORK_0x400f, 3
    VMJumpIf CMP_EQ, L_034D
    VMJump L_0359

L_034D:
    WorkSetConst 0x8023, 13
    VMJump L_0378

L_0359:
    WorkCmpConst EVENT_WORK_0x400f, 4
    VMJumpIf CMP_EQ, L_036C
    VMJump L_0378

L_036C:
    WorkSetConst 0x8023, 14
    VMJump L_0378

L_0378:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8023, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm the VFX specialist![f000]븁\u0000\nYou've seen those green screens\nin the soundstage, right?[f000]븁\u0000\nImages filmed in front of them\nare turned into impressive[f000]븀\u0000\nmovies using computers!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios3_Text_ImVfxSpecialistYouve, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
