#include "asm/field_script.inc"
#include "text/script/plasma_frigate_11.h"

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

Script_8:
    VMCall L_0052
    VMHalt

Script_7:
    VMCall L_0052
    VMHalt

Script_6:
    VMCall L_0052
    VMHalt

L_0052:
    WorkSetConst 0x8024, 0
    GameGetVersion 0x8024
    VMStackPush 0x8024
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007F
    ObjInitWarpGPos 3, 0, 0, 0
    VMJump L_0089

L_007F:
    ObjInitWarpGPos 0, 0, 0, 0

L_0089:
    WorkSetConst 0x8024, 0
    VMReturn

Script_1:
    ActorsPauseAll
    WordSetLoadRivalName 1
    WordSetPlayerName 0
    VMStackPushFlag EVENT_FLAG_0x01e7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FD
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: Go on ahead![f000]븁\u0000\nI'm going to make them tell me\nabout the Shadow Triad![f000]븁\u0000\nIf you find Purrloin or the Shadow Triad,\nlet me know!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate11_Text_GoAheadImGoing, 4, 0, 0
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E5
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00F3

L_00E5:
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 4, Movement_01E4
    ActorCmdWait

L_00F3:
    FlagSet EVENT_FLAG_0x01e7
    VMJump L_0117

L_00FD:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    // "You...\nWhere are the Shadow Triad?"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate11_Text_WhereShadowTriad, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0117:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Cretin! Don't interfere with\nTeam Plasma's conquest of Unova!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate11_Text_CretinDontInterfereTeam, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Struggle and squirm![f000]븁\u0000\nKyurem's ice is more powerful\nthan Reshiram's columns of fire[f000]븀\u0000\nor Zekrom's crackling lightning!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate11_Text_StruggleSquirmKyuremsIce, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "H-he's terrifying!\nHe can't be human![f000]븁\u0000\nBut he has a silly-looking\nQwilfish hairstyle!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate11_Text_HHesTerrifyingHe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "All of Unova will be\nfrozen with Kyurem's ice![f000]븁\u0000\nI wonder if Zinzolin will be OK...\nHe really doesn't like the cold."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate11_Text_AllUnovaWillFrozen, 0, 0
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
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_01E4:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 161, 1
    MoveEnd
