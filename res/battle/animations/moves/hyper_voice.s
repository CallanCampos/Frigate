// MOVE_HYPER_VOICE's animation: member 304 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HyperVoice

HyperVoice:
    CameraMovePreset 1, 9, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    PlayCry EFFVM_ATTACKER, 34304, 127, 60, -64, 1, 0
    PlayCry EFFVM_ATTACKER, 44, 127, 60, 16, 0, 16
    ParticleLoad 474
    PaletteFade 4, 0, 6, 3, 990
    EmitterMove 474, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 10, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_FLD_114, 2, EFFVM_DEFENDER, 0, 200, 80, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 2, 4
    EmitterAdd 474, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 6, 0, 3, 990
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 12, 0, 8
    Wait EFFVM_WAIT_CRY
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
