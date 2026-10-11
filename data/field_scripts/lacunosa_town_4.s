#include "asm/field_script.inc"
#include "text/script/lacunosa_town_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    RTCGetDayPart 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0041
    FlagReset EVENT_FLAG_0x0312
    VMJump L_0045

L_0041:
    FlagSet EVENT_FLAG_0x0312

L_0045:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Of course it's dangerous\nto go out at night.[f000]븁\u0000\nMaybe you should stay inside\nduring the afternoon, too.[f000]븀\u0000\nThen there's no danger at all!"
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown4_Text_CourseItsDangerousGo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
