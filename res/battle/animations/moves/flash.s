// MOVE_FLASH's animation: member 148 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Flash

Flash:
    CameraMovePreset 1, 14, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 16, 0, 0
    PaletteFade 4, 0, 12, 1, 31710
    PlaySE SEQ_SE_W148_01, 1, EFFVM_ATTACKER, 0, 200, 100, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 1, EFFVM_ATTACKER, 20, 200, 50, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 1, EFFVM_ATTACKER, 40, 200, 20, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 1, EFFVM_ATTACKER, 60, 200, 5, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 0, 200, 100, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 20, 200, 50, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 40, 200, 20, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 60, 200, 5, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 16, 16, 0, 0
    PaletteFade 4, 12, 12, 0, 31710
    WaitFrames 50
    McssPalFade EFFVM_ATTACKER, 16, 0, 1, 0
    PaletteFade 4, 12, 0, 1, 31710
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
