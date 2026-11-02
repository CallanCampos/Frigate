// MOVE_MIND_READER's animation: member 170 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MindReader

MindReader:
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 13, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W113_02, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W295_02, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 341
    EmitterAdd 341, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 341, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 2
    EmitterAdd 341, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 2
    Wait EFFVM_WAIT_PARTICLES
    WaitFrames 20
    PaletteFade 4, 13, 0, 0, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
