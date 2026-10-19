#include "asm/field_script.inc"
#include "text/script/global_10490.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_2:
    VMHalt

Script_3:
    EntreeForestSpawnAllPkm
    VMHalt

Script_4:
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_003E
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_003E:
    AreaNPCCmd_0218 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0069
    WordSetLoadEntreeForestPkmName 0x8011, 0
    // "[f000]ā\u0001\u0000 is\nmoving around energetically!"
    SystemMsg Global10490_Text_MovingAroundEnergetically, 0
    LastKeyWait
    InfoMsgClose
    VMReturn

L_0069:
    PokePartyGetCount 0x8010, 5
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B2
    BoxGetCount 0x8010, 5
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B2
    WordSetLoadEntreeForestPkmName 0x8011, 0
    WordSetPlayerName 1
    // "[f000]ā\u0001\u0000 looks happy\nto meet [f000]Ā\u0001\u0001![f000]븁\u0000"
    SystemMsg Global10490_Text_LooksHappyMeet, 0
    VMCall L_011F
    VMReturn

L_00B2:
    WordSetLoadEntreeForestPkmName 0x8011, 0
    WordSetPlayerName 1
    // "[f000]ā\u0001\u0000 seems to want\nto join [f000]Ā\u0001\u0001's party...[f000]븁\u0000"
    SystemMsg Global10490_Text_SeemsWantJoinS, 2
    // "Would you like to add\nthe Pokémon to your party?"
    SystemMsg Global10490_Text_WouldLikeAddPokemon, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0117
    EntreeForestStartBattle 0x8011, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0113
    RecordAdd 46, 1
    AreaNPCCmd_021D 0x8011
    ActorDelete 0x8011
    CallWildBattleEnd
    MapChangeEntreeForest 9
    VMJump L_0115

L_0113:
    CallWildBattleEnd

L_0115:
    VMReturn

L_0117:
    VMCall L_011F
    VMReturn

L_011F:
    AreaNPCCmd_0218 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A8
    WordSetLoadEntreeForestPkmName 0x8011, 0
    // "Would you like [f000]ā\u0001\u0000\nto move to the Forest Clearing?"
    SystemMsg Global10490_Text_WouldLikeMoveForest, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A2
    AreaNPCCmd_0219 0x8011, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018E
    WordSetLoadEntreeForestPkmName 0x8011, 0
    // "There's no more room in the Forest\nClearing, so [f000]ā\u0001\u0000 could not move!"
    SystemMsg Global10490_Text_TheresNoMoreRoom_2, 2
    LastKeyWait
    InfoMsgClose
    VMReturn
    VMJump L_01A2

L_018E:
    VMCall L_0214
    // "[f000]ā\u0001\u0000 moved to\nthe Forest Clearing![f000]븁\u0000"
    SystemMsg Global10490_Text_MovedForestClearing, 0
    InfoMsgClose
    MapChangeEntreeForest 9
    VMReturn

L_01A2:
    VMJump L_0212

L_01A8:
    WordSetLoadEntreeForestPkmName 0x8011, 0
    // "Would you like [f000]ā\u0001\u0000\nto move to the Deepest Clearing?"
    SystemMsg Global10490_Text_WouldLikeMoveDeepest, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0212
    AreaNPCCmd_0219 0x8011, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FE
    WordSetLoadEntreeForestPkmName 0x8011, 0
    // "There's no more room in the Deepest\nClearing, so [f000]ā\u0001\u0000 could not move!"
    SystemMsg Global10490_Text_TheresNoMoreRoom, 2
    LastKeyWait
    InfoMsgClose
    VMReturn
    VMJump L_0212

L_01FE:
    VMCall L_0214
    // "[f000]ā\u0001\u0000 moved to\nthe Deepest Clearing![f000]븁\u0000"
    SystemMsg Global10490_Text_MovedDeepestClearing, 0
    InfoMsgClose
    MapChangeEntreeForest 9
    VMReturn

L_0212:
    VMReturn

L_0214:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    AreaNPCCmd_0218 3, 0x8020
    AreaNPCCmd_0218 4, 0x8021
    PVPlay 0x8020, 0x8021
    PVWait
    ActorDelete 0x8011
    VMReturn
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0

Script_5:
    ActorsPauseAll
    ActorCmdExec 255, Movement_025C
    ActorCmdWait
    MapChangeEntreeForest 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_025C:
    Move 0, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    ActorCmdExec 255, Movement_027C
    ActorCmdWait
    MapChangeEntreeForest 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_027C:
    Move 1, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    ActorCmdExec 255, Movement_029C
    ActorCmdWait
    MapChangeEntreeForest 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_029C:
    Move 2, 1
    MoveEnd

Script_8:
    ActorsPauseAll
    ActorCmdExec 255, Movement_02BC
    ActorCmdWait
    MapChangeEntreeForest 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_02BC:
    Move 3, 1
    MoveEnd
