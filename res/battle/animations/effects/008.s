// Effect BTLV_EFFECT_FIRST + 8 (0x239): member 8 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect008

Effect008:
    SetYield FALSE
    SetValue 3
    IfPokeExists 1, FALSE, Effect008_00DA
    McssScale 1, 0, 0, 0, 0, 0, 0
    McssSetMode 1, 3
    McssSetVanish 1, TRUE
    McssSetValue 1, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE + 1, EFFVM_CMP_EQ, TRUE, Effect008_00DA
    McssMoveXY 1, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 45, EFFVM_CMP_EQ, 1, Effect008_00DA
    SetValue 0

Effect008_00DA:
    IfPokeExists 3, FALSE, Effect008_016C
    McssScale 3, 0, 0, 0, 0, 0, 0
    McssSetMode 3, 3
    McssSetVanish 3, TRUE
    McssSetValue 3, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE + 3, EFFVM_CMP_EQ, TRUE, Effect008_016C
    McssMoveXY 3, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 47, EFFVM_CMP_EQ, 1, Effect008_016C
    SetValue 0

Effect008_016C:
    IfPokeExists 5, FALSE, Effect008_01FE
    McssScale 5, 0, 0, 0, 0, 0, 0
    McssSetMode 5, 3
    McssSetVanish 5, TRUE
    McssSetValue 5, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE + 5, EFFVM_CMP_EQ, TRUE, Effect008_01FE
    McssMoveXY 5, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 49, EFFVM_CMP_EQ, 1, Effect008_01FE
    SetValue 0

Effect008_01FE:
    IfPokeExists 7, FALSE, Effect008_0290
    McssScale 7, 0, 0, 0, 0, 0, 0
    McssSetMode 7, 3
    McssSetVanish 7, TRUE
    McssSetValue 7, 1
    IfVarConst EFFVM_VAR_NO_BOUNCE + 7, EFFVM_CMP_EQ, TRUE, Effect008_0290
    McssMoveXY 7, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 51, EFFVM_CMP_EQ, 1, Effect008_0290
    SetValue 0

Effect008_0290:
    WaitFrames 6
    PlaySE SEQ_SE_NAGERU, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    SetYield FALSE
    IfPokeExists 1, FALSE, Effect008_030A
    SetBallSource 1
    ParticleLoad 48
    EmitterAdd 48, 0, 1, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

Effect008_030A:
    IfPokeExists 3, FALSE, Effect008_0352
    SetBallSource 3
    ParticleLoad 48
    EmitterAdd 48, 0, 3, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

Effect008_0352:
    IfPokeExists 5, FALSE, Effect008_039A
    SetBallSource 5
    ParticleLoad 48
    EmitterAdd 48, 0, 5, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

Effect008_039A:
    IfPokeExists 7, FALSE, Effect008_03E2
    SetBallSource 7
    ParticleLoad 48
    EmitterAdd 48, 0, 7, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

Effect008_03E2:
    IfPokeExists 9, FALSE, Effect008_0446
    McssScale 9, 1, 0xccd, 0xccd, 20, 0, 0
    McssMoveXY 9, 1, -0x1000, 0x1000, 20, 0, 0
    McssAlpha 9, 1, 2, 20, 0, 0

Effect008_0446:
    IfPokeExists 11, FALSE, Effect008_04AA
    McssScale 11, 1, 0xccd, 0xccd, 20, 0, 0
    McssMoveXY 11, 1, -0x1000, 0x1000, 20, 0, 0
    McssAlpha 11, 1, 2, 20, 0, 0

Effect008_04AA:
    IfPokeExists 13, FALSE, Effect008_050E
    McssScale 13, 1, 0xccd, 0xccd, 20, 0, 0
    McssMoveXY 13, 1, -0x1000, 0x1000, 20, 0, 0
    McssAlpha 13, 1, 2, 20, 0, 0

Effect008_050E:
    Wait EFFVM_WAIT_MCSS
    IfPokeExists 9, FALSE, Effect008_0528
    DeleteMcss 9

Effect008_0528:
    IfPokeExists 11, FALSE, Effect008_053C
    DeleteMcss 11

Effect008_053C:
    IfPokeExists 13, FALSE, Effect008_0550
    DeleteMcss 13

Effect008_0550:
    WaitFrames 5
    IfPokeExists 1, FALSE, Effect008_0570
    SetBallSource 1
    ParticleDelete 48

Effect008_0570:
    IfPokeExists 3, FALSE, Effect008_058A
    SetBallSource 3
    ParticleDelete 48

Effect008_058A:
    IfPokeExists 5, FALSE, Effect008_05A4
    SetBallSource 5
    ParticleDelete 48

Effect008_05A4:
    IfPokeExists 7, FALSE, Effect008_05BE
    SetBallSource 7
    ParticleDelete 48

Effect008_05BE:
    SetYield FALSE
    PlaySE SEQ_SE_BOWA2, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    IfPokeExists 1, FALSE, Effect008_068A
    SetBallSource 1
    ParticleLoad 3
    EmitterAddAll 3, 1, 8, 0x4000, 0, FALSE, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade 1, 16, 16, 0, 0x7fff
    McssScale 1, 1, 0x1333, 0x1333, 32, 0, 0
    McssSetMode 1, 4
    PlayCry 1, 0x80000000, 0, 0, 0, 0, 0

Effect008_068A:
    IfPokeExists 3, FALSE, Effect008_072A
    SetBallSource 3
    ParticleLoad 3
    EmitterAddAll 3, 3, 8, 0x4000, 0, FALSE, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade 3, 16, 16, 0, 0x7fff
    McssScale 3, 1, 0x1333, 0x1333, 32, 0, 0
    McssSetMode 3, 4
    PlayCry 3, 0x80000000, 0, 0, 0, 0, 0

Effect008_072A:
    IfPokeExists 5, FALSE, Effect008_07CA
    SetBallSource 5
    ParticleLoad 3
    EmitterAddAll 3, 5, 8, 0x4000, 0, FALSE, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade 5, 16, 16, 0, 0x7fff
    McssScale 5, 1, 0x1333, 0x1333, 32, 0, 0
    McssSetMode 5, 4
    PlayCry 5, 0x80000000, 0, 0, 0, 0, 0

Effect008_07CA:
    IfPokeExists 7, FALSE, Effect008_086A
    SetBallSource 7
    ParticleLoad 3
    EmitterAddAll 3, 7, 8, 0x4000, 0, FALSE, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade 7, 16, 16, 0, 0x7fff
    McssScale 7, 1, 0x1333, 0x1333, 32, 0, 0
    McssSetMode 7, 4
    PlayCry 7, 0x80000000, 0, 0, 0, 0, 0

Effect008_086A:
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    IfPokeExists 1, FALSE, Effect008_08E8
    McssPalFade 1, 16, 0, 1, 0x7fff
    McssScale 1, 1, 0x1000, 0x1000, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE + 1, EFFVM_CMP_EQ, TRUE, Effect008_08E8
    McssMoveXY 1, 1, 0, -0x1000, 16, 0, 0

Effect008_08E8:
    IfPokeExists 3, FALSE, Effect008_095A
    McssPalFade 3, 16, 0, 1, 0x7fff
    McssScale 3, 1, 0x1000, 0x1000, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE + 3, EFFVM_CMP_EQ, TRUE, Effect008_095A
    McssMoveXY 3, 1, 0, -0x1000, 16, 0, 0

Effect008_095A:
    IfPokeExists 5, FALSE, Effect008_09CC
    McssPalFade 5, 16, 0, 1, 0x7fff
    McssScale 5, 1, 0x1000, 0x1000, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE + 5, EFFVM_CMP_EQ, TRUE, Effect008_09CC
    McssMoveXY 5, 1, 0, -0x1000, 16, 0, 0

Effect008_09CC:
    IfPokeExists 7, FALSE, Effect008_0A3E
    McssPalFade 7, 16, 0, 1, 0x7fff
    McssScale 7, 1, 0x1000, 0x1000, 16, 0, 0
    IfVarConst EFFVM_VAR_NO_BOUNCE + 7, EFFVM_CMP_EQ, TRUE, Effect008_0A3E
    McssMoveXY 7, 1, 0, -0x1000, 16, 0, 0

Effect008_0A3E:
    Wait EFFVM_WAIT_ALL
    SetYield TRUE
    ParticleLoad 325
    SetYield FALSE
    IfPokeExists 1, FALSE, Effect008_0B16
    IfVarConst EFFVM_VAR_NO_BOUNCE + 1, EFFVM_CMP_EQ, TRUE, Effect008_0B16
    McssMoveXY 1, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 45, EFFVM_CMP_EQ, 1, Effect008_0B16
    IfVarConst EFFVM_VAR_WEIGHT + 1, EFFVM_CMP_LT, 1500, Effect008_0AEC
    EmitterAdd 325, 2, 1, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect008_0AEC:
    IfVarConst EFFVM_VAR_WEIGHT + 1, EFFVM_CMP_LT, 500, Effect008_0B16
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect008_0B16
    SetValue 1

Effect008_0B16:
    IfPokeExists 3, FALSE, Effect008_0BD6
    IfVarConst EFFVM_VAR_NO_BOUNCE + 3, EFFVM_CMP_EQ, TRUE, Effect008_0BD6
    McssMoveXY 3, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 47, EFFVM_CMP_EQ, 1, Effect008_0BD6
    IfVarConst EFFVM_VAR_WEIGHT + 3, EFFVM_CMP_LT, 1500, Effect008_0BAC
    EmitterAdd 325, 2, 3, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect008_0BAC:
    IfVarConst EFFVM_VAR_WEIGHT + 3, EFFVM_CMP_LT, 500, Effect008_0BD6
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect008_0BD6
    SetValue 1

Effect008_0BD6:
    IfPokeExists 5, FALSE, Effect008_0C96
    IfVarConst EFFVM_VAR_NO_BOUNCE + 5, EFFVM_CMP_EQ, TRUE, Effect008_0C96
    McssMoveXY 5, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 49, EFFVM_CMP_EQ, 1, Effect008_0C96
    IfVarConst EFFVM_VAR_WEIGHT + 5, EFFVM_CMP_LT, 1500, Effect008_0C6C
    EmitterAdd 325, 2, 5, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect008_0C6C:
    IfVarConst EFFVM_VAR_WEIGHT + 5, EFFVM_CMP_LT, 500, Effect008_0C96
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect008_0C96
    SetValue 1

Effect008_0C96:
    IfPokeExists 7, FALSE, Effect008_0D56
    IfVarConst EFFVM_VAR_NO_BOUNCE + 7, EFFVM_CMP_EQ, TRUE, Effect008_0D56
    McssMoveXY 7, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 51, EFFVM_CMP_EQ, 1, Effect008_0D56
    IfVarConst EFFVM_VAR_WEIGHT + 7, EFFVM_CMP_LT, 1500, Effect008_0D2C
    EmitterAdd 325, 2, 7, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect008_0D2C:
    IfVarConst EFFVM_VAR_WEIGHT + 7, EFFVM_CMP_LT, 500, Effect008_0D56
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect008_0D56
    SetValue 1

Effect008_0D56:
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    IfPokeExists 1, FALSE, Effect008_0D9C
    IfVarConst 45, EFFVM_CMP_NE, 1, Effect008_0D9C
    McssShake 1, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect008_0D9C:
    IfPokeExists 3, FALSE, Effect008_0DD6
    IfVarConst 47, EFFVM_CMP_NE, 1, Effect008_0DD6
    McssShake 3, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect008_0DD6:
    IfPokeExists 5, FALSE, Effect008_0E10
    IfVarConst 49, EFFVM_CMP_NE, 1, Effect008_0E10
    McssShake 5, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect008_0E10:
    IfPokeExists 7, FALSE, Effect008_0E4A
    IfVarConst 51, EFFVM_CMP_NE, 1, Effect008_0E4A
    McssShake 7, 1, 0x7fff800, 0xffff000, 0x800, 32

Effect008_0E4A:
    SetYield FALSE
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 2, Effect008_0E88
    PlaySE SEQ_SE_TOUJOU_LARGE, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0

Effect008_0E88:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 1, Effect008_0EC0
    PlaySE SEQ_SE_TOUJOU_MID, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0

Effect008_0EC0:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 0, Effect008_0EF8
    PlaySE SEQ_SE_TOUJOU_SMALL, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0

Effect008_0EF8:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 2, Effect008_0FD4
    CameraMove 2, 0, 0x1000, 0, 0, 0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x1000, 0, 0, -0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, 0x1000, 0, 0, 0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x1000, 0, 0, -0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA

Effect008_0FD4:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 1, Effect008_10A6
    CameraMove 2, 0, 0x800, 0, 0, 0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x800, 0, 0, -0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, 0x800, 0, 0, 0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x800, 0, 0, -0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA

Effect008_10A6:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 0, Effect008_111E
    CameraMove 2, 0, 0x400, 0, 0, 0x400, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x400, 0, 0, -0x400, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    Wait EFFVM_WAIT_CRY

Effect008_111E:
    SetYield FALSE
    IfPokeExists 1, FALSE, Effect008_1198
    IfVarConst EFFVM_VAR_FAMOUS + 1, EFFVM_CMP_EQ, FALSE, Effect008_1158
    Call BTLV_EFFECT_FIRST + 114, 1, 1
    Jump Effect008_1198

Effect008_1158:
    IfVarConst EFFVM_VAR_RARE + 1, EFFVM_CMP_EQ, FALSE, Effect008_1178
    Call BTLV_EFFECT_FIRST + 47, 1, 1

Effect008_1178:
    IfVarConst EFFVM_VAR_N_POKEMON + 1, EFFVM_CMP_EQ, FALSE, Effect008_1198
    Call BTLV_EFFECT_FIRST + 98, 1, 1

Effect008_1198:
    McssSetVanish 1, FALSE
    McssSetValue 1, 0
    IfPokeExists 3, FALSE, Effect008_1220
    IfVarConst EFFVM_VAR_FAMOUS + 3, EFFVM_CMP_EQ, FALSE, Effect008_11E0
    Call BTLV_EFFECT_FIRST + 114, 3, 3
    Jump Effect008_1220

Effect008_11E0:
    IfVarConst EFFVM_VAR_RARE + 3, EFFVM_CMP_EQ, FALSE, Effect008_1200
    Call BTLV_EFFECT_FIRST + 47, 3, 3

Effect008_1200:
    IfVarConst EFFVM_VAR_N_POKEMON + 3, EFFVM_CMP_EQ, FALSE, Effect008_1220
    Call BTLV_EFFECT_FIRST + 98, 3, 3

Effect008_1220:
    McssSetVanish 3, FALSE
    McssSetValue 3, 0
    IfPokeExists 5, FALSE, Effect008_12A8
    IfVarConst EFFVM_VAR_FAMOUS + 5, EFFVM_CMP_EQ, FALSE, Effect008_1268
    Call BTLV_EFFECT_FIRST + 114, 5, 5
    Jump Effect008_12A8

Effect008_1268:
    IfVarConst EFFVM_VAR_RARE + 5, EFFVM_CMP_EQ, FALSE, Effect008_1288
    Call BTLV_EFFECT_FIRST + 47, 5, 5

Effect008_1288:
    IfVarConst EFFVM_VAR_N_POKEMON + 5, EFFVM_CMP_EQ, FALSE, Effect008_12A8
    Call BTLV_EFFECT_FIRST + 98, 5, 5

Effect008_12A8:
    McssSetVanish 5, FALSE
    McssSetValue 5, 0
    IfPokeExists 7, FALSE, Effect008_1330
    IfVarConst EFFVM_VAR_FAMOUS + 7, EFFVM_CMP_EQ, FALSE, Effect008_12F0
    Call BTLV_EFFECT_FIRST + 114, 7, 7
    Jump Effect008_1330

Effect008_12F0:
    IfVarConst EFFVM_VAR_RARE + 7, EFFVM_CMP_EQ, FALSE, Effect008_1310
    Call BTLV_EFFECT_FIRST + 47, 7, 7

Effect008_1310:
    IfVarConst EFFVM_VAR_N_POKEMON + 7, EFFVM_CMP_EQ, FALSE, Effect008_1330
    Call BTLV_EFFECT_FIRST + 98, 7, 7

Effect008_1330:
    McssSetVanish 7, FALSE
    McssSetValue 7, 0
    Wait EFFVM_WAIT_ALL
    IfPokeExists 1, FALSE, Effect008_1376
    McssMoveXY 1, 5, 0, 0, 0, 0, 0

Effect008_1376:
    IfPokeExists 3, FALSE, Effect008_13A2
    McssMoveXY 3, 5, 0, 0, 0, 0, 0

Effect008_13A2:
    IfPokeExists 5, FALSE, Effect008_13CE
    McssMoveXY 5, 5, 0, 0, 0, 0, 0

Effect008_13CE:
    IfPokeExists 7, FALSE, Effect008_13FA
    McssMoveXY 7, 5, 0, 0, 0, 0, 0

Effect008_13FA:
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
