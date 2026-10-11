#include "asm/field_script.inc"
#include "text/script/driftveil_city_gym_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_2:
    ActorsPauseAll
    Cmd_0187 0
    FadeInBlack
    Cmd_018E 0
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    ActorNew 7, 4, 1, 251, 227, 0
    Cmd_0187 1
    FadeInBlack
    FadeWait
    Cmd_018E 2
    WorkSetConst EVENT_WORK_0x4000, 1
    ActorCmdExec 251, Movement_01A0
    VMSleep 20
    ActorCmdExec 255, Movement_01A8
    ActorCmdWait
    // "Clay: Good dancers are crucial\nfer puttin' on a good show![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym2_Text_ClayGoodDancersCrucial, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0168
    VMSleep 20
    ActorCmdExec 255, Movement_0198
    ActorCmdWait
    ActorDelete 251
    WorkSetConst EVENT_WORK_0x40c3, 4
    FlagReset EVENT_FLAG_0x02cd
    ObjInitPointGPos 2, 31, 0, 0
    HollowRivalCmd_0262 1, 11
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    PlayerGetDir 0x8010
    DebugPrint EVENT_WORK_0x4000
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0105
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    SEPlay SEQ_SE_FLD_61
    // "[f000]Ā\u0001\u0000 pressed the\nswitch on the elevator!"
    InfoMsg DriftveilCityGym2_Text_PressedSwitchElevator, 2
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    FadeOutBlack
    Cmd_018E 1
    FadeWait
    RTReserveScript 2
    MapChangeCore ZONE_DRIFTVEIL_CITY_GYM, 12, 0, 87, 0

L_0105:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    TrainerCardHasBadge 0x8008, 4
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013B
    // "Driftveil Pokémon Gym[f000]븁\u0000\nGym Leader: Clay\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0001"
    InfoMsg DriftveilCityGym2_Text_DriftveilPokemonGymGym, 2
    VMJump L_0140

L_013B:
    // "Driftveil Pokémon Gym[f000]븁\u0000\nGym Leader: Clay\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg DriftveilCityGym2_Text_DriftveilPokemonGymGym_2, 2

L_0140:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Driftveil Pokémon Gym![f000]븁\u0000\nIn this Gym, elevators are provided for\nyour use."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym2_Text_WelcomeDriftveilPokemonGym, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0168:
    Move 13, 10
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
    Move 32, 1
    MoveEnd

Movement_0198:
    Move 33, 1
    MoveEnd

Movement_01A0:
    Move 34, 1
    MoveEnd

Movement_01A8:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
