// MOVE_MIRROR_COAT's animation: member 243 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MirrorCoat

MirrorCoat:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_ATTACKER, 45, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 414
    EmitterAdd 414, 2, EFFVM_EMIT_ATTACKER, 8, 0x1666, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 414, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 10
    EmitterAdd 414, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 40
    EmitterAdd 414, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
