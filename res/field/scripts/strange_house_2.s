#include "asm/field_script.inc"
#include "text/script/strange_house_2.h"

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
    // "There is a Pokémon called Cresselia\nin the far Sinnoh region.[f000]븁\u0000\nIts wings shine like the crescent moon\nand keep nightmares away."
    InfoMsg StrangeHouse2_Text_TherePokemonCalledCresselia, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There is a Pokémon called Darkrai\nin the far Sinnoh region.[f000]븁\u0000\nTo protect itself, it drives people and\nPokémon away with terrible nightmares."
    InfoMsg StrangeHouse2_Text_TherePokemonCalledDarkrai, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There are Pokémon called Drowzee.[f000]븁\u0000\nThey put others to sleep and eat their\ndreams. Eating nightmares can upset[f000]븀\u0000\ntheir stomachs."
    InfoMsg StrangeHouse2_Text_TherePokemonCalledDrowzee, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There are Pokémon called Hypno.[f000]븁\u0000\nEach one carries a pendulum that\nit can swing to make people drowsy.[f000]븁\u0000\nIt has been said that a Hypno once\nhypnotized a child and took it away..."
    InfoMsg StrangeHouse2_Text_TherePokemonCalledHypno, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Some Pokémon know a move called\nDream Eater.[f000]븁\u0000\nWith this move, a Pokémon attacks while\nthe target is asleep and eats its dream.[f000]븁\u0000\nIt restores HP equal to half of\nthe damage inflicted on the target."
    InfoMsg StrangeHouse2_Text_SomePokemonKnowMove, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Some Pokémon have the Forewarn Ability.[f000]븁\u0000\nA Pokémon with this Ability is alerted to\none of the opposing Pokémon's moves.[f000]븁\u0000\nHigh-power moves will be recognized first."
    InfoMsg StrangeHouse2_Text_SomePokemonHaveForewarn, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "A lot of books from other regions\nare stored on the tall shelf."
    InfoMsg StrangeHouse2_Text_LotBooksFromOther, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
