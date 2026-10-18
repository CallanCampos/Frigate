#include "asm/field_script.inc"
#include "text/script/mistralton_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntry Script_11
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

Script_11:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    RTCGetDate 0x802a, 0x8029
    ItemGetCount ITEM_SWEET_HEART, 0x802b
    ItemCheckSpace ITEM_HEART_SCALE, 1, 0x802c
    WordSetItemName 0, 93
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sweets, lovely Sweet Hearts! ♪\nCheck feelings between two people.[f000]븁\u0000\nIf you are a great match, you can\nget sweet on Sweet Hearts! ♪[f000]븀\u0000\nMeltingly sweet Sweet Hearts! ♪[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_SweetsLovelySweetHearts, 1, 0, 0
    VMStackPush 0x802a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x802a
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0111
    VMStackPush 0x8029
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0105
    WorkSetConst 0x802e, 5
    VMJump L_010B

L_0105:
    WorkSetConst 0x802e, 10

L_010B:
    VMJump L_0117

L_0111:
    WorkSetConst 0x802e, 10

L_0117:
    DebugPrint 0x802e
    DebugPrint 0x802b
    VMStackPush 0x802b
    VMStackPush 0x802e
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_01B0
    VMStackPush 0x802a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x802a
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0198
    VMStackPush 0x8029
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0180
    // "I'm in a special mood today![f000]븁\u0000\nWould you trade me five Sweet Hearts\nfor a Heart Scale?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_ImSpecialMoodToday, 1, 0, 0
    VMCall L_0266
    VMJump L_0192

L_0180:
    // "Oh, uh, you?\nYou have Sweet Hearts![f000]븁\u0000\nWill you trade 10 Sweet Hearts\nfor my Heart Scale?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OhUhHaveSweet, 1, 0, 0
    VMCall L_01C6

L_0192:
    VMJump L_01AA

L_0198:
    // "Oh, uh, you?\nYou have Sweet Hearts![f000]븁\u0000\nWill you trade 10 Sweet Hearts\nfor my Heart Scale?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OhUhHaveSweet, 1, 0, 0
    VMCall L_01C6

L_01AA:
    VMJump L_01C0

L_01B0:
    // "If you bring a lot of Sweet Hearts,\nI will trade you for something happy!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_IfBringLotSweet, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01C0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01C6:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0254
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0206
    // "Oh... What a pity.\nWhat a great pity![f000]븁\u0000\nYou must have a lot of\nHeart Scales already."
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OhWhatPityWhat, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_024E

L_0206:
    ActorCmdExec 1, Movement_0860
    ActorCmdWait
    // "Yaaaay!\nI have a lot of your heart![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_YaaaayHaveLotHeart, 1, 0, 0
    MsgWinCloseAll
    // "Gave the Sweet Hearts and received\na Heart Scale in return.[f000]븁\u0000"
    SystemMsg MistraltonCity3_Text_GaveSweetHeartsReceived, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 93
    WorkSet 0x8001, 1
    RTCallGlobal 2802
    VMStackPop 0x8001
    VMStackPop 0x8000
    ItemSub ITEM_SWEET_HEART, 10, 0x802d

L_024E:
    VMJump L_0264

L_0254:
    // "OK...\nAww...that's too bad."
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkAwwThatsToo, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0264:
    VMReturn

L_0266:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F4
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A6
    // "Oh... What a pity.\nWhat a great pity![f000]븁\u0000\nYou must have a lot of\nHeart Scales already."
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OhWhatPityWhat, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02EE

L_02A6:
    ActorCmdExec 1, Movement_0860
    ActorCmdWait
    // "Yaaaay!\nI have a lot of your heart![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_YaaaayHaveLotHeart, 1, 0, 0
    MsgWinCloseAll
    // "Gave the Sweet Hearts and received\na Heart Scale in return.[f000]븁\u0000"
    SystemMsg MistraltonCity3_Text_GaveSweetHeartsReceived, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 93
    WorkSet 0x8001, 1
    RTCallGlobal 2802
    VMStackPop 0x8001
    VMStackPop 0x8000
    ItemSub ITEM_SWEET_HEART, 5, 0x802d

L_02EE:
    VMJump L_0304

L_02F4:
    // "OK...\nAww...that's too bad."
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkAwwThatsToo, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0304:
    VMReturn

Script_2:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 244
    WorkSet 0x8001, 1
    WorkSet 0x8002, 121
    WorkSet 0x8003, 32
    WorkSet 0x8004, 33
    WorkSet 0x8005, 33
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    TrainerCardHasBadge 0x8008, 5
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 3
    VMStackCmp CMP_LE
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03B9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Professor Juniper is researching\nPokémon at Celestial Tower,[f000]븀\u0000\nwhich is at the end of Route 7.[f000]븁\u0000\nPay attention to the signs\nso you don't get lost on the way."
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_ProfessorJuniperResearchingPokemon, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0524

L_03B9:
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0524
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0122
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FB
    // "Looks like the Professor's reached a\nstopping point in her investigation.[f000]븁\u0000\nReady to hop aboard my plane?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_LooksLikeProfessorsReached, 4, 0, 0
    FlagSet EVENT_FLAG_0x0122
    VMJump L_0407

L_03FB:
    // "Are you ready to get aboard the plane?\nThe sky is calling!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_ReadyGetAboardPlane, 4, 0, 0

L_0407:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0514
    // "Hee-hee!\nReady for takeoff!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HeeHeeReadyTakeoff, 4, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0840
    ActorCmdWait
    // "OK, Skyla, we're ready.\nPlease take us to Lentimas Town![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkSkylaWereReady, 2, 0, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x0301
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 3
    SEWait
    ActorWalkRoute 3, 18, 21, 1, 4, 1
    ActorCmdWait
    ActorCmdExec 3, Movement_0848
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0483
    VMJump L_0491

L_0483:
    ActorCmdExec 255, Movement_0850
    VMJump L_04BA

L_0491:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_04A4
    VMJump L_04BA

L_04A4:
    ActorCmdExec 255, Movement_0850
    ActorCmdExec 4, Movement_0850
    VMJump L_04BA

L_04BA:
    ActorCmdExec 2, Movement_0850
    ActorCmdWait
    // "Bianca: Waaaaaaaaaaait![f000]븁\u0000\nYou guys! Wait, wait, wait, wait!\nHff...pff...I want to fly, too![f000]븁\u0000\nI want to do some research\nin Reversal Mountain...hff...pff...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_BiancaWaaaaaaaaaaaitGuysWait, 3, 0, 0
    MsgWinCloseAll
    // "Professor Juniper: Bianca, you're here?[f000]븁\u0000\nYou're starting to show the dedication\nof a serious researcher these days![f000]븁\u0000\nOK, everyone, off we go to Lentimas Town![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_ProfessorJuniperBiancaYoure, 2, 1, 0
    MsgWinCloseAll
    // "Skyla: Hee-hee!\nLooks like everyone's here![f000]븁\u0000\nFinally, it's time to fly\nthe Unova skies![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_SkylaHeeHeeLooks, 4, 1, 0
    MsgWinCloseAll
    VMCall L_07D6
    FlagSet EVENT_FLAG_0x02ff
    FlagSet EVENT_FLAG_0x0300
    FlagSet EVENT_FLAG_0x0301
    FlagSet EVENT_FLAG_0x0316
    FlagReset EVENT_FLAG_0x03ee
    WorkSetConst EVENT_WORK_0x40cb, 1
    VMJump L_0524

L_0514:
    // "Roger!\nCome talk to me when you're ready."
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_RogerComeTalkWhen, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0524:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Professor Juniper: Hi there!\nLooks like I kept you waiting! Sorry...[f000]븁\u0000\nShall we give Skyla her chance to\nshow us her piloting skills?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_ProfessorJuniperHiThere, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x802f, 0
    MedalIsObtained 0x802f, 57
    VMStackPush 0x802f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05AE
    VMStackPushFlag EVENT_FLAG_0x0177
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_059A
    // "Hrmph! You there!\nDo you have the Ace Pilot Medal?[f000]븁\u0000\nWe Pilots figure that's the coolest\nMedal of them all, so I thought a[f000]븀\u0000\ncool customer like you might have it!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HrmphThereHaveAce, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0177
    VMJump L_05A8

L_059A:
    // "If you get the Ace Pilot Medal,\nplease come show it to me!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_IfGetAcePilot, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_05A8:
    VMJump L_05FA

L_05AE:
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05FA
    VMStackPushFlag EVENT_FLAG_0x0178
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05EC
    // "Ooh! Is that it? Shiny![f000]븁\u0000\nTh-the Ace Pilot Medal...[f000]븁\u0000\nBrimming with adventure,\nshimmering like the sky--[f000]븀\u0000\nthe Ace Pilot Medal![f000]븁\u0000\nSo cool.\nThank you for letting me see it!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OohShinyThAce, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0178
    VMJump L_05FA

L_05EC:
    // "“Ace Pilot\"...\nThere's a splendid sound to that.[f000]븁\u0000\nHere's to you and your Pokémon\nfor flying freely through the skies![f000]븁\u0000\nFor my part, I'll keep working on\nmy skills as an airplane pilot!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_AcePilotTheresSplendid, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_05FA:
    WorkSetConst 0x802f, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40cb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0637
    // "Hello!\nThis is Mistralton Cargo Service.[f000]븁\u0000\n“Deliver a lot of cargo quickly!\"[f000]븁\u0000\nWe can also carry passengers now,\nbut that's up to the Gym Leader."
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HelloMistraltonCargoService, 6, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_07D0

L_0637:
    ItemCheckAmount ITEM_PERMIT, 1, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06A5
    // "Hello!\nThis is Mistralton Cargo Service.[f000]븁\u0000\n“Deliver a lot of cargo quickly!\"\nWould you like to board the flight[f000]븀\u0000\nto Lentimas Town?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HelloMistraltonCargoService_2, 6, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_068F
    // "OK. We will contact Skyla,\nso please board the plane and wait.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkWeWillContact, 6, 2, 0
    MsgWinCloseAll
    VMCall L_07F6
    VMJump L_069F

L_068F:
    // "OK!\nFeel free to fly with us anytime!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkFeelFreeFly, 6, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_069F:
    VMJump L_07D0

L_06A5:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01a5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0737
    // "This is Mistralton Cargo Service.\n“Deliver a lot of cargo quickly!\"[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_MistraltonCargoServiceDeliver, 6, 2, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0858
    ActorCmdWait
    // "Oh!\nThat's a Permit![f000]븁\u0000\nThat lets you enter the\nNature Preserve, which is far,[f000]븀\u0000\nfar away from the Unova region![f000]븁\u0000\nWould you like to go to the\nNature Preserve, then?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OhThatsPermitLets, 6, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0721
    FlagSet EVENT_FLAG_0x01a5
    // "OK. We will contact Skyla,\nso please board the plane and wait.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkWeWillContact, 6, 2, 0
    MsgWinCloseAll
    VMCall L_0816
    VMJump L_0731

L_0721:
    // "OK!\nFeel free to fly with us anytime!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkFeelFreeFly, 6, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0731:
    VMJump L_07D0

L_0737:
    // "“Deliver a lot of cargo quickly!\"\nThis is Mistralton Cargo Service.[f000]븁\u0000\nWhere would you like to fly?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_DeliverLotCargoQuickly, 6, 2, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32801
    ListMenuAdd 17, 65535, 0
    ListMenuAdd 18, 65535, 1
    ListMenuAdd 19, 65535, 2
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0793
    // "OK. We will contact Skyla,\nso please board the plane and wait.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkWeWillContact, 6, 2, 0
    MsgWinCloseAll
    VMCall L_07F6
    VMJump L_07D0

L_0793:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C0
    // "OK. We will contact Skyla,\nso please board the plane and wait.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkWeWillContact, 6, 2, 0
    MsgWinCloseAll
    VMCall L_0816
    VMJump L_07D0

L_07C0:
    // "OK!\nFeel free to fly with us anytime!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OkFeelFreeFly, 6, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_07D0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_07D6:
    FadeOutBlackQ
    FadeWait
    FieldClose
    Call3DDemo 24, 0
    FieldOpen
    RTReserveScript 9
    MapChangeCore ZONE_LENTIMAS_TOWN, 617, 65531, 305, 2
    VMReturn

L_07F6:
    FadeOutBlackQ
    FadeWait
    FieldClose
    Call3DDemo 24, 0
    FieldOpen
    RTReserveScript 9
    MapChangeCore ZONE_LENTIMAS_TOWN, 616, 65531, 305, 1
    VMReturn

L_0816:
    FadeOutBlackQ
    FadeWait
    FieldClose
    Call3DDemo 24, 0
    FieldOpen
    RTReserveScript 3
    MapChangeCore ZONE_NATURE_PRESERVE, 21, 0, 31, 0
    VMReturn
    .balign 4, 0
    Move 35, 1
    MoveEnd

Movement_0840:
    Move 34, 1
    MoveEnd

Movement_0848:
    Move 32, 1
    MoveEnd

Movement_0850:
    Move 33, 1
    MoveEnd

Movement_0858:
    Move 75, 1
    MoveEnd

Movement_0860:
    Move 160, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you use the move Fly,\nyou can return to a Pokémon Center[f000]븀\u0000\nyou've already visited."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_WhenUseMoveFly, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you don't know a lot about the\nstructure of planes, how can you[f000]븀\u0000\nmaintain them?[f000]븁\u0000\nPokémon battling is the same.[f000]븁\u0000\nThe more you know about Pokémon,\nthe more you can win!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_IfDontKnowLot, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 572, 0
    // "Chulululuwa!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_Chulululuwa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01d8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09A2
    // "Hey!\nDo you have any Flying- or[f000]븀\u0000\nPsychic-type Pokémon with you?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HeyHaveAnyFlying, 0, 0
    VMCall L_09BC
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_098E
    // "Oh!\n[f000]ā\u0001\u0000 is with you![f000]븁\u0000\nWould you help me get some luggage\nthat's too high for me to reach?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_OhWouldHelpGet, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 4
    FadeExWait
    VMSleep 45
    FadeEx 3, 16, 0, 4
    FadeExWait
    // "Here's a little something to\nthank you for helping me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HeresLittleSomethingThank, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 6
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Having Pokémon with you\ncan be a big help sometimes, eh?"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HavingPokemonCanBig, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01d8
    VMJump L_099C

L_098E:
    // "That's too bad.[f000]븁\u0000\nI need a Flying- or Psychic-type\nPokémon to help me out."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_ThatsTooBadNeed, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_099C:
    VMJump L_09B6

L_09A2:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Having Pokémon with you\ncan be a big help sometimes, eh?"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity3_Text_HavingPokemonCanBig, 0, 0
    LastKeyWait
    ActorMsgClose

L_09B6:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_09BC:
    WorkSetConst 0x8023, 0
    PokePartyGetCount 0x8022, 0

L_09C8:
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0A5C
    PokePartyGetTypes 0x8024, 0x8025, 0x8023
    PokePartyIsEgg 0x8028, 0x8023
    VMStackPush 0x8024
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0A50
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A50
    PokePartyGetSpecies 0x8026, 0x8023
    WordSetPokeSpecies 0, 0x8026
    WorkSetConst 0x8027, 1

L_0A50:
    WorkAdd 0x8023, 1
    VMJump L_09C8

L_0A5C:
    VMReturn
    .balign 4, 0
