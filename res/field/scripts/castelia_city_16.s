#include "asm/field_script.inc"
#include "text/script/castelia_city_16.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 232
    WorkSet 0x8001, 1
    WorkSet 0x8002, 143
    WorkSet 0x8003, 0
    WorkSet 0x8004, 1
    WorkSet 0x8005, 1
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x00d2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F5
    // "Oh! A company tour?\nAnyway, let's have a battle![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_OhCompanyTourAnyway, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_CLERK_M_CLEMENS, 0, 0
    VMCall L_0109
    // "What power! I'm moved,\nso I'll give you this present!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_WhatPowerImMoved, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 15
    WorkSet 0x8001, 2
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "A Quick Ball makes it easier to catch a\nPokémon if you use it at the very[f000]븀\u0000\nbeginning of a battle."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_QuickBallMakesEasier, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x00d2
    VMJump L_0103

L_00F5:
    // "A Quick Ball makes it easier to catch a\nPokémon if you use it at the very[f000]븀\u0000\nbeginning of a battle."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_QuickBallMakesEasier, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0103:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0109:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0128
    CallTrainerBattleEnd
    VMJump L_012A

L_0128:
    CallTrainerLose

L_012A:
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x016a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A7
    // "Did you come for Pokémon practice?\nI'll be happy to help you out![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_DidComePokemonPractice, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_CLERK_M_WARREN, 0, 0
    VMCall L_0109
    // "With skills like that,\nyou can get the most out of these!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_SkillsLikeCanGet, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 10
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "A Timer Ball makes it easier to catch\na Pokémon you've been battling[f000]븀\u0000\nfor a long time!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_TimerBallMakesEasier, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x016a
    VMJump L_01B5

L_01A7:
    // "A Timer Ball makes it easier to catch\na Pokémon you've been battling[f000]븀\u0000\nfor a long time!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity16_Text_TimerBallMakesEasier, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01B5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
