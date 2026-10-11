#include "asm/field_script.inc"
#include "text/script/striaton_city_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh my! You have eight Gym Badges?!\nWhy, you must be very strong![f000]븁\u0000\nBut, I wonder what would separate\nTrainers who both have eight Badges..."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_OhHaveEightGym, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There's a model who I've been\na fan of for years![f000]븁\u0000\nHer name is Elesa,\nand her Pokémon are strong, too![f000]븁\u0000\nHuh? You've battled with her?\nYou're really something!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_TheresModelWhoIve, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The GTS! It links the world from the\nsecond floor of a Pokémon Center![f000]븁\u0000\nThe full name of the GTS is the\nGlobal Trade Station![f000]븁\u0000\nNow in Driftveil City, you can find the\nPokémon World Tournament.[f000]븀\u0000\nIt's the PWT for short!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_GtsLinksWorldFrom, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0997
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0245
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi, hi!\nLet's play Pokémon rock-paper-scissors![f000]븁\u0000\nI'm really good at it!\nI've beaten all of my friends![f000]븀\u0000\nAre you ready?[f000]븁\u0000\nHere we go!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_HiHiLetsPlay, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    ListMenuAdd 4, 65535, 0
    ListMenuAdd 5, 65535, 1
    ListMenuAdd 6, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_00E5
    VMJump L_00F7

L_00E5:
    WorkSetConst 0x8024, 4
    WorkSetConst 0x8025, 5
    VMJump L_0141

L_00F7:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_010A
    VMJump L_011C

L_010A:
    WorkSetConst 0x8024, 5
    WorkSetConst 0x8025, 6
    VMJump L_0141

L_011C:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_012F
    VMJump L_0141

L_012F:
    WorkSetConst 0x8024, 6
    WorkSetConst 0x8025, 4
    VMJump L_0141

L_0141:
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0158
    VMJump L_0176

L_0158:
    WorkSetConst 0x8020, 14
    WorkSetConst 0x8021, 15
    WorkSetConst 0x8022, 14
    WorkSetConst 0x8023, 4
    VMJump L_01D8

L_0176:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0189
    VMJump L_01A7

L_0189:
    WorkSetConst 0x8020, 10
    WorkSetConst 0x8021, 6
    WorkSetConst 0x8022, 20
    WorkSetConst 0x8023, 13
    VMJump L_01D8

L_01A7:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_01BA
    VMJump L_01D8

L_01BA:
    WorkSetConst 0x8020, 18
    WorkSetConst 0x8021, 6
    WorkSetConst 0x8022, 9
    WorkSetConst 0x8023, 14
    VMJump L_01D8

L_01D8:
    MsgWinCloseAll
    // "Shoot!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_Shoot, 3, 2, 1
    MsgWaitAdvance
    MsgWinCloseAll
    MultiMsg 0x8024, 0x8020, 0x8021, 1
    MultiMsg 0x8025, 0x8022, 0x8023, 2
    VMSleep 60
    MsgWinCloseNo 1
    MsgWinCloseNo 2
    // "Waaaah! I lost!\nMy win streak's over... Sniff...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_WaaaahLostWinStreaks, 2, 0
    MsgWinCloseAll
    Cmd_0275 0, 41, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink!"
    SystemMsg StriatonCity7_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "Until I figure out a way\nto win every time for sure,[f000]븀\u0000\nI won't play anymore!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_UntilFigureOutWay, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0997
    VMJump L_0259

L_0245:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Until I figure out a way\nto win every time for sure,[f000]븀\u0000\nI won't play anymore!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity7_Text_UntilFigureOutWay, 0, 0
    LastKeyWait
    ActorMsgClose

L_0259:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
