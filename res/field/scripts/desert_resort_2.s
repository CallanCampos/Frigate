#include "asm/field_script.inc"
#include "text/script/desert_resort_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_4:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh! A sea of sand! I don't need\nGo-Goggles here![f000]븁\u0000\nThe Mirage Tower in a desert\nof the Hoenn region has disappeared.[f000]븁\u0000\nUnova's desert is also swallowing\nup the Relic Castle little by little."
    ParentActorMsg MSGFILE_SCRIPT, DesertResort2_Text_OhSeaSandDont, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I came clear out to the Desert Resort\nto train, but...[f000]븁\u0000\nIt would be so much easier to\nproceed if I had a Water-type Pokémon..."
    ParentActorMsg MSGFILE_SCRIPT, DesertResort2_Text_CameClearOutDesert, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x00d8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D5
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 237
    WorkSet 0x8001, 1
    WorkSet 0x8002, 216
    WorkSet 0x8003, 1
    WorkSet 0x8004, 2
    WorkSet 0x8005, 2
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0116

L_00D5:
    VMStackPushFlag EVENT_FLAG_0x09c7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0102
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "According to what I've heard, that\nRelic Castle is the ruins of a city built[f000]븀\u0000\nby the hero of old and the dragon[f000]븀\u0000\nPokémon that accompanied the hero."
    ParentActorMsg MSGFILE_SCRIPT, DesertResort2_Text_AccordingWhatIveHeard, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0116

L_0102:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If a Pokémon holds this Soft Sand, the\npower of its Ground-type moves goes up!"
    ParentActorMsg MSGFILE_SCRIPT, DesertResort2_Text_IfPokemonHoldsSoft, 0, 0
    LastKeyWait
    ActorMsgClose

L_0116:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x098f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "An expansive desert and\na castle buried in sand![f000]븁\u0000\nThere's no doubt about it!\nTreasure is here![f000]븁\u0000\nIt's been a year since the day my\ninternal treasure detector went off,[f000]븀\u0000\nbut I still haven't found any yet.[f000]븀\u0000\nI'm still following my dream, though...[f000]븁\u0000\nAnd I'm having so much fun\nI can barely stand it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DesertResort2_Text_ExpansiveDesertCastleBuried, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8021, 0
    GameGetVersion 0x8021
    VMStackPush 0x8021
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016D
    Cmd_0275 0, 9, 0
    VMJump L_0174

L_016D:
    Cmd_0275 0, 10, 0

L_0174:
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink!"
    SystemMsg DesertResort2_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "Spending each day living my dream...\nIs THAT my treasure?!"
    ParentActorMsg MSGFILE_SCRIPT, DesertResort2_Text_SpendingEachDayLiving, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x098f
    VMJump L_01B0

L_019C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Spending each day living my dream...\nIs THAT my treasure?!"
    ParentActorMsg MSGFILE_SCRIPT, DesertResort2_Text_SpendingEachDayLiving, 0, 0
    LastKeyWait
    ActorMsgClose

L_01B0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
