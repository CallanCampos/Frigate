// MOVE_STEALTH_ROCK's animation: member 446 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant StealthRock

StealthRock:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_FLD_81, 1, EFFVM_ATTACKER, 15, 0, 80, 0, 0, 0
    MoveSE 1, 0, 1, 80, 0, 35, 70, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 60, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 65, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 70, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 70, 0, 0
    SetYield TRUE
    ParticleLoad 625
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, StealthRock_0340
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 1, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 1, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 1, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 1, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    CameraMovePreset 1, 1, 16, 0, 8
    WaitFrames 3
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 1, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 1, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 1, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 4
    EmitterAdd 625, 0, 1, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 625, 1, 1, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Jump StealthRock_051E

StealthRock_0340:
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 0, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 0, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 0, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 0, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    CameraMovePreset 1, 0, 16, 0, 8
    WaitFrames 3
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 0, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 0, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 625, 2, 2, EFFVM_EMIT_ATTACKER, 0, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 4
    EmitterAdd 625, 0, 0, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 625, 1, 0, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

StealthRock_051E:
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
