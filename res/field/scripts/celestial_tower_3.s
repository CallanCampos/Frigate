#include "asm/field_script.inc"
#include "text/script/celestial_tower_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x0338
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0053
    ActorSetGPos 2, 14, 0, 7, 1

L_0053:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0107
    // "I'm invincible now!\n'Cause I'm full of hate![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_ImInvincibleNowCause, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_NURSERY_AIDE_ILSE, 0, 0
    VMCall L_01AB
    // "Why... How come?[f000]븁\u0000\nAll I wanted to do was raise the same\nPokémon as he did and battle together!"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_WhyHowComeAll, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ee, 3
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00E7
    ActorWalkRoute 2, 14, 9, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 2, Movement_024C
    ActorCmdWait
    VMJump L_0101

L_00E7:
    ActorWalkRoute 2, 14, 7, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 2, Movement_0254
    ActorCmdWait

L_0101:
    VMJump L_0128

L_0107:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0128
    // "Why... How come?[f000]븁\u0000\nAll I wanted to do was raise the same\nPokémon as he did and battle together!"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_WhyHowComeAll, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0128:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerFlagGet TRAINER_SCHOOL_KID_ALBERTA, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0197
    TrainerBGMPlayPush TRAINER_SCHOOL_KID_ALBERTA
    // "Thanks to my little Litwick's light,\nmy victory is coming into view![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_ThanksLittleLitwicksLight, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_SCHOOL_KID_ALBERTA, 0, 0
    VMCall L_01AB
    WorkSetConst EVENT_WORK_0x40ef, 1
    FlagSet EVENT_FLAG_0x0155
    FlagSet EVENT_FLAG_0x033b
    FlagReset EVENT_FLAG_0x033c
    TrainerFlagSet TRAINER_SCHOOL_KID_ALBERTA
    // "Losing a battle is so draining.\nI feel really burned out somehow."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_LosingBattleDrainingFeel, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01A5

L_0197:
    // "Losing a battle is so draining.\nI feel really burned out somehow."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_LosingBattleDrainingFeel, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01A5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01AB:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CA
    CallTrainerBattleEnd
    VMJump L_01CC

L_01CA:
    CallTrainerLose

L_01CC:
    VMReturn

Script_4:
    ActorsPauseAll
    TrainerBGMPlayPush TRAINER_SCHOOL_KID_ALBERTA
    ActorCmdExec 3, Movement_026C
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 1
    ActorWalkRoute 3, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_024C
    ActorCmdWait
    // "Thanks to my little Litwick's light,\nmy victory is coming into view![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_ThanksLittleLitwicksLight, 3, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_SCHOOL_KID_ALBERTA, 0, 0
    VMCall L_01AB
    WorkSetConst EVENT_WORK_0x40ef, 1
    FlagSet EVENT_FLAG_0x0155
    FlagSet EVENT_FLAG_0x033b
    FlagReset EVENT_FLAG_0x033c
    TrainerFlagSet TRAINER_SCHOOL_KID_ALBERTA
    // "Losing a battle is so draining.\nI feel really burned out somehow."
    ActorMsg MSGFILE_SCRIPT, CelestialTower3_Text_LosingBattleDrainingFeel, 3, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_024C:
    Move 32, 1
    MoveEnd

Movement_0254:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_026C:
    Move 189, 1
    MoveEnd
