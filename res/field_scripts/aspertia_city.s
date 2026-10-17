#include "asm/field_script.inc"
#include "text/script/aspertia_city.h"

// Script plugin 14, from the zones that use this file

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
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntry Script_25
    ScriptEntry Script_26
    ScriptEntry Script_27
    ScriptEntry Script_28
    ScriptEntry Script_29
    ScriptEntry Script_30
    ScriptEntry Script_31
    ScriptEntry Script_32
    ScriptEntry Script_33
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_5:
    VMHalt

Script_6:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D1
    ActorSetGPos 3, 42, 1, 751, 3
    ActorSetGPos 0, 43, 1, 751, 2
    VMJump L_0121

L_00D1:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EA
    VMJump L_0121

L_00EA:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0121
    ActorSetGPos 2, 49, 1, 740, 2
    ActorSetGPos 1, 38, 1, 740, 3
    ActorSetGPos 3, 38, 1, 741, 3

L_0121:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0150
    ActorSetGPos 6, 36, 1, 741, 3

L_0150:
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016F
    ActorSetGPos 2, 42, 1, 741, 2

L_016F:
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018E
    ActorSetGPos 2, 40, 1, 741, 1

L_018E:
    VMStackPush EVENT_WORK_0x4115
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B9
    ActorSetGPos 0, 45, 1, 762, 3
    ActorSetGPos 3, 45, 1, 763, 3

L_01B9:
    VMHalt

Script_22:
    ActorsPauseAll
    ActorCmdExec 254, Movement_1F14
    ActorCmdWait
    ActorCmdExec 254, Movement_1EB4
    ActorCmdExec 255, Movement_1EDC
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Oh, I get it![f000]븁\u0000\nThe outlook is Aspertia's\nmost famous spot![f000]븁\u0000\nI'll bet Bianca is up there\nlooking at the scenery![f000]븁\u0000\nC'mon!\nGo get your Pokémon already![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OhGetOutlookAspertias, 254, 0, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x02e9
    ActorAdd 0
    PlayerGetGPos 0x8021, 0x8022
    ActorSetGPos 0, 0x8021, 7, 720, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 1
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorDelete 254
    VMStackPush 0x8021
    VMStackPushConst 36
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0255
    ActorWalkRoute 0, 36, 720, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 0, Movement_1ED4
    ActorCmdWait

L_0255:
    WorkSetConst EVENT_WORK_0x40a4, 1
    WorkSetConst EVENT_WORK_0x40a1, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_029B
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Listen, there's no way\nI got this wrong!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_ListenTheresNoWay, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02D1

L_029B:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C7
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: See!\nBianca was here, right?[f000]븁\u0000\nNow, c'mon!\nGo and get your Pokémon!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SeeBiancaHereRight, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02D1

L_02C7:
    BGMPlay SEQ_BGM_E_HUE
    VMCall L_0B1A

L_02D1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_02F8
    VMCall L_03EB
    VMJump L_03E5

L_02F8:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0317
    VMCall L_0538
    VMJump L_03E5

L_0317:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0346
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Still, that Professor Juniper![f000]븁\u0000\nThe normal thing to do is to\nget an OK before sending[f000]븀\u0000\nsomeone clear out here, right?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_StillProfessorJuniperNormal, 2, 1, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03E5

L_0346:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0373
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca: Ooh, I thought of something cool![f000]븁\u0000\nYou both have Pokémon, right?\nWhy don't you have a Pokémon battle?"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaOohThoughtSomething, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_03E5

L_0373:
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D1
    GameCommCheckDSiWiFi 0x8008
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Check this! The C-Gear was activated,\nand that screen showed up![f000]븁\u0000\nIf you touch the “?\" icon in the\nbottom-right corner of the[f000]븀\u0000\nC-Gear screen, you can read about[f000]븀\u0000\nthe C-Gear.[f000]븁\u0000\nLike, what are you going to do now?\nYou know, there's another Pokémon Gym[f000]븀\u0000\nin Virbank City, which is just past[f000]븀\u0000\nFloccesy Town."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_CheckCGearActivated, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_03CB

L_03B7:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you want to turn on the C-Gear, touch\nthe Power symbol at the bottom right of[f000]븀\u0000\nthe C-Gear screen.[f000]븁\u0000\nThen, after turning on the power,\nif you touch the “?\" icon in the[f000]븀\u0000\nbottom-right corner of the[f000]븀\u0000\nC-Gear screen, you can read about[f000]븀\u0000\nthe C-Gear.[f000]븁\u0000\nLike, what are you going to do now?\nYou know, there's another Pokémon Gym[f000]븀\u0000\nin Virbank City, which is just past[f000]븀\u0000\nFloccesy Town."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_IfWantTurnC, 0, 0
    LastKeyWait
    ActorMsgClose

L_03CB:
    VMJump L_03E5

L_03D1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca: Well, OK![f000]븁\u0000\nI don't really get it, but going\non a journey is always good![f000]븁\u0000\nAnyway, I just happen to have\nanother Pokédex on me![f000]븁\u0000\nIt looks like Pokémon distribution has\nreally changed compared to two years[f000]븀\u0000\nago, so the more, the merrier![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaWellOkDont, 0, 0
    LastKeyWait
    ActorMsgClose

L_03E5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_03EB:
    SEPlay SEQ_SE_MESSAGE
    BGMPlay SEQ_BGM_E_BERU
    // "???: It's sooo pretty![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_ItsSoooPretty, 2, 1, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_0418
    VMJump L_0426

L_0418:
    ActorCmdExec 2, Movement_1EDC
    VMJump L_0468

L_0426:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0439
    VMJump L_0447

L_0439:
    ActorCmdExec 2, Movement_1EEC
    VMJump L_0468

L_0447:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_045A
    VMJump L_0468

L_045A:
    ActorCmdExec 2, Movement_1EE4
    VMJump L_0468

L_0468:
    ActorCmdWait
    // "Hey there!\nDon't you agree?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_HeyThereDontAgree, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1F14
    ActorCmdWait
    WordSetPlayerName 0
    // "Oh!\nMy name is Bianca![f000]븁\u0000\nI'm the assistant of the\nPokémon Professor--Professor Juniper.[f000]븁\u0000\nBy the way, I'm looking for someone.\nDo you know a person named [f000]Ā\u0001\u0000?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OhNameBiancaIm, 2, 1, 0
    YesNoWin 0x8010
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1F14
    ActorCmdWait
    // "Oh, wait!\nYou're [f000]Ā\u0001\u0000![f000]븁\u0000\nWooow! You're ex-act-ly like\nwhat I heard![f000]븁\u0000"
    // "Oh, wait!\nYou're [f000]Ā\u0001\u0000![f000]븁\u0000\nWooow! You're ex-act-ly like\nwhat I heard![f000]븁\u0000"
    ActorMsgGendered 1024, AspertiaCity_Text_OhWaitYoureWooow, AspertiaCity_Text_OhWaitYoureWooow_2, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0528
    ActorCmdWait
    // "Bianca: Nice to meet you![f000]븁\u0000\nI have a really important request\nto ask you![f000]븁\u0000\nWill you help us complete\nthe Pokédex?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaNiceMeetHave, 2, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050D
    WorkSetConst 0x8023, 1

L_04E4:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050D
    // "N-no way...[f000]븁\u0000\nI-I must have misheard you.\nRight?[f000]븁\u0000\nThis is a very important request.[f000]븁\u0000\nWill you please help us complete\nthe Pokédex?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_NNoWayMust, 2, 1, 0
    YesNoWin 0x8023
    VMJump L_04E4

L_050D:
    // "Oh, wow, thanks![f000]븁\u0000\nYour support will help Professor\nJuniper's research move forward![f000]븁\u0000\nAnyway, filling up the\nPokédex is totally fun!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OhWowThanksSupport, 2, 1, 0
    LastKeyWait
    MsgWinCloseAll
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40a1, 3
    VMReturn
    .balign 4, 0

Movement_0528:
    Move 100, 1
    MoveEnd
    VMStackAdd
    VMHalt
    .byte 0xfe
    .byte 0x00
    VMNop

L_0538:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca: OK, then![f000]븁\u0000\nTa-daaa![f000]븁\u0000\nIn here is the Pokémon\nthat will be your partner![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaOkThenTa, 2, 1, 0
    MsgWinCloseAll
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    FadeOutBlackQ
    FadeWait
    FieldClose
    callPoke3Select 0x8024
    FieldOpen
    FadeInWhiteQ
    FadeWait
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_059D
    WorkSetConst EVENT_WORK_0x4030, 0
    Cmd_0209 8, 3
    WorkSetConst 0x8025, 495
    WordSetPokeSpecies 1, 495
    WordSetPokeSpecies 2, 495
    VMJump L_05EE

L_059D:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05D2
    WorkSetConst EVENT_WORK_0x4030, 1
    Cmd_0209 8, 1
    WorkSetConst 0x8025, 498
    WordSetPokeSpecies 1, 498
    WordSetPokeSpecies 2, 498
    VMJump L_05EE

L_05D2:
    WorkSetConst EVENT_WORK_0x4030, 2
    Cmd_0209 8, 2
    WorkSetConst 0x8025, 501
    WordSetPokeSpecies 1, 501
    WordSetPokeSpecies 2, 501

L_05EE:
    MEPlay SEQ_ME_POKEGET
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 chose [f000]ā\u0001\u0001!"
    SystemMsg AspertiaCity_Text_Chose, 1
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    PokePartyAdd 0x8010, 0x8025, 0, 5
    FlagSet EVENT_FLAG_0x0961
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    // "Bianca: Oh, wow! You and [f000]ā\u0001\u0002\nare a perfect match![f000]븁\u0000\nBy the way, would you like to give\na nickname to the Pokémon you chose?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaOhWowPerfect, 2, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_064C
    MsgWinCloseAll
    VMCall L_06C6
    VMJump L_0658

L_064C:
    // "Bianca: Oh, OK, gotcha.\nYou're not going to give it a nickname.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaOhOkGotcha, 2, 1, 0

L_0658:
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0528
    ActorCmdWait
    // "Bianca: Now you've got your Pokémon,\nso I'll give you this, too--a Pokédex![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaNowYouveGot, 2, 1, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0962
    MEPlay SEQ_ME_KEYITEM
    TrainerCardGetSex 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_069B
    PlayFieldEffect 63
    VMJump L_069F

L_069B:
    PlayFieldEffect 64

L_069F:
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received\nthe [f000][ff00]\u0001\u0002Pokédex[f000][ff00]\u0001\u0000!"
    SystemMsg AspertiaCity_Text_ReceivedPokedex, 1
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    // "Bianca: You want to know what it does?[f000]븁\u0000\nThe Pokédex is a high-tech device\nthat automatically records the[f000]븀\u0000\nPokémon you encounter![f000]븁\u0000\nSo Professor Juniper wants you to carry\nthis Pokédex, visit a lot of places, and[f000]븀\u0000\nmeet all the Pokémon in the Unova region!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaWantKnowWhat, 2, 1, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40a1, 4
    VMReturn

L_06C6:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0

L_06D2:
    VMStackPush 0x8026
    VMStackPushConst 555
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0758
    FadeOutBlackQ
    FadeWait
    CallPokeNameInput 0x8010, 0, 0
    FadeInBlackQ
    FadeWait
    PokePartyGetParam 0x8027, 0, 117
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_074C
    WordSetPartyPokeName 0, 0
    // "Bianca: [f000]Ă\u0001\u0000!\nIs that the nickname you want?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaNicknameWant, 2, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0744
    WorkSetConst 0x8026, 555
    VMJump L_0746

L_0744:
    MsgWinCloseAll

L_0746:
    VMJump L_0752

L_074C:
    WorkSetConst 0x8026, 555

L_0752:
    VMJump L_06D2

L_0758:
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0782
    WordSetPartyPokeName 0, 0
    // "Bianca: [f000]Ă\u0001\u0000!\nThat is such a great name![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaSuchGreatName, 2, 1, 0
    VMJump L_078E

L_0782:
    // "Bianca: Oh, OK, gotcha.\nYou're not going to give it a nickname.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaOhOkGotcha, 2, 1, 0

L_078E:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    VMReturn

Script_24:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Cheren: Bianca makes a good point.[f000]븁\u0000\nI'll tell you what I know about Pokémon\nAbilities and Pokémon type matchups.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_CherenBiancaMakesGood, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D8
    WordSetPokeSpecies 2, 495
    VMJump L_07FB

L_07D8:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07F6
    WordSetPokeSpecies 2, 498
    VMJump L_07FB

L_07F6:
    WordSetPokeSpecies 2, 501

L_07FB:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Mom: Bon voyage![f000]븁\u0000\nTake [f000]ā\u0001\u0002 and go see\nmany different Pokémon and[f000]븀\u0000\npeople with your own eyes!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_MomBonVoyageTake, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001's Sister: Get along\nwith your Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SSisterGetAlong, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    ActorCmdExec 0, Movement_1F14
    ActorCmdWait
    ActorCmdExec 0, Movement_1EEC
    VMSleep 8
    ActorCmdExec 255, Movement_1EE4
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Listen, there's no way\nI got this wrong![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_ListenTheresNoWay_2, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_1E9C
    ActorCmdWait
    ActorCmdExec 0, Movement_1ED4
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 0, Movement_1F14
    ActorCmdWait
    ActorCmdExec 0, Movement_1EEC
    VMSleep 8
    ActorCmdExec 255, Movement_1EE4
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: See!\nBianca was here, right?[f000]븁\u0000\nNow, c'mon!\nGo and get your Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SeeBiancaHereRight_2, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_1E9C
    ActorCmdWait
    ActorCmdExec 0, Movement_1ED4
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_33:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 0, 0x8021, 716, 0, 8, 0
    ActorCmdWait
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0907
    ActorCmdExec 255, Movement_1EDC
    ActorCmdWait

L_0907:
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Heeey! How long are you\nplanning on keeping me waiting, anyway?[f000]븁\u0000\nHey! What's that?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_HeeeyHowLongPlanning, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1F14
    ActorCmdWait
    ActorCmdExec 0, Movement_1E9C
    ActorCmdWait
    // "So that's your partner, huh?[f000]븁\u0000\nThat's great![f000]븁\u0000\nMy sister already said so, but\ntake really, really good care[f000]븀\u0000\nof your Pokémon! Got it?[f000]븁\u0000\nWhat's that you're holding there?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_ThatsPartnerHuhThats, 0, 0, 0
    MsgWinCloseAll
    WorkCmpConst 0x8021, 36
    VMJumpIf CMP_EQ, L_094D
    VMJump L_0959

L_094D:
    WorkSetConst 0x8021, 37
    VMJump L_0997

L_0959:
    WorkCmpConst 0x8021, 37
    VMJumpIf CMP_EQ, L_096C
    VMJump L_0978

L_096C:
    WorkSetConst 0x8021, 36
    VMJump L_0997

L_0978:
    WorkCmpConst 0x8021, 38
    VMJumpIf CMP_EQ, L_098B
    VMJump L_0997

L_098B:
    WorkSetConst 0x8021, 37
    VMJump L_0997

L_0997:
    ActorWalkRoute 2, 0x8021, 713, 0, 8, 0
    ActorCmdWait
    // "Bianca: It's a Pokédex![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaItsPokedex, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1EDC
    ActorCmdWait
    ActorCmdExec 0, Movement_1F24
    ActorCmdWait
    VMSleep 30
    ActorWalkRoute 0, 0x8021, 715, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_1ED4
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8021, 36
    VMJumpIf CMP_EQ, L_0A00
    VMJump L_0A0E

L_0A00:
    ActorCmdExec 255, Movement_1EEC
    VMJump L_0A50

L_0A0E:
    WorkCmpConst 0x8021, 37
    VMJumpIf CMP_EQ, L_0A21
    VMJump L_0A2F

L_0A21:
    ActorCmdExec 255, Movement_1EE4
    VMJump L_0A50

L_0A2F:
    WorkCmpConst 0x8021, 38
    VMJumpIf CMP_EQ, L_0A42
    VMJump L_0A50

L_0A42:
    ActorCmdExec 255, Movement_1EE4
    VMJump L_0A50

L_0A50:
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Please give me\na Pokédex, too![f000]븁\u0000\nI want to get stronger![f000]븁\u0000\nIf I have a Pokédex,\nI can learn more about Pokémon...[f000]븀\u0000\nThat'll make me tougher, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_PleaseGivePokedexToo, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1F1C
    ActorCmdWait
    // "Bianca: Um...who are you again?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaUmWhoAgain, 2, 0, 0
    MsgWinCloseAll
    WordSetLoadRivalName 1
    // "I'm [f000]Ā\u0001\u0001![f000]븁\u0000\nI'm going to travel the Unova region\nwith my Pokémon partner in order[f000]븀\u0000\nto search for something very important![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_ImImGoingTravel, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1EE4
    ActorCmdWait
    ActorCmdExec 2, Movement_1F24
    ActorCmdWait
    VMSleep 20
    ActorCmdExec 2, Movement_1EDC
    ActorCmdWait
    // "Bianca: Well, OK![f000]븁\u0000\nI don't really get it, but going\non a journey is always good![f000]븁\u0000\nAnyway, I just happen to have\nanother Pokédex on me![f000]븁\u0000\nIt looks like Pokémon distribution has\nreally changed compared to two years[f000]븀\u0000\nago, so the more, the merrier![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaWellOkDont, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1E94
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40a1, 5
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    ActorCmdExec 0, Movement_1F14
    ActorCmdWait
    BGMPlay SEQ_BGM_E_HUE
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 0, 0x8021, 715, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_1EDC
    VMSleep 8
    ActorCmdExec 255, Movement_1ED4
    ActorCmdWait
    VMCall L_0B1A
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0B1A:
    WordSetLoadRivalName 1
    WorkCmpConst EVENT_WORK_0x4030, 0
    VMJumpIf CMP_EQ, L_0B30
    VMJump L_0B3B

L_0B30:
    WordSetPokeSpecies 3, 498
    VMJump L_0B77

L_0B3B:
    WorkCmpConst EVENT_WORK_0x4030, 1
    VMJumpIf CMP_EQ, L_0B4E
    VMJump L_0B59

L_0B4E:
    WordSetPokeSpecies 3, 501
    VMJump L_0B77

L_0B59:
    WorkCmpConst EVENT_WORK_0x4030, 2
    VMJumpIf CMP_EQ, L_0B6C
    VMJump L_0B77

L_0B6C:
    WordSetPokeSpecies 3, 495
    VMJump L_0B77

L_0B77:
    ActorCmdExec 2, Movement_1EBC
    ActorCmdExec 0, Movement_1F34
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Let's see how good\na Trainer you are![f000]븁\u0000\nI'll use my [f000]ā\u0001\u0003\nthat I raised from an Egg![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_LetsSeeHowGood, 0, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BB8
    CallTrainerBattle TRAINER_RIVAL, 0, 1
    VMJump L_0BE1

L_0BB8:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BD9
    CallTrainerBattle TRAINER_RIVAL_2, 0, 1
    VMJump L_0BE1

L_0BD9:
    CallTrainerBattle TRAINER_RIVAL_3, 0, 1

L_0BE1:
    CallTrainerBattleEnd
    WordSetLoadRivalName 1
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C0F
    // "[f000]Ā\u0001\u0001: I lost...\nThis is different than battling[f000]븀\u0000\nwith wild Pokémon![f000]븁\u0000\nWell, whatever.\nI'm just happy to know you're[f000]븀\u0000\na Trainer I can count on![f000]븁\u0000\nCool. I'm heading off first!\nGet stronger![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_LostDifferentThanBattling, 0, 0, 0
    VMJump L_0C1D

L_0C0F:
    PokePartyRecoverAll
    // "[f000]Ā\u0001\u0001: That was good enough\nfor your first battle![f000]븁\u0000\nCool. I'm heading off first!\nGet stronger![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_GoodEnoughFirstBattle, 0, 0, 0

L_0C1D:
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 37
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 715
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0C5C
    ActorWalkRoute 0, 37, 724, 2, 8, 1
    VMJump L_0C6A

L_0C5C:
    ActorWalkRoute 0, 37, 724, 2, 8, 0

L_0C6A:
    FlagSet EVENT_FLAG_0x02e9
    VMSleep 20
    ActorCmdExec 255, Movement_1EDC
    BGMChangeMap
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8021, 36
    VMJumpIf CMP_EQ, L_0C97
    VMJump L_0CA3

L_0C97:
    WorkAdd 0x8021, 1
    VMJump L_0CE1

L_0CA3:
    WorkCmpConst 0x8021, 37
    VMJumpIf CMP_EQ, L_0CB6
    VMJump L_0CC2

L_0CB6:
    WorkSub 0x8021, 1
    VMJump L_0CE1

L_0CC2:
    WorkCmpConst 0x8021, 38
    VMJumpIf CMP_EQ, L_0CD5
    VMJump L_0CE1

L_0CD5:
    WorkSub 0x8021, 1
    VMJump L_0CE1

L_0CE1:
    ActorWalkRoute 2, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    // "Bianca: The Pokémon on both\nsides did their best![f000]븁\u0000\nBut this little one is still weak,\nso battle with it and make it stronger![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaPokemonBothSides, 2, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8021, 36
    VMJumpIf CMP_EQ, L_0D18
    VMJump L_0D32

L_0D18:
    ActorCmdExec 2, Movement_1EE4
    VMSleep 8
    ActorCmdExec 255, Movement_1EEC
    VMJump L_0D8C

L_0D32:
    WorkCmpConst 0x8021, 37
    VMJumpIf CMP_EQ, L_0D45
    VMJump L_0D5F

L_0D45:
    ActorCmdExec 2, Movement_1EEC
    VMSleep 8
    ActorCmdExec 255, Movement_1EE4
    VMJump L_0D8C

L_0D5F:
    WorkCmpConst 0x8021, 38
    VMJumpIf CMP_EQ, L_0D72
    VMJump L_0D8C

L_0D72:
    ActorCmdExec 2, Movement_1EEC
    VMSleep 8
    ActorCmdExec 255, Movement_1EE4
    VMJump L_0D8C

L_0D8C:
    ActorCmdWait
    // "All righty, let's go make your Pokémon\nbetter at the Pokémon Center![f000]븁\u0000\nIt's like the best place ever for\nPokémon who battle and get hurt![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_AllRightyLetsGo, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0E90
    VMSleep 6
    ActorCmdExec 255, Movement_0E90
    VMSleep 10
    FadeEx 3, 0, 16, 4
    FadeExWait
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x308000, 0x1000f, 0x2e48000, 1
    EvCameraWait
    ActorSetGPos 2, 47, 1, 741, 0
    ActorSetGPos 255, 49, 1, 741, 0
    VMSleep 60
    FadeEx 3, 16, 0, 4
    FadeExWait
    // "Bianca: The Pokémon Center is the\nsame no matter where you are![f000]븁\u0000\nLet's go inside![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaPokemonCenterSame, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0E70
    ActorCmdWait
    WorkSetConst 0x8028, 0
    BMCreateHandleByGPos 0x8028, 1, 48, 739
    BMHndAudioVisualAnmPlay 0x8028, 0
    BMHndAnmWait 0x8028
    ActorCmdExec 2, Movement_0E7C
    ActorCmdWait
    ActorDelete 2
    ActorCmdExec 255, Movement_0E84
    ActorCmdWait
    RTReserveScript 3
    MapChangeWarp ZONE_ASPERTIA_CITY_POKEMON_CENTER, 7, 19, 0
    EvCameraRebind
    EvCameraEnd
    FlagSet EVENT_FLAG_0x02e9
    WorkSetConst 0x8028, 0
    VMReturn

Movement_0E70:
    Move 15, 1
    Move 12, 1
    MoveEnd

Movement_0E7C:
    Move 12, 1
    MoveEnd

Movement_0E84:
    Move 14, 1
    Move 12, 2
    MoveEnd

Movement_0E90:
    Move 168, 6
    MoveEnd

Script_4:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EB8
    WordSetPokeSpecies 2, 495
    VMJump L_0EDB

L_0EB8:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ED6
    WordSetPokeSpecies 2, 498
    VMJump L_0EDB

L_0ED6:
    WordSetPokeSpecies 2, 501

L_0EDB:
    ActorCmdExec 2, Movement_1F14
    ActorCmdWait
    ActorCmdExec 255, Movement_1EE4
    ActorCmdWait
    ActorWalkRoute 1, 46, 740, 1, 8, 0
    VMSleep 4
    ActorWalkRoute 3, 46, 741, 1, 8, 0
    ActorCmdWait
    WordSetPlayerName 0
    // "Mom: Oh! Nice to meet you!\nYou must be Bianca, right?[f000]븁\u0000\nAnd [f000]Ā\u0001\u0000 picked\n[f000]ā\u0001\u0002, then![f000]븁\u0000\nHee hee.\nLooking good![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_MomOhNiceMeet, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_1F14
    ActorCmdWait
    // "Oh!\nI almost forgot![f000]븁\u0000\nHere! Take these!\nThey're Running Shoes![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OhAlmostForgotHere, 1, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 1, 47, 740, 1, 8, 0
    ActorCmdWait
    GiveRunningShoes
    MEPlay SEQ_ME_KEYITEM
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received\na pair of [f000][ff00]\u0001\u0002Running Shoes[f000][ff00]\u0001\u0000!"
    SystemMsg AspertiaCity_Text_ReceivedPairRunningShoes, 0
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    // "Mom: A perfect fit!\nI'll read the instructions to you![f000]븁\u0000\n“Hold the B Button to run faster than\nnormal. Put on the Running Shoes and[f000]븀\u0000\nrace around to your heart's content!\"[f000]븁\u0000\nNow, you and [f000]ā\u0001\u0002 can\nrun anywhere you want![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_MomPerfectFitIll, 1, 1, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 48, 741, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 3, Movement_1ED4
    VMSleep 8
    ActorCmdExec 255, Movement_1EDC
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001's Sister: Um...\nThis is from me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SSisterUmFrom, 3, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 442
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 2, Movement_1EDC
    ActorCmdWait
    ActorCmdExec 2, Movement_1F1C
    ActorCmdWait
    // "Bianca: Why are there\ntwo Town Maps?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaWhyThereTwo, 2, 0, 0
    MsgWinCloseAll
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001's Sister: I want you to\ngive the other one to my big brother![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SSisterWantGive, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1EE4
    ActorCmdWait
    // "Mom: That's a good idea! Even if it is\na single road to the ocean, having a[f000]븀\u0000\nTown Map is always nice.[f000]븁\u0000\nI mean, if you use a Town Map,\nyou'll know all about what the[f000]븀\u0000\nUnova region is like![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_MomThatsGoodIdea, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_1EEC
    ActorCmdWait
    // "Bianca: Aww, you guys!\nJust watching this makes me happy![f000]븁\u0000\nC'mon, we're headed for Route 19!\nI'll teach you how to catch a Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaAwwGuysJust, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 53, 725, 1, 8, 0
    VMSleep 20
    ActorCmdExec 3, Movement_1EEC
    ActorCmdWait
    ActorDelete 2
    ActorCmdExec 255, Movement_1EE4
    ActorCmdExec 3, Movement_1ED4
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1077
    WordSetPokeSpecies 2, 495
    VMJump L_109A

L_1077:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1095
    WordSetPokeSpecies 2, 498
    VMJump L_109A

L_1095:
    WordSetPokeSpecies 2, 501

L_109A:
    // "Mom: Bon voyage![f000]븁\u0000\nTake [f000]ā\u0001\u0002 and go see\nmany different Pokémon and[f000]븀\u0000\npeople with your own eyes!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_MomBonVoyageTake, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40a1, 7
    FlagSet EVENT_FLAG_0x02e6
    FlagSet EVENT_FLAG_0x02e4
    FlagSet EVENT_FLAG_0x0323
    HollowRivalCmd_0262 4, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    // "Bianca: Heeey![f000]븁\u0000"
    // "Bianca: Yoo-hoo![f000]븁\u0000"
    ActorMsgGendered 1024, AspertiaCity_Text_BiancaHeeey, AspertiaCity_Text_BiancaYooHoo, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1EAC
    ActorCmdExec 255, Movement_1524
    ActorCmdWait
    // "How was it?\nHow did your Pokémon battle with[f000]븀\u0000\nthe Gym Leader go?[f000]븁\u0000\nOh! If it isn't the Basic Badge![f000]븁\u0000\nWow! Amazing! And you just set off\non your journey with your Pokémon![f000]븁\u0000\nYou definitely have potential\nas a Trainer! I'm sure of it![f000]븁\u0000\nThis is from me!\nIt's the TM for the move Return.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_HowHowDidPokemon, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1EAC
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 354
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "When a Pokémon knows Return,\nthe more it gets along with the Trainer,[f000]븀\u0000\nthe more powerful the move is![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_WhenPokemonKnowsReturn, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1530
    ActorCmdWait
    // "Still, that Cheren...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_StillCheren, 2, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x278000, 0x1000f, 0x2e2b000, 24
    SEPlay SEQ_SE_KAIDAN
    FlagReset EVENT_FLAG_0x02e5
    ActorAdd 7
    SEWait
    EvCameraWait
    // "Cheren: Bianca![f000]븁\u0000\nIt's been two years, hasn't it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_CherenBiancaItsBeen, 7, 3, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1540
    ActorCmdExec 255, Movement_1ED4
    ActorCmdWait
    // "Bianca: Oh wow![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaOhWow, 2, 5, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1ED4
    ActorCmdWait
    // "Wh-what's up?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_WhWhatsUp, 2, 5, 0
    MsgWinCloseAll
    // "Cheren: I thought it would be a\ngood idea to register each other[f000]븀\u0000\nin the Xtransceiver![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_CherenThoughtWouldGood, 7, 3, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 24
    ActorWalkRoute 7, 39, 740, 1, 8, 0
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 7, Movement_1EDC
    VMSleep 8
    ActorCmdExec 255, Movement_1ED4
    ActorCmdWait
    SEPlay SEQ_SE_SW_LC_NO
    SEWait
    // "Now, you can communicate\nwith me from your Xtransceiver.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_NowCanCommunicateFrom, 7, 0, 0
    MsgWinCloseAll
    // "Bianca: M-me, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaMToo, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1EE4
    VMSleep 8
    ActorCmdExec 255, Movement_1EEC
    ActorCmdWait
    SEPlay SEQ_SE_SW_LC_NO
    SEWait
    // "I registered Professor Juniper\nfor you, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_RegisteredProfessorJuniperToo, 2, 0, 0
    MsgWinCloseAll
    HollowRivalCmd_0263 2
    HollowRivalCmd_0263 3
    HollowRivalCmd_0263 0
    MEPlay SEQ_ME_CALL
    // "The Xtransceiver is ringing!"
    SystemMsg AspertiaCity_Text_XtransceiverRinging, 2
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 picked up the\nXtransceiver![f000]븁\u0000"
    SystemMsg AspertiaCity_Text_PickedUpXtransceiver, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    CallXTransceiver 6, 0
    FadeInBlackQ
    FadeWait
    ActorCmdExec 2, Movement_1F2C
    ActorCmdWait
    // "Bianca: Hey, [f000]Ā\u0001\u0000!\nIsn't Professor Juniper cool?[f000]븁\u0000\nIf you talk to her on the Xtransceiver,\nshe'll evaluate the completeness[f000]븀\u0000\nof your Pokédex or tell you a lot about[f000]븀\u0000\nhow Pokémon evolve![f000]븁\u0000\nAnd you can call us, too, of course![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaHeyIsntProfessor, 2, 0, 0
    // "I'll tell you how well\nyou and your Pokémon[f000]븀\u0000\nare getting along, OK?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_IllTellHowWell, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 7, Movement_1EDC
    VMSleep 8
    ActorCmdExec 255, Movement_1ED4
    ActorCmdWait
    // "Cheren: Bianca makes a good point.[f000]븁\u0000\nI'll tell you what I know about Pokémon\nAbilities and Pokémon type matchups.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_CherenBiancaMakesGood, 7, 0, 0
    MsgWinCloseAll
    ActorNew 53, 740, 1, 251, 291, 0
    ActorWalkRoute 251, 41, 740, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 251, Movement_1F14
    VMSleep 8
    ActorCmdExec 7, Movement_1EEC
    VMSleep 8
    ActorCmdExec 2, Movement_1EB4
    ActorCmdExec 255, Movement_1ECC
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Are you the Gym Leader?\nOne, two, three--let's battle![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_GymLeaderOneTwo, 251, 0, 0
    MsgWinCloseAll
    // "Cheren: You look like a tough Trainer.[f000]븁\u0000\nUnderstood.\nPlease come into my Pokémon Gym![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_CherenLookLikeTough, 7, 0, 0
    MsgWinCloseAll
    ActorCmdExec 7, Movement_1548
    VMSleep 8
    ActorCmdExec 255, Movement_1EB4
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 7
    SEWait
    ActorCmdExec 251, Movement_1550
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: What was that weak answer?!\nI'm definitely going to take you down![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_WhatWeakAnswerIm, 251, 0, 1
    ActorMsgClose
    ActorWalkRoute 251, 39, 738, 4, 4, 0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    // "Bianca: Being a Gym Leader\nis even harder than I imagined.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_BiancaBeingGymLeader, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1EE4
    VMSleep 8
    ActorCmdExec 255, Movement_1EEC
    ActorCmdWait
    // "The next thing is to make it so you\ncan use the C-Gear.[f000]븁\u0000\nThe C-Gear is a cool device for\ncommunications, such as Infrared[f000]븀\u0000\nConnection or Nintendo Wi-Fi Connection.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_NextThingMakeCan, 2, 0, 0
    MsgWinCloseAll
    VMCall L_1406
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40a8, 3
    WorkSetConst EVENT_WORK_0x40ab, 1
    WorkSetConst EVENT_WORK_0x4153, 1
    FlagSet EVENT_FLAG_0x02e5
    FlagSet EVENT_FLAG_0x006a
    FlagReset EVENT_FLAG_0x0408
    HollowRivalCmd_0262 3, 1
    TrainerCardCmd_00E7 1
    TrainerCardCmd_00E7 2
    MedalDiscover 73
    MedalDiscover 174
    MedalDiscover 179
    MedalDiscover 180
    MedalDiscover 195
    MedalDiscover 199
    MedalDiscover 232
    MedalDiscover 233
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_1406:
    MEPlay SEQ_ME_KEYITEM
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 obtained\nthe [f000][ff00]\u0001\u0001C-Gear[f000][ff00]\u0001\u0000!"
    SystemMsg AspertiaCity_Text_ObtainedCGear, 2
    MEWait
    MsgWaitAdvance
    CGearControlWarning 1
    WorkSetConst 0x8029, 0

L_1421:
    VMStackPush 0x8029
    VMStackPushConst 555
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_151B
    // "Turn on the C-Gear and\nestablish communications?"
    SystemMsg AspertiaCity_Text_TurnCGearEstablish, 2
    ListMenu_AnchorTopRight 31, 13, 0, 1, 32784
    ListMenuAdd 102, 65535, 0
    ListMenuAdd 103, 65535, 1
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14C5
    CGearControlWarning 0
    GameCommCheckDSiWiFi 0x8008
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14A1
    MsgWinCloseAll
    SEPlay SEQ_SE_DECIDE3
    CGearPowerOn 1
    SEWait
    // "Check this! The C-Gear was activated,\nand that screen showed up![f000]븁\u0000\nIf you touch the “?\" icon in the\nbottom-right corner of the[f000]븀\u0000\nC-Gear screen, you can read about[f000]븀\u0000\nthe C-Gear.[f000]븁\u0000\nLike, what are you going to do now?\nYou know, there's another Pokémon Gym[f000]븀\u0000\nin Virbank City, which is just past[f000]븀\u0000\nFloccesy Town."
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_CheckCGearActivated, 2, 0, 0
    VMJump L_14B9

L_14A1:
    // "Wireless communications\nare turned OFF.[f000]븁\u0000\nTurn wireless communications\nON in the System Settings.[f000]븀\u0000\nError code: 50699[f000]븁\u0000"
    SystemMsg AspertiaCity_Text_WirelessCommunicationsTurnedOff, 2
    MsgWinCloseAll
    CGearPowerOn 0
    // "If you want to turn on the C-Gear, touch\nthe Power symbol at the bottom right of[f000]븀\u0000\nthe C-Gear screen.[f000]븁\u0000\nThen, after turning on the power,\nif you touch the “?\" icon in the[f000]븀\u0000\nbottom-right corner of the[f000]븀\u0000\nC-Gear screen, you can read about[f000]븀\u0000\nthe C-Gear.[f000]븁\u0000\nLike, what are you going to do now?\nYou know, there's another Pokémon Gym[f000]븀\u0000\nin Virbank City, which is just past[f000]븀\u0000\nFloccesy Town."
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_IfWantTurnC, 2, 0, 0

L_14B9:
    WorkSetConst 0x8029, 555
    VMJump L_1515

L_14C5:
    // "Some functions of the C-Gear\nwill be restricted. Is that OK?"
    SystemMsg AspertiaCity_Text_SomeFunctionsCGear, 2
    ListMenu_AnchorTopRight 31, 13, 0, 1, 32784
    ListMenuAdd 102, 65535, 0
    ListMenuAdd 103, 65535, 1
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1515
    MsgWinCloseAll
    CGearControlWarning 0
    CGearPowerOn 0
    // "If you want to turn on the C-Gear, touch\nthe Power symbol at the bottom right of[f000]븀\u0000\nthe C-Gear screen.[f000]븁\u0000\nThen, after turning on the power,\nif you touch the “?\" icon in the[f000]븀\u0000\nbottom-right corner of the[f000]븀\u0000\nC-Gear screen, you can read about[f000]븀\u0000\nthe C-Gear.[f000]븁\u0000\nLike, what are you going to do now?\nYou know, there's another Pokémon Gym[f000]븀\u0000\nin Virbank City, which is just past[f000]븀\u0000\nFloccesy Town."
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_IfWantTurnC, 2, 0, 0
    WorkSetConst 0x8029, 555

L_1515:
    VMJump L_1421

L_151B:
    WorkSetConst 0x8029, 0
    VMReturn
    .balign 4, 0

Movement_1524:
    Move 13, 2
    Move 35, 1
    MoveEnd

Movement_1530:
    Move 32, 1
    Move 63, 3
    Move 34, 1
    MoveEnd

Movement_1540:
    Move 50, 1
    MoveEnd

Movement_1548:
    Move 12, 2
    MoveEnd

Movement_1550:
    Move 42, 4
    MoveEnd

Script_18:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "Hi, [f000]Ā\u0001\u0000![f000]븁\u0000"
    InfoMsg AspertiaCity_Text_Hi, 2
    MsgWinCloseAll
    ActorWalkRoute 3, 42, 758, 1, 8, 0
    ActorWalkRoute 0, 43, 758, 1, 8, 0
    VMSleep 30
    ActorWalkRoute 255, 43, 760, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_1F34
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Hey!\nYou get a Pokémon yet?[f000]븁\u0000\nThere aren't any Pokémon Trainers\naround here, and I'm getting bored![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_HeyGetPokemonYet, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1F14
    ActorCmdWait
    // "What's that?[f000]븁\u0000\n...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_Whats, 0, 0, 0
    // "A person named Bianca is\ngiving you a Pokémon? Really?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_PersonNamedBiancaGiving, 0, 0, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001's Sister: [f000]Ā\u0001\u0000...[f000]븁\u0000\nIf you get a Pokémon,\ntake really, really good care of it, OK?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SSisterIfGet, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1EE4
    ActorCmdWait
    ActorCmdExec 0, Movement_1F24
    ActorCmdWait
    VMSleep 20
    // "[f000]Ā\u0001\u0001: Yeah...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_Yeah, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1EDC
    ActorCmdWait
    // "OK! Let's go get your Pokémon![f000]븁\u0000\nThere's something I have to do![f000]븁\u0000\nAnd to do that, I need someone\nI can trust besides my partner Pokémon.[f000]븀\u0000\nA person I can trust![f000]븁\u0000\nThat's right! I'm talking about you!\nYou seem like you've got good instincts![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OkLetsGoGet, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1EE4
    VMSleep 8
    ActorCmdExec 3, Movement_1EEC
    ActorCmdWait
    // "You head on home.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_HeadHome, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_1EDC
    ActorCmdWait
    // "OK, big brother![f000]븁\u0000\nBye-bye, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OkBigBrotherBye, 3, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 43, 751, 0, 8, 0
    ActorCmdExec 0, Movement_1EDC
    VMSleep 16
    // "[f000]Ā\u0001\u0001: All riiight!\nLet's go find that person named Bianca!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_AllRiiightLetsGo, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdWait
    ActorDelete 3
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 41
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_16B0
    WorkAdd 0x8021, 1
    VMJump L_16B6

L_16B0:
    WorkSub 0x8021, 1

L_16B6:
    ActorWalkRoute 0, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_1EEC
    VMSleep 8
    ActorCmdExec 255, Movement_1EE4
    ActorCmdWait
    // "Let's go!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_LetsGo, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 0
    WorkSet 0x8001, 2
    WorkSet 0x8002, 1
    WorkSet 0x8003, 0
    WorkSet 0x8004, 10539
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst EVENT_WORK_0x40a1, 1
    FlagSet EVENT_FLAG_0x02e8
    FlagSet EVENT_FLAG_0x02e9
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    VMCall L_17B9
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Wait![f000]븁\u0000\nI was just in the Pokémon Center,\nand there wasn't anyone like that there.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_WaitJustPokemonCenter, 254, 0, 0
    MsgWinCloseAll
    VMCall L_186F
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    VMCall L_17B9
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: The Trainers' School\nwas just finished![f000]븁\u0000\nNo one is allowed inside until a\nTeacher, or better said, a Gym Leader,[f000]븀\u0000\nstarts working there![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_TrainersSchoolJustFinished, 254, 0, 0
    MsgWinCloseAll
    VMCall L_186F
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    VMCall L_17B9
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: That goes to Route 19.[f000]븁\u0000\nIf we don't find Bianca here in town,\nI'll go check it for you![f000]븁\u0000\n'Cause I already have a Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_GoesRoute19If, 254, 0, 0
    MsgWinCloseAll
    VMCall L_186F
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_17B9:
    PlayerGetDir 0x8020
    ActorCmdExec 254, Movement_1F14
    ActorCmdWait
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_17DA
    VMJump L_17F0

L_17DA:
    ActorCmdExec 255, Movement_1EEC
    ActorCmdExec 254, Movement_1EC4
    VMJump L_186B

L_17F0:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_1803
    VMJump L_1819

L_1803:
    ActorCmdExec 255, Movement_1EE4
    ActorCmdExec 254, Movement_1ECC
    VMJump L_186B

L_1819:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_182C
    VMJump L_1842

L_182C:
    ActorCmdExec 255, Movement_1EDC
    ActorCmdExec 254, Movement_1EB4
    VMJump L_186B

L_1842:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_1855
    VMJump L_186B

L_1855:
    ActorCmdExec 255, Movement_1ED4
    ActorCmdExec 254, Movement_1EBC
    VMJump L_186B

L_186B:
    ActorCmdWait
    VMReturn

L_186F:
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_1E94
    ActorCmdWait
    ActorPairSetMoveEnable 0
    VMReturn

Script_11:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 1
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_18B0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wish the Trainers' School\nwould hurry up and open![f000]븁\u0000\nThere's so much about\nPokémon I want to know!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_WishTrainersSchoolWould, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_18C4

L_18B0:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I know lots about Pokémon![f000]븁\u0000\n'Cause I learned so much\nat the Trainers' School!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_KnowLotsAboutPokemon, 0, 0
    LastKeyWait
    ActorMsgClose

L_18C4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_31:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 1
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_18F9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Trainers are the ones who\nhave their Pokémon partners battle.[f000]븁\u0000\nI hear Gym Leaders are\nreally strong Trainers!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_TrainersOnesWhoHave, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_190D

L_18F9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So about the Gym Leader Cheren...[f000]븁\u0000\nA few years ago he traveled all over\nthe Unova region with his Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_AboutGymLeaderCheren, 0, 0
    LastKeyWait
    ActorMsgClose

L_190D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The view of Route 19\nfrom the outlook is[f000]븀\u0000\nAspertia City's pride and joy."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_ViewRoute19From, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The power of science is amazing![f000]븁\u0000\nNow you can use communications\nto play with a hundred people[f000]븀\u0000\nat the same time!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_PowerScienceAmazingNow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_26:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "People go on journeys and become adults.\nMaybe I should leave this city, too..."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_PeopleGoJourneysBecome, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_27:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes wild Pokémon attack people![f000]븁\u0000\nBut the ones you befriend, the ones that\nstay by your side, are Pokémon, too!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SometimesWildPokemonAttack, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_28:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Your mom's really good at\ngetting Pokémon to rest[f000]븀\u0000\nand making them feel better!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_MomsReallyGoodGetting, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "This is Aspertia City.\nA city that reaches for the sky."
    MsgPlaceSign AspertiaCity_Text_AspertiaCityCityReaches, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WordSetPlayerName 0
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0000's House"
    MsgPlaceSign AspertiaCity_Text_SHouse, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Aspertia City Outlook Ahead\nUnova Unfolds before Your Eyes"
    MsgPlaceSign AspertiaCity_Text_AspertiaCityOutlookAhead, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 1
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_1A1D
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainers' School\nUnder Construction"
    MsgPlaceSign AspertiaCity_Text_TrainersSchoolUnderConstruction, 2
    MsgPlaceSignClose
    VMJump L_1A2F

L_1A1D:
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Aspertia City Pokémon Gym\nGym Leader: Cheren[f000]븀\u0000\nThe one who seeks the right path."
    MsgPlaceSign AspertiaCity_Text_AspertiaCityPokemonGym, 2
    MsgPlaceSignClose

L_1A2F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    ActorCmdExec 255, Movement_1EE4
    ActorCmdWait
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Hey! My sis has something\nshe wants to tell you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_HeySisHasSomething, 0, 1, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 47, 763, 1, 8, 0
    VMSleep 8
    ActorCmdExec 255, Movement_1EDC
    ActorCmdWait
    ActorCmdExec 3, Movement_1ED4
    ActorCmdWait
    // "[f000]Ā\u0001\u0001's Sister: Um...\n[f000]Ā\u0001\u0000...[f000]븁\u0000\nMy Purrloin...[f000]븁\u0000\nIt evolved, but thank you\nvery much for finding it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SSisterUmPurrloin, 3, 0, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: There's more, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_TheresMoreRight, 0, 1, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001's Sister: Um...\n[f000]Ā\u0001\u0000...[f000]븁\u0000\nThese days, I've been having\ndreams about a Pokémon.[f000]븀\u0000\nA Pokémon called Zoroark.[f000]븁\u0000\nIt was calling your name, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_SSisterUmThese, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_1EE4
    ActorCmdExec 3, Movement_1DB0
    // "[f000]Ā\u0001\u0001: I don't really get it,\nbut I hear that the Zoroark from her[f000]븀\u0000\ndreams is on Victory Road![f000]븁\u0000\nThat's what she wanted to say.\nBe seeing you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_DontReallyGetBut, 0, 1, 0
    ActorCmdWait
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1DC0
    ActorCmdExec 3, Movement_1DD0
    ActorCmdWait
    ActorCmdExec 0, Movement_1DE0
    ActorCmdWait
    // "Oh! Almost forgot.[f000]븁\u0000\nCongrats on becoming the Champion![f000]븁\u0000\nI called it!\nYou've got good instincts![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OhAlmostForgotCongrats, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1DF8
    ActorCmdExec 3, Movement_1DEC
    ActorCmdWait
    ActorDelete 0
    ActorDelete 3
    FlagSet EVENT_FLAG_0x02e9
    FlagSet EVENT_FLAG_0x02e8
    WorkSetConst EVENT_WORK_0x4115, 2
    HollowRivalCmd_0262 0, 10
    HollowRivalCmd_0262 1, 41
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_29:
    ActorsPauseAll
    ActorNew 45, 763, 3, 251, 357, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_30:
    ActorsPauseAll
    Plugin14_Cmd1000
    ActorCmdExec 251, Movement_1E38
    ActorCmdWait
    VMSleep 45
    MEPlay SEQ_ME_CALL
    MEWait
    ActorCmdExec 251, Movement_1F14
    ActorCmdWait
    ActorCmdExec 251, Movement_1E44
    ActorCmdWait
    // "Oh, hi![f000]븁\u0000\nWhy, Aurea Juniper![f000]븁\u0000\nIt's been far too long!\nWhat can I do for you?[f000]븁\u0000\n...[f000]븁\u0000\nWow! A Pokédex...\nFor my child?[f000]븁\u0000\nWhy, that's great! I think a journey\nwould be a wonderful experience![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_OhHiWhyAurea, 251, 0, 0
    MsgWinCloseAll
    VMSleep 30
    // "What now? She's already here?[f000]븁\u0000\nOh, for Pete's sake.\nYou never change.[f000]븁\u0000\nOnce you've decided on something,\nyou just start going.[f000]븁\u0000\nOK! Bianca, right?\nA big, green hat. Got it.[f000]븁\u0000\nOK!\nNo worries![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity_Text_WhatNowShesAlready, 251, 0, 0
    MsgWinCloseAll
    VMSleep 15
    SEPlay SEQ_SE_SYS_72
    ActorCmdExec 251, Movement_1E4C
    ActorCmdWait
    SEWait
    ActorWalkRoute 251, 47, 762, 1, 8, 0
    ActorCmdWait
    BMCreateHandleByGPos 0x8010, 1, 47, 761
    BMHndAudioVisualAnmPlay 0x8010, 0
    BMHndAnmWait 0x8010
    ActorCmdExec 251, Movement_1E9C
    ActorCmdWait
    ActorDelete 251
    FadeOutBlack
    RTReserveScript 17
    FadeWait
    MapChangeCore ZONE_ASPERTIA_CITY_2, 14, 0, 3, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_32:
    ActorsPauseAll
    Plugin14_Cmd1011
    ActorDelete 4
    ActorSetGPos 255, 53, 0, 750, 1
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1C26
    FadeEx 1, 16, 0, 2
    VMJump L_1C30

L_1C26:
    FadeEx 4, 16, 0, 2

L_1C30:
    ActorCmdExec 255, Movement_1E54
    FadeExWait
    ActorCmdWait
    VMSleep 30
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x2b9000, 0x1000f, 0x2fb8000, 60
    EvCameraWait
    VMSleep 20
    ActorCmdExec 13, Movement_1EF4
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 255, Movement_1E68
    ActorCmdWait
    VMSleep 30
    EvCameraMoveTo 9688, 0, 0xed000, 0x2f8000, 0x1000f, 0x2fa8000, 60
    ActorWalkRoute 13, 46, 763, 0, 16, 0
    VMSleep 4
    ActorWalkRoute 255, 46, 762, 0, 16, 0
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 13, Movement_1EF4
    VMSleep 8
    ActorCmdExec 255, Movement_1EDC
    ActorCmdWait
    VMSleep 60
    ActorWalkRoute 13, 47, 762, 0, 16, 0
    VMSleep 24
    ActorCmdExec 255, Movement_1EEC
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 13, Movement_1EF4
    ActorCmdWait
    BMCreateHandleByGPos 0x8010, 1, 47, 761
    BMHndAudioVisualAnmPlay 0x8010, 0
    BMHndAnmWait 0x8010
    ActorCmdExec 13, Movement_1E70
    VMSleep 8
    ActorCmdExec 255, Movement_1EA4
    ActorCmdWait
    ActorCmdExec 255, Movement_1EFC
    ActorCmdWait
    VMSleep 45
    EvCameraMoveTo 9688, 0, 0xed000, 0x2f8000, 0xb500f, 0x2fa8000, 100
    VMSleep 20
    ActorCmdExec 255, Movement_1E7C
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1D81
    FadeEx 1, 0, 16, 4
    VMJump L_1D8B

L_1D81:
    FadeEx 4, 0, 16, 4

L_1D8B:
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8010, 1
    BMHndAnmWait 0x8010
    BMReleaseHandle 0x8010
    FadeExWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FlagSet EVENT_FLAG_0x03ef
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_1DB0:
    Move 63, 2
    Move 14, 2
    Move 35, 1
    MoveEnd

Movement_1DC0:
    Move 14, 1
    Move 12, 1
    Move 75, 1
    MoveEnd

Movement_1DD0:
    Move 14, 2
    Move 12, 3
    Move 33, 1
    MoveEnd

Movement_1DE0:
    Move 13, 1
    Move 35, 1
    MoveEnd

Movement_1DEC:
    Move 63, 2
    Move 12, 9
    MoveEnd

Movement_1DF8:
    Move 12, 11
    MoveEnd
    Move 13, 1
    Move 14, 1
    Move 32, 1
    MoveEnd
    VMStackSub
    VMHalt
    Move 32, 1
    MoveEnd
    VMStackSub
    VMHalt
    VMStackDiv
    DebugPrint 12
    VMStackDiscard
    PokePartyGetSpecies 0, 13
    VMHalt
    VMStackDiv
    VMHalt
    .byte 0xfe
    .balign 4, 0

Movement_1E38:
    Move 100, 1
    Move 1, 1
    MoveEnd

Movement_1E44:
    Move 183, 1
    MoveEnd

Movement_1E4C:
    Move 186, 1
    MoveEnd

Movement_1E54:
    Move 13, 2
    Move 14, 10
    Move 13, 2
    Move 9, 2
    MoveEnd

Movement_1E68:
    Move 164, 6
    MoveEnd

Movement_1E70:
    Move 167, 1
    Move 69, 1
    MoveEnd

Movement_1E7C:
    Move 167, 1
    Move 62, 1
    Move 69, 1
    MoveEnd
    Move 154, 1
    MoveEnd

Movement_1E94:
    Move 13, 1
    MoveEnd

Movement_1E9C:
    Move 12, 1
    MoveEnd

Movement_1EA4:
    Move 15, 1
    MoveEnd

Movement_1EAC:
    Move 14, 1
    MoveEnd

Movement_1EB4:
    Move 0, 1
    MoveEnd

Movement_1EBC:
    Move 1, 1
    MoveEnd

Movement_1EC4:
    Move 2, 1
    MoveEnd

Movement_1ECC:
    Move 3, 1
    MoveEnd

Movement_1ED4:
    Move 32, 1
    MoveEnd

Movement_1EDC:
    Move 33, 1
    MoveEnd

Movement_1EE4:
    Move 34, 1
    MoveEnd

Movement_1EEC:
    Move 35, 1
    MoveEnd

Movement_1EF4:
    Move 28, 1
    MoveEnd

Movement_1EFC:
    Move 29, 1
    MoveEnd
    Move 30, 1
    MoveEnd
    Move 31, 1
    MoveEnd

Movement_1F14:
    Move 75, 1
    MoveEnd

Movement_1F1C:
    Move 159, 1
    MoveEnd

Movement_1F24:
    Move 161, 1
    MoveEnd

Movement_1F2C:
    Move 160, 1
    MoveEnd

Movement_1F34:
    Move 100, 1
    MoveEnd
