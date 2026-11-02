// MOVE_TECHNO_BLAST's animation: member 546 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 5
    ScriptVariant TechnoBlast
    ScriptVariant TechnoBlast_Variant1
    ScriptVariant TechnoBlast_Variant2
    ScriptVariant TechnoBlast_Variant3
    ScriptVariant TechnoBlast_Variant4

TechnoBlast:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 2, 0, 10, 2, 0
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_SYS_68, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W157_04, 2, EFFVM_ATTACKER, 35, 2000, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_58, 1, EFFVM_ATTACKER, 55, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W025, 2, EFFVM_ATTACKER, 65, 0, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 20, 0, 0
    SetYield TRUE
    ParticleLoad 715
    EmitterAdd 715, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 715, 7, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 715, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 715, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 70
    EmitterMove 715, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x99a, 0x1000, 0
    CameraMovePreset 1, 11, 12, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_SYS_73, 1, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 2, EFFVM_DEFENDER, 0, 500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 715, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 715, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 715, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

TechnoBlast_Variant1:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 2, 0, 10, 2, 0
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_SYS_68, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W157_04, 2, EFFVM_ATTACKER, 35, 2000, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_58, 1, EFFVM_ATTACKER, 55, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W025, 2, EFFVM_ATTACKER, 65, 0, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 20, 0, 0
    SetYield TRUE
    ParticleLoad 716
    EmitterAdd 716, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 716, 7, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 716, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 716, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 70
    EmitterMove 716, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x99a, 0x1000, 0
    CameraMovePreset 1, 11, 12, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_SYS_73, 1, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 2, EFFVM_DEFENDER, 0, 500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 716, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 716, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 716, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

TechnoBlast_Variant2:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 2, 0, 10, 2, 0
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_SYS_68, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W157_04, 2, EFFVM_ATTACKER, 35, 2000, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_58, 1, EFFVM_ATTACKER, 55, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W025, 2, EFFVM_ATTACKER, 65, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 20, 0, 0
    SetYield TRUE
    ParticleLoad 717
    EmitterAdd 717, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 717, 7, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 717, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 717, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 70
    EmitterMove 717, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x99a, 0x1000, 0
    CameraMovePreset 1, 11, 12, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_SYS_73, 1, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 2, EFFVM_DEFENDER, 0, 500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 717, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 717, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 717, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

TechnoBlast_Variant3:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 2, 0, 10, 2, 0
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_SYS_68, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W157_04, 2, EFFVM_ATTACKER, 35, 2000, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_58, 1, EFFVM_ATTACKER, 55, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W025, 2, EFFVM_ATTACKER, 65, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 20, 0, 0
    SetYield TRUE
    ParticleLoad 718
    EmitterAdd 718, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 718, 7, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 718, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 718, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 70
    EmitterMove 718, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x99a, 0x1000, 0
    CameraMovePreset 1, 11, 12, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_SYS_73, 1, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 718, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 718, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 718, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

TechnoBlast_Variant4:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 2, 0, 10, 2, 0
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_SYS_68, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W157_04, 2, EFFVM_ATTACKER, 35, 2000, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_58, 1, EFFVM_ATTACKER, 55, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W025, 2, EFFVM_ATTACKER, 65, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 20, 0, 0
    SetYield TRUE
    ParticleLoad 719
    EmitterAdd 719, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 719, 7, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 719, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 719, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 70
    EmitterMove 719, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x99a, 0x1000, 0
    CameraMovePreset 1, 11, 12, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_SYS_73, 1, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 2, EFFVM_DEFENDER, 0, 500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 719, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 719, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 719, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
