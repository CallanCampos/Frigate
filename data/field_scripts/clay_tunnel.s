#include "asm/field_script.inc"
#include "text/script/clay_tunnel.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sigh! We've been digging and\ndigging, but...[f000]븁\u0000\nWait a minute! Do you have a Pokémon\nthat's learned Rock Smash?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_SighWeveBeenDigging, 0, 0
    MsgWaitAdvance
    PokePartyHasMoveAny 0x8020, 249
    VMStackPush 0x8020
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0065
    // "Oh... It looks like none of your\nPokémon has learned Rock Smash..."
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhLooksLikeNone, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01C6

L_0065:
    MsgWinCloseAll
    ActorCmdExec 8, Movement_022C
    ActorCmdWait
    // "All right! We can break all the rocks\nif we work together![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_AllRightWeCan, 0, 0
    MsgWinCloseAll
    ActorCmdExec 8, Movement_01D4
    VMSleep 5
    ActorCmdExec 7, Movement_01D4
    ActorCmdExec 6, Movement_01D4
    VMSleep 10
    FadeEx 3, 0, 16, 2
    FadeExWait
    ActorCmdWait
    ActorDelete 9
    ActorDelete 10
    ActorDelete 11
    ActorDelete 12
    ActorDelete 13
    ActorDelete 14
    ActorDelete 15
    ActorDelete 19
    ActorDelete 20
    ActorDelete 21
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    ActorCmdExec 8, Movement_01CC
    VMSleep 5
    ActorCmdExec 7, Movement_01CC
    VMSleep 5
    ActorCmdExec 6, Movement_01CC
    ActorCmdWait
    // "Hooray! All right! Now, the tunnel\nis connected to Twist Mountain![f000]븁\u0000\nI'll call the guy who's in charge of\nthe cart and go home![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_HoorayAllRightNow, 1, 0
    MsgWinCloseAll
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0128
    VMJump L_014A

L_0128:
    ActorCmdExec 8, Movement_01E0
    VMSleep 12
    ActorCmdExec 6, Movement_01F8
    ActorCmdExec 7, Movement_01E8
    VMJump L_01B4

L_014A:
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_015D
    VMJump L_017F

L_015D:
    ActorCmdExec 8, Movement_0204
    ActorCmdExec 6, Movement_021C
    VMSleep 12
    ActorCmdExec 7, Movement_0210
    VMJump L_01B4

L_017F:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0192
    VMJump L_01B4

L_0192:
    ActorCmdExec 8, Movement_01E0
    VMSleep 12
    ActorCmdExec 6, Movement_01F8
    ActorCmdExec 7, Movement_01E8
    VMJump L_01B4

L_01B4:
    ActorCmdWait
    ActorDelete 8
    ActorDelete 7
    ActorDelete 6
    FlagSet EVENT_FLAG_0x03be

L_01C6:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_01CC:
    Move 50, 2
    MoveEnd

Movement_01D4:
    Move 50, 1
    Move 42, 20
    MoveEnd

Movement_01E0:
    Move 13, 10
    MoveEnd

Movement_01E8:
    Move 13, 1
    Move 15, 1
    Move 13, 10
    MoveEnd

Movement_01F8:
    Move 15, 1
    Move 13, 10
    MoveEnd

Movement_0204:
    Move 15, 1
    Move 13, 10
    MoveEnd

Movement_0210:
    Move 15, 2
    Move 13, 10
    MoveEnd

Movement_021C:
    Move 12, 1
    Move 15, 2
    Move 13, 10
    MoveEnd

Movement_022C:
    Move 75, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Dig, dig! Dig, dig! Dig, dig!\nDig, dig! Dig, dig! Dig, dig!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_DigDigDigDig, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Dig, dig! Dig, dig! Dig, dig!\nDig, dig! Dig, dig! Dig, dig!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_DigDigDigDig, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02CF
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    ActorCmdExec 16, Movement_0400
    FadeWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet EVENT_FLAG_0x03b4
    FlagReset EVENT_FLAG_0x03b7
    RTReserveScript 2
    MapChangeCore ZONE_CLAY_TUNNEL_2, 27, 0, 14, 2
    VMJump L_02DD

L_02CF:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02DD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034A
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    ActorCmdExec 18, Movement_03E8
    FadeWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet EVENT_FLAG_0x03b6
    FlagReset EVENT_FLAG_0x03bc
    RTReserveScript 7
    MapChangeCore ZONE_CLAY_TUNNEL_3, 19, 0, 35, 1
    VMJump L_0358

L_034A:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0358:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C5
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    ActorCmdExec 17, Movement_0400
    FadeWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet EVENT_FLAG_0x03b5
    FlagReset EVENT_FLAG_0x03bd
    RTReserveScript 7
    MapChangeCore ZONE_CLAY_TUNNEL_3, 56, 0, 84, 1
    VMJump L_03D3

L_03C5:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03D3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03E8:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0400:
    Move 35, 1
    MoveEnd
