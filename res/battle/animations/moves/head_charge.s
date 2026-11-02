// MOVE_HEAD_CHARGE's animation: member 543 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HeadCharge

HeadCharge:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 10, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 41, 0, 0, 0, 0, 0
    SetYield TRUE
    LoadBg 12
    SetVanish 2, 0
    McssCircle EFFVM_ATTACKER, 2, 2, 0, 0x2000, 0xa000, 0, 0x1000, 0
    PaletteFade 4, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_FLD_74, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_SYS_64, 0, EFFVM_DEFENDER, 5, 0, 127, 0, 0, 0
    SetYield TRUE
    BgMove 2, 0, 8, 3, 0, 10
    ParticleLoad 712
    EmitterAdd 712, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 712, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 712, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x666, -0x666, 6, 0, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x1000, 0, 3, 0, 10
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 16, 0, 1, 0
    BgPalAnim 0, 3
    WaitFrames 40
    PaletteFade 4, 16, 0, 1, 0
    SetVanish 0, 0
    SetVanish 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
