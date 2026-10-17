#include "asm/field_script.inc"
#include "text/script/desert_resort.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Desert Resort Ahead\nIt's nicer than the average desert!"
    MsgPlaceSign DesertResort_Text_DesertResortAheadIts, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Go through the gate, and just head\nstraight. That's how you get to[f000]븀\u0000\nthe Relic Castle."
    ParentActorMsg MSGFILE_SCRIPT, DesertResort_Text_GoThroughGateJust, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Desert Resort is vast, and\nthe Pokémon here are strong![f000]븀\u0000\nIt's an ideal place to test your skills!"
    ParentActorMsg MSGFILE_SCRIPT, DesertResort_Text_DesertResortVastPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
