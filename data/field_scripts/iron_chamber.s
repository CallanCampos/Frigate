#include "asm/field_script.inc"
#include "text/script/iron_chamber.h"

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
    // "Iron Chamber"
    InfoMsg IronChamber_Text_IronChamber, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It protects this place\nwith the power of steel."
    InfoMsg IronChamber_Text_ProtectsPlacePowerSteel, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The Pokémon statue that exudes the\npower of steel started moving![f000]븁\u0000"
    SystemMsg IronChamber_Text_PokemonStatueExudesPower, 2
    InfoMsgClose
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_00BF
    VMJump L_00CD

L_00BF:
    ActorCmdExec 0, Movement_01D4
    VMJump L_0130

L_00CD:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_00E0
    VMJump L_00EE

L_00E0:
    ActorCmdExec 0, Movement_01CC
    VMJump L_0130

L_00EE:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0101
    VMJump L_010F

L_0101:
    ActorCmdExec 0, Movement_01E4
    VMJump L_0130

L_010F:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0122
    VMJump L_0130

L_0122:
    ActorCmdExec 0, Movement_01DC
    VMJump L_0130

L_0130:
    ActorCmdWait
    PVPlay 379, 0
    // "Ji-ji-ze-ji-zoh..."
    ScreamMsg IronChamber_Text_JiJiZeJi, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 379, 65, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0172
    FlagSet EVENT_FLAG_0x039b
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
    // "Registeel disappeared deep\ninto the ruins..."
    SystemMsg IronChamber_Text_RegisteelDisappearedDeepInto, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_01C5

L_01A8:
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_01BB
    VMJump L_01C5

L_01BB:
    FlagSet EVENT_FLAG_0x0191
    VMJump L_01C5

L_01C5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01CC:
    Move 0, 1
    MoveEnd

Movement_01D4:
    Move 1, 1
    MoveEnd

Movement_01DC:
    Move 2, 1
    MoveEnd

Movement_01E4:
    Move 3, 1
    MoveEnd
