#include "asm/field_script.inc"
#include "text/script/plasma_frigate_15.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Helping someone even though the person\ndoesn't ask for help...[f000]븀\u0000\nIt's like, “Who do you think you are?\"[f000]븁\u0000\nI don't get it, because I can't tell\nwhether another person is happy or not."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate15_Text_HelpingSomeoneEvenThough, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nobody has used the beds\nin this room.[f000]븁\u0000\nIf you think I'm lying, take a look,\nthen take a rest!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate15_Text_NobodyHasUsedBeds, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "The bed looks nice and comfortable.\nWill you take a quick rest?"
    SystemMsg PlasmaFrigate15_Text_BedLooksNiceComfortable, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009C
    // "[f000]Ā\u0001\u0000 hopped into bed and\nfell asleep...[f000]븁\u0000"
    SystemMsg PlasmaFrigate15_Text_HoppedIntoBedFell, 0
    InfoMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "[f000]Ā\u0001\u0000 and the Pokémon\ntook a nap and regained energy!"
    SystemMsg PlasmaFrigate15_Text_PokemonTookNapRegained, 0
    LastKeyWait

L_009C:
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
