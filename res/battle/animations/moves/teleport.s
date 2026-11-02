// MOVE_TELEPORT's animation: member 100 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Teleport

Teleport:
    PaletteFade 2, 0, 15, 0, 0
    CameraMovePreset 1, 15, 24, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W179_03, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 120, 0, 30, 20, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W100_02, 2, EFFVM_ATTACKER, 20, 200, 75, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_ATTACKER, 0, 60, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 1, 0x19a, 0x1000, 6, 0, 0
    WaitFrames 3
    McssMoveXY EFFVM_ATTACKER, 2, 0x666, 0, 2, 0, 2
    WaitFrames 8
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0xa000, 20, 0, 0
    ParticleLoad 272
    EmitterAdd 272, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetMode EFFVM_ATTACKER, 3
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 20, 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_SE_PLAYER_1
    PaletteFade 2, 15, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
