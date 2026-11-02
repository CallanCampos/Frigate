// MOVE_SHADOW_FORCE's animation: member 467 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant ShadowForce
    ScriptVariant ShadowForce_Variant1

ShadowForce:
    CameraMovePreset 1, 15, 24, 0, 0
    PaletteFade 4, 0, 12, 3, 16912
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_SYS_31, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_SYS_31, 1, EFFVM_ATTACKER, 15, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_SYS_31, 1, EFFVM_ATTACKER, 30, 0, 20, 0, 0, 0
    SetYield TRUE
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    Wait EFFVM_WAIT_ALL
    McssSetMode EFFVM_ATTACKER, 3
    CameraMovePreset 1, 17, 24, 0, 0
    PaletteFade 4, 12, 0, 3, 16912
    Wait EFFVM_WAIT_ALL
    End

ShadowForce_Variant1:
    CameraMovePreset 1, 14, 24, 0, 0
    PaletteFade 4, 0, 12, 3, 16912
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W225_02, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 30, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W379_POWER, 1, EFFVM_ATTACKER, 30, 0, 120, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 30, 40, 0, 0
    MoveSE 1, 0, 1, 120, 0, 50, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 647
    EmitterAddOrtho 647, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 647, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 647, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 9
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 2
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 2
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 3, 16912
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
