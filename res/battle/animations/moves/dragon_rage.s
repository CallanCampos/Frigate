// MOVE_DRAGON_RAGE's animation: member 82 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DragonRage

DragonRage:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W225_02, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 3000, 0, 84, 0, 0
    MoveSE 1, 0, 1, 110, 0, 70, 10, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 2, EFFVM_DEFENDER, 40, 1000, 120, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 1, EFFVM_DEFENDER, 45, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 1, EFFVM_DEFENDER, 55, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 1, EFFVM_DEFENDER, 65, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 1, EFFVM_DEFENDER, 75, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 2, EFFVM_DEFENDER, 85, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 250
    EmitterMove 250, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 250, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 250, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 10
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    EmitterAdd 250, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
