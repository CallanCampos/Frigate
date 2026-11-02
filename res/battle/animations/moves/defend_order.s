// MOVE_DEFEND_ORDER's animation: member 455 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DefendOrder

DefendOrder:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W454_MUSHI, 1, EFFVM_ATTACKER, 0, 300, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 65, 15, 0, 0
    PlaySE SEQ_SE_FLD_29, 2, EFFVM_ATTACKER, 10, 70, 70, 0, 0, 0
    SetYield TRUE
    ParticleLoad 634
    EmitterAdd 634, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 634, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 634, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xa66, 0x1000
    EmitterAdd 634, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0xa66, 0x1000
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_FLD_103, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 15, 40, 0, 0
    PlaySE SEQ_SE_FLD_103, 1, EFFVM_ATTACKER, 5, -50, 60, 0, 0, 0
    MoveSE 1, 0, 1, 60, 0, 20, 40, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 12, 2, 0x1e
    EmitterAdd 634, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 2, 0x1e
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
