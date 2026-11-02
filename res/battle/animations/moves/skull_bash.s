// MOVE_SKULL_BASH's animation: member 130 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant SkullBash
    ScriptVariant SkullBash_Variant1

SkullBash:
    CameraMovePreset 1, 9, 16, 0, 8
    McssMoveXY EFFVM_ATTACKER, 1, -0x1000, 0, 6, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, -500, 110, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    WaitFrames 30
    McssRotate EFFVM_ATTACKER, 2, 0xff8e3955, 6, 2, 2
    SetYield FALSE
    PlaySE SEQ_SE_W050_01, 1, EFFVM_ATTACKER, 0, 700, 120, 0, 0, 0
    PlaySE SEQ_SE_W050_01, 1, EFFVM_ATTACKER, 40, 700, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 1, 0x1000, 0, 6, 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, -500, 110, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    WaitFrames 30
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End

SkullBash_Variant1:
    PaletteFade 4, 0, 13, 2, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 2, 1, 0, 0x3000, 0xa000, 0, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 299
    EmitterAdd 299, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 299, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 299, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 0, 9
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 13, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
