#include "asm/field_script.inc"
#include "text/script/nacrene_city_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You see, practicing is sort of like\neating food every day.[f000]븁\u0000\nUnless it becomes something you do\nwithout thinking about it, you'll never[f000]븀\u0000\nbe great at guitar or with Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity6_Text_SeePracticingSortLike, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ditto... Everstone...\nNatures... What's the connection?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity6_Text_DittoEverstoneNaturesWhats, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    Cmd_01F6 0, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am a poet...\nI write poems.[f000]븁\u0000\nEvery day, I stretch my imagination and\ndevote myself to my creative activity.[f000]븁\u0000\nPeople dream when sleeping, but if a\nPokémon dreams...[f000]븁\u0000\nI cannot even imagine how it would be..."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity6_Text_AmPoetWritePoems, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0093

L_007F:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am a poet...\nI write poems.[f000]븁\u0000\nEvery day, I stretch my imagination and\ndevote myself to my creative activity.[f000]븁\u0000\nPeople dream when sleeping, but\nif a Pokémon dreams...[f000]븁\u0000\nJust thinking about it fuels\nmy imagination.[f000]븁\u0000\nPokémon probably enjoy sleeping, too!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity6_Text_AmPoetWritePoems_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_0093:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
