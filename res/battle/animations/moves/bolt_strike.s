// MOVE_BOLT_STRIKE's animation: member 550 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BoltStrike

BoltStrike:
    LoadBg 213
    SetVanish 2, 0
    CameraMovePreset 1, 9, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 2, EFFVM_ATTACKER, 45, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 1, EFFVM_ATTACKER, 50, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_132, 2, EFFVM_ATTACKER, 80, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W017_01, 1, EFFVM_ATTACKER, 130, -800, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 131, 5, 0, 0
    SetYield TRUE
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    SetVanish 0, 1
    SetVanish 1, 1
    McssPalFade EFFVM_ATTACKER, 0, 8, 4, 0x3ff
    ParticleLoad 723
    EmitterAdd 723, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 723, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 723, 9, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 723, 7, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    EmitterAdd 723, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 2, 0x666, -0x666, 6, 0, 1
    McssCircle EFFVM_ATTACKER, 2, 2, 0, 0x2000, 0xa000, 0, 0x1000, 0
    WaitFrames 10
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_SYS_73, 2, EFFVM_DEFENDER, 0, -500, 75, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 2, EFFVM_DEFENDER, 45, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_FLD_69, 3, EFFVM_DEFENDER, 0, 1000, 60, 0, 0, 0
    MoveSE 3, 0, 0, 0, 5000, 0, 100, 0, 0
    MoveSE 3, 0, 1, 60, 0, 80, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    SetYield TRUE
    PaletteFade 3, 16, 0, 2, 0
    BgMove 2, 6, 0, 2, 0, 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_ATTACKER, 8, 0, 4, 0x3ff
    McssPalFade EFFVM_DEFENDER, 0, 8, 2, 0x3ff
    EmitterAdd 723, 8, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 723, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 723, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 723, 6, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 723, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssCircle EFFVM_DEFENDER, 2, 2, 0, 0x2000, 0xa000, 0, 0x1000, 0
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 0, 18
    WaitFrames 1
    SetVanish 2, 1
    WaitFrames 1
    SetVanish 2, 0
    WaitFrames 1
    SetVanish 2, 1
    WaitFrames 1
    SetVanish 2, 0
    WaitFrames 1
    SetVanish 2, 1
    WaitFrames 1
    SetVanish 2, 0
    WaitFrames 1
    SetVanish 2, 1
    WaitFrames 1
    SetVanish 2, 0
    WaitFrames 1
    PaletteFade 4, 16, 0, 2, 0
    WaitFrames 20
    EmitterAdd 723, 5, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1800, 0x1000, 0x1000
    WaitFrames 20
    PaletteFade 4, 0, 16, 2, 0
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    SetVanish 1, 0
    SetVanish 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 2, 0
    McssPalFade EFFVM_DEFENDER, 8, 0, 4, 0x3ff
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
