#include "asm/field_script.inc"
#include "text/script/mistralton_cave.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    FlagSet EVENT_FLAG_0x03b6
    FlagSet EVENT_FLAG_0x03b7
    FlagSet EVENT_FLAG_0x03b9
    FlagSet EVENT_FLAG_0x03bb
    FlagSet EVENT_FLAG_0x03bd
    FlagReset EVENT_FLAG_0x03b4
    FlagReset EVENT_FLAG_0x03b5
    FlagReset EVENT_FLAG_0x03b8
    FlagReset EVENT_FLAG_0x03ba
    FlagReset EVENT_FLAG_0x03bc
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I heard there was a legendary Pokémon\nin Mistralton Cave..."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCave_Text_HeardThereLegendaryPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
