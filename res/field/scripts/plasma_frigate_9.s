#include "asm/field_script.inc"
#include "text/script/plasma_frigate_9.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01da
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0085
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The legendary Pokémon that\ncreated the Unova region, Reshiram.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate9_Text_LegendaryPokemonCreatedUnova, 0, 0
    ActorMsgClose
    CallPokemonPreview 643, 0, 0, 0
    PokeDexRegist 0, 643
    // "Reshiram has been registered\nin the Pokédex."
    SystemMsg PlasmaFrigate9_Text_ReshiramHasBeenRegistered, 0
    MsgWaitAdvance
    InfoMsgClose
    // "And the other legendary\nPokémon, Zekrom.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate9_Text_OtherLegendaryPokemonZekrom, 0, 0
    ActorMsgClose
    CallPokemonPreview 644, 0, 0, 0
    PokeDexRegist 0, 644
    // "Zekrom has been registered\nin the Pokédex."
    SystemMsg PlasmaFrigate9_Text_ZekromHasBeenRegistered, 0
    MsgWaitAdvance
    InfoMsgClose
    // "If you have these Pokémon,\nit's so easy to control[f000]븀\u0000\nthe Unova region, right?"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate9_Text_IfHaveThesePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01da
    VMJump L_0099

L_0085:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you have these Pokémon,\nit's so easy to control[f000]븀\u0000\nthe Unova region, right?"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate9_Text_IfHaveThesePokemon, 0, 0
    LastKeyWait
    ActorMsgClose

L_0099:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
