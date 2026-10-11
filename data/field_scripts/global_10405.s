#include "asm/field_script.inc"
#include "text/script/black_city_4.h"
#include "text/script/global_10405.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_15:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_00A8
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_00A8
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    VMNop

Data_00A8:
    VMNop2
    ActorCmdExec 3, Movement_00B2

Movement_00B2:
    VMReturn
    .byte 0x1f
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x0a
    .byte 0x00
    .byte 0x20
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_00FC
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_00FC
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Data_00FC:
    .byte 0x01
    .byte 0x00
    .byte 0x65
    .byte 0x00
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x06
    .byte 0x00
    .byte 0x21
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x22
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_17:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_0150
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_0150
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Data_0150:
    .byte 0x01
    .byte 0x00
    .byte 0x66
    .byte 0x00
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x23
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x09
    .byte 0x00
    .byte 0x24
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_18:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_01A4
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_01A4
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Data_01A4:
    .byte 0x01
    .byte 0x00
    .byte 0x67
    .byte 0x00
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x09
    .byte 0x00
    .byte 0x25
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x0a
    .byte 0x00
    .byte 0x26
    .byte 0x02
    .byte 0x03
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_01F8
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_01F8
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Data_01F8:
    .byte 0x01
    .byte 0x00
    .byte 0x2c
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x0c
    .byte 0x00
    .byte 0x2d
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x0d
    .byte 0x00
    .byte 0x2e
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_024C
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_024C
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Data_024C:
    .byte 0x01
    .byte 0x00
    .byte 0x2f
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x08
    .byte 0x00
    .byte 0x30
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_0298
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_0298
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    VMNop

Data_0298:
    VMNop2
    ABKeyWait
    DebugStack 0
    VMNop
    VMReturn
    .byte 0x32
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_02E4
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_02E4
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    VMNop

Data_02E4:
    VMNop2
    InfoMsgClose
    DebugStack 0
    VMNop
    VMReturn
    .byte 0x37
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_0330
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_0330
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .byte 0x00
    .byte 0x00

Data_0330:
    .byte 0x01
    .byte 0x00
    .byte 0x38
    .byte 0x00
    DebugStack 0
    VMNop
    VMHalt
    .byte 0x39
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x03
    .byte 0x00
    .byte 0xee
    .byte 0x01
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_0384
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_0384
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Data_0384:
    .byte 0x01
    .byte 0x00
    .byte 0x3a
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x0c
    .byte 0x00
    .byte 0x3b
    .byte 0x00
    .byte 0x07
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0xff
    .byte 0xff

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    ElevatorSetTablePtr Data_03D0
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    ElevatorSetTablePtr Data_03D0
    VMCall L_03E6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .byte 0x00
    .byte 0x00

Data_03D0:
    .byte 0x01
    .byte 0x00
    .byte 0x3c
    .byte 0x00
    DebugStack 0
    VMNop
    VMStackAdd
    // "                                            "
    ParentActorMsg 7, BlackCity4_Text_Empty, 0, 65535

L_03E6:
    SEPlay SEQ_SE_MESSAGE
    VMCall L_045C
    // "Which floor would you like to go to?"
    SystemMsg Global10405_Text_WhichFloorWouldLike, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ElevatorBuildListMenu
    ListMenuAdd 0, 65535, 255
    ListMenuShow
    InfoMsgClose
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMStackPush 0x8010
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0454
    VMSleep 10
    SEPlay SEQ_SE_FLD_23
    FadeOutBlackQ
    FadeWait
    ElevatorChangeMap 0x8010
    FadeInBlackQ
    FadeWait
    VMSleep 15
    SEStop
    SEPlay SEQ_SE_FLD_87
    SEWait

L_0454:
    VMCall L_04E8
    VMReturn

L_045C:
    VMSleep 15
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8021, 2
    WorkSub 0x8022, 1
    BMCreateHandleByGPos 0x8023, 1, 0x8021, 0x8022
    BMHndAudioVisualAnmPlay 0x8023, 0
    BMHndAnmWait 0x8023
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A7
    ActorCmdExec 255, Movement_04D4
    VMJump L_04AF

L_04A7:
    ActorCmdExec 255, Movement_04CC

L_04AF:
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8023, 1
    BMHndAnmWait 0x8023
    BMReleaseHandle 0x8023
    ActorCmdExec 255, Movement_04E0
    ActorCmdWait
    VMReturn
    .balign 4, 0

Movement_04CC:
    Move 12, 2
    MoveEnd

Movement_04D4:
    Move 15, 1
    Move 12, 2
    MoveEnd

Movement_04E0:
    Move 1, 1
    MoveEnd

L_04E8:
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8022, 1
    BMCreateHandleByGPos 0x8023, 1, 0x8021, 0x8022
    BMHndAudioVisualAnmPlay 0x8023, 0
    BMHndAnmWait 0x8023
    ActorCmdExec 255, Movement_0524
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8023, 1
    BMHndAnmWait 0x8023
    BMReleaseHandle 0x8023
    VMReturn
    .balign 4, 0

Movement_0524:
    Move 13, 2
    MoveEnd
