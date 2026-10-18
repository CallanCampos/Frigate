#include "asm/field_script.inc"
#include "text/script/castelia_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

L_003A:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0061
    ObjInitWarpGPos 2, 9, 0, 11
    VMJump L_006B

L_0061:
    ObjInitWarpGPos 1, 9, 0, 11

L_006B:
    VMReturn

Script_5:
    VMCall L_003A
    VMHalt

Script_8:
    VMCall L_003A
    VMHalt

Script_6:
    VMHalt

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x032e
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 2
    SEWait
    BGMPlay SEQ_BGM_E_BERU
    TrainerCardGetSex 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C1
    // "Bianca: Heeey![f000]븁\u0000"
    InfoMsg CasteliaGate_Text_BiancaHeeey, 2
    VMJump L_00C6

L_00C1:
    // "Bianca: Hey there![f000]븁\u0000"
    InfoMsg CasteliaGate_Text_BiancaHeyThere, 2

L_00C6:
    MsgWinCloseAll
    PlayerGetGPos 0x8022, 0x8023
    WorkAdd 0x8023, 1
    ActorWalkRoute 2, 0x8022, 0x8023, 1, 8, 0
    VMSleep 8
    ActorCmdExec 255, Movement_0368
    ActorCmdWait
    // "When I saw you in the city,\nI just had to catch up with you![f000]븁\u0000\nHere, take this!\nThis is a Dowsing Machine![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_WhenSawCityJust, 2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 471
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "You can use the Dowsing Machine\nto find places where items are hidden.[f000]븁\u0000\nIt's exciting to find an item while\nyou're looking for a Pokémon.[f000]븁\u0000\nOoh, good luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_CanUseDowsingMachine, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0340
    ActorCmdWait
    // "Oh![f000]븁\u0000\nIf you often use the Dowsing Machine,\nthe Habitat List, and so on,[f000]븀\u0000\nwhy don't you register them?[f000]븁\u0000\nEr...\nI think it's written in this book...[f000]븁\u0000\nI found it!\nOK. I'll read it.[f000]븁\u0000\n“You can use the registered item\njust by pressing the Y Button!\"[f000]븀\u0000\nSee? OK. Bye![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_OhIfOftenUse, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 5, 14, 1, 8, 0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 2
    SEWait
    BGMChangeMap
    FlagSet EVENT_FLAG_0x032e
    WorkSetConst EVENT_WORK_0x40ed, 1
    MedalDiscover 60
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokémon Breeder on Route 4 past\nthis gate always challenges Trainers[f000]븀\u0000\nto battle when she sees them.[f000]븁\u0000\nJust what you expect from Route 4, which\nhas ruins. Discovery is so exciting!"
    // "The Pokémon Breeder on Route 4 past\nthis gate always challenges Trainers[f000]븀\u0000\nto battle when she sees them.[f000]븁\u0000\nJust what you expect from Route 4,\nwhich has a lot of buildings.[f000]븀\u0000\nChanges are so exciting!"
    ActorMsgVersioned 1024, CasteliaGate_Text_PokemonBreederRoute4, CasteliaGate_Text_PokemonBreederRoute4_2, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nimbasa City is at the end\nof Route 4."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_NimbasaCityEndRoute, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_031F
    // "Free-for-all! It's the Castelia\nHarlequin Hunt! You haven't visited...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_FreeAllItsCastelia, 0, 0
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_022E
    // "You still need to visit\nthis many places: Wow! Zero![f000]븁\u0000\nThat means you've completed\nthe Castelia Harlequin Hunt![f000]븁\u0000\nCongratulations!\nThis is a small commemorative gift![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_StillNeedVisitMany_2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 50
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "The Castelia Harlequin Hunt is a way\nto make more people love Castelia City![f000]븁\u0000\nThat's why we generously gave you a\nBicycle at the beginning. It's the best[f000]븀\u0000\nway to get around Castelia City![f000]븁\u0000\nKeep loving Castelia City![f000]븁\u0000\nCastelia City, Castelia City,\nCastelia City! ♪ Here we go! ♪"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_CasteliaHarlequinHuntWay, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40e2, 6
    VMJump L_0319

L_022E:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

L_023A:
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0304
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0138
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0288
    // "The Medal Office![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_MedalOffice, 3, 0, 0
    WorkAdd 0x8024, 1
    VMJump L_02F8

L_0288:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0139
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02C3
    // "Passerby Analytics HQ![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_PasserbyAnalyticsHq, 3, 0, 0
    WorkAdd 0x8024, 1
    VMJump L_02F8

L_02C3:
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x013a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02F8
    // "The Battle Company![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_BattleCompany, 3, 0, 0
    WorkAdd 0x8024, 1

L_02F8:
    WorkAdd 0x8025, 1
    VMJump L_023A

L_0304:
    WordSetNumber 0, 0x8024, 1
    // "You still need to visit\nthis many places: [f000]Ȁ\u0001\u0000![f000]븁\u0000\nSo explore Castelia City, and enjoy\nthe Castelia Harlequin Hunt!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_StillNeedVisitMany, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0319:
    VMJump L_032D

L_031F:
    // "The Castelia Harlequin Hunt is a way\nto make more people love Castelia City![f000]븁\u0000\nThat's why we generously gave you a\nBicycle at the beginning. It's the best[f000]븀\u0000\nway to get around Castelia City![f000]븁\u0000\nKeep loving Castelia City![f000]븁\u0000\nCastelia City, Castelia City,\nCastelia City! ♪ Here we go! ♪"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaGate_Text_CasteliaHarlequinHuntWay, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_032D:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0340:
    Move 13, 1
    Move 75, 1
    Move 32, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_0368:
    Move 33, 1
    MoveEnd
