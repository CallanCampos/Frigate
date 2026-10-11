#include "asm/field_script.inc"
#include "text/script/route_7_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01e5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Long ago, I was a sharp-lookin' young\nman, and my wife was a fine-lookin' girl.[f000]븀\u0000\nThis is a story from way back then.[f000]븁\u0000\nRight around here, a certain Pokémon\ntaught Tornadus and Thundurus a lesson.[f000]븁\u0000\nFor about 30 years after that, those\ntwo Pokémon kept it calm and quiet.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route73_Text_LongAgoSharpLookin, 1, 0, 0
    MsgWinCloseAll
    // "Those two Pokémon were quieted\nby a Pokémon known as Landorus.[f000]븁\u0000\nI remember hearin' that there's a\nshrine built in honor of Landorus[f000]븀\u0000\nsomewhere on Route 14."
    ActorMsg MSGFILE_SCRIPT, Route73_Text_ThoseTwoPokemonWere, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00D5

L_0051:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Long ago, I was a sharp-lookin' young\nman, and my wife was a fine-lookin' girl.[f000]븀\u0000\nThis is a story from way back then.[f000]븁\u0000\nRight around here, a certain Pokémon\ntaught Tornadus and Thundurus a lesson.[f000]븁\u0000\nFor about 30 years after that, those\ntwo Pokémon kept it calm and quiet.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route73_Text_LongAgoSharpLookin_2, 1, 0, 0
    // "I'll show them to you.\nThis is Tornadus.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route73_Text_IllShowThemTornadus, 1, 0, 0
    MsgWinCloseAll
    CallPokemonPreview 641, 0, 0, 0
    PokeDexRegist 0, 641
    // "And this is Thundurus![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route73_Text_Thundurus, 1, 0, 0
    MsgWinCloseAll
    CallPokemonPreview 642, 0, 0, 0
    PokeDexRegist 0, 642
    // "Tornadus and Thundurus have been\nregistered in the Pokédex."
    SystemMsg Route73_Text_TornadusThundurusHaveBeen, 0
    MsgWaitAdvance
    InfoMsgClose
    // "The Pokémon who quieted the others\nis known as Landorus.[f000]븁\u0000\nFeast yer eyes![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route73_Text_PokemonWhoQuietedOthers, 1, 0, 0
    MsgWinCloseAll
    CallPokemonPreview 645, 0, 0, 0
    PokeDexRegist 0, 645
    // "Landorus has been registered\nin the Pokédex."
    SystemMsg Route73_Text_LandorusHasBeenRegistered, 0
    MsgWaitAdvance
    InfoMsgClose
    FlagSet EVENT_FLAG_0x01e5

L_00D5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In the Unova region, you see,\nthere's a Pokémon called Thundurus[f000]븀\u0000\nthat causes thunder, and another called[f000]븀\u0000\nTornadus that causes heavy rain.[f000]븁\u0000\nThey fly around the region lettin' wild\nwinds loose while the rain pounds and[f000]븀\u0000\nthe lightnin' crashes.[f000]븁\u0000\nThey ruin the crops I work so hard\nto raise."
    ParentActorMsg MSGFILE_SCRIPT, Route73_Text_UnovaRegionSeeTheres, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I saw them![f000]븁\u0000\nTheir appearances were totally\ndifferent, but they must be Landorus,[f000]븀\u0000\nTornadus, and Thundurus!"
    ParentActorMsg MSGFILE_SCRIPT, Route73_Text_SawThemTheirAppearances, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PokePartyGetCount 0x8020, 0

L_013F:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0193
    PokePartyIsFullHP 0x8022, 0x8021
    PokePartyIsFullPP 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0187
    WorkAdd 0x8024, 1

L_0187:
    WorkAdd 0x8021, 1
    VMJump L_013F

L_0193:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_01E6
    // "Oh, dear. Your Pokémon team...\nIt seems a bit sluggish.[f000]븁\u0000\nRest for just a moment now![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route73_Text_OhDearPokemonTeam, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Walking on the raised walkways is\nmore tiring than you think, right?"
    ParentActorMsg MSGFILE_SCRIPT, Route73_Text_WalkingRaisedWalkwaysMore, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01F4

L_01E6:
    // "Walking on the raised walkways is\nmore tiring than you think, right?"
    ParentActorMsg MSGFILE_SCRIPT, Route73_Text_WalkingRaisedWalkwaysMore, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01F4:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
