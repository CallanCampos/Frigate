// MOVE_POWER_TRICK's animation: member 379 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PowerTrick

PowerTrick:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W325_BUNN, 1, EFFVM_ATTACKER, 0, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W325_BUNN, 1, EFFVM_ATTACKER, 30, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W379_POWER, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 40, 20, 0, 0
    SetYield TRUE
    ParticleLoad 558
    EmitterAdd 558, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 558, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 558, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    WaitFrames 0
    Wait EFFVM_WAIT_ALL
    End
