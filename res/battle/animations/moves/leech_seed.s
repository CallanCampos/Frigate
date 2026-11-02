// MOVE_LEECH_SEED's animation: member 73 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LeechSeed

LeechSeed:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 0, 300, 120, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 10, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_ATTACKER, 14, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 28, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_ATTACKER, 42, 300, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 24, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 14, 24, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 28, 24, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 42, 24, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W331_01, 2, EFFVM_DEFENDER, 100, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_DEFENDER, 110, -200, 90, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_DEFENDER, 120, -400, 40, 0, 0, 0
    SetYield TRUE
    ParticleLoad 238
    ParticleLoad 239
    ParticleLoad 240
    EmitterMoveOrtho 238, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x2d000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMoveOrtho 238, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x2d000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMoveOrtho 238, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x2d000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMoveOrtho 238, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x2d000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 10
    EmitterAdd 239, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 240, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 239, 0, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 239, 2, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 5
    ParticleDelete 238
    ParticleLoad 238
    EmitterAdd 238, 0, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 238, 1, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 5
    EmitterAdd 240, 0, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 240, 2, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
