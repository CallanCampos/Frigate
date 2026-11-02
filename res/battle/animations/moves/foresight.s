// MOVE_FORESIGHT's animation: member 193 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Foresight

Foresight:
    CameraMovePreset 1, 11, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 2, EFFVM_DEFENDER, 30, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 363
    EmitterAdd 363, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 363, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 363, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 363, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 50
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x7fff
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
