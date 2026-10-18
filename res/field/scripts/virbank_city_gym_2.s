#include "asm/field_script.inc"
#include "text/script/virbank_city_gym_2.h"

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
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    TrainerCardHasBadge 0x8008, 1
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B3
    VMStackPushFlag EVENT_FLAG_0x010e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007C
    WorkSetConst EVENT_WORK_0x4000, 1
    WorkSetConst EVENT_WORK_0x4003, 1
    VMJump L_0088

L_007C:
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst EVENT_WORK_0x4003, 0

L_0088:
    VMStackPushFlag EVENT_FLAG_0x010f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A7
    WorkSetConst EVENT_WORK_0x4001, 1
    VMJump L_00AD

L_00A7:
    WorkSetConst EVENT_WORK_0x4001, 0

L_00AD:
    VMJump L_00BF

L_00B3:
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst EVENT_WORK_0x4001, 0

L_00BF:
    VMStackPushFlag EVENT_FLAG_0x02fb
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EA
    WorkSetConst EVENT_WORK_0x4000, 1
    WorkSetConst EVENT_WORK_0x4001, 1
    WorkSetConst EVENT_WORK_0x4002, 1
    WorkSetConst EVENT_WORK_0x4003, 1

L_00EA:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0103
    Cmd_0292 1

L_0103:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011A
    Cmd_0292 2

L_011A:
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0157
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "It's too loud!\nShe can't hear your voice!"
    InfoMsg VirbankCityGym2_Text_ItsTooLoudShe, 2
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01CD

L_0157:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0478
    TrainerCardHasBadge 0x8008, 1
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0188
    VMCall L_01D3
    VMJump L_01CD

L_0188:
    Cmd_0291 0
    Cmd_0291 1
    Cmd_0291 2
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BD
    // "Your Pokémon WANTED this win![f000]븁\u0000\nKeep on going on like this,\nand do all sorts of stuff!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_PokemonWantedWinKeep, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01CD

L_01BD:
    // "Using the Pokémon I like and\ngetting through to people with[f000]븀\u0000\nthe music I like is who I am![f000]븁\u0000\nIf you can do what you enjoy\nin the way you want, then great![f000]븁\u0000\nJust be sure not to cause\nanyone any trouble!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_UsingPokemonLikeGetting, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_01CD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01D3:
    // "Get ready! I'm gonna knock\nsome sense outta ya![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_GetReadyImGonna, 0, 0, 1
    MsgWinCloseAll
    WorkSetConst 0x8024, 0
    GameGetDifficulty 0x8024
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020C
    CallTrainerBattle TRAINER_LEADER_ROXIE_2, 0, 0
    VMJump L_0214

L_020C:
    CallTrainerBattle TRAINER_LEADER_ROXIE, 0, 0

L_0214:
    WorkSetConst 0x8024, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B2
    FlagSet EVENT_FLAG_0x02fb
    FlagReset EVENT_FLAG_0x02d1
    ActorDelete 0
    ActorAdd 4
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0258
    VMJump L_0266

L_0258:
    ActorCmdExec 4, Movement_0744
    VMJump L_02A8

L_0266:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0279
    VMJump L_0287

L_0279:
    ActorCmdExec 4, Movement_075C
    VMJump L_02A8

L_0287:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_029A
    VMJump L_02A8

L_029A:
    ActorCmdExec 4, Movement_0754
    VMJump L_02A8

L_02A8:
    ActorCmdWait
    CallTrainerBattleEnd
    VMJump L_02B4

L_02B2:
    CallTrainerLose

L_02B4:
    VMCall L_0478
    Cmd_0291 0
    Cmd_0291 1
    Cmd_0291 2
    // "Sigh!\nWhat are you doing losing, Roxie?![f000]븁\u0000\nWell...\nI guess that means you're strong![f000]븁\u0000\nThis stinks,\nbut I gave it everything I had, and[f000]븀\u0000\nI feel revitalized and refreshed now![f000]븁\u0000\nHere!\nProof that you beat me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_SighWhatDoingLosing, 4, 0, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 1
    TrainerCardAddBadge 1
    WordSetPlayerName 0
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x8025, 0
    TrainerCardGetSex 0x8025
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030A
    PlayFieldEffect 4
    VMJump L_030E

L_030A:
    PlayFieldEffect 56

L_030E:
    MEWait
    WorkSetConst 0x8025, 0
    // "[f000]Ā\u0001\u0000 received the\nToxic Badge from Roxie![f000]븁\u0000"
    SystemMsg VirbankCityGym2_Text_ReceivedToxicBadgeFrom, 0
    InfoMsgClose
    // "Two Badges![f000]븁\u0000\nNow Pokémon up to Lv. 30,\neven Pokémon you got in trades,[f000]븀\u0000\nwill realize how good you are[f000]븀\u0000\nand won't ignore your commands![f000]븁\u0000\nAlso, here!\nUse this TM![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_TwoBadgesNowPokemon, 4, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 336
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "TM09 is Venoshock!\nIt covers the target in[f000]븀\u0000\na special poisonous liquid![f000]븁\u0000\nEven better, if your target's already\npoisoned, it does double damage![f000]븀\u0000\nHeh heh! It almost packs too much[f000]븀\u0000\nof a punch!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_Tm09VenoshockCoversTarget, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    TrainerFlagSet TRAINER_ROUGHNECK_NICKY
    TrainerFlagSet TRAINER_GUITARIST_BILLY_JO
    WorkSetConst EVENT_WORK_0x40ad, 1
    WorkSetConst EVENT_WORK_0x410a, 1
    WorkSetConst EVENT_WORK_0x40ac, 3
    FlagSet EVENT_FLAG_0x02d5
    FlagSet EVENT_FLAG_0x096f
    VMReturn

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Your Pokémon WANTED this win![f000]븁\u0000\nKeep on going on like this,\nand do all sorts of stuff!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_PokemonWantedWinKeep, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 178
    VMStackPushFlag EVENT_FLAG_0x010e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CF
    VMCall L_04F9
    VMCall L_049D
    VMJump L_0404

L_03CF:
    VMCall L_050F
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FE
    CallTrainerBattleEnd
    VMCall L_049D
    FlagSet EVENT_FLAG_0x010e
    VMJump L_0400

L_03FE:
    CallTrainerLose

L_0400:
    TrainerFlagSet 0x8020

L_0404:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 179
    VMStackPushFlag EVENT_FLAG_0x010f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_043D
    VMCall L_04F9
    VMCall L_04D0
    VMJump L_0472

L_043D:
    VMCall L_050F
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046C
    CallTrainerBattleEnd
    VMCall L_04D0
    FlagSet EVENT_FLAG_0x010f
    VMJump L_046E

L_046C:
    CallTrainerLose

L_046E:
    TrainerFlagSet 0x8020

L_0472:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0478:
    ISSSwitchQuery 0x8010, 3
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_049B
    ISSSwitchDisable 3
    WorkSetConst EVENT_WORK_0x4002, 1

L_049B:
    VMReturn

L_049D:
    ISSSwitchQuery 0x8010, 1
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04CE
    ISSSwitchDisable 1
    ISSSwitchDisable 4
    Cmd_0292 1
    WorkSetConst EVENT_WORK_0x4000, 1
    WorkSetConst EVENT_WORK_0x4003, 1

L_04CE:
    VMReturn

L_04D0:
    ISSSwitchQuery 0x8010, 2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F7
    ISSSwitchDisable 2
    Cmd_0292 2
    WorkSetConst EVENT_WORK_0x4001, 1

L_04F7:
    VMReturn

L_04F9:
    TrainerGetMessageTypes 0x8021, 0x8022, 0x8023
    TrainerSayMessage 0x8020, 0x8022, 0x8011
    LastKeyWait
    ActorMsgClose
    VMReturn

L_050F:
    TrainerGetMessageTypes 0x8021, 0x8022, 0x8023
    TrainerSayMessage 0x8020, 0x8021, 0x8011
    ActorMsgClose
    CallTrainerBattle 0x8020, 0, 0
    VMReturn

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That's a Gym Leader for you!\nShe really brings out the charms[f000]븀\u0000\nof her Pokémon![f000]븁\u0000\nBut she's too wrapped up\nin what she's doing here..."
    ParentActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_ThatsGymLeaderShe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm running the PA![f000]븁\u0000\nI balance the sound in the venue\nso it's easy to hear![f000]븁\u0000\nAre you a Trainer?[f000]븁\u0000\nSo does that mean you're thinking about\nthe type balance of the Pokémon in[f000]븀\u0000\nyour party?"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_ImRunningPaBalance, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    PlayerGetGPos 0x8026, 0x8027
    ActorSetGPos 3, 13, 0, 6, 2
    BGMPlay SEQ_BGM_PW_LAND
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05A8
    ActorCmdExec 3, Movement_06C0
    VMJump L_05B0

L_05A8:
    ActorCmdExec 3, Movement_06C8

L_05B0:
    ActorCmdWait
    ActorCmdExec 255, Movement_077C
    ActorCmdWait
    // "Hey you![f000]븁\u0000\nI felt like you were something special\nduring your battle with Roxie.[f000]븁\u0000\nPlease come with me to Pokéstar Studios![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_HeyFeltLikeWere, 3, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05EF
    ActorCmdExec 3, Movement_06D0
    VMSleep 10
    VMJump L_05FB

L_05EF:
    ActorCmdExec 3, Movement_06D8
    VMSleep 10

L_05FB:
    BGMChangeMap
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0622
    VMSleep 30
    ActorCmdExec 4, Movement_06E0
    VMJump L_062E

L_0622:
    VMSleep 30
    ActorCmdExec 4, Movement_06F8

L_062E:
    ActorCmdWait
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0649
    VMJump L_0653

L_0649:
    ActorCmdExec 255, Movement_0774
    ActorCmdWait

L_0653:
    // "You're going to Pokéstar Studios?![f000]븁\u0000\nOH! I forgot! The old man!\nI have to get him back to work[f000]븀\u0000\nas captain![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym2_Text_YoureGoingPokestarStudios, 4, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0682
    ActorCmdExec 4, Movement_0710
    VMJump L_0696

L_0682:
    ActorCmdExec 4, Movement_0718
    VMSleep 20
    ActorCmdExec 255, Movement_077C

L_0696:
    ActorCmdWait
    ActorDelete 3
    ActorDelete 4
    WorkSetConst EVENT_WORK_0x40ad, 2
    FlagSet EVENT_FLAG_0x02d1
    FlagSet EVENT_FLAG_0x02d0
    FlagSet EVENT_FLAG_0x02d5
    WorkSetConst EVENT_WORK_0x40ac, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_06C0:
    Move 14, 9
    MoveEnd

Movement_06C8:
    Move 14, 8
    MoveEnd

Movement_06D0:
    Move 15, 10
    MoveEnd

Movement_06D8:
    Move 15, 9
    MoveEnd

Movement_06E0:
    Move 14, 1
    Move 13, 1
    Move 14, 3
    Move 13, 2
    Move 34, 1
    MoveEnd

Movement_06F8:
    Move 14, 1
    Move 13, 1
    Move 14, 4
    Move 13, 2
    Move 35, 1
    MoveEnd

Movement_0710:
    Move 15, 10
    MoveEnd

Movement_0718:
    Move 13, 1
    Move 15, 10
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_0744:
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd

Movement_0754:
    Move 2, 1
    MoveEnd

Movement_075C:
    Move 3, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_0774:
    Move 34, 1
    MoveEnd

Movement_077C:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
