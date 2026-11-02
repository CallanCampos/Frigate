// MOVE_QUICK_GUARD's animation: member 501 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant QuickGuard

QuickGuard:
    SetYield FALSE
    PlaySE SEQ_SE_W014_01, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_SYS_31, 2, EFFVM_ATTACKER, 20, 200, 127, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 21, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 2, -0x19a, 0x666, 20, 0, 1
    McssPalFade EFFVM_ATTACKER, 10, 0, 3, 0x7fff
    WaitFrames 6
    ParticleLoad 673
    EmitterAdd 673, 0, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 673, 1, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 673, 2, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 673, 3, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 673, 4, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
