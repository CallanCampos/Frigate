#include "asm/field_script.inc"
#include "text/script/global_10100.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0047
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0026:
    ActorMsgClose
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0045
    RTCallGlobal 2005
    VMReturn

L_0045:
    VMReturn

L_0047:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    VMStackPushFlag EVENT_FLAG_0x006a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    // "Sorry, we're getting things ready.\nPlease come back later."
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_SorryWereGettingThings, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_007E:
    PokePartyGetCount 0x8022, 4
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00A9
    // "You have at least one Pokémon\nthat can't be taken."
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_HaveLeastOnePokemon, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_00A9:
    // "Welcome to the Pokémon Wi-Fi Club![f000]븁\u0000\nWould you like to use\nNintendo Wi-Fi Connection?"
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_WelcomePokemonWiFi, 0x8011, 4, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 1

L_00C1:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020C
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_00E7
    VMJump L_00F9

L_00E7:
    VMCall L_0220
    WorkGet 0x8023, 0x8020
    VMJump L_0206

L_00F9:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_010C
    VMJump L_0124

L_010C:
    // "At any Wi-Fi Club, you may play using\nNintendo Wi-Fi Connection.[f000]븁\u0000\nBy doing so, you may trade or battle with\nfriends you have registered in your[f000]븀\u0000\nPal Pad.[f000]븁\u0000\nCheck the monitor inside the room to find\nother people to play with.[f000]븁\u0000\nOr you may play with your friends\nby talking to them directly.[f000]븁\u0000\nWould you like to use\nNintendo Wi-Fi Connection?"
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_AnyWiFiClub, 0x8011, 4, 0
    WorkSetConst 0x8023, 0
    VMJump L_0206

L_0124:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_0137
    VMJump L_0149

L_0137:
    VMCall L_0276
    WorkGet 0x8023, 0x8020
    VMJump L_0206

L_0149:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_015C
    VMJump L_016E

L_015C:
    VMCall L_033C
    WorkGet 0x8023, 0x8020
    VMJump L_0206

L_016E:
    WorkCmpConst 0x8023, 10
    VMJumpIf CMP_EQ, L_0181
    VMJump L_0199

L_0181:
    // "Please do visit again."
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_PleaseVisitAgain, 0x8011, 4, 0
    WorkSetConst 0x8023, 12
    VMJump L_0206

L_0199:
    WorkCmpConst 0x8023, 11
    VMJumpIf CMP_EQ, L_01AC
    VMJump L_01C4

L_01AC:
    // "Communication error."
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_CommunicationError, 0x8011, 4, 0
    WorkSetConst 0x8023, 12
    VMJump L_0206

L_01C4:
    WorkCmpConst 0x8023, 12
    VMJumpIf CMP_EQ, L_01D7
    VMJump L_01E7

L_01D7:
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8024, 0
    VMJump L_0206

L_01E7:
    WorkCmpConst 0x8023, 13
    VMJumpIf CMP_EQ, L_01FA
    VMJump L_0206

L_01FA:
    WorkSetConst 0x8024, 0
    VMJump L_0206

L_0206:
    VMJump L_00C1

L_020C:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    VMReturn

L_0220:
    WorkSetConst 0x8025, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32805
    ListMenuAdd 10, 65535, 2
    ListMenuAdd 11, 65535, 1
    ListMenuAdd 12, 65535, 10
    ListMenuShow
    VMStackPush 0x8025
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0268
    WorkSetConst 0x8020, 10
    VMJump L_026E

L_0268:
    WorkGet 0x8020, 0x8025

L_026E:
    WorkSetConst 0x8025, 0
    VMReturn

L_0276:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    VMCall L_0026
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A9
    WorkSetConst 0x8020, 10
    VMReturn

L_02A9:
    Cmd_015B 0x8028
    Cmd_015C 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_02CC
    WorkSetConst 0x8020, 3
    VMReturn

L_02CC:
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F3
    // "...Oh?[f000]븁\u0000\nI'm sorry, but you don't have any\nfriends registered in your Pal Pad.[f000]븁\u0000\nThe Wi-Fi Club cannot be used if\nno friends are in your Pal Pad.[f000]븁\u0000\nPlease register friends in your\nPal Pad, and then come back.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_OhImSorryBut_2, 0x8011, 4, 0
    WorkSetConst 0x8020, 10
    VMReturn

L_02F3:
    // "...Oh?[f000]븁\u0000\nI'm sorry, but you don't have any\nfriends registered in your Pal Pad.[f000]븁\u0000\nThe Wi-Fi Club cannot be used if\nno friends are in your Pal Pad.[f000]븁\u0000\nWould you like to get your own\nFriend Code at least?[f000]Ȁ\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_OhImSorryBut, 0x8011, 4, 0
    YesNoWin 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0322
    WorkSetConst 0x8020, 3
    VMJump L_0328

L_0322:
    WorkSetConst 0x8020, 10

L_0328:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    VMReturn

L_033C:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    // "Would you like to launch\nNintendo WFC?"
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_WouldLikeLaunchNintendo, 0x8011, 4, 0
    YesNoWin 0x8029
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0379
    WorkSetConst 0x8020, 10
    VMReturn

L_0379:
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    RTCallGlobal 2003
    WorkSet 0x8029, 0x8000
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_03D0
    WorkSetConst 0x8020, 10
    VMReturn

L_03D0:
    VMStackPush 0x8000
    RTCallGlobal 2004
    WorkSet 0x8029, 0x8000
    VMStackPop 0x8000
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0403
    WorkSetConst 0x8020, 10
    VMReturn
    VMJump L_041E

L_0403:
    VMStackPush 0x8029
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_041E
    WorkSetConst 0x8020, 11
    VMReturn

L_041E:
    Cmd_02C5 26
    FunfestBGMReturn
    // "Right this way, please.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10100_Text_RightWayPlease, 0x8011, 4, 0
    ActorMsgClose
    PokePartyRecoverAll
    RTCallGlobal 2105
    FieldSetNextZoneHere
    FlagSet EVENT_FLAG_CONTINUE_SCRIPT
    NetConnectWiFiClub
    RTCallGlobal 2106
    WorkSetConst 0x8020, 13
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x21
    .byte 0x80
    .balign 4, 0
