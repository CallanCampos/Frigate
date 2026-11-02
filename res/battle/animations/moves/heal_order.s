// MOVE_HEAL_ORDER's animation: member 456 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HealOrder

HealOrder:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W454_MUSHI, 1, EFFVM_ATTACKER, 0, 200, 110, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 90, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_FLD_29, 2, EFFVM_ATTACKER, 10, 70, 80, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 60, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 60, 0, 0
    SetYield TRUE
    ParticleLoad 635
    EmitterAdd 635, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 635, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 635, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xa66, 0x1000
    EmitterAdd 635, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xa66, 0x1000
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 20, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 40, 0, 60, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 12, 2, 0x53d4
    EmitterAdd 635, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_ATTACKER, 12, 0, 2, 0x53d4
    Wait EFFVM_WAIT_ALL
    End
