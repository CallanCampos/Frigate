#include "asm/field_script.inc"
#include "text/script/abundant_shrine.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_2:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    PokePartyFindEx 645, 0, 0x8023, 0x8024
    IsOneShotDRObtained_02F2 7, 0x8025
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4150
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_008D
    FlagReset EVENT_FLAG_0x0404
    WorkSetConst EVENT_WORK_0x4150, 1
    VMJump L_00AA

L_008D:
    FlagSet EVENT_FLAG_0x0404
    VMStackPush EVENT_WORK_0x4150
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AA
    WorkSetConst EVENT_WORK_0x4150, 0

L_00AA:
    VMStackPush EVENT_WORK_0x4155
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C1
    FlagReset EVENT_FLAG_0x040e

L_00C1:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    VMHalt

Script_3:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 1
    ActorCmdExec 11, Movement_04C8
    ActorCmdWait
    VMSleep 6
    ActorCmdExec 11, Movement_04B0
    ActorCmdWait
    // "Hmm![f000]븁\u0000"
    InfoMsg AbundantShrine_Text_Hmm, 1
    MsgWinCloseAll
    ActorWalkRoute 11, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait
    // "Wow! Wow!\nWhat an awesome Trainer![f000]븁\u0000\nThat Pokémon you are traveling with\nis Landorus in Therian Forme![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_WowWowWhatAwesome, 11, 5, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_04C0
    ActorCmdWait
    // "A long, long time ago...[f000]븁\u0000\nTornadus and Thundurus\nstarted a rampage in Therian Forme.[f000]븁\u0000\nWhen everybody thought the Unova region\nwould be ruined and gave up hope,[f000]븀\u0000\nLandorus, also in Therian Forme,[f000]븀\u0000\nappeared out of nowhere[f000]븀\u0000\nand stopped them.[f000]븁\u0000\nThat's what they say.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_LongLongTimeAgo, 11, 5, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_021C
    ActorCmdWait
    // "Oh! Hey!\nWait a second.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_OhHeyWaitSecond, 11, 5, 0
    MsgWinCloseAll
    ActorWalkRoute 11, 22, 13, 1, 8, 0
    ActorCmdWait
    VMSleep 8
    ActorCmdExec 11, Movement_04A8
    ActorCmdWait
    VMSleep 8
    ActorWalkRoute 11, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait
    // "This has been placed in the shrine.\nTake this.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_HasBeenPlacedShrine, 11, 5, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 638
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    SetOneShotDRObtained 6
    // "They say the Reveal Glass brings\nThundurus, Tornadus, and Landorus[f000]븀\u0000\nback to their original appearance.[f000]븁\u0000\nHowever, I don't know whether\nIncarnate Forme or Therian Forme[f000]븀\u0000\nis the true Forme.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_TheySayRevealGlass, 11, 5, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E9
    ActorWalkRoute 11, 23, 24, 1, 8, 0
    VMJump L_01F7

L_01E9:
    ActorWalkRoute 11, 22, 24, 1, 8, 0

L_01F7:
    VMSleep 16
    ActorCmdExec 255, Movement_04B0
    ActorCmdWait
    ActorDelete 11
    FlagSet EVENT_FLAG_0x0404
    WorkSetConst EVENT_WORK_0x4150, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_021C:
    Move 33, 1
    Move 75, 1
    MoveEnd

Script_5:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x338000, 32795, 0x318000, 40
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 50
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026D
    ActorCmdExec 255, Movement_0364
    VMJump L_0275

L_026D:
    ActorCmdExec 255, Movement_0374

L_0275:
    ActorCmdWait
    EvCameraWait
    // "So you know what Landorus is like\nwhen it means business, huh?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_KnowWhatLandorusLike, 13, 5, 0
    MsgWinCloseAll
    ActorCmdExec 12, Movement_037C
    VMSleep 8
    ActorCmdExec 13, Movement_04B0
    ActorCmdWait
    // "Yeah! I do![f000]븁\u0000\nIt changes into another Forme\nand flies around the sky![f000]븁\u0000\nTornadus and Thundurus\nchange Formes, too![f000]븀\u0000\n...Well, that's the story, anyway."
    // "Yeah! I do![f000]븁\u0000\nIt changes into another Forme\nand flies around the sky![f000]븁\u0000\nThundurus and Tornadus\nchange Formes, too![f000]븀\u0000\n...Well, that's the story, anyway."
    ActorMsgVersioned 1024, AbundantShrine_Text_YeahChangesIntoAnother, AbundantShrine_Text_YeahChangesIntoAnother_2, 12, 6, 0
    MsgWaitAdvance
    ActorMsgClose
    ActorCmdExec 13, Movement_0398
    ActorCmdWait
    // "That's good, then![f000]븁\u0000\n...You know, I've only heard\nabout it in stories, too."
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_ThatsGoodThenKnow, 13, 5, 0
    MsgWaitAdvance
    MsgWinCloseAll
    EvCameraMoveToDefault 34
    ActorCmdExec 13, Movement_03B0
    VMSleep 2
    ActorCmdExec 12, Movement_03A0
    VMSleep 8
    ActorCmdExec 255, Movement_04B0
    ActorCmdWait
    ActorDelete 12
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 13, Movement_03C0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 13
    SEWait
    ActorCmdExec 14, Movement_03C8
    VMSleep 8
    ActorCmdExec 255, Movement_04A8
    ActorCmdWait
    // "This little shrine is dedicated to a\nPokémon called Landorus.[f000]븁\u0000\nThey say that when Tornadus and\nThundurus were whipping up storms that[f000]븀\u0000\ndamaged houses and fields all over the[f000]븀\u0000\nUnova region, Landorus made them stop.[f000]븁\u0000\nThat's a story from when my\ngrandpa was just a little kid!"
    ActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_LittleShrineDedicatedPokemon, 14, 3, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 14, Movement_03D4
    VMSleep 16
    ActorCmdExec 255, Movement_04B0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 14
    SEWait
    WorkSetConst EVENT_WORK_0x4155, 1
    FlagSet EVENT_FLAG_0x040e
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0364:
    Move 12, 1
    Move 15, 1
    Move 32, 0
    MoveEnd

Movement_0374:
    Move 12, 1
    MoveEnd

Movement_037C:
    Move 51, 1
    Move 19, 1
    Move 17, 3
    Move 19, 1
    Move 32, 0
    Move 48, 1
    MoveEnd

Movement_0398:
    Move 17, 1
    MoveEnd

Movement_03A0:
    Move 17, 2
    Move 18, 1
    Move 17, 1
    MoveEnd

Movement_03B0:
    Move 17, 4
    Move 18, 1
    Move 37, 0
    MoveEnd

Movement_03C0:
    Move 17, 1
    MoveEnd

Movement_03C8:
    Move 15, 1
    Move 13, 1
    MoveEnd

Movement_03D4:
    Move 14, 1
    Move 13, 3
    MoveEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0992
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0436
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Why do delicious vegetables grow in the\nfields of Mistralton City?[f000]븁\u0000\nIt's because the ones who grow them are\nvery picky about the mulch they use.[f000]븁\u0000\nIt's good mulch. Strong mulch. It's\nstored up a lot of energy from nature.[f000]븀\u0000\nAnd that makes it really stinky![f000]븁\u0000\nSuch a powerful aroma! It burns\nall the way to the back of your nose![f000]븁\u0000\nAww. Just remembering it\nmakes me dizzy![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_WhyDeliciousVegetablesGrow, 0, 0
    MsgWinCloseAll
    Cmd_0275 0, 39, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg AbundantShrine_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "Stinky, stinky mulch\ngrows delicious vegetables!"
    ParentActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_StinkyStinkyMulchGrows, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0992
    VMJump L_044A

L_0436:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Stinky, stinky mulch\ngrows delicious vegetables!"
    ParentActorMsg MSGFILE_SCRIPT, AbundantShrine_Text_StinkyStinkyMulchGrows, 0, 0
    LastKeyWait
    ActorMsgClose

L_044A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a small, weathered shrine."
    InfoMsg AbundantShrine_Text_ItsSmallWeatheredShrine, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
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

Movement_04A8:
    Move 32, 1
    MoveEnd

Movement_04B0:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_04C0:
    Move 35, 1
    MoveEnd

Movement_04C8:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 161, 1
    MoveEnd
