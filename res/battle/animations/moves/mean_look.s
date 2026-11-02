// MOVE_MEAN_LOOK's animation: member 212 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MeanLook

MeanLook:
    CameraMovePreset 1, 11, 16, 0, 12
    PlaySE SEQ_SE_W157_03, 3, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, 3000, -1000, 0, 100, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 382
    EmitterAdd 382, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 382, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 382, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 382, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 12, 1, 31775
    WaitFrames 100
    SetYield FALSE
    PlaySE SEQ_SE_W319_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    PaletteFade 4, 12, 12, 1, 32767
    WaitFrames 4
    PaletteFade 4, 12, 0, 1, 32767
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
