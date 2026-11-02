// MOVE_TRI_ATTACK's animation: member 161 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant TriAttack

TriAttack:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W235_03, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W235_03, 1, EFFVM_ATTACKER, 10, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 329
    ParticleLoad 330
    ParticleLoad 331
    EmitterAdd 329, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 330, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 331, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 329, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 330, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    MoveSE 2, 0, 1, 110, 0, 30, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 10, -500, 125, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 15, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 20, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 25, -500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 329, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 40
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    EmitterAdd 331, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 40
    SetYield FALSE
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 0, 300, 100, 0, 0, 0
    SetYield TRUE
    EmitterAdd 330, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
