// MOVE_STORED_POWER's animation: member 500 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant StoredPower

StoredPower:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_FLD_05, 1, EFFVM_ATTACKER, 0, -1000, 70, 0, 0, 0
    PlaySE SEQ_SE_SYS_73, 1, EFFVM_ATTACKER, 42, 0, 127, 0, 0, 0
    SetYield TRUE
    PaletteFade 2, 0, 10, 2, 0
    McssPalFade EFFVM_ATTACKER, 0, 10, 0, 0x1f
    Wait EFFVM_WAIT_ALL
    ParticleLoad 672
    EmitterAdd 672, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 672, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 10, 0, 0, 0x1f
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
