#include "asm/field_script.inc"
#include "text/script/route_12.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 12"
    MsgPlaceSign Route12_Text_Route12, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nTry pressing SELECT while\norganizing your PC Box.[f000]븁\u0000\nIt'll let you move your Pokémon\naround more easily![f000]븁\u0000\nThe more Pokémon you deposit,\nthe more Boxes you'll have."
    MsgPlaceSign Route12_Text_TrainerTipsTryPressing, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a71
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 12"
    MsgPlaceSign Route12_Text_Route12_2, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
