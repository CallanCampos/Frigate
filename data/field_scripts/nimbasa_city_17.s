#include "asm/field_script.inc"
#include "text/script/nimbasa_city_17.h"

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
    ScriptEntriesEnd

Script_1:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    VMStackPushFlag EVENT_FLAG_0x0179
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0075
    WorkSetConst 0x8020, 1
    VMJump L_007B

L_0075:
    WorkSetConst 0x8020, 0

L_007B:
    VMStackPushFlag EVENT_FLAG_0x017a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009A
    WorkSetConst 0x8021, 1
    VMJump L_00A0

L_009A:
    WorkSetConst 0x8021, 0

L_00A0:
    Cmd_017B 0x8020, 0x8021
    VMHalt

Script_13:
    VMStackPushFlag EVENT_FLAG_0x0179
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C7
    ActorSetGPos 1, 17, 2, 17, 2

L_00C7:
    VMStackPushFlag EVENT_FLAG_0x017a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E6
    ActorSetGPos 0, 51, 2, 42, 2

L_00E6:
    VMHalt

Script_14:
    FieldGetContinueFlag 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0101
    Cmd_024D

L_0101:
    VMHalt

Script_10:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0118
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0118:
    Move 56, 1
    Move 34, 1
    MoveEnd

Script_11:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0138
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0138:
    Move 57, 1
    MoveEnd

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 16
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0173
    VMCall L_018B
    VMJump L_0179

L_0173:
    VMCall L_0195

L_0179:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_018B:
    FlagReset EVENT_FLAG_0x037f
    ActorAdd 1
    VMReturn

L_0195:
    FlagReset EVENT_FLAG_0x0380
    ActorAdd 0
    VMReturn

Script_2:
    ActorsPauseAll
    Cmd_017C 0
    VMCall L_01E7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    Cmd_017C 1
    VMCall L_01E7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    Cmd_017C 2
    VMCall L_01E7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    Cmd_017C 3
    VMCall L_01E7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01E7:
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 pressed the switch.[f000]븁\u0000\nThe roller coaster's path\nhas been changed!"
    SystemMsg NimbasaCity17_Text_PressedSwitchRollerCoasters, 2
    LastKeyWait
    InfoMsgClose
    VMReturn

Script_6:
    ActorsPauseAll
    ActorCmdExec 255, Movement_03B0
    ActorCmdWait
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    PlayerGetGPos 0x8024, 0x8025
    VMStackPush 0x8024
    VMStackPushConst 16
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0233
    VMCall L_024B
    VMJump L_0239

L_0233:
    VMCall L_026D

L_0239:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_024B:
    ActorCmdExec 255, Movement_03C0
    ActorCmdExec 1, Movement_03B8
    ActorCmdWait
    // "Hey there!\nAre you cool riding a roller coaster?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_HeyThereCoolRiding, 1, 0, 0
    ActorMsgClose
    VMReturn

L_026D:
    ActorCmdExec 255, Movement_03C0
    ActorCmdExec 0, Movement_03B8
    ActorCmdWait
    // "Your roller-coaster ride is finally\nnearing its finale.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_RollerCoasterRideFinally, 0, 0, 0
    ActorMsgClose
    VMReturn

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    PlayerGetGPos 0x8026, 0x8027
    VMStackPush 0x8026
    VMStackPushConst 16
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C6
    VMCall L_02E2
    Cmd_017D 1
    VMJump L_02D0

L_02C6:
    VMCall L_0349
    Cmd_017D 2

L_02D0:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_02E2:
    TrainerBGMPlayPush TRAINER_RICH_BOY_ROLAN
    ActorCmdExec 1, Movement_03D0
    ActorCmdWait
    // "I'm going to overwhelm you with the speed\nI learned riding the roller coaster![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_ImGoingOverwhelmSpeed, 1, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_RICH_BOY_ROLAN, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0325
    CallTrainerBattleEnd
    VMJump L_032B

L_0325:
    FlagSet EVENT_FLAG_0x037f
    CallTrainerLose

L_032B:
    // "Your way of battling...\nIt's elegant! You've got style![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_WayBattlingItsElegant, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_03D8
    ActorCmdWait
    FlagSet EVENT_FLAG_0x0179
    VMReturn

L_0349:
    TrainerBGMPlayPush TRAINER_LADY_COLETTE
    ActorCmdExec 0, Movement_03D0
    ActorCmdWait
    // "I'm also a Pokémon Trainer who was\ntoughened up by Ms. Elesa.[f000]븀\u0000\nI won't give up easily![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_ImAlsoPokemonTrainer, 0, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_LADY_COLETTE, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_038C
    CallTrainerBattleEnd
    VMJump L_0392

L_038C:
    FlagSet EVENT_FLAG_0x0380
    CallTrainerLose

L_0392:
    // "It pleases me to be the opponent of a\nstrong and honorable Trainer like you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_PleasesOpponentStrongHonorable, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_03D8
    ActorCmdWait
    FlagSet EVENT_FLAG_0x017a
    VMReturn

Movement_03B0:
    Move 75, 1
    MoveEnd

Movement_03B8:
    Move 57, 1
    MoveEnd

Movement_03C0:
    Move 71, 1
    Move 17, 2
    Move 72, 1
    MoveEnd

Movement_03D0:
    Move 13, 1
    MoveEnd

Movement_03D8:
    Move 2, 1
    Move 71, 1
    Move 15, 1
    Move 72, 1
    MoveEnd

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you press the switches, the\nroller coaster's path will change."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_IfPressSwitchesRoller, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In other places, roller coasters\nare called jet coasters."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_OtherPlacesRollerCoasters, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01cb
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0453
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So, I hear the Gym Leader\nlikes the thrill of this roller coaster."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_HearGymLeaderLikes, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_049B

L_0453:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh my! Did you come here\nlooking for the Gym Leader?[f000]븁\u0000\nI'm sorry, you just missed her.\nShe just left for the Gym.[f000]븁\u0000\nTake this for making it\nall the way here![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_OhDidComeHere, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 22
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "So, I hear the Gym Leader\nlikes the thrill of this roller coaster."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_HearGymLeaderLikes, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01cb
    FlagSet EVENT_FLAG_0x029d

L_049B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "To ride this roller coaster,\nthe first step is to get in the car.[f000]븁\u0000\nNext comes the platform!\nThere, you can change[f000]븀\u0000\nwhere the coaster is going![f000]븁\u0000\nSometimes you continue by riding\nthe cars of opponents you defeat.[f000]븀\u0000\nThat's how you aim for the back!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_RideRollerCoasterFirst, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Elesa's really amazing![f000]븁\u0000\nUsually, you just remodel the Gym,\nbut she built a completely new one![f000]븁\u0000\nElesa's really amazing!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_ElesasReallyAmazingUsually, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Faster! Faster!\nA speed boost makes you feel great!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity17_Text_FasterFasterSpeedBoost, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
