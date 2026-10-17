#include "asm/field_script.inc"
#include "text/script/nuvema_town_3.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_1:
    Cmd_02B2 3, 0x8020
    Cmd_02B2 4, 0x8021
    Cmd_02B2 5, 0x8022
    Cmd_02B2 6, 0x8023
    Cmd_02B2 7, 0x8024
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007D
    FlagReset EVENT_FLAG_0x0273

L_007D:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0094
    FlagReset EVENT_FLAG_0x0274

L_0094:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AB
    FlagReset EVENT_FLAG_0x0275

L_00AB:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C2
    FlagReset EVENT_FLAG_0x0276

L_00C2:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D9
    FlagReset EVENT_FLAG_0x0277

L_00D9:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a Wii console!\nIt has a Wii Remote!"
    SystemMsg NuvemaTown3_Text_ItsWiiConsoleHas, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a shiny flat-screen television\nthat someone has been polishing..."
    InfoMsg NuvemaTown3_Text_ItsShinyFlatScreen, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This PC doesn't look like it's\nbeen used in a while..."
    SystemMsg NuvemaTown3_Text_PcDoesntLookLike, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The sheets on the bed don't have\na single wrinkle."
    SystemMsg NuvemaTown3_Text_SheetsBedDontHave, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's an award for completing\nthe Unova Pokédex!"
    InfoMsg NuvemaTown3_Text_ItsAwardCompletingUnova, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's an award for completing\nthe National Mode Pokédex!"
    InfoMsg NuvemaTown3_Text_ItsAwardCompletingNational, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a trophy proving you defeated\nthe Single Master in the Battle Subway!"
    InfoMsg NuvemaTown3_Text_ItsTrophyProvingDefeated, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a trophy proving you defeated\nthe Double Master in the Battle Subway!"
    InfoMsg NuvemaTown3_Text_ItsTrophyProvingDefeated_2, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a trophy for defeating\nthe Multi Master in the Battle Subway!"
    InfoMsg NuvemaTown3_Text_ItsTrophyDefeatingMulti, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
