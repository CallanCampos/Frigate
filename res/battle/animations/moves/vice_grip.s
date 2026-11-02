// MOVE_VICE_GRIP's animation: member 11 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ViceGrip

ViceGrip:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 175
    EmitterAdd 175, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 175, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 175, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
