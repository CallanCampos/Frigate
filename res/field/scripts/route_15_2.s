#include "asm/field_script.inc"
#include "text/script/route_15_2.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0103
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0035
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am so grateful that you traded Pokémon\nwith me![f000]븁\u0000\nI've traded a Rotom and a Ditto\nbefore as well...[f000]븁\u0000\nI guess I just like Ditto!"
    ParentActorMsg MSGFILE_SCRIPT, Route152_Text_AmGratefulTradedPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0114

L_0035:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "By any chance, have you caught a\nPokémon called Rotom?[f000]븁\u0000\nI would be very happy if you would trade\nmy Rotom for your Ditto."
    ActorMsg MSGFILE_SCRIPT, Route152_Text_ByAnyChanceHave, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0104
    ActorMsgClose
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EE
    WorkSetConst 0x8022, 0
    FieldTradeCheck 0x8022, TRADE_ROTOM, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D8
    // "My heart is beating so fast!\nOK, let's trade![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route152_Text_HeartBeatingFastOk, 0, 0, 0
    MsgWinCloseAll
    FieldTradeStart 28, 0x8020
    // "Thank you very much! Please treat my\nRotom with love![f000]븁\u0000\nI will also take good care of the\nDitto that you traded to me!"
    ActorMsg MSGFILE_SCRIPT, Route152_Text_ThankVeryMuchPlease, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x0103
    VMJump L_00E8

L_00D8:
    // "I'd like to trade for a Ditto..."
    ActorMsg MSGFILE_SCRIPT, Route152_Text_IdLikeTradeDitto, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_00E8:
    VMJump L_00FE

L_00EE:
    // "Well, if you don't want to, I understand.[f000]븁\u0000\nBut if you ever change your mind, please\ntrade Pokémon with me!"
    ActorMsg MSGFILE_SCRIPT, Route152_Text_WellIfDontWant, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_00FE:
    VMJump L_0114

L_0104:
    // "Well, if you don't want to, I understand.[f000]븁\u0000\nBut if you ever change your mind, please\ntrade Pokémon with me!"
    ActorMsg MSGFILE_SCRIPT, Route152_Text_WellIfDontWant, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0114:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
