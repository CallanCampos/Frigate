// Effect BTLV_EFFECT_FIRST + 82 (0x283): member 82 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect082

Effect082:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W295_02, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 20, -800, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 548
    EmitterAdd 548, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssSetMode EFFVM_ATTACKER, 3
    EmitterAdd 548, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER_MIRRORED, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
