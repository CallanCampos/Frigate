// MOVE_GRASS_WHISTLE's animation: member 320 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant GrassWhistle

GrassWhistle:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W320_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    ParticleLoad 494
    PaletteFade 4, 0, 6, 2, 960
    EmitterMove 494, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0, 0, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 6, 0, 2, 960
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
