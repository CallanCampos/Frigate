#include "asm/field_script.inc"
#include "text/script/musical_theater.h"

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
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntriesEnd

Script_10:
    VMStackPush EVENT_WORK_0x4087
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006F
    ActorSetGPos 11, 14, 0, 13, 0
    VMJump L_008E

L_006F:
    VMStackPush EVENT_WORK_0x4087
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008E
    ActorSetGPos 11, 14, 0, 14, 1

L_008E:
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    Cmd_02B4 1, 0x8020
    ActorCmdExec 255, Movement_04BC
    ActorCmdWait
    ActorCmdExec 11, Movement_04C4
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01EA
    // "Hmm... Have we met\nbefore?[f000]븀\u0000\nNever mind. I must be imagining things.[f000]븁\u0000\nI'm the owner of this theater.\nPleasure to meet you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HmmHaveWeMet, 11, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 11, Movement_0484
    ActorCmdWait
    // "Whoa!\nYou have no ordinary aura.[f000]븁\u0000\nIt resembles that of a superstar\nwho attracts a lot of attention.[f000]븁\u0000\nBy the way, do you know\nwhat Dress Up is?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_WhoaHaveNoOrdinary, 11, 0, 0
    WorkSetConst 0x8022, 0
    YesNoWin 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0122
    // "That makes it easy!\nHere's a Prop Case for you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_MakesEasyHeresProp, 11, 0, 0
    VMJump L_012E

L_0122:
    // "In Dress Up, we use Props to make\nyour Pokémon fashionable and glamorous![f000]븁\u0000\nTo get you started, here's a Prop Case![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_DressUpWeUse, 11, 0, 0

L_012E:
    WorkSetConst 0x8022, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 11, Movement_0484
    ActorCmdWait
    Cmd_02B5 1, 0
    // "You know what? I'll give you this\nProp Case![f000]븁\u0000\nIt contains all the Props our superstar\n[f000]Ā\u0001\u0000 used to have.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_KnowWhatIllGive, 11, 0, 0
    MsgWinCloseAll
    // "Would you like a set of Props that are\nthe same as the ones [f000]Ā\u0001\u0000 used?"
    SystemMsg MusicalTheater_Text_WouldLikeSetProps, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D6
    // "You can receive this set of Props\nonly once. Do you want it?"
    SystemMsg MusicalTheater_Text_CanReceiveSetProps, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C2
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4087, 1
    // "Oh, you!\nYou remind me of...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OhRemind, 11, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_MUSICAL_THEATER_3, 14, 0, 15, 1
    VMJump L_01D0

L_01C2:
    MsgWinCloseAll
    VMCall L_02FA
    VMCall L_026E

L_01D0:
    VMJump L_01E4

L_01D6:
    MsgWinCloseAll
    VMCall L_02FA
    VMCall L_026E

L_01E4:
    VMJump L_025C

L_01EA:
    // "Hello! How do you do?[f000]븁\u0000\nI'm the owner of this theater.\nPleasure to meet you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HelloHowImOwner, 11, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 11, Movement_0484
    ActorCmdWait
    // "Ooh! I must say that you seem like\na phenomenal Trainer![f000]븁\u0000\nWhat do we do here? We use Props to make\nyour Pokémon fashionable and glamorous![f000]븁\u0000\nDo you want to join in and play Dress Up?\nTo get you started, here's a Prop Case![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OohMustSaySeem, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_04F0
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 578
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 11, Movement_0504
    ActorCmdWait
    // "This Prop Case lets you store Props for\ndecorating your Pokémon![f000]븁\u0000\nFirst off, you need to pick a Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_PropCaseLetsStore, 11, 0, 0
    MsgWinCloseAll
    VMCall L_026E

L_025C:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_026E:
    Cmd_0167 0, 0, 0, 0
    WorkSetConst 0x8023, 0
    MusicalCmd_0165 11, 0, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B8
    VMSleep 16
    ActorCmdExec 11, Movement_0484
    ActorCmdWait
    // "Aw, I'm sorry, but that Pokémon can't\nplay Dress Up. Please choose a[f000]븀\u0000\ndifferent Pokémon next time.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_AwImSorryBut, 11, 0, 0
    VMJump L_02BE

L_02B8:
    VMCall L_034E

L_02BE:
    WorkSetConst 0x8023, 0
    Cmd_0167 1, 0, 0, 0
    // "If you have a Pokémon that can play\nDress Up, you can participate in the[f000]븀\u0000\nPokémon Musical![f000]븁\u0000\nWould you please join us?\nThe receptionist can explain everything.[f000]븁\u0000\nLet us say a brief farewell!\nI eagerly anticipate seeing you on stage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_IfHavePokemonCan, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_0514
    ActorCmdWait
    ActorSetGPos 11, 17, 6, 3, 1
    WorkSetConst EVENT_WORK_0x4087, 2
    VMReturn

L_02FA:
    // "Are you sure?[f000]븁\u0000\nPlease talk to me again if you want a set\nof Props that are the same as the[f000]븀\u0000\nones [f000]Ā\u0001\u0000 used!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_SurePleaseTalkAgain, 11, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 11, Movement_04F0
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 578
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 11, Movement_0504
    ActorCmdWait
    // "This Prop Case lets you store Props for\ndecorating your Pokémon![f000]븁\u0000\nFirst off, you need to pick a Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_PropCaseLetsStore, 11, 0, 0
    MsgWinCloseAll
    VMReturn

L_034E:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

L_035A:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039A
    Cmd_016A 0x8025, 0x8024
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0394
    // "Now, now. Don't be like that.\nPlease reconsider and select a Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_NowNowDontLike, 11, 0, 0
    MsgWinCloseAll

L_0394:
    VMJump L_035A

L_039A:
    // "Then... It's time to play Dress Up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_ThenItsTimePlay, 11, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    BGMPush 6
    FadeWait
    FieldClose
    MusicalCmd_0164 0x8024
    FieldOpen
    FadeInWhiteQ
    BGMPop 0, 60
    FadeWait
    MusicalCmd_0165 13, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03EE
    // "Hmm, I guess I get it! You are expressing\nyour Pokémon's innate charm by choosing[f000]븀\u0000\nnot to decorate it![f000]븁\u0000\nHonestly, though, I think using Props\nwould be better received in this musical.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HmmGuessGetExpressing, 11, 0, 0
    VMJump L_03FA

L_03EE:
    // "Wow! You have the talent! You did a\nfantastic job coordinating everything![f000]븀\u0000\nUtterly charming![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_WowHaveTalentDid, 11, 0, 0

L_03FA:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    VMReturn

Script_17:
    ActorsPauseAll
    WorkSetConst 0x8026, 0
    FadeInBlackQ
    FadeWait
    // "Mmm, that takes me back!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_MmmTakesBack, 11, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 11, Movement_04F0
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 578
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 11, Movement_0504
    ActorCmdWait
    MusicalCmd_02B7 0x8026
    // "I hope you'll be the superstar of a\nnew generation![f000]븁\u0000\nIt's time to play Dress Up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HopeYoullSuperstarNew, 11, 0, 0
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x404b, 1
    VMCall L_026E
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0484:
    Move 75, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 12, 3
    Move 63, 1
    Move 35, 1
    MoveEnd

Movement_04BC:
    Move 12, 4
    MoveEnd

Movement_04C4:
    Move 1, 1
    Move 75, 1
    Move 13, 1
    MoveEnd
    Move 14, 1
    Move 33, 1
    Move 63, 1
    Move 15, 1
    Move 33, 1
    Move 63, 1
    MoveEnd

Movement_04F0:
    Move 13, 1
    Move 63, 2
    Move 33, 1
    Move 63, 1
    MoveEnd

Movement_0504:
    Move 71, 1
    Move 12, 1
    Move 72, 1
    MoveEnd

Movement_0514:
    Move 14, 9
    MoveEnd
    VMStackSub
    VMHalt
    VMStackDiv
    VMNop2
    VMStackSub
    VMHalt
    .byte 0xfe
    .byte 0x00
    VMNop

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    MusicalCmd_0165 7, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_055E
    // "There is a Musical Photo saved from\nlast time![f000]븁\u0000"
    InfoMsg MusicalTheater_Text_ThereMusicalPhotoSaved, 2
    MsgWinCloseAll
    MusicalCmd_0163 0, 2
    VMJump L_0567

L_055E:
    // "You can hang Musical Photos from your\nprevious shows here."
    InfoMsg MusicalTheater_Text_CanHangMusicalPhotos, 2
    LastKeyWait
    MsgWinCloseAll

L_0567:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Musical Theater\nProps + Music + Dance = Moving Spectacle!"
    InfoMsg MusicalTheater_Text_MusicalTheaterPropsMusic, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you play Dress Up, matching the\nshow you're performing in is im-por-tant![f000]븁\u0000\nIf it's a good match, you can win the\nhearts of the audience![f000]븀\u0000\nYou'll be sure to attract attention.[f000]븁\u0000\nIf you're going to get on stage,\nit's a waste if you don't stand out!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_WhenPlayDressUp, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon Props each have their own image:\ncool, cute, elegant, or quirky.[f000]븁\u0000\nBefore you play Dress Up, take a moment\nto think about the image you prefer."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_PokemonPropsEachHave, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm not trying to brag, but we're\nrather rich.[f000]븁\u0000\nWhen you say rich people, you think\nmusical. It's a matter of taste, I guess."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_ImNotTryingBrag, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what? When there are cute Pokémon,\nmy eyes are glued to the stage!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_KnowWhatWhenThere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Originally, people performed in this\nMusical Theater.[f000]븁\u0000\nOne time, a Pokémon wandered up on\nstage and started imitating the actors.[f000]븁\u0000\nEverybody thought it was sensational!\nTa-daaa! The Pokémon Musical was born."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OriginallyPeoplePerformedMusical, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Cmd_02B4 1, 0x8028
    MusicalCmd_02B6 0, 0x8027
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0750
    MusicalGetOwnedPropCount 0x802a
    Cmd_02B5 1, 0
    VMStackPush 0x802a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06AE
    // "Hmm...[f000]븁\u0000\nYour aura is definitely similar\nto that of the superstar who[f000]븀\u0000\nattracted a lot of attention.[f000]븁\u0000\nHere is a present for you![f000]븁\u0000\nThese are the same Props that\n[f000]Ā\u0001\u0000 used to have.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HmmAuraDefinitelySimilar, 11, 0, 0
    // "Oh, you!\nYou remind me of...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OhRemind, 11, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_MUSICAL_THEATER_3, 14, 0, 15, 1
    VMJump L_074A

L_06AE:
    // "Hmm...[f000]븁\u0000\nYour aura is definitely similar\nto that of the superstar who[f000]븀\u0000\nattracted a lot of attention.[f000]븁\u0000\nHere is a present for you![f000]븁\u0000\nThese are the same Props that\n[f000]Ā\u0001\u0000 used to have.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HmmAuraDefinitelySimilar, 11, 0, 0
    MsgWinCloseAll
    // "Would you like a set of Props that are\nthe same as the ones [f000]Ā\u0001\u0000 used?"
    SystemMsg MusicalTheater_Text_WouldLikeSetProps, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0738
    // "You can receive this set of Props\nonly once. Do you want it?"
    SystemMsg MusicalTheater_Text_CanReceiveSetProps, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0720
    MsgWinCloseAll
    // "Oh, you!\nYou remind me of...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OhRemind, 11, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_MUSICAL_THEATER_3, 14, 0, 15, 1
    VMJump L_0732

L_0720:
    MsgWinCloseAll
    // "Are you sure?[f000]븁\u0000\nPlease talk to me again if you want a set\nof Props that are the same as the[f000]븀\u0000\nones [f000]Ā\u0001\u0000 used!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_SurePleaseTalkAgain, 11, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0732:
    VMJump L_074A

L_0738:
    MsgWinCloseAll
    // "Are you sure?[f000]븁\u0000\nPlease talk to me again if you want a set\nof Props that are the same as the[f000]븀\u0000\nones [f000]Ā\u0001\u0000 used!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_SurePleaseTalkAgain, 11, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_074A:
    VMJump L_0756

L_0750:
    VMCall L_0804

L_0756:
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    WorkSetConst 0x802b, 0
    FadeInBlackQ
    FadeWait
    MusicalCmd_02B7 0x802b
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C8
    WordSetPlayerName 0
    MEPlay SEQ_ME_KEYITEM
    // "[f000]Ā\u0001\u0000 received\na set of Props![f000]븁\u0000"
    SystemMsg MusicalTheater_Text_ReceivedSetProps, 0
    MEWait
    InfoMsgClose
    // "Mmm, that takes me back!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_MmmTakesBack, 11, 0, 0
    MsgWaitAdvance
    // "I believe you'll find a good use\nfor these Props![f000]븁\u0000\nI hope you'll enjoy the musical!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_BelieveYoullFindGood, 11, 0, 0
    VMJump L_07EE

L_07C8:
    // "Mmm, that takes me back!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_MmmTakesBack, 11, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 11, Movement_0484
    ActorCmdWait
    // "Oh my![f000]븁\u0000\nYou already have all the Props\nthe superstar used to have.[f000]븁\u0000\nI knew you were something!"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OhAlreadyHaveAll, 11, 0, 0

L_07EE:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x404b, 1
    WorkSetConst 0x802b, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0804:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802c, 0
    MusicalCmd_0165 15, 24, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08AB
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    TrainerCardGetBirthDate 0x802e, 0x802d
    RTCGetDate 0x8030, 0x802f
    VMStackPush 0x802e
    VMStackPush 0x8030
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPush 0x802f
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0893
    WorkSetConst 0x802c, 1
    // "Happy birthday!\nI've got this festive Prop to give you![f000]븁\u0000\nIt's a cake from me, the owner![f000]븁\u0000\nOf course, this is a Prop, so you should\nattach it to your Pokémon and not eat it.[f000]븁\u0000\nBy the way, even if today isn't your\nbirthday, I won't take it back![f000]븁\u0000\nWhy?\nBecause I'm the owner![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HappyBirthdayIveGot, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8008, 24
    WorkSetConst 0x8009, 0
    RTCallGlobal 10466

L_0893:
    WorkSetConst 0x8030, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802d, 0

L_08AB:
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0921
    MusicalCmd_0165 15, 67, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0921
    FlagGet EVENT_FLAG_0x00f3, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0921
    WorkSetConst 0x802c, 1
    // "Wait! Please, wait![f000]븁\u0000\nYou participated in a musical\nwith your friends, right?[f000]븁\u0000\nI felt like you opened up new possibilities\nfor musicals![f000]븁\u0000\nThat's why I feel impelled to give you\nthis present! Please accept it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_WaitPleaseWaitParticipated, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8008, 67
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "I eagerly anticipate seeing you\non stage again!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_EagerlyAnticipateSeeingStage, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0921:
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0998
    MusicalCmd_0165 15, 55, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0998
    MusicalCmd_0165 1, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0998
    WorkSetConst 0x802c, 1
    // "Your continued participation makes me\na happy owner![f000]븁\u0000\nYou're attracting a lot of attention as\nan up-and-coming stylist.[f000]븁\u0000\nOf course, I'm watching you closely,\nas well.[f000]븁\u0000\nPlease accept this as a token of\nmy appreciation.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_ContinuedParticipationMakesHappy, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8008, 55
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "Ah, I eagerly anticipate seeing you\non stage again![f000]븁\u0000\n...Actually, it feels like I've said this\nline before.[f000]븁\u0000\nWell, that's OK. Because I'm the owner!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_AhEagerlyAnticipateSeeing, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0998:
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A0F
    MusicalCmd_0165 15, 82, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A0F
    MusicalCmd_0165 1, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0A0F
    WorkSetConst 0x802c, 1
    // "Seeing your continued participation\nmakes me a happy owner.[f000]븁\u0000\nApparently, some audience members have\nbeen calling you a top stylist as of late![f000]븁\u0000\nBest of all... How impressive am I for\nrecognizing your talent?[f000]븁\u0000\nPlease accept this as a token of\nmy gratitude![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_SeeingContinuedParticipationMakes, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8008, 82
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "I hope you continue to enjoy the musical!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_HopeContinueEnjoyMusical, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0A0F:
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A30
    // "We really want you to participate in\nthe musical![f000]븁\u0000\nI apologize in advance if any of our\nProps don't suit some kinds of Pokémon.[f000]븁\u0000\nPokémon are individuals, after all!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_WeReallyWantParticipate, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0A30:
    WorkSetConst 0x802c, 0
    VMReturn

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Gwah!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_Gwah, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    MusicalCmd_0165 1, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0A98
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It is such a treat when Trainers have\nplayed Dress Up with their Pokémon[f000]븀\u0000\nwith such charming results."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_SuchTreatWhenTrainers, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0AD9

L_0A98:
    VMStackPush 0x8010
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0AC5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Your Pokémon are wonderful!\nI'm always watching them.[f000]븁\u0000\nI hope you can keep entertaining us with\nyour performances."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_PokemonWonderfulImAlways, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0AD9

L_0AC5:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I always make sure to watch the\nshows you participate in![f000]븁\u0000\nEven from the perspective of a rich\nman like me, the Pokémon Musical[f000]븀\u0000\nis impressive![f000]븁\u0000\nIt's unparalleled entertainment!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_AlwaysMakeSureWatch, 0, 0
    LastKeyWait
    ActorMsgClose

L_0AD9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    FlagGet EVENT_FLAG_0x00f2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B52
    MusicalCmd_0165 6, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B38
    WordSetMusicalInfo 7, 0, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Great work! I saw your Pokémon up\nthere today![f000]븁\u0000\nOverall, totally [f000]ģ\u0001\u0000![f000]븁\u0000\nThe [f000]ģ\u0001\u0001 Prop\nwas a great accent.[f000]븁\u0000\nI noticed that your [f000]ģ\u0001\u0002\nfactor was a bit subdued today.[f000]븁\u0000\nOK! I'm not going to lose!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_GreatWorkSawPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x00f2
    VMJump L_0B4C

L_0B38:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to make Pokémon more glamorous\nthan ever before, so I'm researching the[f000]븀\u0000\nstyles others use when playing Dress Up."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_WantMakePokemonMore, 0, 0
    LastKeyWait
    ActorMsgClose

L_0B4C:
    VMJump L_0B66

L_0B52:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to make Pokémon more glamorous\nthan ever before, so I'm researching the[f000]븀\u0000\nstyles others use when playing Dress Up."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_WantMakePokemonMore, 0, 0
    LastKeyWait
    ActorMsgClose

L_0B66:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 376
    WorkSet 0x8001, 1
    WorkSet 0x8002, 211
    WorkSet 0x8003, 31
    WorkSet 0x8004, 32
    WorkSet 0x8005, 33
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The seats are beyond this entrance,\nbut I think you belong on the[f000]븀\u0000\nspectacular stage!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_SeatsBeyondEntranceBut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    Cmd_02B4 1, 0x8031
    MusicalCmd_02B6 1, 0x8032
    MusicalCmd_0165 0, 0, 0x8033
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8031
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C44
    // "There used to be a Trainer\nwho performed wonderful shows.[f000]븀\u0000\nI was a big fan back then.[f000]븁\u0000\nI wonder if we'll ever see\nanother superstar like that..."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_ThereUsedTrainerWho, 0, 0
    VMJump L_0C87

L_0C44:
    Cmd_02B5 1, 0
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8033
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0C7D
    // "Oh, you seem a bit like [f000]Ā\u0001\u0000!\nI was a big fan a while ago.[f000]븁\u0000\nYou still lack a certain charisma\ncompared to [f000]Ā\u0001\u0000, though."
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OhSeemBitLike, 0, 0
    VMJump L_0C87

L_0C7D:
    // "Oh, you are as wonderful an entertainer\nas [f000]Ā\u0001\u0000![f000]븁\u0000\nI'm a big fan of you now!\nThank you for a wonderful show!"
    ParentActorMsg MSGFILE_SCRIPT, MusicalTheater_Text_OhWonderfulEntertainerIm, 0, 0

L_0C87:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8031, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
