#include "asm/field_script.inc"
#include "text/script/castelia_city_gym_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMCall L_0042
    MapChangeCore ZONE_CASTELIA_CITY_GYM, 4, 20, 13, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMCall L_0042
    MapChangeCore ZONE_CASTELIA_CITY_GYM, 8, 0, 4, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0042:
    WorkSetConst 0x8020, 0
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_005F
    VMJump L_006D

L_005F:
    ActorCmdExec 255, Movement_00E0
    VMJump L_00D0

L_006D:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0080
    VMJump L_008E

L_0080:
    ActorCmdExec 255, Movement_00EC
    VMJump L_00D0

L_008E:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_00A1
    VMJump L_00AF

L_00A1:
    ActorCmdExec 255, Movement_00F8
    VMJump L_00D0

L_00AF:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_00C2
    VMJump L_00D0

L_00C2:
    ActorCmdExec 255, Movement_0104
    VMJump L_00D0

L_00D0:
    ActorCmdWait
    FadeOutBlackQ
    FadeWait
    WorkSetConst 0x8020, 0
    VMReturn
    .balign 4, 0

Movement_00E0:
    Move 52, 1
    Move 69, 1
    MoveEnd

Movement_00EC:
    Move 53, 1
    Move 69, 1
    MoveEnd

Movement_00F8:
    Move 54, 1
    Move 69, 1
    MoveEnd

Movement_0104:
    Move 55, 1
    Move 69, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013D
    VMCall L_017C
    VMJump L_0176

L_013D:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0166
    // "How many discoveries have you made\nsince you started your adventure?[f000]븁\u0000\nWhen I was a kid, my innocent heart was\ncaptured by the beauty of[f000]븀\u0000\nBug-type Pokémon.[f000]븁\u0000\nI drew with them and battled with them,\nand after all this time, I continue[f000]븀\u0000\nto discover new things.[f000]븁\u0000\nA world shared with Pokémon is a world\nswarming with mysteries."
    ActorMsg MSGFILE_SCRIPT, CasteliaCityGym2_Text_HowManyDiscoveriesHave, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0176

L_0166:
    // "Burgh: Hello!\nHow have you been?[f000]븁\u0000\nSo, now I'm working on\na piece with a Pokémon motif![f000]븀\u0000\nWell, I always do that, really.[f000]븁\u0000\nEvery now and then, I get artist's block.\nBut when I look at my Pokémon...[f000]븁\u0000\nI get filled with the urge to\ndraw, and I can't stop!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCityGym2_Text_BurghHelloHowHave, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0176:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_017C:
    // "Good work back there in the sewers.[f000]븁\u0000\nMy Bug-type Pokémon have been scurrying\nwith excitement about getting to[f000]븀\u0000\nbattle you.[f000]븁\u0000\nI'd say my Bug-type Pokémon are\npretty great![f000]븀\u0000\nC'mon, let me brag a little![f000]븁\u0000\nDwebble's round little eyes are cute!\nIt's resilient and reliable![f000]븁\u0000\nMy ace is Leavanny!\nIt's really the best![f000]븁\u0000\nI think it's so sweet how it makes clothes\nfor other Pokémon out of leaves.[f000]븁\u0000\nOf course, I'm really proud\nof all of my Pokémon![f000]븁\u0000\nWell now...\nLet's get right to it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym2_Text_GoodWorkBackThere, 0, 0
    ActorMsgClose
    WorkSetConst 0x8021, 0
    GameGetDifficulty 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B3
    CallTrainerBattle TRAINER_LEADER_BURGH_2, 0, 0
    VMJump L_01BB

L_01B3:
    CallTrainerBattle TRAINER_LEADER_BURGH, 0, 0

L_01BB:
    WorkSetConst 0x8021, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E0
    CallTrainerBattleEnd
    VMJump L_01E2

L_01E0:
    CallTrainerLose

L_01E2:
    // "Oh hoo...\nYou are very strong indeed![f000]븁\u0000\nI guess it's no surprise I lost.[f000]븁\u0000\nHere! Take this Insect Badge!\nI think it'll suit you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym2_Text_OhHooVeryStrong, 0, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 2
    TrainerCardAddBadge 2
    WordSetPlayerName 0
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x8022, 0
    TrainerCardGetSex 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0224
    PlayFieldEffect 5
    VMJump L_0228

L_0224:
    PlayFieldEffect 57

L_0228:
    MEWait
    WorkSetConst 0x8022, 0
    // "[f000]Ā\u0001\u0000 received the\nInsect Badge from Burgh.[f000]븁\u0000"
    SystemMsg CasteliaCityGym2_Text_ReceivedInsectBadgeFrom, 0
    InfoMsgClose
    // "Ooh! The Insect Badge suits you even\nbetter than I thought it would![f000]븁\u0000\nIf you have three Badges,\nPokémon up to Lv. 40 will obey you,[f000]븀\u0000\nincluding traded Pokémon.[f000]븁\u0000\nAnd, uh, you know what,\nI'll also give you this.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym2_Text_OohInsectBadgeSuits, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 403
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Struggle Bug also lowers the\nSp. Atk of the target that was damaged.[f000]븁\u0000\nI'm the best guy to tell you this.\nIt's the little things that count!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityGym2_Text_StruggleBugAlsoLowers, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x0970
    WorkSetConst EVENT_WORK_0x40b4, 1
    FlagReset EVENT_FLAG_0x02f4
    HollowRivalCmd_0262 1, 5
    FlagSet EVENT_FLAG_0x02f1
    WorkSetConst EVENT_WORK_0x40b2, 4
    TrainerFlagSet TRAINER_HARLEQUIN_ANDERS
    VMReturn
    .balign 4, 0
