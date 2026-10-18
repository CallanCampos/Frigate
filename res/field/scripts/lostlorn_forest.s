#include "asm/field_script.inc"
#include "text/script/lostlorn_forest.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi, Trainer!\nYou have a Pokédex, I see.[f000]븁\u0000\nI'm a traveler. I enjoy trekking around\nthe world and talking with various people.[f000]븁\u0000\nBy the way, do you know a Pokémon\ncalled Zoroark?"
    ParentActorMsg MSGFILE_SCRIPT, LostlornForest_Text_HiTrainerHavePokedex, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    // "Great![f000]븁\u0000\nAre you armed with knowledge\nso that it won't trick you?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, LostlornForest_Text_GreatArmedKnowledgeWont, 0, 0
    VMJump L_005B

L_0051:
    // "Oh my...[f000]븁\u0000\nDon't you know anything of the world?\nIt may trick you.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, LostlornForest_Text_OhDontKnowAnything, 0, 0

L_005B:
    // "It's your choice to trust it and be\ntricked, or to live in doubt.[f000]븁\u0000\nI enjoyed talking with you.\nThis is a small present.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, LostlornForest_Text_ItsChoiceTrustTricked, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 620
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "They say Zoroark settled in around here\nabout two years ago.[f000]븁\u0000\nIt changed the appearance of this\ngrassland with its Illusion Ability[f000]븀\u0000\nand tricked people and Pokémon.[f000]븁\u0000\nIt's an outrageous rumor, but\na rumor has some truth in it.[f000]븁\u0000\nEvery rumor has a kernel of truth to it.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, LostlornForest_Text_TheySayZoroarkSettled, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CE
    ActorWalkRoute 7, 9, 15, 1, 8, 0
    VMSleep 16
    ActorCmdExec 255, Movement_0170
    ActorCmdWait
    VMJump L_00EA

L_00CE:
    ActorWalkRoute 7, 8, 14, 1, 8, 1
    VMSleep 8
    ActorCmdExec 255, Movement_0170
    ActorCmdWait

L_00EA:
    ActorDelete 7
    VMSleep 2
    VMStackPush 0x8021
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0119
    ActorNew 9, 15, 1, 251, 196, 0
    VMJump L_0127

L_0119:
    ActorNew 8, 14, 1, 251, 196, 0

L_0127:
    VMSleep 6
    ActorCmdExec 255, Movement_0190
    PVPlay 571, 0
    // "Growwwwf!"
    ActorMsg MSGFILE_SCRIPT, LostlornForest_Text_Growwwwf, 251, 0, 0
    PVWait
    MsgWaitAdvance
    ActorMsgClose
    ActorCmdWait
    ActorCmdExec 251, Movement_0188
    ActorCmdWait
    ActorDelete 251
    FlagSet EVENT_FLAG_0x03c7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 32, 1
    MoveEnd

Movement_0170:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0188:
    Move 17, 4
    MoveEnd

Movement_0190:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
