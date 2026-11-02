// MOVE_BUG_BUZZ's animation: member 405 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BugBuzz

BugBuzz:
    CameraMovePreset 1, 9, 10, 0, 6
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_MSCL_03, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_MSCL_03, 1, EFFVM_ATTACKER, 50, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    MoveSE 2, 0, 0, 0, 1000, 0, 90, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 75, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 584
    EmitterAdd 584, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 584, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 584, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 584, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 584, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 10, 0, 6
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 6
    SetYield FALSE
    PlaySE SEQ_SE_W194_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 20, 30, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 10, 0, 6
    Wait EFFVM_WAIT_ALL
    End
