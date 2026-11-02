// MOVE_FUTURE_SIGHT's animation: member 248 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant FutureSight
    ScriptVariant FutureSight_Variant1

FutureSight:
    CameraMovePreset 1, 15, 24, 0, 0
    PaletteFade 4, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W248_TIKIRIN, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x7fff
    McssMoveXY EFFVM_ATTACKER, 2, 0x800, 0, 4, 0, 3
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x7fff
    PaletteFade 4, 13, 0, 2, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End

FutureSight_Variant1:
    CameraMovePreset 1, 16, 24, 0, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 12
    PaletteFade 4, 0, 13, 2, 0
    PlaySE SEQ_SE_W307_03, 0, EFFVM_PAN_CENTER, 0, 0, 120, 0, 0, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    CameraShake 0, 0x333, 0, 1, 0, 4
    McssScale EFFVM_DEFENDER, 3, -0x666, 0x666, 2, 2, 1
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x7fff
    McssMoveXY EFFVM_DEFENDER, 2, 0xb33, 0, 2, 0, 4
    WaitFrames 14
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x7fff
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
