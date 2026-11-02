// MOVE_AURA_SPHERE's animation: member 396 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AuraSphere

AuraSphere:
    LoadBg 27
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 2, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, -22, 0, 150, 0, 0
    SetVanish 1, 1
    SetVanish 0, 1
    PaletteFade 4, 15, 0, 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, 0, 3000, 0, 80, 0, 0
    MoveSE 3, 0, 1, 120, 0, 60, 20, 0, 0
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W396_HADOKEN, 1, EFFVM_ATTACKER, 80, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_FLD_37, 2, EFFVM_DEFENDER, 100, 0, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 75, 30, 0, 0
    MoveSE 1, 0, 1, 100, 0, 80, 30, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 75, 30, 0, 0
    SetYield TRUE
    ParticleLoad 575
    EmitterAdd 575, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 575, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    EmitterAdd 575, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterMove 575, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 575, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 8, 10, 0, 8
    WaitFrames 12
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 2, 1, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 1, 4
    PaletteFade 4, 0, 15, 2, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    SetVanish 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 2, 1
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 15, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
