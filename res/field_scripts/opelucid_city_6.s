#include "asm/field_script.inc"
#include "text/script/opelucid_city_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 1
    MusicalIsPropOwned 99, EVENT_WORK_0x4001
    MusicalIsPropOwned 98, EVENT_WORK_0x4002
    MusicalIsPropOwned 95, EVENT_WORK_0x4003
    MusicalIsPropOwned 96, EVENT_WORK_0x4004
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4004
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_007A
    WorkSetConst EVENT_WORK_0x4084, 1

L_007A:
    ItemCheckAmount ITEM_PROP_CASE, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0222
    VMStackPush EVENT_WORK_0x4084
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BE
    // "You know what? I'm a huge musical fan! ♪[f000]븁\u0000\nI want people all over the world\nto enjoy musicals. ♪"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatImHuge_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_021C

L_00BE:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0aaa
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E7
    // "You know what? I'm a huge musical fan! ♪[f000]븁\u0000\nIf you come see me again tomorrow,\nI'll give you a different Prop! ♪"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatImHuge, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_021C

L_00E7:
    // "Know what? I am a huge musical fan! ♪[f000]븁\u0000\nOh, you have a Prop Case! You must be\na huge musical fan, too! ♪[f000]븁\u0000\nWould you like a new Prop to use\nin the musical?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatAmHuge_2, 0, 0, 0
    WorkSetConst 0x8020, 0
    YesNoWin 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020C
    // "I am so glad! ♪ You are also a huge\nmusical fan! ♪[f000]븁\u0000\nYou should try putting various Props\non your Pokémon! That would be fun. ♪[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_AmGladAlsoHuge, 0, 0, 0
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015D
    WorkSetConst 0x8008, 99
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "You know what? I'm a huge musical fan! ♪[f000]븁\u0000\nIf you come see me again tomorrow,\nI'll give you a different Prop! ♪"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatImHuge, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0202

L_015D:
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0196
    WorkSetConst 0x8008, 98
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "You know what? I'm a huge musical fan! ♪[f000]븁\u0000\nIf you come see me again tomorrow,\nI'll give you a different Prop! ♪"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatImHuge, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0202

L_0196:
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CF
    WorkSetConst 0x8008, 95
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "You know what? I'm a huge musical fan! ♪[f000]븁\u0000\nIf you come see me again tomorrow,\nI'll give you a different Prop! ♪"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatImHuge, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0202

L_01CF:
    VMStackPush EVENT_WORK_0x4004
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0202
    WorkSetConst 0x8008, 96
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "You know what? I'm a huge musical fan! ♪[f000]븁\u0000\nI want people all over the world\nto enjoy musicals. ♪"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatImHuge_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0202:
    FlagSet EVENT_FLAG_DAILY_0x0aaa
    VMJump L_021C

L_020C:
    // "What?[f000]븁\u0000\nDon't you know how cute Pokémon\nwith Props are?!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_WhatDontKnowHow, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_021C:
    VMJump L_0232

L_0222:
    // "Know what? I am a huge musical fan! ♪[f000]븁\u0000\nWhat's this? What's this, what's this? ♪[f000]븁\u0000\nOh, you don't have a Prop Case![f000]븁\u0000\nWhy don't you go watch the musical\nin Nimbasa City?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatAmHuge, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0232:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you have a lot of Props, it makes you\nwant to put them on Pokémon.[f000]븁\u0000\nIf you put Props on Pokémon, it makes\nyou want to participate in a musical![f000]븁\u0000\nDon't you agree?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_IfHaveLotProps, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what? I hear there is a Prop\nyou can get on your birthday!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity6_Text_KnowWhatHearThere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
