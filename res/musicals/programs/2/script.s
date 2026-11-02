// Musical program 2's stage scripts: member 2 of ARCID_MUSICAL_PROGRAM + 2 (a/1/1/4)

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
    PlayBgm SEQ_BGM_MSL_03
    ButtonEnable
    Nop 3
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 1764, 0, -0x28000, 0
    LightFollow 1, 1, 1764, 0, -0x28000, 0
    LightFollow 2, 2, 1764, 0, -0x28000, 0
    LightFollow 3, 3, 1764, 0, -0x28000, 0
    Wait 612
    SyncWait 4
    Nop 4
    Wait 576
    SyncWait 5
    Nop 5
    Wait 288
    SyncWait 6
    Nop 6
    Wait 288
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    SyncWait 7
    Nop 7
    AudienceFocus 0
    Wait 0
    SyncWait 8
    Nop 8
    LightAdd 0, 0
    LightFollow 0, 0, 288, 0, -0x28000, 0
    PokeMoveBy 0, 0, 0, 0, 0x64000, FALSE
    Wait 288
    PokeMoveBy 0, 0, 0, 0, -0x64000, FALSE
    LightDel 0
    AudienceUnfocus
    SyncWait 9
    LightAdd 1, 0
    LightFollow 1, 1, 288, 0, -0x28000, 0
    PokeMoveBy 1, 0, 0, 0, 0x46000, FALSE
    Wait 288
    PokeMoveBy 1, 0, 0, 0, -0x46000, FALSE
    LightDel 1
    SyncWait 10
    LightAdd 2, 0
    LightFollow 2, 2, 288, 0, -0x28000, 0
    PokeMoveBy 2, 0, 0, 0, 0x28000, FALSE
    PokeMoveBy 3, 0, 0, 0, -0x1e000, FALSE
    Wait 288
    PokeMoveBy 2, 0, 0, 0, -0x28000, FALSE
    PokeMoveBy 3, 0, 0, 0, 0x1e000, FALSE
    LightDel 2
    SyncWait 11
    LightAdd 3, 0
    LightFollow 3, 3, 288, 0, -0x28000, 0
    PokeMoveBy 3, 0, 0, 0, 0xa000, FALSE
    Wait 288
    Wait 36
    PokeMoveBy 3, 0, 0, 0, -0xa000, FALSE
    LightDel 3
    SyncWait 12
    AudienceUnfocus
    Nop 12
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 432, 0, -0x28000, 0
    LightFollow 1, 1, 432, 0, -0x28000, 0
    LightFollow 2, 2, 432, 0, -0x28000, 0
    LightFollow 3, 3, 432, 0, -0x28000, 0
    Wait 432
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    SyncWait 13
    Nop 13
    LineUp 36, TRUE
    Wait 24
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
    Wait 180
    ButtonDisable
    StopBgm
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
    PokeStartAnime 0
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
    Wait 10
    PokeJump STA_SCRIPT_OWN_POKE, 26, 1, 0xc000, TRUE
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 36, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 36, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 36, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 36, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 36
    SyncWait 4
    Nop 4
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 72, 0x1e000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 72, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 36
    SetPokeMask TRUE, FALSE, TRUE, FALSE
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 36, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    SetPokeMask TRUE, FALSE, FALSE, FALSE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 36, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    SyncWait 5
    Nop 5
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 72, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 72, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 72, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 72, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 18
    SyncWait 6
    Nop 6
    PokeRotate STA_SCRIPT_OWN_POKE, 36, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 36, -0x23000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 36, -20, 20, FALSE
    Wait 36
    PokeJump STA_SCRIPT_OWN_POKE, 36, 1, 0x18000, FALSE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 18
    Wait 36
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, -0x23000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 3, 0x10000, TRUE
    Wait 16
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 25, -0xa000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 25, 1, 0x10000, TRUE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 13, 0xa000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 13, 1, 0xc000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    AppealScript 0, 18
    SyncWait 9
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    Wait 36
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 36, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 18
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 36, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 18
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0x10000, TRUE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0x10000, TRUE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 18
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 0, 11
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
    PokeStartAnime 1
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
    SyncWait 4
    Nop 4
    Wait 108
    SetPokeMask FALSE, TRUE, FALSE, TRUE
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0x5000, 0, 0, FALSE
    Wait 36
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 36, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    SetPokeMask FALSE, TRUE, FALSE, FALSE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 36, 1, 0x1c000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, TRUE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    SyncWait 9
    AppealScript 1, 18
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 1, 11
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End

Position2:
    SetPokeMask FALSE, FALSE, TRUE, FALSE
    PokeMove STA_SCRIPT_OWN_POKE, 0, 0x2bd000, 0x9b000, 0x8c000, FALSE
    PokeShow STA_SCRIPT_OWN_POKE, TRUE
    SyncWait 0
    PokeStartAnime 2
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
    SyncWait 4
    Nop 4
    Wait 396
    SetPokeMask FALSE, FALSE, TRUE, FALSE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 36, 1, 0x1c000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, TRUE
    Wait 18
    PokeMoveBy STA_SCRIPT_OWN_POKE, 18, 0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 18, 1, 0xc000, TRUE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    SyncWait 9
    SyncWait 10
    AppealScript 2, 18
    SyncWait 11
    SyncWait 12
    Nop 12
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 2, 11
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End

Position3:
    SetPokeMask FALSE, FALSE, FALSE, TRUE
    PokeMove STA_SCRIPT_OWN_POKE, 0, 0x303000, 0x9b000, 0xaa000, FALSE
    PokeShow STA_SCRIPT_OWN_POKE, TRUE
    SyncWait 0
    PokeStartAnime 3
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
    SyncWait 4
    Nop 4
    Wait 396
    SetPokeMask FALSE, FALSE, FALSE, TRUE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 18
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 36, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 18
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 18
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    SyncWait 9
    SyncWait 10
    SyncWait 11
    AppealScript 3, 18
    SyncWait 12
    Nop 12
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 3, 11
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End
