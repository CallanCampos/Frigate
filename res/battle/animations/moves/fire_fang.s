// MOVE_FIRE_FANG's animation: member 424 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FireFang

FireFang:
    CameraMovePreset 1, 11, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 20, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 25, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 603
    EmitterAdd 603, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 603, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 603, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 603, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 603, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 603, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
