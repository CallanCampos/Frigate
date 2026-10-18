#include "asm/field_script.inc"
#include "text/script/undella_town.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

L_005A:
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0081
    ObjInitWarpGPos 6, 707, 65532, 299
    VMJump L_008B

L_0081:
    ObjInitWarpGPos 5, 707, 65532, 299

L_008B:
    VMReturn

Script_3:
    VMCall L_005A
    FlagSet EVENT_FLAG_0x02a6
    WorkSetConst 0x8024, 0
    RTCGetSeason 0x8024
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C4
    FlagReset EVENT_FLAG_0x02a6

L_00C4:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0321
    VMStackPush EVENT_WORK_0x4098
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4098
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0118
    FlagSet EVENT_FLAG_0x02ac
    FlagSet EVENT_FLAG_0x02ad
    FlagSet EVENT_FLAG_0x02af
    FlagSet EVENT_FLAG_0x02b0
    FlagSet EVENT_FLAG_0x02b1
    FlagReset EVENT_FLAG_0x02ab
    VMJump L_031B

L_0118:
    VMStackPush EVENT_WORK_0x4098
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_031B
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0aba
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01E6
    FlagSet EVENT_FLAG_0x02ab
    FlagSet EVENT_FLAG_0x02ac
    FlagSet EVENT_FLAG_0x02ad
    FlagSet EVENT_FLAG_0x02af
    FlagSet EVENT_FLAG_0x02b0
    FlagSet EVENT_FLAG_0x02b1
    FlagReset EVENT_FLAG_0x02ab
    Random 0x8026, 2
    WorkGet EVENT_WORK_0x4166, 0x8026
    Random 0x8025, 4
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01A5
    FlagReset EVENT_FLAG_0x02ac
    Random 0x8026, 5
    WorkGet EVENT_WORK_0x4167, 0x8026
    VMJump L_01A9

L_01A5:
    FlagSet EVENT_FLAG_0x02ac

L_01A9:
    Random 0x8025, 2
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D8
    FlagReset EVENT_FLAG_0x02ad
    Random 0x8026, 5
    WorkGet EVENT_WORK_0x4168, 0x8026
    VMJump L_01DC

L_01D8:
    FlagSet EVENT_FLAG_0x02ad

L_01DC:
    FlagSet EVENT_FLAG_DAILY_0x0aba
    VMJump L_031B

L_01E6:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0aba
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02E0
    FlagSet EVENT_FLAG_0x02ab
    FlagSet EVENT_FLAG_0x02ac
    FlagSet EVENT_FLAG_0x02ad
    FlagSet EVENT_FLAG_0x02af
    FlagSet EVENT_FLAG_0x02b0
    FlagSet EVENT_FLAG_0x02b1
    Random 0x8026, 5
    WorkGet EVENT_WORK_0x4166, 0x8026
    FlagReset EVENT_FLAG_0x02ab
    Random 0x8025, 4
    DebugPrint 0x8025
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0264
    FlagReset EVENT_FLAG_0x02af
    Random 0x8026, 5
    WorkGet EVENT_WORK_0x416a, 0x8026
    VMJump L_0268

L_0264:
    FlagSet EVENT_FLAG_0x02af

L_0268:
    Random 0x8025, 2
    DebugPrint 0x8025
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029B
    FlagReset EVENT_FLAG_0x02b0
    Random 0x8026, 5
    WorkGet EVENT_WORK_0x416b, 0x8026
    VMJump L_029F

L_029B:
    FlagSet EVENT_FLAG_0x02b0

L_029F:
    Random 0x8025, 4
    DebugPrint 0x8025
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D2
    FlagReset EVENT_FLAG_0x02b1
    Random 0x8026, 5
    WorkGet EVENT_WORK_0x416c, 0x8026
    VMJump L_02D6

L_02D2:
    FlagSet EVENT_FLAG_0x02b1

L_02D6:
    FlagSet EVENT_FLAG_DAILY_0x0aba
    VMJump L_031B

L_02E0:
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_031B
    FlagSet EVENT_FLAG_0x02ab
    FlagSet EVENT_FLAG_0x02ac
    FlagSet EVENT_FLAG_0x02ad
    FlagSet EVENT_FLAG_0x02af
    FlagSet EVENT_FLAG_0x02b0
    FlagSet EVENT_FLAG_0x02b1

L_031B:
    VMJump L_0339

L_0321:
    FlagSet EVENT_FLAG_0x02ab
    FlagSet EVENT_FLAG_0x02ac
    FlagSet EVENT_FLAG_0x02ad
    FlagSet EVENT_FLAG_0x02af
    FlagSet EVENT_FLAG_0x02b0
    FlagSet EVENT_FLAG_0x02b1

L_0339:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01a1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0366
    FlagReset EVENT_FLAG_0x03b0
    HollowRivalCmd_0262 1, 41

L_0366:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    VMHalt

Script_16:
    VMCall L_005A
    VMHalt

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8021, 0x8022
    ActorNew 0x8021, 307, 1, 251, 291, 0
    BGMPlay SEQ_BGM_E_HUE
    // "[f000]Ā\u0001\u0001: Wait up![f000]븁\u0000"
    InfoMsg UndellaTown_Text_WaitUp, 2
    ActorCmdExec 255, Movement_0B14
    ActorCmdWait
    InfoMsgClose_0039
    WorkAdd 0x8022, 2
    ActorWalkRoute 251, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait
    // "Let's see how well we've\nraised our Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown_Text_LetsSeeHowWell, 251, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F8
    CallTrainerBattle TRAINER_RIVAL_10, 0, 0
    VMJump L_0421

L_03F8:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0419
    CallTrainerBattle TRAINER_RIVAL_11, 0, 0
    VMJump L_0421

L_0419:
    CallTrainerBattle TRAINER_RIVAL_12, 0, 0

L_0421:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0440
    CallTrainerBattleEnd
    VMJump L_0442

L_0440:
    CallTrainerLose

L_0442:
    // "[f000]Ā\u0001\u0001: Great![f000]븁\u0000\nIf we're this strong, Team Plasma will\nrun screaming when they see us![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown_Text_GreatIfWereStrong, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0B24
    ActorCmdWait
    // "I won't let 'em get away, though![f000]븁\u0000\nHer Purrloin...\nI'll get it back for sure![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WontLetEmGet, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0B0C
    ActorCmdWait
    // "So, [f000]Ā\u0001\u0000!\nKeep helping me out![f000]븁\u0000\nAlso, continue to work hard\non the Pokédex![f000]븁\u0000\nYou're the one who was officially\nasked to complete it, after all![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KeepHelpingOutAlso, 251, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 752
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_04A1
    ActorCmdExec 251, Movement_04E4
    VMJump L_04A9

L_04A1:
    ActorCmdExec 251, Movement_04D8

L_04A9:
    VMSleep 12
    ActorCmdExec 255, Movement_0B0C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40cd, 1
    HollowRivalCmd_0262 1, 21
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04D8:
    Move 15, 1
    Move 12, 5
    MoveEnd

Movement_04E4:
    Move 14, 1
    Move 12, 5
    MoveEnd

Script_12:
    ActorsPauseAll
    MEPlay SEQ_ME_CALL
    // "The Xtransceiver is ringing."
    SystemMsg UndellaTown_Text_XtransceiverRinging, 2
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 picked up the Xtransceiver.[f000]븁\u0000"
    SystemMsg UndellaTown_Text_PickedUpXtransceiver, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    CallXTransceiver 7, 0
    FadeInBlackQ
    FadeWait
    WorkSetConst EVENT_WORK_0x4146, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Undellaaaaa!"
    ActorMsg MSGFILE_SCRIPT, UndellaTown_Text_Undellaaaaa, 0, 0, 1
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    Cmd_02D5 18, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0576
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Cynthia is participating in the\nPokémon World Tournament![f000]븀\u0000\nI have to cheer for her!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_CynthiaParticipatingPokemonWorld, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_058A

L_0576:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokémon World Tournament...\nI wonder if Cynthia will participate, too."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_PokemonWorldTournamentWonder, 0, 0
    LastKeyWait
    ActorMsgClose

L_058A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Riches moved far away,\nand it's a little bit lonelier around here."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_RichesMovedFarAway, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The sunlight is strong...[f000]븁\u0000\nDepending on the Pokémon, that can be\neither an advantage or a disadvantage.[f000]븁\u0000\nStrong sunlight makes Fire-type moves\nstronger and Water-type moves weaker."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_SunlightStrongDependingPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Yaaaay! Yaaay!\nUndella Town!!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_YaaaayYaaayUndellaTown, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We're starting construction to\nfurther develop Undella's resorts.[f000]븁\u0000\nWe just connected to the volcano,\nand we're in awe of nature's power!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WereStartingConstructionFurther, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes Jellicent\nfloat into Undella Bay.[f000]븀\u0000\nThey have a reputation for[f000]븀\u0000\nbeing a little...unusual."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_SometimesJellicentFloatInto, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Undella Town\nA Town of Rippling Waves"
    MsgPlaceSign UndellaTown_Text_UndellaTownTownRippling, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Marine Tube Ahead\nThe Walk-Through Aquarium"
    MsgPlaceSign UndellaTown_Text_MarineTubeAheadWalk, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Reversal Mountain Ahead"
    MsgPlaceSign UndellaTown_Text_ReversalMountainAhead, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPushFlag EVENT_FLAG_0x019e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0800
    // "[f000]Ā\u0001\u0001: What?![f000]븁\u0000\nIt isn't like I came here because\nI heard rumors about Cynthia[f000]븀\u0000\nbeing here and I wanted to challenge[f000]븀\u0000\nher or anything...[f000]븁\u0000\nI was interested in the Abyssal Ruins![f000]븁\u0000\nSee! Here's proof! You can have it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WhatIsntLikeCame, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 425
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x019e
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: It's Dive![f000]븁\u0000\nIf you have a Pokémon that knows it,\nyou can dive to the ocean floor.[f000]븁\u0000\nIf it wasn't for you, I wouldn't have\nfound my sister's Purrloin...[f000]븀\u0000\nOr should I say her Liepard...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_ItsDiveIfHave, 0, 0
    // "OK! [f000]Ā\u0001\u0000![f000]븁\u0000\nLet's see who are Aspertia's\nstrongest Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_OkLetsSeeWho, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07E6
    // "Go get 'em, guys![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_GoGetEmGuys, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0719
    CallTrainerBattle TRAINER_RIVAL_19, 0, 0
    VMJump L_0742

L_0719:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_073A
    CallTrainerBattle TRAINER_RIVAL_20, 0, 0
    VMJump L_0742

L_073A:
    CallTrainerBattle TRAINER_RIVAL_21, 0, 0

L_0742:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0761
    CallTrainerBattleEnd
    VMJump L_0763

L_0761:
    CallTrainerLose

L_0763:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000...[f000]븁\u0000\nI'm really glad you're my friend![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_ImReallyGladYoure, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 300
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07A2
    ActorWalkRoute 6, 752, 296, 1, 8, 0
    VMJump L_07B0

L_07A2:
    ActorWalkRoute 6, 753, 296, 1, 8, 1

L_07B0:
    VMSleep 24
    ActorCmdExec 255, Movement_0B0C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 6
    SEWait
    FlagSet EVENT_FLAG_0x01a1
    FlagSet EVENT_FLAG_0x03b0
    FlagReset EVENT_FLAG_0x03ce
    HollowRivalCmd_0262 1, 42
    VMCall L_093B
    VMJump L_07FA

L_07E6:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "What's the deal?\nDon't act all cool."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WhatsDealDontAct, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_07FA:
    VMJump L_0935

L_0800:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "OK! [f000]Ā\u0001\u0000![f000]븁\u0000\nLet's see who are Aspertia's\nstrongest Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_OkLetsSeeWho, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0921
    // "Go get 'em, guys![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_GoGetEmGuys, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0854
    CallTrainerBattle TRAINER_RIVAL_19, 0, 0
    VMJump L_087D

L_0854:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0875
    CallTrainerBattle TRAINER_RIVAL_20, 0, 0
    VMJump L_087D

L_0875:
    CallTrainerBattle TRAINER_RIVAL_21, 0, 0

L_087D:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_089C
    CallTrainerBattleEnd
    VMJump L_089E

L_089C:
    CallTrainerLose

L_089E:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000...[f000]븁\u0000\nI'm really glad you're my friend![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_ImReallyGladYoure, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 300
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08DD
    ActorWalkRoute 6, 752, 296, 1, 8, 0
    VMJump L_08EB

L_08DD:
    ActorWalkRoute 6, 753, 296, 1, 8, 1

L_08EB:
    VMSleep 24
    ActorCmdExec 255, Movement_0B0C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 6
    SEWait
    FlagSet EVENT_FLAG_0x01a1
    FlagSet EVENT_FLAG_0x03b0
    FlagReset EVENT_FLAG_0x03ce
    HollowRivalCmd_0262 1, 42
    VMCall L_093B
    VMJump L_0935

L_0921:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "What's the deal?\nDon't act all cool."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WhatsDealDontAct, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0935:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_093B:
    FlagReset EVENT_FLAG_0x03d3
    ActorAdd 7
    ActorSetGPos 7, 764, 65531, 304, 2
    PlayerGetGPos 0x8021, 0x8022
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkCmpConst 0x8021, 752
    VMJumpIf CMP_EQ, L_0974
    VMJump L_0986

L_0974:
    WorkSetConst 0x8027, 753
    WorkSetConst 0x8028, 301
    VMJump L_09EF

L_0986:
    WorkCmpConst 0x8021, 753
    VMJumpIf CMP_EQ, L_0999
    VMJump L_09CA

L_0999:
    WorkSetConst 0x8027, 753
    VMStackPush 0x8022
    VMStackPushConst 302
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09BE
    WorkSetConst 0x8028, 303
    VMJump L_09C4

L_09BE:
    WorkSetConst 0x8028, 301

L_09C4:
    VMJump L_09EF

L_09CA:
    WorkCmpConst 0x8021, 754
    VMJumpIf CMP_EQ, L_09DD
    VMJump L_09EF

L_09DD:
    WorkSetConst 0x8027, 753
    WorkSetConst 0x8028, 301
    VMJump L_09EF

L_09EF:
    ActorWalkRoute 7, 0x8027, 0x8028, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_0B2C
    ActorCmdWait
    WorkCmpConst 0x8021, 752
    VMJumpIf CMP_EQ, L_0A1C
    VMJump L_0A32

L_0A1C:
    ActorCmdExec 255, Movement_0B24
    ActorCmdExec 7, Movement_0B1C
    VMJump L_0A84

L_0A32:
    WorkCmpConst 0x8021, 753
    VMJumpIf CMP_EQ, L_0A45
    VMJump L_0A5B

L_0A45:
    ActorCmdExec 255, Movement_0B14
    ActorCmdExec 7, Movement_0B0C
    VMJump L_0A84

L_0A5B:
    WorkCmpConst 0x8021, 754
    VMJumpIf CMP_EQ, L_0A6E
    VMJump L_0A84

L_0A6E:
    ActorCmdExec 255, Movement_0B1C
    ActorCmdExec 7, Movement_0B24
    VMJump L_0A84

L_0A84:
    ActorCmdWait
    // "Zinzolin: Mmm, it's so warm here.[f000]븁\u0000\nLet me get to the point.[f000]븁\u0000\nI have papers that\nLord Ghetsis left behind.[f000]븁\u0000\nWith these, you can read the ancient\nscripts in the Abyssal Ruins.[f000]븁\u0000\nAs my own small, little way to atone\nfor my sins, I'll read them to you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown_Text_ZinzolinMmmItsWarm, 7, 0, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 is now able to read\nthe Abyssal Ruins script!"
    SystemMsg UndellaTown_Text_NowAbleReadAbyssal, 2
    MsgWaitAdvance
    InfoMsgClose
    // "Just to make sure you know,\nyou reach the Abyssal Ruins[f000]븀\u0000\nby using Dive in Undella Bay.[f000]븁\u0000\nAnd you must write down the\nciphers you find in the[f000]븀\u0000\nAbyssal Ruins by yourself."
    ActorMsg MSGFILE_SCRIPT, UndellaTown_Text_JustMakeSureKnow, 7, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ACA
    WorkSetConst EVENT_WORK_0x418f, 1

L_0ACA:
    VMReturn
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_0B0C:
    Move 32, 1
    MoveEnd

Movement_0B14:
    Move 33, 1
    MoveEnd

Movement_0B1C:
    Move 34, 1
    MoveEnd

Movement_0B24:
    Move 35, 1
    MoveEnd

Movement_0B2C:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x418f, 0
    VMJumpIf CMP_EQ, L_0B57
    VMJump L_0B61

L_0B57:
    DebugPrint EVENT_WORK_0x418f
    VMJump L_0D80

L_0B61:
    WorkCmpConst EVENT_WORK_0x418f, 1
    VMJumpIf CMP_EQ, L_0B74
    VMJump L_0B88

L_0B74:
    // "Just to make sure you know,\nyou reach the Abyssal Ruins[f000]븀\u0000\nby using Dive in Undella Bay.[f000]븁\u0000\nAnd you must write down the\nciphers you find in the[f000]븀\u0000\nAbyssal Ruins by yourself."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_JustMakeSureKnow, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0D80

L_0B88:
    WorkCmpConst EVENT_WORK_0x418f, 2
    VMJumpIf CMP_EQ, L_0B9B
    VMJump L_0BB9

L_0B9B:
    // "According to Lord Ghetsis's papers,\nthe ancient peoples read in the[f000]븀\u0000\nopposite direction to how we read now.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_AccordingLordGhetsissPapers, 0, 0
    // "Keep searching![f000]븁\u0000\nYou need to write down the\nAbyssal Ruins ciphers."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KeepSearchingNeedWrite, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0D80

L_0BB9:
    WorkCmpConst EVENT_WORK_0x418f, 3
    VMJumpIf CMP_EQ, L_0BCC
    VMJump L_0BEA

L_0BCC:
    // "This is what was written\nin the papers...[f000]븁\u0000\nIf you reach the second floor,\nread by shifting one letter.[f000]븁\u0000\nIf you reach the third floor,\nread by shifting two letters.[f000]븁\u0000\nWhat I mean by “shifting\" is replacing\na letter with the previous[f000]븀\u0000\nletter in the alphabet.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WhatWrittenPapersIf, 0, 0
    // "Keep searching![f000]븁\u0000\nYou need to write down the\nAbyssal Ruins ciphers."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KeepSearchingNeedWrite, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0D80

L_0BEA:
    WorkCmpConst EVENT_WORK_0x418f, 4
    VMJumpIf CMP_EQ, L_0BFD
    VMJump L_0C2B

L_0BFD:
    // "What?!\nYou made it even further, you say?[f000]븁\u0000\nAnd what kind of ciphers were there?[f000]븁\u0000\n...\n...[f000]븁\u0000\nAhem... Oh, yes, now I see.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WhatMadeEvenFurther, 0, 0
    // "The king must be the presence that\nstopped the war and united the people.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KingMustPresenceStopped, 0, 0
    // "Keep searching![f000]븁\u0000\nYou need to write down the\nAbyssal Ruins ciphers."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KeepSearchingNeedWrite, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x418f, 5
    VMJump L_0D80

L_0C2B:
    WorkCmpConst EVENT_WORK_0x418f, 5
    VMJumpIf CMP_EQ, L_0C3E
    VMJump L_0C5C

L_0C3E:
    // "The king must be the presence that\nstopped the war and united the people.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KingMustPresenceStopped, 0, 0
    // "Keep searching![f000]븁\u0000\nYou need to write down the\nAbyssal Ruins ciphers."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KeepSearchingNeedWrite, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0D80

L_0C5C:
    WorkCmpConst EVENT_WORK_0x418f, 6
    VMJumpIf CMP_EQ, L_0C6F
    VMJump L_0C9D

L_0C6F:
    // "What?!\nYou made it even further, you say?[f000]븁\u0000\nAnd what kind of ciphers were there?[f000]븁\u0000\n...\n...[f000]븁\u0000\nAhem... Oh, yes, now I see.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WhatMadeEvenFurther, 0, 0
    // "The king could see the future\nand talk to all living things.[f000]븀\u0000\nHe united the people.[f000]븁\u0000\nIf that is the truth,\nhe was just like the hero![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KingCouldSeeFuture, 0, 0
    // "Keep searching![f000]븁\u0000\nYou need to write down the\nAbyssal Ruins ciphers."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KeepSearchingNeedWrite, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x418f, 7
    VMJump L_0D80

L_0C9D:
    WorkCmpConst EVENT_WORK_0x418f, 7
    VMJumpIf CMP_EQ, L_0CB0
    VMJump L_0CCE

L_0CB0:
    // "The king could see the future\nand talk to all living things.[f000]븀\u0000\nHe united the people.[f000]븁\u0000\nIf that is the truth,\nhe was just like the hero![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KingCouldSeeFuture, 0, 0
    // "Keep searching![f000]븁\u0000\nYou need to write down the\nAbyssal Ruins ciphers."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_KeepSearchingNeedWrite, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0D80

L_0CCE:
    WorkCmpConst EVENT_WORK_0x418f, 8
    VMJumpIf CMP_EQ, L_0CE1
    VMJump L_0D80

L_0CE1:
    // "What?!\nYou made it even further, you say?[f000]븁\u0000\nAnd what kind of ciphers were there?[f000]븁\u0000\n...\n...[f000]븁\u0000\nAhem... Oh, yes, now I see.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_WhatMadeEvenFurther, 0, 0
    // "There are some characters I can't\nrecognize, but based on the context...[f000]븁\u0000\nAn extremely wonderful king\nwas laid to rest in those ruins.[f000]븁\u0000\nIf that king has descendants,\nmaybe those special powers[f000]븀\u0000\nwere passed down...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_ThereSomeCharactersCant, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 89
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Now that I've learned everything\nI wanted to know about the[f000]븀\u0000\nAbyssal Ruins, I take my leave.[f000]븁\u0000\nYou are my enemy,\nbut your accomplishments and skill[f000]븀\u0000\nmake you a worthwhile enemy. Adieu![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown_Text_NowIveLearnedEverything, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 300
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D50
    ActorWalkRoute 7, 752, 296, 1, 8, 0
    VMJump L_0D5E

L_0D50:
    ActorWalkRoute 7, 753, 296, 1, 8, 1

L_0D5E:
    VMSleep 24
    ActorCmdExec 255, Movement_0B0C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 7
    SEWait
    FlagSet EVENT_FLAG_0x03d3
    VMJump L_0D80

L_0D80:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
