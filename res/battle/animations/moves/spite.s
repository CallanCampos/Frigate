// MOVE_SPITE's animation: member 180 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Spite

Spite:
    CameraMovePreset 1, 14, 24, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W379_POWER, 2, EFFVM_PAN_CENTER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_PAN_CENTER, 150, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 40, 0, 130, 20, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 1, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_W029_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x7fff
    McssMoveXY EFFVM_DEFENDER, 2, 0, 0x4cd, 1, 2, 2
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W029_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x7fff
    McssMoveXY EFFVM_DEFENDER, 2, 0, 0x4cd, 1, 2, 2
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x7fff
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
