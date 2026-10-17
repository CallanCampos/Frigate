#include "asm/field_script.inc"
#include "text/script/accumula_town_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The triplets in Striaton City\noften head over to Route 2.[f000]븁\u0000\nThey compete with Trainers and Pokémon,\nso they're really tough!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown4_Text_TripletsStriatonCityOften, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Join Avenue...\nI wonder what it's like now.[f000]븁\u0000\nAnd Pokéstar Studios!\nWhat movies are playing?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown4_Text_JoinAvenueWonderWhat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Guess what I know![f000]븁\u0000\nPokémon can only know four\nmoves at one time!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown4_Text_GuessWhatKnowPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
