#include "asm/field_script.inc"
#include "text/script/driftveil_city_13.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Getting to know someone\ncreates both joy and sorrow.[f000]븁\u0000\nPuns that were funny when you first met\nget old when you hear them all the time."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity13_Text_GettingKnowSomeoneCreates, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "As the Gym Leader of Nimbasa City,\nElesa has a shockingly packed schedule.[f000]븁\u0000\nThat's what you'd expect\nfrom an electrifying model!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity13_Text_GymLeaderNimbasaCity, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Three years ago, Team Plasma talked me\ninto letting my dear Pokémon go.[f000]븁\u0000\nEver since, I've been staying\nin hotels as I please.[f000]븁\u0000\n...To be honest, I feel lonely,\nbut it's a good thing not to have Pokémon[f000]븀\u0000\nwho'll be left behind and feel sad[f000]븀\u0000\nafter I pass away..."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity13_Text_ThreeYearsAgoTeam, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
