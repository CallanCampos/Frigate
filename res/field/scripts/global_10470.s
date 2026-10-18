#include "asm/field_script.inc"
#include "text/script/global_10470.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_68
    SEWait
    // "Click![f000]븁\u0000\nThe sound reverberates."
    InfoMsg Global10470_Text_ClickSoundReverberates, 2
    LastKeyWait
    InfoMsgClose_0039
    FlagReset EVENT_FLAG_0x00d7
    FlagReset EVENT_FLAG_0x00d6
    WorkSetConst EVENT_WORK_0x4094, 0
    WorkSetConst EVENT_WORK_0x4095, 0
    WorkSetConst EVENT_WORK_0x4096, 0
    ActorSetGPos 0, 80, 3, 79, 1
    ActorSetGPos 1, 80, 3, 77, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_153
    SEWait
    // "A dull sound came from far away."
    InfoMsg Global10470_Text_DullSoundCameFrom, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_154
    EvCameraShake 5, 0, 3, 8, 1, 0, 1, 5
    SEWait
    // "A dull sound echoed."
    InfoMsg Global10470_Text_DullSoundEchoed, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_155
    EvCameraShake 8, 0, 3, 15, 1, 0, 1, 5
    SEWait
    // "The dull sound is close!"
    InfoMsg Global10470_Text_DullSoundClose, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    // "It's a torrent of water!"
    InfoMsg Global10470_Text_ItsTorrentWater, 2
    LastKeyWait
    InfoMsgClose_0039
    SEPlay SEQ_SE_FLD_156
    EvCameraShake 10, 0, 3, 20, 1, 0, 1, 5
    SEWait
    CallDiving 2
    FlagReset EVENT_FLAG_0x00d7
    FlagReset EVENT_FLAG_0x00d6
    WorkSetConst EVENT_WORK_0x4094, 0
    WorkSetConst EVENT_WORK_0x4095, 0
    WorkSetConst EVENT_WORK_0x4096, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x00d7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016F
    EvCameraShake 0, 1, 3, 6, 1, 0, 1, 5
    SEPlay SEQ_SE_FLD_126
    ActorCmdExec 0, Movement_01D0
    ActorCmdExec 1, Movement_01D0
    ActorCmdWait
    SEWait
    // "The wall moved, and you can proceed now!"
    InfoMsg Global10470_Text_WallMovedCanProceed, 2
    LastKeyWait
    InfoMsgClose_0039
    FlagSet EVENT_FLAG_0x00d7
    WorkSetConst EVENT_WORK_0x4095, 1

L_016F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x00d6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C7
    EvCameraShake 0, 1, 3, 6, 1, 0, 1, 5
    SEPlay SEQ_SE_FLD_126
    ActorCmdExec 0, Movement_01DC
    ActorCmdExec 1, Movement_01DC
    ActorCmdWait
    SEWait
    // "The wall moved, and you can proceed now!"
    InfoMsg Global10470_Text_WallMovedCanProceed, 2
    LastKeyWait
    InfoMsgClose_0039
    FlagSet EVENT_FLAG_0x00d6
    WorkSetConst EVENT_WORK_0x4096, 1

L_01C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01D0:
    Move 4, 1
    Move 8, 2
    MoveEnd

Movement_01DC:
    Move 7, 1
    Move 11, 2
    MoveEnd
