// MOVE_LUNAR_DANCE's animation: member 461 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LunarDance

LunarDance:
    LoadBg 186
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 0, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 15, 0, 0, 0
    BgMove 4, 0, 2, 80, 0, 0
    ParticleLoad 640
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 10, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 30, 0, 60, 0, 0, 0
    SetYield TRUE
    EmitterAdd 640, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 640, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 640, 2, EFFVM_EMIT_ATTACKER, 8, 0x4cd, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 640, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 15, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 15, 0, 0, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
