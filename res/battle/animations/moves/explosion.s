// MOVE_EXPLOSION's animation: member 153 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Explosion

Explosion:
    CameraMovePreset 1, 9, 16, 0, 8
    LoadBg 0
    SetVanish 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -2000, 0, 60, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 80, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 65, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 180, 0, 0, 0, 0, 0
    MoveSE 3, 0, 1, 110, 0, 140, 40, 0, 0
    SetYield TRUE
    ParticleLoad 321
    EmitterAdd 321, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 321, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 321, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 321, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 321, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 321, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 321, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 70
    PaletteFade 2, 0, 13, 2, 32767
    WaitFrames 4
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    SetVanish 0, 1
    SetVanish 1, 1
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x1f
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 10, 2, 1
    CameraShake 0, 0x4cd, 0, 2, 0, 12
    WaitFrames 70
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 2, 13, 0, 3, 32767
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x1f
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
