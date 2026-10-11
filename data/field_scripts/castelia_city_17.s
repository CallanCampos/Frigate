#include "asm/field_script.inc"
#include "text/script/castelia_city_17.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    ActorCmdExec 2, Movement_01DC
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006B
    WorkSub 0x8022, 1
    ActorWalkRoute 2, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 2, Movement_01CC
    ActorCmdWait
    VMJump L_0081

L_006B:
    WorkSub 0x8022, 1
    ActorWalkRoute 2, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait

L_0081:
    // "Oh! You are the Pokémon Trainer\nI met at the entrance![f000]븁\u0000\nYou've come this far. That means\nyou must be quite strong.[f000]븁\u0000\nAre you prepared to face the strongest\nperson in this building?[f000]븁\u0000\nThen, the time has come to challenge\nmy grandson, the company president![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity17_Text_OhPokemonTrainerMet, 2, 5, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorCmdExec 2, Movement_019C
    ActorCmdExec 1, Movement_01BC
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E0
    WorkSub 0x8022, 1
    ActorWalkRoute 1, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 1, Movement_01CC
    ActorCmdWait
    VMJump L_00F6

L_00E0:
    WorkSub 0x8022, 1
    ActorWalkRoute 1, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait

L_00F6:
    // "I'll show you just how much\nthis Battle Company has researched[f000]븀\u0000\nPokémon and Trainers![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity17_Text_IllShowJustHow, 1, 3, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_SCHOOL_KID_NEIL, 0, 0
    VMCall L_013E
    // "I lost...[f000]븁\u0000\nBecause you were strong,\nand I was weak.[f000]븁\u0000\nWe must do even more research\nfor Pokémon and for Trainers!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity17_Text_LostBecauseWereStrong, 1, 3, 0
    MsgWaitAdvance
    MsgWinCloseAll
    // "Thinking that when you win\nit's because of your Pokémon[f000]븀\u0000\nand that when you lose[f000]븀\u0000\nit's because of yourself[f000]븀\u0000\nmight make you stronger...[f000]븁\u0000\nBut enjoying yourself\nis much, much more important!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity17_Text_ThinkingWhenWinIts, 2, 5, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40fd, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_013E:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015D
    CallTrainerBattleEnd
    VMJump L_015F

L_015D:
    CallTrainerLose

L_015F:
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you want to make a weak\nPokémon stronger, you should[f000]븀\u0000\nuse that Exp. Share.[f000]븁\u0000\nAny Pokémon that holds it receives\nExp. Points, even when it doesn't[f000]븀\u0000\nparticipate in battle!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity17_Text_IfWantMakeWeak, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My dream as president is to\nrelease a device that lets[f000]븀\u0000\npeople talk to Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity17_Text_DreamPresidentReleaseDevice, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_019C:
    Move 15, 1
    Move 33, 1
    MoveEnd
    VMStackMul
    VMNop2
    RTReserveScript 1
    PokePartyGetSpecies 0, 15
    VMHalt
    .byte 0xfe
    .balign 4, 0

Movement_01BC:
    Move 14, 2
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_01CC:
    Move 33, 1
    MoveEnd
    Move 75, 1
    MoveEnd

Movement_01DC:
    Move 33, 1
    Move 75, 1
    MoveEnd
