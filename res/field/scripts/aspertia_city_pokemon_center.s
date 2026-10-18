#include "asm/field_script.inc"
#include "text/script/aspertia_city_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_6:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x03dd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0049
    ActorSetGPos 11, 1, 0, 12, 1

L_0049:
    VMHalt

Script_3:
    ActorsPauseAll
    // "Bianca: OK! I'll show you around\nthe Pokémon Center![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_BiancaOkIllShow, 8, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 255, 7, 12, 1, 8, 0
    ActorCmdExec 8, Movement_024C
    ActorCmdWait
    ActorCmdExec 8, Movement_0300
    ActorCmdWait
    // "The Pokémon Center heals\nPokémon for free![f000]븁\u0000\nYou should bring your Pokémon here\nanytime they are weak.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_PokemonCenterHealsPokemon, 8, 0, 0
    MsgWinCloseAll
    // "I'll heal your Pokémon.\nHand me your Poké Ball for a sec![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_IllHealPokemonHand, 6, 0, 0
    MsgWinCloseAll
    PlayerSetSpecialSequence 64
    ActorCmdExec 255, Movement_0360
    ActorCmdWait
    PlayerSetSpecialSequence 8
    ActorCmdExec 255, Movement_0368
    ActorCmdWait
    VMCall L_0378
    PokePartyRecoverAll
    RecordAdd 11, 1
    PlayerSetSpecialSequence 64
    ActorCmdExec 255, Movement_0370
    ActorCmdWait
    PlayerSetSpecialSequence 8
    ActorCmdExec 8, Movement_0310
    VMSleep 8
    ActorCmdExec 255, Movement_0310
    ActorCmdWait
    // "Next, I'll explain the PC![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_NextIllExplainPc, 8, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 255, 4, 12, 1, 8, 0
    ActorCmdExec 8, Movement_025C
    ActorCmdWait
    ActorCmdExec 255, Movement_0300
    ActorCmdExec 8, Movement_0300
    ActorCmdWait
    // "This square thing is a PC!\nAny Trainer is free to use it![f000]븁\u0000\nYou can deposit Pokémon in it.[f000]븁\u0000\nAlso, you can withdraw\nPokémon from it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_SquareThingPcAny, 8, 0, 0
    MsgWinCloseAll
    ActorCmdExec 8, Movement_0308
    ActorCmdExec 255, Movement_0308
    ActorCmdWait
    // "The next thing is over here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_NextThingOverHere, 8, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 8, 4, 13, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 8, Movement_0300
    ActorCmdExec 255, Movement_02E8
    ActorCmdWait
    ActorCmdExec 8, Movement_0330
    ActorCmdExec 255, Movement_0320
    ActorCmdWait
    ActorWalkRoute 8, 9, 17, 1, 8, 0
    VMSleep 4
    ActorWalkRoute 255, 9, 16, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_0318
    ActorCmdExec 8, Movement_0318
    ActorCmdWait
    // "This is the Poké Mart![f000]븁\u0000\nHere you can buy and\nsell many different items![f000]븁\u0000\nThe Poké Balls you use\nto catch Pokémon can also[f000]븀\u0000\nbe bought at the Poké Mart![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_PokeMartHereCan, 8, 0, 0
    MsgWinCloseAll
    ActorCmdExec 8, Movement_0300
    ActorCmdExec 255, Movement_0308
    ActorCmdWait
    WordSetPlayerName 0
    // "Here, [f000]Ā\u0001\u0000,\nI'll give you some Poké Balls![f000]븁\u0000"
    // "Here, [f000]Ā\u0001\u0000,\nI'll give you some Poké Balls![f000]븁\u0000"
    ActorMsgGendered 1024, AspertiaCityPokemonCenter_Text_HereIllGiveSome, AspertiaCityPokemonCenter_Text_HereIllGiveSome_2, 8, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 4
    WorkSet 0x8001, 10
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Bianca: Next up![f000]븁\u0000\nI'll show you how\nto use those Poké Balls![f000]븀\u0000\nFollow me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_BiancaNextUpIll, 8, 0, 0
    MsgWinCloseAll
    ActorCmdExec 8, Movement_0278
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 8
    SEWait
    FlagReset EVENT_FLAG_0x097e
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40a1, 6
    FlagSet EVENT_FLAG_0x02dc
    FlagReset EVENT_FLAG_0x02e8
    FlagReset EVENT_FLAG_0x02e7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_024C:
    Move 12, 4
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_025C:
    Move 14, 3
    Move 35, 1
    MoveEnd
    VMStackDiv
    VMReturn
    VMStackSub
    VMReturn
    Move 15, 1
    MoveEnd

Movement_0278:
    Move 14, 2
    Move 13, 3
    MoveEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 255
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 21
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 0, 1
    MoveEnd

Movement_02E8:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_0300:
    Move 32, 1
    MoveEnd

Movement_0308:
    Move 33, 1
    MoveEnd

Movement_0310:
    Move 34, 1
    MoveEnd

Movement_0318:
    Move 35, 1
    MoveEnd

Movement_0320:
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0330:
    Move 15, 1
    MoveEnd
    VMStackMul
    VMNop2
    PokePartyGetSpecies 0, 75
    VMNop2
    PokePartyGetSpecies 0, 100
    DebugPrint 4
    VMNop
    ActorCmdWait
    VMReturn
    Move 100, 1
    Move 62, 1
    MoveEnd

Movement_0360:
    Move 102, 1
    MoveEnd

Movement_0368:
    Move 0, 1
    MoveEnd

Movement_0370:
    Move 104, 1
    MoveEnd

L_0378:
    WorkSetConst 0x8020, 0
    ActorCmdExec 6, Movement_03A4
    ActorCmdWait
    PokePartyGetCount 0x8020, 1
    PokecenPlayHealingSequence 0x8020
    ActorCmdExec 6, Movement_03AC
    ActorCmdWait
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x20
    .byte 0x80
    .balign 4, 0

Movement_03A4:
    Move 0, 1
    MoveEnd

Movement_03AC:
    Move 1, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    PokePartyGetMemberByType 0x8021, 2
    PokePartyGetParam 0x8022, 0x8021, 112
    WordSetPlayerName 0
    WordSetPartyPokeSpecies 1, 0x8021
    WordSetLoadNature 2, 0x8022
    // "Oh?\nYour [f000]ā\u0001\u0001...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_Oh, 0, 0
    // "Its Nature is [f000]Ĉ\u0001\u0002![f000]븁\u0000\nWith a Pokémon like that by your side,\nI'm sure you'll have a fun journey!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_ItsNaturePokemonLike, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "All right! Here's some advice from a\nguy who spends all of his time[f000]븀\u0000\nin Pokémon Centers![f000]븁\u0000\nWhen your Pokémon's HP goes down,\nmake sure to restore it!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCityPokemonCenter_Text_AllRightHeresSome, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
