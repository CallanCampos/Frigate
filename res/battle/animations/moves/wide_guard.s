// MOVE_WIDE_GUARD's animation: member 469 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WideGuard

WideGuard:
    CameraMovePreset 1, 21, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_FLD_112, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_ATTACKER, 0, -500, 60, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_ATTACKER, 90, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 150, 0, 0, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 3, 0x1800, 0, 2, 0, 7
    ParticleLoad 649
    EmitterAdd 649, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 649, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 80
    EmitterAdd 649, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
