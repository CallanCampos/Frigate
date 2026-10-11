#include "asm/field_script.inc"
#include "text/script/route_gate_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This electric bulletin board has been\npitch black for a really long time."
    ParentActorMsg MSGFILE_SCRIPT, RouteGateGate_Text_ElectricBulletinBoardHas, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "One thing I learned during my travels\nis that home is a really nice place, too![f000]븀\u0000\nFunny--before I set off, I absolutely[f000]븀\u0000\ncouldn't wait to leave..."
    ParentActorMsg MSGFILE_SCRIPT, RouteGateGate_Text_OneThingLearnedDuring, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon just love to battle![f000]븁\u0000\nI'm absolutely worn out...\nSo I'm resting here..."
    ParentActorMsg MSGFILE_SCRIPT, RouteGateGate_Text_PokemonJustLoveBattle, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
