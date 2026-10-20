#include "asm/field_script.inc"
#include "text/script/global_10520.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    FadeInBlackQ_
    FadeWait
    PlayerSetSpecialSequence 64
    ActorCmdExec 255, Movement_0120
    ActorCmdWait
    PlayerSetSpecialSequence 8
    ActorCmdExec 255, Movement_0128
    ActorCmdWait
    PlayerGetGPos 0x8020, 0x8021
    WorkSub 0x8021, 2
    ActorFindByGPos 0x8011, 0x8022, 0x8020, 0, 0x8021
    PokePartyRecoverAll
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009B
    // "First, let's restore your Pokémon\nto full health."
    ActorMsg MSGFILE_SCRIPT, Global10520_Text_FirstLetsRestorePokemon, 0x8011, 0, 0
    VMCall L_0144
    ActorMsgClose

L_009B:
    PlayerSetSpecialSequence 64
    ActorCmdExec 255, Movement_0130
    ActorCmdWait
    PlayerSetSpecialSequence 8
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0105
    ActorCmdExec 0x8011, Movement_0138
    ActorCmdWait
    TrainerCardHasBadge 0x8010, 0
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F5
    // "Please visit a Pokémon Center when your\nPokémon's HP goes down.[f000]븁\u0000\nIf you're planning to travel any\ndistance, you should stock up on Potions[f000]븀\u0000\nat a Poké Mart.[f000]븁\u0000\nGood luck, Trainer!"
    ActorMsg MSGFILE_SCRIPT, Global10520_Text_PleaseVisitPokemonCenter, 0x8011, 0, 0
    VMJump L_0101

L_00F5:
    // "There! All happy and healthy![f000]븁\u0000\nGood luck and take care!"
    ActorMsg MSGFILE_SCRIPT, Global10520_Text_ThereAllHappyHealthy, 0x8011, 0, 0

L_0101:
    LastKeyWait
    ActorMsgClose

L_0105:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0120:
    Move 102, 1
    MoveEnd

Movement_0128:
    Move 0, 1
    MoveEnd

Movement_0130:
    Move 104, 1
    MoveEnd

Movement_0138:
    Move 100, 1
    Move 62, 1
    MoveEnd

L_0144:
    WorkSetConst 0x8023, 0
    ActorCmdExec 0x8011, Movement_0170
    ActorCmdWait
    PokePartyGetCount 0x8023, 1
    PokecenPlayHealingSequence 0x8023
    ActorCmdExec 0x8011, Movement_0178
    ActorCmdWait
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x23
    .byte 0x80
    .balign 4, 0

Movement_0170:
    Move 0, 1
    MoveEnd

Movement_0178:
    Move 1, 1
    MoveEnd

Script_3:
    WorkSetConst 0x8024, 0
    SaveDataCheckRequired 0x8024
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_019D
    VMJump L_01A9

L_019D:
    VMCall Script_4
    VMJump L_01C8

L_01A9:
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_01BC
    VMJump L_01C8

L_01BC:
    WorkSetConst 0x8000, 0
    VMJump L_01C8

L_01C8:
    RTEndGlobal
    WorkSetConst 0x8024, 0
    VMHalt

Script_4:
    VMCall L_01DC
    RTEndGlobal
    VMHalt

L_01DC:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8001, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8004, 0
    WorkSetConst 0x8029, 0
    SaveDataGetStatus 0x8026, 0x8027, 0x8028
    FieldSubscreenChange 0
    // "Would you like to save the game?"
    SystemMsg Global10520_Text_WouldLikeSaveGame, 2
    YesNoWin 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0247
    WorkSetConst 0x8025, 1

L_0247:
    InfoMsgClose
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_028A
    // "There is already a saved game file.\nIt is impossible to save.[f000]븁\u0000\nPlease refer to the Instruction\nBooklet for details.[f000]븁\u0000\nPress Up + SELECT + B Button on\nthe title screen if you want to erase[f000]븀\u0000\nthe current saved game file.[f000]븁\u0000"
    SystemMsg Global10520_Text_ThereAlreadySavedGame_2, 2
    InfoMsgClose
    WorkSetConst 0x8025, 0

L_028A:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0329
    WorkCmpConst 0x8028, 0
    VMJumpIf CMP_EQ, L_02B0
    VMJump L_02BC

L_02B0:
    // "Saving...\nDon't turn off the power."
    SystemMsgAsync Global10520_Text_SavingDontTurnOff, 2
    VMJump L_02DB

L_02BC:
    WorkCmpConst 0x8028, 1
    VMJumpIf CMP_EQ, L_02CF
    VMJump L_02DB

L_02CF:
    // "Saving a lot of data...\nDon't turn off the power."
    SystemMsgAsync Global10520_Text_SavingLotDataDont, 2
    VMJump L_02DB

L_02DB:
    VMSleep 1
    MsgSetLoadingSpinner 0
    SaveDataWrite 0x8029
    SEStop
    InfoMsgClose
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_031B
    SEPlay SEQ_SE_SAVE
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 saved the game."
    SystemMsg Global10520_Text_SavedGame, 2
    MsgWaitAdvance
    InfoMsgClose
    WorkSetConst 0x8000, 0
    VMJump L_0329

L_031B:
    // "Save error.[f000]븁\u0000"
    SystemMsg Global10520_Text_SaveError, 2
    InfoMsgClose
    WorkSetConst 0x8000, 1

L_0329:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0342
    WorkSetConst 0x8000, 1

L_0342:
    FieldSubscreenReturn
    VMReturn
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0

Script_5:
    VMCall L_036E
    RTEndGlobal
    VMHalt

L_036E:
    GameCommGetStatus 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_038D
    WorkSetConst 0x8000, 0
    VMReturn

L_038D:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D9
    // "It looks like you are connecting\nwith somebody. Is it OK to disconnect?"
    SystemMsg Global10520_Text_LooksLikeConnectingSomebody, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C7
    WorkSetConst 0x8000, 1
    InfoMsgClose
    VMReturn

L_03C7:
    // "Ending communication.\nPlease wait."
    SystemMsg Global10520_Text_EndingCommunicationPleaseWait, 2
    GameCommDisconnect 0x8000
    InfoMsgClose
    VMJump L_03DD

L_03D9:
    GameCommDisconnect 0x8000

L_03DD:
    VMReturn

Script_6:
    // "Wireless communications are turned OFF.\nTurn wireless communications ON[f000]븀\u0000\nin the System Settings.[f000]븀\u0000\nError code: 50699[f000]븁\u0000"
    SystemMsg Global10520_Text_WirelessCommunicationsTurnedOff, 2
    InfoMsgClose
    RTEndGlobal

Script_7:
    ActorsPauseAll
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802a, 24
    WorkAdd 0x802a, 0x8000
    WordSetPlayerName 0
    WordSetItemName 1, 0x8001
    SystemMsg 0x802a, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst 0x802a, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
