// MOVE_COTTON_GUARD's animation: member 538 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant CottonGuard

CottonGuard:
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 1, EFFVM_ATTACKER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W075_01, 2, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    MoveSE 2, 0, 0, -500, 1000, 0, 30, 0, 0
    MoveSE 2, 0, 1, 127, 0, 20, 10, 0, 0
    PlaySE SEQ_SE_FLD_54, 1, EFFVM_ATTACKER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_54, 2, EFFVM_ATTACKER, 62, 100, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_54, 1, EFFVM_ATTACKER, 70, 200, 100, 0, 0, 0
    PlaySE SEQ_SE_FLD_54, 2, EFFVM_ATTACKER, 78, 300, 90, 0, 0, 0
    PlaySE SEQ_SE_FLD_54, 1, EFFVM_ATTACKER, 84, 400, 60, 0, 0, 0
    PlaySE SEQ_SE_FLD_54, 2, EFFVM_ATTACKER, 89, 500, 50, 0, 0, 0
    PlaySE SEQ_SE_FLD_54, 1, EFFVM_ATTACKER, 94, 600, 40, 0, 0, 0
    PlaySE SEQ_SE_FLD_54, 2, EFFVM_ATTACKER, 99, 700, 30, 0, 0, 0
    SetYield TRUE
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 707
    EmitterAdd 707, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 707, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 707, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 707, 2, EFFVM_EMIT_ATTACKER, 8, 0x800, 0, 0, 0x99a, 0x1000, 0xb33, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
