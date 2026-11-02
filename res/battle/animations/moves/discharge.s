// MOVE_DISCHARGE's animation: member 435 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Discharge

Discharge:
    CameraMovePreset 1, 9, 16, 0, 12
    PaletteFade 4, 0, 12, 1, 0
    McssPalFade EFFVM_ATTACKER, 0, 12, 6, 0x33ff
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 10, 40, 0, 0
    PlaySE SEQ_SE_W295_02, 1, EFFVM_ATTACKER, 60, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_ATTACKER, 20, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_ATTACKER, 40, 400, 110, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 614
    EmitterAdd 614, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 614, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 614, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 70
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x33ff
    CameraMovePreset 1, 11, 32, 0, 22
    SetYield FALSE
    PlaySE SEQ_SE_W077_02, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x33ff
    EmitterAdd 614, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 6
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x33ff
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
