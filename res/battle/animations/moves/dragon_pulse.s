// MOVE_DRAGON_PULSE's animation: member 406 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DragonPulse

DragonPulse:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 13, 0, 0
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 585
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 2000, 0, 70, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 2, EFFVM_ATTACKER, 70, 2000, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 170, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 140, 30, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 70, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 170, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 100, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 106, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 112, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 118, 0, 127, 0, 0, 0
    MoveSE 1, 0, 0, 0, 2000, 0, 100, 0, 0
    MoveSE 1, 0, 1, 115, 0, 140, 30, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 40, 0, 0
    SetYield TRUE
    EmitterAdd 585, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 585, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 585, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    CameraMovePreset 1, 11, 32, 0, 22
    EmitterAdd 585, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 14
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 0, 0
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
