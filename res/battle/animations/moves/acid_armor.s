// MOVE_ACID_ARMOR's animation: member 151 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AcidArmor

AcidArmor:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W142_01, 1, EFFVM_ATTACKER, 0, 300, 125, 127, 120, 0
    MoveSE 1, 0, 0, 1000, -1000, 0, 70, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 2, EFFVM_ATTACKER, 40, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 35, 50, 0, 0
    SetYield TRUE
    ParticleLoad 319
    EmitterAdd 319, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetValue EFFVM_ATTACKER, 1
    McssPalFade EFFVM_ATTACKER, 0, 16, 1, 0x7fff
    McssSetValue EFFVM_ATTACKER, 1
    McssAlpha EFFVM_ATTACKER, 1, 2, 20, 1, 0
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 30, 2, 1
    Wait EFFVM_WAIT_PARTICLES
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 16, 0, 1, 0x7fff
    McssAlpha EFFVM_ATTACKER, 1, 31, 20, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
