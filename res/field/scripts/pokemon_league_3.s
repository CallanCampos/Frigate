#include "asm/field_script.inc"
#include "text/script/pokemon_league_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Nothing happens!\nIt seems you can't go back until you win."
    InfoMsg PokemonLeague3_Text_NothingHappensSeemsCant, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
