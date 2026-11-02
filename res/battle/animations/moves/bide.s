// MOVE_BIDE's animation: member 117 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Bide
    ScriptVariant Bide_Variant1

Bide:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 10, 4, 0x1f
    ParticleLoad 287
    EmitterAdd 287, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 287, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 0, 1000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 10, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 12, 1000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 22, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 24, 1000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 34, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 1, EFFVM_ATTACKER, 36, 1000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 46, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 1000, 5000, 20, 100, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_ATTACKER, 0, 110, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 4, 0x1f
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

Bide_Variant1:
    ChangeScript MOVE_TACKLE
