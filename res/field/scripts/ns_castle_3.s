#include "asm/field_script.inc"
#include "text/script/ns_castle_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntry Script_11
    ScriptEntriesEnd

Script_11:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 10
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorWalkRoute 255, 9, 18, 0, 8, 0
    ActorNew 9, 20, 0, 251, 103, 0
    ActorDelete 254
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x98000, 0, 0xf5000, 40
    ActorWalkRoute 251, 8, 18, 0, 8, 0
    ActorCmdWait
    EvCameraWait
    // "N: [f000]븉\u0001\u0001This place...\nwas my entire world...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle3_Text_NPlaceEntireWorld, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 9, 16, 0, 8, 0
    ActorCmdWait
    // "[f000]븉\u0001\u0001When I was little, I was abandoned\ndeep in the woods.[f000]븁\u0000\nThe ones who took me in and raised me\nwere the Pokémon who lived there.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle3_Text_WhenLittleAbandonedDeep, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_028C
    ActorCmdWait
    // "[f000]븉\u0001\u0001Then, one day, a man appeared\nbefore me, claiming to be my father.[f000]븁\u0000\nThat...was Ghetsis.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle3_Text_ThenOneDayMan, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_015C
    ActorCmdWait
    // "[f000]븉\u0001\u0001The things he gave me were...\nthe name “Harmonia\"...[f000]븁\u0000\nthe knowledge a king would need...[f000]븁\u0000\nPokémon with their hearts shut so very\ntightly I couldn't even talk to them...[f000]븁\u0000\nand this room...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle3_Text_ThingsHeGaveWere, 251, 0, 0
    MsgWinCloseAll
    VMSleep 8
    ActorCmdExec 251, Movement_0174
    ActorCmdWait
    // "[f000]븉\u0001\u0001I'll be outside...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle3_Text_IllOutside, 251, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    ActorWalkRoute 251, 8, 20, 0, 8, 0
    VMSleep 40
    ActorCmdExec 255, Movement_028C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x4113, 2
    FlagSet EVENT_FLAG_0x019f
    FlagReset EVENT_FLAG_0x038f
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_015C:
    Move 30, 1
    Move 63, 1
    Move 31, 1
    Move 63, 1
    Move 29, 1
    MoveEnd

Movement_0174:
    Move 182, 1
    MoveEnd

Script_10:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a slightly dirty basketball.\nThe name “Harmonia\" is written on it."
    InfoMsg NsCastle3_Text_ItsSlightlyDirtyBasketball, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a home-style basketball hoop.\nIt's been knocked over."
    InfoMsg NsCastle3_Text_ItsHomeStyleBasketball, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a toy box, but its contents\nhave spilled out."
    InfoMsg NsCastle3_Text_ItsToyBoxBut, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The rubber of these wheels\nhas rotted away."
    InfoMsg NsCastle3_Text_RubberTheseWheelsHas, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "A halfpipe for skateboards...[f000]븁\u0000\nIt has Pokémon scratch marks\non it here and there..."
    InfoMsg NsCastle3_Text_HalfpipeSkateboardsHasPokemon, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "An art panel with a\nprinted geometric pattern."
    InfoMsg NsCastle3_Text_ArtPanelPrintedGeometric, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This panel has a dart stuck in it."
    InfoMsg NsCastle3_Text_PanelHasDartStuck, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a set of trains and tracks.\nIt hasn't been touched in a long time."
    InfoMsg NsCastle3_Text_ItsSetTrainsTracks, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This toy box has been tipped over."
    InfoMsg NsCastle3_Text_ToyBoxHasBeen, 2
    LastKeyWait
    MsgWinCloseAll
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

Movement_028C:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
