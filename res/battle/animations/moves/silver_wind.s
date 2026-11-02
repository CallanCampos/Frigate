// MOVE_SILVER_WIND's animation: member 318 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SilverWind

SilverWind:
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    PlaySE SEQ_SE_FLD_84, 1, EFFVM_ATTACKER, 10, 0, 120, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1500, 20, 100, 0, 0
    MoveSE 1, 0, 1, 127, 0, 80, 30, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 170, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W377_01, 2, EFFVM_ATTACKER, 5, 0, 100, 0, 0, 0
    MoveSE 2, 0, 1, 127, 0, 35, 40, 0, 0
    ParticleLoad 492
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, SilverWind_01CC
    EmitterAdd 492, 0, 0, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 492, 2, 0, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, SilverWind_0228

SilverWind_01CC:
    EmitterAdd 492, 1, 0, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 492, 3, 0, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

SilverWind_0228:
    PaletteFade 4, 0, 12, 1, 0
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x318c
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x318c
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 13
    WaitFrames 60
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x318c
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x318c
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
