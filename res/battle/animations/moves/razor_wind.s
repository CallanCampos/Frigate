// MOVE_RAZOR_WIND's animation: member 13 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant RazorWind
    ScriptVariant RazorWind_Variant1

RazorWind:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 1500, 0, 70, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 50, 30, 0, 0
    SetYield TRUE
    ParticleLoad 177
    EmitterAdd 177, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 177, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_ATTACKER, 2, 0x800, 0, 2, 0, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End

RazorWind_Variant1:
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 5, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 10, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 15, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 20, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 25, 0, 110, 0, 0, 0
    MoveSE 1, 0, 0, 1000, 0, 0, 30, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 178
    EmitterAdd 178, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 5
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
