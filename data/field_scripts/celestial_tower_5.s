#include "asm/field_script.inc"
#include "text/script/celestial_tower_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F0
    // "I can't believe those\nthree down there lost...[f000]븁\u0000\nEven so, you're not getting past me![f000]븁\u0000\nYou're fighting for that couple,\nand I'm fighting for myself![f000]븀\u0000\nYou know which is stronger, right?![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower5_Text_CantBelieveThoseThree, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_MAID_TAMMY, 0, 0
    VMCall L_0117
    // "I see...[f000]븁\u0000\nWe were thinking only about ourselves.\nWe sure weren't thinking about the[f000]븀\u0000\nPokémon at our sides, were we?"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower5_Text_SeeWeWereThinking, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ee, 5
    FlagSet EVENT_FLAG_0x0337
    FlagSet EVENT_FLAG_0x0338
    FlagSet EVENT_FLAG_0x0339
    FlagSet EVENT_FLAG_0x033a
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00B6
    ActorWalkRoute 2, 11, 16, 1, 8, 1
    ActorCmdWait
    VMJump L_00D6

L_00B6:
    ActorWalkRoute 2, 12, 10, 1, 8, 0
    ActorCmdWait
    ActorWalkRoute 2, 11, 16, 1, 8, 1
    ActorCmdWait

L_00D6:
    ActorWalkRoute 2, 11, 19, 1, 8, 1
    ActorCmdWait
    ActorDelete 2
    VMJump L_0111

L_00F0:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0111
    // "I see...[f000]븁\u0000\nWe were thinking only about ourselves.\nWe sure weren't thinking about the[f000]븀\u0000\nPokémon at our sides, were we?"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower5_Text_SeeWeWereThinking, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0111:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0117:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0136
    CallTrainerBattleEnd
    VMJump L_0138

L_0136:
    CallTrainerLose

L_0138:
    VMReturn
    .balign 4, 0
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
