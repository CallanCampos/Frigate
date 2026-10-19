#include "asm/field_script.inc"
#include "text/script/abundant_shrine_2.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4150
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0035
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Both people and Pokémon have to\nwork together to protect[f000]븀\u0000\nabundant land."
    ParentActorMsg MSGFILE_SCRIPT, AbundantShrine2_Text_BothPeoplePokemonHave, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0049

L_0035:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It is the Great Landorus that protects\nthis land.[f000]븁\u0000\nWith its help, we are assured of rich soil\nand a prosperous harvest."
    ParentActorMsg MSGFILE_SCRIPT, AbundantShrine2_Text_GreatLandorusProtectsLand, 0, 0
    LastKeyWait
    ActorMsgClose

L_0049:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
