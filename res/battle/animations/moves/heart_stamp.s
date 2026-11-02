// MOVE_HEART_STAMP's animation: member 531 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HeartStamp

HeartStamp:
    SetYield FALSE
    PlaySE SEQ_SE_W075_05, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W006_03, 2, EFFVM_DEFENDER, 25, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_03, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 700
    EmitterAdd 700, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 700, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 700, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 700, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
