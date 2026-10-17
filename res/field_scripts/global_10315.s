#include "asm/field_script.inc"
#include "text/script/global_10315.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8028, 0
    SEPlay SEQ_SE_MESSAGE
    PokePartyGetCountBySpecies 479, 0x8028
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006D
    WorkSetConst 0x8022, 0
    VMCall L_0132
    VMJump L_012C

L_006D:
    // "It's full of cardboard boxes with\nelectrical appliances in them.[f000]븁\u0000\nOh? Rotom would like to investigate the\nmotors of the electrical appliances...[f000]븁\u0000\nIs that OK?"
    SystemMsg Global10315_Text_ItsFullCardboardBoxes_2, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0120
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C4
    PokePartyFindBySpecies 479, 0x8010, 0x8021
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BE
    VMCall L_01CC

L_00BE:
    VMJump L_011A

L_00C4:
    // "Which Rotom will you allow\nto enter a motor?[f000]븁\u0000"
    SystemMsg Global10315_Text_WhichRotomWillAllow, 2
    InfoMsgClose
    CallPokeSelect 0, 0x8010, 0x8021, 0
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010E
    VMCall L_0150
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0108
    VMCall L_01CC

L_0108:
    VMJump L_011A

L_010E:
    WorkSetConst 0x8022, 2
    VMCall L_0132

L_011A:
    VMJump L_012C

L_0120:
    WorkSetConst 0x8022, 2
    VMCall L_0132

L_012C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0132:
    SystemMsg 0x8022, 2
    LastKeyWait
    InfoMsgClose
    VMReturn

L_013E:
    SystemMsg 0x8022, 2
    MEPlay SEQ_ME_LVUP
    MEWait
    LastKeyWait
    InfoMsgClose
    VMReturn

L_0150:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8010, 0
    PokePartyIsEgg 0x802a, 0x8021
    VMStackPush 0x802a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0193
    WorkSetConst 0x8022, 5
    VMCall L_0132
    WorkSetConst 0x8010, 0
    VMJump L_01CA

L_0193:
    PokePartyGetSpecies 0x8029, 0x8021
    VMStackPush 0x8029
    VMStackPushConst 479
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B8
    WorkSetConst 0x8010, 1
    VMJump L_01CA

L_01B8:
    WorkSetConst 0x8022, 4
    VMCall L_0132
    WorkSetConst 0x8010, 0

L_01CA:
    VMReturn

L_01CC:
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    PokePartyGetForme 0x8020, 0x8021
    WordSetPartyPokeName 0, 0x8021

L_01EF:
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02FF
    // "Which appliance's motor will you\nallow [f000]Ă\u0001\u0000 to enter?"
    SystemMsg Global10315_Text_WhichAppliancesMotorWill, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32813
    ListMenuAdd 7, 65535, 1
    ListMenuAdd 8, 65535, 2
    ListMenuAdd 10, 65535, 3
    ListMenuAdd 9, 65535, 4
    ListMenuAdd 11, 65535, 5
    ListMenuAdd 12, 65535, 0
    ListMenuAdd 13, 65535, 6
    ListMenuShow
    VMStackPush 0x802d
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0272
    WorkSetConst 0x8022, 25
    VMCall L_0132
    VMReturn
    VMJump L_0293

L_0272:
    VMStackPush 0x802d
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0293
    WorkSetConst 0x8022, 25
    VMCall L_0132
    VMReturn

L_0293:
    WorkGet 0x802b, 0x802d
    WorkGet 0x8026, 0x802b
    VMCall L_038F
    WorkGet 0x8024, 0x8027
    VMStackPush 0x8020
    VMStackPush 0x802b
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F3
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E2
    WordSetPartyPokeName 0, 0x8021
    // "[f000]Ă\u0001\u0000 hasn't entered a motor.[f000]븁\u0000"
    SystemMsg Global10315_Text_HasntEnteredMotor, 2
    VMJump L_02ED

L_02E2:
    WordSetPartyPokeName 0, 0x8021
    // "This [f000]Ă\u0001\u0000 has already entered\nthat appliance motor.[f000]븁\u0000"
    SystemMsg Global10315_Text_HasAlreadyEnteredAppliance, 2

L_02ED:
    VMJump L_02F9

L_02F3:
    WorkSetConst 0x802c, 1

L_02F9:
    VMJump L_01EF

L_02FF:
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0335
    VMCall L_044B
    WordSetPartyPokeName 0, 0x8021
    WorkSetConst 0x8022, 23
    VMCall L_0132
    WorkSetConst 0x8010, 1
    VMJump L_0356

L_0335:
    WordSetPartyPokeName 0, 0x8021
    PVPlay 479, 0
    // "[f000]Ă\u0001\u0000 entered the motor."
    SystemMsg Global10315_Text_EnteredMotor, 2
    PVWait
    MsgWaitAdvance
    WorkSetConst 0x8023, 0
    VMCall L_04C1

L_0356:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0377
    PokePartyChangeRotomForme 0x8021, 0x8023, 0x802b
    VMJump L_038D

L_0377:
    WordSetPartyPokeName 0, 0x8021
    WordSetMoveName 1, 0x8024
    WorkSetConst 0x8022, 17
    VMCall L_0132

L_038D:
    VMReturn

L_038F:
    WorkCmpConst 0x8026, 0
    VMJumpIf CMP_EQ, L_03A2
    VMJump L_03AE

L_03A2:
    WorkSetConst 0x8027, 84
    VMJump L_0449

L_03AE:
    WorkCmpConst 0x8026, 1
    VMJumpIf CMP_EQ, L_03C1
    VMJump L_03CD

L_03C1:
    WorkSetConst 0x8027, 315
    VMJump L_0449

L_03CD:
    WorkCmpConst 0x8026, 2
    VMJumpIf CMP_EQ, L_03E0
    VMJump L_03EC

L_03E0:
    WorkSetConst 0x8027, 56
    VMJump L_0449

L_03EC:
    WorkCmpConst 0x8026, 3
    VMJumpIf CMP_EQ, L_03FF
    VMJump L_040B

L_03FF:
    WorkSetConst 0x8027, 59
    VMJump L_0449

L_040B:
    WorkCmpConst 0x8026, 4
    VMJumpIf CMP_EQ, L_041E
    VMJump L_042A

L_041E:
    WorkSetConst 0x8027, 403
    VMJump L_0449

L_042A:
    WorkCmpConst 0x8026, 5
    VMJumpIf CMP_EQ, L_043D
    VMJump L_0449

L_043D:
    WorkSetConst 0x8027, 437
    VMJump L_0449

L_0449:
    VMReturn

L_044B:
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkGet 0x8026, 0x8020
    VMCall L_038F
    WorkGet 0x802e, 0x8027
    PokePartyHasMove 0x8010, 0x802e, 0x8021
    PokePartyGetMoveCount 0x802f, 0x8021
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04BF
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04AF
    WorkGet 0x8025, 0x802e
    VMCall L_0574
    VMJump L_04BF

L_04AF:
    WordSetPartyPokeName 0, 0x8021
    WordSetMoveName 2, 0x802e
    // "[f000]Ă\u0001\u0000 forgot [f000]ć\u0001\u0002...[f000]븁\u0000"
    SystemMsg Global10315_Text_Forgot, 2

L_04BF:
    VMReturn

L_04C1:
    WorkSetConst 0x8030, 0
    WorkGet 0x8026, 0x8020
    VMCall L_038F
    WorkGet 0x8030, 0x8027
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_04FA
    PokePartyHasMove 0x8010, 0x8030, 0x8021
    VMJump L_0500

L_04FA:
    WorkSetConst 0x8010, 0

L_0500:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_052B
    WorkGet 0x8025, 0x8030
    VMCall L_0574
    WorkSetConst 0x8010, 1
    VMJump L_0572

L_052B:
    WorkSetConst 0x8031, 0
    PokePartyGetMoveCount 0x8031, 0x8021
    VMStackPush 0x8031
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0556
    VMCall L_0597
    VMJump L_0572

L_0556:
    WorkSetConst 0x8010, 1
    WordSetPartyPokeName 0, 0x8021
    WordSetMoveName 2, 0x8024
    WorkSetConst 0x8022, 20
    VMCall L_013E

L_0572:
    VMReturn

L_0574:
    WordSetPartyPokeName 0, 0x8021
    WordSetMoveName 1, 0x8025
    WordSetMoveName 2, 0x8024
    // "1, [f000]븂\u0001\u00142, and[f000]븂\u0001\u0014... [f000]븂\u0001\u0014... [f000]븂\u0001\u0014... Ta-da![f000]븅\u0001\u0003[f000]븅\u0001\u0006[f000]븁\u0000\n[f000]Ă\u0001\u0000 forgot how to\nuse [f000]ć\u0001\u0001.[f000]븁\u0000\nAnd...[f000]븁\u0000"
    SystemMsg Global10315_Text_12TaDa, 2
    WorkSetConst 0x8022, 20
    VMCall L_013E
    VMReturn

L_0597:
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8032, 0

L_05A3:
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06C5
    WordSetPartyPokeName 0, 0x8021
    WordSetMoveName 1, 0x8024
    // "[f000]Ă\u0001\u0000 is trying to\nlearn [f000]ć\u0001\u0001.[f000]븁\u0000\nBut [f000]Ă\u0001\u0000 can't learn\nmore than four moves.[f000]븁\u0000\nDelete a move to make\nroom for [f000]ć\u0001\u0001?"
    SystemMsg Global10315_Text_TryingLearnButCant, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0698
    InfoMsgClose
    CallPokeMoveReplace 0x8010, 0x8023, 0x8021, 0x8024
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0629
    VMCall L_06C7
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0623
    InfoMsgClose
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8032, 1

L_0623:
    VMJump L_0692

L_0629:
    PokePartyGetMove 0x8025, 0x8021, 0x8023
    WordSetMoveName 1, 0x8025
    // "Is it OK to forget\nthe move [f000]ć\u0001\u0001?"
    SystemMsg Global10315_Text_OkForgetMove, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_066B
    VMCall L_0574
    WorkSetConst 0x8010, 1
    WorkSetConst 0x8032, 1
    VMJump L_0692

L_066B:
    VMCall L_06C7
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0692
    InfoMsgClose
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8032, 1

L_0692:
    VMJump L_06BF

L_0698:
    VMCall L_06C7
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06BF
    InfoMsgClose
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8032, 1

L_06BF:
    VMJump L_05A3

L_06C5:
    VMReturn

L_06C7:
    WordSetMoveName 1, 0x8024
    // "Give up on learning the\nmove [f000]ć\u0001\u0001?"
    SystemMsg Global10315_Text_GiveUpLearningMove, 2
    YesNoWin 0x8010
    VMReturn
