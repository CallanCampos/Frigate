// MOVE_WORK_UP's animation: member 526 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WorkUp

WorkUp:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, -700, 100, 0, 0, 0
    MoveSE 1, 0, 1, 100, 0, 40, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W077_03, 2, EFFVM_ATTACKER, 50, -300, 127, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 8, 4, 0x1ff
    ParticleLoad 696
    EmitterAdd 696, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1800, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x19a, -0x19a, 2, 0, 6
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 8, 0, 4, 0x1ff
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
