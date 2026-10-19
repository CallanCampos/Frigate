#include "asm/field_script.inc"
#include "text/script/global_2500.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's lined with Pokémon picture books."
    InfoMsg Global2500_Text_ItsLinedPokemonPicture, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Are these books for Pokémon\nto read?"
    InfoMsg Global2500_Text_TheseBooksPokemonRead, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's chock full of Pokémon books and\nphoto collections."
    InfoMsg Global2500_Text_ItsChockFullPokemon, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There are lots of Pokémon magazines![f000]븁\u0000\n“Pokémon Pal\"\n“Pokémon Handbook\"[f000]븀\u0000\n“Adorable Pokémon\""
    InfoMsg Global2500_Text_ThereLotsPokemonMagazines, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The trash can is empty."
    InfoMsg Global2500_Text_TrashCanEmpty, 2
    LastKeyWait
    InfoMsgClose_0039
    RecordAdd 43, 1
    Cmd_02C5 11
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The shelves are jammed with vibrant\nPokémon goods."
    InfoMsg Global2500_Text_ShelvesJammedVibrantPokemon, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It looks like a cabinet full of\nPokémon goods."
    InfoMsg Global2500_Text_LooksLikeCabinetFull, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Goods are arranged on the shelves in a\nvery orderly fashion."
    InfoMsg Global2500_Text_GoodsArrangedShelvesVery, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
