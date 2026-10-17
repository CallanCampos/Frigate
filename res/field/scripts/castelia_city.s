#include "asm/field_script.inc"
#include "text/script/castelia_city.h"

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
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_23:
    VMHalt

Script_22:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    ActorGetRailPos 255, 0x8021, 0x8022, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 11
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00A1
    ActorCmdExec 0, Movement_0A0C
    ActorCmdWait

L_00A1:
    // "Iris: Hey, c'mon![f000]븁\u0000\nThumb Pier is past here![f000]븁\u0000\nIf you mention a suspicious\nplace in Castelia City, the[f000]븀\u0000\nonly places that come to mind[f000]븀\u0000\nare Narrow Street and here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_IrisHeyCmonThumb, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8023
    VMStackPushConst 11
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00D4
    DebugPrint 300
    ActorCmdExec 0, Movement_010C
    VMJump L_00DC

L_00D4:
    ActorCmdExec 0, Movement_0114

L_00DC:
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40b1, 2
    FlagSet EVENT_FLAG_0x02ef
    FlagReset EVENT_FLAG_0x0406
    ActorAdd 12
    ActorDelete 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_010C:
    Move 15, 12
    MoveEnd

Movement_0114:
    Move 15, 8
    MoveEnd

Script_9:
    ActorsPauseAll
    // "Iris: This way! This way!\nC'mon! Have a look![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_IrisWayWayCmon, 12, 0, 0
    MsgWinCloseAll
    ActorCmdExec 12, Movement_015C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 12
    SEWait
    WorkSetConst EVENT_WORK_0x40b1, 3
    FlagSet EVENT_FLAG_0x0406
    FlagReset EVENT_FLAG_0x02f1
    WorkSetConst EVENT_WORK_0x40b2, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_015C:
    Move 13, 8
    MoveEnd

Script_20:
    ActorsPauseAll
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    ActorCmdExec 11, Movement_0A2C
    ActorCmdWait
    PlayerGetRailPos 0x8024, 0x8025, 0x8026
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01CB
    ActorCmdExec 11, Movement_08D8
    VMJump L_03CD

L_01CB:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_020C
    ActorCmdExec 11, Movement_08E4
    VMJump L_03CD

L_020C:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_024D
    ActorCmdExec 11, Movement_08F0
    VMJump L_03CD

L_024D:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_028E
    ActorCmdExec 11, Movement_08FC
    VMJump L_03CD

L_028E:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02CF
    ActorCmdExec 11, Movement_0908
    VMJump L_03CD

L_02CF:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 28
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0310
    ActorCmdExec 11, Movement_0914
    VMJump L_03CD

L_0310:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 27
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0351
    ActorCmdExec 11, Movement_091C
    VMJump L_03CD

L_0351:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 26
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0392
    ActorCmdExec 11, Movement_0928
    VMJump L_03CD

L_0392:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 25
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03CD
    ActorCmdExec 11, Movement_0934

L_03CD:
    ActorCmdWait
    // "Welcome to Castelia City!!\nIt's the Castelia Harlequin Hunt![f000]븁\u0000\nI bet this is your first visit to\nCastelia City.[f000]븁\u0000\nThat's great!\nHere, have this Bicycle![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_WelcomeCasteliaCityIts, 11, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 450
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "The Castelia Harlequin Hunt's\nrules are simple![f000]븁\u0000\nAll you have to do is visit specific\nplaces and talk to the Harlequin there![f000]븀\u0000\nWhether you participate is up to you![f000]븀\u0000\nThe places to look are...[f000]븁\u0000\nThe Medal Office![f000]븁\u0000\nPasserby Analytics HQ![f000]븁\u0000\nAnd the Battle Company![f000]븁\u0000\nRide the Bicycle I gave\nyou, and go, go, go!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_CasteliaHarlequinHuntsRules, 11, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_044E
    ActorCmdExec 11, Movement_0940
    VMJump L_0650

L_044E:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_048F
    ActorCmdExec 11, Movement_0950
    VMJump L_0650

L_048F:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04D0
    ActorCmdExec 11, Movement_0960
    VMJump L_0650

L_04D0:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0511
    ActorCmdExec 11, Movement_0970
    VMJump L_0650

L_0511:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0552
    ActorCmdExec 11, Movement_0980
    VMJump L_0650

L_0552:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 28
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0593
    ActorCmdExec 11, Movement_0990
    VMJump L_0650

L_0593:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 27
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05D4
    ActorCmdExec 11, Movement_099C
    VMJump L_0650

L_05D4:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 26
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0615
    ActorCmdExec 11, Movement_09AC
    VMJump L_0650

L_0615:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 25
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0650
    ActorCmdExec 11, Movement_09BC

L_0650:
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40e2, 2
    MedalDiscover 19
    FlagSet EVENT_FLAG_ARRIVED_CASTELIA_CITY
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_07E7
    // "Free-for-all! It's the Castelia\nHarlequin Hunt! You haven't visited...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_FreeAllItsCastelia, 0, 0
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06F6
    // "You still need to visit\nthis many places: Wow! Zero![f000]븁\u0000\nThat means you've completed\nthe Castelia Harlequin Hunt![f000]븁\u0000\nCongratulations!\nThis is a small commemorative gift![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_StillNeedVisitMany_2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 50
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "The Castelia Harlequin Hunt is a way\nto make more people love Castelia City![f000]븁\u0000\nThat's why we generously gave you a\nBicycle at the beginning. It's the best[f000]븀\u0000\nway to get around Castelia City![f000]븁\u0000\nKeep loving Castelia City![f000]븁\u0000\nCastelia City, Castelia City,\nCastelia City! ♪ Here we go! ♪"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_CasteliaHarlequinHuntWay, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40e2, 6
    VMJump L_07E1

L_06F6:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

L_0702:
    VMStackPush 0x8028
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_07CC
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0138
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0750
    // "The Medal Office![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_MedalOffice, 11, 0, 0
    WorkAdd 0x8027, 1
    VMJump L_07C0

L_0750:
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0139
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_078B
    // "Passerby Analytics HQ![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_PasserbyAnalyticsHq, 11, 0, 0
    WorkAdd 0x8027, 1
    VMJump L_07C0

L_078B:
    VMStackPush 0x8028
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x013a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07C0
    // "The Battle Company![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_BattleCompany, 11, 0, 0
    WorkAdd 0x8027, 1

L_07C0:
    WorkAdd 0x8028, 1
    VMJump L_0702

L_07CC:
    WordSetNumber 0, 0x8027, 1
    // "You still need to visit\nthis many places: [f000]Ȁ\u0001\u0000![f000]븁\u0000\nSo explore Castelia City, and enjoy\nthe Castelia Harlequin Hunt!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_StillNeedVisitMany, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_07E1:
    VMJump L_07F5

L_07E7:
    // "The Castelia Harlequin Hunt is a way\nto make more people love Castelia City![f000]븁\u0000\nThat's why we generously gave you a\nBicycle at the beginning. It's the best[f000]븀\u0000\nway to get around Castelia City![f000]븁\u0000\nKeep loving Castelia City![f000]븁\u0000\nCastelia City, Castelia City,\nCastelia City! ♪ Here we go! ♪"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_CasteliaHarlequinHuntWay, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_07F5:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Castelia City\nA City of Grandeur"
    MsgPlaceSign CasteliaCity_Text_CasteliaCityCityGrandeur, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Castelia City\nA City of Grandeur"
    MsgPlaceSign CasteliaCity_Text_CasteliaCityCityGrandeur, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Ahead: Castelia Street\nOcean Side: Cruise Dock"
    MsgPlaceSign CasteliaCity_Text_AheadCasteliaStreetOcean, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Ahead: Mode Street\nOcean Side: Prime Pier"
    MsgPlaceSign CasteliaCity_Text_AheadModeStreetOcean, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Ahead: Narrow Street\nOcean Side: Unity Pier"
    MsgPlaceSign CasteliaCity_Text_AheadNarrowStreetOcean, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Ahead: Pokémon Gym\nOcean Side: Liberty Pier"
    MsgPlaceSign CasteliaCity_Text_AheadPokemonGymOcean, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Passerby Analytics HQ\n...People pass by as they walk along."
    MsgPlaceSign CasteliaCity_Text_PasserbyAnalyticsHqPeople, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Battle Company\nTalented Trainers Welcome!"
    MsgPlaceSign CasteliaCity_Text_BattleCompanyTalentedTrainers, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_08D8:
    Move 14, 5
    Move 13, 2
    MoveEnd

Movement_08E4:
    Move 14, 4
    Move 13, 2
    MoveEnd

Movement_08F0:
    Move 14, 3
    Move 13, 2
    MoveEnd

Movement_08FC:
    Move 14, 2
    Move 13, 2
    MoveEnd

Movement_0908:
    Move 14, 1
    Move 13, 2
    MoveEnd

Movement_0914:
    Move 13, 2
    MoveEnd

Movement_091C:
    Move 15, 1
    Move 13, 2
    MoveEnd

Movement_0928:
    Move 15, 2
    Move 13, 2
    MoveEnd

Movement_0934:
    Move 15, 3
    Move 13, 2
    MoveEnd

Movement_0940:
    Move 12, 2
    Move 15, 5
    Move 33, 1
    MoveEnd

Movement_0950:
    Move 12, 2
    Move 15, 4
    Move 33, 1
    MoveEnd

Movement_0960:
    Move 12, 2
    Move 15, 3
    Move 33, 1
    MoveEnd

Movement_0970:
    Move 12, 2
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_0980:
    Move 12, 2
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_0990:
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_099C:
    Move 12, 2
    Move 14, 1
    Move 33, 1
    MoveEnd

Movement_09AC:
    Move 12, 2
    Move 14, 2
    Move 33, 1
    MoveEnd

Movement_09BC:
    Move 12, 2
    Move 14, 3
    Move 33, 1
    MoveEnd
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

Movement_0A0C:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0A2C:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "To live surrounded by people,\ndo you have to grow to like everyone,[f000]븀\u0000\nor do you only have to like yourself?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_LiveSurroundedByPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder what Castelia\nwas like before it got this big."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_WonderWhatCasteliaLike, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's wonderful how the Pokémon\nand women here are so full of life!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_ItsWonderfulHowPokemon, 6, 1, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Battle Company needs\npeople for focus testing.[f000]븁\u0000\nTrainers who are confident in\ntheir abilities should come try it!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_BattleCompanyNeedsPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Seasons and trends always pass..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_SeasonsTrendsAlwaysPass, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "...When I fall asleep in my bed.[f000]븁\u0000\n...When my Pokémon's attack\nhits critically![f000]븁\u0000\nMy life is full of things\nthat bring me joy!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_WhenFallAsleepBed, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm a battling Clerk ⑭!\nIf I don't give up, I'll win someday!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_ImBattlingClerkIf, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Everyone's faces blend together!\nIs that because I'm tired?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_EveryonesFacesBlendTogether, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you're tired, rest!\nDon't force yourself to be energetic!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_WhenYoureTiredRest, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Should I change my Pokémon's moves\nor its held items?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity_Text_ShouldChangePokemonsMoves, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
