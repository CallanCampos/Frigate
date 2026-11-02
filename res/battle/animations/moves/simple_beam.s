// MOVE_SIMPLE_BEAM's animation: member 493 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SimpleBeam

SimpleBeam:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 12, 1, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 8, 2, 0x7fff
    McssScale EFFVM_ATTACKER, 2, 0x19a, -0x19a, 2, 0, 6
    ParticleLoad 667
    EmitterMove 667, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x46000, 0, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, -300, 90, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 20, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    SetYield TRUE
    WaitFrames 2
    EmitterMove 667, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x46000, 0, 0x1000, 0x1000, 0
    WaitFrames 2
    EmitterMove 667, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x46000, 0, 0x1000, 0x1000, 0
    WaitFrames 2
    EmitterMove 667, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x46000, 0, 0x1000, 0x1000, 0
    WaitFrames 2
    EmitterMove 667, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x46000, 0, 0x1000, 0x1000, 0
    WaitFrames 2
    McssPalFade EFFVM_ATTACKER, 8, 0, 2, 0x7fff
    EmitterMove 667, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x46000, 0, 0x1000, 0x1000, 0
    WaitFrames 2
    EmitterMove 667, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x46000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 10, 0, 8
    WaitFrames 15
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 667, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_FLD_114, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 15
    EmitterAdd 667, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 8, 2, 0x7fff
    WaitFrames 15
    EmitterAdd 667, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 5
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 3
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 8, 0, 2, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
