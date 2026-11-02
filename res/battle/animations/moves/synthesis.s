// MOVE_SYNTHESIS's animation: member 235 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Synthesis

Synthesis:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W235_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_ATTACKER, 0, -400, 60, 0, 0, 0
    MoveSE 2, 0, 1, 60, 0, 60, 30, 0, 0
    MoveSE 2, 0, 0, -400, 300, 0, 90, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W235_01, 1, EFFVM_ATTACKER, 40, 0, 50, 0, 0, 0
    SetYield TRUE
    ParticleLoad 406
    EmitterAdd 406, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 406, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x294
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x294
    WaitFrames 7
    EmitterAdd 406, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 406, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x7fff
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x7fff
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
