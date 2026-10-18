#include "asm/field_script.inc"
#include "text/script/marvelous_bridge.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0381
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4073
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0061
    FlagSet EVENT_FLAG_0x0381
    WorkSetConst EVENT_WORK_0x4073, 1

L_0061:
    FlagSet EVENT_FLAG_0x0403
    WorkSetConst EVENT_WORK_0x414f, 0
    VMStackPushFlag EVENT_FLAG_0x01e8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A9
    Random 0x8010, 100
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPushConst 20
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_00A9
    FlagReset EVENT_FLAG_0x0102
    FlagReset EVENT_FLAG_0x0403
    WorkSetConst EVENT_WORK_0x414f, 1

L_00A9:
    VMHalt

Script_6:
    VMStackPushFlag EVENT_FLAG_0x0403
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C4
    VMCall L_00C6

L_00C4:
    VMHalt

L_00C6:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PlayerGetGPos 0x8023, 0x8024
    VMStackPush 0x8023
    VMStackPushConst 147
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 36
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0107
    VMCall L_016D
    VMJump L_015F

L_0107:
    VMStackPush 0x8023
    VMStackPushConst 148
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 35
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0136
    VMCall L_016D
    VMJump L_015F

L_0136:
    VMStackPush 0x8023
    VMStackPushConst 149
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 36
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_015F
    VMCall L_016D

L_015F:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    VMReturn

L_016D:
    ActorDelete 3
    FlagSet EVENT_FLAG_0x0403
    WorkSetConst EVENT_WORK_0x414f, 0
    VMReturn

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x00fa
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019E
    VMCall L_01B8
    VMJump L_01B2

L_019E:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Man: Oh, yeah...\nReturns not accepted, got that?"
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_ManOhYeahReturns, 0, 0
    LastKeyWait
    ActorMsgClose

L_01B2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01B8:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Man: Son!\nI have a deal for YOU! And for you alone.[f000]븁\u0000\nHere's your chance. I will sell you the\nsecret Pokémon Magikarp...[f000]븀\u0000\nFor an unbelievable $500![f000]븁\u0000\nHow about it? Interested?"
    // "Man: Miss!\nI have a deal for YOU! And for you alone.[f000]븁\u0000\nHere's your chance. I will sell you the\nsecret Pokémon Magikarp...[f000]븀\u0000\nFor an unbelievable $500![f000]븁\u0000\nHow about it? Interested?"
    ActorMsgGendered 1024, MarvelousBridge_Text_ManSonHaveDeal, MarvelousBridge_Text_ManMissHaveDeal, 0, 2, 0
    MoneyWinDisp 31, 1
    WorkSetConst 0x8025, 0
    YesNoWin 0x8025
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F7
    WorkSetConst 0x8026, 0
    PokePartyGetCount 0x8026, 0
    WorkSetConst 0x8027, 0
    MoneyCheck 0x8027, 500
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0232
    MoneyWinClose
    // "Looks like you don't have enough money."
    ActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_LooksLikeDontHave, 0, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02D9

L_0232:
    VMStackPush 0x8026
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025D
    MoneyWinClose
    // "You have no room in your party!"
    ActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_HaveNoRoomParty, 0, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02D9

L_025D:
    ActorMsgClose
    WordSetPlayerName 0
    MEPlay SEQ_ME_POKEGET
    MoneySub 500
    MoneyWinUpdate
    // "[f000]Ā\u0001\u0000 bought the Magikarp\nfor $500."
    SystemMsg MarvelousBridge_Text_BoughtMagikarp500, 2
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll
    MoneyWinClose
    PokePartyAdd 0x8010, 129, 0, 5
    PokePartySetIV 0x8026, 73, 31
    // "Would you like to give a\nnickname to this Magikarp?"
    SystemMsg MarvelousBridge_Text_WouldLikeGiveNickname, 2
    WorkSetConst 0x8028, 0
    YesNoWin 0x8028
    InfoMsgClose
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C5
    WorkSetConst 0x8029, 0
    CallPokeNameInput 0x8029, 0x8026, 1
    VMJump L_02C5

L_02C5:
    // "Man: Oh, yeah...\nReturns not accepted, got that?"
    ActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_ManOhYeahReturns, 0, 2, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x00fa

L_02D9:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    VMJump L_0309

L_02F7:
    MoneyWinClose
    // "Oh, that's too bad..."
    ActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_OhThatsTooBad, 0, 2, 0
    LastKeyWait
    ActorMsgClose

L_0309:
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F3
    // "[f000]븉\u0001\u0002Oh... Oh...\nSo...thirsty...[f000]븁\u0000\nI met you on Village Bridge...[f000]븁\u0000\nG-g-give me...\nFresh Water...?[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_OhOhThirstyMet, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DF
    ItemSub ITEM_FRESH_WATER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CB
    MsgWinCloseAll
    SEPlay SEQ_SE_ARDEMO_01
    SEWait
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge...\nNo, I'll leave for the Marine Tube!"
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039D
    ActorCmdExec 1, Movement_041C
    VMJump L_03A5

L_039D:
    ActorCmdExec 1, Movement_0430

L_03A5:
    VMSleep 20
    ActorCmdExec 255, Movement_0440
    ActorCmdWait
    ActorDelete 1
    WorkSetConst EVENT_WORK_0x4108, 5
    FlagSet EVENT_FLAG_0x035e
    FlagReset EVENT_FLAG_0x035f
    VMJump L_03D9

L_03CB:
    // "[f000]븉\u0001\u0002But... You don't have Fresh Water...\nI appreciate the thought, though...[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_ButDontHaveFresh, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03D9:
    VMJump L_03ED

L_03DF:
    // "[f000]븉\u0001\u0002Thank...[f000]븁\u0000\nWhat?\nOh...[f000]븁\u0000\nWithout Fresh Water...\nI can't run on bridges anymore.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_ThankWhatOhWithout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03ED:
    VMJump L_0414

L_03F3:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0414
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge...\nNo, I'll leave for the Marine Tube!"
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0414:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_041C:
    Move 37, 4
    Move 17, 1
    Move 19, 5
    Move 23, 8
    MoveEnd

Movement_0430:
    Move 39, 4
    Move 19, 5
    Move 23, 8
    MoveEnd

Movement_0440:
    Move 3, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    // "The Lunar Wing started shining!\nDo you want to hold it up high?"
    SystemMsg MarvelousBridge_Text_LunarWingStartedShining, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F6
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x0381
    WorkSetConst EVENT_WORK_0x4073, 2
    ActorCmdExec 255, Movement_05DC
    ActorCmdWait
    ActorCmdExec 255, Movement_05E4
    ActorCmdWait
    VMSleep 30
    PVPlay 488, 0
    // "Lunaaan..."
    InfoMsg MarvelousBridge_Text_Lunaaan, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    ActorCmdExec 255, Movement_05D4
    PlayerGetGPos 0x8021, 0x8022
    ActorAdd 2
    ActorSetGPos 2, 110, 2, 14, 3
    WorkAdd 0x8021, 2
    ActorMoveLinear 2, 0x8021, 0, 0x8022, 64
    ActorSetGPos 2, 0x8021, 0, 0x8022, 2
    ActorCmdExec 2, Movement_05CC
    ActorCmdWait
    ActorCmdExec 255, Movement_05C4
    ActorCmdWait
    VMSleep 16
    VMJump L_04F8

L_04F6:
    MsgWinCloseAll

L_04F8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FlagSet EVENT_FLAG_0x01e8
    WorkSetConst EVENT_WORK_0x400a, 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 488, 0
    // "Lunaaan..."
    ScreamMsg MarvelousBridge_Text_Lunaaan, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 488, 68, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0556
    FlagSet EVENT_FLAG_0x0381
    WorkSetConst EVENT_WORK_0x4073, 3
    ActorDelete 2
    CallWildBattleEnd
    VMJump L_0558

L_0556:
    CallWildLose

L_0558:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_056F
    VMJump L_0579

L_056F:
    FlagSet EVENT_FLAG_0x017b
    VMJump L_059F

L_0579:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0599
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0599
    VMJump L_059F

L_0599:
    VMJump L_059F

L_059F:
    VMStackPushFlag EVENT_FLAG_0x017b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05BC
    // "Cresselia disappeared somewhere..."
    SystemMsg MarvelousBridge_Text_CresseliaDisappearedSomewhere, 2
    LastKeyWait
    InfoMsgClose

L_05BC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_05C4:
    Move 35, 1
    MoveEnd

Movement_05CC:
    Move 34, 1
    MoveEnd

Movement_05D4:
    Move 32, 1
    MoveEnd

Movement_05DC:
    Move 33, 1
    MoveEnd

Movement_05E4:
    Move 154, 1
    MoveEnd

Script_8:
    ActorsPauseAll
    ActorCmdExec 3, Movement_062C
    ActorCmdWait
    ActorDelete 3
    ActorCmdExec 4, Movement_0694
    ActorCmdWait
    // "Huh... Wha...\nD-did she just disappear?"
    ActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_HuhWhaDDid, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x414f, 2
    FlagSet EVENT_FLAG_0x0403
    FlagSet EVENT_FLAG_0x0102
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_062C:
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    Move 60, 1
    Move 70, 1
    Move 69, 1
    MoveEnd

Movement_0694:
    Move 3, 1
    Move 75, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01e8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06DF
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ahh...\nSuch magnificent scenery..."
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_AhhSuchMagnificentScenery, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0720

L_06DF:
    VMStackPushFlag EVENT_FLAG_0x0102
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_070C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Huh... Wha...\nD-did she just disappear?"
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_HuhWhaDDid, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0720

L_070C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ahh...\nSuch magnificent scenery..."
    ParentActorMsg MSGFILE_SCRIPT, MarvelousBridge_Text_AhhSuchMagnificentScenery, 0, 0
    LastKeyWait
    ActorMsgClose

L_0720:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
