// MOVE_ROCK_SMASH's animation: member 249 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockSmash

RockSmash:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W088_02, 1, EFFVM_DEFENDER, 0, -1000, 115, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 3, 8, 0, 0
    PlaySE SEQ_SE_W307_EXP, 2, EFFVM_DEFENDER, 11, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 17, -200, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 419
    EmitterAdd 419, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 419, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x666, -0x666, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
