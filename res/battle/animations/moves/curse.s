// MOVE_CURSE's animation: member 174 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Curse
    ScriptVariant Curse_Variant1

Curse:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 20, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 40, 200, 110, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 60, 300, 110, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 80, 400, 110, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 1000, 0, 90, 0, 0
    SetYield TRUE
    ParticleLoad 344
    EmitterAdd 344, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x1f
    McssMoveXY EFFVM_ATTACKER, 2, 0x1800, 0, 10, 1, 2
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x1f
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

Curse_Variant1:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W014_03, 2, EFFVM_ATTACKER, 0, -300, 110, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 1, EFFVM_ATTACKER, 5, -1700, 120, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 0, 20, 0, 0
    PlaySE SEQ_SE_W014_03, 2, EFFVM_ATTACKER, 25, -300, 110, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 1, EFFVM_ATTACKER, 30, -1700, 120, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 0, 20, 0, 0
    PlaySE SEQ_SE_W014_03, 2, EFFVM_ATTACKER, 50, -300, 110, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 1, EFFVM_ATTACKER, 55, -1700, 120, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 0, 20, 0, 0
    SetYield TRUE
    ParticleLoad 345
    EmitterAdd 345, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 345, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 345, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 345, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
