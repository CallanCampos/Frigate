// MOVE_TOXIC_SPIKES's animation: member 390 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ToxicSpikes

ToxicSpikes:
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, -500, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 18, -500, 115, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 20, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 36, -500, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 45, 20, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ALL, TRUE
    ParticleLoad 569
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, ToxicSpikes_0268
    CameraMovePreset 1, 1, 32, 0, 12
    EmitterMove 569, 1, 2, EFFVM_EMIT_ATTACKER, 1, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 569, 1, 2, EFFVM_EMIT_ATTACKER, 1, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 569, 1, 2, EFFVM_EMIT_ATTACKER, 1, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    EmitterAdd 569, 2, 1, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 569, 3, 1, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Jump ToxicSpikes_0370

ToxicSpikes_0268:
    CameraMovePreset 1, 0, 32, 0, 12
    EmitterMove 569, 1, 2, EFFVM_EMIT_ATTACKER, 0, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 569, 1, 2, EFFVM_EMIT_ATTACKER, 0, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 569, 1, 2, EFFVM_EMIT_ATTACKER, 0, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    EmitterAdd 569, 2, 0, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 569, 3, 0, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

ToxicSpikes_0370:
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ALL, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
