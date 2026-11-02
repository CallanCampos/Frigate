// Effect BTLV_EFFECT_FIRST + 3 (0x234): member 3 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect003

Effect003:
    McssReset TRUE
    SetValue 3
    SetYield FALSE
    IfPokeExists 0, FALSE, Effect003_00E0
    McssScale 0, 0, 0, 0, 0, 0, 0
    McssSetMode 0, 3
    McssSetVanish 0, TRUE
    McssSetValue 0, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect003_00E0
    McssMoveXY 0, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 44, EFFVM_CMP_EQ, 1, Effect003_00E0
    SetValue 0

Effect003_00E0:
    IfPokeExists 2, FALSE, Effect003_0172
    McssScale 2, 0, 0, 0, 0, 0, 0
    McssSetMode 2, 3
    McssSetVanish 2, TRUE
    McssSetValue 2, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE + 2, EFFVM_CMP_EQ, TRUE, Effect003_0172
    McssMoveXY 2, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 46, EFFVM_CMP_EQ, 1, Effect003_0172
    SetValue 0

Effect003_0172:
    IfPokeExists 4, FALSE, Effect003_0204
    McssScale 4, 0, 0, 0, 0, 0, 0
    McssSetMode 4, 3
    McssSetVanish 4, TRUE
    McssSetValue 4, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE + 4, EFFVM_CMP_EQ, TRUE, Effect003_0204
    McssMoveXY 4, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 48, EFFVM_CMP_EQ, 1, Effect003_0204
    SetValue 0

Effect003_0204:
    IfPokeExists 6, FALSE, Effect003_0296
    McssScale 6, 0, 0, 0, 0, 0, 0
    McssSetMode 6, 3
    McssSetVanish 6, TRUE
    McssSetValue 6, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE + 6, EFFVM_CMP_EQ, TRUE, Effect003_0296
    McssMoveXY 6, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 50, EFFVM_CMP_EQ, 1, Effect003_0296
    SetValue 0

Effect003_0296:
    IfPokeExists 8, FALSE, Effect003_02AE
    McssPlayAnim 8, 1

Effect003_02AE:
    IfPokeExists 10, FALSE, Effect003_02C6
    McssPlayAnim 10, 1

Effect003_02C6:
    IfPokeExists 12, FALSE, Effect003_02DE
    McssPlayAnim 12, 1

Effect003_02DE:
    WaitFrames 34
    PlaySE SEQ_SE_NAGERU, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    SetYield FALSE
    IfPokeExists 0, FALSE, Effect003_0360
    SetBallSource 0
    ParticleLoad 48
    EmitterMoveFromOrtho 48, 0, 3, -0x2800, 0x1e66, 0xc000, 0, 0x2000, 0x1e000, 0x3000, 0x1000, 0x1000, 0x800

Effect003_0360:
    IfPokeExists 2, FALSE, Effect003_03B0
    SetBallSource 2
    ParticleLoad 48
    EmitterMoveFromOrtho 48, 0, 3, -0x2800, 0x1e66, 0xc000, 2, 0x2000, 0x1e000, 0x3000, 0x1000, 0x1000, 0x800

Effect003_03B0:
    IfPokeExists 4, FALSE, Effect003_044A
    SetBallSource 4
    ParticleLoad 48
    IfPokeExists 12, TRUE, Effect003_0414
    EmitterMoveFromOrtho 48, 0, 3, -0x2800, 0x1e66, 0xc000, 4, 0x2000, 0x1e000, 0x3000, 0x1000, 0x1000, 0x800
    Jump Effect003_044A

Effect003_0414:
    EmitterMoveFromOrtho 48, 0, 3, 0x1000, 0x1ccd, 0xc000, 4, 0x2000, 0x1e000, 0x3000, 0x1000, 0x1000, 0x800

Effect003_044A:
    IfPokeExists 6, FALSE, Effect003_049A
    SetBallSource 6
    ParticleLoad 48
    EmitterMoveFromOrtho 48, 0, 3, -0x2800, 0x1e66, 0xc000, 6, 0x2000, 0x1e000, 0x3000, 0x1000, 0x1000, 0x800

Effect003_049A:
    WaitFrames 30
    SetYield FALSE
    IfPokeExists 8, FALSE, Effect003_04E4
    McssPalFade 8, 0, 16, 0, 0
    McssAlpha 8, 1, 1, 16, 0, 0

Effect003_04E4:
    IfPokeExists 10, FALSE, Effect003_0522
    McssPalFade 10, 0, 16, 0, 0
    McssAlpha 10, 1, 1, 16, 0, 0

Effect003_0522:
    IfPokeExists 12, FALSE, Effect003_0560
    McssPalFade 12, 0, 16, 0, 0
    McssAlpha 12, 1, 1, 16, 0, 0

Effect003_0560:
    Wait EFFVM_WAIT_MCSS
    IfPokeExists 8, FALSE, Effect003_057A
    DeleteMcss 8

Effect003_057A:
    IfPokeExists 10, FALSE, Effect003_058E
    DeleteMcss 10

Effect003_058E:
    IfPokeExists 12, FALSE, Effect003_05A2
    DeleteMcss 12

Effect003_05A2:
    IfPokeExists 0, FALSE, Effect003_05BC
    SetBallSource 0
    ParticleDelete 48

Effect003_05BC:
    IfPokeExists 2, FALSE, Effect003_05D6
    SetBallSource 2
    ParticleDelete 48

Effect003_05D6:
    IfPokeExists 4, FALSE, Effect003_05F0
    SetBallSource 4
    ParticleDelete 48

Effect003_05F0:
    IfPokeExists 6, FALSE, Effect003_060A
    SetBallSource 6
    ParticleDelete 48

Effect003_060A:
    SetYield FALSE
    PlaySE SEQ_SE_BOWA2, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    IfPokeExists 0, FALSE, Effect003_06D6
    SetBallSource 0
    ParticleLoad 3
    EmitterAddAll 3, 0, 8, 0x4000, 0, 4096, 0x800, 0x1000, 0x800, 0x800
    McssPalFade 0, 16, 16, 0, 0x7fff
    McssScale 0, 1, 0xb33, 0xb33, 32, 0, 0
    McssSetMode 0, 4
    PlayCry 0, 0x80000000, 0, 0, 0, 0, 0

Effect003_06D6:
    IfPokeExists 2, FALSE, Effect003_0776
    SetBallSource 2
    ParticleLoad 3
    EmitterAddAll 3, 2, 8, 0x4000, 0, 4096, 0x800, 0x1000, 0x800, 0x800
    McssPalFade 2, 16, 16, 0, 0x7fff
    McssScale 2, 1, 0xb33, 0xb33, 32, 0, 0
    McssSetMode 2, 4
    PlayCry 2, 0x80000000, 0, 0, 0, 0, 0

Effect003_0776:
    IfPokeExists 4, FALSE, Effect003_0816
    SetBallSource 4
    ParticleLoad 3
    EmitterAddAll 3, 4, 8, 0x4000, 0, 4096, 0x800, 0x1000, 0x800, 0x800
    McssPalFade 4, 16, 16, 0, 0x7fff
    McssScale 4, 1, 0xb33, 0xb33, 32, 0, 0
    McssSetMode 4, 4
    PlayCry 4, 0x80000000, 0, 0, 0, 0, 0

Effect003_0816:
    IfPokeExists 6, FALSE, Effect003_08B6
    SetBallSource 6
    ParticleLoad 3
    EmitterAddAll 3, 6, 8, 0x4000, 0, 4096, 0x800, 0x1000, 0x800, 0x800
    McssPalFade 6, 16, 16, 0, 0x7fff
    McssScale 6, 1, 0xb33, 0xb33, 32, 0, 0
    McssSetMode 6, 4
    PlayCry 6, 0x80000000, 0, 0, 0, 0, 0

Effect003_08B6:
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    IfPokeExists 0, FALSE, Effect003_0934
    McssPalFade 0, 16, 0, 1, 0x7fff
    McssScale 0, 1, 0x800, 0x800, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect003_0934
    McssMoveXY 0, 1, 0, -0x1000, 16, 0, 0

Effect003_0934:
    IfPokeExists 2, FALSE, Effect003_09A6
    McssPalFade 2, 16, 0, 1, 0x7fff
    McssScale 2, 1, 0x800, 0x800, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE + 2, EFFVM_CMP_EQ, TRUE, Effect003_09A6
    McssMoveXY 2, 1, 0, -0x1000, 16, 0, 0

Effect003_09A6:
    IfPokeExists 4, FALSE, Effect003_0A18
    McssPalFade 4, 16, 0, 1, 0x7fff
    McssScale 4, 1, 0x800, 0x800, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE + 4, EFFVM_CMP_EQ, TRUE, Effect003_0A18
    McssMoveXY 4, 1, 0, -0x1000, 16, 0, 0

Effect003_0A18:
    IfPokeExists 6, FALSE, Effect003_0A8A
    McssPalFade 6, 16, 0, 1, 0x7fff
    McssScale 6, 1, 0x800, 0x800, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE + 6, EFFVM_CMP_EQ, TRUE, Effect003_0A8A
    McssMoveXY 6, 1, 0, -0x1000, 16, 0, 0

Effect003_0A8A:
    Wait EFFVM_WAIT_ALL
    SetYield TRUE
    ParticleLoad 325
    SetYield FALSE
    IfPokeExists 0, FALSE, Effect003_0B62
    IfVarConst EFFVM_VAR_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect003_0B62
    McssMoveXY 0, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 44, EFFVM_CMP_EQ, 1, Effect003_0B62
    IfVarConst EFFVM_VAR_WEIGHT, EFFVM_CMP_LT, 1500, Effect003_0B38
    EmitterAdd 325, 2, 0, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect003_0B38:
    IfVarConst EFFVM_VAR_WEIGHT, EFFVM_CMP_LT, 500, Effect003_0B62
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect003_0B62
    SetValue 1

Effect003_0B62:
    IfPokeExists 2, FALSE, Effect003_0C22
    IfVarConst EFFVM_VAR_NO_BOUNCE + 2, EFFVM_CMP_EQ, TRUE, Effect003_0C22
    McssMoveXY 2, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 46, EFFVM_CMP_EQ, 1, Effect003_0C22
    IfVarConst EFFVM_VAR_WEIGHT + 2, EFFVM_CMP_LT, 1500, Effect003_0BF8
    EmitterAdd 325, 2, 2, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect003_0BF8:
    IfVarConst EFFVM_VAR_WEIGHT + 2, EFFVM_CMP_LT, 500, Effect003_0C22
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect003_0C22
    SetValue 1

Effect003_0C22:
    IfPokeExists 4, FALSE, Effect003_0CE2
    IfVarConst EFFVM_VAR_NO_BOUNCE + 4, EFFVM_CMP_EQ, TRUE, Effect003_0CE2
    McssMoveXY 4, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 48, EFFVM_CMP_EQ, 1, Effect003_0CE2
    IfVarConst EFFVM_VAR_WEIGHT + 4, EFFVM_CMP_LT, 1500, Effect003_0CB8
    EmitterAdd 325, 2, 4, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect003_0CB8:
    IfVarConst EFFVM_VAR_WEIGHT + 4, EFFVM_CMP_LT, 500, Effect003_0CE2
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect003_0CE2
    SetValue 1

Effect003_0CE2:
    IfPokeExists 6, FALSE, Effect003_0DA2
    IfVarConst EFFVM_VAR_NO_BOUNCE + 6, EFFVM_CMP_EQ, TRUE, Effect003_0DA2
    McssMoveXY 6, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 50, EFFVM_CMP_EQ, 1, Effect003_0DA2
    IfVarConst EFFVM_VAR_WEIGHT + 6, EFFVM_CMP_LT, 1500, Effect003_0D78
    EmitterAdd 325, 2, 6, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect003_0D78:
    IfVarConst EFFVM_VAR_WEIGHT + 6, EFFVM_CMP_LT, 500, Effect003_0DA2
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect003_0DA2
    SetValue 1

Effect003_0DA2:
    WaitFrames 10
    SetYield FALSE
    IfPokeExists 0, FALSE, Effect003_0DE8
    IfVarConst 44, EFFVM_CMP_NE, 1, Effect003_0DE8
    McssShake 0, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect003_0DE8:
    IfPokeExists 2, FALSE, Effect003_0E22
    IfVarConst 46, EFFVM_CMP_NE, 1, Effect003_0E22
    McssShake 2, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect003_0E22:
    IfPokeExists 4, FALSE, Effect003_0E5C
    IfVarConst 48, EFFVM_CMP_NE, 1, Effect003_0E5C
    McssShake 4, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect003_0E5C:
    IfPokeExists 6, FALSE, Effect003_0E96
    IfVarConst 50, EFFVM_CMP_NE, 1, Effect003_0E96
    McssShake 6, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect003_0E96:
    SetYield FALSE
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 2, Effect003_0ED4
    PlaySE SEQ_SE_TOUJOU_LARGE, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0

Effect003_0ED4:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 1, Effect003_0F0C
    PlaySE SEQ_SE_TOUJOU_MID, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0

Effect003_0F0C:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 0, Effect003_0F44
    PlaySE SEQ_SE_TOUJOU_SMALL, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0

Effect003_0F44:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 2, Effect003_1020
    CameraMove 2, 0, 0x1000, 0, 0, 0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x1000, 0, 0, -0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, 0x1000, 0, 0, 0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x1000, 0, 0, -0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA

Effect003_1020:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 1, Effect003_10F2
    CameraMove 2, 0, 0x800, 0, 0, 0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x800, 0, 0, -0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, 0x800, 0, 0, 0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x800, 0, 0, -0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA

Effect003_10F2:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 0, Effect003_116A
    CameraMove 2, 0, 0x400, 0, 0, 0x400, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x400, 0, 0, -0x400, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    Wait EFFVM_WAIT_CRY

Effect003_116A:
    SetYield FALSE
    IfPokeExists 0, FALSE, Effect003_11E4
    IfVarConst EFFVM_VAR_FAMOUS, EFFVM_CMP_EQ, FALSE, Effect003_11A4
    Call BTLV_EFFECT_FIRST + 114, 0, 0
    Jump Effect003_11E4

Effect003_11A4:
    IfVarConst EFFVM_VAR_RARE, EFFVM_CMP_EQ, FALSE, Effect003_11C4
    Call BTLV_EFFECT_FIRST + 47, 0, 0

Effect003_11C4:
    IfVarConst EFFVM_VAR_N_POKEMON, EFFVM_CMP_EQ, FALSE, Effect003_11E4
    Call BTLV_EFFECT_FIRST + 98, 0, 0

Effect003_11E4:
    McssSetVanish 0, FALSE
    McssSetValue 0, 0
    IfPokeExists 2, FALSE, Effect003_126C
    IfVarConst EFFVM_VAR_FAMOUS + 2, EFFVM_CMP_EQ, FALSE, Effect003_122C
    Call BTLV_EFFECT_FIRST + 114, 2, 2
    Jump Effect003_126C

Effect003_122C:
    IfVarConst EFFVM_VAR_RARE + 2, EFFVM_CMP_EQ, FALSE, Effect003_124C
    Call BTLV_EFFECT_FIRST + 47, 2, 2

Effect003_124C:
    IfVarConst EFFVM_VAR_N_POKEMON + 2, EFFVM_CMP_EQ, FALSE, Effect003_126C
    Call BTLV_EFFECT_FIRST + 98, 2, 2

Effect003_126C:
    McssSetVanish 2, FALSE
    McssSetValue 2, 0
    IfPokeExists 4, FALSE, Effect003_12F4
    IfVarConst EFFVM_VAR_FAMOUS + 4, EFFVM_CMP_EQ, FALSE, Effect003_12B4
    Call BTLV_EFFECT_FIRST + 114, 4, 4
    Jump Effect003_12F4

Effect003_12B4:
    IfVarConst EFFVM_VAR_RARE + 4, EFFVM_CMP_EQ, FALSE, Effect003_12D4
    Call BTLV_EFFECT_FIRST + 47, 4, 4

Effect003_12D4:
    IfVarConst EFFVM_VAR_N_POKEMON + 4, EFFVM_CMP_EQ, FALSE, Effect003_12F4
    Call BTLV_EFFECT_FIRST + 98, 4, 4

Effect003_12F4:
    McssSetVanish 4, FALSE
    McssSetValue 4, 0
    IfPokeExists 6, FALSE, Effect003_137C
    IfVarConst EFFVM_VAR_FAMOUS + 6, EFFVM_CMP_EQ, FALSE, Effect003_133C
    Call BTLV_EFFECT_FIRST + 114, 6, 6
    Jump Effect003_137C

Effect003_133C:
    IfVarConst EFFVM_VAR_RARE + 6, EFFVM_CMP_EQ, FALSE, Effect003_135C
    Call BTLV_EFFECT_FIRST + 47, 6, 6

Effect003_135C:
    IfVarConst EFFVM_VAR_N_POKEMON + 6, EFFVM_CMP_EQ, FALSE, Effect003_137C
    Call BTLV_EFFECT_FIRST + 98, 6, 6

Effect003_137C:
    McssSetVanish 6, FALSE
    McssSetValue 6, 0
    Wait EFFVM_WAIT_PARTICLES
    Wait EFFVM_WAIT_MCSS
    WaitFrames 5
    SetYield FALSE
    CameraMovePreset 1, 8, 10, 0, 8
    IfPokeExists 0, FALSE, Effect003_1408
    McssScale 0, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 0, 5, 0, 0, 0, 0, 0

Effect003_1408:
    IfPokeExists 1, FALSE, Effect003_1452
    McssScale 1, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 1, 5, 0, 0, 0, 0, 0

Effect003_1452:
    IfPokeExists 2, FALSE, Effect003_149C
    McssScale 2, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 2, 5, 0, 0, 0, 0, 0

Effect003_149C:
    IfPokeExists 3, FALSE, Effect003_14E6
    McssScale 3, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 3, 5, 0, 0, 0, 0, 0

Effect003_14E6:
    IfPokeExists 4, FALSE, Effect003_1530
    McssScale 4, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 4, 5, 0, 0, 0, 0, 0

Effect003_1530:
    IfPokeExists 5, FALSE, Effect003_157A
    McssScale 5, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 5, 5, 0, 0, 0, 0, 0

Effect003_157A:
    IfPokeExists 6, FALSE, Effect003_15C4
    McssScale 6, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 6, 5, 0, 0, 0, 0, 0

Effect003_15C4:
    IfPokeExists 7, FALSE, Effect003_160E
    McssScale 7, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 7, 5, 0, 0, 0, 0, 0

Effect003_160E:
    IfPokeExists 9, FALSE, Effect003_1658
    McssScale 9, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 9, 5, 0, 0, 0, 0, 0

Effect003_1658:
    IfPokeExists 11, FALSE, Effect003_16A2
    McssScale 11, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 11, 5, 0, 0, 0, 0, 0

Effect003_16A2:
    IfPokeExists 13, FALSE, Effect003_16EC
    McssScale 13, 1, 0x1000, 0x1000, 10, 0, 0
    McssMoveXY 13, 5, 0, 0, 0, 0, 0

Effect003_16EC:
    Wait EFFVM_WAIT_ALL
    End
