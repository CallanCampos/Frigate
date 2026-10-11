#include "asm/field_script.inc"
#include "text/script/nacrene_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Well, how many gates do you think\nthere are in the Unova region?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_WellHowManyGates, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My friend's friend was saying that\nPokémon Eggs are sometimes discovered[f000]븀\u0000\nat the Day Care on Route 3!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_FriendsFriendSayingPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Are you checking the electric\nbulletin boards?[f000]븁\u0000\nRight now, news about mass outbreaks\nof Pokémon is really hot!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_CheckingElectricBulletinBoards, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01b5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0153
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01b4
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B4
    // "This Pokémon Egg was\nfound at the Day Care.[f000]븀\u0000\nWould you raise it?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_PokemonEggFoundDay, 0, 0
    VMJump L_00C2

L_00B4:
    // "Um... Excuse me...\nTrainer?[f000]븁\u0000\nThis Pokémon Egg was found at the\nDay Care. Would you raise it?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_UmExcuseTrainerPokemon, 0, 0
    FlagSet EVENT_FLAG_0x01b4

L_00C2:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013F
    PokePartyAddEgg 0x8010, 440, 0
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012B
    // "Oh! OK![f000]븁\u0000\nPlease take good care of this Egg![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_OhOkPleaseTake, 0, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01b5
    WordSetPlayerName 0
    MEPlay SEQ_ME_TAMAGO_GET
    // "[f000]Ā\u0001\u0000 received the Egg!"
    SystemMsg NacreneGate_Text_ReceivedEgg, 0
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "Apparently, putting Pokémon Eggs next\nto healthy Pokémon is a good thing.[f000]븁\u0000\nIn other words, walk with the Egg.[f000]븁\u0000\nYou know, the Day Care is on Route 3.\nThey might be able to tell you more."
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_ApparentlyPuttingPokemonEggs, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0139

L_012B:
    // "Um...[f000]븁\u0000\nYour party is full.\nYou don't have room to hold the Egg."
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_UmPartyFullDont, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0139:
    VMJump L_014D

L_013F:
    // "Oh... OK.\nI understand.[f000]븁\u0000\nIf you change your mind,\nplease come talk to me again."
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_OhOkUnderstandIf, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_014D:
    VMJump L_0167

L_0153:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Apparently, putting Pokémon Eggs next\nto healthy Pokémon is a good thing.[f000]븁\u0000\nIn other words, walk with the Egg.[f000]븁\u0000\nYou know, the Day Care is on Route 3.\nThey might be able to tell you more."
    ParentActorMsg MSGFILE_SCRIPT, NacreneGate_Text_ApparentlyPuttingPokemonEggs, 0, 0
    LastKeyWait
    ActorMsgClose

L_0167:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
