// MOVE_DRAGON_DANCE's animation: member 349 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DragonDance

DragonDance:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 130, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 130, 0, 0
    MoveSE 1, 0, 1, 110, 0, 100, 30, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 130, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -2000, 4000, 0, 130, 0, 0
    MoveSE 2, 0, 1, 115, 0, 100, 30, 0, 0
    SetYield TRUE
    ParticleLoad 526
    EmitterAdd 526, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x1f
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 1, 2, 16
    WaitFrames 80
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x1f
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
