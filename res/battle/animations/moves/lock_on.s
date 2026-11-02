// MOVE_LOCK_ON's animation: member 199 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LockOn

LockOn:
    CameraMovePreset 1, 11, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W199_01, 1, EFFVM_DEFENDER, 0, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W199_01, 1, EFFVM_DEFENDER, 30, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W199_01, 1, EFFVM_DEFENDER, 60, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 2, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 369
    EmitterAdd 369, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 369, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 369, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 369, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    PaletteFade 4, 0, 12, 1, 32767
    WaitFrames 4
    PaletteFade 4, 12, 0, 1, 32767
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
