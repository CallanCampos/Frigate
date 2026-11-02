// MOVE_ENDURE's animation: member 203 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Endure

Endure:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_CAMERA
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 10, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 12, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 22, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 24, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 34, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 36, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 46, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 48, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 58, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 20, 100, 0, 0
    SetYield TRUE
    ParticleLoad 373
    EmitterAdd 373, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 0, 10, 3, 0x1f
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 1, 1, 4
    McssScale EFFVM_ATTACKER, 2, 0x19a, -0x19a, 10, 1, 1
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 3, 0x1f
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
