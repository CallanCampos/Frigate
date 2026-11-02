// MOVE_PSYCHO_CUT's animation: member 427 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PsychoCut

PsychoCut:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W379_POWER, 1, EFFVM_ATTACKER, 0, -500, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -500, 2000, 0, 79, 0, 0
    MoveSE 1, 0, 1, 120, 0, 70, 9, 0, 0
    PlaySE SEQ_SE_W077_03, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W229_KIIN, 1, EFFVM_ATTACKER, 80, -500, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 75, 25, 0, 0
    SetYield TRUE
    ParticleLoad 606
    EmitterAdd 606, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 100, -700, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 110, -700, 60, 0, 0, 0
    SetYield TRUE
    WaitFrames 5
    EmitterAdd 606, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 606, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    EmitterMove 606, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 30
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
