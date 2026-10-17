#include "asm/field_script.inc"
#include "text/script/nimbasa_city.h"

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
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntry Script_25
    ScriptEntry Script_26
    ScriptEntry Script_27
    ScriptEntry Script_28
    ScriptEntry Script_29
    ScriptEntry Script_30
    ScriptEntry Script_31
    ScriptEntry Script_32
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_8:
    WorkSetConst EVENT_WORK_0x417c, 1
    WorkSetConst EVENT_WORK_0x4160, 0
    FlagReset EVENT_FLAG_0x00dc
    FlagSet EVENT_FLAG_0x026e
    FlagSet EVENT_FLAG_0x026f
    FlagSet EVENT_FLAG_0x0270
    FlagSet EVENT_FLAG_0x0271
    FlagSet EVENT_FLAG_0x0272
    FlagSet EVENT_FLAG_0x0295
    FlagSet EVENT_FLAG_0x0296
    FlagSet EVENT_FLAG_0x0297
    FlagSet EVENT_FLAG_0x0298
    FlagSet EVENT_FLAG_0x0299
    FlagSet EVENT_FLAG_0x00f2
    VMStackPushFlag EVENT_FLAG_0x02fc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0112
    TrainerCardGetSex 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010C
    WorkSetConst EVENT_WORK_0x4020, 240
    VMJump L_0112

L_010C:
    WorkSetConst EVENT_WORK_0x4020, 231

L_0112:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0141
    HollowRivalCmd_0262 2, 14
    VMJump L_016A

L_0141:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_016A
    HollowRivalCmd_0262 2, 0

L_016A:
    VMHalt

Script_9:
    VMCall L_0174
    VMHalt

L_0174:
    VMStackPush EVENT_WORK_0x40c0
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C3
    ActorSetGPos 3, 407, 1, 438, 0
    ActorSetGPos 1, 406, 1, 438, 3
    ActorSetGPos 4, 407, 1, 437, 1
    ActorSetGPos 0, 406, 1, 439, 3
    ActorSetGPos 2, 407, 1, 440, 0

L_01C3:
    VMReturn

Script_7:
    ActorsPauseAll
    CallPlaceNameDisp
    ActorCmdExec 255, Movement_01DC
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01DC:
    Move 13, 1
    MoveEnd
    WorkSetConst 0x8024, 0

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Nimbasa City\nLit by the Flash of Lightning!"
    MsgPlaceSign NimbasaCity_Text_NimbasaCityLitBy, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Big Stadium\nBaseball, Football, and Soccer"
    MsgPlaceSign NimbasaCity_Text_BigStadiumBaseballFootball, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Small Court\nTennis and Basketball"
    MsgPlaceSign NimbasaCity_Text_SmallCourtTennisBasketball, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Battle Subway\nBattle and Ride!"
    MsgPlaceSign NimbasaCity_Text_BattleSubwayBattleRide, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Musical Theater\nProps, Music, Dance, Excitement!"
    MsgPlaceSign NimbasaCity_Text_MusicalTheaterPropsMusic, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nGames occur at specific times in\nBig Stadium and Small Court![f000]븁\u0000\nYou might be able to have a Pokémon\nbattle with your favorite athlete!"
    MsgPlaceSign NimbasaCity_Text_TrainerTipsGamesOccur, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a64
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nThe Musical Theater is always\nlooking for participants![f000]븁\u0000\nYou might get more wonderful Props\nif you participate repeatedly!"
    MsgPlaceSign NimbasaCity_Text_TrainerTipsMusicalTheater, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a65
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Battle Institute!\nTest your Trainer Skills!"
    MsgPlaceSign NimbasaCity_Text_BattleInstituteTestTrainer, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The combination of athletes and Pokémon\nis a super play for sure![f000]븁\u0000\nWhere can you see it?\nCheck out Big Stadium and Small Court!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_CombinationAthletesPokemonSuper, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nimbasa City is full of plaaaces ♪\nfor toughening uuuuup your Pokémon! ♪"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_NimbasaCityFullPlaaaces, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to go to Anville Town.\nWhere is Gear Station?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_WantGoAnvilleTown, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    WordSetLoadJoinAvenueName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When I pass by people, I eagerly\nawait the opening of a new store[f000]븀\u0000\nin [f000]Ĺ\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_WhenPassByPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_26:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You have to have three Pokémon for\nTriple Battles and Rotation Battles![f000]븁\u0000\nWhat should I do? I still only have\ntwo partners right now!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_HaveHaveThreePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_27:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I was a good girl, so I got\nto go to the amusement park![f000]븀\u0000\nThis time Pansear came, too!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_GoodGirlGotGo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_28:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 513, 0
    // "Raesnap!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_Raesnap, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_29:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "And I rode the roller coaster,\nand it went like zoom, zoom![f000]븁\u0000\nAnd it was like a Pokémon move.\nLike Quick Attack!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_RodeRollerCoasterWent, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_30:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nimbasa City's Gym Leader,\nElesa, is a fashion model![f000]븁\u0000\nI'd love to see her glide\nlightly down the catwalk!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_NimbasaCitysGymLeader, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40c0, 2
    HollowRivalCmd_0262 1, 7
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1978000, 0x1000f, 0x1b68000, 20
    EvCameraWait
    BGMPlay SEQ_BGM_E_PLASMA
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Hold it!\nWhat are you guys up to here anyway?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_HoldWhatGuysUp, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0FC8
    VMSleep 6
    ActorCmdExec 1, Movement_0FD0
    ActorCmdWait
    VMSleep 40
    ActorCmdExec 0, Movement_0F94
    ActorCmdExec 1, Movement_0F94
    ActorCmdWait
    // "Team Plasma: Nothing...\nWe're just standing here.[f000]븁\u0000\nSo what did we do to you that\nyou're bothering us with questions?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaNothingWere, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_0F74
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: You didn't do anything.\nNot to me, at least...[f000]븁\u0000\nBut I can never forgive\nPokémon thieves like you![f000]븁\u0000\nJust to let you know...[f000]븁\u0000\nYou're about to feel my rage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_DidntAnythingNotLeast, 3, 0, 0
    MsgWinCloseAll
    // "Team Plasma: Yeesh...\nTrainers sure are unruly these days.[f000]븁\u0000\nIs that it? You're mistaking your\nPokémon's strength for your own?[f000]븁\u0000\nI couldn't care less about a runt like\nyou, but I don't like being messed with![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaYeeshTrainers, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0F9C
    VMSleep 3
    ActorCmdExec 2, Movement_0F9C
    ActorCmdWait
    ActorCmdExec 4, Movement_0FD0
    VMSleep 3
    ActorCmdExec 2, Movement_0FC8
    ActorCmdWait
    ActorCmdExec 3, Movement_0FB4
    ActorCmdWait
    VMSleep 8
    ActorCmdExec 3, Movement_0FE0
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000!\nCover me!"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_Cover, 3, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 3, Movement_0FC8
    ActorCmdWait
    EvCameraReturn 20
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    BGMChangeMap
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000!\nCover me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_Cover_2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_0FC8
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Team Plasma: M-me, losing in an instant?!\nWho IS this guy?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaMLosing, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0544
    PlayerSetSpecialSequence 1

L_0544:
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_4
    // "Team Plasma: Heh heh heh![f000]븁\u0000\nI stole this Pokémon two years ago,\nand I've been training it ever since![f000]븀\u0000\nIt's tough![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaHehHeh, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_4, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0581
    WorkAdd EVENT_WORK_0x40c0, 1
    CallTrainerBattleEnd
    VMJump L_0589

L_0581:
    WorkSetConst EVENT_WORK_0x40c0, 2
    CallTrainerLose

L_0589:
    // "Team Plasma: You've got to be kidding![f000]븁\u0000\nAnd I bullied it so much over the last\ntwo years to toughen it up, too![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaYouveGot, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 405, 439, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0FE0
    ActorCmdWait
    ActorWalkRoute 4, 405, 437, 1, 8, 0
    VMSleep 3
    ActorCmdExec 3, Movement_0FD8
    ActorCmdWait
    ActorCmdExec 4, Movement_0FE0
    ActorCmdWait
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_47
    ActorCmdExec 255, Movement_0FD0
    ActorCmdExec 2, Movement_0FC8
    ActorCmdWait
    // "Team Plasma: Looks like I'm up next!\nJust to warn you, I show no mercy![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaLooksLike, 2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_47, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0626
    WorkAdd EVENT_WORK_0x40c0, 1
    CallTrainerBattleEnd
    VMJump L_062E

L_0626:
    WorkSetConst EVENT_WORK_0x40c0, 2
    CallTrainerLose

L_062E:
    // "Team Plasma: If we keep going like this,\nit'll end up like it did two years ago![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaIfWe, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 405, 440, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 2, Movement_0FE0
    ActorCmdWait
    VMCall L_0828
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0685
    PlayerSetSpecialSequence 1

L_0685:
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_47
    // "Team Plasma: You've got some nerve\nfor a little brat![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaYouveGot_2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_47, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06C2
    WorkAdd EVENT_WORK_0x40c0, 1
    CallTrainerBattleEnd
    VMJump L_06CA

L_06C2:
    WorkSetConst EVENT_WORK_0x40c0, 2
    CallTrainerLose

L_06CA:
    // "Team Plasma: If we keep going like this,\nit'll end up like it did two years ago![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaIfWe, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 405, 440, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 2, Movement_0FE0
    ActorCmdWait
    ActorWalkRoute 4, 405, 437, 1, 8, 0
    VMSleep 3
    ActorCmdExec 3, Movement_0FD8
    ActorCmdWait
    ActorCmdExec 4, Movement_0FE0
    ActorCmdWait
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_4
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 408
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 440
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0759
    WorkSub 0x8022, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait

L_0759:
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0770
    VMJump L_0786

L_0770:
    ActorCmdExec 255, Movement_0FC8
    ActorCmdExec 0, Movement_0FD0
    VMJump L_07AF

L_0786:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0799
    VMJump L_07AF

L_0799:
    ActorCmdExec 255, Movement_0FD8
    ActorCmdExec 0, Movement_0FE0
    VMJump L_07AF

L_07AF:
    ActorCmdWait
    // "Team Plasma: I'm next![f000]븁\u0000\nI stole this Pokémon two years ago,\nand I've been training it ever since![f000]븀\u0000\nIt's tough![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaImNext, 0, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_4, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07EC
    WorkAdd EVENT_WORK_0x40c0, 1
    CallTrainerBattleEnd
    VMJump L_07F4

L_07EC:
    WorkSetConst EVENT_WORK_0x40c0, 2
    CallTrainerLose

L_07F4:
    // "Team Plasma: You've got to be kidding![f000]븁\u0000\nAnd I bullied it so much over the last\ntwo years to toughen it up, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaYouveGot, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 405, 439, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0FE0
    ActorCmdWait
    VMCall L_0828
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0828:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40c0, 4
    ActorWalkRoute 1, 405, 438, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 1, Movement_0FE0
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 408
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 440
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0883
    ActorWalkRoute 255, 407, 439, 1, 8, 1
    ActorCmdWait

L_0883:
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08A4
    ActorCmdExec 255, Movement_0FD8
    ActorCmdWait

L_08A4:
    // "Team Plasma: We give up!\nI can't believe we lost to two kids![f000]븁\u0000\nJust because you're strong,\nI'll tell you this...[f000]븁\u0000\nWe, Team Plasma,\nare searching for something...[f000]븁\u0000\nWhen we find it, that's when our secret\nweapon will be able to use its true power![f000]븁\u0000\nFarewell![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_TeamPlasmaWeGive, 1, 0, 0
    MsgWinCloseAll
    VMSleep 15
    FadeEx 3, 0, 16, 4
    FadeExWait
    ActorDelete 1
    ActorDelete 0
    ActorDelete 4
    ActorDelete 2
    BGMFadeOutAll 40
    VMSleep 45
    FlagSet EVENT_FLAG_0x02fa
    FadeEx 3, 16, 0, 4
    FadeExWait
    VMSleep 45
    ActorCmdExec 3, Movement_0FAC
    ActorCmdWait
    BGMPlay SEQ_BGM_E_EMOTION
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Five years ago...[f000]븁\u0000\nTeam Plasma stole\nmy little sister's Purrloin.[f000]븀\u0000\nIt had been given to her as a present.[f000]븁\u0000\nI was only a little kid...\nI couldn't do anything...[f000]븁\u0000\nSo... So that's why I have\nto get stronger![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_FiveYearsAgoTeam, 3, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 406, 439, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 3, Movement_0FE0
    ActorCmdWait
    // "Good work! I knew you\nhad good instincts![f000]븁\u0000\nWell then...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_GoodWorkKnewHad, 3, 0, 0
    MsgWinCloseAll
    PokePartyRecoverAll
    SEPlay SEQ_SE_RECOVERY
    SEWait
    WordSetLoadRivalName 1
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0001 fully healed\n[f000]Ā\u0001\u0000's Pokémon!"
    SystemMsg NimbasaCity_Text_FullyHealedSPokemon, 0
    MsgWaitAdvance
    InfoMsgClose
    // "[f000]Ā\u0001\u0001: Listen up!\nYou fill up that Pokédex.[f000]븁\u0000\nKeep getting stronger and stronger,\nand back me up![f000]븁\u0000\nI'll be counting on you from\nhere on out, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_ListenUpFillUp, 3, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 411, 440, 1, 8, 1
    VMSleep 20
    ActorCmdExec 255, Movement_0FE0
    ActorCmdWait
    ActorWalkRoute 3, 411, 447, 1, 8, 1
    ActorCmdWait
    ActorDelete 3
    FlagSet EVENT_FLAG_0x02f9
    HollowRivalCmd_0262 1, 8
    BGMChangeMap
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0120
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09D0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I heard that in Gear Station\nyou can have Pokémon battles[f000]븀\u0000\nin the subway![f000]븁\u0000\nSo I came here to test my skills,\nand, what do you know...[f000]븀\u0000\nthe Subway Bosses were here![f000]븁\u0000\nIsn't that amazing? I mean, they're\nthe strongest Trainers in Gear Station![f000]븁\u0000\nAnd they said they'd battle\nif it's two on two![f000]븁\u0000\nThis is an a-MAZ-ing opportunity!\nWould you PLEASE battle with me?"
    // "I heard that in Gear Station\nyou can have Pokémon battles[f000]븀\u0000\nin the subway![f000]븁\u0000\nSo I came here to test my skills,\nand, what do you know...[f000]븀\u0000\nthe Subway Bosses were here![f000]븁\u0000\nIsn't that amazing? I mean, they're\nthe strongest Trainers in Gear Station![f000]븁\u0000\nAnd they said they'd battle\nif it's two on two![f000]븁\u0000\nThis is an awesome opportunity!\nWould you battle with me?"
    ActorMsgGendered 1024, NimbasaCity_Text_HeardGearStationCan, NimbasaCity_Text_HeardGearStationCan_2, 5, 2, 0
    FlagSet EVENT_FLAG_0x0120
    VMJump L_09E4

L_09D0:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You're prepared, right?\nWill you battle alongside me?"
    // "You're prepared, right?\nWill you battle alongside me?"
    ActorMsgGendered 1024, NimbasaCity_Text_YourePreparedRightWill, NimbasaCity_Text_YourePreparedRightWill_2, 5, 2, 0

L_09E4:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DC0
    // "Thanks![f000]븁\u0000\nOh! The name's Rosa![f000]븁\u0000\nLet's become the best partners ever\nand surpass the Subway Bosses![f000]븁\u0000"
    // "Thanks![f000]븁\u0000\nOh! The name's Nate![f000]븁\u0000\nPokémon-fan synergy can create a\ncombination that's better than perfect![f000]븁\u0000"
    ActorMsgGendered 1024, NimbasaCity_Text_ThanksOhNamesRosa, NimbasaCity_Text_ThanksOhNamesNate, 5, 2, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0FC8
    VMSleep 8
    ActorCmdExec 255, Movement_0FC8
    ActorCmdWait
    // "So I'll have you challenge the\nSubway Bosses with me, then![f000]븁\u0000"
    // "So I'll have you help me battle\nthe Subway Bosses, then![f000]븁\u0000"
    ActorMsgGendered 1024, NimbasaCity_Text_IllHaveChallengeSubway, NimbasaCity_Text_IllHaveHelpBattle, 5, 2, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0FE0
    ActorCmdExec 7, Movement_0FD8
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 6, Movement_0FD0
    ActorCmdExec 7, Movement_0FD0
    ActorCmdWait
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1a68000, 0x1000f, 0x1ce8000, 1
    EvCameraWait
    ActorSetGPos 255, 420, 1, 461, 3
    ActorSetGPos 5, 420, 1, 463, 3
    ActorSetGPos 6, 424, 1, 463, 2
    ActorSetGPos 7, 424, 1, 461, 2
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ACE
    PlayerSetSpecialSequence 1

L_0ACE:
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Ingo: Having a battle in a place like this\nis a little irregular, but this must've[f000]븀\u0000\nhappened for a reason.[f000]븁\u0000\nBattling in a different place will let me\nsee different scenery, and I might learn[f000]븀\u0000\nsomething, too.[f000]븁\u0000\nNow, Emmet, if you have something\nto add, please![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_IngoHavingBattlePlace, 6, 2, 0
    MsgWinCloseAll
    // "Emmet: Follow the rules and drive safely![f000]븁\u0000\nWe're headed for victory!\nAll aboard![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_EmmetFollowRulesDrive, 7, 1, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0F6C
    ActorCmdExec 5, Movement_0F6C
    ActorCmdExec 6, Movement_0F74
    ActorCmdExec 7, Movement_0F74
    ActorCmdWait
    TrainerCardGetSex 0x8023
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0B53
    CallTrainerMultiBattle 363, 732, 733, 0
    VMJump L_0C29

L_0B53:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0B86
    CallTrainerMultiBattle 360, 732, 733, 0
    VMJump L_0C29

L_0B86:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0BB9
    CallTrainerMultiBattle 364, 732, 733, 0
    VMJump L_0C29

L_0BB9:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0BEC
    CallTrainerMultiBattle 361, 732, 733, 0
    VMJump L_0C29

L_0BEC:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0C1F
    CallTrainerMultiBattle 365, 732, 733, 0
    VMJump L_0C29

L_0C1F:
    CallTrainerMultiBattle 362, 732, 733, 0

L_0C29:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C81
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1a68000, 0x1000f, 0x1ce8000, 1
    EvCameraWait
    PokePartyGetCount 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C79
    PokePartyRecoverAll

L_0C79:
    CallTrainerBattleEnd
    VMJump L_0C83

L_0C81:
    CallTrainerLose

L_0C83:
    VMSleep 30
    // "Ingo: Bravo![f000]븁\u0000\nThe combination of you and your\nPokémon is truly fantastic![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_IngoBravoCombinationPokemon, 6, 2, 0
    MsgWinCloseAll
    ActorCmdExec 7, Movement_0FD8
    ActorCmdWait
    // "Emmet: I'm Emmet.[f000]븁\u0000\nWhile I may have lost to you...[f000]븁\u0000\nI had a good time!\nLet's play again sometime![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_EmmetImEmmetWhile, 7, 1, 0
    MsgWinCloseAll
    // "Ingo: Yes! Definitely! Next time,\nplease ride the subway[f000]븀\u0000\nand battle with us there![f000]븁\u0000\nWell then, we're off! All aboard![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_IngoYesDefinitelyNext, 6, 2, 0
    MsgWinCloseAll
    ActorWalkRoute 7, 422, 459, 1, 8, 1
    VMSleep 2
    ActorWalkRoute 6, 422, 460, 1, 8, 1
    VMSleep 16
    ActorCmdExec 5, Movement_0F7C
    ActorCmdExec 255, Movement_0F7C
    ActorCmdWait
    ActorCmdExec 6, Movement_0F7C
    ActorCmdWait
    ActorDelete 7
    ActorWalkRoute 6, 422, 459, 1, 8, 1
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 6
    SEWait
    VMSleep 15
    ActorWalkRoute 5, 422, 461, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 5, Movement_0FD8
    VMSleep 10
    ActorCmdExec 255, Movement_0FE0
    ActorCmdWait
    WordSetPlayerName 0
    // "They were so tough, even when they were\nholding back for us![f000]븀\u0000\nThat's the Subway Bosses for you![f000]븁\u0000\nI want to get even stronger and\nbattle the Subway Bosses when[f000]븀\u0000\nthey're giving it their all![f000]븁\u0000\nThanks, [f000]Ā\u0001\u0000!\nThis is a token of my appreciation![f000]븀\u0000\nPlease take it![f000]븁\u0000"
    // "They were really tough, even when they\nweren't going all out![f000]븀\u0000\nThat's the Subway Bosses for you![f000]븁\u0000\nI want to get even stronger and\nbattle the Subway Bosses when[f000]븀\u0000\nthey're giving it their all![f000]븁\u0000\nThanks, [f000]Ā\u0001\u0000!\nThis is a token of my appreciation![f000]븀\u0000\nHere![f000]븁\u0000"
    ActorMsgGendered 1024, NimbasaCity_Text_TheyWereToughEven, NimbasaCity_Text_TheyWereReallyTough, 5, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 465
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "When you have the Vs. Recorder,\nyou can record your battles with[f000]븀\u0000\nyour friends or battles on the[f000]븀\u0000\nBattle Subway![f000]븁\u0000\n[f000]Ā\u0001\u0000![f000]븁\u0000\nI had a really great time\nbattling with you![f000]븁\u0000\nI hope we can battle together\nagain sometime! See you![f000]븁\u0000"
    // "When you have the Vs. Recorder,\nyou can record your battles with[f000]븀\u0000\nyour friends or battles on the[f000]븀\u0000\nBattle Subway![f000]븁\u0000\n[f000]Ā\u0001\u0000![f000]븁\u0000\nI had a really great time\nbattling with you![f000]븁\u0000\nI hope we can battle together\nagain sometime! See you![f000]븁\u0000"
    ActorMsgGendered 1024, NimbasaCity_Text_WhenHaveVsRecorder, NimbasaCity_Text_WhenHaveVsRecorder_2, 5, 2, 0
    MsgWinCloseAll
    ActorWalkRoute 5, 422, 459, 1, 8, 1
    VMSleep 8
    ActorCmdExec 255, Movement_0F7C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 5
    SEWait
    EvCameraReturn 16
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FlagSet EVENT_FLAG_0x02fc
    VMJump L_0DDC

L_0DC0:
    // "Oh, I see!\nYou aren't prepared right now![f000]븁\u0000\nOK, I'll stop them for now, but\nlet's battle together sometime!"
    // "Oh, I see!\nYou aren't ready yet![f000]븁\u0000\nOK, I'll stop them for now, but\nlet's battle together sometime!"
    ActorMsgGendered 1024, NimbasaCity_Text_OhSeeArentPrepared, NimbasaCity_Text_OhSeeArentReady, 5, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0FC8
    ActorCmdWait

L_0DDC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm a Subway Boss.\nMy name is Ingo![f000]븁\u0000\nUsually, I'm having Pokémon\nbattles in the subway that[f000]븀\u0000\ndeparts from Gear Station."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_ImSubwayBossName, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm Emmet.[f000]븁\u0000\nI'm a Subway Boss.[f000]븁\u0000\nI love Double Battles![f000]븁\u0000\nAnd I love the combination\nof two Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_ImEmmetImSubway, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What? A Trainer in Anville Town\nis looking for a Pokémon?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_WhatTrainerAnvilleTown, 0, 0
    MsgWinCloseAll
    VMSleep 4
    ActorCmdExec 9, Movement_0FE0
    ActorCmdExec 8, Movement_0FD8
    ActorCmdWait
    // "You sure have come a long way.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_SureHaveComeLong, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_0E67
    VMJump L_0E75

L_0E67:
    ActorCmdExec 9, Movement_0FD0
    VMJump L_0E96

L_0E75:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_0E88
    VMJump L_0E96

L_0E88:
    ActorCmdExec 9, Movement_0FD8
    VMJump L_0E96

L_0E96:
    ActorCmdWait
    // "Got it! I'll send this little\nfella to Anville Town![f000]븁\u0000\nI appreciate you telling me!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_GotIllSendLittle, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 438
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EEF
    ActorWalkRoute 9, 417, 437, 1, 8, 0
    ActorWalkRoute 8, 418, 437, 1, 8, 0
    VMSleep 8
    ActorCmdExec 255, Movement_0FD8
    ActorCmdWait
    VMJump L_0F19

L_0EEF:
    ActorWalkRoute 9, 417, 438, 1, 8, 1
    ActorWalkRoute 8, 418, 438, 1, 8, 1
    VMSleep 16
    ActorCmdExec 255, Movement_0FD8
    ActorCmdWait

L_0F19:
    ActorDelete 9
    ActorDelete 8
    VMSleep 8
    FlagSet EVENT_FLAG_0x0360
    FlagSet EVENT_FLAG_0x0361
    FlagReset EVENT_FLAG_0x0362
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 511, 0
    // "Ook!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_Ook, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0F5C:
    Move 13, 1
    MoveEnd

Movement_0F64:
    Move 12, 1
    MoveEnd

Movement_0F6C:
    Move 15, 1
    MoveEnd

Movement_0F74:
    Move 14, 1
    MoveEnd

Movement_0F7C:
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0

Movement_0F94:
    Move 3, 1
    MoveEnd

Movement_0F9C:
    Move 15, 2
    MoveEnd
    VMStackMul
    VMHalt
    .byte 0xfe
    .balign 4, 0

Movement_0FAC:
    Move 10, 1
    MoveEnd

Movement_0FB4:
    Move 61, 1
    Move 32, 1
    Move 33, 1
    Move 61, 1
    MoveEnd

Movement_0FC8:
    Move 32, 1
    MoveEnd

Movement_0FD0:
    Move 33, 1
    MoveEnd

Movement_0FD8:
    Move 34, 1
    MoveEnd

Movement_0FE0:
    Move 35, 1
    MoveEnd
    Move 71, 1
    Move 11, 1
    Move 72, 1
    MoveEnd

Movement_0FF8:
    Move 71, 1
    Move 13, 1
    Move 72, 1
    MoveEnd

Movement_1008:
    Move 0, 1
    Move 75, 1
    MoveEnd

Script_32:
    ActorsPauseAll
    ItemCheckAmount ITEM_VS_RECORDER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_104B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh my! You have a Vs. Recorder!\nIf you have a Vs. Recorder,[f000]븀\u0000\nyou can record battles with friends[f000]븀\u0000\nor on the Battle Subway![f000]븁\u0000\nBut only the strongest\ncan enter this Battle Institute![f000]븁\u0000\nSo, if you beat the Champion, come back!\nYou'll be invited to the Battle Institute!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_OhHaveVsRecorder_2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_105F

L_104B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "No matter which way I look at it,\nyou're a Trainer![f000]븁\u0000\nBut only the strongest\ncan enter this Battle Institute![f000]븁\u0000\nSo, if you beat the Champion, come back!\nYou'll be invited to the Battle Institute!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity_Text_NoMatterWhichWay_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_105F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_31:
    ActorsPauseAll
    WorkSetConst 0x8025, 0
    BMCreateHandleByGPos 0x8025, 1, 399, 467
    BMHndAudioVisualAnmPlay 0x8025, 0
    BMHndAnmWait 0x8025
    ActorNew 399, 467, 1, 251, 31, 0
    ActorCmdExec 255, Movement_1008
    ActorCmdWait
    ActorCmdExec 251, Movement_0F5C
    ActorCmdExec 255, Movement_0FF8
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8025, 1
    BMHndAnmWait 0x8025
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10E6
    WorkSetConst 0x8026, 52
    WorkSetConst 0x8027, 53
    VMJump L_10F2

L_10E6:
    WorkSetConst 0x8026, 54
    WorkSetConst 0x8027, 55

L_10F2:
    ItemCheckAmount ITEM_VS_RECORDER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_111F
    ActorMsg MSGFILE_SCRIPT, 0x8026, 251, 0, 0
    VMJump L_112B

L_111F:
    ActorMsg MSGFILE_SCRIPT, 0x8027, 251, 0, 0

L_112B:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1172
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0FC8
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8025, 0
    BMHndAnmWait 0x8025
    ActorCmdExec 251, Movement_0F64
    ActorCmdWait
    ActorDelete 251
    BMHndAudioVisualAnmPlay 0x8025, 1
    BMHndAnmWait 0x8025
    VMJump L_1182

L_1172:
    LastKeyWait
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x0409
    ActorAdd 19
    ActorDelete 251

L_1182:
    BMReleaseHandle 0x8025
    WorkSetConst EVENT_WORK_0x4141, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
