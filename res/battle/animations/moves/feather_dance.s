// MOVE_FEATHER_DANCE's animation: member 297 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FeatherDance

FeatherDance:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W123_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W123_01, 2, EFFVM_ATTACKER, 40, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W123_01, 1, EFFVM_ATTACKER, 80, 0, 127, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 2, 0, 0x1000, 0x1000, 0x28000, 0, 0x3000, 0
    McssScale EFFVM_ATTACKER, 2, -0x666, 0x666, 20, 0, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W017_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_SYS_67, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    MoveSE 2, 0, 0, 0, -300, 0, 40, 0, 0
    PlaySE SEQ_SE_W017_01, 1, EFFVM_DEFENDER, 35, -200, 115, 0, 0, 0
    PlaySE SEQ_SE_SYS_67, 2, EFFVM_DEFENDER, 40, -100, 50, 0, 0, 0
    MoveSE 2, 0, 0, -300, -400, 45, 40, 0, 0
    PlaySE SEQ_SE_SYS_67, 2, EFFVM_DEFENDER, 80, -400, 5, 0, 0, 0
    MoveSE 2, 0, 0, -400, -1000, 80, 40, 0, 0
    SetYield TRUE
    ParticleLoad 467
    EmitterAdd 467, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
