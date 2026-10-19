#include "asm/field_script.inc"
#include "text/script/castelia_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_2:
    WorkSetConst EVENT_WORK_0x411a, 0
    CasteliaRushInit
    VMHalt

Script_3:
    CasteliaRushInit
    VMHalt
    .balign 4, 0
    Move 3, 1
    Move 75, 1
    MoveEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 4 Ahead"
    MsgPlaceSign CasteliaCity4_Text_Route4Ahead, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nimbasa City has\nthe Pokémon Musical.[f000]븁\u0000\nI've tried it several times.\nSigh. It never seems to go well.[f000]븁\u0000\nBut I'll keep trying until I get\nthe perfect Props for my Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity4_Text_NimbasaCityHasPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Trainers and Pokémon make\nan amazing combination![f000]븁\u0000\nThey were responsible for\nexcavating the ruins on Route 4!"
    // "Trainers and Pokémon make\nan amazing combination![f000]븁\u0000\nThey were responsible for the row of\nbrand-new buildings on Route 4!"
    ActorMsgVersioned 1024, CasteliaCity4_Text_TrainersPokemonMakeAmazing, CasteliaCity4_Text_TrainersPokemonMakeAmazing_2, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
