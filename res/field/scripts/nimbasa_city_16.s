#include "asm/field_script.inc"
#include "text/script/nimbasa_city_16.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x00d5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005F
    // "If I compared the glow of Nimbasa to\nsomething, it would be the sun![f000]븁\u0000\nYes, the sun!\nIt inspires me to give you this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity16_Text_IfComparedGlowNimbasa, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 80
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x00d5

L_005F:
    // "Sun Stones are stones that make certain\nPokémon evolve![f000]븁\u0000\nI gave it to Petilil as a present!"
    // "Sun Stones are stones that make certain\nPokémon evolve![f000]븁\u0000\nI gave it to Cottonee as a present!"
    ActorMsgVersioned 1024, NimbasaCity16_Text_SunStonesStonesMake, NimbasaCity16_Text_SunStonesStonesMake_2, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    PokePartyGetMemberByType 0x8010, 2
    WordSetPartyPokeSpecies 0, 0x8010
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you love your Pokémon, even if it\nchanges its appearance, you'll stay[f000]븀\u0000\nconnected with your Pokémon.[f000]븁\u0000\nYou and [f000]ā\u0001\u0000 are...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity16_Text_IfLovePokemonEven, 0, 0
    PokePartyGetHappiness 0x8020, 0x8010
    VMStackPush 0x8020
    VMStackPushConst 120
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00BD
    // "Very connected to each other!\nThat's what it looks like to me!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity16_Text_VeryConnectedEachOther, 0, 0
    VMJump L_00EA

L_00BD:
    VMStackPush 0x8020
    VMStackPushConst 70
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00E0
    // "Just starting to understand\neach other. That's what it seems like..."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity16_Text_JustStartingUnderstandEach, 0, 0
    VMJump L_00EA

L_00E0:
    // "Perhaps capable of being more connected.\nI hope you enjoy the changes."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity16_Text_PerhapsCapableBeingMore, 0, 0

L_00EA:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon evolve in many different ways.[f000]븁\u0000\nSome evolve by becoming stronger\nthrough battle.[f000]븁\u0000\nOthers evolve when certain items\nare used on them.[f000]븁\u0000\nSome even evolve during Link Trades.[f000]븁\u0000\nIf you ask Professor Juniper, she'll\ntell you anything you need to know!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity16_Text_PokemonEvolveManyDifferent, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
