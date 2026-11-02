// MOVE_DESTINY_BOND's animation: member 194 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DestinyBond

DestinyBond:
    CameraMove 2, 0x3000, 0, 0x5000, 0, 0, 0, 32, 0, 28
    PaletteFade 4, 0, 16, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_ATTACKER, 0, -800, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, -800, 0, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 40, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 50, 0, 0
    SetYield TRUE
    ParticleLoad 364
    EmitterMove 364, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x32000, 0, 0x1000, 0x1000, 0
    WaitFrames 20
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W123_01, 0, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W194_01, 2, EFFVM_DEFENDER, 0, 500, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 60, 20, 0, 0
    MoveSE 2, 0, 0, 0, 1000, 0, 80, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x7bde
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x7bde
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x7bde
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x7bde
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 16, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
