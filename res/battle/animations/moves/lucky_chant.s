// MOVE_LUCKY_CHANT's animation: member 381 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LuckyChant

LuckyChant:
    CameraMovePreset 1, 21, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W116_01, 1, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W381_OMAJINAI, 2, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W319_01, 1, EFFVM_ATTACKER, 40, -50, 70, 0, 0, 0
    SetYield TRUE
    ParticleLoad 560
    EmitterAdd 560, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 560, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_ATTACKER, 12, 0, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    End
