#include "asm/field_script.inc"
#include "text/script/clay_tunnel_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel2_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0071
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel2_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    ActorCmdExec 7, Movement_00AC
    FadeWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet EVENT_FLAG_0x03b7
    FlagReset EVENT_FLAG_0x03b4
    RTReserveScript 7
    MapChangeCore ZONE_CLAY_TUNNEL, 27, 0, 38, 2
    VMJump L_007F

L_0071:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel2_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_007F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_00AC:
    Move 35, 1
    MoveEnd
