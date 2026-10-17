#include "asm/field_script.inc"
#include "text/script/pokestar_studios_5.h"

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
    ScriptEntry Script_12
    ScriptEntriesEnd

Script_1:
    VMStackPush EVENT_WORK_0x413c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    ActorSetGPos 0, 12, 0, 16, 2

L_0051:
    VMHalt

Script_2:
    ActorsPauseAll
    WordSetPlayerName 0
    // "This is a first-class, ultra-deluxe\ndressing room that only stars recognized[f000]븀\u0000\nby Mr. Deeoh can use![f000]븁\u0000\nFrom now on, only you have\npermission to use this room freely![f000]븁\u0000\nSee you, [f000]Ā\u0001\u0000!\nI'm looking forward to[f000]븀\u0000\nseeing your next movie!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios5_Text_FirstClassUltraDeluxe, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    ActorCmdExec 0, Movement_008C
    ActorCmdWait
    ActorSetGPos 0, 12, 0, 30, 2
    WorkSetConst EVENT_WORK_0x413c, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_008C:
    Move 15, 7
    MoveEnd

Script_3:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Top stars always have such\ngreat faces...[f000]븁\u0000\n[f000]Ā\u0001\u0000, your eyelids\nshine brighter than any eye shadow![f000]븁\u0000\nJust like the boss when he\nwas younger..."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios5_Text_TopStarsAlwaysHave, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Top stars always have such\ngreat faces...[f000]븁\u0000\n[f000]Ā\u0001\u0000, your lips\nare glossier than any lipstick![f000]븁\u0000\nJust like the boss when he\nwas younger..."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios5_Text_TopStarsAlwaysHave_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, [f000]Ā\u0001\u0000,\nare you using your own Pokémon[f000]븀\u0000\nwhen you shoot movies?[f000]븁\u0000\nPokémon are actors, too![f000]븁\u0000\nIf they act well in movies, they'll\nalso become like shining stars!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios5_Text_HeyUsingOwnPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm the guard for this room.\nI've been waiting for your arrival.[f000]븁\u0000\nWhen I was young, my boss was...[f000]븁\u0000\nMr. Brycen...[f000]븁\u0000\nYou're the third star\nto use this dressing room."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios5_Text_ImGuardRoomIve, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a lot of fan letters!"
    InfoMsg PokestarStudios5_Text_ItsLotFanLetters, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's packed with worn-\nout, old movie scripts...[f000]븁\u0000\n“Shuckle-Berry Finneon\"[f000]븁\u0000\n“The Kricketune of Castelia Square\"[f000]븁\u0000\n“The Black Clamperl\"[f000]븁\u0000\n“Saint Geodude and the Dragonite\"[f000]븁\u0000\n“Make Way for Ducklett\"[f000]븁\u0000\n“The Boxcar Cinccino\"[f000]븁\u0000\n“The Trumpet of the Swanna\"[f000]븁\u0000\n“The Legend of Sleepy Drowzee\"[f000]븁\u0000\n“Little Wurmple\"[f000]븁\u0000\n“A Tale of Two Skitty\"[f000]븁\u0000\n“The Safari Zone Book\"[f000]븁\u0000\n“The Tale of Betty Buneary\"[f000]븁\u0000\n“One Basculin, Two Basculin,\nRed-Striped Basculin,[f000]븀\u0000\nBlue-Striped Basculin\"[f000]븁\u0000\n“The Empoleon's New Clothes\"[f000]븁\u0000\n“The Cobalion and the Sandshrew\"[f000]븁\u0000\n“The House at Foongus Corner\"[f000]븁\u0000\n“The Reluctant Dragonite\"[f000]븁\u0000\n“Three Little Tepig\"[f000]븁\u0000\n“Rip Van Dwebble\"[f000]븁\u0000\n“Galvantula's Travels\"[f000]븁\u0000\n“Galvantula's Travels 2:\nEelektrik Boogaloo\"[f000]븁\u0000\n“The Golett\"[f000]븁\u0000\n“The Pokey Little Lillipup\"[f000]븁\u0000\n“Enspoinklopedia Brown\"[f000]븁\u0000\n“Snivy in Ivyland\""
    InfoMsg PokestarStudios5_Text_ItsPackedWornOut, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a huge mirror that looks\nlike it will reach the ceiling!"
    InfoMsg PokestarStudios5_Text_ItsHugeMirrorLooks, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "These vividly colored bottles\nare for makeup..."
    InfoMsg PokestarStudios5_Text_TheseVividlyColoredBottles, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There are costumes with lamé fabric!\nThe gold gleams in the light!"
    InfoMsg PokestarStudios5_Text_ThereCostumesLameFabric, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a costume made of gold lamé!\nIt gleams in the light![f000]븁\u0000\nThere's also a dress with a Swanna\nDoll that wraps around the waist!"
    InfoMsg PokestarStudios5_Text_ItsCostumeMadeGold, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
