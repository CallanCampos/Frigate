// MOVE_ROLE_PLAY's animation: member 272 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RolePlay

RolePlay:
    CameraMovePreset 1, 15, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 4, 0, 12, 1, 0
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 5, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 10, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 15, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 20, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 25, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 30, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 35, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W096_01, 1, EFFVM_ATTACKER, 43, 0, 120, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, -0x2800, 0, 1, 0, 24
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x7fff
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x7fff
    WaitFrames 30
    McssScale EFFVM_ATTACKER, 3, 0x333, -0x666, 3, 1, 1
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x7fff
    CameraMovePreset 1, 16, 24, 0, 0
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x7fff
    WaitFrames 30
    PaletteFade 4, 12, 0, 1, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
