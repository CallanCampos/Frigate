// MOVE_SNATCH's animation: member 289 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Snatch
    ScriptVariant Snatch_Variant1

Snatch:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssCircle EFFVM_ATTACKER, 2, 1, 0x4cd, 0x800, 0x1e000, 0, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, -1000, 115, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End

Snatch_Variant1:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 1, EFFVM_DEFENDER, 40, 500, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 35, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 60, 0, 115, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W289_01, 2, EFFVM_PAN_CENTER, 70, 0, 115, 0, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 65, 30, 0, 0
    SetYield TRUE
    ParticleLoad 460
    EmitterAdd 460, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 460, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 460, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 28
    CameraMovePreset 1, 11, 16, 0, 12
    EmitterMove 460, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 19
    CameraMovePreset 1, 8, 16, 0, 12
    EmitterMove 460, 1, 2, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x1000, 0xe000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 16
    McssSetMode EFFVM_ATTACKER, 4
    Wait EFFVM_WAIT_ALL
    End
