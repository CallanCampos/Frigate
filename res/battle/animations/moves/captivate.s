// MOVE_CAPTIVATE's animation: member 445 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Captivate

Captivate:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W445_YUUWAKU, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W445_YUUWAKU, 1, EFFVM_ATTACKER, 30, 0, 30, 0, 0, 0
    PlaySE SEQ_SE_W113_02, 2, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 624
    EmitterAdd 624, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 624, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssRotate EFFVM_ATTACKER, 3, 0xaaaa00, 4, 1, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
