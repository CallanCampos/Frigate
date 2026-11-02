// MOVE_MUD_SPORT's animation: member 300 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MudSport

MudSport:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W300_BESYO, 1, EFFVM_ATTACKER, 0, -700, 80, 0, 0, 0
    PlaySE SEQ_SE_W123_02, 2, EFFVM_ATTACKER, 0, 200, 30, 0, 0, 0
    PlaySE SEQ_SE_W300_BESYO, 1, EFFVM_ATTACKER, 30, -1000, 80, 0, 0, 0
    PlaySE SEQ_SE_W123_02, 2, EFFVM_ATTACKER, 30, -200, 30, 0, 0, 0
    PlaySE SEQ_SE_W123_02, 1, EFFVM_ATTACKER, 50, -200, 70, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0x4cd, 0x4cd, 4, 2, 3
    ParticleLoad 470
    EmitterAddOrtho 470, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 470, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W300_PITYA, 2, EFFVM_DEFENDER, 5, -300, 127, 0, 0, 0
    PlaySE SEQ_SE_W123_02, 1, EFFVM_DEFENDER, 0, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W300_PITYA, 2, EFFVM_DEFENDER, 15, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 25, 10, 0, 0
    MoveSE 2, 0, 1, 127, 0, 25, 10, 0, 0
    SetYield TRUE
    EmitterAddOrtho 470, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 1, 0x10cd, 0xe66, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_DEFENDER, 1, 0x1000, 0x1000, 2, 0, 1
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
