#include "asm/field_script.inc"
#include "text/script/castelia_city_25.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "By taking a Feeling Check, you can get\na Sweet Heart. That's a good item, right?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_ByTakingFeelingCheck, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    Cmd_01CC 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006B
    // "I am the Feeling Reader.[f000]븁\u0000\nI've heard a lot of people take\nFeeling Checks using the C-Gear."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_AmFeelingReaderIve, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00D9

L_006B:
    // "I am the Feeling Reader.[f000]븁\u0000\nFrom the results of your Feeling Checks,\nI'll tell you your lucky person[f000]븀\u0000\nfor today![f000]븁\u0000\nAre you interested?\nDo you want to know your lucky person?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_AmFeelingReaderFrom, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CB
    Cmd_01CD 0
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B7
    // "The person who can make your day\nexceptionally happy is...[f000]븁\u0000\nOh dear. You've taken a Feeling Check\nonly with [f000]Ā\u0001\u0000.[f000]븁\u0000\nI suggest that you take Feeling Checks\nwith a lot of people!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_PersonWhoCanMake_2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00C5

L_00B7:
    // "The person who can make your day\nexceptionally happy...[f000]븁\u0000\nIt's [f000]Ā\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_PersonWhoCanMake, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00C5:
    VMJump L_00D9

L_00CB:
    // "Oh, you don't have to hesitate.\nI will read it for free!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_OhDontHaveHesitate, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00D9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025F
    // "Hello![f000]븁\u0000\nIf you'd like, I will massage\nyour Pokémon."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_HelloIfYoudLike, 5, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0249
    MsgWinCloseAll
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    CallPokeSelect 0, 0x8023, 0x8022, 0
    PokePartyIsEgg 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0233
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_021D
    // "All right! Let me get started.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_AllRightLetGet, 5, 0, 0
    ActorMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    MEPlay SEQ_SE_FLD_145
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    Random EVENT_WORK_0x400a, 100
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 5
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01CA
    PokePartyAdjustHappiness 0x8022, 30, 1
    // "There. All done![f000]븁\u0000\nThe massage has made your Pokémon\nmuch more friendly to you!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_ThereAllDoneMassage, 5, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0213

L_01CA:
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 25
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01FB
    PokePartyAdjustHappiness 0x8022, 10, 1
    // "There. All done![f000]븁\u0000\nThe massage has made your Pokémon\nmore friendly to you!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_ThereAllDoneMassage_2, 5, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0213

L_01FB:
    PokePartyAdjustHappiness 0x8022, 5, 1
    // "There. All done![f000]븁\u0000\nThe massage has made your Pokémon\na little bit more friendly to you!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_ThereAllDoneMassage_3, 5, 0, 0
    LastKeyWait
    ActorMsgClose

L_0213:
    FlagSet EVENT_FLAG_DAILY_0x0ac9
    VMJump L_022D

L_021D:
    // "Oh, I see.\nPlease see me if you change your mind."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_OhSeePleaseSee, 5, 0, 0
    LastKeyWait
    ActorMsgClose

L_022D:
    VMJump L_0243

L_0233:
    // "Massage the Egg?\nIt may be a bit too early for that."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_MassageEggMayBit, 5, 0, 0
    LastKeyWait
    ActorMsgClose

L_0243:
    VMJump L_0259

L_0249:
    // "Oh, I see.\nPlease see me if you change your mind."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_OhSeePleaseSee, 5, 0, 0
    LastKeyWait
    ActorMsgClose

L_0259:
    VMJump L_026F

L_025F:
    // "Sorry!\nI am exhausted from the massage earlier.[f000]븁\u0000\nPlease come back again tomorrow!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_SorryAmExhaustedFrom, 5, 0, 0
    LastKeyWait
    ActorMsgClose

L_026F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can't change the name of a Pokémon\nyou got from someone.[f000]븁\u0000\nBecause the name contains wishes\nof the person who named it!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_CantChangeNamePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Detect: Faafoon!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity25_Text_DetectFaafoon, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
