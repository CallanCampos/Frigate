// Effect BTLV_EFFECT_FIRST + 95 (0x290): member 95 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect095

Effect095:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_KOUKA_M, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    End
    .balign 4
