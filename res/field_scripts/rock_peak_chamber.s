#include "asm/field_script.inc"
#include "text/script/rock_peak_chamber.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0048
    VMSleep 4
    FadeOutBlack
    ActorCmdWait
    FadeWait
    RTReserveScript 5
    MapChangeCore ZONE_UNDERGROUND_RUINS, 15, 0, 0, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0048:
    Move 9, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    FadeInBlack
    ActorCmdExec 255, Movement_0068
    ActorCmdWait
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0068:
    Move 8, 2
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Rock Peak Chamber"
    InfoMsg RockPeakChamber_Text_RockPeakChamber, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It protects this place\nwith the power of rock."
    InfoMsg RockPeakChamber_Text_ProtectsPlacePowerRock, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The Pokémon statue that exudes the\npower of rock started moving![f000]븁\u0000"
    SystemMsg RockPeakChamber_Text_PokemonStatueExudesPower, 2
    InfoMsgClose
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_00BF
    VMJump L_00CD

L_00BF:
    ActorCmdExec 0, Movement_01F8
    VMJump L_0130

L_00CD:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_00E0
    VMJump L_00EE

L_00E0:
    ActorCmdExec 0, Movement_01F0
    VMJump L_0130

L_00EE:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0101
    VMJump L_010F

L_0101:
    ActorCmdExec 0, Movement_0208
    VMJump L_0130

L_010F:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0122
    VMJump L_0130

L_0122:
    ActorCmdExec 0, Movement_0200
    VMJump L_0130

L_0130:
    ActorCmdWait
    PVPlay 377, 0
    // "Zaza zari za..."
    ScreamMsg RockPeakChamber_Text_ZazaZariZa, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 377, 65, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0172
    FlagSet EVENT_FLAG_0x039a
    ActorDelete 0x8011
    CallWildBattleEnd
    VMJump L_0174

L_0172:
    CallWildLose

L_0174:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0198
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0198
    VMJump L_01A8

L_0198:
    // "Regirock disappeared deep\ninto the ruins..."
    SystemMsg RockPeakChamber_Text_RegirockDisappearedDeepInto, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_01EA

L_01A8:
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_01BB
    VMJump L_01EA

L_01BB:
    FlagSet EVENT_FLAG_0x0190
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E0
    CallUnovaLinkKeyUnlock 3
    VMJump L_01E4

L_01E0:
    CallUnovaLinkKeyUnlock 4

L_01E4:
    VMJump L_01EA

L_01EA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_01F0:
    Move 0, 1
    MoveEnd

Movement_01F8:
    Move 1, 1
    MoveEnd

Movement_0200:
    Move 2, 1
    MoveEnd

Movement_0208:
    Move 3, 1
    MoveEnd
