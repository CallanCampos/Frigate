#include "asm/field_script.inc"
#include "text/script/twist_mountain.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Twist Mountain ahead.\nWatch out for wild Pokémon."
    MsgPlaceSign TwistMountain_Text_TwistMountainAheadWatch, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm Marshal, one of the Elite Four![f000]븁\u0000\nYou look like you're a\nPokémon Trainer with potential,[f000]븀\u0000\nbut I can't let you into Twist Mountain![f000]븁\u0000\nThe inside collapsed,\nand you can't get through!"
    ParentActorMsg MSGFILE_SCRIPT, TwistMountain_Text_ImMarshalOneElite, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01e9
    VMJump L_0152

L_005B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PlayerGetDir 0x8010
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000![f000]븁\u0000\nOh, I see! So, you travel all around\nlike this and toughen yourself up, then."
    ParentActorMsg MSGFILE_SCRIPT, TwistMountain_Text_OhSeeTravelAll, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0288
    VMSleep 10
    ActorCmdExec 255, Movement_02D0
    ActorCmdWait
    // "Well, I suppose you have\nmany battles ahead of you.[f000]븁\u0000\nPokémon battles as a Pokémon Trainer.[f000]븁\u0000\nBattles about how you\nshould live your life...[f000]븁\u0000\nYou'll lose sometimes,\nbut I think what matters[f000]븀\u0000\nis that you do things your own way.[f000]븁\u0000\nIf you surpass what you've done before,\nyou have bested yourself."
    ParentActorMsg MSGFILE_SCRIPT, TwistMountain_Text_WellSupposeHaveMany, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_00AD
    VMJump L_00BB

L_00AD:
    ActorCmdExec 0, Movement_0254
    VMJump L_00DC

L_00BB:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_00CE
    VMJump L_00DC

L_00CE:
    ActorCmdExec 0, Movement_0260
    VMJump L_00DC

L_00DC:
    ActorCmdWait
    // "Well then. I'll be waiting for\nyour challenge at the Pokémon League![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, TwistMountain_Text_WellThenIllWaiting, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_02B0
    ActorCmdWait
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0107
    VMJump L_0117

L_0107:
    ActorJumpToGPos 0, 143, 0, 206
    VMJump L_013A

L_0117:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_012A
    VMJump L_013A

L_012A:
    ActorJumpToGPos 0, 141, 0, 206
    VMJump L_013A

L_013A:
    ActorCmdExec 0, Movement_026C
    ActorCmdWait
    ActorDelete 0
    FlagSet EVENT_FLAG_0x029c
    WorkSetConst EVENT_WORK_0x413b, 1

L_0152:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AD
    ActorCmdExec 0, Movement_02E8
    ActorCmdWait
    ActorCmdExec 0, Movement_02B0
    ActorCmdExec 255, Movement_02C8
    ActorCmdWait
    // "I'm Marshal, one of the Elite Four![f000]븁\u0000\nYou look like you're a\nPokémon Trainer with potential,[f000]븀\u0000\nbut I can't let you into Twist Mountain![f000]븁\u0000\nThe inside collapsed,\nand you can't get through!"
    ActorMsg MSGFILE_SCRIPT, TwistMountain_Text_ImMarshalOneElite, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0298
    ActorCmdWait
    FlagSet EVENT_FLAG_0x01e9
    VMJump L_024E

L_01AD:
    ActorCmdExec 0, Movement_02E8
    ActorCmdWait
    ActorCmdExec 0, Movement_02B0
    ActorCmdExec 255, Movement_02C8
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000![f000]븁\u0000\nOh, I see! So, you travel all around\nlike this and toughen yourself up, then."
    ActorMsg MSGFILE_SCRIPT, TwistMountain_Text_OhSeeTravelAll, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0274
    VMSleep 10
    ActorCmdExec 255, Movement_02E0
    ActorCmdWait
    // "Well, I suppose you have\nmany battles ahead of you.[f000]븁\u0000\nPokémon battles as a Pokémon Trainer.[f000]븁\u0000\nBattles about how you\nshould live your life...[f000]븁\u0000\nYou'll lose sometimes,\nbut I think what matters[f000]븀\u0000\nis that you do things your own way.[f000]븁\u0000\nIf you surpass what you've done before,\nyou have bested yourself."
    ActorMsg MSGFILE_SCRIPT, TwistMountain_Text_WellSupposeHaveMany, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_02D8
    ActorCmdWait
    // "Well then. I'll be waiting for\nyour challenge at the Pokémon League![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, TwistMountain_Text_WellThenIllWaiting, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_02B0
    ActorCmdWait
    ActorJumpToGPos 0, 143, 0, 206
    ActorCmdExec 255, Movement_02B0
    ActorCmdExec 0, Movement_026C
    ActorCmdWait
    ActorDelete 0
    FlagSet EVENT_FLAG_0x029c
    WorkSetConst EVENT_WORK_0x413b, 1

L_024E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0254:
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_0260:
    Move 14, 1
    Move 32, 1
    MoveEnd

Movement_026C:
    Move 13, 8
    MoveEnd

Movement_0274:
    Move 15, 1
    Move 13, 1
    MoveEnd
    Move 13, 1
    MoveEnd

Movement_0288:
    Move 9, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0298:
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_02B0:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_02C8:
    Move 32, 1
    MoveEnd

Movement_02D0:
    Move 33, 1
    MoveEnd

Movement_02D8:
    Move 34, 1
    MoveEnd

Movement_02E0:
    Move 35, 1
    MoveEnd

Movement_02E8:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
