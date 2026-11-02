// Musical program 3's stage scripts: member 2 of ARCID_MUSICAL_PROGRAM + 3 (a/1/1/5)

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
    PlayBgm SEQ_BGM_MSL_04
    ButtonEnable
    Nop 3
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 435, 0, -0x28000, 0
    LightFollow 1, 1, 435, 0, -0x28000, 0
    LightFollow 2, 2, 435, 0, -0x28000, 0
    LightFollow 3, 3, 435, 0, -0x28000, 0
    Wait 435
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    SyncWait 4
    Nop 4
    Wait 242
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 1373, 0, -0x28000, 0
    LightFollow 1, 1, 1373, 0, -0x28000, 0
    LightFollow 2, 2, 1373, 0, -0x28000, 0
    LightFollow 3, 3, 1373, 0, -0x28000, 0
    Wait 0
    SyncWait 5
    Nop 5
    Wait 912
    SyncWait 6
    Nop 6
    Wait 461
    SyncWait 7
    Nop 7
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    Wait 1
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
    Wait 261
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
    Wait 1
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 259, 0, -0x28000, 0
    LightFollow 1, 1, 259, 0, -0x28000, 0
    LightFollow 2, 2, 259, 0, -0x28000, 0
    LightFollow 3, 3, 259, 0, -0x28000, 0
    Wait 159
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    LineUp 58, TRUE
    Wait 11
    LightAdd 0, 0
    LightAdd 1, 0
    LightAdd 2, 0
    LightAdd 3, 0
    LightFollow 0, 0, 120, 0, -0x28000, 0
    LightFollow 1, 1, 120, 0, -0x28000, 0
    LightFollow 2, 2, 120, 0, -0x28000, 0
    LightFollow 3, 3, 120, 0, -0x28000, 0
    SyncWait 13
    Wait 120
    SyncWait 14
    Nop 14
    Wait 60
    StopBgm
    ButtonDisable
    CurtainClose TRUE
    SyncWait 15
    LightDel 0
    LightDel 1
    LightDel 2
    LightDel 3
    Wait 60
    SyncWait 16
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
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 10
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, -0x2d000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 25, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -25, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, 0x2d000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 25, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -25, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 29
    PokeFront STA_SCRIPT_OWN_POKE, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, FALSE
    Wait 20
    PokeFront STA_SCRIPT_OWN_POKE, TRUE
    Wait 9
    SyncWait 4
    Nop 4
    Wait 10
    SetPokeMask TRUE, FALSE, FALSE, FALSE
    LightAdd 0, 0
    LightFollow 0, 0, 29, 0, -0x28000, 0
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 10, FALSE
    Wait 29
    PokeFront STA_SCRIPT_OWN_POKE, TRUE
    Wait 29
    LightDel 0
    Wait 174
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 10, -10, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 145, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 5, 0x10000, TRUE
    Wait 14
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 20, FALSE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 20, -20, FALSE
    SyncWait 5
    Nop 5
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, -0x2d000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 25, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -25, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, 0x2d000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 25, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -25, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x20000, FALSE
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 15
    Wait 29
    PokeMoveBy STA_SCRIPT_OWN_POKE, 145, -0x2d000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -25, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 25, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 58, 1, 0x28000, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, -360, FALSE
    Wait 44
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, TRUE
    Wait 15
    PokeMoveBy STA_SCRIPT_OWN_POKE, 145, 0x2d000, 0, 0, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 25, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, -25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 29, -25, 25, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeJump STA_SCRIPT_OWN_POKE, 58, 1, 0x28000, FALSE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 29, 0, 360, FALSE
    Wait 44
    Wait 14
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    Wait 15
    SyncWait 6
    Nop 6
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 116, -0x28000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 4, 0x10000, TRUE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, 0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 29
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 29
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x1c000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 26
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -20, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -20, 20, FALSE
    Wait 29
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    AppealScript 0, 19
    SyncWait 9
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    Wait 14
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, -15, FALSE
    Wait 29
    PokeRotate STA_SCRIPT_OWN_POKE, 0, -15, 15, FALSE
    Wait 15
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 15, FALSE
    Wait 29
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x1c000, TRUE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 15, -15, FALSE
    Wait 43
    SyncWait 13
    AppealScript 0, 13
    SyncWait 14
    Nop 14
    SyncWait 15
    SyncWait 16
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
    Wait 10
    Wait 58
    SetPokeMask FALSE, TRUE, FALSE, FALSE
    LightAdd 1, 0
    LightFollow 1, 1, 29, 0, -0x28000, 0
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 10, FALSE
    Wait 29
    PokeFront STA_SCRIPT_OWN_POKE, TRUE
    Wait 29
    LightDel 1
    Wait 116
    SyncWait 5
    Nop 5
    SyncWait 6
    Nop 6
    SyncWait 7
    Nop 7
    SyncWait 8
    Nop 8
    SyncWait 9
    AppealScript 1, 19
    SyncWait 10
    SyncWait 11
    SyncWait 12
    Nop 12
    SyncWait 13
    AppealScript 1, 13
    SyncWait 14
    Nop 14
    SyncWait 15
    SyncWait 16
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
    Wait 10
    Wait 116
    SetPokeMask FALSE, FALSE, TRUE, FALSE
    LightAdd 2, 0
    LightFollow 2, 2, 29, 0, -0x28000, 0
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 10, FALSE
    Wait 29
    PokeFront STA_SCRIPT_OWN_POKE, TRUE
    Wait 29
    LightDel 2
    Wait 58
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
    AppealScript 2, 19
    Wait 29
    SyncWait 11
    SyncWait 12
    Nop 12
    SyncWait 13
    AppealScript 2, 13
    SyncWait 14
    Nop 14
    SyncWait 15
    SyncWait 16
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
    SyncWait 4
    Nop 4
    Wait 10
    Wait 174
    SetPokeMask FALSE, FALSE, FALSE, TRUE
    LightAdd 3, 0
    LightFollow 3, 3, 29, 0, -0x28000, 0
    PokeFlip STA_SCRIPT_OWN_POKE, FALSE
    PokeMoveBy STA_SCRIPT_OWN_POKE, 29, -0x14000, 0, 0, FALSE
    PokeJump STA_SCRIPT_OWN_POKE, 29, 1, 0x18000, FALSE
    PokeRotate STA_SCRIPT_OWN_POKE, 0, 0, 10, FALSE
    Wait 29
    PokeFront STA_SCRIPT_OWN_POKE, TRUE
    Wait 28
    LightDel 3
    Wait 1
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
    AppealScript 3, 19
    SyncWait 12
    Nop 12
    SyncWait 13
    AppealScript 3, 13
    SyncWait 14
    Nop 14
    SyncWait 15
    SyncWait 16
    End
