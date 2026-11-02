// MOVE_BONE_RUSH's animation: member 198 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BoneRush

BoneRush:
    SetYield FALSE
    PlaySE SEQ_SE_W014_02, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    ParticleLoad 368
    EmitterAdd 368, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 368, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
