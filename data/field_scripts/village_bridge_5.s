#include "asm/field_script.inc"
#include "text/script/village_bridge_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if veteran Pokémon raise\nor train young Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge5_Text_WonderIfVeteranPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wish somebody would raise me like this.\nWith three meals and a nap every day."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge5_Text_WishSomebodyWouldRaise, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Follow me, Ducklett!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge5_Text_FollowDucklett, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Buuuurp!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge5_Text_Buuuurp, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 580, 0
    // "Quak!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge5_Text_Quak, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
