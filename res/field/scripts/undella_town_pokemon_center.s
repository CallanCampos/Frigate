#include "asm/field_script.inc"
#include "text/script/undella_town_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0127
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AE
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    TrainerCardGetSex 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0066
    // "Hello, boy![f000]븁\u0000\nDo you have a villa in Undella?\nYou are rich![f000]븁\u0000\nAs a token of our new acquaintance,\nplease accept this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_HelloBoyHaveVilla, 8, 0, 0
    ActorMsgClose
    VMJump L_0074

L_0066:
    // "Hello, girl![f000]븁\u0000\nDo you have a villa in Undella?\nYou are rich![f000]븁\u0000\nAs a token of our new acquaintance,\nplease accept this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_HelloGirlHaveVilla, 8, 0, 0
    ActorMsgClose

L_0074:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 537
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Isn't the Prism Scale beautiful?\nIt may be good to let a Pokémon hold it."
    ActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_IsntPrismScaleBeautiful, 8, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x0127
    VMJump L_00C2

L_00AE:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Isn't the Prism Scale beautiful?\nIt may be good to let a Pokémon hold it."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_IsntPrismScaleBeautiful, 0, 0
    LastKeyWait
    ActorMsgClose

L_00C2:
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Undella Bay's Abyssal Ruins\nhave messages carved into the walls..."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_UndellaBaysAbyssalRuins, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I can buy the Poké Balls sold here,\nbecause somewhere, somebody[f000]븀\u0000\nis making them.[f000]븁\u0000\nThank you, person I don't know,\nmaking Poké Balls somewhere!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_CanBuyPokeBalls, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 255
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 12
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x099b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Heh heh... Hey, you.\nIf someone offered you[f000]븀\u0000\na plain old Sitrus Berry for $1,000,[f000]븀\u0000\nwould you buy it?"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_HehHehHeyIf, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AE
    // "Heh heh...\nYou're all right.[f000]븁\u0000\nWell, whatever. If that's a price\nyou feel you agree with, then great![f000]븁\u0000\nThat's what shopping's all about, right?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_HehHehYoureAll, 0, 0
    VMJump L_01B8

L_01AE:
    // "Heh heh...\nFigured as much.[f000]븁\u0000\nWell, either way is fine. If you don't\nagree with the price, don't buy it...[f000]븁\u0000\nThat's what shopping's all about, right?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_HehHehFiguredMuch, 0, 0

L_01B8:
    MsgWinCloseAll
    Cmd_0275 0, 24, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg UndellaTownPokemonCenter_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "Heh heh...[f000]븁\u0000\nWhat's important is\nwhether you agree or not!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_HehHehWhatsImportant, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x099b
    VMJump L_01FD

L_01E9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Heh heh...[f000]븁\u0000\nWhat's important is\nwhether you agree or not!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTownPokemonCenter_Text_HehHehWhatsImportant, 0, 0
    LastKeyWait
    ActorMsgClose

L_01FD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
