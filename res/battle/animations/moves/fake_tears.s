// MOVE_FAKE_TEARS's animation: member 313 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FakeTears

FakeTears:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W310_02, 1, EFFVM_ATTACKER, 0, 100, 120, 0, 0, 0
    PlaySE SEQ_SE_W310_02, 2, EFFVM_ATTACKER, 25, 100, 120, 0, 0, 0
    PlaySE SEQ_SE_W310_02, 2, EFFVM_ATTACKER, 50, 100, 120, 0, 0, 0
    SetYield TRUE
    McssRotate EFFVM_ATTACKER, 3, 0xaaaa00, 3, 1, 3
    ParticleLoad 488
    EmitterAdd 488, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 488, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
