#include "asm/field_script.inc"
#include "text/script/moor_of_icirrus.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_2:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PokePartyGetCount 0x8020, 0

L_003E:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00CB
    PokePartyGetSpecies 0x8022, 0x8021
    PokePartyGetForme 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 647
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BB
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A2
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_009C
    WorkSetConst 0x8024, 2

L_009C:
    VMJump L_00BB

L_00A2:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_00BB
    WorkSetConst 0x8024, 1

L_00BB:
    DebugPrint 0x8023
    WorkAdd 0x8021, 1
    VMJump L_003E

L_00CB:
    DebugPrint 0x8024
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_00E2
    VMJump L_00FC

L_00E2:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There is an old legend about this place.[f000]븁\u0000\nLong ago, when a war between people\nstarted an intense fire in this forest,[f000]븀\u0000\na single young Pokémon was separated[f000]븀\u0000\nfrom its parents.[f000]븁\u0000\nCobalion, Terrakion, and Virizion\nteamed up to take care of this Pokémon.[f000]븁\u0000\nI wonder what that young Pokémon\ngrew up to be like..."
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_ThereOldLegendAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0192

L_00FC:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_010F
    VMJump L_0147

L_010F:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ooh! That Pokémon! It couldn't be...[f000]븁\u0000\nA bright red mane...and a lush tail...and\na single, noble horn![f000]븁\u0000\nIt's exactly like the old legend![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_OohPokemonCouldntBright, 0, 0
    // "Long ago, when a war between people\nstarted an intense fire in this forest,[f000]븀\u0000\na single young Pokémon was separated[f000]븀\u0000\nfrom its parents.[f000]븁\u0000\nCobalion, Terrakion, and Virizion\nteamed up to take care of this Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_LongAgoWhenWar, 0, 0
    // "The three acted as its parents and\ntaught it the knowledge and the moves[f000]븀\u0000\nit needed to survive...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_ThreeActedItsParents, 0, 0
    // "The young Pokémon grew rapidly and\ndeveloped a power that surpassed its[f000]븀\u0000\nthree caretakers.[f000]븁\u0000\nHowever... One day, that Pokémon\ndisappeared from the forest.[f000]븁\u0000\nNo one knows why.\nBut when I think about it...[f000]븁\u0000\nYoung ones are always reckless and\ndrawn to adventure..."
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_YoungPokemonGrewRapidly, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0192

L_0147:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_015A
    VMJump L_0192

L_015A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ooh! That Pokémon! It couldn't be...[f000]븁\u0000\nA bright red mane...and a lush tail...\nAnd it even has a horn more magnificent[f000]븀\u0000\nthan in the old legend![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_OohPokemonCouldntBright_2, 0, 0
    // "Long ago, when a war between people\nstarted an intense fire in this forest,[f000]븀\u0000\na single young Pokémon was separated[f000]븀\u0000\nfrom its parents.[f000]븁\u0000\nCobalion, Terrakion, and Virizion\nteamed up to take care of this Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_LongAgoWhenWar, 0, 0
    // "The three acted as its parents and\ntaught it the knowledge and the moves[f000]븀\u0000\nit needed to survive...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_ThreeActedItsParents, 0, 0
    // "The young Pokémon grew rapidly and\ndeveloped a power that surpassed its[f000]븀\u0000\nthree caretakers.[f000]븁\u0000\nHowever... One day, that Pokémon\ndisappeared from the forest.[f000]븁\u0000\nNo one knows why.\nBut when I think about it...[f000]븁\u0000\nYoung ones are always reckless and\ndrawn to adventure..."
    ParentActorMsg MSGFILE_SCRIPT, MoorOfIcirrus_Text_YoungPokemonGrewRapidly, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0192

L_0192:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
