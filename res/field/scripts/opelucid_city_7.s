#include "asm/field_script.inc"
#include "text/script/opelucid_city_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is just between you and me, OK?\nI used to be part of Team Plasma.[f000]븁\u0000\nBut, I felt like if I were just going\nto do what I was told without thinking,[f000]븀\u0000\nit didn't have to be me doing it.[f000]븁\u0000\nSo I left Team Plasma.[f000]븁\u0000\nDon't tell this story to the guy on the\nopposite side."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity7_Text_JustBetweenOkUsed, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is just between you and me, OK?\nI'm really not very good as a Trainer.[f000]븁\u0000\nSo I think Team Plasma might have the\nright idea when they take Pokémon away[f000]븀\u0000\nfrom weak Trainers.[f000]븁\u0000\nI do feel sorry for the people who were\nrobbed, though.[f000]븁\u0000\nDon't tell this story to the guy on the\nopposite side."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity7_Text_JustBetweenOkIm, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
