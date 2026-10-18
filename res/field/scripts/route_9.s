#include "asm/field_script.inc"
#include "text/script/route_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 9"
    MsgPlaceSign Route9_Text_Route9, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Shopping Mall Nine\nColorful and wonderful!"
    MsgPlaceSign Route9_Text_ShoppingMallNineColorful, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nOne kind of Pokémon\ncan have different Abilities.[f000]븁\u0000\nTry to catch Pokémon you've\nalready caught before!"
    MsgPlaceSign Route9_Text_TrainerTipsOneKind, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a70
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Tubeline Bridge\nUnova's famous railway bridge"
    MsgPlaceSign Route9_Text_TubelineBridgeUnovasFamous, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
