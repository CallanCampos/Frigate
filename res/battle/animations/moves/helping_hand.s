// MOVE_HELPING_HAND's animation: member 270 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HelpingHand

HelpingHand:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W003_01, 1, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 440
    EmitterAdd 440, 0, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 440, 1, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 440, 6, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 6, 0, 1
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W003_01, 1, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAdd 440, 3, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 440, 4, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 440, 6, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 6, 0, 1
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W003_01, 1, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAdd 440, 5, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 440, 2, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 440, 6, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
