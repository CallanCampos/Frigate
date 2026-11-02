// MOVE_GROWTH's animation: member 74 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Growth

Growth:
    CameraMovePreset 1, 9, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x33f6
    McssScale EFFVM_ATTACKER, 3, -0x19a, -0x333, 4, 1, 1
    ParticleLoad 241
    EmitterAdd 241, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 241, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W099_01, 1, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W074_01, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    MoveSE 1, 0, 0, 1000, 5000, 0, 40, 0, 0
    MoveSE 1, 0, 1, 120, 0, 30, 10, 0, 0
    SetYield TRUE
    WaitFrames 22
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x33f6
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
