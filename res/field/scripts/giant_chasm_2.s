#include "asm/field_script.inc"
#include "text/script/giant_chasm_2.h"

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
    VMStackPushFlag EVENT_FLAG_0x016c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005D
    ObjInitWarpGPos 1, 34, 0, 47
    VMJump L_0067

L_005D:
    ObjInitWarpGPos 2, 34, 0, 47

L_0067:
    VMReturn

Script_1:
    VMCall L_003A
    VMHalt

Script_8:
    VMCall L_003A
    VMHalt

Script_2:
    VMHalt

Script_7:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    FlagReset EVENT_FLAG_0x036e
    FlagReset EVENT_FLAG_0x0370
    ActorAdd 9
    ActorAdd 10
    ActorWalkRoute 9, 61, 36, 1, 8, 1
    VMSleep 40
    ActorCmdExec 255, Movement_02FC
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: You're already here, huh?[f000]븁\u0000\nAnyone who tries to block us\nwill go running away with[f000]븀\u0000\ntheir tail between their legs![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_YoureAlreadyHereHuh, 9, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 10, 60, 41, 1, 8, 1
    VMSleep 8
    ActorCmdExec 6, Movement_0304
    ActorCmdExec 11, Movement_0304
    VMSleep 16
    ActorCmdExec 9, Movement_0324
    ActorCmdExec 255, Movement_0304
    ActorCmdWait
    // "Hey, it's good! We're all set here![f000]븁\u0000"
    InfoMsg GiantChasm2_Text_HeyItsGoodWere, 2
    MsgWinCloseAll
    // "It's finally time!\nC'mon! Let's go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_ItsFinallyTimeCmon, 6, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 6, 61, 45, 1, 8, 1
    ActorWalkRoute 11, 61, 45, 1, 8, 0
    VMSleep 16
    ActorCmdExec 10, Movement_0304
    ActorCmdWait
    ActorDelete 11
    ActorDelete 6
    ActorWalkRoute 9, 61, 39, 1, 8, 1
    VMSleep 4
    ActorWalkRoute 255, 60, 39, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0304
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: You're all alone now...\nSo, which one of us are you gonna tackle?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_YoureAllAloneNow, 9, 5, 0
    MsgWinCloseAll
    ActorCmdExec 10, Movement_0324
    ActorCmdWait
    ActorCmdExec 10, Movement_02FC
    ActorCmdWait
    // "Oi! It's me! Me!\nThe ex-Team Plasma guy![f000]븀\u0000\nThe one Rood asked to be a spy![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_OiItsExTeam, 10, 4, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Oh, yeah...\nI remember you.[f000]븁\u0000\nBeing ex-Team Plasma\nis really rough, eh?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_OhYeahRememberBeing, 9, 5, 0
    MsgWinCloseAll
    // "You know, N was so nice to Pokémon...[f000]븁\u0000\nAnd all I wanted to do was protect\nPokémon from bad people...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_KnowNNicePokemon, 10, 4, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Well, I suppose...[f000]븁\u0000\nIf we don't crush Team Plasma, then\nguys like you and that N guy[f000]븀\u0000\nwill always be treated like villains.[f000]븁\u0000\nOK!\nI'm outta here, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_WellSupposeIfWe, 9, 5, 0
    MsgWinCloseAll
    ActorWalkRoute 9, 61, 51, 1, 4, 1
    VMSleep 12
    ActorCmdExec 10, Movement_0304
    ActorCmdWait
    ActorCmdExec 10, Movement_02FC
    ActorCmdWait
    // "Oh, thank you.\nThat's so nice.[f000]븁\u0000\nEven if we said it was for Pokémon, in the\nend, we were doing what we wanted to do.[f000]븁\u0000\nPlease excuse me.\nThere's still something I have to do![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_OhThankThatsNice, 10, 4, 0
    MsgWinCloseAll
    ActorWalkRoute 10, 60, 51, 1, 8, 1
    ActorCmdWait
    ActorDelete 9
    ActorDelete 10
    FlagSet EVENT_FLAG_0x036c
    FlagSet EVENT_FLAG_0x036e
    FlagSet EVENT_FLAG_0x0370
    FlagSet EVENT_FLAG_0x0359
    WorkSetConst EVENT_WORK_0x4070, 2
    HollowRivalCmd_0262 1, 33
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 14, 1
    Move 33, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "How long does “wait until everyone\nelse arrives\" mean, exactly?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_HowLongDoesWait, 11, 3, 0
    MsgWinCloseAll
    // "The area past here is important![f000]븁\u0000\nThe people heading to Route 21\nare definitely important, but we have[f000]븀\u0000\nan important role as well!"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_AreaPastHereImportant, 6, 5, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, thank you.\nThat's so nice.[f000]븁\u0000\nEven if we said it was for Pokémon, in the\nend, we were doing what we wanted to do.[f000]븁\u0000\nPlease excuse me.\nThere's still something I have to do![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_OhThankThatsNice, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "We're standing watch here\nso our allies don't go AWOL!"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_WereStandingWatchHere, 7, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "None shall pass!\nSages' orders!"
    ActorMsg MSGFILE_SCRIPT, GiantChasm2_Text_NoneShallPassSages, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll
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

Movement_02FC:
    Move 32, 1
    MoveEnd

Movement_0304:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd

Movement_0324:
    Move 159, 1
    MoveEnd
