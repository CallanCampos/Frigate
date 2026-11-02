// MOVE_WEATHER_BALL's animation: member 311 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 5
    ScriptVariant WeatherBall
    ScriptVariant WeatherBall_Variant1
    ScriptVariant WeatherBall_Variant2
    ScriptVariant WeatherBall_Variant3
    ScriptVariant WeatherBall_Variant4

WeatherBall:
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 482
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_DEFENDER, 65, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 125, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 65, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 105, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 100, 0, 115, 0, 0, 0
    SetYield TRUE
    EmitterAdd 482, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 482, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 482, 3, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 482, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 482, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

WeatherBall_Variant1:
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 483
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_DEFENDER, 65, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 125, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 65, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 105, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 100, 0, 115, 0, 0, 0
    SetYield TRUE
    EmitterAdd 483, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 483, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 483, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 483, 2, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 483, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

WeatherBall_Variant2:
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 484
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_DEFENDER, 65, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 125, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 65, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 105, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 100, 0, 115, 0, 0, 0
    SetYield TRUE
    EmitterAdd 484, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 484, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 484, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 484, 2, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 484, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

WeatherBall_Variant3:
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 485
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_DEFENDER, 65, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 125, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 65, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 105, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 100, 0, 115, 0, 0, 0
    SetYield TRUE
    EmitterAdd 485, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 485, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 485, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 485, 2, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 485, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

WeatherBall_Variant4:
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 486
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_DEFENDER, 65, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 125, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 65, 50, 0, 0
    MoveSE 1, 0, 1, 115, 0, 105, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 100, 0, 115, 0, 0, 0
    SetYield TRUE
    EmitterAdd 486, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 486, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 486, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 486, 2, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 486, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
