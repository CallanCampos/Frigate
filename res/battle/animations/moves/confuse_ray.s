// MOVE_CONFUSE_RAY's animation: member 109 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ConfuseRay

ConfuseRay:
    CameraMovePreset 1, 20, 24, 0, 0
    PaletteFade 4, 0, 12, 2, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 278
    EmitterMove 278, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x78000, 0, 0x1333, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_SYS_65, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_FLD_103, 2, EFFVM_ATTACKER, 15, 200, 60, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    WaitFrames 30
    CameraMovePreset 1, 11, 32, 0, 28
    WaitFrames 30
    EmitterAdd 278, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W278_01, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY, 1, EFFVM_DEFENDER, 130, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 80, 50, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 2, 0
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 2, 1
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
