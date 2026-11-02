// MOVE_TRICK_ROOM's animation: member 433 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant TrickRoom

TrickRoom:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 165
    SetVanish 2, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 15, 0, 1, 0
    SetVanish 1, 1
    SetVanish 0, 1
    PlaySE SEQ_SE_W295_02, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    WaitFrames 50
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 15, 0, 1, 0
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
