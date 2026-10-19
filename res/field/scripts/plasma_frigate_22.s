#include "asm/field_script.inc"
#include "text/script/plasma_frigate_22.h"

// Script plugin 12, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01da
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009D
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The legendary Fire-type\nPokémon Reshiram![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_LegendaryFireTypePokemon, 0, 0
    ActorMsgClose
    CallPokemonPreview 643, 0, 0, 0
    PokeDexRegist 0, 643
    // "Reshiram has been registered\nin the Pokédex."
    SystemMsg PlasmaFrigate22_Text_ReshiramHasBeenRegistered, 0
    MsgWaitAdvance
    InfoMsgClose
    // "And this is the legendary Electric-type\nPokémon Zekrom.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_LegendaryElectricTypePokemon, 0, 0
    ActorMsgClose
    CallPokemonPreview 644, 0, 0, 0
    PokeDexRegist 0, 644
    // "Zekrom has been registered\nin the Pokédex."
    SystemMsg PlasmaFrigate22_Text_ZekromHasBeenRegistered, 0
    MsgWaitAdvance
    InfoMsgClose
    // "I yearned for them since I was a child.\nWhen I saw them two years ago,[f000]븀\u0000\nI was so moved that I cried!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_YearnedThemSinceChild, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01da
    VMJump L_00B1

L_009D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I yearned for them since I was a child.\nWhen I saw them two years ago,[f000]븀\u0000\nI was so moved that I cried!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_YearnedThemSinceChild, 0, 0
    LastKeyWait
    ActorMsgClose

L_00B1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E6
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I make bread every day.[f000]븁\u0000\nThat's what I like, so I'm satisfied,\nbut I wonder if everybody is fine[f000]븀\u0000\nwith only bread and water every day."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_MakeBreadEveryDay, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00FA

L_00E6:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Eat a lot![f000]븁\u0000\nYou can't love anyone\non an empty stomach!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_EatLotCantLove, 0, 0
    LastKeyWait
    ActorMsgClose

L_00FA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Colress told me that if I can bring out\nthe power of Pokémon by taking good care[f000]븀\u0000\nof them, I can stay here!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_ColressToldIfCan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "As long as I did what Team Plasma\nordered, I was able to eat bread...[f000]븁\u0000\nI didn't think for myself, and\nthat's why I became like this.[f000]븁\u0000\nAh!\nBut the bread here is yummy!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_LongDidWhatTeam, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's you the announcement\nwas warning about?[f000]븁\u0000\nFine.\nI'm very strong.[f000]븁\u0000\nI'll look the other way\nfor a child like you..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_ItsAnnouncementWarningAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I failed in my duty,\nand my Pokémon were taken away.[f000]븁\u0000\nI lost a lot of Pokémon\nI had stolen, too..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate22_Text_FailedDutyPokemonWere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_SW_PLAZMASHIP_09
    SEWait
    // "Warning! Warning!\nIntruders in the vessel![f000]븀\u0000\nEveryone, please respond."
    InfoMsg PlasmaFrigate22_Text_WarningWarningIntrudersVessel, 2
    LastKeyWait
    InfoMsgClose_0039
    WorkSetConst EVENT_WORK_0x4149, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
