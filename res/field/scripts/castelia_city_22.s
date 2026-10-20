#include "asm/field_script.inc"
#include "text/script/castelia_city_22.h"

// Script plugin 16, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8023, 0
    PokePartyFindEx 648, 1, 0x8022, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008E
    PokePartyFindEx 648, 0, 0x8022, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x00db
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_008E
    WorkSetConst 0x8023, 2

L_008E:
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_020D
    // "The scent you brought me reminded me of\na long-ago melody from the fringes of my[f000]븀\u0000\nsepia-toned memories.[f000]븁\u0000\nIt was a song my mother loved...\nIt makes me feel so nostalgic.[f000]븁\u0000\nWould you like to hear the melody?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_ScentBroughtRemindedLong, 0, 0
    ActorMsgClose
    BGMPlay SEQ_BGM_SILENCE_FIELD
    VMCall L_0254
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x58000, 0, 0x68000, 30
    EvCameraWait
    VMCall L_0270
    WordSetPartyPokeName 0, 0x8022
    // "[f000]Ă\u0001\u0000 popped out of\nthe Poké Ball![f000]븁\u0000"
    InfoMsg CasteliaCity22_Text_PoppedOutPokeBall, 2
    InfoMsgClose_0039
    VMCall L_02ED
    BGMPlay SEQ_BGM_E_INISHIE
    ActorCmdExec 0x8011, Movement_03B4
    Plugin16_Cmd1005 251
    DebugPrint 1
    ActorCmdExec 251, Movement_0418
    VMSleep 129
    EvCameraMoveTo 9569, 0, 0xe9cf8, 0x53000, 0x2a000, 0x60000, 120
    VMSleep 437
    VMSleep 8
    ActorCmdWait
    DebugPrint 2
    ActorCmdExec 251, Movement_0424
    ActorCmdExec 0x8011, Movement_03BC
    VMSleep 261
    ActorCmdWait
    DebugPrint 3
    ActorCmdExec 251, Movement_042C
    ActorCmdExec 0x8011, Movement_03C4
    Plugin16_Cmd1006
    DebugPrint 4
    ActorCmdWait
    DebugPrint 5
    BGMWait
    BGMChangeMap
    PlayFieldEffect 33
    ActorCmdExec 251, Movement_0438
    ActorCmdWait
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019B
    WordSetPartyPokeName 0, 0x8022
    // "[f000]Ă\u0001\u0000 remembered the\nRelic Song it had forgotten![f000]븁\u0000"
    SystemMsg CasteliaCity22_Text_RememberedRelicSongHad, 2
    InfoMsgClose

L_019B:
    Plugin16_Cmd1007
    EvCameraMoveTo 9688, 0, 0xed000, 0x58000, 0, 0x78000, 25
    EvCameraWait
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D6
    VMCall L_0284
    VMJump L_01F9

L_01D6:
    VMCall L_0335
    FlagSet EVENT_FLAG_0x00db
    VMStackPushFlag EVENT_FLAG_0x09fd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F9
    WorkSetConst EVENT_WORK_0x400a, 1

L_01F9:
    // "When you came through the door,\nI remembered the melody![f000]븁\u0000\nAnd the Pokémon that had forgotten how\nto dance began to step lightly once more![f000]븁\u0000\nYou are the inspiration.\nYou are a mysterious child..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_WhenCameThroughDoor, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0213

L_020D:
    VMCall L_0219

L_0213:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0219:
    WorkSetConst 0x8024, 0
    VMStackPushFlag EVENT_FLAG_0x00db
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023E
    WorkSetConst 0x8024, 4
    VMJump L_0244

L_023E:
    WorkSetConst 0x8024, 0

L_0244:
    ParentActorMsg MSGFILE_SCRIPT, 0x8024, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_0254:
    ActorWalkRoute 255, 5, 9, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0448
    ActorCmdWait
    VMReturn

L_0270:
    ActorCmdExec 3, Movement_0458
    ActorCmdExec 4, Movement_0460
    ActorCmdWait
    VMReturn

L_0284:
    WorkGet 0x8000, 0x8022
    WorkSetConst 0x8001, 547
    RTCallGlobal 2288
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A7

L_02A7:
    VMCall L_0335
    FlagSet EVENT_FLAG_0x00db
    VMStackPushFlag EVENT_FLAG_0x09fd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02CA
    WorkSetConst EVENT_WORK_0x400a, 1

L_02CA:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EB
    // "My mother once told me of a Pokémon that\nplayed a melody and danced so lightly[f000]븀\u0000\nthat it filled people's hearts with joy.[f000]븁\u0000\nThen sorrow darkened the entire world,\nand the Pokémon's melody was lost...[f000]븁\u0000\nAt the same time, somewhere,\nsome red shoes were lost...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_MotherOnceToldPokemon, 0, 0
    VMSleep 8

L_02EB:
    VMReturn

L_02ED:
    Cmd_020E 0, 5, 2, 7, 3, 8
    Cmd_020F 0, 5, 2, 7
    Cmd_0211 0
    FadeEx 12, 0, 16, 2
    FadeExWait
    ActorNew 5, 7, 1, 251, 123, 0
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 0
    VMReturn

L_0335:
    EvCameraMoveTo 9688, 0, 0xed000, 0x58000, 0, 0x98000, 25
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    Cmd_020F 1, 5, 3, 7
    Cmd_0211 1
    FadeEx 12, 0, 16, 2
    FadeExWait
    ActorDelete 251
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 1
    Cmd_020E 1, 5, 3, 7, 3, 8
    ActorCmdExec 255, Movement_0440
    ActorCmdWait
    VMReturn
    PlayerGetGPos 0x8020, 0x8021
    // "When you came through the door,\nI remembered the melody![f000]븁\u0000\nAnd the Pokémon that had forgotten how\nto dance began to step lightly once more![f000]븁\u0000\nYou are the inspiration.\nYou are a mysterious child..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_WhenCameThroughDoor, 0, 0
    MsgWaitAdvance
    ActorMsgClose
    VMReturn
    .balign 4, 0

Movement_03B4:
    Move 33, 67
    MoveEnd

Movement_03BC:
    Move 33, 33
    MoveEnd

Movement_03C4:
    Move 33, 30
    MoveEnd
    VMStackMul
    VMNop2
    VMStackSub
    VMReturn
    Move 15, 1
    Move 13, 1
    Move 32, 1
    MoveEnd
    Move 13, 4
    Move 32, 1
    MoveEnd
    Move 13, 4
    Move 14, 1
    Move 13, 1
    Move 32, 1
    MoveEnd
    Move 13, 4
    Move 15, 1
    Move 13, 1
    Move 32, 1
    MoveEnd

Movement_0418:
    Move 179, 8
    Move 177, 7
    MoveEnd

Movement_0424:
    Move 192, 11
    MoveEnd

Movement_042C:
    Move 177, 5
    Move 178, 1
    MoveEnd

Movement_0438:
    Move 1, 1
    MoveEnd

Movement_0440:
    Move 12, 3
    MoveEnd

Movement_0448:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_0458:
    Move 34, 1
    MoveEnd

Movement_0460:
    Move 35, 1
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

Movement_0480:
    Move 3, 1
    MoveEnd

Movement_0488:
    Move 75, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 33
    WorkSet 0x8001, 1
    WorkSet 0x8002, 133
    WorkSet 0x8003, 11
    WorkSet 0x8004, 12
    WorkSet 0x8005, 13
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

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've been letting my tired mind relax\nin this café this whole time..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_IveBeenLettingTired, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Listen to him playing guitar...\nI went back to the home I'd left behind,[f000]븀\u0000\nand my mother cried tears of joy...[f000]븀\u0000\nThat's mothers for you."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_ListenHimPlayingGuitar, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x00db
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0563
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can't change your family,\nbut you can change how you[f000]븀\u0000\ninteract with one another.[f000]븁\u0000\nStill, that said..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_CantChangeFamilyBut, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_05D5

L_0563:
    VMStackPushFlag EVENT_FLAG_0x09fd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05C1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Thanks to you and Meloetta,\nmemories of my birthplace...[f000]븀\u0000\nmemories of when I was little...[f000]븀\u0000\nmemories of singing together after[f000]븀\u0000\nwe finished gathering Berries...[f000]븀\u0000\nAll these memories came flooding back.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_ThanksMeloettaMemoriesBirthplace, 4, 0, 0
    MsgWinCloseAll
    Cmd_0275 0, 44, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg CasteliaCity22_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x09fd
    WorkSetConst EVENT_WORK_0x400a, 2
    // "I'm going to have fun gathering Berries\nwith the people living in the countryside[f000]븀\u0000\nand the Pokémon I've met here in[f000]븀\u0000\nCastelia City![f000]븁\u0000\nOf course, I'll be humming\nMeloetta's tune while I gather them!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_ImGoingHaveFun, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_05D5

L_05C1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm going to have fun gathering Berries\nwith the people living in the countryside[f000]븀\u0000\nand the Pokémon I've met here in[f000]븀\u0000\nCastelia City![f000]븁\u0000\nOf course, I'll be humming\nMeloetta's tune while I gather them!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_ImGoingHaveFun, 0, 0
    LastKeyWait
    ActorMsgClose

L_05D5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    PlayerGetGPos 0x8020, 0x8021
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_05F6
    VMJump L_0602

L_05F6:
    WorkSetConst 0x8020, 3
    VMJump L_0640

L_0602:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_0615
    VMJump L_0621

L_0615:
    WorkSetConst 0x8020, 4
    VMJump L_0640

L_0621:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_0634
    VMJump L_0640

L_0634:
    WorkSetConst 0x8020, 5
    VMJump L_0640

L_0640:
    ActorCmdExec 4, Movement_0480
    ActorCmdWait
    ActorCmdExec 4, Movement_0488
    ActorCmdWait
    ActorWalkRoute 4, 0x8020, 0x8021, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0458
    ActorCmdWait
    // "Thanks to you and Meloetta,\nmemories of my birthplace...[f000]븀\u0000\nmemories of when I was little...[f000]븀\u0000\nmemories of singing together after[f000]븀\u0000\nwe finished gathering Berries...[f000]븀\u0000\nAll these memories came flooding back.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_ThanksMeloettaMemoriesBirthplace, 4, 0, 0
    MsgWinCloseAll
    Cmd_0275 0, 44, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg CasteliaCity22_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x09fd
    // "I'm going to have fun gathering Berries\nwith the people living in the countryside[f000]븀\u0000\nand the Pokémon I've met here in[f000]븀\u0000\nCastelia City![f000]븁\u0000\nOf course, I'll be humming\nMeloetta's tune while I gather them!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity22_Text_ImGoingHaveFun, 4, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 4, 2, 8, 0, 8, 0
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x400a, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
