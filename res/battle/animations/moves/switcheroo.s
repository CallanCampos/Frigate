// MOVE_SWITCHEROO's animation: member 415 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Switcheroo

Switcheroo:
    CameraMove 2, 0x5000, 0x5000, 0x3000, 0, 0, 0, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 3, 0, 0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 12
    PaletteFade 4, 0, 12, 1, 0
    WaitFrames 12
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 20, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, 0, 0, 20, 0, 0
    PlaySE SEQ_SE_TB_KARA, 1, EFFVM_PAN_CENTER, 20, -200, 115, 0, 0, 0
    PlaySE SEQ_SE_TB_KARA, 1, EFFVM_PAN_CENTER, 40, -200, 115, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_015, 2, EFFVM_PAN_CENTER, 65, -500, 110, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_015, 2, EFFVM_PAN_CENTER, 95, -500, 110, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_015, 2, EFFVM_PAN_CENTER, 130, -500, 110, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 140, 30, 0, 0
    ParticleLoad 594
    EmitterAddAt 594, 0, 0, 0, -0x3000, 0, 0, 0, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 594, 1, 0, 0, -0x3000, 0, 0, 0, 0xb33, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 594, 2, 0, 0, -0x3000, 0, 0, 0, 0xb33, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 594, 3, 0, 0, -0x3000, 0, 0, 0, 0xb33, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 594, 4, 0, 0, -0x3000, 0, 0, 0, 0xb33, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 0
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
