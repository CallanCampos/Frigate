// MOVE_SWEET_SCENT's animation: member 230 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SweetScent

SweetScent:
    CameraMovePreset 1, 14, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    PlaySE SEQ_SE_FLD_18, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    ParticleLoad 400
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, SweetScent_0136
    EmitterAddOrtho 400, 2, 0, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 400, 0, 0, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 400, 4, 0, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, SweetScent_01C0

SweetScent_0136:
    EmitterAddOrtho 400, 3, 0, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 400, 1, 0, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 400, 4, 0, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000

SweetScent_01C0:
    PaletteFade 4, 0, 6, 0, 22943
    WaitFrames 80
    PaletteFade 4, 6, 0, 1, 22943
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
