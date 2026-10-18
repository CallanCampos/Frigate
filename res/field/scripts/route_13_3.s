#include "asm/field_script.inc"
#include "text/script/route_13_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm really annoyed by what\nTeam Plasma said in the past.[f000]븁\u0000\nSo, I live with Pokémon\nwithout putting them in Poké Balls![f000]븁\u0000\nBut Panpour doesn't\nlisten to me at all..."
    ParentActorMsg MSGFILE_SCRIPT, Route133_Text_ImReallyAnnoyedBy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 515, 0
    // "Chatttteeeer!!"
    ParentActorMsg MSGFILE_SCRIPT, Route133_Text_Chatttteeeer, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The creation of Poké Balls made it\npossible for everyone to be with Pokémon.[f000]븁\u0000\nYes, everyone..."
    ParentActorMsg MSGFILE_SCRIPT, Route133_Text_CreationPokeBallsMade, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
