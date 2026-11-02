// MOVE_MAGIC_COAT's animation: member 277 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MagicCoat

MagicCoat:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W113_01, 1, EFFVM_ATTACKER, 20, -80, 127, 0, 0, 0
    PlaySE SEQ_SE_W006_04, 2, EFFVM_ATTACKER, 70, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W113_02, 1, EFFVM_ATTACKER, 70, 0, 70, 0, 0, 0
    SetYield TRUE
    ParticleLoad 447
    EmitterAdd 447, 2, EFFVM_EMIT_ATTACKER, 8, 0x1800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 447, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 16
    EmitterAdd 447, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 40
    EmitterAdd 447, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
