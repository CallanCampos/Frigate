#include "asm/field_script.inc"
#include "text/script/humilau_city_gym.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 7
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    VMCall L_009C
    VMJump L_0096

L_0051:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0080
    // "Uihaa!"
    ActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_Uihaa, 0, 0, 0
    MsgWaitAdvance
    ActorMsgClose
    VMCall L_0250
    VMJump L_0096

L_0080:
    // "Waves can be rough or calm,\nbut it's still the same sea![f000]븁\u0000\nEh, there're lots of ways\nto look at the same thing!"
    ActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_WavesCanRoughCalm, 0, 0, 0
    MsgWaitAdvance
    ActorMsgClose
    VMCall L_0250

L_0096:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_009C:
    // "Sup! Here already, huh?[f000]븁\u0000\nYou look strong!\nShoots! Let's start![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_SupHereAlreadyHuh, 0, 0
    ActorMsgClose
    WorkSetConst 0x8023, 0
    GameGetDifficulty 0x8023
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D3
    CallTrainerBattle TRAINER_LEADER_MARLON_2, 0, 0
    VMJump L_00DB

L_00D3:
    CallTrainerBattle TRAINER_LEADER_MARLON, 0, 0

L_00DB:
    WorkSetConst 0x8023, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0100
    CallTrainerBattleEnd
    VMJump L_0102

L_0100:
    CallTrainerLose

L_0102:
    // "Marlon: You don't just look\nstrong, you're strong fo' reals![f000]븁\u0000\nEh, I was swept away, too![f000]븁\u0000\nOh yeah, yo. I was so surprised that\nI forgot! I gotta give this to you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_MarlonDontJustLook, 0, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 7
    TrainerCardAddBadge 7
    WordSetPlayerName 0
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x8024, 0
    TrainerCardGetSex 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0144
    PlayFieldEffect 10
    VMJump L_0148

L_0144:
    PlayFieldEffect 62

L_0148:
    MEWait
    WorkSetConst 0x8024, 0
    // "[f000]Ā\u0001\u0000 received the Wave Badge\nfrom Marlon![f000]븁\u0000"
    SystemMsg HumilauCityGym_Text_ReceivedWaveBadgeFrom, 0
    InfoMsgClose
    // "That's the Wave Badge,\nthe Unova region's new[f000]븀\u0000\nGym Badge! Pretty sweet, right?[f000]븁\u0000\nNow you got all eight Badges,\nso you can be tight with any Pokémon![f000]븁\u0000\nOh yeah, got a TM for you, too![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_ThatsWaveBadgeUnova, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 382
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "So Scald sometimes burns\nthe target, 'K.[f000]븁\u0000\nOh, and you can even use\nit when you're all frozen and[f000]븀\u0000\nchillin' and stuff![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_ScaldSometimesBurnsTarget, 0, 0
    // "Shoots! I'm off then!\nHope it's useful![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_ShootsImOffThen, 0, 0
    ActorMsgClose
    TrainerFlagSet TRAINER_ACE_TRAINER_DOYLE
    TrainerFlagSet TRAINER_ACE_TRAINER_ENZIO
    TrainerFlagSet TRAINER_ACE_TRAINER_SANTINO
    TrainerFlagSet TRAINER_ACE_TRAINER_MELINA
    TrainerFlagSet TRAINER_ACE_TRAINER_JEANNE
    TrainerFlagSet TRAINER_ACE_TRAINER_SABLE
    FlagSet EVENT_FLAG_0x0975
    WorkSetConst EVENT_WORK_0x40df, 1
    WorkSetConst EVENT_WORK_0x40e3, 1
    FlagReset EVENT_FLAG_0x032a
    FlagSet EVENT_FLAG_0x03ea
    HollowRivalCmd_0262 1, 27
    PlayerGetDir 0x8020
    ActorCmdExec 0, Movement_0434
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FD
    ActorCmdExec 255, Movement_043C
    VMJump L_022C

L_01FD:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_022C
    VMSleep 4
    ActorCmdExec 255, Movement_049C

L_022C:
    ActorCmdWait
    ActorJumpToGPos 0, 16, 65532, 1
    Cmd_02A2
    ActorCmdExec 0, Movement_0454
    ActorCmdWait
    VMSleep 35
    Cmd_02A3
    VMSleep 80
    VMReturn

L_0250:
    PlayerGetDir 0x8020
    ActorCmdExec 0, Movement_0434
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027D
    ActorCmdExec 255, Movement_043C
    VMJump L_02AC

L_027D:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02AC
    VMSleep 4
    ActorCmdExec 255, Movement_049C

L_02AC:
    ActorCmdWait
    ActorJumpToGPos 0, 16, 65532, 1
    Cmd_02A2
    ActorCmdExec 0, Movement_0454
    ActorCmdWait
    VMSleep 35
    Cmd_02A3
    VMSleep 80
    VMReturn

Script_4:
    ActorsPauseAll
    ActorCmdExec 7, Movement_0310
    VMSleep 16
    ActorCmdExec 255, Movement_04A4
    ActorCmdWait
    // "If you're looking for the Gym Leader,\nhe went swimming off into the ocean[f000]븀\u0000\nyelling about the sea![f000]븁\u0000\nPlease look for him if you'd like."
    ActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_IfYoureLookingGym, 7, 0, 0
    MsgWaitAdvance
    ActorMsgClose
    ActorCmdExec 7, Movement_04A4
    ActorCmdExec 255, Movement_045C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0310:
    Move 32, 1
    Move 75, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x411f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you're looking for the Gym Leader,\nhe went swimming off into the ocean[f000]븀\u0000\nyelling about the sea![f000]븁\u0000\nPlease look for him if you'd like."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_IfYoureLookingGym, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_03EF

L_034B:
    TrainerCardHasBadge 0x8008, 7
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DB
    VMStackPushFlag EVENT_FLAG_0x0075
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This Gym may feel like a resort,\nbut the Gym Leader's no picnic![f000]븁\u0000\nThis is a present from me.\nPlease focus and prepare![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_GymMayFeelLike, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "In Humilau's Pokémon Gym,\nyou proceed by hopping on the lily pads[f000]븀\u0000\nand sliding across the water's surface.[f000]븁\u0000\nHere's another piece of advice![f000]븁\u0000\nWater-type Pokémon really don't\nlike Electric- or Grass-type moves![f000]븁\u0000\nBut I'm sure the Gym Leader\nhas planned for that!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_HumilausPokemonGymProceed, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x0075
    VMJump L_03D5

L_03C1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In Humilau's Pokémon Gym,\nyou proceed by hopping on the lily pads[f000]븀\u0000\nand sliding across the water's surface.[f000]븁\u0000\nHere's another piece of advice![f000]븁\u0000\nWater-type Pokémon really don't\nlike Electric- or Grass-type moves![f000]븁\u0000\nBut I'm sure the Gym Leader\nhas planned for that!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_HumilausPokemonGymProceed, 0, 0
    LastKeyWait
    ActorMsgClose

L_03D5:
    VMJump L_03EF

L_03DB:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Marlon's swimming around, isn't he..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityGym_Text_MarlonsSwimmingAroundIsnt, 0, 0
    LastKeyWait
    ActorMsgClose

L_03EF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    TrainerCardHasBadge 0x8008, 7
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0425
    // "Humilau City Pokémon Gym[f000]븁\u0000\nGym Leader: Marlon\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0001"
    InfoMsg HumilauCityGym_Text_HumilauCityPokemonGym, 2
    VMJump L_042A

L_0425:
    // "Humilau City Pokémon Gym[f000]븁\u0000\nGym Leader: Marlon\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg HumilauCityGym_Text_HumilauCityPokemonGym_2, 2

L_042A:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0434:
    Move 16, 2
    MoveEnd

Movement_043C:
    Move 3, 1
    Move 71, 1
    Move 18, 1
    Move 72, 1
    Move 32, 1
    MoveEnd

Movement_0454:
    Move 69, 1
    MoveEnd

Movement_045C:
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

Movement_049C:
    Move 32, 1
    MoveEnd

Movement_04A4:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
