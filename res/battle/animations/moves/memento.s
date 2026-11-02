// MOVE_MEMENTO's animation: member 262 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Memento

Memento:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 1000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    ParticleLoad 432
    EmitterAdd 432, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x1806
    McssScale EFFVM_ATTACKER, 2, 0, 0xccd, 12, 1, 1
    WaitFrames 20
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x1806
    CameraMovePreset 1, 11, 32, 0, 22
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x1806
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 2, EFFVM_DEFENDER, 0, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 1000, -1000, 0, 50, 0, 0
    MoveSE 2, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    EmitterAdd 432, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x1806
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
