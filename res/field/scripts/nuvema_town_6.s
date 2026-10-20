#include "asm/field_script.inc"
#include "text/script/nuvema_town_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 43
    WorkSet 0x8001, 1
    WorkSet 0x8002, 413
    WorkSet 0x8003, 0
    WorkSet 0x8004, 1
    WorkSet 0x8005, 1
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

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Cmd_02B4 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00B4
    WordSetPlayerName 0
    Cmd_02B5 0, 1
    // "Cheren's Mom: Oh my...\nYou resemble [f000]Ā\u0001\u0001 somehow...[f000]븁\u0000\nYour name's [f000]Ā\u0001\u0000, you say?\nWow! You have a Pokédex, too![f000]븁\u0000\nEveryone grows up like this\nnow, don't they?"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown6_Text_CherensMomOhResemble, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00C5

L_00B4:
    WordSetPlayerName 0
    // "Cheren's Mom: Oh... So your name's\n[f000]Ā\u0001\u0000, then.[f000]븁\u0000\nWow! You have a Pokédex, too![f000]븁\u0000\nEveryone grows up like this now,\ndon't they?"
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown6_Text_CherensMomOhNames, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00C5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
