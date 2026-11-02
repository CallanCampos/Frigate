// MOVE_GROWL's animation: member 45 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Growl

Growl:
    PlayCry EFFVM_ATTACKER, 26624, 0, 0, 0, 1, 0
    PlayCry EFFVM_ATTACKER, -44, 0, 0, 0, 0, 14
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 210
    EmitterAdd 210, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 210, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_CRY
    End
