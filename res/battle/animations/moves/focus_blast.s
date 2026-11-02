// MOVE_FOCUS_BLAST's animation: member 411 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FocusBlast

FocusBlast:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 2, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W063_02, 1, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 0, 110, 0, 0
    MoveSE 1, 0, 1, 80, 0, 80, 30, 0, 0
    PlaySE SEQ_SE_W411_HAAA, 1, EFFVM_ATTACKER, 111, 300, 70, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 111, 40, 0, 0
    PlaySE SEQ_SE_W063_02, 2, EFFVM_ATTACKER, 111, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 170, 0, 0, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 111, 40, 0, 0
    MoveSE 2, 0, 1, 127, 0, 160, 10, 0, 0
    SetYield TRUE
    ParticleLoad 590
    EmitterAdd 590, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 590, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    LoadBg 24
    SetVanish 1, 1
    SetVanish 0, 1
    SetVanish 2, 0
    BgMove 4, -20, 0, 70, 1, 0
    EmitterMove 590, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x3c000, 0, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    BgMove 4, -20, 0, 100, 1, 0
    EmitterAdd 590, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W396_HADOKEN, 1, EFFVM_DEFENDER, 0, -200, 40, 0, 0, 0
    PlaySE SEQ_SE_FLD_37, 2, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x99a, 0, 2, 0, 8
    Wait EFFVM_WAIT_SE_PLAYER_1
    McssSetVanish EFFVM_DEFENDER, FALSE
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    PaletteFade 2, 13, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
