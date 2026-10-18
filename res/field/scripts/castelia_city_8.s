#include "asm/field_script.inc"
#include "text/script/castelia_city_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_4:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    ActorCmdExec 255, Movement_01AC
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Liberty Garden, huh?[f000]븁\u0000\nA long time ago, an extremely rich\nperson hid a very amazing[f000]븀\u0000\nPokémon called Victini there!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity8_Text_LibertyGardenHuhLong, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ho ho! Trying to become stronger\nby misusing Victini's powers...[f000]븁\u0000\nTeam Plasma fell apart exactly because\nthey planned to do things like that!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity8_Text_HoHoTryingBecome, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01be
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B2
    VMStackPushFlag EVENT_FLAG_0x00f8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009E
    FlagSet EVENT_FLAG_0x00f8
    // "It is small compared to a luxury liner.[f000]븁\u0000\nBut the size of a ship doesn't change the\nfeeling of adventure when you're out on[f000]븀\u0000\nthe open sea.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity8_Text_SmallComparedLuxuryLiner, 0, 0

L_009E:
    // "Would you like to go to Liberty Garden?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity8_Text_WouldLikeGoLiberty, 0, 0
    YesNoWin 0x8010
    VMJump L_00C0

L_00B2:
    // "Would you like to go\nto Liberty Garden again?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity8_Text_WouldLikeGoLiberty_2, 0, 0
    YesNoWin 0x8010

L_00C0:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010D
    // "Your timing is perfect.[f000]븁\u0000\nThe ship is about to leave.\nPlease get aboard the ship and wait.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity8_Text_TimingPerfectShipAbout, 0, 0
    MsgWinCloseAll
    VMCall L_0129
    RTReserveScript 11
    FlagSet EVENT_FLAG_0x01be
    FadeOutBlackQ
    FadeWait
    FieldClose
    Call3DDemo 10, 0
    FieldOpen
    MapChangeCore ZONE_LIBERTY_GARDEN, 295, 1, 748, 3
    VMJump L_0123

L_010D:
    // "If you'd like to go to Liberty Garden,\nplease let me know.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity8_Text_IfYoudLikeGo, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_01A4
    ActorCmdWait

L_0123:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0129:
    WorkSetConst 0x8020, 0
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014A
    PlayerSetSpecialSequence 1

L_014A:
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016F
    ActorCmdExec 255, Movement_01B4
    VMJump L_0198

L_016F:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0190
    ActorCmdExec 255, Movement_01C4
    VMJump L_0198

L_0190:
    ActorCmdExec 255, Movement_01D0

L_0198:
    ActorCmdWait
    WorkSetConst 0x8020, 0
    VMReturn
    .balign 4, 0

Movement_01A4:
    Move 34, 1
    MoveEnd

Movement_01AC:
    Move 15, 1
    MoveEnd

Movement_01B4:
    Move 15, 1
    Move 13, 3
    Move 14, 1
    MoveEnd

Movement_01C4:
    Move 13, 2
    Move 14, 1
    MoveEnd

Movement_01D0:
    Move 13, 1
    Move 14, 1
    MoveEnd
