// MOVE_FREEZE_SHOCK's animation: member 553 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant FreezeShock
    ScriptVariant FreezeShock_Variant1

FreezeShock:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_ATTACKER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_127, 2, EFFVM_ATTACKER, 20, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 726
    EmitterAdd 726, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 726, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 726, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 726, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

FreezeShock_Variant1:
    LoadBg 219
    SetVanish 2, 0
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 16, 2, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    ParticleLoad 727
    ParticleLoad 728
    ParticleLoad 729
    EmitterAdd 728, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 729, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 727, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 0, EFFVM_ATTACKER, 15, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W063_02, 2, EFFVM_ATTACKER, 70, 0, 70, 0, 0, 0
    MoveSE 2, 0, 0, 0, 1000, 70, 30, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 71, 30, 0, 0
    SetYield TRUE
    WaitFrames 60
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterMove 727, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x800, 0x1000, 0
    EmitterMove 728, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x800, 0x1000, 0
    EmitterMove 729, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x800, 0x1000, 0
    SetVanish 1, 1
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W465_01, 1, EFFVM_DEFENDER, 10, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 1, EFFVM_DEFENDER, 30, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 1, EFFVM_DEFENDER, 60, 0, 55, 0, 0, 0
    PlaySE SEQ_SE_FLD_132, 2, EFFVM_DEFENDER, 10, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_FLD_69, 3, EFFVM_DEFENDER, 0, 0, 50, 0, 0, 0
    MoveSE 3, 0, 0, 0, 5000, 0, 100, 0, 0
    MoveSE 3, 0, 1, 50, 0, 70, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    SetYield TRUE
    PaletteFade 3, 16, 0, 2, 0
    WaitFrames 3
    BgPalAnim 38, 6
    BgMove 4, 8, 0, 80, 0, 0
    McssCircle EFFVM_DEFENDER, 2, 2, 0, 0x2000, 0xa000, 0, 0x1000, 0
    McssScale EFFVM_DEFENDER, 3, 0x4cd, -0x4cd, 2, 0, 3
    EmitterAdd 727, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 728, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 727, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 16, 2, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
