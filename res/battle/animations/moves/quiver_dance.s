// MOVE_QUIVER_DANCE's animation: member 483 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant QuiverDance

QuiverDance:
    SetYield FALSE
    PlaySE SEQ_SE_FLD_112, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 50, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W258_ARARE, 2, EFFVM_ATTACKER, 25, 0, 100, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 8, 2, 0
    McssCircle EFFVM_ATTACKER, 2, 2, 0x2000, 0x2000, 0xa000, 0, 0x3000, 0
    WaitFrames 10
    ParticleLoad 658
    EmitterAdd 658, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 658, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 658, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
