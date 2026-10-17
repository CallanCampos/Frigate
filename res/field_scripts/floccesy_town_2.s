#include "asm/field_script.inc"
#include "text/script/floccesy_town_2.h"

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

Script_1:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40a6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0057
    ActorSetGPos 2, 6, 0, 11, 0

L_0057:
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 2, Movement_02DC
    ActorCmdExec 255, Movement_02EC
    ActorCmdWait
    ActorCmdExec 1, Movement_02F4
    ActorCmdExec 0, Movement_02F4
    ActorCmdWait
    // "Girl: Oh! Are we going to help\ntrain that person, sir?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_GirlOhWeGoing, 0, 0, 0
    MsgWinCloseAll
    // "Boy: Really? But that Trainer\nlooks really tough![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_BoyReallyButTrainer, 1, 0, 0
    MsgWinCloseAll
    // "Alder: That's right! This Trainer may\nbe tough, but you can learn from[f000]븀\u0000\nlosing as well.[f000]븁\u0000\nMore importantly, haven't I been telling\nyou just to enjoy Pokémon battles?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderThatsRightTrainer, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0C9C
    ActorCmdExec 255, Movement_0CA4
    ActorCmdWait
    WordSetPlayerName 0
    // "So, [f000]Ā\u0001\u0000!\nPlease be their opponent![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_PleaseTheirOpponent, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 1, 6, 9, 1, 8, 0
    ActorCmdExec 2, Movement_0C8C
    ActorCmdExec 255, Movement_0C8C
    ActorCmdWait
    // "I'll show you what's\ncool about my Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_IllShowWhatsCool, 1, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011B
    CallTrainerBattle TRAINER_SCHOOL_KID_SEYMOUR, 0, 0
    VMJump L_0144

L_011B:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013C
    CallTrainerBattle TRAINER_SCHOOL_KID_SEYMOUR_2, 0, 0
    VMJump L_0144

L_013C:
    CallTrainerBattle TRAINER_SCHOOL_KID_SEYMOUR_3, 0, 0

L_0144:
    VMCall L_02B6
    ActorCmdExec 1, Movement_0300
    // "That was a fine battle,\nboth of you.[f000]븁\u0000\nWell, next we have...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_FineBattleBothWell, 2, 0, 0
    MsgWinCloseAll
    ActorCmdWait
    // "Ready![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_Ready, 0, 0, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    // "Alder: First we have to heal\nyour Pokémon, [f000]Ā\u0001\u0000.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderFirstWeHave, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0C9C
    ActorCmdExec 255, Movement_0CA4
    ActorCmdWait
    SEPlay SEQ_SE_RECOVERY
    SEWait
    PokePartyRecoverAll
    ActorCmdExec 2, Movement_0C8C
    ActorCmdExec 255, Movement_0C8C
    ActorWalkRoute 0, 6, 9, 1, 8, 0
    ActorCmdWait
    // "Girl: Some Pokémon battles are\ndecided by type matchups![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_GirlSomePokemonBattles, 0, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01EA
    CallTrainerBattle TRAINER_SCHOOL_KID_CASSIE, 0, 0
    VMJump L_0213

L_01EA:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020B
    CallTrainerBattle TRAINER_SCHOOL_KID_CASSIE_2, 0, 0
    VMJump L_0213

L_020B:
    CallTrainerBattle TRAINER_SCHOOL_KID_CASSIE_3, 0, 0

L_0213:
    VMCall L_02B6
    // "Alder: That was truly\na rousing battle![f000]븁\u0000\nI could tell that all of the Pokémon\nwere enjoying themselves as well![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderTrulyRousingBattle, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0C9C
    ActorCmdExec 255, Movement_0CA4
    ActorCmdExec 0, Movement_0310
    ActorCmdWait
    WordSetPlayerName 0
    // "How about it, [f000]Ā\u0001\u0000?[f000]븁\u0000\nPokémon types are very important\nin battle, aren't they?[f000]븁\u0000\nWater is strong against Fire...\nFire is strong against Grass...[f000]븀\u0000\nGrass is strong against Water...[f000]븁\u0000\nType matchups don't\ndecide everything, though![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_HowAboutPokemonTypes, 2, 0, 0
    MsgWinCloseAll
    // "But, listen![f000]븁\u0000\nWhen a Pokémon uses a move\nthat matches its type, the move[f000]븀\u0000\nbecomes more powerful![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_ButListenWhenPokemon, 0, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8024, 0
    PokePartyGetMemberByType 0x8024, 2
    WordSetPartyPokeSpecies 1, 0x8024
    WordSetPlayerName 0
    // "Let's heal those hard-working Pokémon![f000]븁\u0000\n[f000]ā\u0001\u0001!\nYou did a great job for [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_LetsHealThoseHard, 2, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_RECOVERY
    SEWait
    PokePartyRecoverAll
    // "Meeting Pokémon and people you\nnever would have met otherwise[f000]븀\u0000\nis truly one of the great things[f000]븀\u0000\nabout traveling!"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_MeetingPokemonPeopleNever, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40a6, 1
    WorkSetConst EVENT_WORK_0x40a5, 5
    FlagReset EVENT_FLAG_0x02da
    WorkSetConst EVENT_WORK_0x40a3, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_02B6:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D5
    CallTrainerBattleEnd
    VMJump L_02D7

L_02D5:
    CallTrainerLose

L_02D7:
    VMReturn
    .balign 4, 0

Movement_02DC:
    Move 12, 1
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_02EC:
    Move 12, 2
    MoveEnd

Movement_02F4:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_0300:
    Move 15, 1
    Move 12, 1
    Move 33, 1
    MoveEnd

Movement_0310:
    Move 14, 1
    Move 12, 1
    Move 33, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    KeysCmd_02D1 0x8023
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C8
    VMStackPushFlag EVENT_FLAG_0x01ac
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0358
    VMCall L_0452
    VMJump L_03C2

L_0358:
    VMStackPushFlag EVENT_FLAG_0x01ac
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0399
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Alder: He's a lively one,\neven for MY grandson![f000]븁\u0000\nI'll explain what he was\ntalking about.[f000]븁\u0000\nHe challenged a place called\nthe White Treehollow, which[f000]븀\u0000\nappeared in White Forest.[f000]븁\u0000\nYou'll find out what kind of\nplace it is if you go there.[f000]븁\u0000\nAn ordinary Trainer, however,\ncan't make it to where he is.[f000]븁\u0000\nSo that's the story!\nIf you'd like, you should take[f000]븀\u0000\nthe challenge as well."
    // "Alder: He's a lively one,\neven for MY grandson![f000]븁\u0000\nI'll explain what he was\ntalking about.[f000]븁\u0000\nHe challenged a place called\nthe Black Tower, which[f000]븀\u0000\nappeared in Black City.[f000]븁\u0000\nYou'll find out what kind of\nplace it is if you go there.[f000]븁\u0000\nAn ordinary Trainer, however,\ncan't make it to where he is.[f000]븁\u0000\nSo that's the story!\nIf you'd like, you should take[f000]븀\u0000\nthe challenge as well."
    ActorMsgVersioned 1024, FloccesyTown2_Text_AlderHesLivelyOne_2, FloccesyTown2_Text_AlderHesLivelyOne, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03C2

L_0399:
    VMStackPush 0x8023
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_03C2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Alder: When you find something\nyou want to do, you should[f000]븀\u0000\ntake it on without a moment of doubt![f000]븁\u0000\nDon't worry!\nYou have Pokémon by your side, right?[f000]븁\u0000\nIf you're together,\nyou can do things you can't do alone,[f000]븀\u0000\nand your Pokémon can go places[f000]븀\u0000\nthey couldn't go on their own."
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderWhenFindSomething, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03C2:
    VMJump L_044C

L_03C8:
    TrainerCardHasBadge 0x8008, 0
    WorkSetConst 0x8025, 0
    TrainerCardGetBadgeCount 0x8025
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0405
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Meeting Pokémon and people you\nnever would have met otherwise[f000]븀\u0000\nis truly one of the great things[f000]븀\u0000\nabout traveling!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_MeetingPokemonPeopleNever, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0446

L_0405:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0432
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Alder: Getting that many\nGym Badges is impressive![f000]븁\u0000\nBut you're only partway\nthrough your journey...[f000]븁\u0000\nWhat does being strong\nreally mean?"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderGettingManyGym, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0446

L_0432:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Alder: Oh!\nYou won a Gym Badge![f000]븁\u0000\nThat's the result of\nunderstanding your Pokémon[f000]븀\u0000\nand bringing out their power!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderOhWonGym, 0, 0
    LastKeyWait
    ActorMsgClose

L_0446:
    WorkSetConst 0x8025, 0

L_044C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0452:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Cmd_02D5 19, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0483
    // "Alder: Oh! You've come![f000]븁\u0000\nI want to show them what's\nincredible about Trainers as well![f000]븁\u0000\nCould I spar with you,\nthe strongest Trainer[f000]븀\u0000\nin the Unova region?"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderOhYouveCome_2, 2, 0, 0
    VMJump L_048F

L_0483:
    // "Alder: Oh! You've come![f000]븁\u0000\nCould I spar with you,\nthe strongest Trainer[f000]븀\u0000\nin the Unova region?"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderOhYouveCome, 2, 0, 0

L_048F:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A60
    MsgWinCloseAll
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_04C9
    ActorCmdExec 2, Movement_0C94
    ActorCmdWait

L_04C9:
    ActorCmdExec 2, Movement_0C64
    ActorCmdWait
    VMSleep 8
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04FA
    ActorCmdExec 2, Movement_0C8C
    ActorCmdWait
    VMJump L_053A

L_04FA:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_051D
    ActorCmdExec 2, Movement_0CA4
    ActorCmdWait
    VMJump L_053A

L_051D:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_053A
    ActorCmdExec 2, Movement_0C9C
    ActorCmdWait

L_053A:
    // "Oh!\nMy heart jumps for joy![f000]븁\u0000\nWell, then, prepare yourself\nfor battle![f000]븁\u0000\nKiai![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_OhHeartJumpsJoy, 2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_ALDER, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_056F
    CallTrainerBattleEnd
    VMJump L_0571

L_056F:
    CallTrainerLose

L_0571:
    // "Alder: That's the Champion for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderThatsChampion, 2, 0, 0
    MsgWinCloseAll
    VMSleep 16
    SEPlay SEQ_SE_KAIDAN
    ActorNew 6, 12, 0, 251, 304, 0
    SEWait
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_05B6
    ActorCmdExec 2, Movement_0C94

L_05B6:
    ActorWalkRoute 251, 6, 6, 1, 4, 1
    ActorCmdWait
    WorkSetConst 0x8026, 0
    PlayerGetDir 0x8020
    GameGetVersion 0x8026
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush 0x8026
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0602
    // "???: Gramps![f000]븁\u0000\nI did it![f000]븁\u0000\nI did it![f000]븁\u0000\nI DID it![f000]븁\u0000\nThe Black Tower![f000]븁\u0000\nI made it to the top![f000]븁\u0000"
    InfoMsg FloccesyTown2_Text_GrampsDidDidDid, 2
    VMJump L_063E

L_0602:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush 0x8026
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0630
    // "???: Gramps![f000]븁\u0000\nI did it![f000]븁\u0000\nI did it![f000]븁\u0000\nI DID it![f000]븁\u0000\nThe White Treehollow![f000]븁\u0000\nI made it to the deepest part![f000]븁\u0000"
    InfoMsg FloccesyTown2_Text_GrampsDidDidDid_2, 2
    VMJump L_063E

L_0630:
    // "???: Gramps![f000]븁\u0000\nI did it![f000]븁\u0000\nI did it![f000]븁\u0000\nI DID it![f000]븁\u0000\nThe White Treehollow![f000]븁\u0000\nI made it to the deepest part![f000]븁\u0000"
    // "???: Gramps![f000]븁\u0000\nI did it![f000]븁\u0000\nI did it![f000]븁\u0000\nI DID it![f000]븁\u0000\nThe Black Tower![f000]븁\u0000\nI made it to the top![f000]븁\u0000"
    ActorMsgVersioned 1024, FloccesyTown2_Text_GrampsDidDidDid_2, FloccesyTown2_Text_GrampsDidDidDid, 251, 4, 0

L_063E:
    MsgWinCloseAll
    WorkSetConst 0x8026, 0
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0685
    ActorWalkRoute 255, 5, 3, 1, 8, 0
    ActorCmdWait
    VMJump L_068D

L_0685:
    ActorCmdExec 255, Movement_0C94

L_068D:
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06CA
    // "Alder: Why, if it isn't Benga![f000]븁\u0000\nAre you serious, boy?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderWhyIfIsnt, 2, 5, 0
    VMJump L_06D6

L_06CA:
    // "Alder: Why, if it isn't Benga![f000]븁\u0000\nAre you serious, boy?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderWhyIfIsnt, 2, 3, 0

L_06D6:
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_06F6
    // "Benga: Hey![f000]븁\u0000\nGramps![f000]븁\u0000\nYou know how\nstrong I am![f000]븁\u0000\nHey, hey![f000]븁\u0000"
    InfoMsg FloccesyTown2_Text_BengaHeyGrampsKnow, 2
    VMJump L_0702

L_06F6:
    // "Benga: Hey![f000]븁\u0000\nGramps![f000]븁\u0000\nYou know how\nstrong I am![f000]븁\u0000\nHey, hey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_BengaHeyGrampsKnow, 251, 4, 0

L_0702:
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0741
    ActorWalkRoute 251, 6, 5, 1, 8, 1
    VMJump L_07BD

L_0741:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0782
    ActorWalkRoute 251, 7, 5, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 251, Movement_0C8C
    VMJump L_07BD

L_0782:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07BD
    ActorWalkRoute 251, 5, 5, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 251, Movement_0C8C

L_07BD:
    ActorCmdWait
    ActorCmdExec 251, Movement_0CB4
    ActorCmdWait
    // "Who's that?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_Whos, 251, 4, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    WorkSetConst 0x8027, 0
    TrainerCardGetSex 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_083E
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_082C
    // "Alder: His name is\n[f000]Ā\u0001\u0000![f000]븁\u0000\nHe's the strongest\nTrainer in the Unova region.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderHisNameHes, 2, 5, 0
    VMJump L_0838

L_082C:
    // "Alder: His name is\n[f000]Ā\u0001\u0000![f000]븁\u0000\nHe's the strongest\nTrainer in the Unova region.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderHisNameHes, 2, 3, 0

L_0838:
    VMJump L_087F

L_083E:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0873
    // "Alder: Her name is\n[f000]Ā\u0001\u0000![f000]븁\u0000\nShe's the strongest\nTrainer in the Unova region.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderHerNameShes, 2, 5, 0
    VMJump L_087F

L_0873:
    // "Alder: Her name is\n[f000]Ā\u0001\u0000![f000]븁\u0000\nShe's the strongest\nTrainer in the Unova region.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderHerNameShes, 2, 3, 0

L_087F:
    MsgWinCloseAll
    // "Benga: I knew it![f000]븁\u0000\nThat Trainer smells tough![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_BengaKnewTrainerSmells, 251, 4, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_08C4
    // "Alder: How about it, Benga?\nDo you want to spar, perhaps?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderHowAboutBenga, 2, 5, 0
    VMJump L_08D0

L_08C4:
    // "Alder: How about it, Benga?\nDo you want to spar, perhaps?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_AlderHowAboutBenga, 2, 3, 0

L_08D0:
    MsgWinCloseAll
    // "Benga: No![f000]븁\u0000\nI know that Trainer's strong![f000]븁\u0000\nAnd I want to battle![f000]븁\u0000\nBut those Pokémon\nhaven't been through enough![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_BengaNoKnowTrainers, 251, 4, 0
    // "Me![f000]븁\u0000\nAnd my Pokémon![f000]븁\u0000\nWe made it to the deepest part\nof the White Treehollow![f000]븁\u0000\nYou do that, too![f000]븁\u0000\nThere we'll see who's stronger![f000]븁\u0000\nLet's try hard to be the best![f000]븁\u0000"
    // "Me![f000]븁\u0000\nAnd my Pokémon![f000]븁\u0000\nWe climbed to the top of the Black Tower![f000]븁\u0000\nYou do that, too![f000]븁\u0000\nThere we'll see who's stronger![f000]븁\u0000\nLet's try hard to be the best![f000]븁\u0000"
    ActorMsgVersioned 1024, FloccesyTown2_Text_PokemonWeMadeDeepest, FloccesyTown2_Text_PokemonWeClimbedTop, 251, 4, 0
    WordSetPlayerName 0
    // "Later![f000]븁\u0000\nGramps and [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_LaterGramps, 251, 4, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_093A
    ActorWalkRoute 251, 6, 12, 1, 4, 1
    VMJump L_0958

L_093A:
    ActorWalkRoute 251, 6, 6, 1, 4, 1
    ActorCmdWait
    ActorWalkRoute 251, 6, 12, 1, 4, 1

L_0958:
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_099B
    ActorCmdExec 255, Movement_0C8C
    VMJump L_0A07

L_099B:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_09D4
    ActorCmdExec 2, Movement_0CA4
    ActorCmdExec 255, Movement_0C9C
    VMJump L_0A07

L_09D4:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0A07
    ActorCmdExec 2, Movement_0C9C
    ActorCmdExec 255, Movement_0CA4

L_0A07:
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0A40
    // "Alder: He's a lively one,\neven for MY grandson![f000]븁\u0000\nI'll explain what he was\ntalking about.[f000]븁\u0000\nHe challenged a place called\nthe White Treehollow, which[f000]븀\u0000\nappeared in White Forest.[f000]븁\u0000\nYou'll find out what kind of\nplace it is if you go there.[f000]븁\u0000\nAn ordinary Trainer, however,\ncan't make it to where he is.[f000]븁\u0000\nSo that's the story!\nIf you'd like, you should take[f000]븀\u0000\nthe challenge as well."
    // "Alder: He's a lively one,\neven for MY grandson![f000]븁\u0000\nI'll explain what he was\ntalking about.[f000]븁\u0000\nHe challenged a place called\nthe Black Tower, which[f000]븀\u0000\nappeared in Black City.[f000]븁\u0000\nYou'll find out what kind of\nplace it is if you go there.[f000]븁\u0000\nAn ordinary Trainer, however,\ncan't make it to where he is.[f000]븁\u0000\nSo that's the story!\nIf you'd like, you should take[f000]븀\u0000\nthe challenge as well."
    ActorMsgVersioned 1024, FloccesyTown2_Text_AlderHesLivelyOne_2, FloccesyTown2_Text_AlderHesLivelyOne, 2, 5, 0
    VMJump L_0A4E

L_0A40:
    // "Alder: He's a lively one,\neven for MY grandson![f000]븁\u0000\nI'll explain what he was\ntalking about.[f000]븁\u0000\nHe challenged a place called\nthe White Treehollow, which[f000]븀\u0000\nappeared in White Forest.[f000]븁\u0000\nYou'll find out what kind of\nplace it is if you go there.[f000]븁\u0000\nAn ordinary Trainer, however,\ncan't make it to where he is.[f000]븁\u0000\nSo that's the story!\nIf you'd like, you should take[f000]븀\u0000\nthe challenge as well."
    // "Alder: He's a lively one,\neven for MY grandson![f000]븁\u0000\nI'll explain what he was\ntalking about.[f000]븁\u0000\nHe challenged a place called\nthe Black Tower, which[f000]븀\u0000\nappeared in Black City.[f000]븁\u0000\nYou'll find out what kind of\nplace it is if you go there.[f000]븁\u0000\nAn ordinary Trainer, however,\ncan't make it to where he is.[f000]븁\u0000\nSo that's the story!\nIf you'd like, you should take[f000]븀\u0000\nthe challenge as well."
    ActorMsgVersioned 1024, FloccesyTown2_Text_AlderHesLivelyOne_2, FloccesyTown2_Text_AlderHesLivelyOne, 2, 3, 0

L_0A4E:
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01ac
    FlagSet EVENT_FLAG_0x02ee
    VMJump L_0A70

L_0A60:
    // "Hrm...\nWell, I shall wait here, then."
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_HrmWellShallWait, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0A70:
    VMReturn

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    // "Hey![f000]븁\u0000\nHey![f000]븁\u0000\nHey![f000]븁\u0000\nIt was a promise![f000]븁\u0000\nBetween me and you![f000]븁\u0000\nWe had the best match![f000]븁\u0000\nThis is to remember it by![f000]븁\u0000\nRaise this Dratini![f000]븁\u0000"
    // "Hey![f000]븁\u0000\nHey![f000]븁\u0000\nHey![f000]븁\u0000\nIt was a promise![f000]븁\u0000\nBetween me and you![f000]븁\u0000\nWe had the best match![f000]븁\u0000\nThis is to remember it by![f000]븁\u0000\nRaise this Gible![f000]븁\u0000"
    ActorMsgVersioned 1024, FloccesyTown2_Text_HeyHeyHeyPromise_2, FloccesyTown2_Text_HeyHeyHeyPromise, 3, 0, 0
    MsgWinCloseAll
    GameGetVersion 0x8028
    VMStackPush 0x8028
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ACF
    PokePartyAddEx 0x8029, 443, 0, 1, 0, 0, 1, 216, 4
    WordSetPokeSpecies 1, 443
    VMJump L_0AE8

L_0ACF:
    PokePartyAddEx 0x8029, 147, 0, 1, 0, 0, 1, 216, 4
    WordSetPokeSpecies 1, 147

L_0AE8:
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BCB
    WordSetPlayerName 0
    MEPlay SEQ_ME_POKEGET
    // "[f000]Ā\u0001\u0000 received\n[f000]ā\u0001\u0001!"
    SystemMsg FloccesyTown2_Text_Received, 0
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    // "Would you like to give a\nnickname to this [f000]ā\u0001\u0001?"
    SystemMsg FloccesyTown2_Text_WouldLikeGiveNickname, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    YesNoWin 0x802a
    InfoMsgClose
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B59
    WorkSetConst 0x802c, 0
    PokePartyGetCount 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSub 0x802c, 1
    CallPokeNameInput 0x802b, 0x802c, 1

L_0B59:
    // "I'll leave it with you![f000]븁\u0000\nTake good care of it![f000]븁\u0000\n[f000]Ā\u0001\u0000![f000]븁\u0000\nI'll be waiting for you\nin the White Treehollow[f000]븁\u0000"
    // "I'll leave it with you![f000]븁\u0000\nTake good care of it![f000]븁\u0000\n[f000]Ā\u0001\u0000![f000]븁\u0000\nI'll be waiting for you in the Black Tower![f000]븁\u0000"
    ActorMsgVersioned 1024, FloccesyTown2_Text_IllLeaveTakeGood_2, FloccesyTown2_Text_IllLeaveTakeGood, 3, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B8E
    ActorCmdExec 3, Movement_0C08
    VMJump L_0B96

L_0B8E:
    ActorCmdExec 3, Movement_0C1C

L_0B96:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0BB5
    VMSleep 8
    ActorCmdExec 255, Movement_0C94

L_0BB5:
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 3
    SEWait
    FlagSet EVENT_FLAG_0x03e4
    VMJump L_0BDB

L_0BCB:
    // "Hey! What's going on?[f000]븁\u0000\nYour party's full, huh?[f000]븁\u0000\nOK![f000]븁\u0000\nOK![f000]븁\u0000\nGot it![f000]븁\u0000\nI'll wait here!"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_HeyWhatsGoingPartys, 3, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0BDB:
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0C08:
    Move 14, 1
    Move 13, 2
    Move 14, 1
    Move 13, 7
    MoveEnd

Movement_0C1C:
    Move 13, 3
    Move 14, 2
    Move 13, 6
    MoveEnd

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I think I can get even stronger\nif I learn more about Pokémon![f000]븁\u0000\nThat's why I want to go to many\ndifferent places--so I can learn a lot!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_ThinkCanGetEven, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My dream is to strengthen my Pokémon\nhere and become the strongest[f000]븀\u0000\nTrainer in Unova!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown2_Text_DreamStrengthenPokemonHere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0C64:
    Move 100, 1
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

Movement_0C8C:
    Move 32, 1
    MoveEnd

Movement_0C94:
    Move 33, 1
    MoveEnd

Movement_0C9C:
    Move 34, 1
    MoveEnd

Movement_0CA4:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd

Movement_0CB4:
    Move 159, 1
    MoveEnd
