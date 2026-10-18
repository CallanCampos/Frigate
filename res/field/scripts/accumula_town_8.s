#include "asm/field_script.inc"
#include "text/script/accumula_town_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0

Script_1:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4107
    VMStackPushConst 1
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_005F
    VMCall L_00E8
    VMJump L_00E2

L_005F:
    ISSSwitchQuery 0x8010, 1
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Shall I play a song I like for you?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_ShallPlaySongLike, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C1
    // "Hee hee!\nListen closely, then!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_HeeHeeListenClosely, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0480
    ActorCmdWait
    ISSSwitchEnable 1
    VMJump L_00CF

L_00C1:
    // "Alas. That's unfortunate...\nIf you change your mind, let me know!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_AlasThatsUnfortunateIf, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00CF:
    VMJump L_00E2

L_00D5:
    SEPlay SEQ_SE_MESSAGE
    // "She's absorbed in her performance!"
    InfoMsg AccumulaTown8_Text_ShesAbsorbedHerPerformance, 2
    LastKeyWait
    MsgWinCloseAll

L_00E2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00E8:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4107
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0243
    PokePartyGetCount 0x8023, 0

L_0107:
    VMStackPush 0x8023
    VMStackPush 0x8024
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_015B
    PokePartyGetSpecies 0x8025, 0x8024
    PokePartyIsEgg 0x8027, 0x8024
    VMStackPush 0x8025
    VMStackPushConst 401
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_014F
    WorkSetConst 0x8026, 1

L_014F:
    WorkAdd 0x8024, 1
    VMJump L_0107

L_015B:
    // "Playing the piano isn't much fun lately...[f000]븁\u0000\nMaybe what I need is to hear a cry\nwith a wonderful melody.[f000]븀\u0000\nThat might get my imagination going.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_PlayingPianoIsntMuch, 0, 0
    // "Do you know about this?[f000]븁\u0000\nA Pokémon called Kricketot has a cry\nthat sounds like an instrument![f000]븀\u0000\nI'd sure like to hear it sometime."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_KnowAboutPokemonCalled, 0, 0
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0239
    MsgWaitAdvance
    MsgWinCloseAll
    PVPlay 401, 0
    PVWait
    ActorCmdExec 0, Movement_04A0
    ActorCmdWait
    VMSleep 8
    // "The cry of that Kricketot with you...[f000]븁\u0000\nDedelee dun dun dun dum! ♪\nIt sounds just like a xylophone![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_CryKricketotDedeleeDun, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0480
    ActorCmdWait
    VMSleep 32
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_01CD
    VMJump L_01DB

L_01CD:
    ActorCmdExec 0, Movement_0488
    VMJump L_021D

L_01DB:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_01EE
    VMJump L_01FC

L_01EE:
    ActorCmdExec 0, Movement_0498
    VMJump L_021D

L_01FC:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_020F
    VMJump L_021D

L_020F:
    ActorCmdExec 0, Movement_0490
    VMJump L_021D

L_021D:
    ActorCmdWait
    // "One more thing...[f000]븁\u0000\nIf I could hear just one more cry,\nmy heart would flutter and my fingers[f000]븀\u0000\nwould dance over the keys!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_OneMoreThingIf, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4107, 1
    VMJump L_023D

L_0239:
    LastKeyWait
    MsgWinCloseAll

L_023D:
    VMJump L_03CC

L_0243:
    VMStackPush EVENT_WORK_0x4107
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CC
    PokePartyGetCount 0x8023, 0

L_025C:
    VMStackPush 0x8023
    VMStackPush 0x8024
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_02B0
    PokePartyGetSpecies 0x8025, 0x8024
    PokePartyIsEgg 0x8027, 0x8024
    VMStackPush 0x8025
    VMStackPushConst 293
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02A4
    WorkSetConst 0x8026, 1

L_02A4:
    WorkAdd 0x8024, 1
    VMJump L_025C

L_02B0:
    // "I hear a Pokémon called Whismur\nhas a very quiet cry.[f000]븁\u0000\nIf I knew what it sounded like,\nI could play perfect pianissimo!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_HearPokemonCalledWhismur, 0, 0
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C8
    MsgWaitAdvance
    MsgWinCloseAll
    PVPlay 293, 0
    PVWait
    ActorCmdExec 0, Movement_04A0
    ActorCmdWait
    VMSleep 8
    // "The cry of that Whismur with you...[f000]븁\u0000\nIt's wonderfully pianissimo!\nIt reverberates like a sweet murmur!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_CryWhismurItsWonderfully, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0480
    ActorCmdWait
    VMSleep 16
    ActorCmdExec 0, Movement_04A8
    ActorCmdWait
    ActorCmdExec 0, Movement_0480
    ActorCmdWait
    VMSleep 32
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_0332
    VMJump L_0340

L_0332:
    ActorCmdExec 0, Movement_0488
    VMJump L_0382

L_0340:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_0353
    VMJump L_0361

L_0353:
    ActorCmdExec 0, Movement_0498
    VMJump L_0382

L_0361:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0374
    VMJump L_0382

L_0374:
    ActorCmdExec 0, Movement_0490
    VMJump L_0382

L_0382:
    ActorCmdWait
    // "Thanks to you and your Pokémon,\nmy mind is overflowing with melodies![f000]븁\u0000\nI can play the piano again![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_ThanksPokemonMindOverflowing, 0, 0
    // "This is a token of my appreciation for\nyour getting me out of my slump. Thanks!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_TokenAppreciationGettingOut, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 277
    WorkSet 0x8001, 1
    RTCallGlobal 2802
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst EVENT_WORK_0x4107, 2
    VMJump L_03CC

L_03C8:
    LastKeyWait
    MsgWinCloseAll

L_03CC:
    VMReturn

Script_2:
    ActorsPauseAll
    ISSSwitchQuery 0x8010, 2
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0446
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you want to listen to my drum?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_WantListenDrum, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0432
    // "My! Heart!\nAs long as my heart beats![f000]븀\u0000\nI will keep! On! Drumming!!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_HeartLongHeartBeats, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0488
    ActorCmdWait
    ISSSwitchEnable 2
    VMJump L_0440

L_0432:
    // "...I thought so."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_Thought, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0440:
    VMJump L_0453

L_0446:
    SEPlay SEQ_SE_MESSAGE
    // "He is concentrating on his performance!"
    InfoMsg AccumulaTown8_Text_HeConcentratingHisPerformance, 2
    LastKeyWait
    MsgWinCloseAll

L_0453:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 572, 0
    // "Chip kwip!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown8_Text_ChipKwip, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0480:
    Move 32, 1
    MoveEnd

Movement_0488:
    Move 33, 1
    MoveEnd

Movement_0490:
    Move 35, 1
    MoveEnd

Movement_0498:
    Move 34, 1
    MoveEnd

Movement_04A0:
    Move 75, 1
    MoveEnd

Movement_04A8:
    Move 61, 1
    Move 35, 1
    Move 34, 1
    Move 61, 1
    MoveEnd
