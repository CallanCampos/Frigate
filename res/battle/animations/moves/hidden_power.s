// MOVE_HIDDEN_POWER's animation: member 237 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HiddenPower

HiddenPower:
    PaletteFade 4, 0, 16, 2, 0
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 1
    SetVanish 0, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W157_02, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 80, 0, 0
    MoveSE 2, 0, 0, -1000, 2000, 0, 80, 0, 0
    SetYield TRUE
    ParticleLoad 408
    EmitterAdd 408, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W319_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    EmitterAdd 408, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 408, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 408, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 408, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    SetVanish 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 2, 0
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
