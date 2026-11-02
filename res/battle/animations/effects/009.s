// Effect BTLV_EFFECT_FIRST + 9 (0x23a): member 9 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect009

Effect009:
    Gauge 0, 0
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 48
    CameraMovePreset 1, 11, 200, 0, 0
    EmitterMoveFrom 48, 0, 2, 0, -0x2000, 0xa000, EFFVM_EMIT_DEFENDER, 0x2666, 0x23000, 0x5000, 0x1000, 0x1000, 0
    IfVarConst EFFVM_VAR_CRITICAL, EFFVM_CMP_NE, 1, Effect009_00F2
    PlaySE SEQ_SE_NAGERU2, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    Jump Effect009_0118

Effect009_00F2:
    PlaySE SEQ_SE_NAGERU, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0

Effect009_0118:
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 35, 0, 0
    WaitFrames 40
    CameraMove 2, -0x44cd, 0x2000, -0x7000, 0x59a, 0, -0x7000, 10, 0, 0
    Wait EFFVM_WAIT_CAMERA
    ParticleLoad 49
    ParticleLoad 29
    EmitterAddAll 29, EFFVM_EMIT_DEFENDER, 8, 0x2666, 0, 4096, 0x800, 0x1000, 0x800, 0x1000
    EmitterAdd 49, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xe8f, 0x1000
    EmitterAdd 49, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xe8f, 0x1000
    ParticleDelete 48
    PlaySE SEQ_SE_BOWA2, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    McssPalFade EFFVM_DEFENDER, 0, 16, 1, 0x7fff
    McssSetValue EFFVM_DEFENDER, 1
    Wait EFFVM_WAIT_MCSS
    McssMoveXY EFFVM_DEFENDER, 1, 0, 0x2800, 20, 0, 0
    McssScale EFFVM_DEFENDER, 1, 0, 0, 20, 0, 0
    Wait EFFVM_WAIT_MCSS
    McssSetMode EFFVM_DEFENDER, 3
    McssMoveXY EFFVM_DEFENDER, 1, 0, -0x2800, 0, 0, 0
    ParticleLoad 46
    EmitterAdd 46, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xe7f, 0x1000
    ParticleDelete 49
    ParticleDelete 29
    WaitFrames 20
    BallAdd 0, EFFVM_DEFENDER, -0x4, 0x2785, 0xb0a, 0xb0a
    ClactPalFade 0, 16, 0, 2, 0x73ff
    Wait EFFVM_WAIT_ALL
    ParticleDelete 46
    IfVarConst EFFVM_VAR_CRITICAL, EFFVM_CMP_NE, 1, Effect009_03A2
    ClactMove 0, 3, 0x1, 0, 1, 0, 2
    PlaySE SEQ_SE_SYS_91, 5, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 16

Effect009_03A2:
    WaitFrames 1
    ClactMove 0, 1, 0, 0x28, 12, 0, 0
    ClactSetAnim 0, 4
    WaitFrames 1
    SetYield FALSE
    BallAdd 1, EFFVM_DEFENDER, 0, 0x2b8, 0x666, 0x333
    ClactPalFade 1, 16, 16, 0, 0x2529
    ClactScale 1, 0, 0xb33, 0x4cd, 0, 0, 0
    WaitFrames 4
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_KON, 5, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    ClactMove 0, 1, 0, -0x8, 4, 0, 0
    WaitFrames 4
    ClactMove 0, 1, 0, -0x2, 3, 0, 0
    WaitFrames 3
    ClactMove 0, 1, 0, 0x2, 3, 0, 0
    WaitFrames 3
    ClactMove 0, 1, 0, 0x8, 4, 0, 0
    WaitFrames 2
    PlaySE SEQ_SE_KON, 5, EFFVM_DEFENDER, 0, 0, 96, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    ClactMove 0, 1, 0, -0x3, 1, 0, 0
    WaitFrames 1
    ClactMove 0, 1, 0, -0x1, 1, 0, 0
    WaitFrames 1
    ClactMove 0, 1, 0, 0x1, 1, 0, 0
    WaitFrames 1
    ClactMove 0, 1, 0, 0x3, 1, 0, 0
    WaitFrames 1
    PlaySE SEQ_SE_KON, 5, EFFVM_DEFENDER, 0, 0, 64, 0, 0, 0
    WaitFrames 40
    Wait EFFVM_WAIT_ALL
    IfVarConst EFFVM_VAR_CRITICAL, EFFVM_CMP_EQ, 1, Effect009_0688
    IfVarConst EFFVM_VAR_SHAKES, EFFVM_CMP_EQ, 0, Effect009_0766
    ClactSetAnim 0, 1
    PlaySE SEQ_SE_BOWA1, 5, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    WaitFrames 65
    Wait EFFVM_WAIT_ALL
    IfVarConst EFFVM_VAR_SHAKES, EFFVM_CMP_EQ, 1, Effect009_0766
    ClactSetAnim 0, 2
    PlaySE SEQ_SE_BOWA1, 5, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    WaitFrames 70
    Wait EFFVM_WAIT_ALL

Effect009_0688:
    ClactSetAnim 0, 3
    PlaySE SEQ_SE_BOWA1, 5, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    WaitFrames 75
    Wait EFFVM_WAIT_ALL
    IfVarConst EFFVM_VAR_CAUGHT, EFFVM_CMP_EQ, 0, Effect009_0766
    PlaySE SEQ_SE_GETTING, 5, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    ClactPalFade 0, 0, 8, -2, 0
    ParticleLoad 28
    EmitterAddOrtho 28, 0, EFFVM_EMIT_DEFENDER, 8, 0, -0x800, 0, 0x800, 0xccd, 0x1000, 0xb33
    Wait EFFVM_WAIT_ALL
    WaitExternal
    CameraMovePreset 1, 8, 40, 0, 0
    End

Effect009_0766:
    ParticleLoad 49
    EmitterAdd 49, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x400, 0x1000, 0x1000
    WaitFrames 21
    ClactDelete 0
    ClactDelete 1
    ParticleLoad 3
    EmitterAddAll 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 4096, 0xc00, 0xccd, 0xc00, 0x1000
    PlaySE SEQ_SE_BOWA2, 5, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    McssSetMode EFFVM_DEFENDER, 4
    PlayCry EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    McssScale EFFVM_DEFENDER, 1, 0x1000, 0x1000, 20, 0, 0
    McssPalFade EFFVM_DEFENDER, 16, 0, 1, 0x7fff
    McssSetValue EFFVM_DEFENDER, 0
    Gauge 1, 0
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 8, 20, 0, 0
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_CRY
    End
