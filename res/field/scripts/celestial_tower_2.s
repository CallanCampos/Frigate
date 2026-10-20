#include "asm/field_script.inc"
#include "text/script/celestial_tower_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x0337
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0057
    ActorSetGPos 1, 15, 0, 24, 1

L_0057:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 5
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C0
    // "Professor Juniper: Hi there!\nHow were things with Skyla?[f000]븁\u0000\nOh? You still haven't\nearned the Gym Badge yet?[f000]븁\u0000\nWell, if that's the case,\nI'll keep up the field work[f000]븀\u0000\nuntil the plane is ready to fly.[f000]븁\u0000\nOh yeah!\nWhy don't you try using this?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_ProfessorJuniperHiThere, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 231
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Professor Juniper: Giving that Lucky Egg\nto a Pokémon to hold increases the[f000]븀\u0000\namount of Exp. Points received in[f000]븀\u0000\nbattle a little bit![f000]븁\u0000\nHaving strong Pokémon will make\nit easier to fill your Pokédex pages!"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_ProfessorJuniperGivingLucky, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkAdd EVENT_WORK_0x40c2, 1
    VMJump L_0271

L_00C0:
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0250
    // "Professor Juniper: Celestial Tower...\nIt's a giant memorial...[f000]븁\u0000\nI wonder if this building was built\nin a place with many Ghost- and[f000]븀\u0000\nPsychic-type Pokémon or if those[f000]븀\u0000\nPokémon gathered here because it[f000]븀\u0000\nwas built.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_ProfessorJuniperCelestialTower_2, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0114
    ActorCmdExec 0, Movement_02B8
    VMJump L_0171

L_0114:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0135
    ActorCmdExec 0, Movement_02D8
    VMJump L_0171

L_0135:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0156
    ActorCmdExec 0, Movement_0278
    VMJump L_0171

L_0156:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0171
    ActorCmdExec 0, Movement_0298

L_0171:
    ActorCmdWait
    // "Professor Juniper: Oh, right!\nHow were things with Skyla?[f000]븁\u0000\nOh my! You won the Jet Badge![f000]븁\u0000\nWell, the plane should be ready\nto fly, then![f000]븁\u0000\nThanks for coming to get me!\nTake this as thanks![f000]븀\u0000\nTry using it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_ProfessorJuniperOhRight, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 231
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Professor Juniper: Giving that Lucky Egg\nto a Pokémon to hold increases the[f000]븀\u0000\namount of Exp. Points received in[f000]븀\u0000\nbattle by a little bit![f000]븁\u0000\nHaving strong Pokémon will make\nit easier to fill your Pokédex pages![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_ProfessorJuniperGivingLucky_2, 0, 0
    // "OK! I'll be waiting for you\nin Mistralton City![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_OkIllWaitingMistralton, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 16
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0210
    ActorWalkRoute 0, 16, 15, 1, 8, 0
    ActorCmdWait
    ActorWalkRoute 0, 16, 27, 1, 8, 1
    VMSleep 16
    ActorCmdExec 255, Movement_0434
    ActorCmdWait
    VMJump L_022C

L_0210:
    ActorWalkRoute 0, 17, 27, 1, 8, 1
    VMSleep 8
    ActorCmdExec 255, Movement_0434
    ActorCmdWait

L_022C:
    ActorDelete 0
    WorkAdd EVENT_WORK_0x40c2, 1
    FlagSet EVENT_FLAG_0x02fe
    FlagReset EVENT_FLAG_0x0300
    HollowRivalCmd_0262 0, 3
    HollowRivalCmd_0262 1, 18
    VMJump L_0271

L_0250:
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0271
    // "Professor Juniper: Celestial Tower...\nIt's a giant memorial...[f000]븁\u0000\nI wonder if this building was built\nin a place with many Ghost- and[f000]븀\u0000\nPsychic-type Pokémon or if those[f000]븀\u0000\nPokémon gathered here because it[f000]븀\u0000\nwas built."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_ProfessorJuniperCelestialTower, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0271:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0278:
    Move 35, 1
    Move 62, 1
    Move 33, 1
    Move 62, 1
    Move 32, 1
    Move 63, 1
    Move 34, 1
    MoveEnd

Movement_0298:
    Move 34, 1
    Move 62, 1
    Move 33, 1
    Move 62, 1
    Move 32, 1
    Move 63, 1
    Move 35, 1
    MoveEnd

Movement_02B8:
    Move 34, 1
    Move 62, 1
    Move 33, 1
    Move 62, 1
    Move 35, 1
    Move 63, 1
    Move 32, 1
    MoveEnd

Movement_02D8:
    Move 34, 1
    Move 62, 1
    Move 32, 1
    Move 62, 1
    Move 35, 1
    Move 63, 1
    Move 33, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03AA
    // "You! You came here at the request of\nthe couple in Humilau City, didn't you?![f000]븁\u0000\nNo need for a reply!\nIf you want to ring the bell,[f000]븀\u0000\nyou'll have to battle with me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_CameHereRequestCouple, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WAITRESS_JAN, 0, 0
    VMCall L_0409
    // "Win or lose...\nI'm a Waitress...[f000]븁\u0000\nI carry the customers' orders\nwith a heaping side of love..."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_WinLoseImWaitress, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ee, 2
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 15
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 24
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_038A
    ActorWalkRoute 1, 15, 26, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 1, Movement_042C
    ActorCmdWait
    VMJump L_03A4

L_038A:
    ActorWalkRoute 1, 15, 24, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 1, Movement_0434
    ActorCmdWait

L_03A4:
    VMJump L_03CB

L_03AA:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_03CB
    // "Win or lose...\nI'm a Waitress...[f000]븁\u0000\nI carry the customers' orders\nwith a heaping side of love..."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_WinLoseImWaitress, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03CB:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "At the very top of the Tower,\nthere's a big bell.[f000]븁\u0000\nI've heard that when you\nring it, it pleases the spirits."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_VeryTopTowerTheres, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is Celestial Tower, where Pokémon\nare laid to rest..."
    ParentActorMsg MSGFILE_SCRIPT, CelestialTower2_Text_CelestialTowerWherePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0409:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0428
    CallTrainerBattleEnd
    VMJump L_042A

L_0428:
    CallTrainerLose

L_042A:
    VMReturn

Movement_042C:
    Move 32, 1
    MoveEnd

Movement_0434:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
