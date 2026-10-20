#include "asm/field_script.inc"
#include "text/script/plasma_frigate_7.h"

// Script plugin 12, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

Script_2:
    GameGetVersion 0x8020
    VMStackPush EVENT_WORK_0x4100
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0063
    FlagSet EVENT_FLAG_0x0345
    VMJump L_00B7

L_0063:
    VMStackPush EVENT_WORK_0x4100
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0090
    FlagReset EVENT_FLAG_0x0345
    VMJump L_00B7

L_0090:
    VMStackPush EVENT_WORK_0x4100
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00B7
    FlagReset EVENT_FLAG_0x0345

L_00B7:
    VMStackPush EVENT_WORK_0x4100
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D8
    FlagSet EVENT_FLAG_0x03e5
    FlagSet EVENT_FLAG_0x03e6
    VMJump L_0114

L_00D8:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F9
    FlagReset EVENT_FLAG_0x03e5
    FlagSet EVENT_FLAG_0x03e6
    VMJump L_0114

L_00F9:
    VMStackPush 0x8020
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0114
    FlagSet EVENT_FLAG_0x03e5
    FlagReset EVENT_FLAG_0x03e6

L_0114:
    VMHalt

Script_3:
    GameGetVersion 0x8020
    VMStackPush EVENT_WORK_0x40ff
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0149
    ActorSetGPos 7, 16, 0, 25, 1

L_0149:
    VMStackPush EVENT_WORK_0x4100
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0178
    ActorSetGPos 0, 16, 0, 27, 0

L_0178:
    VMHalt

Script_4:
    VMHalt

Script_5:
    ActorsPauseAll
    GameGetVersion 0x8020
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A9
    ActorCmdExec 255, Movement_03E0
    VMJump L_01B1

L_01A9:
    ActorCmdExec 255, Movement_03D4

L_01B1:
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D8
    // "[f000]Ā\u0001\u0001: Again?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_Again, 7, 5, 0
    VMJump L_01E4

L_01D8:
    // "[f000]Ā\u0001\u0001: Again?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_Again, 7, 4, 0

L_01E4:
    MsgWinCloseAll
    ActorCmdExec 0, Movement_052C
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0215
    // "Team Plasma: Wait![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_TeamPlasmaWait, 0, 6, 0
    VMJump L_0221

L_0215:
    // "Team Plasma: Wait![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_TeamPlasmaWait, 0, 3, 0

L_0221:
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0248
    // "No need to be suspicious![f000]븁\u0000\nThis uniform is just a disguise.\nI'm ex-Team Plasma![f000]븁\u0000\nWhat I am now is a spy, to guard against\nanything bad they might cook up.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_NoNeedSuspiciousUniform, 0, 6, 0
    VMJump L_0254

L_0248:
    // "No need to be suspicious![f000]븁\u0000\nThis uniform is just a disguise.\nI'm ex-Team Plasma![f000]븁\u0000\nWhat I am now is a spy, to guard against\nanything bad they might cook up.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_NoNeedSuspiciousUniform, 0, 3, 0

L_0254:
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027B
    // "[f000]Ā\u0001\u0001: It must be a hard job,\nkeeping an eye on your former allies.[f000]븁\u0000\nSay, you don't know anything about a\nPurrloin stolen in Aspertia, do you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_MustHardJobKeeping, 7, 5, 0
    VMJump L_0287

L_027B:
    // "[f000]Ā\u0001\u0001: It must be a hard job,\nkeeping an eye on your former allies.[f000]븁\u0000\nSay, you don't know anything about a\nPurrloin stolen in Aspertia, do you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_MustHardJobKeeping, 7, 4, 0

L_0287:
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02AE
    // "S-sorry, but I don't know\nanything about that...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_SSorryButDont, 0, 6, 0
    VMJump L_02BA

L_02AE:
    // "S-sorry, but I don't know\nanything about that...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_SSorryButDont, 0, 3, 0

L_02BA:
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E1
    // "[f000]Ā\u0001\u0001: That's what I figured.[f000]븁\u0000\nI wouldn't expect someone from a\ngroup bent on world domination[f000]븀\u0000\nto be all that quick on the uptake.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_ThatsWhatFiguredWouldnt, 7, 5, 0
    VMJump L_02ED

L_02E1:
    // "[f000]Ā\u0001\u0001: That's what I figured.[f000]븁\u0000\nI wouldn't expect someone from a\ngroup bent on world domination[f000]븀\u0000\nto be all that quick on the uptake.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_ThatsWhatFiguredWouldnt, 7, 4, 0

L_02ED:
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0322
    ActorCmdExec 7, Movement_0408
    VMSleep 4
    ActorCmdExec 255, Movement_0524
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    VMJump L_0338

L_0322:
    ActorCmdExec 7, Movement_03FC
    VMSleep 16
    ActorCmdExec 255, Movement_0514
    ActorCmdWait

L_0338:
    ActorDelete 7
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0351
    SEWait

L_0351:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0372
    ActorCmdExec 255, Movement_0514
    VMJump L_037A

L_0372:
    ActorCmdExec 255, Movement_050C

L_037A:
    ActorCmdWait
    ActorCmdExec 0, Movement_053C
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03AB
    // "Oh...[f000]븁\u0000\nBut I honestly wanted\nto save Pokémon..."
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_OhButHonestlyWanted, 0, 6, 0
    VMJump L_03B7

L_03AB:
    // "Oh...[f000]븁\u0000\nBut I honestly wanted\nto save Pokémon..."
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_OhButHonestlyWanted, 0, 3, 0

L_03B7:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ff, 2
    WorkSetConst EVENT_WORK_0x4149, 1
    FlagSet EVENT_FLAG_0x040c
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03D4:
    Move 15, 1
    Move 12, 1
    MoveEnd

Movement_03E0:
    Move 14, 1
    Move 13, 1
    MoveEnd
    Move 12, 4
    MoveEnd
    Move 13, 4
    MoveEnd

Movement_03FC:
    Move 18, 3
    Move 17, 7
    MoveEnd

Movement_0408:
    Move 19, 3
    Move 17, 3
    Move 19, 2
    MoveEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In order to reach the heart of\nthis ship, you need a password.[f000]븁\u0000\nPlease get the password from\nthe members of Team Plasma."
    // "In this ship, the Plasma Frigate,\nyou move around by using warp panels.[f000]븀\u0000\nLook sharp!"
    ActorMsgVersioned 1024, PlasmaFrigate7_Text_OrderReachHeartShip, PlasmaFrigate7_Text_ShipPlasmaFrigateMove, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0467
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma has disbanded...[f000]븁\u0000\nAnd I finally just learned how to\nuse the warp panels to get around![f000]븁\u0000\nI'm so frustrated![f000]븁\u0000\nSo I'm going to stay here and use\nthe warp panels as much as I want!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_TeamPlasmaHasDisbanded, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_047B

L_0467:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I just joined Team Plasma, and\nthey won't give me a Pokémon yet.[f000]븁\u0000\nBut even a guy like me has\nsomething useful to share.[f000]븁\u0000\nYou need the Plasma Card\nto enter the password!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_JustJoinedTeamPlasma, 0, 0
    LastKeyWait
    ActorMsgClose

L_047B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04B0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma has disbanded...[f000]븁\u0000\nAnd I finally just learned how to\nuse the warp panels to get around![f000]븁\u0000\nI'm so frustrated![f000]븁\u0000\nSo I'm going to stay here and use\nthe warp panels as much as I want!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_TeamPlasmaHasDisbanded, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04C4

L_04B0:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I can't remember how to use\nthe warp panels to get around.[f000]븀\u0000\nAnd they won't give me a Pokémon yet...[f000]븁\u0000\nBut even a guy like me has\nsomething useful to share.[f000]븁\u0000\nYou step on four switches\nto remove the barrier!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate7_Text_CantRememberHowUse, 0, 0
    LastKeyWait
    ActorMsgClose

L_04C4:
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

Movement_050C:
    Move 32, 1
    MoveEnd

Movement_0514:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0524:
    Move 35, 1
    MoveEnd

Movement_052C:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_053C:
    Move 161, 1
    MoveEnd
