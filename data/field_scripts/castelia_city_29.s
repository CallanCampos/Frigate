#include "asm/field_script.inc"
#include "text/script/castelia_city_29.h"

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

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome!\nWelcome to my party![f000]븁\u0000\nPlease enjoy conversations\nwith everyone!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_WelcomeWelcomePartyPlease, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ich habe mir nun ein Auto zugelegt![f000]븁\u0000\nDamit ist die nächste Stadt auch\nganz ohne Orden zum Greifen nah![f000]븁\u0000\nEs sei denn, ich rassele durch\nmeine Führerscheinprüfung...[f000]븁\u0000\nUm...\nI just bought a car![f000]븁\u0000\nNow, even without the Gym Badge, it's\na quick trip to the next city![f000]븁\u0000\nWell, it will be after I get my license..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_IchHabeMirNun, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello. Have you tried a Casteliacone yet?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_HelloHaveTriedCasteliacone, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, Trainer!\nCheck out my moves![f000]븁\u0000\nHave you gone to the next city yet?\nI learned this move at the[f000]븀\u0000\nMusical Theater over there.[f000]븀\u0000\nPretty cool, isn't it?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_HeyTrainerCheckOut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 372
    WorkSet 0x8001, 1
    WorkSet 0x8002, 416
    WorkSet 0x8003, 4
    WorkSet 0x8004, 5
    WorkSet 0x8005, 5
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Did you see the outfit that the\nsupermodel, Elesa, was wearing?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_DidSeeOutfitSupermodel, 5, 3, 0
    MsgWinCloseAll
    // "Whatever Elesa wears is beautiful.\nA stunning ensemble!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_WhateverElesaWearsBeautiful, 6, 5, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My boyfriend isn't good at getting out of\nbed in the morning...[f000]븁\u0000\nIn fact, he's so slow getting out of bed\nthat I asked his Pokémon to use[f000]븀\u0000\nWake-Up Slap on him!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_BoyfriendIsntGoodGetting, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ciao!\nTi stai divertendo?[f000]븁\u0000\nNon sentirti in imbarazzo.\nParla pure con chi vuoi.[f000]븁\u0000\nUmm... Hi there.\nHaving a good time?[f000]븁\u0000\nYou don't have to be shy.\nFeel free to talk to anyone."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_CiaoTiStaiDivertendo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's hard to get up the morning after\na fun day like today."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_ItsHardGetUp, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've asked my Pokémon to use Sing\ninstead of setting an alarm clock.[f000]븁\u0000\nBut I cannot get up at all.\nI don't know why."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_IveAskedPokemonUse, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "¡Es la primera vez que vengo aquí![f000]븁\u0000\n¡Pero este precioso paisaje hace\nque me sienta como en casa![f000]븁\u0000\n¡La próxima vez traeré a mis amigos![f000]븁\u0000\nOh, excuse me.\nDo you understand me now?[f000]븁\u0000\nThis is the first time I've come here,\nand the scenery and the homey[f000]븀\u0000\nenvironment are wonderful![f000]븁\u0000\nNext time, I'll bring my friends!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity29_Text_EsLaPrimeraVez, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
