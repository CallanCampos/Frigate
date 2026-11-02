// MOVE_SPIKES's animation: member 191 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Spikes

Spikes:
    McssSetVanish EFFVM_ALL, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 10, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 20, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 30, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W014_03, 2, EFFVM_DEFENDER, 30, 1000, 100, 0, 0, 0
    PlaySE SEQ_SE_W014_03, 2, EFFVM_DEFENDER, 40, 1000, 100, 0, 0, 0
    PlaySE SEQ_SE_W014_03, 2, EFFVM_DEFENDER, 50, 1000, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    ParticleLoad 361
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, Spikes_027E
    CameraMovePreset 1, 1, 32, 0, 12
    EmitterMove 361, 0, 2, EFFVM_EMIT_ATTACKER, 1, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 361, 0, 2, EFFVM_EMIT_ATTACKER, 1, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 361, 0, 2, EFFVM_EMIT_ATTACKER, 1, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    EmitterAdd 361, 1, 1, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 361, 2, 1, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Jump Spikes_0386

Spikes_027E:
    CameraMovePreset 1, 0, 32, 0, 12
    EmitterMove 361, 0, 2, EFFVM_EMIT_ATTACKER, 0, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 361, 0, 2, EFFVM_EMIT_ATTACKER, 0, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 361, 0, 2, EFFVM_EMIT_ATTACKER, 0, 0x1000, 0x14000, 0x2000, 0x1000, 0x1000, 0
    EmitterAdd 361, 1, 0, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 361, 2, 0, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000

Spikes_0386:
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ALL, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
