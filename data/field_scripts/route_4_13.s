#include "asm/field_script.inc"
#include "text/script/route_4_13.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sandstorms are terrible![f000]븁\u0000\nBut Rock-, Ground-, and\nSteel-type Pokémon can weather[f000]븀\u0000\na sandstorm without damage."
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_SandstormsTerribleButRock, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Ggggggrrr!"
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_Ggggggrrr, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x019c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0083
    WordSetLoadPastTradePkmName 1, 0
    // "[f000]Ă\u0001\u0000! [f000]Ă\u0001\u0000!\nThe nickname you gave to the Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_NicknameGavePokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0156

L_0083:
    // "I want to trade your Petilil\nand my Cottonee!"
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_WantTradePetililCottonee, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0148
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0134
    WorkSetConst 0x8022, 0
    FieldTradeCheck 0x8022, TRADE_COTTONEE, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0120
    FieldTradeSavePokemon 0x8020, 1
    // "Pokémon trade!\nPokémon trade![f000]븁\u0000\nPokémon come and go\nvia Infrared Connection![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_PokemonTradePokemonTrade, 0, 0
    MsgWinCloseAll
    FieldTradeStart 25, 0x8020
    // "They were your Petilil and\nmy Cottonee.[f000]븁\u0000\nBut now they are your Cottonee\nand my Petilil!"
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_TheyWerePetililCottonee, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x019c
    VMJump L_012E

L_0120:
    // "The Pokémon I want is Petilil."
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_PokemonWantPetilil, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_012E:
    VMJump L_0142

L_0134:
    // "I see...[f000]븁\u0000\nThen, next time."
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_SeeThenNextTime, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0142:
    VMJump L_0156

L_0148:
    // "I see...[f000]븁\u0000\nThen, next time."
    ParentActorMsg MSGFILE_SCRIPT, Route413_Text_SeeThenNextTime, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0156:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
