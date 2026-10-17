#include "asm/field_script.inc"
#include "text/script/aspertia_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    ActorCmdExec 0, Movement_0224
    ActorCmdWait
    WordSetPlayerName 0
    // "Hey! [f000]Ā\u0001\u0000!\nYou can't go without...[f000]븁\u0000\n...Oh?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_HeyCantGoWithout, 0, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007A
    ActorCmdExec 255, Movement_0204
    VMJump L_0088

L_007A:
    ActorWalkRoute 255, 7, 0x8022, 1, 8, 0

L_0088:
    ActorCmdWait
    WordSetPlayerName 0
    PokePartyGetMemberByType 0x8023, 2
    WordSetPartyPokeSpecies 1, 0x8023
    // "You're with [f000]ā\u0001\u0001![f000]븁\u0000\nOK. This is a going-away gift!\nDon't be shy. Take it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_YoureOkGoingAway, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    WordSetPlayerName 0
    // "When Pokémon get hurt, take it easy\nand go to a Pokémon Center."
    ActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_WhenPokemonGetHurt, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40e0, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40e0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0162
    WordSetPlayerName 0
    // "Hey! [f000]Ā\u0001\u0000!\nYou can't go without...[f000]븁\u0000\n...Oh?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_HeyCantGoWithout, 0, 0
    WordSetPlayerName 0
    PokePartyGetMemberByType 0x8023, 2
    WordSetPartyPokeSpecies 1, 0x8023
    // "You're with [f000]ā\u0001\u0001![f000]븁\u0000\nOK. This is a going-away gift!\nDon't be shy. Take it.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_YoureOkGoingAway, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    WordSetPlayerName 0
    // "When Pokémon get hurt, take it easy\nand go to a Pokémon Center."
    ActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_WhenPokemonGetHurt, 0, 0, 0
    WorkSetConst EVENT_WORK_0x40e0, 1
    VMJump L_016F

L_0162:
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000,\nhow is your journey?"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_HowJourney, 0, 0

L_016F:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A8
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Remember that?[f000]븁\u0000\nThe day you passed this gate\nwith your Pokémon for the first time."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_RememberDayPassedGate, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01FD

L_01A8:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hmm...\nI see. I see![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_HmmSeeSee, 1, 0, 0
    // "I love to read news and information\nabout the city displayed[f000]븀\u0000\non the electric bulletin board[f000]븀\u0000\non the wall!"
    ActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_LoveReadNewsInformation, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_01FD

L_01E9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I love to read news and information\nabout the city displayed[f000]븀\u0000\non the electric bulletin board[f000]븀\u0000\non the wall!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaGate_Text_LoveReadNewsInformation, 0, 0
    LastKeyWait
    ActorMsgClose

L_01FD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0204:
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_0224:
    Move 75, 1
    MoveEnd
