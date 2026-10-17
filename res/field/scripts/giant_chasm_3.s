#include "asm/field_script.inc"
#include "text/script/giant_chasm_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rood: Oh! You're safe!\nI'm so glad![f000]븁\u0000\nAs for Team Plasma's ship,\na man in a white lab coat appeared.[f000]븁\u0000\nHe said that a Trainer had given\nhim the answer he'd been seeking,[f000]븀\u0000\nand, as thanks, he would disband[f000]븀\u0000\nTeam Plasma...[f000]븁\u0000\nThen he said, “Farewell! Walk a just path\nwith Pokémon!\" and he flew the ship away.[f000]븀\u0000\nSo everything has been resolved.[f000]븁\u0000\nWe were even able to confirm\nthat Lord N is safe."
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm3_Text_RoodOhYoureSafe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm in no position to say anything.\nIn the past, I stole Pokémon, too."
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm3_Text_ImNoPositionSay, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if that guy who ran off\nhad figured something out..."
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm3_Text_WonderIfGuyWho, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
