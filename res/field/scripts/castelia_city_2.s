#include "asm/field_script.inc"
#include "text/script/castelia_city_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_3:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004D
    ActorCmdExec 0, Movement_04EC
    VMJump L_0055

L_004D:
    ActorCmdExec 0, Movement_04E4

L_0055:
    ActorCmdWait
    ActorCmdExec 0, Movement_04F4
    ActorCmdWait
    BGMPlay SEQ_BGM_E_ACHROMA
    // "???: Oh, it's you again![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_OhItsAgain, 0, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00A8
    ActorCmdExec 255, Movement_04E4
    VMJump L_00D3

L_00A8:
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00D3
    ActorCmdExec 255, Movement_04EC

L_00D3:
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FA
    WorkSub 0x8021, 1
    VMJump L_0100

L_00FA:
    WorkAdd 0x8021, 1

L_0100:
    ActorWalkRoute 0, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0137
    ActorCmdExec 0, Movement_04EC
    VMJump L_013F

L_0137:
    ActorCmdExec 0, Movement_04E4

L_013F:
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    // "If it's not an inconvenience,\nmay I have a look at your Pokémon?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_IfItsNotInconvenience, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_017C
    // "???: I appreciate your cooperation![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_AppreciateCooperation, 0, 0, 0
    VMJump L_0188

L_017C:
    // "???: Are you sure?\nBut this is for science![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_SureButScience, 0, 0, 0

L_0188:
    WorkSetConst 0x8023, 0
    PokePartyGetMemberByType 0x8023, 2
    WordSetPartyPokeSpecies 0, 0x8023
    // "Oh![f000]븁\u0000\nHow interesting![f000]븁\u0000\nYour [f000]ā\u0001\u0000 seems to display\nmore self-confidence than others[f000]븀\u0000\nof the same species.[f000]븁\u0000\nAnd you're a Trainer with\nmerely three Badges...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_OhHowInterestingSeems, 0, 0, 0
    MsgWinCloseAll
    VMSleep 8
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01DC
    ActorCmdExec 0, Movement_04EC
    VMJump L_0205

L_01DC:
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FD
    ActorCmdExec 0, Movement_04AC
    VMJump L_0205

L_01FD:
    ActorCmdExec 0, Movement_04A4

L_0205:
    ActorCmdWait
    // "Fantastic![f000]븁\u0000\nI'm not sure how you're doing it,\nbut you're bringing out[f000]븀\u0000\nthe power of your Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_FantasticImNotSure, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0250
    ActorCmdExec 0, Movement_04F4
    ActorCmdWait
    ActorCmdExec 0, Movement_04E4
    VMJump L_0279

L_0250:
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0271
    ActorCmdExec 0, Movement_0460
    VMJump L_0279

L_0271:
    ActorCmdExec 0, Movement_0454

L_0279:
    ActorCmdWait
    // "Oh, excuse me! I am a scientist.\nMy name is Colress.[f000]븁\u0000\nThe theme of my research is:\n“Bringing out the[f000]븀\u0000\npower of Pokémon.\"[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_OhExcuseAmScientist, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02BA
    ActorCmdExec 0, Movement_04EC
    VMJump L_02E3

L_02BA:
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DB
    ActorCmdExec 0, Movement_0474
    VMJump L_02E3

L_02DB:
    ActorCmdExec 0, Movement_046C

L_02E3:
    ActorCmdWait
    // "Bringing out the power of Pokémon![f000]븁\u0000\nIs it possible to bring out their\nmaximum power through the bond[f000]븀\u0000\nthey share with their Trainers?[f000]븁\u0000\nOr is there some other,\ndifferent method?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_BringingOutPowerPokemon, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0324
    ActorCmdExec 0, Movement_04E4
    VMJump L_034D

L_0324:
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0345
    ActorCmdExec 0, Movement_04A4
    VMJump L_034D

L_0345:
    ActorCmdExec 0, Movement_04AC

L_034D:
    ActorCmdWait
    // "I'd like to test my theory\nby battling with you.[f000]븀\u0000\nDo you find this acceptable?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_IdLikeTestTheory, 0, 0, 0
    YesNoWin 0x8010
    // "Either way, I'll be waiting on\nRoute 4. It's just beyond here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_EitherWayIllWaiting, 0, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8023, 0
    ActorWalkRoute 0, 15, 1, 0, 8, 0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40b4, 2
    FlagSet EVENT_FLAG_0x02f4
    FlagReset EVENT_FLAG_0x02f6
    WorkSetConst EVENT_WORK_0x40b6, 1
    FlagSet EVENT_FLAG_0x03dc
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Castelia City, Central Plaza\nAhead: Route 4"
    MsgPlaceSign CasteliaCity2_Text_CasteliaCityCentralPlaza, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Ahead: Mode Street\nCasteliacones and Studio Castelia"
    MsgPlaceSign CasteliaCity2_Text_AheadModeStreetCasteliacones, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0994
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0437
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've got some advice for you![f000]븁\u0000\nIf you want to become strong,\nbattle lots of Trainers[f000]븀\u0000\nand know your Pokémon well![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_IveGotSomeAdvice, 0, 0
    MsgWinCloseAll
    Cmd_0275 0, 7, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink!"
    SystemMsg CasteliaCity2_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "If you keep on battling,\nyou'll get stronger someday!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_IfKeepBattlingYoull, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0994
    VMJump L_044B

L_0437:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you keep on battling,\nyou'll get stronger someday!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity2_Text_IfKeepBattlingYoull, 0, 0
    LastKeyWait
    ActorMsgClose

L_044B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0454:
    Move 75, 1
    Move 14, 1
    MoveEnd

Movement_0460:
    Move 75, 1
    Move 15, 1
    MoveEnd

Movement_046C:
    Move 11, 1
    MoveEnd

Movement_0474:
    Move 10, 1
    MoveEnd
    VMStackDiv
    VMHalt
    VMStackAdd
    VMStackAdd
    PokePartyGetSpecies 0, 14
    VMHalt
    Move 12, 12
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_04A4:
    Move 15, 1
    MoveEnd

Movement_04AC:
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
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_04E4:
    Move 34, 1
    MoveEnd

Movement_04EC:
    Move 35, 1
    MoveEnd

Movement_04F4:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
