#include "asm/field_script.inc"
#include "text/script/driftveil_drawbridge.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0999
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes, above the bridge, you can\nsee the shadows of bird Pokémon, right?[f000]븁\u0000\nTheir feathers drift to the ground here![f000]븁\u0000\nAnd when you try to pick them up,\noccasionally you'll run into a Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_SometimesAboveBridgeCan, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    Cmd_0275 0, 14, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink!"
    SystemMsg DriftveilDrawbridge_Text_FunfestMissionHasBeen, 0
    SEWait
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0999
    VMJump L_0070

L_005C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes, above the bridge, you can\nsee the shadows of bird Pokémon, right?[f000]븁\u0000\nTheir feathers drift to the ground here![f000]븁\u0000\nAnd when you try to pick them up,\noccasionally you'll run into a Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_SometimesAboveBridgeCan, 0, 0
    LastKeyWait
    ActorMsgClose

L_0070:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Driftveil City is famous for this\ndrawbridge, the PWT, and of course,[f000]븀\u0000\nthe heartbreaker, Charles."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_DriftveilCityFamousDrawbridge, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x011a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if the people who made\nthis drawbridge were with Charizard."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_WonderIfPeopleWho, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_014A

L_00C1:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PokePartyFindBySpecies 6, 0x8020, 0x8021
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0102
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Driftveil Drawbridge...\nIt's also known as the Charizard Bridge![f000]븁\u0000\nBecause the raised drawbridge looks like\nthe Pokémon called Charizard.[f000]븁\u0000\n...But, I've never seen Charizard,\nso I don't know..."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_DriftveilDrawbridgeItsAlso, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_014A

L_0102:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Driftveil Drawbridge...\nIt's also known as the Charizard Bridge![f000]븁\u0000\nBecause the raised drawbridge looks like\nthe Pokémon called Charizard.[f000]븁\u0000\n...But, I've never seen Charizard,\nso I don't know...[f000]븁\u0000\n...What?!\nAre you with Charizard?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_DriftveilDrawbridgeItsAlso_2, 0, 0
    MsgWinCloseAll
    VMCall L_0150
    // "Wow! Awesome!\nYeah, I think it looks like Charizard.[f000]븁\u0000\nThanks! As a token of my appreciation,\nplease accept this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_WowAwesomeYeahThink, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 36
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x011a

L_014A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0150:
    WorkSetConst 0x8022, 0
    PlayerGetDir 0x8022
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_016D
    VMJump L_017B

L_016D:
    ActorCmdExec 2, Movement_031C
    VMJump L_01DE

L_017B:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_018E
    VMJump L_019C

L_018E:
    ActorCmdExec 2, Movement_0350
    VMJump L_01DE

L_019C:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_01AF
    VMJump L_01BD

L_01AF:
    ActorCmdExec 2, Movement_0384
    VMJump L_01DE

L_01BD:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_01D0
    VMJump L_01DE

L_01D0:
    ActorCmdExec 2, Movement_03B8
    VMJump L_01DE

L_01DE:
    ActorCmdWait
    WorkSetConst 0x8022, 0
    VMReturn

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D0
    // "[f000]븉\u0001\u0002Panting... Panting...\nI'm...thirsty...[f000]븁\u0000\nI met you on the Skyarrow Bridge...[f000]븁\u0000\nG-g-give me...\nFresh Water...?[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_PantingPantingImThirsty, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BC
    ItemSub ITEM_FRESH_WATER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A8
    MsgWinCloseAll
    SEPlay SEQ_SE_ARDEMO_01
    SEWait
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027A
    ActorCmdExec 3, Movement_02F8
    VMJump L_0282

L_027A:
    ActorCmdExec 3, Movement_0308

L_0282:
    VMSleep 20
    ActorCmdExec 255, Movement_0314
    ActorCmdWait
    ActorDelete 3
    WorkSetConst EVENT_WORK_0x4108, 2
    FlagSet EVENT_FLAG_0x035b
    FlagReset EVENT_FLAG_0x035c
    VMJump L_02B6

L_02A8:
    // "[f000]븉\u0001\u0002But... You don't have Fresh Water...\nI appreciate the thought, though...[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_ButDontHaveFresh, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02B6:
    VMJump L_02CA

L_02BC:
    // "[f000]븉\u0001\u0002Thank...[f000]븁\u0000\nWhat?\nOh...[f000]븁\u0000\nWithout Fresh Water...\nI can't run on bridges anymore.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_ThankWhatOhWithout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02CA:
    VMJump L_02F1

L_02D0:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F1
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilDrawbridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02F1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_02F8:
    Move 36, 4
    Move 16, 1
    Move 18, 10
    MoveEnd

Movement_0308:
    Move 38, 4
    Move 18, 10
    MoveEnd

Movement_0314:
    Move 2, 1
    MoveEnd

Movement_031C:
    Move 14, 1
    Move 13, 1
    Move 35, 1
    Move 13, 1
    Move 15, 1
    Move 32, 1
    Move 15, 1
    Move 12, 1
    Move 34, 1
    Move 12, 1
    Move 14, 1
    Move 33, 1
    MoveEnd

Movement_0350:
    Move 12, 1
    Move 14, 1
    Move 33, 1
    Move 14, 1
    Move 13, 1
    Move 35, 1
    Move 13, 1
    Move 15, 1
    Move 32, 1
    Move 15, 1
    Move 12, 1
    Move 34, 1
    MoveEnd

Movement_0384:
    Move 12, 1
    Move 15, 1
    Move 33, 1
    Move 15, 1
    Move 13, 1
    Move 34, 1
    Move 13, 1
    Move 14, 1
    Move 32, 1
    Move 14, 1
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_03B8:
    Move 14, 1
    Move 12, 1
    Move 35, 1
    Move 12, 1
    Move 15, 1
    Move 33, 1
    Move 15, 1
    Move 13, 1
    Move 34, 1
    Move 13, 1
    Move 14, 1
    Move 32, 1
    MoveEnd
