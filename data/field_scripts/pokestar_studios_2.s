#include "asm/field_script.inc"
#include "text/script/pokestar_studios_2.h"

// Script plugin 10, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The jumpsuit I'm wearing is an essential\npart of creating visual effects![f000]븁\u0000\nIt may be cutting-edge technology,\nbut it's pretty embarrassing to wear!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios2_Text_JumpsuitImWearingEssential, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, ya got this, kid?\nWhat we mean by VFX[f000]븀\u0000\nis visual effects.[f000]븁\u0000\nIt's a technology that lets us\nuse computers to process images.[f000]븁\u0000\nIn Pokéstar Studios movies,\nthe effects are really important!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios2_Text_HeyYaGotKid, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm the screenwriter![f000]븁\u0000\nI write the scripts that become\nthe movie's stories.[f000]븁\u0000\nFeels like my head is packed\nwith nothing but stories."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios2_Text_ImScreenwriterWriteScripts, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello! I'm the cinematographer.[f000]븁\u0000\nWell, put more simply,\nI'm the cameraman."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios2_Text_HelloImCinematographerWell, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey there! I'm the audio engineer![f000]븁\u0000\nMy job is doing things like\nrecording the actor's lines.[f000]븁\u0000\nMost of Pokéstar Studios'\ndialog is dubbed in later, so right now,[f000]븀\u0000\nI have a bit of time on my hands!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios2_Text_HeyThereImAudio, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm a big guy, but I work with\nsmall movie props.[f000]븁\u0000\nSometimes I even make\nthe items actors use or the[f000]븀\u0000\nfurniture on the sets!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios2_Text_ImBigGuyBut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "C'mon... Where could it be?[f000]븁\u0000\nI'm going to get in trouble again\nif I can't find that megaphone![f000]븁\u0000\nOh, do you work in films?\nI'm working as the AD.[f000]븁\u0000\nI guess you could call the\nAD the assistant director."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios2_Text_CmonWhereCouldIm, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
