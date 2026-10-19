#include "asm/field_script.inc"
#include "text/script/castelia_city_gym.h"

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
    ScriptEntriesEnd

Script_10:
    VMStackPushFlag EVENT_FLAG_0x010a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x409d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_005F
    ActorSetGPos 0, 25, 20, 17, 2
    VMJump L_008E

L_005F:
    VMStackPushFlag EVENT_FLAG_0x010a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x409d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_008E
    ActorSetGPos 0, 27, 20, 16, 1

L_008E:
    VMHalt

Script_1:
    WorkSetConst 0x8020, 0
    PlayerGetDir 0x8020
    Cmd_0269
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_00AF
    VMJump L_00BD

L_00AF:
    ActorCmdExec 255, Movement_0280
    VMJump L_00FF

L_00BD:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_00D0
    VMJump L_00DE

L_00D0:
    ActorCmdExec 255, Movement_02A0
    VMJump L_00FF

L_00DE:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_00F1
    VMJump L_00FF

L_00F1:
    ActorCmdExec 255, Movement_02C0
    VMJump L_00FF

L_00FF:
    ActorCmdWait
    ActorCmdExec 255, Movement_0318
    ActorCmdWait
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkGet 0x8021, 0x8000
    WorkGet 0x8022, 0x8001
    ActorCmdExec 255, Movement_0320
    ActorCmdWait
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_0148
    VMJump L_0156

L_0148:
    ActorCmdExec 255, Movement_02E0
    VMJump L_0198

L_0156:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_0169
    VMJump L_0177

L_0169:
    ActorCmdExec 255, Movement_02E8
    VMJump L_0198

L_0177:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_018A
    VMJump L_0198

L_018A:
    ActorCmdExec 255, Movement_02F0
    VMJump L_0198

L_0198:
    SEPlay SEQ_SE_GYM_M03
    ActorCmdWait
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    VMHalt

Script_3:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkGet 0x8023, 0x8000
    VMStackPush 0x8023
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E3
    MapChangeCore ZONE_CASTELIA_CITY_GYM_2, 15, 65535, 20, 0
    VMJump L_01EF

L_01E3:
    MapChangeCore ZONE_CASTELIA_CITY_GYM_2, 9, 65535, 13, 0

L_01EF:
    FadeInBlackQ
    FadeWait
    ActorCmdExec 255, Movement_02F8
    SEPlay SEQ_SE_GYM_M03
    ActorCmdWait
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    VMHalt

Script_4:
    ActorCmdExec 255, Movement_0320
    ActorCmdWait
    VMStackPushFlag EVENT_FLAG_0x0109
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023A
    Cmd_0268 0
    VMCall L_025A
    FlagSet EVENT_FLAG_0x0109

L_023A:
    ActorCmdExec 255, Movement_0300
    SEPlay SEQ_SE_GYM_M03
    ActorCmdWait
    VMHalt

Script_5:
    ActorCmdExec 255, Movement_030C
    SEPlay SEQ_SE_GYM_M03
    ActorCmdWait
    VMHalt

L_025A:
    SEPlay SEQ_SE_GYM_M04
    VMSleep 5
    SEPlay SEQ_SE_GYM_M04
    VMSleep 5
    SEPlay SEQ_SE_GYM_M04
    VMSleep 7
    SEPlay SEQ_SE_GYM_M05
    VMSleep 28
    SEPlay SEQ_SE_GYM_M06
    VMReturn

Movement_0280:
    Move 75, 1
    Move 40, 2
    Move 71, 1
    Move 73, 1
    Move 20, 2
    Move 72, 1
    Move 74, 1
    MoveEnd

Movement_02A0:
    Move 75, 1
    Move 42, 2
    Move 71, 1
    Move 73, 1
    Move 22, 2
    Move 72, 1
    Move 74, 1
    MoveEnd

Movement_02C0:
    Move 75, 1
    Move 43, 2
    Move 71, 1
    Move 73, 1
    Move 23, 2
    Move 72, 1
    Move 74, 1
    MoveEnd

Movement_02E0:
    Move 57, 1
    MoveEnd

Movement_02E8:
    Move 58, 1
    MoveEnd

Movement_02F0:
    Move 59, 1
    MoveEnd

Movement_02F8:
    Move 52, 1
    MoveEnd

Movement_0300:
    Move 70, 1
    Move 57, 1
    MoveEnd

Movement_030C:
    Move 70, 1
    Move 58, 1
    MoveEnd

Movement_0318:
    Move 69, 1
    MoveEnd

Movement_0320:
    Move 70, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    Cmd_0268 1
    ActorCmdExec 255, Movement_03B0
    VMCall L_025A
    ActorCmdWait
    FlagSet EVENT_FLAG_0x010a
    TrainerBGMPlayPush TRAINER_HARLEQUIN_JACK
    ActorCmdExec 0, Movement_03B8
    ActorCmdWait
    // "When the cocoon breaks open,\nthe one that pops out is--moi![f000]븁\u0000\nOn that note, have a battle with moi![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCityGym_Text_WhenCocoonBreaksOpen, 0, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_HARLEQUIN_JACK, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0385
    CallTrainerBattleEnd
    VMJump L_038B

L_0385:
    FlagReset EVENT_FLAG_0x010a
    CallTrainerLose

L_038B:
    // "Hiding makes battle instincts dull,\nyou know.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCityGym_Text_HidingMakesBattleInstincts, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_03C4
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x409d, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03B0:
    Move 75, 1
    MoveEnd

Movement_03B8:
    Move 13, 2
    Move 34, 1
    MoveEnd

Movement_03C4:
    Move 15, 2
    Move 12, 1
    Move 33, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hiding makes battle instincts dull,\nyou know."
    ActorMsg MSGFILE_SCRIPT, CasteliaCityGym_Text_HidingMakesBattleInstincts_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047E
    VMStackPushFlag EVENT_FLAG_0x006d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046A
    // "Clyde: Hello! You're probably\ntired from wandering all over[f000]븀\u0000\nthe crowded streets of Castelia City[f000]븀\u0000\nlooking for Team Plasma and[f000]븀\u0000\nthe Gym Leader.[f000]븁\u0000\nSo, here,\nI'll give you this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym_Text_ClydeHelloYoureProbably, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "The theme of this Gym\nis none other than cocoons![f000]븁\u0000\nYou head upward by going inside\nthe cocoons and traveling[f000]븀\u0000\nup the threads![f000]븁\u0000\nThe threads are definitely\nconnected to Burgh...eventually!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym_Text_ThemeGymNoneOther, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x006d
    VMJump L_0478

L_046A:
    // "The theme of this Gym\nis none other than cocoons![f000]븁\u0000\nYou head upward by going inside\nthe cocoons and traveling[f000]븀\u0000\nup the threads![f000]븁\u0000\nThe threads are definitely\nconnected to Burgh...eventually!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym_Text_ThemeGymNoneOther, 0, 0
    LastKeyWait
    ActorMsgClose

L_0478:
    VMJump L_048C

L_047E:
    // "Wow! That's amazing![f000]븁\u0000\nYou swatted aside our Gym Leader\nBurgh's tricky attacks...[f000]븁\u0000\nI can barely imagine how much\nstronger you're going to get!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym_Text_WowThatsAmazingSwatted, 0, 0
    LastKeyWait
    ActorMsgClose

L_048C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    TrainerCardHasBadge 0x8008, 2
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C5
    WordSetPlayerName 0
    // "Castelia Pokémon Gym[f000]븁\u0000\nGym Leader: Burgh\nCertified Trainers:"
    InfoMsg CasteliaCityGym_Text_CasteliaPokemonGymGym, 2
    VMJump L_04E8

L_04C5:
    VMStackPushFlag EVENT_FLAG_ARRIVED_NIMBASA_CITY
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E3
    // "Castelia Pokémon Gym[f000]븁\u0000\nGym Leader: Burgh\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000"
    InfoMsg CasteliaCityGym_Text_CasteliaPokemonGymGym_2, 2
    VMJump L_04E8

L_04E3:
    // "Castelia Pokémon Gym[f000]븁\u0000\nGym Leader: Burgh\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg CasteliaCityGym_Text_CasteliaPokemonGymGym_3, 2

L_04E8:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
