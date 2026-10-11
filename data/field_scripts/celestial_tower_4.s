#include "asm/field_script.inc"
#include "text/script/celestial_tower_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 4
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x0339
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_004B
    ActorSetGPos 1, 17, 0, 23, 0

L_004B:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FF
    // "Not hearing the peal of the bell\nmeans no happily ever after...[f000]븀\u0000\nIf we keep it from ringing,[f000]븀\u0000\nthen those two will never find happiness.[f000]븁\u0000\nThat's why I can't let you\ngo one step further![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower4_Text_NotHearingPealBell, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_CLERK_F_LANA, 0, 0
    VMCall L_0126
    // "People who don't give any love\nshouldn't ask for any...[f000]븀\u0000\nfrom people or Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower4_Text_PeopleWhoDontGive, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ee, 4
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00DF
    ActorWalkRoute 1, 17, 21, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 1, Movement_0154
    ActorCmdWait
    VMJump L_00F9

L_00DF:
    ActorWalkRoute 1, 17, 23, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 1, Movement_014C
    ActorCmdWait

L_00F9:
    VMJump L_0120

L_00FF:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 4
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0120
    // "People who don't give any love\nshouldn't ask for any...[f000]븀\u0000\nfrom people or Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower4_Text_PeopleWhoDontGive, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0120:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0126:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0145
    CallTrainerBattleEnd
    VMJump L_0147

L_0145:
    CallTrainerLose

L_0147:
    VMReturn
    .balign 4, 0

Movement_014C:
    Move 32, 1
    MoveEnd

Movement_0154:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
