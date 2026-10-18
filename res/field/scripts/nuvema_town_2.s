#include "asm/field_script.inc"
#include "text/script/nuvema_town_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_3:
    VMHalt

Script_4:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x68000, 0, 0x58000, 40
    EvCameraWait
    // "Welcome home, dear.[f000]븁\u0000\nDid you find the friend you\nwere looking for?[f000]븁\u0000\nWasn't his name something like N?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_WelcomeHomeDearDid, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0288
    ActorCmdWait
    ActorCmdExec 0, Movement_02A0
    ActorCmdWait
    // "Huh...?[f000]븁\u0000\nExcuse me![f000]븁\u0000\nHow embarrassing!\nMistaking a visitor for my own child![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_HuhExcuseHowEmbarrassing, 0, 0, 0
    MsgWinCloseAll
    EvCameraReturn 30
    ActorWalkRoute 0, 6, 9, 0, 8, 1
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMSleep 16
    ActorCmdExec 0, Movement_02B0
    ActorCmdWait
    WordSetPlayerName 0
    // "You're [f000]Ā\u0001\u0000?"
    // "You're [f000]Ā\u0001\u0000?"
    ActorMsgGendered 1024, NuvemaTown2_Text_Youre, NuvemaTown2_Text_Youre_2, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C6
    // "I knew it!\nYou look just like her![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_KnewLookJustLike, 0, 0, 0
    VMJump L_00D2

L_00C6:
    // "Come now! Don't say that!\nMy eyes are actually quite sharp![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_ComeNowDontSay, 0, 0, 0

L_00D2:
    // "I know your mom![f000]븁\u0000\nI met her when she was working\nat the Pokémon Center[f000]븀\u0000\nand I was a Trainer.[f000]븁\u0000\nThat's right! How are your Pokémon?[f000]븁\u0000\nYou're always welcome to let\nthem rest here!"
    ActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_KnowMomMetHer, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 6, 5, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 0, Movement_0288
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x407d, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0161
    // "My, now how are your Pokémon?\nLet them rest here for a moment![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_NowHowPokemonLet, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkSetConst EVENT_WORK_0x400a, 1
    VMCall L_016D
    VMJump L_0167

L_0161:
    VMCall L_016D

L_0167:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_016D:
    Random 0x8010, 100
    VMStackPush 0x8010
    VMStackPushConst 19
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0196
    // "Great! You and your Pokémon\nlook raring to go!"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_GreatPokemonLookRaring, 0, 0
    VMJump L_021C

L_0196:
    VMStackPush 0x8010
    VMStackPushConst 39
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01B9
    // "Pokémon work so hard to help you out![f000]븁\u0000\nBe sure to be kind to them, OK!"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_PokemonWorkHardHelp, 0, 0
    VMJump L_021C

L_01B9:
    VMStackPush 0x8010
    VMStackPushConst 59
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01DC
    // "No news is good news they say.[f000]븁\u0000\nBeing happy about something\nlike that is a little difficult though.[f000]븁\u0000\nBeing a parent is tough."
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_NoNewsGoodNews, 0, 0
    VMJump L_021C

L_01DC:
    VMStackPush 0x8010
    VMStackPushConst 79
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01FF
    // "Maybe I should go on a journey\nto go find my child![f000]븁\u0000\nOh, but I would feel really bad\nif they stopped by while I was gone."
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_MaybeShouldGoJourney, 0, 0
    VMJump L_021C

L_01FF:
    VMStackPush 0x8010
    VMStackPushConst 99
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_021C
    // "I sure wish you two could meet!"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_SureWishTwoCould, 0, 0

L_021C:
    LastKeyWait
    MsgWinCloseAll
    DebugPrint 0x8010
    VMReturn

Script_2:
    ActorsPauseAll
    ActorSetGPos 0, 6, 0, 7, 1
    FadeInBlackQ_
    FadeWait
    WordSetPlayerName 0
    // "                                                                        "
    ActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_Empty, 0, 0, 0
    FadeEx 3, 0, 16, 2
    FadeExWait
    ActorMsgClose
    MEPlay SEQ_ME_ASA
    MEWait
    PokePartyRecoverAll
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "                                                                                                                                                                                                                     "
    ActorMsg MSGFILE_SCRIPT, NuvemaTown2_Text_Empty_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 32, 1
    MoveEnd

Movement_0288:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_02A0:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_02B0:
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd
