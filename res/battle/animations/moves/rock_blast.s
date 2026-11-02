// MOVE_ROCK_BLAST's animation: member 350 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockBlast

RockBlast:
    SetYield FALSE
    PlaySE SEQ_SE_W350_PASU, 1, EFFVM_ATTACKER, 0, -500, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_ATTACKER, 0, -700, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    ParticleLoad 527
    EmitterMove 527, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 527, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 527, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W155_02, 2, EFFVM_DEFENDER, 0, -500, 120, 0, 0, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 6
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
