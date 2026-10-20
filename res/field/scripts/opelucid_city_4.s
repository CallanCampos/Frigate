#include "asm/field_script.inc"
#include "text/script/opelucid_city_4.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome! Glad to see you here at the\nTriple Battle House.[f000]븁\u0000\nThat's what we would say up until\na few years ago.[f000]븁\u0000\nNow, the Trainers are seeing\nwho's the best at Triple Battles[f000]븀\u0000\nin the Pokémon World Tournament[f000]븀\u0000\nin Driftveil City!"
    // "Welcome! Glad to see you here at the\nRotation Battle House.[f000]븁\u0000\nThat's what we would say up until\na few years ago.[f000]븁\u0000\nNow, the Trainers are seeing\nwho's the best at Rotation Battles[f000]븀\u0000\nin the Pokémon World Tournament[f000]븀\u0000\nin Driftveil City!"
    ActorMsgVersioned 1024, OpelucidCity4_Text_WelcomeGladSeeHere_2, OpelucidCity4_Text_WelcomeGladSeeHere, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
