// MOVE_COIL's animation: member 489 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Coil

Coil:
    CameraMovePreset 1, 9, 12, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 664
    SetYield FALSE
    PlaySE SEQ_SE_FLD_112, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_FLD_112, 2, EFFVM_ATTACKER, 70, 500, 127, 0, 0, 0
    MoveSE 2, 0, 0, 500, 2000, 71, 89, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 150, 0, 0, 0, 0, 0
    SetYield TRUE
    EmitterAdd 664, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 664, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 664, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 664, 5, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 664, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 664, 4, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 664, 6, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    McssScale EFFVM_ATTACKER, 2, 0x19a, -0x19a, 40, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
