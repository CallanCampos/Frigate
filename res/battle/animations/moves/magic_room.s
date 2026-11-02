// MOVE_MAGIC_ROOM's animation: member 478 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MagicRoom

MagicRoom:
    LoadBg 198
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W470_WA, 1, EFFVM_PAN_CENTER, 0, 470, 70, 0, 0, 0
    MoveSE 1, 0, 1, 70, 0, 80, 30, 0, 0
    PlaySE SEQ_SE_W054_01, 2, EFFVM_PAN_CENTER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    PaletteFade 3, 15, 0, 1, 0
    McssSetValue EFFVM_ALL, 1
    BgMove 1, 0, -24, 80, 0, 0
    McssSetVanish EFFVM_ALL, TRUE
    McssScale EFFVM_ALL, 2, -0x1000, -0x1000, 1, 0, 40
    McssMoveXY EFFVM_ALL, 2, 0x800, 0, 60, 0, 1
    Wait EFFVM_WAIT_BG
    SetYield FALSE
    PaletteFade 3, 0, 15, 1, 0
    PaletteFade 2, 15, 0, 1, 0
    SetVanish 0, 0
    SetVanish 1, 0
    McssSetVanish EFFVM_ALL, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
