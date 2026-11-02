// MOVE_CHIP_AWAY's animation: member 498 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ChipAway

ChipAway:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    PlaySE SEQ_SE_W075_06, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 12, 0, 0x7fff
    McssCircle EFFVM_ATTACKER, 2, 0, 0x1000, 0x4000, 0xc000, 0, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    ParticleLoad 670
    EmitterAdd 670, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 670, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 2, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
