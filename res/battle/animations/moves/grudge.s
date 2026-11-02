// MOVE_GRUDGE's animation: member 288 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Grudge

Grudge:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W288_DORO, 1, EFFVM_ATTACKER, 0, -100, 127, 0, 0, 0
    PlaySE SEQ_SE_W288_HYUU, 2, EFFVM_ATTACKER, 0, 0, 70, 5, 0, 0
    MoveSE 1, 0, 1, 127, 0, 60, 80, 0, 0
    MoveSE 2, 0, 1, 70, 0, 50, 70, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 459
    EmitterAdd 459, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 459, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0, 0x800, 16, 1, 1
    PaletteFade 4, 0, 12, 1, 0
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x5816
    CameraMovePreset 1, 8, 32, 0, 12
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x5816
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x5816
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x5816
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
