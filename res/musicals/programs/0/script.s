// Musical program 0's stage scripts: member 2 of ARCID_MUSICAL_PROGRAM + 0 (a/1/1/2)

#include "asm/sta_script.inc"

    ScriptHeader
    ScriptEntry Main  // 0: the program's script
    ScriptEntry Main  // 1: not read
    ScriptEntry Position0  // 2 to 5: the four positions' scripts, run beside the first where they differ from it
    ScriptEntry Position1
    ScriptEntry Position2
    ScriptEntry Position3
    ScriptEntry Main  // 6 to 13: not read
    ScriptEntry Main
    ScriptEntry Main
    ScriptEntry Main
    ScriptEntry Main
    ScriptEntry Main
    ScriptEntry Main
    ScriptEntry Main

Main:
    Nop 0
    Wait 60
    Nop2 1
    MessageColor 15, 2, 0
    SyncWait 0
    CurtainOpen TRUE
    Wait 60
    SyncWait 1
    Nop 1
    PlayBgm SEQ_BGM_MSL_BEFORE
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeScript STA_SCRIPT_OWN_POKE, 1
    Wait 700
    StopBgm
    SyncWait 2
    Nop 2
    Wait 240
    AudienceUnfocus
    Wait 60
    SyncWait 3
    PlayBgm SEQ_BGM_MSL_01
    ButtonEnable
    Nop 3
    AudienceFocus 0
    LightAdd 0, 0
    LightFollow 0, 0, 2175, 0, -0x28000, 0
    Wait 29
    AudienceFocus 1
    LightAdd 1, 0
    LightFollow 1, 1, 2175, 0, -0x28000, 0
    Wait 29
    AudienceFocus 2
    LightAdd 2, 0
    LightFollow 2, 2, 2175, 0, -0x28000, 0
    Wait 29
    AudienceFocus 3
    LightAdd 3, 0
    LightFollow 3, 3, 2175, 0, -0x28000, 0
    Wait 29
    AudienceUnfocus
    Wait 348
    SyncWait 4
    Nop 4
    Wait 464
    SyncWait 5
    Nop 5
    Wait 406
    SyncWait 6
    Nop 6
    Wait 464
    SyncWait 7
    Nop 7
    Wait 377
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    SyncWait 8
    Nop 8
    LightAdd 0, 0
    LightFollow 0, 0, 232, 0, -0x28000, 0
    PokeMoveBy 0, 0, 0, 0, 0x64000, FALSE
    Wait 232
    PokeMoveBy 0, 0, 0, 0, -0x64000, FALSE
    LightDel 0
    SyncWait 9
    LightAdd 1, 0
    LightFollow 1, 1, 232, 0, -0x28000, 0
    PokeMoveBy 1, 0, 0, 0, 0x46000, FALSE
    Wait 232
    PokeMoveBy 1, 0, 0, 0, -0x46000, FALSE
    LightDel 1
    SyncWait 10
    LightAdd 2, 0
    LightFollow 2, 2, 232, 0, -0x28000, 0
    PokeMoveBy 2, 0, 0, 0, 0x28000, FALSE
    PokeMoveBy 3, 0, 0, 0, -0x1e000, FALSE
    Wait 232
    PokeMoveBy 2, 0, 0, 0, -0x28000, FALSE
    PokeMoveBy 3, 0, 0, 0, 0x1e000, FALSE
    LightDel 2
    SyncWait 11
    LightAdd 3, 0
    LightFollow 3, 3, 232, 0, -0x28000, 0
    PokeMoveBy 3, 0, 0, 0, 0xa000, FALSE
    Wait 232
    PokeMoveBy 3, 0, 0, 0, -0xa000, FALSE
    LightDel 3
    SyncWait 12
    AudienceUnfocus
    Nop 12
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 754, 0, -0x28000, 0
    LightFollow 1, 1, 754, 0, -0x28000, 0
    LightFollow 2, 2, 754, 0, -0x28000, 0
    LightFollow 3, 3, 754, 0, -0x28000, 0
    Wait 754
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    SyncWait 13
    Nop 13
    LineUp 29, TRUE
    Wait 25
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 120, 0, -0x28000, 0
    LightFollow 1, 1, 120, 0, -0x28000, 0
    LightFollow 2, 2, 120, 0, -0x28000, 0
    LightFollow 3, 3, 120, 0, -0x28000, 0
    SyncWait 14
    Wait 120
    SyncWait 15
    Nop 15
    Wait 100
    StopBgm
    ButtonDisable
    CurtainClose TRUE
    SyncWait 16
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    Wait 60
    SyncWait 17
    End

Position0:
    Nop 0
    SetPokeMask TRUE, FALSE, FALSE, FALSE
    PokeMove STA_SCRIPT_OWN_POKE, 0, 0x231000, 0x9b000, 0x50000, FALSE
    PokeShow STA_SCRIPT_OWN_POKE, TRUE
    SyncWait 0
    SyncWait 1
    Nop 1
    SyncWait 2
    Nop 2
    Wait 0
    AudienceFocus 0
    LightAdd 0, 0
    LightFollow 0, 0, 60, 0, -0x28000, 0
    PokeScript 0, 0
    Wait 30
    LightDel 0
    SyncWait 3
    Nop 3
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    Wait 7
    SetPokeMask FALSE, FALSE, FALSE, FALSE
    PokeFlip 0, FALSE
    Wait 0
    PokeJump 0, 58, 1, 0x20000, TRUE
    Wait 138
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 29
    SyncWait 4
    Nop 4
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, 0x3c000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 4, 0x18000, TRUE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 15
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x20000, TRUE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 28
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, -0x3c000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 4, 0x18000, TRUE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 15
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x20000, TRUE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 31
    SyncWait 5
    Nop 5
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 28, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 28, 1, 0x18000, TRUE
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 14
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    Wait 29
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 14
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 14
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 58, 0, 360, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 58, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 43
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 19
    SyncWait 6
    Nop 6
    SetPokeMask TRUE, TRUE, FALSE, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, -0x3c000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 4, 0x10000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 58, 1, 0x20000, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 15, 345, FALSE
    Wait 44
    Wait 15
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 15
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, 0x3c000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 4, 0x10000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 58, 1, 0x20000, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -15, -345, FALSE
    Wait 44
    Wait 28
    SyncWait 7
    Nop 7
    SetPokeMask FALSE, FALSE, FALSE, FALSE
    PokeJump 0, 29, 1, 0x10000, FALSE
    Wait 14
    PokeFlip 0, FALSE
    Wait 15
    PokeRotate 0, 0, 0, 15, FALSE
    Wait 14
    PokeRotate 0, 0, 15, -15, FALSE
    Wait 15
    PokeRotate 0, 0, 0, 15, FALSE
    Wait 14
    PokeRotate 0, 0, 15, -15, FALSE
    Wait 15
    PokeJump 0, 58, 1, 0x20000, TRUE
    Wait 14
    PokeFront 0, FALSE
    Wait 29
    PokeFront 0, TRUE
    Wait 29
    PokeRotate 0, 0, 0, 15, FALSE
    Wait 14
    PokeRotate 0, 0, 15, -15, FALSE
    Wait 15
    PokeRotate 0, 0, 0, 15, FALSE
    Wait 14
    PokeRotate 0, 0, 15, -15, FALSE
    Wait 15
    PokeJump 0, 58, 1, 0x20000, TRUE
    Wait 15
    PokeJump 0, 29, 1, 0x10000, FALSE
    Wait 14
    PokeFlip 0, FALSE
    Wait 15
    SyncWait 8
    Nop 8
    AppealScript 0, 16
    SyncWait 9
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeMoveBy STA_SCRIPT_OWN_POKE, 58, 0x1e000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 360, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, FALSE
    Wait 14
    PokeFront STA_SCRIPT_OWN_POKE, FALSE
    Wait 15
    PokeMoveBy STA_SCRIPT_OWN_POKE, 58, 0x1e000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 375, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, FALSE
    Wait 14
    PokeFront STA_SCRIPT_OWN_POKE, TRUE
    Wait 15
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeMoveBy STA_SCRIPT_OWN_POKE, 174, -0x3c000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 3, 0x14000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 58, 0, -360, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 58, 1, 0x20000, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 29
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 0, 14
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End

Position1:
    Nop 0
    SetPokeMask FALSE, TRUE, FALSE, FALSE
    PokeMove STA_SCRIPT_OWN_POKE, 0, 0x277000, 0x9b000, 0x6e000, FALSE
    PokeShow STA_SCRIPT_OWN_POKE, TRUE
    SyncWait 0
    SyncWait 1
    Nop 1
    SyncWait 2
    Nop 2
    Wait 60
    AudienceFocus 1
    LightAdd 1, 0
    LightFollow 1, 1, 60, 0, -0x28000, 0
    PokeScript 1, 0
    Wait 30
    LightDel 1
    SyncWait 3
    Nop 3
    PokeFlip 1, FALSE
    Wait 29
    PokeJump 1, 58, 1, 0x20000, TRUE
    Wait 116
    SyncWait 4
    Nop 4
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SyncWait 7
    Nop 7
    SetPokeMask FALSE, FALSE, FALSE, FALSE
    PokeJump 1, 29, 1, 0x10000, FALSE
    Wait 14
    PokeFlip 1, TRUE
    Wait 15
    PokeMoveBy 1, 29, 0x14000, 0, 0, FALSE
    PokeJump 1, 29, 1, 0x10000, TRUE
    Wait 14
    PokeRotate 1, 0, 0, -15, FALSE
    Wait 15
    PokeJump 1, 29, 1, 0x10000, TRUE
    PokeJump 1, 58, 1, 0x20000, TRUE
    Wait 14
    PokeRotate 1, 0, -15, 15, FALSE
    Wait 15
    PokeMoveBy 1, 29, -0x14000, 0, 0, FALSE
    PokeJump 1, 29, 1, 0x10000, TRUE
    Wait 29
    PokeFront 1, FALSE
    Wait 29
    PokeJump 1, 29, 1, 0x18000, FALSE
    Wait 14
    PokeFront 1, TRUE
    Wait 15
    Wait 29
    PokeJump 1, 29, 1, 0x18000, FALSE
    Wait 14
    PokeFlip 1, FALSE
    Wait 15
    SyncWait 8
    Nop 8
    SyncWait 9
    AppealScript 1, 16
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 1, 14
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End

Position2:
    Nop 0
    SetPokeMask FALSE, FALSE, TRUE, FALSE
    PokeMove STA_SCRIPT_OWN_POKE, 0, 0x2bd000, 0x9b000, 0x8c000, FALSE
    PokeShow STA_SCRIPT_OWN_POKE, TRUE
    SyncWait 0
    SyncWait 1
    Nop 1
    SyncWait 2
    Nop 2
    Wait 120
    AudienceFocus 2
    LightAdd 2, 0
    LightFollow 2, 2, 60, 0, -0x28000, 0
    PokeScript 2, 0
    Wait 30
    LightDel 2
    SyncWait 3
    Nop 3
    PokeFlip 2, FALSE
    Wait 58
    PokeJump 2, 58, 1, 0x20000, TRUE
    Wait 87
    SyncWait 4
    Nop 4
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SetPokeMask FALSE, FALSE, TRUE, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 15
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, 0x3c000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 4, 0x10000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 58, 1, 0x20000, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -15, -345, FALSE
    Wait 44
    Wait 15
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 15
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, -0x3c000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 4, 0x10000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 58, 1, 0x20000, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 15, 345, FALSE
    Wait 44
    Wait 28
    SyncWait 7
    Nop 7
    SetPokeMask FALSE, FALSE, FALSE, FALSE
    PokeJump 2, 29, 1, 0x10000, FALSE
    Wait 14
    PokeFlip 2, FALSE
    Wait 15
    PokeMoveBy 2, 29, -0x14000, 0, 0, FALSE
    PokeJump 2, 29, 1, 0x10000, TRUE
    Wait 14
    PokeRotate 2, 0, 0, 15, FALSE
    Wait 15
    PokeJump 2, 29, 1, 0x10000, TRUE
    PokeJump 2, 58, 1, 0x20000, TRUE
    Wait 14
    PokeRotate 2, 0, 15, -15, FALSE
    Wait 15
    PokeMoveBy 2, 29, 0x14000, 0, 0, FALSE
    PokeJump 2, 29, 1, 0x10000, TRUE
    Wait 29
    PokeFront 2, FALSE
    Wait 29
    PokeJump 2, 29, 1, 0x18000, FALSE
    Wait 14
    PokeFront 2, TRUE
    Wait 15
    Wait 29
    PokeJump 2, 29, 1, 0x18000, FALSE
    Wait 14
    PokeFlip 2, FALSE
    Wait 15
    SyncWait 8
    Nop 8
    SyncWait 9
    SyncWait 10
    AppealScript 2, 16
    SyncWait 11
    SyncWait 12
    Nop 12
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 2, 14
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End

Position3:
    Nop 0
    SetPokeMask FALSE, FALSE, FALSE, TRUE
    PokeMove STA_SCRIPT_OWN_POKE, 0, 0x303000, 0x9b000, 0xaa000, FALSE
    PokeShow STA_SCRIPT_OWN_POKE, TRUE
    SyncWait 0
    SyncWait 1
    Nop 1
    SyncWait 2
    Nop 2
    Wait 180
    AudienceFocus 3
    LightAdd 3, 0
    LightFollow 3, 3, 60, 0, -0x28000, 0
    PokeScript 3, 0
    Wait 30
    LightDel 3
    SyncWait 3
    Nop 3
    PokeFlip 3, FALSE
    Wait 87
    PokeJump 3, 58, 1, 0x20000, TRUE
    SyncWait 4
    Nop 4
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SyncWait 7
    Nop 7
    SetPokeMask FALSE, FALSE, FALSE, FALSE
    PokeJump 3, 29, 1, 0x10000, FALSE
    Wait 14
    PokeFlip 3, TRUE
    Wait 15
    PokeRotate 3, 0, 0, -15, FALSE
    Wait 14
    PokeRotate 3, 0, -15, 15, FALSE
    Wait 15
    PokeRotate 3, 0, 0, -15, FALSE
    Wait 14
    PokeRotate 3, 0, -15, 15, FALSE
    Wait 15
    PokeJump 3, 58, 1, 0x20000, TRUE
    Wait 14
    PokeFront 3, FALSE
    Wait 29
    PokeFront 3, TRUE
    Wait 29
    PokeRotate 3, 0, 0, -15, FALSE
    Wait 14
    PokeRotate 3, 0, -15, 15, FALSE
    Wait 15
    PokeRotate 3, 0, 0, -15, FALSE
    Wait 14
    PokeRotate 3, 0, -15, 15, FALSE
    Wait 15
    PokeJump 3, 58, 1, 0x20000, TRUE
    Wait 15
    PokeJump 3, 29, 1, 0x10000, FALSE
    Wait 14
    PokeFlip 3, FALSE
    Wait 15
    SyncWait 8
    Nop 8
    SyncWait 9
    SyncWait 10
    SyncWait 11
    AppealScript 3, 16
    SyncWait 12
    Nop 12
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 3, 14
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End
