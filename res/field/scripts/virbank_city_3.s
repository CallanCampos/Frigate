#include "asm/field_script.inc"
#include "text/script/virbank_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon used to be wimpy,\nbut they've trained in the complex,[f000]븀\u0000\nand now they are very buff!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity3_Text_PokemonUsedWimpyBut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 19
    WorkSet 0x8001, 1
    WorkSet 0x8002, 279
    WorkSet 0x8003, 1
    WorkSet 0x8004, 2
    WorkSet 0x8005, 2
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Children teach me many things,\nso they are my teachers.[f000]븀\u0000\nPokémon are your teachers!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity3_Text_ChildrenTeachManyThings, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Clerks in Poké Marts\nsell different items!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity3_Text_ClerksPokeMartsSell, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you use a Repel,\nwild Pokémon won't come out as much![f000]븀\u0000\nDid you know that?"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity3_Text_WhenUseRepelWild, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A Pokémon that jumps out in a battle is\nthe Pokémon on the top left in the list.[f000]븁\u0000\nSo have a weak Pokémon\non the top left![f000]븁\u0000\nWhen a battle starts,\nswitch it to a strong Pokémon![f000]븀\u0000\nSee? I know a great thing, don't I?"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity3_Text_PokemonJumpsOutBattle, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
