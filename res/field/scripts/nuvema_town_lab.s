#include "asm/field_script.inc"
#include "text/script/nuvema_town_lab.h"

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

Script_10:
    HollowRivalCmd_0262 0, 0
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Father and daughter...[f000]븁\u0000\nIt's a picture of the two\nProfessor Junipers."
    InfoMsg NuvemaTownLab_Text_FatherDaughterItsPicture, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Adventure Rule No. 1\nThe X Button opens the menu."
    InfoMsg NuvemaTownLab_Text_AdventureRuleNo1, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Adventure Rule No. 2\nRecord your progress with SAVE."
    InfoMsg NuvemaTownLab_Text_AdventureRuleNo2, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There are lots of books about Pokémon!"
    InfoMsg NuvemaTownLab_Text_ThereLotsBooksAbout, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There are lots of materials and\nresearch reports about Pokémon!"
    InfoMsg NuvemaTownLab_Text_ThereLotsMaterialsResearch, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0184
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0101
    // "Professor Juniper: Why, hello!\nThanks for coming clear out here![f000]븁\u0000\nIt's surprising how far Nuvema Town is\nfrom Aspertia City, don't you agree?[f000]븁\u0000\nDid you take Skyarrow Bridge and\nencounter a lot of Pokémon?[f000]븁\u0000\nOn that note...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_ProfessorJuniperWhyHello, 0, 0
    RTCallGlobal 10380
    ItemCheckAmount ITEM_PERMIT, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F7
    ActorCmdExec 1, Movement_0208
    ActorCmdWait
    // "I have something I'd be delighted to\ngive you if you meet every Pokémon[f000]븀\u0000\nregistered in the Unova Pokédex![f000]븁\u0000\nCheck every corner of the Unova region\nfor Pokémon! Do your best!"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_HaveSomethingIdDelighted, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00F7:
    FlagSet EVENT_FLAG_0x0184
    VMJump L_010F

L_0101:
    // "Professor Juniper: Hi there!\nHow have you been doing lately?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_ProfessorJuniperHiThere, 0, 0
    RTCallGlobal 10380

L_010F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    VMStackPushFlag EVENT_FLAG_0x0183
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014B
    // "Cedric: Hey, [f000]Ā\u0001\u0000!\nAre you meeting lots of Pokémon?[f000]븁\u0000\nThere really are lots of Pokémon in the\nUnova region and the rest of the world![f000]븁\u0000\nI made the Habitat List\nso people would know that![f000]븁\u0000\nI'll bet you're here because...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_CedricHeyMeetingLots, 0, 0
    RTCallGlobal 10381
    FlagSet EVENT_FLAG_0x0183
    VMJump L_0159

L_014B:
    // "Cedric: Hey, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_CedricHey, 0, 0
    RTCallGlobal 10381

L_0159:
    VMStackPushFlag EVENT_FLAG_0x0985
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B8
    ActorCmdExec 0, Movement_0208
    ActorCmdWait
    // "Oh, that's right! I completely forgot\nto give this to you in Aspertia![f000]븁\u0000\nHere, this is the Super Rod![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_OhThatsRightCompletely, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 447
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "With this Super Rod, you can even catch\nPokémon who live underwater![f000]븁\u0000\nHere, I'll read you the directions.[f000]븁\u0000\nFirst...\nFace the water and cast![f000]븁\u0000\nSecond...\nCon-cen-trate![f000]븁\u0000\nWhen a Pokémon bites, you'll see a “!\"\nThat means start reeling in![f000]븁\u0000\nSo cool!"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_SuperRodCanEven, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0985
    MedalDiscover 23

L_01B8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Skreet! Skreet!"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_SkreetSkreet, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 573, 0
    // "Pfoooh!"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTownLab_Text_Pfoooh, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0208:
    Move 75, 1
    MoveEnd
