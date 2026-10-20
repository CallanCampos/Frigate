#include "asm/field_script.inc"
#include "text/script/nuvema_town_7.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "[f000]ă\u0001\u0000 checked the PC screen.[f000]븁\u0000\nAdventure Rule No. 1\nThe X Button opens the menu![f000]븁\u0000\nAdventure Rule No. 2\nRecord your progress with SAVE.[f000]븁\u0000\nThere is nothing else on here..."
    SystemMsg NuvemaTown7_Text_CheckedPcScreenAdventure, 2
    LastKeyWait
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
