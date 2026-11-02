// MOVE_ECHOED_VOICE's animation: member 497 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant EchoedVoice

EchoedVoice:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 10, 2, 32767
    Wait EFFVM_WAIT_CAMERA
    SetYield FALSE
    PlaySE SEQ_SE_W103_01, 1, EFFVM_ATTACKER, 0, -1000, 120, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 0, 0, 40, 0, 0
    PlaySE SEQ_SE_SHINKA_W025, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x19a, -0x19a, 2, 0, 12
    ParticleLoad 212
    EmitterAdd 212, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 212, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    ParticleLoad 669
    EmitterAdd 669, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    CameraMovePreset 1, 11, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 6, 0, 2
    Wait EFFVM_WAIT_MCSS
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 10, 0, 2, 32767
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    End
