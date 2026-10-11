#include "asm/field_script.inc"
#include "text/script/nimbasa_city_13.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0106
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AA
    PokePartyGetMemberByType 0x8008, 2
    PokePartyGetHappiness 0x8009, 0x8008
    VMStackPush 0x8009
    VMStackPushConst 70
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0091
    WordSetPartyPokeSpecies 0, 0x8008
    // "Oh...?[f000]븁\u0000\nOh, my![f000]븁\u0000\nYour [f000]ā\u0001\u0000 seems to like you![f000]븁\u0000\nThat's so nice to see. Makes me want\nto give you this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity13_Text_OhOhSeemsLike, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 218
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0106
    // "If a Pokémon holds a Soothe Bell, it will\nbecome more friendly to you."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity13_Text_IfPokemonHoldsSoothe, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00A4

L_0091:
    WordSetPartyPokeSpecies 0, 0x8008
    // "Oh...?[f000]븁\u0000\nOh, my![f000]븁\u0000\nYour [f000]ā\u0001\u0000's feelings\ntoward you seem to be neutral.[f000]븁\u0000\nIf you can win its friendship,\nI will give you something nice!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity13_Text_OhOhSFeelings, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00A4:
    VMJump L_00B8

L_00AA:
    // "If a Pokémon holds a Soothe Bell, it will\nbecome more friendly to you."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity13_Text_IfPokemonHoldsSoothe, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00B8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A sports game, like baseball or football,\nstarts at a certain time every day.[f000]븁\u0000\nThat's in Big Stadium!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity13_Text_SportsGameLikeBaseball, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In the Small Court, you can find games of\nbasketball and tennis."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity13_Text_SmallCourtCanFind, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
