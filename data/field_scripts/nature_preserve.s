#include "asm/field_script.inc"
#include "text/script/nature_preserve.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_3:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 612, 0
    // "Gwaooooogh!"
    ScreamMsg NaturePreserve_Text_Gwaooooogh, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    WorkSetConst 0x8020, 0
    WorkOr 0x8020, 2
    WorkOr 0x8020, 128
    CallWildBattleEx 612, 60, 0, 0x8020
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    FlagSet EVENT_FLAG_0x03b2
    ActorDelete 1
    CallWildBattleEnd
    VMJump L_0080

L_007E:
    CallWildLose

L_0080:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0097
    VMJump L_00A1

L_0097:
    FlagSet EVENT_FLAG_0x01a4
    VMJump L_00D1

L_00A1:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_00C1
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_00C1
    VMJump L_00D1

L_00C1:
    // "The dark Haxorus vanished\ninto the preserve..."
    SystemMsg NaturePreserve_Text_DarkHaxorusVanishedInto, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_00D1

L_00D1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's a hop, skip, and a jump by my plane!\nWant to go back to Mistralton City?"
    ActorMsg MSGFILE_SCRIPT, NaturePreserve_Text_ItsHopSkipJump, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012A
    // "OK! Let's hit the runway![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NaturePreserve_Text_OkLetsHitRunway, 0, 2, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    RTReserveScript 11
    MapChangeCore ZONE_MISTRALTON_CITY_3, 14, 0, 19, 1
    VMJump L_013A

L_012A:
    // "Roger!\nTalk to me when you're ready!"
    ActorMsg MSGFILE_SCRIPT, NaturePreserve_Text_RogerTalkWhenYoure, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_013A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
