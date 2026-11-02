// MOVE_U_TURN's animation: member 369 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant UTurn

UTurn:
    CameraMovePreset 1, 9, 10, 0, 6
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W295_02, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 2, EFFVM_DEFENDER, 10, 0, 115, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 5, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W289_01, 1, EFFVM_ATTACKER, 45, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 40, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 35, 0, 100, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 85, 0, 115, 0, 0, 0
    SetYield TRUE
    ParticleLoad 547
    EmitterAdd 547, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterMove 547, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 1
    CameraMovePreset 1, 11, 10, 0, 6
    WaitFrames 19
    EmitterAdd 547, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterMove 547, 2, 2, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x2000, 0x32000, 0x6000, 0x1000, 0x1000, 0
    EmitterAdd 547, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssCircle EFFVM_DEFENDER, 2, 2, 0, 0x2000, 0x6000, 0x2000, 0x1000, 0
    WaitFrames 16
    CameraMovePreset 1, 9, 10, 0, 6
    EmitterAdd 547, 0, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 50
    McssSetMode EFFVM_ATTACKER, 4
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
