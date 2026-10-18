#include "asm/field_script.inc"
#include "text/script/castelia_city_12.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_4:
    VMHalt

Script_5:
    VMStackPush EVENT_WORK_0x40b2
    VMStackPushConst 1
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0055
    ActorSetGPos 0, 11, 65535, 21, 3

L_0055:
    VMHalt

Script_1:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9467, 0, 0x123000, 0x81000, 0, 0xdb000, 40
    ActorWalkRoute 255, 8, 14, 0, 8, 1
    ActorCmdWait
    EvCameraWait
    // "Iris: You can go inside\nthe sewers from here![f000]븁\u0000\nWhaddaya think?\nSeems pretty suspicious, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_IrisCanGoInside, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_03C8
    EvCameraMoveTo 9688, 4480, 0xed000, 0x114000, 0, 0x11a000, 50
    VMSleep 120
    EvCameraMoveTo 9467, 0, 0x123000, 0x81000, 0, 0xdb000, 40
    ActorCmdWait
    FlagReset EVENT_FLAG_0x02f2
    ActorAdd 2
    ActorCmdExec 1, Movement_03C0
    ActorWalkRoute 2, 8, 15, 0, 4, 1
    ActorCmdWait
    ActorCmdExec 2, Movement_03B0
    ActorCmdExec 255, Movement_03B8
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000!\nDid you find Team Plasma?!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_DidFindTeamPlasma, 2, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0143
    // "Uh, thanks...[f000]븁\u0000\nBut you don't need to lie just\nso I won't be disappointed![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_UhThanksButDont, 2, 0, 0
    VMJump L_014F

L_0143:
    // "Augh! Those dirty Pokémon thieves![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_AughThoseDirtyPokemon, 2, 0, 0

L_014F:
    // "That means the only place\nI still haven't checked is...[f000]븁\u0000\n[f000]Ā\u0001\u0000!\nHelp me out![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_MeansOnlyPlaceStill, 2, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    ActorCmdExec 2, Movement_01C0
    VMSleep 16
    ActorCmdExec 1, Movement_03B8
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 2
    SEWait
    ActorCmdExec 1, Movement_03C0
    ActorCmdExec 255, Movement_03C8
    ActorCmdWait
    // "Iris: Yep! The sewers are\na perfect place for hiding!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_IrisYepSewersPerfect, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40b2, 2
    FlagSet EVENT_FLAG_0x02f2
    HollowRivalCmd_0262 1, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01C0:
    Move 17, 2
    Move 19, 2
    Move 17, 4
    Move 19, 9
    Move 16, 4
    Move 18, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 1, Movement_03D0
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 2
    ActorWalkRoute 1, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    // "Iris: Your friend...[f000]븁\u0000\nHe seemed pretty mad.\nDid everything go OK in the sewers?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_IrisFriendHeSeemed, 1, 0, 0
    // "So what are you going to do now?[f000]븁\u0000\nYou ran into Gym Leader Burgh\nin the sewers, didn't you?[f000]븁\u0000\nMaybe you should go to the Pokémon Gym\nand see how far you've come![f000]븁\u0000\nI'm sure battling will help your Pokémon\ncome to understand you better[f000]븀\u0000\nas a Trainer, too!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_WhatGoingNowRan, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40b2, 4
    FlagSet EVENT_FLAG_0x02f0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 1, Movement_03D0
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 1, 6, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 1, Movement_03C0
    ActorCmdWait
    // "Iris: Your friend...[f000]븁\u0000\nHe seemed pretty mad.\nDid everything go OK in the sewers?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_IrisFriendHeSeemed, 1, 0, 0
    // "So what are you going to do now?[f000]븁\u0000\nYou ran into Gym Leader Burgh\nin the sewers, didn't you?[f000]븁\u0000\nMaybe you should go to the Pokémon Gym\nand see how far you've come![f000]븁\u0000\nI'm sure battling will help your Pokémon\ncome to understand you better[f000]븀\u0000\nas a Trainer, too!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_WhatGoingNowRan, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40b2, 4
    FlagSet EVENT_FLAG_0x02f0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40b2
    VMStackPushConst 3
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_02C3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So what are you going to do now?[f000]븁\u0000\nYou ran into Gym Leader Burgh\nin the sewers, didn't you?[f000]븁\u0000\nMaybe you should go to the Pokémon Gym\nand see how far you've come![f000]븁\u0000\nI'm sure battling will help your Pokémon\ncome to understand you better[f000]븀\u0000\nas a Trainer, too!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_WhatGoingNowRan, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02D7

L_02C3:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Iris: Yep! The sewers are\na perfect place for hiding!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_IrisYepSewersPerfect, 0, 0
    LastKeyWait
    ActorMsgClose

L_02D7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Stick up your thumbs, and curl in\nyour fingers.[f000]븁\u0000\nThis is a thumbs-up pose. That means OK!\nIn some places, it also means well done!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_StickUpThumbsCurl, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40b2
    VMStackPushConst 1
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0328
    SEPlay SEQ_SE_MESSAGE
    // "Some Trainers even toughen up\ntheir Pokémon in the sewers..."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_SomeTrainersEvenToughen, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0369

L_0328:
    VMStackPush EVENT_WORK_0x40b2
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0355
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Because of the tide, you sometimes\nmight not be able to get into[f000]븀\u0000\nthe Castelia Sewers.[f000]븀\u0000\nIt depends on the season."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_BecauseTideSometimesMight, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0369

L_0355:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What?\nYou want to go into the sewers?[f000]븁\u0000\nWell, OK... But watch out\nfor wild Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity12_Text_WhatWantGoInto, 0, 0
    LastKeyWait
    ActorMsgClose

L_0369:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
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

Movement_03B0:
    Move 32, 1
    MoveEnd

Movement_03B8:
    Move 33, 1
    MoveEnd

Movement_03C0:
    Move 34, 1
    MoveEnd

Movement_03C8:
    Move 35, 1
    MoveEnd

Movement_03D0:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
