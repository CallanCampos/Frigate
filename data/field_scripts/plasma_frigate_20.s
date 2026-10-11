#include "asm/field_script.inc"
#include "text/script/plasma_frigate_20.h"

// Script plugin 12, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ah. I slept very well![f000]븁\u0000\nYou're wearing strange clothes.\nAre you new here?[f000]븁\u0000\nAny bed in this room is available.\nFeel free to use them if you're tired!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate20_Text_AhSleptVeryWell, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "The bed looks nice and comfortable.\nWill you take a quick rest?"
    SystemMsg PlasmaFrigate20_Text_BedLooksNiceComfortable, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007C
    // "[f000]Ā\u0001\u0000 hopped into bed and\nfell asleep...[f000]븁\u0000"
    SystemMsg PlasmaFrigate20_Text_HoppedIntoBedFell, 0
    InfoMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "[f000]Ā\u0001\u0000 and the Pokémon\ntook a nap and regained energy!"
    SystemMsg PlasmaFrigate20_Text_PokemonTookNapRegained, 0
    LastKeyWait

L_007C:
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
