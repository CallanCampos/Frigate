// MOVE_NONE's animation: member 0 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant None

None:
    ParticleLoad 165
    EmitterAdd 165, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 165, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
