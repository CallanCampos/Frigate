#include "asm/field_script.inc"
#include "text/script/gear_station_9.h"

// Script plugin 1, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    RTCallGlobal 10339
    VMHalt

Script_2:
    RTCallGlobal 10339
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a subway map of the Unova region.[f000]븁\u0000"
    InfoMsg GearStation9_Text_ItsSubwayMapUnova, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 2, 0
    FieldOpen
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is the platform for the train to\nAnville Town."
    ParentActorMsg MSGFILE_SCRIPT, GearStation9_Text_PlatformTrainAnvilleTown, 0, 0
    LastKeyWait
    ActorMsgClose
    VMHalt
    .balign 4, 0
