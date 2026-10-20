#include "asm/field_script.inc"
#include "text/script/aspertia_city_gym.h"

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

Script_1:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0049
    HollowRivalCmd_0262 2, 0

L_0049:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40a9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0082
    ActorSetGPos 0, 15, 0, 19, 0
    ActorSetGPos 1, 17, 0, 18, 2
    ActorSetGPos 2, 13, 0, 18, 3

L_0082:
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 3, Movement_08E8
    ActorCmdWait
    ActorWalkRoute 3, 14, 23, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 3, Movement_08E0
    VMSleep 8
    ActorCmdExec 255, Movement_08D8
    ActorCmdWait
    // "Hello! I'm Clyde, the guide for Trainers\nwho challenge Pokémon Gyms.[f000]븁\u0000\nThank you for taking on the Gym!\nTake this to commemorate[f000]븀\u0000\nyour debut![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_HelloImClydeGuide, 3, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Pokémon Gyms are facilities\nfor testing Trainers' abilities![f000]븁\u0000\nPut simply, if you can defeat\nthe Gym Leader, it means[f000]븀\u0000\nyou're a really good Trainer![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_PokemonGymsFacilitiesTesting, 3, 0, 0
    // "If you run out of Pokémon that can fight\nduring a Pokémon battle, you lose![f000]븁\u0000\nSo having a lot of Pokémon with you\nmight work to your advantage!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_IfRunOutPokemon, 3, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 3, 12, 22, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 3, Movement_08E0
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x138000, 40
    ActorCmdExec 255, Movement_08A8
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 0, Movement_08E8
    ActorCmdWait
    ActorWalkRoute 0, 15, 20, 0, 8, 1
    ActorCmdWait
    // "???: You must be a challenger![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_MustChallenger, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_08D0
    ActorCmdExec 2, Movement_08D0
    ActorCmdWait
    // "Welcome to Aspertia City's Pokémon Gym.[f000]븁\u0000\nI'm Cheren, the Gym Leader![f000]븁\u0000\nHm? Well, maybe I'd better say that I\njust became the Gym Leader![f000]븁\u0000\nMore importantly, we need to prepare\nbefore welcoming you to the Gym.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_WelcomeAspertiaCitysPokemon, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0244
    ActorCmdWait
    // "We have a challenger!\nYou two, take your places![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_WeHaveChallengerTwo, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    ActorCmdExec 1, Movement_08D8
    ActorWalkRoute 2, 13, 12, 1, 8, 1
    VMSleep 20
    ActorWalkRoute 0, 15, 21, 1, 8, 1
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    // "If you can defeat these two,\nthen I'll be your opponent!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_IfCanDefeatThese, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 15, 11, 1, 8, 1
    ActorCmdExec 2, Movement_08E0
    ActorCmdWait
    ActorSetGPos 0, 15, 2, 2, 1
    ActorSetGPos 2, 13, 0, 9, 3
    FlagSet EVENT_FLAG_0x0073
    WorkSetConst EVENT_WORK_0x40a9, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 33, 1
    Move 75, 1
    MoveEnd
    Move 14, 3
    Move 13, 1
    Move 35, 1
    MoveEnd

Movement_0244:
    Move 34, 1
    Move 63, 1
    Move 35, 1
    Move 63, 1
    MoveEnd

Movement_0258:
    Move 181, 1
    MoveEnd
    Move 12, 1
    Move 34, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    TrainerCardHasBadge 0x8008, 0
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C2
    VMStackPush EVENT_WORK_0x40a9
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_02A6
    VMCall L_030D
    VMJump L_02BC

L_02A6:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you want to battle me,\nplease defeat those two first, OK?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_IfWantBattlePlease, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_02BC:
    VMJump L_0307

L_02C2:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In the Unova region, there are eight\nPokémon Gyms and eight Gym Badges![f000]븁\u0000\nIf you're a Trainer,\nyou could collect all of them![f000]븁\u0000\nThat will make it easier to fill\nthe pages of the Pokédex as well![f000]븁\u0000\nYes, two years ago, Pokédex in hand,\nI left on a journey with my friends."
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_UnovaRegionThereEight, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0307

L_02F1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Cheren: You should decide what\nyou're going to do from here out![f000]븁\u0000\nDon't worry about losing your way--\nyou have Pokémon by your side!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_CherenShouldDecideWhat, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0307:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_030D:
    SEPlay SEQ_SE_MESSAGE
    ActorCmdExec 0, Movement_0258
    ActorCmdWait
    VMSleep 35
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0336
    VMJump L_0344

L_0336:
    ActorCmdExec 0, Movement_08C8
    VMJump L_0386

L_0344:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0357
    VMJump L_0365

L_0357:
    ActorCmdExec 0, Movement_08E0
    VMJump L_0386

L_0365:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0378
    VMJump L_0386

L_0378:
    ActorCmdExec 0, Movement_08D8
    VMJump L_0386

L_0386:
    ActorCmdWait
    // "Just as this is your first Gym challenge,\nthis is my first Pokémon battle[f000]븀\u0000\nas a Gym Leader![f000]븁\u0000\nLet's both do our best and have\na battle we can be proud of![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_JustFirstGymChallenge, 0, 0
    ActorMsgClose
    WorkSetConst 0x8023, 0
    GameGetDifficulty 0x8023
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03BF
    CallTrainerBattle TRAINER_LEADER_CHEREN_2, 0, 0
    VMJump L_03C7

L_03BF:
    CallTrainerBattle TRAINER_LEADER_CHEREN, 0, 0

L_03C7:
    WorkSetConst 0x8023, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03EC
    CallTrainerBattleEnd
    VMJump L_03EE

L_03EC:
    CallTrainerLose

L_03EE:
    // "That battle has made me feel really\nglad you were my first challenger[f000]븀\u0000\nas a Gym Leader...[f000]븁\u0000\nI give you this in honor of the strength\nyou and your Pokémon showed![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_BattleHasMadeFeel, 0, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 0
    TrainerCardAddBadge 0
    WordSetPlayerName 0
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x8024, 0
    TrainerCardGetSex 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0430
    PlayFieldEffect 2
    VMJump L_0434

L_0430:
    PlayFieldEffect 55

L_0434:
    MEWait
    WorkSetConst 0x8024, 0
    // "[f000]Ā\u0001\u0000 received the\nBasic Badge from Cheren![f000]븁\u0000"
    SystemMsg AspertiaCityGym_Text_ReceivedBasicBadgeFrom, 0
    InfoMsgClose
    // "Here is your first Gym Badge,\nthe Basic Badge![f000]븀\u0000\nThis is an important milestone![f000]븁\u0000\nWith this Badge, Pokémon up to Lv. 20\nwill obey you, including traded Pokémon.[f000]븁\u0000\nAnd I want you to take this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_HereFirstGymBadge, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 410
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "TM83 can teach your\nPokémon the move Work Up![f000]븁\u0000\nWhen you use Work Up\nwhile battling, it raises[f000]븀\u0000\nthe user's Attack and Sp. Atk.[f000]븁\u0000\nBy the way, TMs can be used\nas many times as you want![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_Tm83CanTeachPokemon, 0, 0
    // "In the Unova region, there are eight\nPokémon Gyms and eight Gym Badges![f000]븁\u0000\nIf you're a Trainer,\nyou could collect all of them![f000]븁\u0000\nThat will make it easier to fill\nthe pages of the Pokédex as well![f000]븁\u0000\nYes, two years ago, Pokédex in hand,\nI left on a journey with my friends."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_UnovaRegionThereEight, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x096e
    WorkSetConst EVENT_WORK_0x40a8, 2
    FlagReset EVENT_FLAG_0x02e6
    FlagSet EVENT_FLAG_0x02e3
    FlagSet EVENT_FLAG_0x03f7
    MedalDiscover 130
    VMReturn

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 0
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0642
    TrainerFlagGet TRAINER_YOUNGSTER_PEDRO, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0603
    TrainerBGMPlayPush TRAINER_YOUNGSTER_PEDRO
    // "Cheren saw potential in me and\nmade me a Trainer in this Gym![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_CherenSawPotentialMade, 1, 0, 0
    ActorMsgClose
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x128000, 30
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0533
    ActorCmdExec 255, Movement_0888
    ActorCmdWait
    VMJump L_0533

L_0533:
    ActorWalkRoute 255, 13, 18, 0, 8, 0
    VMSleep 20
    ActorCmdExec 1, Movement_08B8
    ActorCmdWait
    ActorCmdExec 255, Movement_08E0
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 255, Movement_0898
    ActorCmdExec 1, Movement_08A0
    ActorCmdWait
    CallTrainerBattle TRAINER_YOUNGSTER_PEDRO, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05B2
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x128000, 1
    EvCameraWait
    CallTrainerBattleEnd
    VMJump L_05B4

L_05B2:
    CallTrainerLose

L_05B4:
    WorkAdd EVENT_WORK_0x40a9, 1
    TrainerFlagSet TRAINER_YOUNGSTER_PEDRO
    VMStackPush EVENT_WORK_0x40a9
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_05E3
    // "Wow! You can challenge\nthe Gym Leader!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_WowCanChallengeGym, 1, 0, 0
    VMJump L_05EF

L_05E3:
    // "If you can defeat the girl, too,\nyou can challenge the Gym Leader!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_IfCanDefeatGirl, 1, 0, 0

L_05EF:
    LastKeyWait
    ActorMsgClose
    EvCameraMoveToDefault 10
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMJump L_063C

L_0603:
    VMStackPush EVENT_WORK_0x40a9
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_062C
    // "Wow! You can challenge\nthe Gym Leader!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_WowCanChallengeGym, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_063C

L_062C:
    // "If you can defeat the girl, too,\nyou can challenge the Gym Leader!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_IfCanDefeatGirl, 1, 0, 0
    LastKeyWait
    ActorMsgClose

L_063C:
    VMJump L_0652

L_0642:
    // "The reason Cheren saw potential\nin me was there was no one else...?[f000]븁\u0000\nNo way! Even if that is the case,\nI just have to get stronger!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_ReasonCherenSawPotential, 1, 0, 0
    LastKeyWait
    ActorMsgClose

L_0652:
    EvCameraRebind
    EvCameraEnd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_067B
    CallTrainerBattleEnd
    VMJump L_067D

L_067B:
    CallTrainerLose

L_067D:
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 0
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0821
    TrainerFlagGet TRAINER_LASS_SERENA, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07E2
    TrainerBGMPlayPush TRAINER_LASS_SERENA
    // "Now I'll show you all of the\nthings I learned from Cheren![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_NowIllShowAll, 2, 0, 0
    ActorMsgClose
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x98000, 30
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_070E
    ActorCmdExec 255, Movement_0888
    ActorCmdWait
    VMJump L_070E

L_070E:
    ActorWalkRoute 255, 17, 9, 0, 8, 0
    VMSleep 20
    ActorCmdExec 2, Movement_08C0
    ActorCmdWait
    ActorCmdExec 255, Movement_08D8
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 255, Movement_08A0
    ActorCmdExec 2, Movement_0898
    ActorCmdWait
    CallTrainerBattle TRAINER_LASS_SERENA, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_078D
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x98000, 1
    EvCameraWait
    CallTrainerBattleEnd
    VMJump L_078F

L_078D:
    CallTrainerLose

L_078F:
    WorkAdd EVENT_WORK_0x40a9, 1
    TrainerFlagSet TRAINER_LASS_SERENA
    VMStackPush EVENT_WORK_0x40a9
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_07C2
    // "OK. You're pretty good!\nBut, can you beat Cheren?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_OkYourePrettyGood, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_07D2

L_07C2:
    // "I guess you can battle a little!\nThink you can beat the other Trainer?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_GuessCanBattleLittle, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_07D2:
    EvCameraMoveToDefault 10
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMJump L_081B

L_07E2:
    VMStackPush EVENT_WORK_0x40a9
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_080B
    // "OK. You're pretty good!\nBut, can you beat Cheren?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_OkYourePrettyGood, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_081B

L_080B:
    // "I guess you can battle a little!\nThink you can beat the other Trainer?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_GuessCanBattleLittle, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_081B:
    VMJump L_0831

L_0821:
    // "I have to learn even more\nabout my Pokémon!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_HaveLearnEvenMore, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_0831:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    TrainerCardHasBadge 0x8008, 0
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_086C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you run out of Pokémon that can fight\nduring a Pokémon battle, you lose![f000]븁\u0000\nSo having a lot of Pokémon with you\nmight work to your advantage!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_IfRunOutPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0880

L_086C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hope you'll remember the wonderful\nmoment in which you received[f000]븀\u0000\nthat Badge forever."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityGym_Text_HopeYoullRememberWonderful, 0, 0
    LastKeyWait
    ActorMsgClose

L_0880:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0888:
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0898:
    Move 15, 1
    MoveEnd

Movement_08A0:
    Move 14, 1
    MoveEnd

Movement_08A8:
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd

Movement_08B8:
    Move 2, 1
    MoveEnd

Movement_08C0:
    Move 3, 1
    MoveEnd

Movement_08C8:
    Move 32, 1
    MoveEnd

Movement_08D0:
    Move 33, 1
    MoveEnd

Movement_08D8:
    Move 34, 1
    MoveEnd

Movement_08E0:
    Move 35, 1
    MoveEnd

Movement_08E8:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
