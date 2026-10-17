#include "asm/field_script.inc"
#include "text/script/reversal_mountain_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPush EVENT_WORK_0x4120
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_005D
    WorkSetConst EVENT_WORK_0x4120, 1
    FlagReset EVENT_FLAG_0x03bf

L_005D:
    VMHalt

Script_2:
    VMHalt

Script_3:
    ActorsPauseAll
    WordSetPlayerName 0
    VMStackPushFlag EVENT_FLAG_0x01ab
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013C
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0094
    PlayerSetSpecialSequence 1

L_0094:
    ActorCmdExec 13, Movement_01D4
    ActorCmdWait
    // "Bianca: Heeey! [f000]Ā\u0001\u0000![f000]븁\u0000"
    // "Bianca: Hi there, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsgGendered 1024, ReversalMountain4_Text_BiancaHeeey, ReversalMountain4_Text_BiancaHiThere, 13, 0, 0
    // "Um, you know what?[f000]븁\u0000\nThere's something I want to investigate\nhere in Reversal Mountain.[f000]븁\u0000\nBut the wild Pokémon here are really\ntough, and I'm having trouble with them![f000]븁\u0000\nCould you come with me? Please?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_UmKnowWhatTheres, 13, 0, 0
    MsgWinCloseAll
    ActorCmdExec 13, Movement_0550
    ActorCmdWait
    // "Oh! Don't worry![f000]븁\u0000\nI'll take care of healing\nour Pokémon, OK?[f000]븁\u0000\nReady? Let's go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_OhDontWorryIll, 13, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0500
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 13
    WorkSet 0x8001, 7
    WorkSet 0x8002, 1
    WorkSet 0x8003, 4
    WorkSet 0x8004, 10538
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01ab
    WorkSetConst EVENT_WORK_0x4121, 1
    HollowRivalCmd_0262 1, 20
    VMJump L_01C3

L_013C:
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0157
    PlayerSetSpecialSequence 1

L_0157:
    ActorCmdExec 13, Movement_01D4
    ActorCmdWait
    // "There's still something I want to\nlook for in Reversal Mountain.[f000]븀\u0000\nCome with me![f000]븁\u0000\nDon't worry. I'll take care of healing\nour Pokémon, OK?[f000]븁\u0000\nOK, here we go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_TheresStillSomethingWant, 13, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0500
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 13
    WorkSet 0x8001, 7
    WorkSet 0x8002, 1
    WorkSet 0x8003, 4
    WorkSet 0x8004, 10538
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000

L_01C3:
    WorkSetConst EVENT_WORK_0x4120, 2
    FlagSet EVENT_FLAG_0x03bf
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01D4:
    Move 75, 1
    Move 14, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    VMCall L_038F
    // "Oh! I want to do a little\nmore looking around.[f000]븀\u0000\nWant to split up for now?"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_OhWantLittleMore, 254, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BE
    VMStackPush EVENT_WORK_0x4121
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0241
    // "OK, then![f000]븁\u0000\nI want to do a little more research\nabout where Heatran might be![f000]븁\u0000\nThank you for coming with me!\nBe careful on the rest of your journey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_OkThenWantLittle, 254, 0, 0
    WorkSetConst EVENT_WORK_0x4120, 3
    FlagSet EVENT_FLAG_0x03bf
    FlagReset EVENT_FLAG_0x03c0
    VMJump L_0251

L_0241:
    // "Oh, OK![f000]븁\u0000\nI want to do a little more\ninvestigating about Heatran.[f000]븁\u0000\nHeatran is a very rarely seen Pokémon,\nso if I find out more about it,[f000]븀\u0000\nProfessor Juniper will be really happy![f000]븁\u0000\nThanks for helping me!\nBe careful on your journey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_OhOkWantLittle, 254, 0, 0
    FlagReset EVENT_FLAG_0x03bf

L_0251:
    MsgWinCloseAll
    ActorWalkRoute 254, 8, 9, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 254, Movement_0540
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 6
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush EVENT_WORK_0x4120
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02B8
    ActorCmdExec 255, Movement_0508
    ActorCmdWait
    RTReserveScript 1
    MapChangeWarp ZONE_REVERSAL_MOUNTAIN_2, 29, 4, 2

L_02B8:
    VMJump L_02DC

L_02BE:
    // "OK! Then let's do a little more\nlooking around![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_OkThenLetsLittle, 254, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_0500
    ActorCmdWait
    ActorPairSetMoveEnable 0

L_02DC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WordSetPlayerName 0
    PlayerGetDir 0x8020
    ActorCmdExec 254, Movement_0550
    VMSleep 8
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0318
    ActorCmdExec 255, Movement_0540
    VMJump L_0320

L_0318:
    ActorCmdExec 255, Movement_0548

L_0320:
    ActorCmdWait
    ActorCmdExec 254, Movement_0530
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034D
    ActorCmdExec 254, Movement_0548
    VMJump L_0355

L_034D:
    ActorCmdExec 254, Movement_0540

L_0355:
    ActorCmdWait
    // "Oh! [f000]Ā\u0001\u0000![f000]븁\u0000\nHere! This is the place!\nLet's look around a little![f000]븁\u0000"
    // "Oh! [f000]Ā\u0001\u0000![f000]븁\u0000\nHere! This is the place!\nLet's look around a little![f000]븁\u0000"
    ActorMsgGendered 1024, ReversalMountain4_Text_OhHerePlaceLets, ReversalMountain4_Text_OhHerePlaceLets_2, 254, 0, 0
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4121, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Reversal Mountain... I wonder...\nCould a Magma Stone be in there?[f000]븁\u0000\nHave you heard of it?\nThey say a Magma Stone was found[f000]븀\u0000\nin a volcano in the distant Sinnoh region.[f000]븁\u0000\nApparently, it had something\nto do with Heatran!"
    ParentActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_ReversalMountainWonderCould, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_038F:
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_03A6
    VMJump L_03BC

L_03A6:
    ActorCmdExec 255, Movement_0548
    ActorCmdExec 254, Movement_0520
    VMJump L_0437

L_03BC:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_03CF
    VMJump L_03E5

L_03CF:
    ActorCmdExec 255, Movement_0540
    ActorCmdExec 254, Movement_0528
    VMJump L_0437

L_03E5:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_03F8
    VMJump L_040E

L_03F8:
    ActorCmdExec 255, Movement_0538
    ActorCmdExec 254, Movement_0510
    VMJump L_0437

L_040E:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0421
    VMJump L_0437

L_0421:
    ActorCmdExec 255, Movement_0530
    ActorCmdExec 254, Movement_0518
    VMJump L_0437

L_0437:
    ActorCmdWait
    VMReturn

Script_7:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01d5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04D5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You strike me as the type\nwho fills out the Habitat List![f000]븁\u0000\nC'mon, tell me what kind of Pokémon\nlive in Reversal Mountain![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_StrikeTypeWhoFills, 0, 0
    PokeDexCheckHabitatList 461, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C1
    // "Huh! That's amazing.\nSo these Pokémon live here, then![f000]븁\u0000\nThanks for showing me something good!\nLet me pay you back with this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_HuhThatsAmazingThese, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 381
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01d5
    // "False Swipe leaves a Pokémon\nwith 1 HP when it would have fainted.[f000]븁\u0000\nIt's a very restrained move.[f000]븁\u0000\nIt's a great TM to use for catching\nPokémon and filling out the Habitat List!"
    ParentActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_FalseSwipeLeavesPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04CF

L_04C1:
    // "Oh... Still looking into it, huh?[f000]븁\u0000\nSome of the Pokémon are on the\noutside of Reversal Mountain as well."
    ParentActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_OhStillLookingInto, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_04CF:
    VMJump L_04E9

L_04D5:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "False Swipe leaves a Pokémon\nwith 1 HP when it would have fainted.[f000]븁\u0000\nIt's a very restrained move.[f000]븁\u0000\nIt's a great TM to use for catching\nPokémon and filling out the Habitat List!"
    ParentActorMsg MSGFILE_SCRIPT, ReversalMountain4_Text_FalseSwipeLeavesPokemon, 0, 0
    LastKeyWait
    ActorMsgClose

L_04E9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0500:
    Move 15, 1
    MoveEnd

Movement_0508:
    Move 14, 1
    MoveEnd

Movement_0510:
    Move 0, 1
    MoveEnd

Movement_0518:
    Move 1, 1
    MoveEnd

Movement_0520:
    Move 2, 1
    MoveEnd

Movement_0528:
    Move 3, 1
    MoveEnd

Movement_0530:
    Move 32, 1
    MoveEnd

Movement_0538:
    Move 33, 1
    MoveEnd

Movement_0540:
    Move 34, 1
    MoveEnd

Movement_0548:
    Move 35, 1
    MoveEnd

Movement_0550:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
