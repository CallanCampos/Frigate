// Effect BTLV_EFFECT_FIRST + 60 (0x26d): member 60 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect060

Effect060:
    SetYield FALSE
    McssReset TRUE
    SetValue 3
    McssSetMode EFFVM_ATTACKER, 3
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Effect060_073E
    McssScale EFFVM_ATTACKER, 0, 0, 0, 0, 0, 0
    IfVarConst EFFVM_VAR_ATTACKER_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect060_00F2
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 52, EFFVM_CMP_EQ, 1, Effect060_00F2
    SetValue 0

Effect060_00F2:
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetValue EFFVM_ATTACKER, 1
    SetYield TRUE
    ParticleLoad 325
    PlaySE SEQ_SE_NAGERU, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield FALSE
    SetBallSource 9
    ParticleLoad 48
    EmitterAddOrtho 48, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 25
    SetBallSource 9
    ParticleDelete 48
    SetYield FALSE
    PlaySE SEQ_SE_BOWA2, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetBallSource 9
    ParticleLoad 3
    EmitterAddAll 3, EFFVM_EMIT_ATTACKER, 8, 0x4000, 0, 4096, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ATTACKER, 16, 16, 0, 0x7fff
    McssScale EFFVM_ATTACKER, 1, 0x1333, 0x1333, 32, 0, 0
    McssSetMode EFFVM_ATTACKER, 4
    PlayCry EFFVM_ATTACKER, 0x80000000, 0, 0, 0, 0, 0
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    McssPalFade EFFVM_ATTACKER, 16, 0, 1, 0x7fff
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 16, 0, 0
    IfVarConst EFFVM_VAR_ATTACKER_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect060_02B8
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x1000, 16, 0, 0

Effect060_02B8:
    Wait EFFVM_WAIT_ALL
    IfVarConst EFFVM_VAR_ATTACKER_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect060_0676
    SetYield FALSE
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 52, EFFVM_CMP_EQ, 1, Effect060_0376
    IfVarConst EFFVM_VAR_ATTACKER_WEIGHT, EFFVM_CMP_LT, 1500, Effect060_034C
    EmitterAdd 325, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect060_034C:
    IfVarConst EFFVM_VAR_ATTACKER_WEIGHT, EFFVM_CMP_LT, 500, Effect060_0376
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 0, Effect060_0376
    SetValue 1

Effect060_0376:
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    IfVarConst 52, EFFVM_CMP_NE, 1, Effect060_03AE
    McssShake EFFVM_ATTACKER, 1, 0x7fff800, 0xffff000, 0x800, 24

Effect060_03AE:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 2, Effect060_03E6
    PlaySE SEQ_SE_TOUJOU_LARGE, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0

Effect060_03E6:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 1, Effect060_041E
    PlaySE SEQ_SE_TOUJOU_MID, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0

Effect060_041E:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 0, Effect060_0456
    PlaySE SEQ_SE_TOUJOU_SMALL, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0

Effect060_0456:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 2, Effect060_0532
    CameraMove 2, 0, 0x1000, 0, 0, 0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x1000, 0, 0, -0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, 0x1000, 0, 0, 0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x1000, 0, 0, -0x1000, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA

Effect060_0532:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 1, Effect060_0604
    CameraMove 2, 0, 0x800, 0, 0, 0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x800, 0, 0, -0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, 0x800, 0, 0, 0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x800, 0, 0, -0x800, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA

Effect060_0604:
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_NE, 0, Effect060_0676
    CameraMove 2, 0, 0x400, 0, 0, 0x400, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMove 2, 0, -0x400, 0, 0, -0x400, 0, 2, 0, 0
    Wait EFFVM_WAIT_CAMERA

Effect060_0676:
    Wait EFFVM_WAIT_CRY
    SetYield FALSE
    IfVarConst EFFVM_VAR_ATTACKER_FAMOUS, EFFVM_CMP_EQ, FALSE, Effect060_06A8
    Call BTLV_EFFECT_FIRST + 114, EFFVM_ATTACKER, EFFVM_DEFENDER
    Jump Effect060_06E8

Effect060_06A8:
    IfVarConst EFFVM_VAR_ATTACKER_RARE, EFFVM_CMP_EQ, FALSE, Effect060_06C8
    Call BTLV_EFFECT_FIRST + 47, EFFVM_ATTACKER, EFFVM_DEFENDER

Effect060_06C8:
    IfVarConst EFFVM_VAR_ATTACKER_N_POKEMON, EFFVM_CMP_EQ, FALSE, Effect060_06E8
    Call BTLV_EFFECT_FIRST + 98, EFFVM_ATTACKER, EFFVM_DEFENDER

Effect060_06E8:
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetValue EFFVM_ATTACKER, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End

Effect060_073E:
    SetValue 3
    McssScale EFFVM_ATTACKER, 0, 0, 0, 0, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetValue EFFVM_ATTACKER, 1
    IfVarConst EFFVM_VAR_ATTACKER_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect060_07BE
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0x4000, 1, 0, 0
    IfVarConst 52, EFFVM_CMP_EQ, 1, Effect060_07BE
    SetValue 0

Effect060_07BE:
    SetYield TRUE
    ParticleLoad 325
    PlaySE SEQ_SE_NAGERU, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield FALSE
    SetBallSource 9
    ParticleLoad 48
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_EQ, 0, Effect060_095E
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_EQ, 3, Effect060_095E
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_EQ, 2, Effect060_08C2
    IfVarConst EFFVM_VAR_ATTACKER, EFFVM_CMP_EQ, 4, Effect060_0886
    EmitterMoveFromOrtho 48, 0, 3, -0x5000, 0x1e66, 0xc000, EFFVM_EMIT_ATTACKER, 0, 0x23000, 0x5000, 0x1000, 0x1000, 0x1000
    Jump Effect060_0994

Effect060_0886:
    EmitterMoveFromOrtho 48, 0, 3, -0x2800, 0x1e66, 0xc000, EFFVM_EMIT_ATTACKER, 0, 0x23000, 0x5000, 0x1000, 0x1000, 0x1000
    Jump Effect060_0994

Effect060_08C2:
    IfVarConst EFFVM_VAR_ATTACKER, EFFVM_CMP_EQ, 4, Effect060_095E
    IfVarConst EFFVM_VAR_ATTACKER, EFFVM_CMP_EQ, 6, Effect060_0922
    EmitterMoveFromOrtho 48, 0, 3, -0x7400, 0x1e66, 0xc000, EFFVM_EMIT_ATTACKER, 0, 0x23000, 0x5000, 0x1000, 0x1000, 0x1000
    Jump Effect060_0994

Effect060_0922:
    EmitterMoveFromOrtho 48, 0, 3, 0, 0x1e66, 0xc000, EFFVM_EMIT_ATTACKER, 0, 0x23000, 0x5000, 0x1000, 0x1000, 0x1000
    Jump Effect060_0994

Effect060_095E:
    EmitterMoveFromOrtho 48, 0, 3, -0x2800, 0x1e66, 0xc000, EFFVM_EMIT_ATTACKER, 0x1000, 0x23000, 0x5000, 0x1000, 0x1000, 0x1000

Effect060_0994:
    SetYield TRUE
    WaitFrames 60
    SetBallSource 9
    ParticleDelete 48
    SetYield FALSE
    PlaySE SEQ_SE_BOWA2, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetBallSource 9
    ParticleLoad 3
    EmitterAddAll 3, EFFVM_EMIT_ATTACKER, 8, 0x4000, 0, 4096, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 16, 16, 0, 0x7fff
    McssScale EFFVM_ATTACKER, 1, 0x1333, 0x1333, 32, 0, 0
    McssSetMode EFFVM_ATTACKER, 4
    PlayCry EFFVM_ATTACKER, 0x80000000, 0, 0, 0, 0, 0
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    McssPalFade EFFVM_ATTACKER, 16, 0, 1, 0x7fff
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 16, 0, 0
    IfVarConst EFFVM_VAR_ATTACKER_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect060_0ADA
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x1000, 16, 0, 0

Effect060_0ADA:
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    IfVarConst EFFVM_VAR_ATTACKER_NO_BOUNCE, EFFVM_CMP_EQ, TRUE, Effect060_0B98
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x3000, 10, 0, 0
    IfVarConst 52, EFFVM_CMP_EQ, 1, Effect060_0B98
    IfVarConst EFFVM_VAR_ATTACKER_WEIGHT, EFFVM_CMP_LT, 1500, Effect060_0B6E
    EmitterAdd 325, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetValue 2

Effect060_0B6E:
    IfVarConst EFFVM_VAR_ATTACKER_WEIGHT, EFFVM_CMP_LT, 500, Effect060_0B98
    IfVarConst EFFVM_VAR_VALUE, EFFVM_CMP_EQ, 2, Effect060_0B98
    SetValue 1

Effect060_0B98:
    Jump Effect060_0376
    .balign 4
