// MOVE_AROMATHERAPY's animation: member 312 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Aromatherapy

Aromatherapy:
    CameraMove 2, 0x3000, 0, 0x6000, 0, 0, 0, 22, 0, 18
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 487
    EmitterAddAt 487, 2, 0, 0, 0, 0, 0, 0, 0x2000, 0, 0, 0x1333, 0x1000, 0x1000, 0x1000
    EmitterAddAt 487, 0, 0, 0, 0, 0, 0, 0, 0x2000, 0, 0, 0x1333, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W312_HARP, 1, EFFVM_DEFENDER, 0, 0, 80, 0, 0, 0
    MoveSE 1, 0, 1, 50, 0, 65, 18, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 83, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W312_HARP, 2, EFFVM_ATTACKER, 5, -20, 40, 0, 0, 0
    MoveSE 2, 0, 1, 20, 0, 65, 18, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 83, 0, 0, 0, 0, 0
    SetYield TRUE
    PaletteFade 4, 0, 12, 1, 8896
    WaitFrames 10
    McssSetValue EFFVM_ALL, 1
    WaitFrames 70
    CameraMovePreset 1, 9, 24, 0, 0
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x5ad6
    EmitterAdd 487, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 487, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W445_YUUWAKU, 2, EFFVM_ATTACKER, 5, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 40
    PaletteFade 4, 12, 0, 1, 8896
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x5ad6
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
