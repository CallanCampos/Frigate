// Effect BTLV_EFFECT_FIRST + 0 (0x231): member 0 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect000

Effect000:
    SetYield FALSE
    CameraMovePreset 0, 1, 0, 0, 0
    CameraRotate 0, 0, -8192, 0, 0, 0
    IfPokeExists 1, FALSE, Effect000_0096
    McssPalFade 1, 12, 12, 0, 0

Effect000_0096:
    IfPokeExists 3, FALSE, Effect000_00BA
    McssPalFade 3, 12, 12, 0, 0

Effect000_00BA:
    IfPokeExists 5, FALSE, Effect000_00DE
    McssPalFade 5, 12, 12, 0, 0

Effect000_00DE:
    IfPokeExists 7, FALSE, Effect000_0102
    McssPalFade 7, 12, 12, 0, 0

Effect000_0102:
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    CameraRotate 1, 0, 8192, 20, 0, 10
    Wait EFFVM_WAIT_CAMERA
    IfPokeExists 1, FALSE, Effect000_0170
    McssPalFade 1, 12, 0, 1, 0
    PlayCry 1, 0x80000000, 0, 0, 0, 0, 0

Effect000_0170:
    IfPokeExists 3, FALSE, Effect000_01B2
    McssPalFade 3, 12, 0, 1, 0
    PlayCry 3, 0x80000000, 0, 0, 0, 0, 0

Effect000_01B2:
    IfPokeExists 5, FALSE, Effect000_01F4
    McssPalFade 5, 12, 0, 1, 0
    PlayCry 5, 0x80000000, 0, 0, 0, 0, 0

Effect000_01F4:
    IfPokeExists 7, FALSE, Effect000_0236
    McssPalFade 7, 12, 0, 1, 0
    PlayCry 7, 0x80000000, 0, 0, 0, 0, 0

Effect000_0236:
    CameraMovePreset 1, 8, 20, 0, 20
    Wait EFFVM_WAIT_CAMERA
    Wait EFFVM_WAIT_MCSS
    Wait EFFVM_WAIT_CRY
    IfPokeExists 1, FALSE, Effect000_02D2
    IfVarConst EFFVM_VAR_FAMOUS + 1, EFFVM_CMP_EQ, FALSE, Effect000_0292
    Call BTLV_EFFECT_FIRST + 114, 1, 1
    Jump Effect000_02D2

Effect000_0292:
    IfVarConst EFFVM_VAR_RARE + 1, EFFVM_CMP_EQ, FALSE, Effect000_02B2
    Call BTLV_EFFECT_FIRST + 47, 1, 1

Effect000_02B2:
    IfVarConst EFFVM_VAR_N_POKEMON + 1, EFFVM_CMP_EQ, FALSE, Effect000_02D2
    Call BTLV_EFFECT_FIRST + 98, 1, 1

Effect000_02D2:
    IfPokeExists 3, FALSE, Effect000_0346
    IfVarConst EFFVM_VAR_FAMOUS + 3, EFFVM_CMP_EQ, FALSE, Effect000_0306
    Call BTLV_EFFECT_FIRST + 114, 3, 3
    Jump Effect000_0346

Effect000_0306:
    IfVarConst EFFVM_VAR_RARE + 3, EFFVM_CMP_EQ, FALSE, Effect000_0326
    Call BTLV_EFFECT_FIRST + 47, 3, 3

Effect000_0326:
    IfVarConst EFFVM_VAR_N_POKEMON + 3, EFFVM_CMP_EQ, FALSE, Effect000_0346
    Call BTLV_EFFECT_FIRST + 98, 3, 3

Effect000_0346:
    IfPokeExists 5, FALSE, Effect000_03BA
    IfVarConst EFFVM_VAR_FAMOUS + 5, EFFVM_CMP_EQ, FALSE, Effect000_037A
    Call BTLV_EFFECT_FIRST + 114, 5, 5
    Jump Effect000_03BA

Effect000_037A:
    IfVarConst EFFVM_VAR_RARE + 5, EFFVM_CMP_EQ, FALSE, Effect000_039A
    Call BTLV_EFFECT_FIRST + 47, 5, 5

Effect000_039A:
    IfVarConst EFFVM_VAR_N_POKEMON + 5, EFFVM_CMP_EQ, FALSE, Effect000_03BA
    Call BTLV_EFFECT_FIRST + 98, 5, 5

Effect000_03BA:
    IfPokeExists 7, FALSE, Effect000_042E
    IfVarConst EFFVM_VAR_FAMOUS + 7, EFFVM_CMP_EQ, FALSE, Effect000_03EE
    Call BTLV_EFFECT_FIRST + 114, 7, 7
    Jump Effect000_042E

Effect000_03EE:
    IfVarConst EFFVM_VAR_RARE + 7, EFFVM_CMP_EQ, FALSE, Effect000_040E
    Call BTLV_EFFECT_FIRST + 47, 7, 7

Effect000_040E:
    IfVarConst EFFVM_VAR_N_POKEMON + 7, EFFVM_CMP_EQ, FALSE, Effect000_042E
    Call BTLV_EFFECT_FIRST + 98, 7, 7

Effect000_042E:
    Wait EFFVM_WAIT_PARTICLES
    End
    .balign 4
