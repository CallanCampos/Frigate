// MOVE_IRON_DEFENSE's animation: member 334 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant IronDefense

IronDefense:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 1, 32767
    PlaySE SEQ_SE_W095_01, 0, EFFVM_ATTACKER, 10, 0, 80, 0, 0, 0
    WaitFrames 22
    PaletteFade 4, 12, 0, 1, 32767
    WaitFrames 22
    PaletteFade 4, 0, 12, 1, 32767
    PlaySE SEQ_SE_W095_01, 0, EFFVM_ATTACKER, 10, 0, 80, 0, 0, 0
    WaitFrames 22
    PaletteFade 4, 12, 0, 1, 32767
    SetYield FALSE
    PlaySE SEQ_SE_W112_01, 1, EFFVM_ATTACKER, 5, 300, 115, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 5, 0, 115, 0, 0, 0
    SetYield TRUE
    ParticleLoad 508
    EmitterAdd 508, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x5ad6
    WaitFrames 40
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x5ad6
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
