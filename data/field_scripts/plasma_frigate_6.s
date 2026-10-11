#include "asm/field_script.inc"
#include "text/script/plasma_frigate_6.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_2:
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0356
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0089
    ActorSetGPos 0, 8, 0, 6, 3
    VMJump L_00B8

L_0089:
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x0356
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00B8
    ActorSetGPos 0, 8, 0, 6, 3

L_00B8:
    VMHalt

Script_4:
    VMHalt

Script_5:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    FlagReset EVENT_FLAG_0x0350
    FlagReset EVENT_FLAG_0x0356
    ActorWalkRoute 255, 11, 8, 1, 8, 0
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xb8000, 0, 0x34000, 80
    EvCameraWait
    // "???: That blasted Colress![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_BlastedColress, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0988
    ActorCmdWait
    ActorCmdExec 4, Movement_08A0
    VMSleep 24
    SEPlay SEQ_SE_SW_GHETSIS_STICK_01
    SEWait
    ActorCmdWait
    // "The fool is far too committed\nto pure science.[f000]븁\u0000\nThis is how he repays me for\nmaking him the boss of Team Plasma?[f000]븁\u0000\nHow dare he put his personal\nintellectual curiosity before our[f000]븀\u0000\nultimate mission of conquering Unova![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_FoolFarTooCommitted, 4, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 4, 12, 2, 0, 16, 0
    ActorCmdWait
    // "How fortunate for you![f000]븁\u0000\nFew get to be the sole audience member\nfor one of my speeches.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_HowFortunateFewGet, 4, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 4, 12, 3, 0, 16, 0
    ActorCmdWait
    // "Team Plasma will use its\nknowledge and technology[f000]븀\u0000\nto take Kyurem's true[f000]븀\u0000\npower to its absolute limit[f000]븀\u0000\nand freeze the Unova region.[f000]븁\u0000\nThe terrified people and Pokémon\nwill bow at Team Plasma's...no...[f000]븀\u0000\nat MY feet![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_TeamPlasmaWillUse, 4, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 4, 13, 5, 0, 16, 0
    ActorCmdWait
    ActorCmdExec 4, Movement_08A0
    VMSleep 24
    SEPlay SEQ_SE_SW_GHETSIS_STICK_01
    SEWait
    ActorCmdWait
    // "Kyurem is an empty being.[f000]븁\u0000\nThe remnants of a certain Pokémon\nwhen it split into Reshiram and[f000]븀\u0000\nZekrom...[f000]븁\u0000\nMy desire is absolute rule of Unova![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_KyuremEmptyBeingRemnants, 4, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 60
    ActorWalkRoute 4, 13, 7, 0, 16, 0
    VMSleep 40
    ActorCmdExec 255, Movement_0998
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    // "That's right! Kyurem will be the vessel\ninto which my desires will be poured![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ThatsRightKyuremWill, 4, 0, 0
    MsgWinCloseAll
    ActorAdd 1
    FlagSet EVENT_FLAG_0x09fa
    ActorCmdExec 1, Movement_08AC
    BGMChangeMap
    ActorCmdWait
    ActorCmdExec 1, Movement_0998
    ActorCmdExec 4, Movement_0990
    ActorCmdExec 255, Movement_0980
    ActorCmdWait
    // "Shadow Triad: Lord Ghetsis.\nKyurem has been transferred![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadLordGhetsis, 1, 0, 0
    MsgWinCloseAll
    // "Ghetsis: It's finally here![f000]븁\u0000\nThe wonderful era in which\nI am the absolute ruler of Unova[f000]븀\u0000\nhas finally arrived![f000]븁\u0000\nI'll let you take care of this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_GhetsisItsFinallyHere, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0384
    VMSleep 26
    ActorCmdExec 1, Movement_0988
    ActorCmdExec 255, Movement_0988
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 4
    SEWait
    ActorAdd 0
    ActorCmdExec 0, Movement_0374
    ActorCmdWait
    ActorCmdExec 1, Movement_0990
    ActorCmdExec 255, Movement_0990
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Wait...\nYou're one of the Shadow Triad, right?[f000]븁\u0000\nTell me about the Purrloin\nthat was stolen in Aspertia.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_WaitYoureOneShadow, 0, 0, 0
    MsgWinCloseAll
    // "Shadow Triad: Very well...[f000]븁\u0000\nThis might be it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadVeryWell, 1, 0, 0
    MsgWinCloseAll
    ActorAdd 2
    PVPlay 510, 0
    // "Grrooowwwl!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_Grrooowwwl, 2, 0, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_09A0
    ActorCmdWait
    // "Shadow Triad: I stole this Pokémon\nfive years ago in Aspertia.[f000]븁\u0000\nSo it seems likely that it is the\nPokémon you're talking about.[f000]븁\u0000\nBut now, it only listens to my commands.[f000]븁\u0000\nSuch is the fate of Pokémon\nthat are trapped in Poké Balls![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadStolePokemon, 1, 0, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Wh-what?[f000]븁\u0000\nDon't mess with me...\nThat's someone else's Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_WhWhatDontMess, 0, 0, 0
    MsgWinCloseAll
    // "Shadow Triad: Ah...\nI feel sorry for Pokémon.[f000]븁\u0000\nThey're ruled by Poké Balls\nand the whims of their Trainers...[f000]븁\u0000\nLord Ghetsis spoke of\nPokémon liberation two years ago[f000]븀\u0000\nsimply for his own ambitions, but...[f000]븁\u0000\nIf his plans had succeeded, many Pokémon\nwould have been saved.[f000]븁\u0000\nThis Liepard--well, you knew it as a\nPurrloin--if it had been released,[f000]븀\u0000\nit might have returned to you.[f000]븁\u0000\nWell then...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadAhFeel, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0988
    ActorCmdExec 255, Movement_0980
    ActorCmdWait
    // "You there!\nI won't let you interfere with[f000]븀\u0000\nLord Ghetsis's plans![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ThereWontLetInterfere, 1, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_SHADOW, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0319
    CallTrainerBattleEnd
    VMJump L_0327

L_0319:
    FlagSet EVENT_FLAG_0x0350
    FlagSet EVENT_FLAG_0x0356
    FlagReset EVENT_FLAG_0x09fa
    CallTrainerLose

L_0327:
    ActorAdd 5
    ActorAdd 3
    ActorCmdExec 5, Movement_08AC
    ActorCmdExec 3, Movement_08AC
    ActorCmdWait
    // "Shadow Triad: You're not done yet..."
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadYoureNot, 5, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x034f
    WorkSetConst EVENT_WORK_0x40f4, 2
    WorkSetConst EVENT_WORK_0x4101, 1
    FlagSet EVENT_FLAG_0x034a
    FlagSet EVENT_FLAG_0x034b
    FlagSet EVENT_FLAG_0x0402
    FlagSet EVENT_FLAG_0x038d
    VMHalt
    .balign 4, 0

Movement_0374:
    Move 68, 1
    Move 16, 3
    Move 39, 1
    MoveEnd

Movement_0384:
    Move 13, 3
    Move 14, 2
    Move 13, 3
    MoveEnd

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Shadow Triad: Can you defeat\nall three of us?"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadCanDefeat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    TrainerFlagGet TRAINER_TEAM_PLASMA_SHADOW_2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0430
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Shadow Triad: I have no problem\nwith you, but this is for Lord Ghetsis![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadHaveNo, 5, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_SHADOW_2, 0, 0
    VMCall L_08C0
    // "Shadow Triad: Even if I lose,\nLord Ghetsis simply has to win..."
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadEvenIf, 5, 0, 0
    WorkAdd EVENT_WORK_0x40f4, 1
    TrainerFlagSet TRAINER_TEAM_PLASMA_SHADOW_2
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0426
    MsgWaitAdvance
    MsgWinCloseAll
    VMCall L_04E4
    VMJump L_042A

L_0426:
    LastKeyWait
    MsgWinCloseAll

L_042A:
    VMJump L_0444

L_0430:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Shadow Triad: Even if I lose,\nLord Ghetsis simply has to win..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadEvenIf, 0, 0
    LastKeyWait
    ActorMsgClose

L_0444:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    TrainerFlagGet TRAINER_TEAM_PLASMA_SHADOW_3, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04CA
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Shadow Triad: We swore to be loyal\nto Lord Ghetsis since he saved us![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadWeSwore, 3, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_SHADOW_3, 0, 0
    VMCall L_08C0
    // "Shadow Triad: Listen well![f000]븁\u0000\nThe only thing we want is\nthe world Lord Ghetsis desires!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadListenWell, 3, 0, 0
    WorkAdd EVENT_WORK_0x40f4, 1
    TrainerFlagSet TRAINER_TEAM_PLASMA_SHADOW_3
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C0
    MsgWaitAdvance
    MsgWinCloseAll
    VMCall L_04E4
    VMJump L_04C4

L_04C0:
    LastKeyWait
    MsgWinCloseAll

L_04C4:
    VMJump L_04DE

L_04CA:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Shadow Triad: Even if I lose,\nLord Ghetsis simply has to win..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadEvenIf, 0, 0
    LastKeyWait
    ActorMsgClose

L_04DE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_04E4:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8022, 0x8023
    PlayerGetDir 0x8021
    VMStackPush 0x8022
    VMStackPushConst 13
    VMStackCmp CMP_GE
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0525
    ActorCmdExec 255, Movement_0990
    VMJump L_0540

L_0525:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0540
    ActorCmdExec 255, Movement_0980

L_0540:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x9e000, 0, 0x68000, 32
    EvCameraWait
    ActorCmdWait
    // "Shadow Triad: No matter...[f000]븁\u0000\nThe only thing you can do is\nwatch Lord Ghetsis use Kyurem[f000]븀\u0000\nto freeze Unova solid. That's all...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadNoMatter, 1, 0, 0
    ActorCmdExec 1, Movement_0990
    ActorCmdWait
    // "You...\nI'll return this Pokémon to you.[f000]븁\u0000\nConsidering what Lord Ghetsis\nis about to do,[f000]븀\u0000\nI have no further need of it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_IllReturnPokemonConsidering, 1, 0, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x09fa
    ActorCmdExec 1, Movement_08B4
    ActorCmdExec 5, Movement_08B4
    ActorCmdExec 3, Movement_08B4
    ActorCmdWait
    BGMChangeMap
    ActorDelete 1
    ActorDelete 5
    ActorDelete 3
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_05D1
    ActorCmdExec 255, Movement_0990
    ActorCmdWait

L_05D1:
    // "[f000]Ā\u0001\u0001: ...[f000]븁\u0000\n...[f000]븁\u0000\nHey...\n[f000]Ā\u0001\u0000...[f000]븁\u0000\nIf we let Team Plasma\ndo whatever they want...[f000]븁\u0000\nThere'll be more sad Pokémon\nlike Purrloin and Kyurem..."
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_HeyIfWeLet, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    EvCameraMoveToDefault 32
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x4101, 2
    FlagSet EVENT_FLAG_0x0350
    FlagSet EVENT_FLAG_0x0359
    VMReturn

Script_9:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_065D
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: Sorry...[f000]븁\u0000\nThis situation is messing with my head...\nI just don't know what to do...[f000]븁\u0000\nI finally found my sister's Pokémon,\nbut now it's glaring at me...[f000]븁\u0000\nWhy?!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_SorrySituationMessingHead, 0, 0, 0
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0649
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0657

L_0649:
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0998
    ActorCmdWait

L_0657:
    VMJump L_069E

L_065D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: ...[f000]븁\u0000\n...[f000]븁\u0000\nHey...\n[f000]Ā\u0001\u0000...[f000]븁\u0000\nIf we let Team Plasma\ndo whatever they want...[f000]븁\u0000\nThere'll be more sad Pokémon\nlike Purrloin and Kyurem..."
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_HeyIfWeLet, 0, 0, 0
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0690
    LastKeyWait
    MsgWinCloseAll
    VMJump L_069E

L_0690:
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0998
    ActorCmdWait

L_069E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    PVPlay 510, 0
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_06D5
    // "Grrooowwwl!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_Grrooowwwl, 2, 0, 0
    VMJump L_06E1

L_06D5:
    // "Grrrrrr..."
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_Grrrrrr, 2, 0, 0

L_06E1:
    PVWait
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0716
    ActorCmdExec 5, Movement_076C
    VMJump L_071E

L_0716:
    ActorCmdExec 5, Movement_0784

L_071E:
    VMSleep 46
    ActorCmdExec 255, Movement_07B4
    ActorCmdWait
    // "Shadow Triad: If you want to go any\nfurther, you'll have to get past us.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate6_Text_ShadowTriadIfWant, 5, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_075B
    ActorCmdExec 5, Movement_079C
    VMJump L_0763

L_075B:
    ActorCmdExec 5, Movement_07A8

L_0763:
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_076C:
    Move 38, 1
    Move 75, 1
    Move 17, 1
    Move 18, 1
    Move 16, 1
    MoveEnd

Movement_0784:
    Move 39, 1
    Move 75, 1
    Move 17, 1
    Move 19, 1
    Move 16, 1
    MoveEnd

Movement_079C:
    Move 19, 1
    Move 36, 1
    MoveEnd

Movement_07A8:
    Move 18, 1
    Move 36, 1
    MoveEnd

Movement_07B4:
    Move 1, 1
    Move 71, 1
    Move 16, 1
    Move 72, 1
    MoveEnd

Script_1:
    ActorsPauseAll
    GameGetVersion 0x8020
    PlayerGetDir 0x8021
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07F5
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_10, 4, 10, 32801
    VMJump L_07FF

L_07F5:
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_13, 4, 10, 32801

L_07FF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0829
    // "A map of the Unova region\nis being displayed."
    InfoMsg PlasmaFrigate6_Text_MapUnovaRegionBeing, 2
    VMJump L_082E

L_0829:
    // "The monitor is showing static..."
    InfoMsg PlasmaFrigate6_Text_MonitorShowingStatic, 2

L_082E:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_085C
    // "Data about the ship\nis being displayed."
    InfoMsg PlasmaFrigate6_Text_DataAboutShipBeing, 2
    VMJump L_0861

L_085C:
    // "The monitor is showing static..."
    InfoMsg PlasmaFrigate6_Text_MonitorShowingStatic, 2

L_0861:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_088F
    // "The monitor is showing static...[f000]븁\u0000\nThe voices of Team Plasma members\nwho should be in the ship can be heard."
    InfoMsg PlasmaFrigate6_Text_MonitorShowingStaticVoices, 2
    VMJump L_0894

L_088F:
    // "The monitor is showing static..."
    InfoMsg PlasmaFrigate6_Text_MonitorShowingStatic, 2

L_0894:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_08A0:
    Move 1, 1
    Move 182, 1
    MoveEnd

Movement_08AC:
    Move 184, 1
    MoveEnd

Movement_08B4:
    Move 185, 1
    Move 69, 1
    MoveEnd

L_08C0:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0939
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 13
    VMStackCmp CMP_GE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0912
    ActorSetGPos 1, 11, 0, 6, 3
    VMJump L_0931

L_0912:
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0931
    ActorSetGPos 1, 11, 0, 6, 1

L_0931:
    CallTrainerBattleEnd
    VMJump L_093B

L_0939:
    CallTrainerLose

L_093B:
    VMReturn
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
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

Movement_0980:
    Move 32, 1
    MoveEnd

Movement_0988:
    Move 33, 1
    MoveEnd

Movement_0990:
    Move 34, 1
    MoveEnd

Movement_0998:
    Move 35, 1
    MoveEnd

Movement_09A0:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 100, 1
    MoveEnd
