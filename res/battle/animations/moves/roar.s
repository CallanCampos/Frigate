// MOVE_ROAR's animation: member 46 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Roar

Roar:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0, 0x333, 3, 0, 6
    PlayCry EFFVM_ATTACKER, -128, 127, 0, 0, 0, 0
    PlayCry EFFVM_ATTACKER, 60, 127, 0, 0, 0, 12
    ParticleLoad 211
    EmitterAdd 211, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterMove 211, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0xb4000, 0, 0x1000, 0x1000, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 1, 8
    CameraMovePreset 1, 11, 26, 0, 22
    WaitFrames 26
    McssMoveXY EFFVM_DEFENDER, 1, -0x1e000, 0, 16, 1, 1
    Wait EFFVM_WAIT_ALL
    Gauge 2, 4
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_CRY
    End
