// MOVE_MAGNET_RISE's animation: member 393 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MagnetRise

MagnetRise:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_SYS_26, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1500, 0, 64, 0, 0
    MoveSE 1, 0, 1, 70, 0, 40, 30, 0, 0
    PlaySE SEQ_SE_W194_01, 2, EFFVM_ATTACKER, 30, 200, 125, 0, 0, 0
    MoveSE 2, 0, 1, 125, 0, 60, 50, 0, 0
    SetYield TRUE
    ParticleLoad 572
    EmitterAdd 572, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 572, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssCircle EFFVM_ATTACKER, 4, 2, 0, 0x1000, 0x1e000, 0x1000, 0x2000, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
