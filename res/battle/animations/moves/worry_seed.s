// MOVE_WORRY_SEED's animation: member 388 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WorrySeed

WorrySeed:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W088_01, 1, EFFVM_DEFENDER, 0, -200, 80, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 40, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W165_01, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    MoveSE 1, 0, 0, 0, -500, 41, 40, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 50, 0, 0
    SetYield TRUE
    ParticleLoad 567
    EmitterMove 567, 1, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 567, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 8
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
