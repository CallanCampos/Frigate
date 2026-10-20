#include "asm/field_script.inc"
#include "text/script/route_14.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 14"
    MsgPlaceSign Route14_Text_Route14, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nThe maximum number of Boxes is now 24![f000]븁\u0000\nIn other words, you can store\n720 Pokémon!"
    MsgPlaceSign Route14_Text_TrainerTipsMaximumNumber, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a73
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nWhile you are using the Xtransceiver,\npress a direction on the +Control Pad.[f000]븁\u0000\nThe appearance of the screen will\nchange in varied ways!"
    MsgPlaceSign Route14_Text_TrainerTipsWhileUsing, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a74
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
