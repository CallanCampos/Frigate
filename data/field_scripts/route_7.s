#include "asm/field_script.inc"
#include "text/script/route_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_4:
    FlagReset EVENT_FLAG_0x0198
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 7"
    MsgPlaceSign Route7_Text_Route7, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Celestial Tower\nA place of rest for innocent spirits"
    MsgPlaceSign Route7_Text_CelestialTowerPlaceRest, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nThe number of Exp. Points you get\nafter a battle is based on levels.[f000]븁\u0000\nWhen your Pokémon is weaker than\nits opponent, it will get more.[f000]븁\u0000\nBut if your Pokémon is stronger,\nit won't get as many."
    MsgPlaceSign Route7_Text_TrainerTipsNumberExp, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a6e
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you stand still on the raised walkway,\nyou'll fall off!"
    ParentActorMsg MSGFILE_SCRIPT, Route7_Text_IfStandStillRaised, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    MEPlay SEQ_ME_CALL
    // "The Xtransceiver is ringing."
    SystemMsg Route7_Text_XtransceiverRinging, 2
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 picked up the Xtransceiver.[f000]븁\u0000"
    SystemMsg Route7_Text_PickedUpXtransceiver, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    CallXTransceiver 4, 0
    FadeInBlackQ
    FadeWait
    WorkSetConst EVENT_WORK_0x4099, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
