#include "asm/field_script.inc"
#include "text/script/global_10090.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    FlagGet EVENT_FLAG_0x096b, 0x8025
    FlagGet EVENT_FLAG_0x0962, 0x8026
    FlagGet EVENT_FLAG_0x0960, 0x8022
    HOFCheckIntegrity 0x8023
    PokecenPCOpen
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 booted up the PC.[f000]븁\u0000"
    SystemMsg Global10090_Text_BootedUpPc, 2

L_005F:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0226
    // "Which PC should be accessed?"
    SystemMsg Global10090_Text_WhichPcShouldAccessed, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32804
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A2
    ListMenuAdd 2, 65535, 2
    VMJump L_00AA

L_00A2:
    ListMenuAdd 3, 65535, 3

L_00AA:
    WordSetPlayerName 0
    ListMenuAdd 4, 65535, 4
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D0
    ListMenuAdd 5, 65535, 5

L_00D0:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EB
    ListMenuAdd 6, 65535, 6

L_00EB:
    ListMenuAdd 7, 65535, 7
    ListMenuAdd 8, 65535, 8
    ListMenuShow2
    InfoMsgClose
    VMSleep 3
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_0116
    VMJump L_0122

L_0116:
    VMCall L_0242
    VMJump L_0220

L_0122:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_0135
    VMJump L_0141

L_0135:
    VMCall L_0242
    VMJump L_0220

L_0141:
    WorkCmpConst 0x8024, 4
    VMJumpIf CMP_EQ, L_0154
    VMJump L_0160

L_0154:
    VMCall L_04A1
    VMJump L_0220

L_0160:
    WorkCmpConst 0x8024, 5
    VMJumpIf CMP_EQ, L_0173
    VMJump L_017F

L_0173:
    VMCall L_05B2
    VMJump L_0220

L_017F:
    WorkCmpConst 0x8024, 6
    VMJumpIf CMP_EQ, L_0192
    VMJump L_019E

L_0192:
    VMCall L_05C2
    VMJump L_0220

L_019E:
    WorkCmpConst 0x8024, 7
    VMJumpIf CMP_EQ, L_01B1
    VMJump L_01BD

L_01B1:
    VMCall L_062E
    VMJump L_0220

L_01BD:
    WorkCmpConst 0x8024, 8
    VMJumpIf CMP_EQ, L_01D0
    VMJump L_01DC

L_01D0:
    WorkSetConst 0x8020, 1
    VMJump L_0220

L_01DC:
    WorkCmpConst 0x8024, 65534
    VMJumpIf CMP_EQ, L_01EF
    VMJump L_01FB

L_01EF:
    WorkSetConst 0x8020, 1
    VMJump L_0220

L_01FB:
    WorkCmpConst 0x8024, 65533
    VMJumpIf CMP_EQ, L_020E
    VMJump L_0220

L_020E:
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 1
    VMJump L_0220

L_0220:
    VMJump L_005F

L_0226:
    PokecenPCClose 0x8021
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0242:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    FlagGet EVENT_FLAG_0x0960, 0x802a
    FlagGet EVENT_FLAG_0x00f9, 0x802b
    SEPlay SEQ_SE_PC_LOGIN
    // "The Pokémon Storage System\nwas accessed.[f000]븁\u0000"
    SystemMsg Global10090_Text_PokemonStorageSystemAccessed, 2
    VMStackPush 0x802a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B2
    FlagGet EVENT_FLAG_0x00f6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B2
    MEPlay SEQ_ME_ACCE
    MEWait
    // "Congratulations![f000]븁\u0000\nWallpapers were added to commemorate\nyour victory against the Champion.[f000]븁\u0000"
    SystemMsg Global10090_Text_CongratulationsWallpapersWereAdded, 2
    FlagSet EVENT_FLAG_0x00f6

L_02B2:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EE
    FlagGet EVENT_FLAG_0x00f7, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EE
    MEPlay SEQ_ME_ACCE
    MEWait
    // "Congratulations![f000]븁\u0000\nWallpapers were added to commemorate\nyour catching Kyurem.[f000]븁\u0000"
    SystemMsg Global10090_Text_CongratulationsWallpapersWereAdded_2, 2
    FlagSet EVENT_FLAG_0x00f7

L_02EE:
    WorkSetConst 0x8027, 0

L_02F4:
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0481
    // ""
    SystemMsg Global10090_Text_Empty_9, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32808
    ListMenuAdd 15, 21, 15
    ListMenuAdd 16, 22, 16
    ListMenuAdd 17, 23, 17
    ListMenuAdd 20, 26, 20
    ListMenuAdd 18, 24, 18
    ListMenuAdd 19, 25, 19
    ListMenuShow2
    InfoMsgClose
    WorkCmpConst 0x8028, 19
    VMJumpIf CMP_EQ, L_035D
    VMJump L_0365

L_035D:
    VMReturn
    VMJump L_03A7

L_0365:
    WorkCmpConst 0x8028, 65534
    VMJumpIf CMP_EQ, L_0378
    VMJump L_0380

L_0378:
    VMReturn
    VMJump L_03A7

L_0380:
    WorkCmpConst 0x8028, 65533
    VMJumpIf CMP_EQ, L_0393
    VMJump L_03A7

L_0393:
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 1
    VMReturn
    VMJump L_03A7

L_03A7:
    FadeOutBlackQ
    FadeWait
    WorkCmpConst 0x8028, 15
    VMJumpIf CMP_EQ, L_03BE
    VMJump L_03CA

L_03BE:
    CallPC 0x8029, 0
    VMJump L_0446

L_03CA:
    WorkCmpConst 0x8028, 16
    VMJumpIf CMP_EQ, L_03DD
    VMJump L_03E9

L_03DD:
    CallPC 0x8029, 1
    VMJump L_0446

L_03E9:
    WorkCmpConst 0x8028, 17
    VMJumpIf CMP_EQ, L_03FC
    VMJump L_0408

L_03FC:
    CallPC 0x8029, 2
    VMJump L_0446

L_0408:
    WorkCmpConst 0x8028, 20
    VMJumpIf CMP_EQ, L_041B
    VMJump L_0427

L_041B:
    CallPC 0x8029, 4
    VMJump L_0446

L_0427:
    WorkCmpConst 0x8028, 18
    VMJumpIf CMP_EQ, L_043A
    VMJump L_0446

L_043A:
    CallPC 0x8029, 3
    VMJump L_0446

L_0446:
    PokecenPCIdle
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0469
    SEPlay SEQ_SE_PC_LOGIN
    VMJump L_047B

L_0469:
    WorkSetConst 0x8027, 1
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 1

L_047B:
    VMJump L_02F4

L_0481:
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    VMReturn

L_04A1:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    SEPlay SEQ_SE_PC_LOGIN
    WordSetPlayerName 0
    // "Accessed [f000]Ā\u0001\u0000's PC.[f000]븁\u0000"
    SystemMsg Global10090_Text_AccessedSPc, 2
    InfoMsgClose
    WorkSetConst 0x802c, 0

L_04C8:
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_059E
    // ""
    SystemMsg Global10090_Text_Empty_9, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32813
    ListMenuAdd 28, 30, 28
    ListMenuAdd 29, 31, 29
    ListMenuShow2
    InfoMsgClose
    WorkCmpConst 0x802d, 29
    VMJumpIf CMP_EQ, L_0511
    VMJump L_0519

L_0511:
    VMReturn
    VMJump L_055B

L_0519:
    WorkCmpConst 0x802d, 65534
    VMJumpIf CMP_EQ, L_052C
    VMJump L_0534

L_052C:
    VMReturn
    VMJump L_055B

L_0534:
    WorkCmpConst 0x802d, 65533
    VMJumpIf CMP_EQ, L_0547
    VMJump L_055B

L_0547:
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 1
    VMReturn
    VMJump L_055B

L_055B:
    FadeOutBlackQ
    FadeWait
    CallMailbox 0x802e
    PokecenPCIdle
    FadeInBlackQ
    FadeWait
    VMStackPush 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0586
    SEPlay SEQ_SE_PC_LOGIN
    VMJump L_0598

L_0586:
    WorkSetConst 0x802c, 1
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 1

L_0598:
    VMJump L_04C8

L_059E:
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    VMReturn

L_05B2:
    SEPlay SEQ_SE_PC_LOGIN
    // "Accessed Professor Juniper's PC.[f000]븁\u0000"
    SystemMsg Global10090_Text_AccessedProfessorJunipersPc, 2
    RTCallGlobal 10382
    VMReturn

L_05C2:
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    SEPlay SEQ_SE_PC_LOGIN
    // "Accessed the Record System![f000]븁\u0000"
    SystemMsg Global10090_Text_AccessedRecordSystem, 2
    InfoMsgClose
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0618
    CallRecordSystem 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0612
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 1

L_0612:
    VMJump L_0620

L_0618:
    // "Your Hall of Fame data is corrupted.[f000]븁\u0000\nIt will be fixed if you enter the\nHall of Fame again.[f000]븁\u0000"
    SystemMsg Global10090_Text_HallFameDataCorrupted, 2
    InfoMsgClose

L_0620:
    WorkSetConst 0x8030, 0
    WorkSetConst 0x802f, 0
    VMReturn

L_062E:
    SEPlay SEQ_SE_PC_LOGIN
    // "Accessed the Help System.[f000]븁\u0000"
    SystemMsg Global10090_Text_AccessedHelpSystem, 2
    InfoMsgClose
    Cmd_0231 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_065D
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 1

L_065D:
    VMReturn
    .balign 4, 0
