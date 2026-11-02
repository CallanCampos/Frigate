// MOVE_STRING_SHOT's animation: member 81 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant StringShot

StringShot:
    CameraMovePreset 1, 9, 12, 0, 8
    PaletteFade 2, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W075_01, 1, EFFVM_ATTACKER, 0, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 50, 1500, 70, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 90, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 2, EFFVM_DEFENDER, 90, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    ParticleLoad 249
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 249, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    WaitFrames 1
    CameraMovePreset 1, 11, 10, 0, 6
    EmitterAdd 249, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 249, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 249, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x19a, 18, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 2, 13, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
