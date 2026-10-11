#include "asm/field_script.inc"
#include "text/script/floccesy_ranch_2.h"

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
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0083
    ActorSetGPos 0, 22, 2, 32, 1
    VMJump L_00CD

L_0083:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A8
    ActorSetGPos 0, 22, 3, 13, 1
    VMJump L_00CD

L_00A8:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CD
    ActorSetGPos 0, 47, 2, 13, 1
    VMJump L_00CD

L_00CD:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_011A
    ActorSetGPos 1, 43, 2, 46, 2
    ActorSetGPos 2, 43, 2, 45, 2
    ActorSetGPos 10, 44, 2, 46, 2
    VMJump L_015D

L_011A:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015D
    ActorSetGPos 1, 46, 2, 42, 1
    ActorSetGPos 2, 47, 2, 42, 1
    ActorSetGPos 10, 45, 2, 40, 3
    ActorSetGPos 3, 47, 2, 40, 2

L_015D:
    VMHalt

Script_3:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorSetGPos 0, 33, 2, 0x8022, 2
    WorkSub 0x8021, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 0
    BGMPlayPush SEQ_BGM_E_HUE
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Oh! Nice!\nYou've come here to toughen up![f000]븁\u0000\nAll right! Let's see how much\nstronger you've become! Come at me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OhNiceYouveCome, 0, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BF
    CallTrainerBattle TRAINER_RIVAL_4, 0, 0
    VMJump L_01E8

L_01BF:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E0
    CallTrainerBattle TRAINER_RIVAL_5, 0, 0
    VMJump L_01E8

L_01E0:
    CallTrainerBattle TRAINER_RIVAL_6, 0, 0

L_01E8:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_021F
    ActorSetGPos 0, 42, 2, 46, 0
    ActorSetGPos 255, 42, 2, 45, 1
    CallTrainerBattleEnd
    VMJump L_0221

L_021F:
    CallTrainerLose

L_0221:
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Not bad...[f000]븁\u0000\nYou're thinking about how to bring out\nyour Pokémon's strength.[f000]븁\u0000\nI should be able to count\non you for backup![f000]븁\u0000\nWhat are you doing here anyway?[f000]븁\u0000\nHuh?\nA Town Map?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_NotBadYoureThinking, 0, 0, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_ARDEMO_01
    // "[f000]Ā\u0001\u0000 handed over\nthe Town Map![f000]븁\u0000"
    SystemMsg FloccesyRanch2_Text_HandedOverTownMap, 0
    InfoMsgClose
    SEWait
    // "[f000]Ā\u0001\u0001: Tch...\nShe didn't have to do that...[f000]븁\u0000\nThanks to you, too. We just left,\nand you've already helped me out.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_TchSheDidntHave, 0, 0, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x02d7
    ActorAdd 2
    ActorWalkRoute 2, 44, 45, 1, 8, 1
    VMSleep 4
    ActorAdd 1
    ActorWalkRoute 1, 44, 46, 1, 8, 1
    ActorAdd 10
    ActorWalkRoute 10, 45, 46, 1, 8, 1
    VMSleep 24
    ActorCmdExec 255, Movement_0F3C
    ActorCmdExec 0, Movement_0F3C
    ActorCmdWait
    // "???: I thought it was lively around here!\nYou were having a Pokémon battle, huh?[f000]븀\u0000\nIsn't it nice to be young![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_ThoughtLivelyAroundHere, 1, 6, 0
    MsgWinCloseAll
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Who are you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_Who, 0, 4, 0
    MsgWinCloseAll
    // "???: Who am I?[f000]븁\u0000\nI'm the owner of this ranch!\nAnd this is my wife![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_WhoAmImOwner, 1, 6, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 2, 43, 45, 1, 8, 0
    ActorCmdWait
    // "Wife: After a Pokémon battle, you\nshould heal your Pokémon's HP, right?[f000]븀\u0000\nHere, I'll give you this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_WifeAfterPokemonBattle, 2, 5, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorWalkRoute 2, 43, 46, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 2, Movement_0F34
    ActorCmdWait
    // "And one for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_One, 2, 5, 0
    MsgWinCloseAll
    VMSleep 12
    ActorWalkRoute 2, 44, 45, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 2, Movement_0F34
    ActorCmdWait
    // "It's nice to have Potions when\nyou're far away from a Pokémon Center.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_ItsNiceHavePotions, 2, 5, 0
    MsgWinCloseAll
    // "Owner: By the way, you didn't happen\nto see a Herdier around here, did you?[f000]븁\u0000\nI can't figure out where it went.[f000]븁\u0000\nOur two Herdier are always together\nand this is the first time one has[f000]븀\u0000\nwandered off, so I'm a little worried...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OwnerByWayDidnt, 1, 6, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0F44
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: You're a little worried?\nAre you KIDDING me?![f000]븁\u0000\nYour Pokémon might be lost forever![f000]븁\u0000\nWhatever! I'll look!\n[f000]Ā\u0001\u0000! Help out![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_YoureLittleWorriedKidding, 0, 4, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0F34
    ActorCmdExec 0, Movement_040C
    ActorCmdWait
    ActorCmdExec 1, Movement_0F4C
    ActorCmdExec 255, Movement_0F3C
    ActorCmdWait
    // "Owner: Why did he get so mad?[f000]븁\u0000\nI think it's probably just\nplaying somewhere in the ranch.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OwnerWhyDidHe, 1, 6, 0
    MsgWinCloseAll
    // "Wife: I wonder...[f000]븁\u0000\nBy the way, dear, if your Pokémon\nget hurt, let me know.[f000]븀\u0000\nI'll make them feel better for you!"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_WifeWonderByWay, 2, 5, 0
    LastKeyWait
    MsgWinCloseAll
    ActorSetGPos 0, 25, 2, 44, 0
    WorkSetConst EVENT_WORK_0x40a7, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .byte 0x00
    VMNop
    VMStackSub
    VMNop2
    VMStackDiv
    DebugPrint 254
    VMNop
    VMStackMul
    VMHalt
    Move 12, 1
    Move 34, 1
    MoveEnd

Movement_040C:
    Move 18, 11
    MoveEnd

Script_4:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8022, 2
    ActorSetGPos 0, 21, 2, 42, 0
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 0
    VMSleep 24
    ActorCmdExec 255, Movement_0F2C
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Did Herdier...\nwander somewhere back here?[f000]븁\u0000\nLet's have a look![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_DidHerdierWanderSomewhere, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 21
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047C
    ActorCmdExec 0, Movement_04AC
    VMJump L_0484

L_047C:
    ActorCmdExec 0, Movement_04B8

L_0484:
    VMSleep 8
    ActorCmdExec 255, Movement_0F24
    ActorCmdWait
    ActorSetGPos 0, 22, 3, 22, 0
    WorkSetConst EVENT_WORK_0x40a7, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04AC:
    Move 19, 1
    Move 16, 12
    MoveEnd

Movement_04B8:
    Move 18, 1
    Move 16, 12
    MoveEnd

Script_5:
    ActorsPauseAll
    ActorSetGPos 0, 22, 3, 19, 0
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04FD
    ActorCmdExec 0, Movement_05BC
    VMSleep 40
    VMJump L_0541

L_04FD:
    VMStackPush 0x8022
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0522
    ActorCmdExec 0, Movement_05C8
    VMSleep 32
    VMJump L_0541

L_0522:
    VMStackPush 0x8022
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0541
    ActorCmdExec 0, Movement_05D4
    VMSleep 24

L_0541:
    ActorCmdExec 255, Movement_0F34
    ActorCmdWait
    // "Oh! Here! I'll share something\ngood with you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OhHereIllShare, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 22
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    WordSetLoadRivalName 1
    // "If your Pokémon is paralyzed,\nuse one of these on it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_IfPokemonParalyzedUse, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_05E0
    ActorCmdWait
    ActorCmdExec 0, Movement_0F3C
    ActorCmdWait
    // "Still, Pokémon don't just wander\noff on their own.[f000]븁\u0000\nIn a worst-case scenario,\nit might be involved in some trouble!"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_StillPokemonDontJust, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40a7, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_05BC:
    Move 16, 8
    Move 39, 1
    MoveEnd

Movement_05C8:
    Move 16, 7
    Move 39, 1
    MoveEnd

Movement_05D4:
    Move 16, 6
    Move 39, 1
    MoveEnd

Movement_05E0:
    Move 33, 1
    Move 161, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    PVPlay 507, 0
    // "Yawrp!"
    InfoMsg FloccesyRanch2_Text_Yawrp, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorSetGPos 0, 35, 2, 11, 3
    ActorCmdExec 0, Movement_06D0
    VMSleep 48
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_062E
    VMJump L_063C

L_062E:
    ActorCmdExec 255, Movement_0F34
    VMJump L_0665

L_063C:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_064F
    VMJump L_065D

L_064F:
    ActorCmdExec 255, Movement_0F24
    VMJump L_0665

L_065D:
    ActorCmdExec 255, Movement_0F24

L_0665:
    ActorCmdWait
    WordSetLoadRivalName 1
    // "Did you hear that just now?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_DidHearJustNow, 0, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_069F
    ActorCmdExec 0, Movement_06E0
    VMJump L_06A7

L_069F:
    ActorCmdExec 0, Movement_06EC

L_06A7:
    VMSleep 16
    ActorCmdExec 255, Movement_0F3C
    ActorCmdWait
    // "I'll check this area![f000]븁\u0000\nYou go deeper in the grove\nand look![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_IllCheckAreaGo, 0, 0, 0
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40a7, 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_06D0:
    Move 19, 6
    Move 17, 1
    Move 75, 1
    MoveEnd

Movement_06E0:
    Move 17, 1
    Move 19, 5
    MoveEnd

Movement_06EC:
    Move 19, 5
    MoveEnd

Script_7:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x338000, 0x2001f, 0x148000, 40
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 255, 51, 23, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0F24
    ActorCmdExec 3, Movement_09A4
    ActorCmdExec 16, Movement_0EFC
    ActorCmdWait
    EvCameraWait
    PVPlay 507, 0
    // "Herdier: Yaarrrp..."
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_HerdierYaarrrp, 3, 3, 0
    PVWait
    MsgWaitAdvance
    ActorMsgClose
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: That cry!\nYou found it![f000]븁\u0000\nWhat a relief![f000]븁\u0000\nOK, I'll go call its Trainer,\nso you stay here with it![f000]븁\u0000"
    InfoMsg FloccesyRanch2_Text_CryFoundWhatRelief, 1
    MsgWinCloseAll
    ActorCmdExec 16, Movement_0F2C
    ActorWalkRoute 0, 42, 12, 1, 4, 0
    ActorCmdWait
    // "???: Tch...\nYou little pest![f000]븁\u0000\nI'm a member of a group that strikes\nfear into the hearts of those who[f000]븀\u0000\nstand before it: Team Plasma![f000]븁\u0000\nEver heard of it?"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_TchLittlePestIm, 16, 5, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07B3
    // "That's right! We're the righteous group\nthat tried to conquer Unova two years[f000]븀\u0000\nback in order to liberate Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_ThatsRightWereRighteous, 16, 5, 0
    VMJump L_07BF

L_07B3:
    // "Really? We're the righteous group\nthat tried to conquer Unova two years[f000]븀\u0000\nback in order to liberate Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_ReallyWereRighteousGroup, 16, 5, 0

L_07BF:
    // "Whatever...[f000]븁\u0000\nFools will never understand us...[f000]븁\u0000\nStill...[f000]븁\u0000\nFirst I got lost chasing Herdier...\nand now some nosy kid caught me![f000]븁\u0000\nAll of this is your fault!\nTake this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_WhateverFoolsWillNever, 16, 5, 0
    MsgWinCloseAll
    ActorNew 51, 20, 1, 251, 110, 0
    VMSleep 4
    ActorWalkRoute 251, 51, 23, 1, 4, 1
    VMSleep 16
    SEPlay SEQ_SE_W003_01
    ActorCmdWait
    SEWait
    ActorDelete 251
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 348
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "OK! I'll use this opportunity\nto retreat for now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OkIllUseOpportunity, 16, 5, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 32
    ActorCmdExec 16, Movement_09B4
    VMSleep 20
    ActorCmdExec 255, Movement_0F2C
    ActorCmdWait
    ActorCmdExec 3, Movement_0A2C
    ActorCmdExec 255, Movement_0F24
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    PVPlay 507, 0
    // "Yap! Bwoof!"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_YapBwoof, 3, 3, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorSetGPos 0, 46, 2, 14, 1
    ActorSetGPos 1, 46, 2, 15, 1
    ActorCmdExec 1, Movement_0A5C
    VMSleep 4
    ActorCmdExec 0, Movement_0A6C
    VMSleep 80
    ActorCmdExec 255, Movement_0F2C
    ActorCmdWait
    WordSetLoadRivalName 1
    // "Owner: Herdier![f000]븁\u0000\nWhat made you come all\nthe way back here?[f000]븁\u0000\nWell, at any rate, I'm really grateful\nfor your help, you two![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OwnerHerdierWhatMade, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_0A8C
    VMSleep 48
    ActorCmdExec 0, Movement_0F3C
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: You're awfully calm\nabout this![f000]븁\u0000\nYour Pokémon might have\nbeen gone for good![f000]븁\u0000\nTake better care of it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_YoureAwfullyCalmAbout, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0A04
    VMSleep 8
    ActorCmdExec 1, Movement_0F34
    ActorCmdExec 255, Movement_0F34
    ActorCmdWait
    ActorDelete 0
    ActorDelete 16
    ActorCmdExec 255, Movement_0F2C
    ActorCmdExec 1, Movement_0F24
    ActorCmdWait
    // "Owner: Hmm...\nI wonder if something happened to him...[f000]븁\u0000\nIt's like he's afraid of\nlosing Pokémon...[f000]븁\u0000\nCome on, Herdier!\nEveryone's waiting! Let's go home![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OwnerHmmWonderIf, 1, 4, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 4
    ActorCmdExec 1, Movement_0AEC
    VMSleep 8
    ActorCmdExec 3, Movement_0ADC
    ActorCmdWait
    FadeExWait
    ActorSetGPos 1, 46, 2, 42, 1
    ActorSetGPos 2, 47, 2, 42, 1
    ActorSetGPos 10, 45, 2, 40, 3
    ActorSetGPos 3, 47, 2, 40, 2
    FadeEx 3, 16, 0, 4
    FadeExWait
    WorkSetConst EVENT_WORK_0x40a7, 5
    WorkSetConst EVENT_WORK_0x40a5, 3
    FlagSet EVENT_FLAG_0x02d9
    FlagSet EVENT_FLAG_0x02d8
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_09A4:
    Move 71, 1
    Move 10, 1
    Move 72, 1
    MoveEnd

Movement_09B4:
    Move 19, 1
    Move 17, 7
    Move 18, 6
    Move 16, 7
    Move 18, 6
    MoveEnd
    WorkAnd 1, 17
    DebugPrint 18
    VMReturn
    VMStackPushFlag EVENT_FLAG_0x000e
    PokePartyGetSpecies 0, 34
    VMNop
    FlagSet 0
    MsgWinCloseAll
    VMNop2
    VMStackCmp CMP_EQ
    PokePartyGetSpecies 0, 17
    VMStackPop 19
    VMReturn
    Move 32, 0
    MoveEnd

Movement_0A04:
    Move 13, 2
    Move 14, 4
    Move 12, 7
    Move 14, 5
    MoveEnd
    VMStackSub
    VMHalt
    VMStackDiv
    VMHalt
    VMStackMul
    VMHalt
    Move 33, 0
    MoveEnd

Movement_0A2C:
    Move 13, 2
    Move 15, 2
    Move 14, 2
    Move 15, 1
    Move 33, 0
    MoveEnd
    VMStackSub
    VMHalt
    VMStackDiv
    VMHalt
    VMStackMul
    VMHalt
    VMStackDiv
    VMHalt
    Move 33, 0
    MoveEnd

Movement_0A5C:
    Move 13, 12
    Move 15, 5
    Move 12, 2
    MoveEnd

Movement_0A6C:
    Move 13, 13
    Move 15, 4
    Move 12, 2
    MoveEnd
    VMStackDiv
    VMHalt
    Move 13, 4
    Move 34, 0
    MoveEnd

Movement_0A8C:
    Move 15, 1
    Move 13, 3
    Move 34, 0
    Move 50, 0
    MoveEnd
    VMStackMul
    VMNop2
    VMStackSub
    VMHalt
    VMStackDiv
    VMNop2
    VMStackSub
    VMHalt
    Move 34, 0
    MoveEnd
    Move 15, 1
    Move 33, 0
    MoveEnd
    Move 14, 1
    Move 33, 0
    MoveEnd
    Move 14, 1
    Move 32, 0
    MoveEnd

Movement_0ADC:
    Move 14, 1
    Move 13, 1
    Move 14, 4
    MoveEnd

Movement_0AEC:
    Move 35, 0
    Move 13, 1
    Move 14, 4
    MoveEnd

Script_8:
    ActorsPauseAll
    WordSetLoadRivalName 1
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B2E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Herdier, where did you go?"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_HerdierWhereDidGo, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0BAF

L_0B2E:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B5B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wow, this ranch is really big!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_WowRanchReallyBig, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0BAF

L_0B5B:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B88
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Still, Pokémon don't just wander\noff on their own.[f000]븁\u0000\nIn a worst-case scenario,\nit might be involved in some trouble!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_StillPokemonDontJust, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0BAF

L_0B88:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BAF
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Look deeper in the grove!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_LookDeeperGrove, 0, 0
    LastKeyWait
    ActorMsgClose

L_0BAF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01e2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0BF8
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Owner: Thanks! It's all thanks\nto you and your Pokémon![f000]븁\u0000\nYou're really great! Hey, is that it?\nDid Alder train you?"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OwnerThanksItsAll, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01e2
    VMJump L_0C49

L_0BF8:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01e2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0C35
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This ranch started when a fence\nwas made to protect Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_RanchStartedWhenFence, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0C49

L_0C35:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Owner: It is strange for Herdier\nto wander off on its own.[f000]븁\u0000\nIt always plays with the other\none or works on the ranch..."
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_OwnerStrangeHerdierWander, 0, 0
    LastKeyWait
    ActorMsgClose

L_0C49:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x010c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0C92
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You and your Pokémon\nfound Herdier! Great!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_PokemonFoundHerdierGreat, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x010c
    VMJump L_0D69

L_0C92:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    PokePartyGetCount 0x8023, 0

L_0CBC:
    VMStackPush 0x8023
    VMStackPush 0x8024
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0D10
    PokePartyIsFullHP 0x8025, 0x8024
    PokePartyIsFullPP 0x8026, 0x8024
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0D04
    WorkAdd 0x8027, 1

L_0D04:
    WorkAdd 0x8024, 1
    VMJump L_0CBC

L_0D10:
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0D5B
    // "You and your Pokémon\nlook a little worn out...[f000]븁\u0000\nRest here a minute--you won't\nget anywhere all tired like that![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_PokemonLookLittleWorn, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkSetConst EVENT_WORK_0x4001, 1
    VMJump L_0D69

L_0D5B:
    // "Yup! You and your Pokémon\nare full of energy!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_YupPokemonFullEnergy, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0D69:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Ba woof! Bawoof!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_BaWoofBawoof, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DE8
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Bawoof! Ba woof!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_BawoofBaWoof, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_0E04

L_0DE8:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Herdier: Bawoo..."
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_HerdierBawoo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose

L_0E04:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 179, 0
    // "Baaah!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_Baaah, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 179, 0
    // "Baa!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_Baa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 179, 0
    // "Baawn!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_Baawn, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 179, 0
    // "Baa baa!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_BaaBaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 179, 0
    // "Baa haa!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_BaaHaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 179, 0
    // "Ba baaa!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyRanch2_Text_BaBaaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_0EFC:
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

Movement_0F24:
    Move 32, 1
    MoveEnd

Movement_0F2C:
    Move 33, 1
    MoveEnd

Movement_0F34:
    Move 34, 1
    MoveEnd

Movement_0F3C:
    Move 35, 1
    MoveEnd

Movement_0F44:
    Move 75, 1
    MoveEnd

Movement_0F4C:
    Move 159, 1
    MoveEnd
