// Musical program 1's stage scripts: member 2 of ARCID_MUSICAL_PROGRAM + 1 (a/1/1/3)

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
    PlayBgm SEQ_BGM_MSL_02
    ButtonEnable
    Nop 3
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 1782, 0, -0x28000, 0
    LightFollow 1, 1, 1782, 0, -0x28000, 0
    LightFollow 2, 2, 1782, 0, -0x28000, 0
    LightFollow 3, 3, 1782, 0, -0x28000, 0
    Wait 459
    SyncWait 4
    Nop 4
    Wait 189
    SyncWait 5
    Nop 5
    Wait 486
    SyncWait 6
    Nop 6
    Wait 459
    SyncWait 7
    Nop 7
    Wait 189
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    SyncWait 8
    Nop 8
    LightAdd 0, 0
    LightFollow 0, 0, 216, 0, -0x28000, 0
    PokeMoveBy 0, 0, 0, 0, 0x64000, FALSE
    Wait 216
    PokeMoveBy 0, 0, 0, 0, -0x64000, FALSE
    LightDel 0
    SyncWait 9
    LightAdd 1, 0
    LightFollow 1, 1, 216, 0, -0x28000, 0
    PokeMoveBy 1, 0, 0, 0, 0x46000, FALSE
    Wait 216
    PokeMoveBy 1, 0, 0, 0, -0x46000, FALSE
    LightDel 1
    SyncWait 10
    LightAdd 2, 0
    LightFollow 2, 2, 216, 0, -0x28000, 0
    PokeMoveBy 2, 0, 0, 0, 0x28000, FALSE
    PokeMoveBy 3, 0, 0, 0, -0x1e000, FALSE
    Wait 216
    PokeMoveBy 2, 0, 0, 0, -0x28000, FALSE
    PokeMoveBy 3, 0, 0, 0, 0x1e000, FALSE
    LightDel 2
    SyncWait 11
    LightAdd 3, 0
    LightFollow 3, 3, 216, 0, -0x28000, 0
    PokeMoveBy 3, 0, 0, 0, 0xa000, FALSE
    Wait 216
    PokeMoveBy 3, 0, 0, 0, -0xa000, FALSE
    LightDel 3
    SyncWait 12
    AudienceUnfocus
    Nop 12
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 972, 0, -0x28000, 0
    LightFollow 1, 1, 972, 0, -0x28000, 0
    LightFollow 2, 2, 972, 0, -0x28000, 0
    LightFollow 3, 3, 972, 0, -0x28000, 0
    Wait 972
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    SyncWait 13
    Nop 13
    LineUp 27, TRUE
    Wait 15
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
    Wait 60
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
    Wait 5
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 22, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 13
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 13
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 27
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x10000, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0x14000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 27
    SyncWait 4
    Nop 4
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, -0x1e000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 2, 0x14000, TRUE
    Wait 27
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x18000, TRUE
    Wait 27
    SyncWait 5
    Nop 5
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 13, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, 0x1e000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 108, 0x28000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 54, 2, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 27
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 27, 0, -30, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x20000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 27, -30, 50, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x20000, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x1c000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 27
    SyncWait 6
    Nop 6
    SetPokeMask TRUE, TRUE, FALSE, FALSE
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, 0x1e000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 2, 0xc000, TRUE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x10000, TRUE
    Wait 27
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x1c000, TRUE
    Wait 27
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 81, -0x23000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 3, 0xc000, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x14000, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 27
    SyncWait 7
    Nop 7
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 25
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 23
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 25
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 23
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 23
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 16
    SyncWait 8
    Nop 8
    AppealScript 0, 17
    SyncWait 9
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    Wait 27
    SetPokeMask TRUE, FALSE, FALSE, FALSE
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront STA_SCRIPT_OWN_POKE, FALSE
    Wait 135
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    Wait 27
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0x1e000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0x1e000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 27
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0x1e000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0x1e000, 0, 0, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 27
    Wait 81
    SetPokeMask TRUE, FALSE, FALSE, FALSE
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront STA_SCRIPT_OWN_POKE, TRUE
    Wait 27
    Wait 27
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x14000, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 27
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x14000, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 27
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, 0x1e000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 54, 0, 375, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 27
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 0, 12
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
    SyncWait 4
    Nop 4
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    SyncWait 9
    AppealScript 1, 17
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    Wait 54
    SetPokeMask FALSE, TRUE, FALSE, FALSE
    PokeFlip 1, FALSE
    PokeJump 1, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront 1, FALSE
    Wait 108
    Wait 243
    Wait 54
    SetPokeMask FALSE, TRUE, FALSE, FALSE
    PokeFlip 1, FALSE
    PokeJump 1, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront 1, TRUE
    Wait 54
    Wait 27
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 1, 12
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
    SyncWait 4
    Nop 4
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SetPokeMask FALSE, FALSE, TRUE, TRUE
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 54, -0x1e000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 2, 0xc000, TRUE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x10000, TRUE
    Wait 27
    PokeJump STA_SCRIPT_OWN_POKE, 54, 1, 0x18000, TRUE
    Wait 27
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 27
    PokeMoveBy STA_SCRIPT_OWN_POKE, 81, 0x23000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 3, 0xc000, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 27, 0xf000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 27, 1, 0x10000, TRUE
    Wait 27
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 27
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    SyncWait 9
    SyncWait 10
    AppealScript 2, 17
    SyncWait 11
    SyncWait 12
    Nop 12
    Wait 81
    SetPokeMask FALSE, FALSE, TRUE, FALSE
    PokeFlip 2, FALSE
    PokeJump 2, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront 2, FALSE
    Wait 81
    Wait 243
    Wait 27
    SetPokeMask FALSE, FALSE, TRUE, FALSE
    PokeFlip 2, FALSE
    PokeJump 2, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront 2, TRUE
    Wait 81
    Wait 27
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 2, 12
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
    AppealScript 3, 17
    SyncWait 12
    Nop 12
    Wait 108
    SetPokeMask FALSE, FALSE, FALSE, TRUE
    PokeFlip 3, FALSE
    PokeJump 3, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront 3, FALSE
    Wait 54
    Wait 243
    Wait 0
    SetPokeMask FALSE, FALSE, FALSE, TRUE
    PokeFlip 3, FALSE
    PokeJump 3, 54, 1, 0x20000, FALSE
    Wait 27
    PokeFront 3, TRUE
    Wait 108
    Wait 27
    SyncWait 13
    Nop 13
    SyncWait 14
    AppealScript 3, 12
    SyncWait 15
    Nop 15
    SyncWait 16
    SyncWait 17
    End
    End
