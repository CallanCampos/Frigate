#include "asm/field_script.inc"
#include "text/script/white_forest_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_004B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ho! Hoo!\nWelcome to White Forest![f000]븁\u0000\nIn White Forest, people live\nin harmony with nature and with Pokémon.[f000]븁\u0000\nPlease listen to the voice of the forest\nand of the White Treehollow.[f000]븁\u0000\nThey're saying “Bienvenue!\"\nHo! Hoo! Hoo!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest2_Text_HoHooWelcomeWhite, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00AF

L_004B:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0088
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ho! Hoo![f000]븁\u0000\nIt seems as if you have heard\nthe voice of the White Treehollow.[f000]븁\u0000\nBut you can't hear its true voice\nuntil you go to the deepest part.[f000]븁\u0000\nCome now, do your best!\nHo! Hoo! Hoo!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest2_Text_HoHooSeemsIf, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00AF

L_0088:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00AF
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ho! Hoo! Hoo![f000]븁\u0000\nIt seems as if you have heard the\nvoice of the White Treehollow's heart.[f000]븀\u0000\nPlease let me hear it...[f000]븁\u0000\nNo, wait!\nThings like these shouldn't be[f000]븀\u0000\nheard from someone else![f000]븁\u0000\nIf I want to hear the true voice,\nI should go to the deepest part[f000]븀\u0000\non my own.[f000]븁\u0000\nWell, I guess I'll just have to\ndo my best, then![f000]븁\u0000\nHo! Hoo! Hoo!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest2_Text_HoHooHooSeems, 0, 0
    LastKeyWait
    ActorMsgClose

L_00AF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When people build too many\ntall buildings, the sky gets smaller,[f000]븀\u0000\nand you can't see the sun anymore!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest2_Text_WhenPeopleBuildToo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to know about how Pokémon\nfeel, so I live in nature with them."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest2_Text_WantKnowAboutHow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you know how delicious a meal of\nBerries can be, you don't have to worry[f000]븀\u0000\nabout wanting to eat this and that!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest2_Text_IfKnowHowDelicious, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
