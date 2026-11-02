// MOVE_REFLECT_TYPE's animation: member 513 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ReflectType

ReflectType:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 683
    McssPalFade EFFVM_DEFENDER, 0, 8, 0, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_DEFENDER, 0, -300, 127, 0, 0, 0
    MoveSE 1, 0, 0, -300, 300, 0, 30, 0, 0
    MoveSE 1, 0, 1, 127, 0, 25, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 40, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_DEFENDER, 29, -300, 40, 0, 0, 0
    MoveSE 2, 0, 0, -300, 300, 29, 30, 0, 0
    MoveSE 2, 0, 1, 40, 0, 56, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 70, 0, 0, 0, 0, 0
    SetYield TRUE
    EmitterAdd 683, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 8, 0, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 20, 0, 127, 0, 0, 0
    MoveSE 1, 0, 0, 0, -1000, 20, 120, 0, 0
    MoveSE 1, 0, 1, 127, 0, 20, 120, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 120, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, FALSE
    EmitterAdd 683, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 30
    McssPalFade EFFVM_ATTACKER, 0, 8, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 8, 0, 0, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
