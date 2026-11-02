// MOVE_SKY_DROP's animation: member 507 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant SkyDrop
    ScriptVariant SkyDrop_Variant1

SkyDrop:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 1, 0x666, 0x666, 24, 0, 0
    WaitFrames 8
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_ATTACKER, 0, 200, 127, 0, 0, 0
    MoveSE 1, 0, 0, 200, -400, 1, 50, 0, 0
    MoveSE 1, 0, 1, 127, 0, 30, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 2, EFFVM_DEFENDER, 30, -200, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 50, 0, 0
    SetYield TRUE
    McssSetMode EFFVM_ATTACKER, 3
    ParticleLoad 460
    EmitterAddOrtho 460, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 460, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 460, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 30
    ParticleDelete 460
    ParticleLoad 460
    CameraMovePreset 1, 11, 12, 0, 8
    EmitterMove 460, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x10000, 0, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 12, -100, 70, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 24, -200, 30, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 36, -300, 20, 0, 0, 0
    PlaySE SEQ_SE_W240_01, 1, EFFVM_DEFENDER, 42, -2000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 75, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 2000, 42, 20, 0, 0
    PlaySE SEQ_SE_W123_01, 2, EFFVM_DEFENDER, 55, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    McssSetMode EFFVM_DEFENDER, 3
    ParticleLoad 184
    EmitterAdd 184, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 48
    CameraMove 2, 0, 0, 0, 0, 0xa000, 0, 12, 0, 10
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 24, 0, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

SkyDrop_Variant1:
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W088_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 32, -200, 127, 0, 0, 0
    SetYield FALSE
    ParticleLoad 185
    EmitterAdd 185, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 185, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 185, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 0, 4
    McssSetMode EFFVM_DEFENDER, 4
    McssSetMode EFFVM_ATTACKER, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
