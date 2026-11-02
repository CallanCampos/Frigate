// MOVE_ROAR_OF_TIME's animation: member 459 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RoarOfTime

RoarOfTime:
    PaletteFade 4, 0, 13, 2, 0
    SetVanish 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W179_03, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 30, 0, 0
    SetYield TRUE
    ParticleLoad 638
    EmitterAdd 638, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 638, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W113_02, 2, EFFVM_ATTACKER, 20, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_SYS_89, 1, EFFVM_ATTACKER, 60, -500, 125, 0, 0, 0
    PlaySE SEQ_SE_FLD_131, 2, EFFVM_DEFENDER, 120, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 638, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 638, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 638, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 60
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 638, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 638, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 13, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    SetVanish 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
