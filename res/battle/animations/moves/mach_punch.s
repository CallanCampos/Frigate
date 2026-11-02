// MOVE_MACH_PUNCH's animation: member 183 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MachPunch

MachPunch:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_W307_01, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 50, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 0, 20, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 30, -200, 120, 0, 0, 0
    ParticleLoad 353
    EmitterAdd 353, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 2
    EmitterAdd 353, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 5
    EmitterAdd 353, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 353, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 3, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
