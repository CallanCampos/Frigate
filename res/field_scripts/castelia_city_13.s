#include "asm/field_script.inc"
#include "text/script/castelia_city_13.h"

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
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8039, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803c, 0
    WorkSetConst 0x803d, 0
    WorkSetConst 0x803e, 0
    WorkSetConst 0x803f, 0
    WorkSetConst 0x8040, 0
    WorkSetConst 0x8041, 0
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8043, 0
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8045, 0
    WorkSetConst 0x8046, 0
    WorkSetConst 0x8047, 0
    WorkSetConst 0x8048, 0
    WorkSetConst 0x8049, 0
    WorkSetConst 0x804a, 0
    WorkSetConst 0x804b, 0
    WorkSetConst 0x804c, 0
    WorkSetConst 0x804d, 0
    WorkSetConst 0x804e, 0
    WorkSetConst 0x804f, 0
    WorkSetConst 0x8050, 0
    WorkSetConst 0x8051, 0
    WorkSetConst 0x8052, 0
    WorkSetConst 0x8053, 0

L_017A:
    ListMenu_AnchorTopRight 31, 10, 0x8008, 1, 32784
    ListMenuAdd 4, 65535, 0
    ListMenuAdd 5, 65535, 1
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AE
    WorkSetConst 0x8010, 1

L_01AE:
    VMReturn

L_01B0:
    WorkSetConst EVENT_WORK_0x408e, 1
    VMReturn

L_01B8:
    WorkSetConst EVENT_WORK_0x408e, 0
    VMReturn

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    FlagGet EVENT_FLAG_0x00cc, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FF
    VMCall L_4298
    FlagSet EVENT_FLAG_0x00cc
    TrainerGameInfoCmd_020C
    VMCall L_01B0
    VMCall L_0236
    VMJump L_0230

L_01FF:
    VMCall L_42C6
    VMCall L_2EA3
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_022A
    VMCall L_42D7
    VMJump L_0230

L_022A:
    VMCall L_0236

L_0230:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0236:
    WorkSetConst 0x8021, 1

L_023C:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030C
    VMCall L_2F28
    WorkCmpConst 0x8010, 12
    VMJumpIf CMP_EQ, L_0268
    VMJump L_0276

L_0268:
    VMCall L_030E
    VMReturn
    VMJump L_0306

L_0276:
    WorkCmpConst 0x8010, 13
    VMJumpIf CMP_EQ, L_0289
    VMJump L_0297

L_0289:
    VMCall L_0362
    VMReturn
    VMJump L_0306

L_0297:
    WorkCmpConst 0x8010, 15
    VMJumpIf CMP_EQ, L_02AA
    VMJump L_02C2

L_02AA:
    VMCall L_4BE7
    // "Then, when you finish the survey,\nplease come and report it to me.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ThenWhenFinishSurvey_2, 0x8011, 2, 0
    VMJump L_0306

L_02C2:
    WorkCmpConst 0x8010, 14
    VMJumpIf CMP_EQ, L_02D5
    VMJump L_02E3

L_02D5:
    VMCall L_4315
    VMReturn
    VMJump L_0306

L_02E3:
    WorkCmpConst 0x8010, 65534
    VMJumpIf CMP_EQ, L_02F6
    VMJump L_0304

L_02F6:
    VMCall L_4315
    VMReturn
    VMJump L_0306

L_0304:
    VMReturn

L_0306:
    VMJump L_023C

L_030C:
    VMReturn

L_030E:
    VMCall L_2EFD
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035A
    VMCall L_42E9
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034C
    VMCall L_4425
    VMJump L_0354

L_034C:
    VMCall L_4401
    TrainerGameInfoCmd_01FE

L_0354:
    VMJump L_0360

L_035A:
    VMCall L_03ED

L_0360:
    VMReturn

L_0362:
    VMCall L_2EFD
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0387
    VMCall L_4303
    VMJump L_03EB

L_0387:
    SurveyGetCurrentQuestionID 0x8022
    TrainerCardCmd_0202 0x8022, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B8
    VMCall L_4437
    VMCall L_058C
    VMReturn
    VMJump L_03EB

L_03B8:
    WorkGet 0x8008, 0x8023
    VMCall L_57CE
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E3
    VMCall L_4425
    VMJump L_03EB

L_03E3:
    VMCall L_4401
    TrainerGameInfoCmd_01FE

L_03EB:
    VMReturn

L_03ED:
    VMCall L_4327
    WorkGet 0x8025, 0x8010
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_040C
    VMJump L_0418

L_040C:
    VMCall L_4494
    VMJump L_0437

L_0418:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_042B
    VMJump L_0437

L_042B:
    VMCall L_44A2
    VMJump L_0437

L_0437:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0584
    WorkSetConst 0x8026, 1

L_0460:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0584
    // "Please choose a request on which\nyou are going to conduct a survey."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_PleaseChooseRequestWhich, 0x8011, 2, 0
    WorkGet 0x8008, 0x8025
    VMCall L_433B
    WorkGet 0x8024, 0x8010
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush 0x8024
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0578
    WorkGet 0x8008, 0x8024
    VMCall L_463F
    TrainerCardCmd_0202 0x8024, 0x8027
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0518
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F8
    VMCall L_44B0
    VMJump L_04FE

L_04F8:
    VMCall L_44C3

L_04FE:
    WorkGet 0x8008, 0x8024
    VMCall L_0679
    VMCall L_058C
    VMReturn
    VMJump L_0572

L_0518:
    VMCall L_43BB
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0553
    ActorMsgClose
    WorkGet 0x8008, 0x8024
    VMCall L_0679
    VMCall L_3C42
    VMCall L_4413
    VMReturn
    VMJump L_0572

L_0553:
    VMCall L_43D5
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0572
    WorkSetConst 0x8026, 0

L_0572:
    VMJump L_057E

L_0578:
    WorkSetConst 0x8026, 0

L_057E:
    VMJump L_0460

L_0584:
    VMCall L_43EF
    VMReturn

L_058C:
    TrainerGameInfoCmd_020B 0x8028
    VMCall L_369C
    VMCall L_518D
    VMCall L_4447
    VMCall L_0629
    VMCall L_068B
    VMCall L_2EA3
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05DF
    VMCall L_445A
    VMCall L_0657
    VMCall L_446D
    VMJump L_0627

L_05DF:
    TrainerGameInfoCmd_020B 0x8029
    VMStackPush 0x8028
    VMStackPush 0x8029
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0608
    VMCall L_44D6
    VMCall L_41E8
    VMJump L_0621

L_0608:
    VMStackPush 0x8029
    VMStackPushConst 5
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0621
    VMCall L_4585

L_0621:
    VMCall L_447F

L_0627:
    VMReturn

L_0629:
    VMCall L_2905
    WorkGet 0x8040, 0x8010
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8040
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMReturn

L_0657:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 50
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMReturn

L_0679:
    VMCall L_1E49
    TrainerGameInfoCmd_01FD 0x8024, 0x8009, 0x800a, 0x800b
    VMReturn

L_068B:
    SurveyGetCurrentQuestionID 0x8040
    WorkGet 0x8008, 0x8040
    VMCall L_06C0
    TrainerGameInfoCmd_01FE
    VMCall L_0C58
    WorkGet 0x8032, 0x8010
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06BE
    TrainerGameInfoCmd_020C

L_06BE:
    VMReturn

L_06C0:
    WorkCmpConst 0x8008, 24
    VMJumpIf CMP_EQ, L_06D3
    VMJump L_06DD

L_06D3:
    FlagSet EVENT_FLAG_0x0091
    VMJump L_0BF8

L_06DD:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_06F0
    VMJump L_06FA

L_06F0:
    FlagSet EVENT_FLAG_0x0092
    VMJump L_0BF8

L_06FA:
    WorkCmpConst 0x8008, 25
    VMJumpIf CMP_EQ, L_070D
    VMJump L_0717

L_070D:
    FlagSet EVENT_FLAG_0x0093
    VMJump L_0BF8

L_0717:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_072A
    VMJump L_0734

L_072A:
    FlagSet EVENT_FLAG_0x0094
    VMJump L_0BF8

L_0734:
    WorkCmpConst 0x8008, 26
    VMJumpIf CMP_EQ, L_0747
    VMJump L_0751

L_0747:
    FlagSet EVENT_FLAG_0x0095
    VMJump L_0BF8

L_0751:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_0764
    VMJump L_076E

L_0764:
    FlagSet EVENT_FLAG_0x0096
    VMJump L_0BF8

L_076E:
    WorkCmpConst 0x8008, 27
    VMJumpIf CMP_EQ, L_0781
    VMJump L_078B

L_0781:
    FlagSet EVENT_FLAG_0x0097
    VMJump L_0BF8

L_078B:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_079E
    VMJump L_07A8

L_079E:
    FlagSet EVENT_FLAG_0x0098
    VMJump L_0BF8

L_07A8:
    WorkCmpConst 0x8008, 28
    VMJumpIf CMP_EQ, L_07BB
    VMJump L_07C5

L_07BB:
    FlagSet EVENT_FLAG_0x0099
    VMJump L_0BF8

L_07C5:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_07D8
    VMJump L_07E2

L_07D8:
    FlagSet EVENT_FLAG_0x009a
    VMJump L_0BF8

L_07E2:
    WorkCmpConst 0x8008, 29
    VMJumpIf CMP_EQ, L_07F5
    VMJump L_07FF

L_07F5:
    FlagSet EVENT_FLAG_0x009b
    VMJump L_0BF8

L_07FF:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_0812
    VMJump L_081C

L_0812:
    FlagSet EVENT_FLAG_0x009c
    VMJump L_0BF8

L_081C:
    WorkCmpConst 0x8008, 30
    VMJumpIf CMP_EQ, L_082F
    VMJump L_0839

L_082F:
    FlagSet EVENT_FLAG_0x009d
    VMJump L_0BF8

L_0839:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_084C
    VMJump L_0856

L_084C:
    FlagSet EVENT_FLAG_0x009e
    VMJump L_0BF8

L_0856:
    WorkCmpConst 0x8008, 31
    VMJumpIf CMP_EQ, L_0869
    VMJump L_0873

L_0869:
    FlagSet EVENT_FLAG_0x009f
    VMJump L_0BF8

L_0873:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_0886
    VMJump L_0890

L_0886:
    FlagSet EVENT_FLAG_0x00a0
    VMJump L_0BF8

L_0890:
    WorkCmpConst 0x8008, 32
    VMJumpIf CMP_EQ, L_08A3
    VMJump L_08AD

L_08A3:
    FlagSet EVENT_FLAG_0x00a1
    VMJump L_0BF8

L_08AD:
    WorkCmpConst 0x8008, 9
    VMJumpIf CMP_EQ, L_08C0
    VMJump L_08CA

L_08C0:
    FlagSet EVENT_FLAG_0x00a2
    VMJump L_0BF8

L_08CA:
    WorkCmpConst 0x8008, 33
    VMJumpIf CMP_EQ, L_08DD
    VMJump L_08E7

L_08DD:
    FlagSet EVENT_FLAG_0x00a3
    VMJump L_0BF8

L_08E7:
    WorkCmpConst 0x8008, 10
    VMJumpIf CMP_EQ, L_08FA
    VMJump L_0904

L_08FA:
    FlagSet EVENT_FLAG_0x00a4
    VMJump L_0BF8

L_0904:
    WorkCmpConst 0x8008, 34
    VMJumpIf CMP_EQ, L_0917
    VMJump L_0921

L_0917:
    FlagSet EVENT_FLAG_0x00a5
    VMJump L_0BF8

L_0921:
    WorkCmpConst 0x8008, 11
    VMJumpIf CMP_EQ, L_0934
    VMJump L_093E

L_0934:
    FlagSet EVENT_FLAG_0x00a6
    VMJump L_0BF8

L_093E:
    WorkCmpConst 0x8008, 35
    VMJumpIf CMP_EQ, L_0951
    VMJump L_095B

L_0951:
    FlagSet EVENT_FLAG_0x00a7
    VMJump L_0BF8

L_095B:
    WorkCmpConst 0x8008, 12
    VMJumpIf CMP_EQ, L_096E
    VMJump L_0978

L_096E:
    FlagSet EVENT_FLAG_0x00a8
    VMJump L_0BF8

L_0978:
    WorkCmpConst 0x8008, 36
    VMJumpIf CMP_EQ, L_098B
    VMJump L_0995

L_098B:
    FlagSet EVENT_FLAG_0x00a9
    VMJump L_0BF8

L_0995:
    WorkCmpConst 0x8008, 13
    VMJumpIf CMP_EQ, L_09A8
    VMJump L_09B2

L_09A8:
    FlagSet EVENT_FLAG_0x00aa
    VMJump L_0BF8

L_09B2:
    WorkCmpConst 0x8008, 37
    VMJumpIf CMP_EQ, L_09C5
    VMJump L_09CF

L_09C5:
    FlagSet EVENT_FLAG_0x00ab
    VMJump L_0BF8

L_09CF:
    WorkCmpConst 0x8008, 14
    VMJumpIf CMP_EQ, L_09E2
    VMJump L_09EC

L_09E2:
    FlagSet EVENT_FLAG_0x00ac
    VMJump L_0BF8

L_09EC:
    WorkCmpConst 0x8008, 38
    VMJumpIf CMP_EQ, L_09FF
    VMJump L_0A09

L_09FF:
    FlagSet EVENT_FLAG_0x00ad
    VMJump L_0BF8

L_0A09:
    WorkCmpConst 0x8008, 15
    VMJumpIf CMP_EQ, L_0A1C
    VMJump L_0A26

L_0A1C:
    FlagSet EVENT_FLAG_0x00ae
    VMJump L_0BF8

L_0A26:
    WorkCmpConst 0x8008, 39
    VMJumpIf CMP_EQ, L_0A39
    VMJump L_0A43

L_0A39:
    FlagSet EVENT_FLAG_0x00af
    VMJump L_0BF8

L_0A43:
    WorkCmpConst 0x8008, 16
    VMJumpIf CMP_EQ, L_0A56
    VMJump L_0A60

L_0A56:
    FlagSet EVENT_FLAG_0x00b0
    VMJump L_0BF8

L_0A60:
    WorkCmpConst 0x8008, 40
    VMJumpIf CMP_EQ, L_0A73
    VMJump L_0A7D

L_0A73:
    FlagSet EVENT_FLAG_0x00b1
    VMJump L_0BF8

L_0A7D:
    WorkCmpConst 0x8008, 17
    VMJumpIf CMP_EQ, L_0A90
    VMJump L_0A9A

L_0A90:
    FlagSet EVENT_FLAG_0x00b2
    VMJump L_0BF8

L_0A9A:
    WorkCmpConst 0x8008, 41
    VMJumpIf CMP_EQ, L_0AAD
    VMJump L_0AB7

L_0AAD:
    FlagSet EVENT_FLAG_0x00b3
    VMJump L_0BF8

L_0AB7:
    WorkCmpConst 0x8008, 18
    VMJumpIf CMP_EQ, L_0ACA
    VMJump L_0AD4

L_0ACA:
    FlagSet EVENT_FLAG_0x00b4
    VMJump L_0BF8

L_0AD4:
    WorkCmpConst 0x8008, 42
    VMJumpIf CMP_EQ, L_0AE7
    VMJump L_0AF1

L_0AE7:
    FlagSet EVENT_FLAG_0x00b5
    VMJump L_0BF8

L_0AF1:
    WorkCmpConst 0x8008, 19
    VMJumpIf CMP_EQ, L_0B04
    VMJump L_0B0E

L_0B04:
    FlagSet EVENT_FLAG_0x00b6
    VMJump L_0BF8

L_0B0E:
    WorkCmpConst 0x8008, 43
    VMJumpIf CMP_EQ, L_0B21
    VMJump L_0B2B

L_0B21:
    FlagSet EVENT_FLAG_0x00b7
    VMJump L_0BF8

L_0B2B:
    WorkCmpConst 0x8008, 20
    VMJumpIf CMP_EQ, L_0B3E
    VMJump L_0B48

L_0B3E:
    FlagSet EVENT_FLAG_0x00b8
    VMJump L_0BF8

L_0B48:
    WorkCmpConst 0x8008, 44
    VMJumpIf CMP_EQ, L_0B5B
    VMJump L_0B65

L_0B5B:
    FlagSet EVENT_FLAG_0x00b9
    VMJump L_0BF8

L_0B65:
    WorkCmpConst 0x8008, 21
    VMJumpIf CMP_EQ, L_0B78
    VMJump L_0B82

L_0B78:
    FlagSet EVENT_FLAG_0x00ba
    VMJump L_0BF8

L_0B82:
    WorkCmpConst 0x8008, 45
    VMJumpIf CMP_EQ, L_0B95
    VMJump L_0B9F

L_0B95:
    FlagSet EVENT_FLAG_0x00bb
    VMJump L_0BF8

L_0B9F:
    WorkCmpConst 0x8008, 22
    VMJumpIf CMP_EQ, L_0BB2
    VMJump L_0BBC

L_0BB2:
    FlagSet EVENT_FLAG_0x00bc
    VMJump L_0BF8

L_0BBC:
    WorkCmpConst 0x8008, 46
    VMJumpIf CMP_EQ, L_0BCF
    VMJump L_0BD9

L_0BCF:
    FlagSet EVENT_FLAG_0x00bd
    VMJump L_0BF8

L_0BD9:
    WorkCmpConst 0x8008, 23
    VMJumpIf CMP_EQ, L_0BEC
    VMJump L_0BF6

L_0BEC:
    FlagSet EVENT_FLAG_0x00be
    VMJump L_0BF8

L_0BF6:
    VMReturn

L_0BF8:
    VMReturn

L_0BFA:
    WorkSetConst 0x804d, 145
    WorkSetConst 0x804e, 190
    WorkGet 0x804f, 0x804d
    WorkSetConst 0x8050, 0

L_0C12:
    VMStackPush 0x804f
    VMStackPush 0x804e
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0C50
    FlagGet 0x804f, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C44
    WorkAdd 0x8050, 1

L_0C44:
    WorkAdd 0x804f, 1
    VMJump L_0C12

L_0C50:
    WorkGet 0x8010, 0x8050
    VMReturn

L_0C58:
    VMCall L_0BFA
    WorkGet 0x8041, 0x8010
    TrainerGameInfoCmd_020B 0x8040
    WorkCmpConst 0x8040, 1
    VMJumpIf CMP_EQ, L_0C7B
    VMJump L_0C87

L_0C7B:
    WorkSetConst 0x8042, 4
    VMJump L_0D03

L_0C87:
    WorkCmpConst 0x8040, 2
    VMJumpIf CMP_EQ, L_0C9A
    VMJump L_0CA6

L_0C9A:
    WorkSetConst 0x8042, 10
    VMJump L_0D03

L_0CA6:
    WorkCmpConst 0x8040, 3
    VMJumpIf CMP_EQ, L_0CB9
    VMJump L_0CC5

L_0CB9:
    WorkSetConst 0x8042, 16
    VMJump L_0D03

L_0CC5:
    WorkCmpConst 0x8040, 4
    VMJumpIf CMP_EQ, L_0CD8
    VMJump L_0CE4

L_0CD8:
    WorkSetConst 0x8042, 21
    VMJump L_0D03

L_0CE4:
    WorkCmpConst 0x8040, 5
    VMJumpIf CMP_EQ, L_0CF7
    VMJump L_0D03

L_0CF7:
    WorkSetConst 0x8042, 47
    VMJump L_0D03

L_0D03:
    WorkGet 0x8010, 0x8042
    WorkSub 0x8010, 0x8041
    VMReturn
    SurveyGetCurrentQuestionID 0x804d
    WorkCmpConst 0x804d, 24
    VMJumpIf CMP_EQ, L_0D28
    VMJump L_0D34

L_0D28:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0D34:
    WorkCmpConst 0x804d, 1
    VMJumpIf CMP_EQ, L_0D47
    VMJump L_0D53

L_0D47:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0D53:
    WorkCmpConst 0x804d, 25
    VMJumpIf CMP_EQ, L_0D66
    VMJump L_0D72

L_0D66:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0D72:
    WorkCmpConst 0x804d, 2
    VMJumpIf CMP_EQ, L_0D85
    VMJump L_0D91

L_0D85:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0D91:
    WorkCmpConst 0x804d, 26
    VMJumpIf CMP_EQ, L_0DA4
    VMJump L_0DB0

L_0DA4:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0DB0:
    WorkCmpConst 0x804d, 3
    VMJumpIf CMP_EQ, L_0DC3
    VMJump L_0DCF

L_0DC3:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0DCF:
    WorkCmpConst 0x804d, 27
    VMJumpIf CMP_EQ, L_0DE2
    VMJump L_0DEE

L_0DE2:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0DEE:
    WorkCmpConst 0x804d, 4
    VMJumpIf CMP_EQ, L_0E01
    VMJump L_0E0D

L_0E01:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0E0D:
    WorkCmpConst 0x804d, 28
    VMJumpIf CMP_EQ, L_0E20
    VMJump L_0E2C

L_0E20:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0E2C:
    WorkCmpConst 0x804d, 5
    VMJumpIf CMP_EQ, L_0E3F
    VMJump L_0E4B

L_0E3F:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0E4B:
    WorkCmpConst 0x804d, 29
    VMJumpIf CMP_EQ, L_0E5E
    VMJump L_0E6A

L_0E5E:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0E6A:
    WorkCmpConst 0x804d, 6
    VMJumpIf CMP_EQ, L_0E7D
    VMJump L_0E89

L_0E7D:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0E89:
    WorkCmpConst 0x804d, 30
    VMJumpIf CMP_EQ, L_0E9C
    VMJump L_0EA8

L_0E9C:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0EA8:
    WorkCmpConst 0x804d, 7
    VMJumpIf CMP_EQ, L_0EBB
    VMJump L_0EC7

L_0EBB:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0EC7:
    WorkCmpConst 0x804d, 31
    VMJumpIf CMP_EQ, L_0EDA
    VMJump L_0EE6

L_0EDA:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0EE6:
    WorkCmpConst 0x804d, 8
    VMJumpIf CMP_EQ, L_0EF9
    VMJump L_0F05

L_0EF9:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0F05:
    WorkCmpConst 0x804d, 32
    VMJumpIf CMP_EQ, L_0F18
    VMJump L_0F24

L_0F18:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0F24:
    WorkCmpConst 0x804d, 9
    VMJumpIf CMP_EQ, L_0F37
    VMJump L_0F43

L_0F37:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0F43:
    WorkCmpConst 0x804d, 33
    VMJumpIf CMP_EQ, L_0F56
    VMJump L_0F62

L_0F56:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0F62:
    WorkCmpConst 0x804d, 10
    VMJumpIf CMP_EQ, L_0F75
    VMJump L_0F81

L_0F75:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_0F81:
    WorkCmpConst 0x804d, 34
    VMJumpIf CMP_EQ, L_0F94
    VMJump L_0FA0

L_0F94:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_0FA0:
    WorkCmpConst 0x804d, 11
    VMJumpIf CMP_EQ, L_0FB3
    VMJump L_0FBF

L_0FB3:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_0FBF:
    WorkCmpConst 0x804d, 35
    VMJumpIf CMP_EQ, L_0FD2
    VMJump L_0FDE

L_0FD2:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_0FDE:
    WorkCmpConst 0x804d, 12
    VMJumpIf CMP_EQ, L_0FF1
    VMJump L_0FFD

L_0FF1:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_0FFD:
    WorkCmpConst 0x804d, 36
    VMJumpIf CMP_EQ, L_1010
    VMJump L_101C

L_1010:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_101C:
    WorkCmpConst 0x804d, 13
    VMJumpIf CMP_EQ, L_102F
    VMJump L_103B

L_102F:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_103B:
    WorkCmpConst 0x804d, 37
    VMJumpIf CMP_EQ, L_104E
    VMJump L_105A

L_104E:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_105A:
    WorkCmpConst 0x804d, 14
    VMJumpIf CMP_EQ, L_106D
    VMJump L_1079

L_106D:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_1079:
    WorkCmpConst 0x804d, 38
    VMJumpIf CMP_EQ, L_108C
    VMJump L_1098

L_108C:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_1098:
    WorkCmpConst 0x804d, 15
    VMJumpIf CMP_EQ, L_10AB
    VMJump L_10B7

L_10AB:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_10B7:
    WorkCmpConst 0x804d, 39
    VMJumpIf CMP_EQ, L_10CA
    VMJump L_10D6

L_10CA:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_10D6:
    WorkCmpConst 0x804d, 16
    VMJumpIf CMP_EQ, L_10E9
    VMJump L_10F5

L_10E9:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_10F5:
    WorkCmpConst 0x804d, 40
    VMJumpIf CMP_EQ, L_1108
    VMJump L_1114

L_1108:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_1114:
    WorkCmpConst 0x804d, 17
    VMJumpIf CMP_EQ, L_1127
    VMJump L_1133

L_1127:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_1133:
    WorkCmpConst 0x804d, 41
    VMJumpIf CMP_EQ, L_1146
    VMJump L_1152

L_1146:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_1152:
    WorkCmpConst 0x804d, 18
    VMJumpIf CMP_EQ, L_1165
    VMJump L_1171

L_1165:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_1171:
    WorkCmpConst 0x804d, 42
    VMJumpIf CMP_EQ, L_1184
    VMJump L_1190

L_1184:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_1190:
    WorkCmpConst 0x804d, 19
    VMJumpIf CMP_EQ, L_11A3
    VMJump L_11AF

L_11A3:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_11AF:
    WorkCmpConst 0x804d, 43
    VMJumpIf CMP_EQ, L_11C2
    VMJump L_11CE

L_11C2:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_11CE:
    WorkCmpConst 0x804d, 20
    VMJumpIf CMP_EQ, L_11E1
    VMJump L_11ED

L_11E1:
    WorkSetConst 0x8010, 2
    VMJump L_12AD

L_11ED:
    WorkCmpConst 0x804d, 44
    VMJumpIf CMP_EQ, L_1200
    VMJump L_120C

L_1200:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_120C:
    WorkCmpConst 0x804d, 21
    VMJumpIf CMP_EQ, L_121F
    VMJump L_122B

L_121F:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_122B:
    WorkCmpConst 0x804d, 45
    VMJumpIf CMP_EQ, L_123E
    VMJump L_124A

L_123E:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_124A:
    WorkCmpConst 0x804d, 22
    VMJumpIf CMP_EQ, L_125D
    VMJump L_1269

L_125D:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_1269:
    WorkCmpConst 0x804d, 46
    VMJumpIf CMP_EQ, L_127C
    VMJump L_1288

L_127C:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_1288:
    WorkCmpConst 0x804d, 23
    VMJumpIf CMP_EQ, L_129B
    VMJump L_12A7

L_129B:
    WorkSetConst 0x8010, 1
    VMJump L_12AD

L_12A7:
    WorkSetConst 0x8010, 0

L_12AD:
    VMReturn
    WorkSetConst 0x8009, 0
    WorkSetConst 0x800a, 0
    WorkSetConst 0x800b, 0
    WorkCmpConst 0x8008, 26
    VMJumpIf CMP_EQ, L_12D4
    VMJump L_12E0

L_12D4:
    WorkSetConst 0x8009, 10
    VMJump L_18A1

L_12E0:
    WorkCmpConst 0x8008, 24
    VMJumpIf CMP_EQ, L_12F3
    VMJump L_12FF

L_12F3:
    WorkSetConst 0x8009, 5
    VMJump L_18A1

L_12FF:
    WorkCmpConst 0x8008, 25
    VMJumpIf CMP_EQ, L_1312
    VMJump L_131E

L_1312:
    WorkSetConst 0x8009, 5
    VMJump L_18A1

L_131E:
    WorkCmpConst 0x8008, 27
    VMJumpIf CMP_EQ, L_1331
    VMJump L_133D

L_1331:
    WorkSetConst 0x8009, 10
    VMJump L_18A1

L_133D:
    WorkCmpConst 0x8008, 28
    VMJumpIf CMP_EQ, L_1350
    VMJump L_135C

L_1350:
    WorkSetConst 0x8009, 10
    VMJump L_18A1

L_135C:
    WorkCmpConst 0x8008, 29
    VMJumpIf CMP_EQ, L_136F
    VMJump L_137B

L_136F:
    WorkSetConst 0x8009, 10
    VMJump L_18A1

L_137B:
    WorkCmpConst 0x8008, 30
    VMJumpIf CMP_EQ, L_138E
    VMJump L_139A

L_138E:
    WorkSetConst 0x8009, 20
    VMJump L_18A1

L_139A:
    WorkCmpConst 0x8008, 31
    VMJumpIf CMP_EQ, L_13AD
    VMJump L_13B9

L_13AD:
    WorkSetConst 0x8009, 20
    VMJump L_18A1

L_13B9:
    WorkCmpConst 0x8008, 32
    VMJumpIf CMP_EQ, L_13CC
    VMJump L_13D8

L_13CC:
    WorkSetConst 0x8009, 20
    VMJump L_18A1

L_13D8:
    WorkCmpConst 0x8008, 33
    VMJumpIf CMP_EQ, L_13EB
    VMJump L_13F7

L_13EB:
    WorkSetConst 0x8009, 20
    VMJump L_18A1

L_13F7:
    WorkCmpConst 0x8008, 36
    VMJumpIf CMP_EQ, L_140A
    VMJump L_1416

L_140A:
    WorkSetConst 0x8009, 30
    VMJump L_18A1

L_1416:
    WorkCmpConst 0x8008, 38
    VMJumpIf CMP_EQ, L_1429
    VMJump L_1435

L_1429:
    WorkSetConst 0x8009, 40
    VMJump L_18A1

L_1435:
    WorkCmpConst 0x8008, 40
    VMJumpIf CMP_EQ, L_1448
    VMJump L_1454

L_1448:
    WorkSetConst 0x8009, 40
    VMJump L_18A1

L_1454:
    WorkCmpConst 0x8008, 42
    VMJumpIf CMP_EQ, L_1467
    VMJump L_1473

L_1467:
    WorkSetConst 0x8009, 50
    VMJump L_18A1

L_1473:
    WorkCmpConst 0x8008, 43
    VMJumpIf CMP_EQ, L_1486
    VMJump L_1492

L_1486:
    WorkSetConst 0x8009, 50
    VMJump L_18A1

L_1492:
    WorkCmpConst 0x8008, 44
    VMJumpIf CMP_EQ, L_14A5
    VMJump L_14B1

L_14A5:
    WorkSetConst 0x8009, 50
    VMJump L_18A1

L_14B1:
    WorkCmpConst 0x8008, 45
    VMJumpIf CMP_EQ, L_14C4
    VMJump L_14D0

L_14C4:
    WorkSetConst 0x8009, 50
    VMJump L_18A1

L_14D0:
    WorkCmpConst 0x8008, 46
    VMJumpIf CMP_EQ, L_14E3
    VMJump L_14EF

L_14E3:
    WorkSetConst 0x8009, 100
    VMJump L_18A1

L_14EF:
    WorkCmpConst 0x8008, 34
    VMJumpIf CMP_EQ, L_1502
    VMJump L_1514

L_1502:
    WorkSetConst 0x8009, 30
    WorkSetConst 0x800a, 30
    VMJump L_18A1

L_1514:
    WorkCmpConst 0x8008, 35
    VMJumpIf CMP_EQ, L_1527
    VMJump L_1539

L_1527:
    WorkSetConst 0x8009, 30
    WorkSetConst 0x800a, 30
    VMJump L_18A1

L_1539:
    WorkCmpConst 0x8008, 37
    VMJumpIf CMP_EQ, L_154C
    VMJump L_155E

L_154C:
    WorkSetConst 0x8009, 30
    WorkSetConst 0x800a, 30
    VMJump L_18A1

L_155E:
    WorkCmpConst 0x8008, 39
    VMJumpIf CMP_EQ, L_1571
    VMJump L_1583

L_1571:
    WorkSetConst 0x8009, 40
    WorkSetConst 0x800a, 40
    VMJump L_18A1

L_1583:
    WorkCmpConst 0x8008, 41
    VMJumpIf CMP_EQ, L_1596
    VMJump L_15A8

L_1596:
    WorkSetConst 0x8009, 40
    WorkSetConst 0x800a, 40
    VMJump L_18A1

L_15A8:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_15BB
    VMJump L_15C7

L_15BB:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_15C7:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_15DA
    VMJump L_15E6

L_15DA:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_15E6:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_15F9
    VMJump L_1605

L_15F9:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_1605:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_1618
    VMJump L_1624

L_1618:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_1624:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_1637
    VMJump L_1643

L_1637:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_1643:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_1656
    VMJump L_1662

L_1656:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_1662:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_1675
    VMJump L_1681

L_1675:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_1681:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_1694
    VMJump L_16A0

L_1694:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_16A0:
    WorkCmpConst 0x8008, 9
    VMJumpIf CMP_EQ, L_16B3
    VMJump L_16BF

L_16B3:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_16BF:
    WorkCmpConst 0x8008, 10
    VMJumpIf CMP_EQ, L_16D2
    VMJump L_16DE

L_16D2:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_16DE:
    WorkCmpConst 0x8008, 19
    VMJumpIf CMP_EQ, L_16F1
    VMJump L_16FD

L_16F1:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_16FD:
    WorkCmpConst 0x8008, 20
    VMJumpIf CMP_EQ, L_1710
    VMJump L_171C

L_1710:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_171C:
    WorkCmpConst 0x8008, 21
    VMJumpIf CMP_EQ, L_172F
    VMJump L_173B

L_172F:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_173B:
    WorkCmpConst 0x8008, 22
    VMJumpIf CMP_EQ, L_174E
    VMJump L_175A

L_174E:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_175A:
    WorkCmpConst 0x8008, 23
    VMJumpIf CMP_EQ, L_176D
    VMJump L_1779

L_176D:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_1779:
    WorkCmpConst 0x8008, 11
    VMJumpIf CMP_EQ, L_178C
    VMJump L_179E

L_178C:
    WorkSetConst 0x8009, 1
    WorkSetConst 0x800a, 1
    VMJump L_18A1

L_179E:
    WorkCmpConst 0x8008, 12
    VMJumpIf CMP_EQ, L_17B1
    VMJump L_17C3

L_17B1:
    WorkSetConst 0x8009, 1
    WorkSetConst 0x800a, 1
    VMJump L_18A1

L_17C3:
    WorkCmpConst 0x8008, 13
    VMJumpIf CMP_EQ, L_17D6
    VMJump L_17E2

L_17D6:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_17E2:
    WorkCmpConst 0x8008, 14
    VMJumpIf CMP_EQ, L_17F5
    VMJump L_1807

L_17F5:
    WorkSetConst 0x8009, 1
    WorkSetConst 0x800a, 1
    VMJump L_18A1

L_1807:
    WorkCmpConst 0x8008, 15
    VMJumpIf CMP_EQ, L_181A
    VMJump L_1826

L_181A:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_1826:
    WorkCmpConst 0x8008, 16
    VMJumpIf CMP_EQ, L_1839
    VMJump L_184B

L_1839:
    WorkSetConst 0x8009, 1
    WorkSetConst 0x800a, 1
    VMJump L_18A1

L_184B:
    WorkCmpConst 0x8008, 17
    VMJumpIf CMP_EQ, L_185E
    VMJump L_186A

L_185E:
    WorkSetConst 0x8009, 1
    VMJump L_18A1

L_186A:
    WorkCmpConst 0x8008, 18
    VMJumpIf CMP_EQ, L_187D
    VMJump L_188F

L_187D:
    WorkSetConst 0x8009, 1
    WorkSetConst 0x800a, 1
    VMJump L_18A1

L_188F:
    WorkSetConst 0x8009, 0
    WorkSetConst 0x800a, 0
    WorkSetConst 0x800b, 0

L_18A1:
    VMReturn
    WorkCmpConst 0x8008, 24
    VMJumpIf CMP_EQ, L_18B6
    VMJump L_18C2

L_18B6:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_18C2:
    WorkCmpConst 0x8008, 25
    VMJumpIf CMP_EQ, L_18D5
    VMJump L_18E1

L_18D5:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_18E1:
    WorkCmpConst 0x8008, 26
    VMJumpIf CMP_EQ, L_18F4
    VMJump L_1900

L_18F4:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1900:
    WorkCmpConst 0x8008, 27
    VMJumpIf CMP_EQ, L_1913
    VMJump L_191F

L_1913:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_191F:
    WorkCmpConst 0x8008, 28
    VMJumpIf CMP_EQ, L_1932
    VMJump L_193E

L_1932:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_193E:
    WorkCmpConst 0x8008, 29
    VMJumpIf CMP_EQ, L_1951
    VMJump L_195D

L_1951:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_195D:
    WorkCmpConst 0x8008, 30
    VMJumpIf CMP_EQ, L_1970
    VMJump L_197C

L_1970:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_197C:
    WorkCmpConst 0x8008, 31
    VMJumpIf CMP_EQ, L_198F
    VMJump L_199B

L_198F:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_199B:
    WorkCmpConst 0x8008, 32
    VMJumpIf CMP_EQ, L_19AE
    VMJump L_19BA

L_19AE:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_19BA:
    WorkCmpConst 0x8008, 33
    VMJumpIf CMP_EQ, L_19CD
    VMJump L_19D9

L_19CD:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_19D9:
    WorkCmpConst 0x8008, 34
    VMJumpIf CMP_EQ, L_19EC
    VMJump L_19F8

L_19EC:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_19F8:
    WorkCmpConst 0x8008, 35
    VMJumpIf CMP_EQ, L_1A0B
    VMJump L_1A17

L_1A0B:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1A17:
    WorkCmpConst 0x8008, 36
    VMJumpIf CMP_EQ, L_1A2A
    VMJump L_1A36

L_1A2A:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1A36:
    WorkCmpConst 0x8008, 37
    VMJumpIf CMP_EQ, L_1A49
    VMJump L_1A55

L_1A49:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1A55:
    WorkCmpConst 0x8008, 38
    VMJumpIf CMP_EQ, L_1A68
    VMJump L_1A74

L_1A68:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1A74:
    WorkCmpConst 0x8008, 39
    VMJumpIf CMP_EQ, L_1A87
    VMJump L_1A93

L_1A87:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1A93:
    WorkCmpConst 0x8008, 40
    VMJumpIf CMP_EQ, L_1AA6
    VMJump L_1AB2

L_1AA6:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1AB2:
    WorkCmpConst 0x8008, 41
    VMJumpIf CMP_EQ, L_1AC5
    VMJump L_1AD1

L_1AC5:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1AD1:
    WorkCmpConst 0x8008, 42
    VMJumpIf CMP_EQ, L_1AE4
    VMJump L_1AF0

L_1AE4:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1AF0:
    WorkCmpConst 0x8008, 43
    VMJumpIf CMP_EQ, L_1B03
    VMJump L_1B0F

L_1B03:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1B0F:
    WorkCmpConst 0x8008, 44
    VMJumpIf CMP_EQ, L_1B22
    VMJump L_1B2E

L_1B22:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1B2E:
    WorkCmpConst 0x8008, 45
    VMJumpIf CMP_EQ, L_1B41
    VMJump L_1B4D

L_1B41:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1B4D:
    WorkCmpConst 0x8008, 46
    VMJumpIf CMP_EQ, L_1B60
    VMJump L_1B6C

L_1B60:
    WorkSetConst 0x8009, 0
    VMJump L_1E3B

L_1B6C:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_1B7F
    VMJump L_1B8B

L_1B7F:
    WorkSetConst 0x8009, 2
    VMJump L_1E3B

L_1B8B:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_1B9E
    VMJump L_1BAA

L_1B9E:
    WorkSetConst 0x8009, 2
    VMJump L_1E3B

L_1BAA:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_1BBD
    VMJump L_1BC9

L_1BBD:
    WorkSetConst 0x8009, 4
    VMJump L_1E3B

L_1BC9:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_1BDC
    VMJump L_1BE8

L_1BDC:
    WorkSetConst 0x8009, 4
    VMJump L_1E3B

L_1BE8:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_1BFB
    VMJump L_1C07

L_1BFB:
    WorkSetConst 0x8009, 4
    VMJump L_1E3B

L_1C07:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_1C1A
    VMJump L_1C26

L_1C1A:
    WorkSetConst 0x8009, 4
    VMJump L_1E3B

L_1C26:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_1C39
    VMJump L_1C45

L_1C39:
    WorkSetConst 0x8009, 8
    VMJump L_1E3B

L_1C45:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_1C58
    VMJump L_1C64

L_1C58:
    WorkSetConst 0x8009, 8
    VMJump L_1E3B

L_1C64:
    WorkCmpConst 0x8008, 9
    VMJumpIf CMP_EQ, L_1C77
    VMJump L_1C83

L_1C77:
    WorkSetConst 0x8009, 8
    VMJump L_1E3B

L_1C83:
    WorkCmpConst 0x8008, 10
    VMJumpIf CMP_EQ, L_1C96
    VMJump L_1CA2

L_1C96:
    WorkSetConst 0x8009, 8
    VMJump L_1E3B

L_1CA2:
    WorkCmpConst 0x8008, 11
    VMJumpIf CMP_EQ, L_1CB5
    VMJump L_1CC1

L_1CB5:
    WorkSetConst 0x8009, 12
    VMJump L_1E3B

L_1CC1:
    WorkCmpConst 0x8008, 12
    VMJumpIf CMP_EQ, L_1CD4
    VMJump L_1CE0

L_1CD4:
    WorkSetConst 0x8009, 12
    VMJump L_1E3B

L_1CE0:
    WorkCmpConst 0x8008, 13
    VMJumpIf CMP_EQ, L_1CF3
    VMJump L_1CFF

L_1CF3:
    WorkSetConst 0x8009, 12
    VMJump L_1E3B

L_1CFF:
    WorkCmpConst 0x8008, 14
    VMJumpIf CMP_EQ, L_1D12
    VMJump L_1D1E

L_1D12:
    WorkSetConst 0x8009, 12
    VMJump L_1E3B

L_1D1E:
    WorkCmpConst 0x8008, 15
    VMJumpIf CMP_EQ, L_1D31
    VMJump L_1D3D

L_1D31:
    WorkSetConst 0x8009, 16
    VMJump L_1E3B

L_1D3D:
    WorkCmpConst 0x8008, 16
    VMJumpIf CMP_EQ, L_1D50
    VMJump L_1D5C

L_1D50:
    WorkSetConst 0x8009, 16
    VMJump L_1E3B

L_1D5C:
    WorkCmpConst 0x8008, 17
    VMJumpIf CMP_EQ, L_1D6F
    VMJump L_1D7B

L_1D6F:
    WorkSetConst 0x8009, 16
    VMJump L_1E3B

L_1D7B:
    WorkCmpConst 0x8008, 18
    VMJumpIf CMP_EQ, L_1D8E
    VMJump L_1D9A

L_1D8E:
    WorkSetConst 0x8009, 16
    VMJump L_1E3B

L_1D9A:
    WorkCmpConst 0x8008, 19
    VMJumpIf CMP_EQ, L_1DAD
    VMJump L_1DB9

L_1DAD:
    WorkSetConst 0x8009, 20
    VMJump L_1E3B

L_1DB9:
    WorkCmpConst 0x8008, 20
    VMJumpIf CMP_EQ, L_1DCC
    VMJump L_1DD8

L_1DCC:
    WorkSetConst 0x8009, 20
    VMJump L_1E3B

L_1DD8:
    WorkCmpConst 0x8008, 21
    VMJumpIf CMP_EQ, L_1DEB
    VMJump L_1DF7

L_1DEB:
    WorkSetConst 0x8009, 20
    VMJump L_1E3B

L_1DF7:
    WorkCmpConst 0x8008, 22
    VMJumpIf CMP_EQ, L_1E0A
    VMJump L_1E16

L_1E0A:
    WorkSetConst 0x8009, 24
    VMJump L_1E3B

L_1E16:
    WorkCmpConst 0x8008, 23
    VMJumpIf CMP_EQ, L_1E29
    VMJump L_1E35

L_1E29:
    WorkSetConst 0x8009, 24
    VMJump L_1E3B

L_1E35:
    WorkSetConst 0x8009, 0

L_1E3B:
    WorkGet 0x800a, 0x8009
    WorkGet 0x800b, 0x8009
    VMReturn

L_1E49:
    WorkCmpConst 0x8008, 24
    VMJumpIf CMP_EQ, L_1E69
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_1E69
    VMJump L_1E81

L_1E69:
    WorkSetConst 0x8009, 0
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_1E81:
    WorkCmpConst 0x8008, 25
    VMJumpIf CMP_EQ, L_1EA1
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_1EA1
    VMJump L_1EB9

L_1EA1:
    WorkSetConst 0x8009, 8
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_1EB9:
    WorkCmpConst 0x8008, 26
    VMJumpIf CMP_EQ, L_1ED9
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_1ED9
    VMJump L_1EF1

L_1ED9:
    WorkSetConst 0x8009, 29
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_1EF1:
    WorkCmpConst 0x8008, 27
    VMJumpIf CMP_EQ, L_1F11
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_1F11
    VMJump L_1F29

L_1F11:
    WorkSetConst 0x8009, 1
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_1F29:
    WorkCmpConst 0x8008, 28
    VMJumpIf CMP_EQ, L_1F49
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_1F49
    VMJump L_1F61

L_1F49:
    WorkSetConst 0x8009, 26
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_1F61:
    WorkCmpConst 0x8008, 29
    VMJumpIf CMP_EQ, L_1F81
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_1F81
    VMJump L_1F99

L_1F81:
    WorkSetConst 0x8009, 25
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_1F99:
    WorkCmpConst 0x8008, 30
    VMJumpIf CMP_EQ, L_1FB9
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_1FB9
    VMJump L_1FD1

L_1FB9:
    WorkSetConst 0x8009, 2
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_1FD1:
    WorkCmpConst 0x8008, 31
    VMJumpIf CMP_EQ, L_1FF1
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_1FF1
    VMJump L_2009

L_1FF1:
    WorkSetConst 0x8009, 7
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2009:
    WorkCmpConst 0x8008, 32
    VMJumpIf CMP_EQ, L_2029
    WorkCmpConst 0x8008, 9
    VMJumpIf CMP_EQ, L_2029
    VMJump L_2041

L_2029:
    WorkSetConst 0x8009, 28
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2041:
    WorkCmpConst 0x8008, 33
    VMJumpIf CMP_EQ, L_2061
    WorkCmpConst 0x8008, 10
    VMJumpIf CMP_EQ, L_2061
    VMJump L_2079

L_2061:
    WorkSetConst 0x8009, 3
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2079:
    WorkCmpConst 0x8008, 34
    VMJumpIf CMP_EQ, L_2099
    WorkCmpConst 0x8008, 11
    VMJumpIf CMP_EQ, L_2099
    VMJump L_20B1

L_2099:
    WorkSetConst 0x8009, 5
    WorkSetConst 0x800a, 11
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_20B1:
    WorkCmpConst 0x8008, 35
    VMJumpIf CMP_EQ, L_20D1
    WorkCmpConst 0x8008, 12
    VMJumpIf CMP_EQ, L_20D1
    VMJump L_20E9

L_20D1:
    WorkSetConst 0x8009, 4
    WorkSetConst 0x800a, 6
    WorkSetConst 0x800b, 14
    VMJump L_2363

L_20E9:
    WorkCmpConst 0x8008, 36
    VMJumpIf CMP_EQ, L_2109
    WorkCmpConst 0x8008, 13
    VMJumpIf CMP_EQ, L_2109
    VMJump L_2121

L_2109:
    WorkSetConst 0x8009, 12
    WorkSetConst 0x800a, 13
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2121:
    WorkCmpConst 0x8008, 37
    VMJumpIf CMP_EQ, L_2141
    WorkCmpConst 0x8008, 14
    VMJumpIf CMP_EQ, L_2141
    VMJump L_2159

L_2141:
    WorkSetConst 0x8009, 24
    WorkSetConst 0x800a, 20
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2159:
    WorkCmpConst 0x8008, 38
    VMJumpIf CMP_EQ, L_2179
    WorkCmpConst 0x8008, 15
    VMJumpIf CMP_EQ, L_2179
    VMJump L_2191

L_2179:
    WorkSetConst 0x8009, 20
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2191:
    WorkCmpConst 0x8008, 39
    VMJumpIf CMP_EQ, L_21B1
    WorkCmpConst 0x8008, 16
    VMJumpIf CMP_EQ, L_21B1
    VMJump L_21C9

L_21B1:
    WorkSetConst 0x8009, 21
    WorkSetConst 0x800a, 22
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_21C9:
    WorkCmpConst 0x8008, 40
    VMJumpIf CMP_EQ, L_21E9
    WorkCmpConst 0x8008, 17
    VMJumpIf CMP_EQ, L_21E9
    VMJump L_2201

L_21E9:
    WorkSetConst 0x8009, 10
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2201:
    WorkCmpConst 0x8008, 41
    VMJumpIf CMP_EQ, L_2221
    WorkCmpConst 0x8008, 18
    VMJumpIf CMP_EQ, L_2221
    VMJump L_2239

L_2221:
    WorkSetConst 0x8009, 16
    WorkSetConst 0x800a, 17
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2239:
    WorkCmpConst 0x8008, 42
    VMJumpIf CMP_EQ, L_2259
    WorkCmpConst 0x8008, 19
    VMJumpIf CMP_EQ, L_2259
    VMJump L_2271

L_2259:
    WorkSetConst 0x8009, 23
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2271:
    WorkCmpConst 0x8008, 43
    VMJumpIf CMP_EQ, L_2291
    WorkCmpConst 0x8008, 20
    VMJumpIf CMP_EQ, L_2291
    VMJump L_22A9

L_2291:
    WorkSetConst 0x8009, 19
    WorkSetConst 0x800a, 9
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_22A9:
    WorkCmpConst 0x8008, 44
    VMJumpIf CMP_EQ, L_22C9
    WorkCmpConst 0x8008, 21
    VMJumpIf CMP_EQ, L_22C9
    VMJump L_22E1

L_22C9:
    WorkSetConst 0x8009, 18
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_22E1:
    WorkCmpConst 0x8008, 45
    VMJumpIf CMP_EQ, L_2301
    WorkCmpConst 0x8008, 22
    VMJumpIf CMP_EQ, L_2301
    VMJump L_2319

L_2301:
    WorkSetConst 0x8009, 27
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2319:
    WorkCmpConst 0x8008, 46
    VMJumpIf CMP_EQ, L_2339
    WorkCmpConst 0x8008, 23
    VMJumpIf CMP_EQ, L_2339
    VMJump L_2351

L_2339:
    WorkSetConst 0x8009, 15
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255
    VMJump L_2363

L_2351:
    WorkSetConst 0x8009, 255
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255

L_2363:
    VMReturn
    WorkGet 0x804d, 0x8008
    WorkCmpConst 0x804d, 24
    VMJumpIf CMP_EQ, L_237E
    VMJump L_238A

L_237E:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_238A:
    WorkCmpConst 0x804d, 1
    VMJumpIf CMP_EQ, L_239D
    VMJump L_23A9

L_239D:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_23A9:
    WorkCmpConst 0x804d, 25
    VMJumpIf CMP_EQ, L_23BC
    VMJump L_23C8

L_23BC:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_23C8:
    WorkCmpConst 0x804d, 2
    VMJumpIf CMP_EQ, L_23DB
    VMJump L_23E7

L_23DB:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_23E7:
    WorkCmpConst 0x804d, 26
    VMJumpIf CMP_EQ, L_23FA
    VMJump L_2406

L_23FA:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_2406:
    WorkCmpConst 0x804d, 3
    VMJumpIf CMP_EQ, L_2419
    VMJump L_2425

L_2419:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2425:
    WorkCmpConst 0x804d, 27
    VMJumpIf CMP_EQ, L_2438
    VMJump L_2444

L_2438:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_2444:
    WorkCmpConst 0x804d, 4
    VMJumpIf CMP_EQ, L_2457
    VMJump L_2463

L_2457:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2463:
    WorkCmpConst 0x804d, 28
    VMJumpIf CMP_EQ, L_2476
    VMJump L_2482

L_2476:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_2482:
    WorkCmpConst 0x804d, 5
    VMJumpIf CMP_EQ, L_2495
    VMJump L_24A1

L_2495:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_24A1:
    WorkCmpConst 0x804d, 29
    VMJumpIf CMP_EQ, L_24B4
    VMJump L_24C0

L_24B4:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_24C0:
    WorkCmpConst 0x804d, 6
    VMJumpIf CMP_EQ, L_24D3
    VMJump L_24DF

L_24D3:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_24DF:
    WorkCmpConst 0x804d, 30
    VMJumpIf CMP_EQ, L_24F2
    VMJump L_24FE

L_24F2:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_24FE:
    WorkCmpConst 0x804d, 7
    VMJumpIf CMP_EQ, L_2511
    VMJump L_251D

L_2511:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_251D:
    WorkCmpConst 0x804d, 31
    VMJumpIf CMP_EQ, L_2530
    VMJump L_253C

L_2530:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_253C:
    WorkCmpConst 0x804d, 8
    VMJumpIf CMP_EQ, L_254F
    VMJump L_255B

L_254F:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_255B:
    WorkCmpConst 0x804d, 32
    VMJumpIf CMP_EQ, L_256E
    VMJump L_257A

L_256E:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_257A:
    WorkCmpConst 0x804d, 9
    VMJumpIf CMP_EQ, L_258D
    VMJump L_2599

L_258D:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2599:
    WorkCmpConst 0x804d, 33
    VMJumpIf CMP_EQ, L_25AC
    VMJump L_25B8

L_25AC:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_25B8:
    WorkCmpConst 0x804d, 10
    VMJumpIf CMP_EQ, L_25CB
    VMJump L_25D7

L_25CB:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_25D7:
    WorkCmpConst 0x804d, 34
    VMJumpIf CMP_EQ, L_25EA
    VMJump L_25F6

L_25EA:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_25F6:
    WorkCmpConst 0x804d, 11
    VMJumpIf CMP_EQ, L_2609
    VMJump L_2615

L_2609:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2615:
    WorkCmpConst 0x804d, 35
    VMJumpIf CMP_EQ, L_2628
    VMJump L_2634

L_2628:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_2634:
    WorkCmpConst 0x804d, 12
    VMJumpIf CMP_EQ, L_2647
    VMJump L_2653

L_2647:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2653:
    WorkCmpConst 0x804d, 36
    VMJumpIf CMP_EQ, L_2666
    VMJump L_2672

L_2666:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_2672:
    WorkCmpConst 0x804d, 13
    VMJumpIf CMP_EQ, L_2685
    VMJump L_2691

L_2685:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2691:
    WorkCmpConst 0x804d, 37
    VMJumpIf CMP_EQ, L_26A4
    VMJump L_26B0

L_26A4:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_26B0:
    WorkCmpConst 0x804d, 14
    VMJumpIf CMP_EQ, L_26C3
    VMJump L_26CF

L_26C3:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_26CF:
    WorkCmpConst 0x804d, 38
    VMJumpIf CMP_EQ, L_26E2
    VMJump L_26EE

L_26E2:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_26EE:
    WorkCmpConst 0x804d, 15
    VMJumpIf CMP_EQ, L_2701
    VMJump L_270D

L_2701:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_270D:
    WorkCmpConst 0x804d, 39
    VMJumpIf CMP_EQ, L_2720
    VMJump L_272C

L_2720:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_272C:
    WorkCmpConst 0x804d, 16
    VMJumpIf CMP_EQ, L_273F
    VMJump L_274B

L_273F:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_274B:
    WorkCmpConst 0x804d, 40
    VMJumpIf CMP_EQ, L_275E
    VMJump L_276A

L_275E:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_276A:
    WorkCmpConst 0x804d, 17
    VMJumpIf CMP_EQ, L_277D
    VMJump L_2789

L_277D:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2789:
    WorkCmpConst 0x804d, 41
    VMJumpIf CMP_EQ, L_279C
    VMJump L_27A8

L_279C:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_27A8:
    WorkCmpConst 0x804d, 18
    VMJumpIf CMP_EQ, L_27BB
    VMJump L_27C7

L_27BB:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_27C7:
    WorkCmpConst 0x804d, 42
    VMJumpIf CMP_EQ, L_27DA
    VMJump L_27E6

L_27DA:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_27E6:
    WorkCmpConst 0x804d, 19
    VMJumpIf CMP_EQ, L_27F9
    VMJump L_2805

L_27F9:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2805:
    WorkCmpConst 0x804d, 43
    VMJumpIf CMP_EQ, L_2818
    VMJump L_2824

L_2818:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_2824:
    WorkCmpConst 0x804d, 20
    VMJumpIf CMP_EQ, L_2837
    VMJump L_2843

L_2837:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2843:
    WorkCmpConst 0x804d, 44
    VMJumpIf CMP_EQ, L_2856
    VMJump L_2862

L_2856:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_2862:
    WorkCmpConst 0x804d, 21
    VMJumpIf CMP_EQ, L_2875
    VMJump L_2881

L_2875:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_2881:
    WorkCmpConst 0x804d, 45
    VMJumpIf CMP_EQ, L_2894
    VMJump L_28A0

L_2894:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_28A0:
    WorkCmpConst 0x804d, 22
    VMJumpIf CMP_EQ, L_28B3
    VMJump L_28BF

L_28B3:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_28BF:
    WorkCmpConst 0x804d, 46
    VMJumpIf CMP_EQ, L_28D2
    VMJump L_28DE

L_28D2:
    WorkSetConst 0x8010, 0
    VMJump L_2903

L_28DE:
    WorkCmpConst 0x804d, 23
    VMJumpIf CMP_EQ, L_28F1
    VMJump L_28FD

L_28F1:
    WorkSetConst 0x8010, 1
    VMJump L_2903

L_28FD:
    WorkSetConst 0x8010, 0

L_2903:
    VMReturn

L_2905:
    SurveyGetCurrentQuestionID 0x804d
    WorkCmpConst 0x804d, 24
    VMJumpIf CMP_EQ, L_291C
    VMJump L_2928

L_291C:
    WorkSetConst 0x8010, 3
    VMJump L_2EA1

L_2928:
    WorkCmpConst 0x804d, 1
    VMJumpIf CMP_EQ, L_293B
    VMJump L_2947

L_293B:
    WorkSetConst 0x8010, 3
    VMJump L_2EA1

L_2947:
    WorkCmpConst 0x804d, 25
    VMJumpIf CMP_EQ, L_295A
    VMJump L_2966

L_295A:
    WorkSetConst 0x8010, 6
    VMJump L_2EA1

L_2966:
    WorkCmpConst 0x804d, 2
    VMJumpIf CMP_EQ, L_2979
    VMJump L_2985

L_2979:
    WorkSetConst 0x8010, 6
    VMJump L_2EA1

L_2985:
    WorkCmpConst 0x804d, 26
    VMJumpIf CMP_EQ, L_2998
    VMJump L_29A4

L_2998:
    WorkSetConst 0x8010, 10
    VMJump L_2EA1

L_29A4:
    WorkCmpConst 0x804d, 3
    VMJumpIf CMP_EQ, L_29B7
    VMJump L_29C3

L_29B7:
    WorkSetConst 0x8010, 10
    VMJump L_2EA1

L_29C3:
    WorkCmpConst 0x804d, 27
    VMJumpIf CMP_EQ, L_29D6
    VMJump L_29E2

L_29D6:
    WorkSetConst 0x8010, 13
    VMJump L_2EA1

L_29E2:
    WorkCmpConst 0x804d, 4
    VMJumpIf CMP_EQ, L_29F5
    VMJump L_2A01

L_29F5:
    WorkSetConst 0x8010, 13
    VMJump L_2EA1

L_2A01:
    WorkCmpConst 0x804d, 28
    VMJumpIf CMP_EQ, L_2A14
    VMJump L_2A20

L_2A14:
    WorkSetConst 0x8010, 14
    VMJump L_2EA1

L_2A20:
    WorkCmpConst 0x804d, 5
    VMJumpIf CMP_EQ, L_2A33
    VMJump L_2A3F

L_2A33:
    WorkSetConst 0x8010, 14
    VMJump L_2EA1

L_2A3F:
    WorkCmpConst 0x804d, 29
    VMJumpIf CMP_EQ, L_2A52
    VMJump L_2A5E

L_2A52:
    WorkSetConst 0x8010, 15
    VMJump L_2EA1

L_2A5E:
    WorkCmpConst 0x804d, 6
    VMJumpIf CMP_EQ, L_2A71
    VMJump L_2A7D

L_2A71:
    WorkSetConst 0x8010, 15
    VMJump L_2EA1

L_2A7D:
    WorkCmpConst 0x804d, 30
    VMJumpIf CMP_EQ, L_2A90
    VMJump L_2A9C

L_2A90:
    WorkSetConst 0x8010, 25
    VMJump L_2EA1

L_2A9C:
    WorkCmpConst 0x804d, 7
    VMJumpIf CMP_EQ, L_2AAF
    VMJump L_2ABB

L_2AAF:
    WorkSetConst 0x8010, 25
    VMJump L_2EA1

L_2ABB:
    WorkCmpConst 0x804d, 31
    VMJumpIf CMP_EQ, L_2ACE
    VMJump L_2ADA

L_2ACE:
    WorkSetConst 0x8010, 28
    VMJump L_2EA1

L_2ADA:
    WorkCmpConst 0x804d, 8
    VMJumpIf CMP_EQ, L_2AED
    VMJump L_2AF9

L_2AED:
    WorkSetConst 0x8010, 28
    VMJump L_2EA1

L_2AF9:
    WorkCmpConst 0x804d, 32
    VMJumpIf CMP_EQ, L_2B0C
    VMJump L_2B18

L_2B0C:
    WorkSetConst 0x8010, 88
    VMJump L_2EA1

L_2B18:
    WorkCmpConst 0x804d, 9
    VMJumpIf CMP_EQ, L_2B2B
    VMJump L_2B37

L_2B2B:
    WorkSetConst 0x8010, 88
    VMJump L_2EA1

L_2B37:
    WorkCmpConst 0x804d, 33
    VMJumpIf CMP_EQ, L_2B4A
    VMJump L_2B56

L_2B4A:
    WorkSetConst 0x8010, 90
    VMJump L_2EA1

L_2B56:
    WorkCmpConst 0x804d, 10
    VMJumpIf CMP_EQ, L_2B69
    VMJump L_2B75

L_2B69:
    WorkSetConst 0x8010, 90
    VMJump L_2EA1

L_2B75:
    WorkCmpConst 0x804d, 34
    VMJumpIf CMP_EQ, L_2B88
    VMJump L_2B94

L_2B88:
    WorkSetConst 0x8010, 93
    VMJump L_2EA1

L_2B94:
    WorkCmpConst 0x804d, 11
    VMJumpIf CMP_EQ, L_2BA7
    VMJump L_2BB3

L_2BA7:
    WorkSetConst 0x8010, 93
    VMJump L_2EA1

L_2BB3:
    WorkCmpConst 0x804d, 35
    VMJumpIf CMP_EQ, L_2BC6
    VMJump L_2BD2

L_2BC6:
    WorkSetConst 0x8010, 87
    VMJump L_2EA1

L_2BD2:
    WorkCmpConst 0x804d, 12
    VMJumpIf CMP_EQ, L_2BE5
    VMJump L_2BF1

L_2BE5:
    WorkSetConst 0x8010, 87
    VMJump L_2EA1

L_2BF1:
    WorkCmpConst 0x804d, 36
    VMJumpIf CMP_EQ, L_2C04
    VMJump L_2C10

L_2C04:
    WorkSetConst 0x8010, 89
    VMJump L_2EA1

L_2C10:
    WorkCmpConst 0x804d, 13
    VMJumpIf CMP_EQ, L_2C23
    VMJump L_2C2F

L_2C23:
    WorkSetConst 0x8010, 89
    VMJump L_2EA1

L_2C2F:
    WorkCmpConst 0x804d, 37
    VMJumpIf CMP_EQ, L_2C42
    VMJump L_2C4E

L_2C42:
    WorkSetConst 0x8010, 91
    VMJump L_2EA1

L_2C4E:
    WorkCmpConst 0x804d, 14
    VMJumpIf CMP_EQ, L_2C61
    VMJump L_2C6D

L_2C61:
    WorkSetConst 0x8010, 91
    VMJump L_2EA1

L_2C6D:
    WorkCmpConst 0x804d, 38
    VMJumpIf CMP_EQ, L_2C80
    VMJump L_2C8C

L_2C80:
    WorkSetConst 0x8010, 51
    VMJump L_2EA1

L_2C8C:
    WorkCmpConst 0x804d, 15
    VMJumpIf CMP_EQ, L_2C9F
    VMJump L_2CAB

L_2C9F:
    WorkSetConst 0x8010, 51
    VMJump L_2EA1

L_2CAB:
    WorkCmpConst 0x804d, 39
    VMJumpIf CMP_EQ, L_2CBE
    VMJump L_2CCA

L_2CBE:
    WorkSetConst 0x8010, 45
    VMJump L_2EA1

L_2CCA:
    WorkCmpConst 0x804d, 16
    VMJumpIf CMP_EQ, L_2CDD
    VMJump L_2CE9

L_2CDD:
    WorkSetConst 0x8010, 45
    VMJump L_2EA1

L_2CE9:
    WorkCmpConst 0x804d, 40
    VMJumpIf CMP_EQ, L_2CFC
    VMJump L_2D08

L_2CFC:
    WorkSetConst 0x8010, 48
    VMJump L_2EA1

L_2D08:
    WorkCmpConst 0x804d, 17
    VMJumpIf CMP_EQ, L_2D1B
    VMJump L_2D27

L_2D1B:
    WorkSetConst 0x8010, 48
    VMJump L_2EA1

L_2D27:
    WorkCmpConst 0x804d, 41
    VMJumpIf CMP_EQ, L_2D3A
    VMJump L_2D46

L_2D3A:
    WorkSetConst 0x8010, 47
    VMJump L_2EA1

L_2D46:
    WorkCmpConst 0x804d, 18
    VMJumpIf CMP_EQ, L_2D59
    VMJump L_2D65

L_2D59:
    WorkSetConst 0x8010, 47
    VMJump L_2EA1

L_2D65:
    WorkCmpConst 0x804d, 42
    VMJumpIf CMP_EQ, L_2D78
    VMJump L_2D84

L_2D78:
    WorkSetConst 0x8010, 46
    VMJump L_2EA1

L_2D84:
    WorkCmpConst 0x804d, 19
    VMJumpIf CMP_EQ, L_2D97
    VMJump L_2DA3

L_2D97:
    WorkSetConst 0x8010, 46
    VMJump L_2EA1

L_2DA3:
    WorkCmpConst 0x804d, 43
    VMJumpIf CMP_EQ, L_2DB6
    VMJump L_2DC2

L_2DB6:
    WorkSetConst 0x8010, 52
    VMJump L_2EA1

L_2DC2:
    WorkCmpConst 0x804d, 20
    VMJumpIf CMP_EQ, L_2DD5
    VMJump L_2DE1

L_2DD5:
    WorkSetConst 0x8010, 52
    VMJump L_2EA1

L_2DE1:
    WorkCmpConst 0x804d, 44
    VMJumpIf CMP_EQ, L_2DF4
    VMJump L_2E00

L_2DF4:
    WorkSetConst 0x8010, 49
    VMJump L_2EA1

L_2E00:
    WorkCmpConst 0x804d, 21
    VMJumpIf CMP_EQ, L_2E13
    VMJump L_2E1F

L_2E13:
    WorkSetConst 0x8010, 49
    VMJump L_2EA1

L_2E1F:
    WorkCmpConst 0x804d, 45
    VMJumpIf CMP_EQ, L_2E32
    VMJump L_2E3E

L_2E32:
    WorkSetConst 0x8010, 106
    VMJump L_2EA1

L_2E3E:
    WorkCmpConst 0x804d, 22
    VMJumpIf CMP_EQ, L_2E51
    VMJump L_2E5D

L_2E51:
    WorkSetConst 0x8010, 106
    VMJump L_2EA1

L_2E5D:
    WorkCmpConst 0x804d, 46
    VMJumpIf CMP_EQ, L_2E70
    VMJump L_2E7C

L_2E70:
    WorkSetConst 0x8010, 92
    VMJump L_2EA1

L_2E7C:
    WorkCmpConst 0x804d, 23
    VMJumpIf CMP_EQ, L_2E8F
    VMJump L_2E9B

L_2E8F:
    WorkSetConst 0x8010, 92
    VMJump L_2EA1

L_2E9B:
    WorkSetConst 0x8010, 30

L_2EA1:
    VMReturn

L_2EA3:
    WorkSetConst 0x804d, 145
    WorkSetConst 0x804e, 190
    WorkGet 0x804f, 0x804d

L_2EB5:
    VMStackPush 0x804f
    VMStackPush 0x804e
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_2EF5
    FlagGet 0x804f, 0x8050
    VMStackPush 0x8050
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2EE9
    WorkSetConst 0x8010, 0
    VMReturn

L_2EE9:
    WorkAdd 0x804f, 1
    VMJump L_2EB5

L_2EF5:
    WorkSetConst 0x8010, 1
    VMReturn

L_2EFD:
    SurveyGetCurrentQuestionID 0x804d
    VMStackPush 0x804d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2F20
    WorkSetConst 0x8010, 0
    VMJump L_2F26

L_2F20:
    WorkSetConst 0x8010, 1

L_2F26:
    VMReturn

L_2F28:
    SurveyGetCurrentQuestionID 0x804d
    // "What would you like to do today?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WhatWouldLikeToday, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 12, 65535, 12
    VMStackPush 0x804d
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2F64
    ListMenuAdd 15, 65535, 15

L_2F64:
    ListMenuAdd 13, 65535, 13
    ListMenuAdd 14, 65535, 14
    ListMenuShow
    VMReturn

L_2F78:
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 18, 65535, 0
    ListMenuAdd 17, 65535, 1
    ListMenuAdd 19, 65535, 2
    ListMenuShow
    VMReturn

L_2F9D:
    WorkSetConst 0x8054, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_2FBF
    VMJump L_331B

L_2FBF:
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_3056
    FlagGet EVENT_FLAG_0x0091, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2FF3
    ListMenuAdd 28, 65535, 24

L_2FF3:
    FlagGet EVENT_FLAG_0x0093, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3014
    ListMenuAdd 29, 65535, 25

L_3014:
    FlagGet EVENT_FLAG_0x0095, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3035
    ListMenuAdd 30, 65535, 26

L_3035:
    FlagGet EVENT_FLAG_0x0097, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3056
    ListMenuAdd 31, 65535, 27

L_3056:
    VMStackPush 0x8009
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_312F
    FlagGet EVENT_FLAG_0x0099, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_308A
    ListMenuAdd 32, 65535, 28

L_308A:
    FlagGet EVENT_FLAG_0x009b, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_30AB
    ListMenuAdd 33, 65535, 29

L_30AB:
    FlagGet EVENT_FLAG_0x009d, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_30CC
    ListMenuAdd 34, 65535, 30

L_30CC:
    FlagGet EVENT_FLAG_0x009f, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_30ED
    ListMenuAdd 35, 65535, 31

L_30ED:
    FlagGet EVENT_FLAG_0x00a1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_310E
    ListMenuAdd 36, 65535, 32

L_310E:
    FlagGet EVENT_FLAG_0x00a3, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_312F
    ListMenuAdd 37, 65535, 33

L_312F:
    VMStackPush 0x8009
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_3208
    FlagGet EVENT_FLAG_0x00a5, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3163
    ListMenuAdd 38, 65535, 34

L_3163:
    FlagGet EVENT_FLAG_0x00a7, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3184
    ListMenuAdd 39, 65535, 35

L_3184:
    FlagGet EVENT_FLAG_0x00a9, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31A5
    ListMenuAdd 40, 65535, 36

L_31A5:
    FlagGet EVENT_FLAG_0x00ab, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31C6
    ListMenuAdd 41, 65535, 37

L_31C6:
    FlagGet EVENT_FLAG_0x00ad, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31E7
    ListMenuAdd 42, 65535, 38

L_31E7:
    FlagGet EVENT_FLAG_0x00af, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3208
    ListMenuAdd 43, 65535, 39

L_3208:
    VMStackPush 0x8009
    VMStackPushConst 4
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_32C0
    FlagGet EVENT_FLAG_0x00b1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_323C
    ListMenuAdd 44, 65535, 40

L_323C:
    FlagGet EVENT_FLAG_0x00b3, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_325D
    ListMenuAdd 45, 65535, 41

L_325D:
    FlagGet EVENT_FLAG_0x00b5, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_327E
    ListMenuAdd 46, 65535, 42

L_327E:
    FlagGet EVENT_FLAG_0x00b7, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_329F
    ListMenuAdd 47, 65535, 43

L_329F:
    FlagGet EVENT_FLAG_0x00b9, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_32C0
    ListMenuAdd 48, 65535, 44

L_32C0:
    VMStackPush 0x8009
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_3315
    FlagGet EVENT_FLAG_0x00bb, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_32F4
    ListMenuAdd 49, 65535, 45

L_32F4:
    FlagGet EVENT_FLAG_0x00bd, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3315
    ListMenuAdd 50, 65535, 46

L_3315:
    VMJump L_368A

L_331B:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_332E
    VMJump L_368A

L_332E:
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_33C5
    FlagGet EVENT_FLAG_0x0092, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3362
    ListMenuAdd 28, 65535, 1

L_3362:
    FlagGet EVENT_FLAG_0x0094, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3383
    ListMenuAdd 29, 65535, 2

L_3383:
    FlagGet EVENT_FLAG_0x0096, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_33A4
    ListMenuAdd 30, 65535, 3

L_33A4:
    FlagGet EVENT_FLAG_0x0098, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_33C5
    ListMenuAdd 31, 65535, 4

L_33C5:
    VMStackPush 0x8009
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_349E
    FlagGet EVENT_FLAG_0x009a, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_33F9
    ListMenuAdd 32, 65535, 5

L_33F9:
    FlagGet EVENT_FLAG_0x009c, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_341A
    ListMenuAdd 33, 65535, 6

L_341A:
    FlagGet EVENT_FLAG_0x009e, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_343B
    ListMenuAdd 34, 65535, 7

L_343B:
    FlagGet EVENT_FLAG_0x00a0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_345C
    ListMenuAdd 35, 65535, 8

L_345C:
    FlagGet EVENT_FLAG_0x00a2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_347D
    ListMenuAdd 36, 65535, 9

L_347D:
    FlagGet EVENT_FLAG_0x00a4, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_349E
    ListMenuAdd 37, 65535, 10

L_349E:
    VMStackPush 0x8009
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_3577
    FlagGet EVENT_FLAG_0x00a6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_34D2
    ListMenuAdd 38, 65535, 11

L_34D2:
    FlagGet EVENT_FLAG_0x00a8, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_34F3
    ListMenuAdd 39, 65535, 12

L_34F3:
    FlagGet EVENT_FLAG_0x00aa, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3514
    ListMenuAdd 40, 65535, 13

L_3514:
    FlagGet EVENT_FLAG_0x00ac, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3535
    ListMenuAdd 41, 65535, 14

L_3535:
    FlagGet EVENT_FLAG_0x00ae, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3556
    ListMenuAdd 42, 65535, 15

L_3556:
    FlagGet EVENT_FLAG_0x00b0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3577
    ListMenuAdd 43, 65535, 16

L_3577:
    VMStackPush 0x8009
    VMStackPushConst 4
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_362F
    FlagGet EVENT_FLAG_0x00b2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_35AB
    ListMenuAdd 44, 65535, 17

L_35AB:
    FlagGet EVENT_FLAG_0x00b4, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_35CC
    ListMenuAdd 45, 65535, 18

L_35CC:
    FlagGet EVENT_FLAG_0x00b6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_35ED
    ListMenuAdd 46, 65535, 19

L_35ED:
    FlagGet EVENT_FLAG_0x00b8, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_360E
    ListMenuAdd 47, 65535, 20

L_360E:
    FlagGet EVENT_FLAG_0x00ba, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_362F
    ListMenuAdd 48, 65535, 21

L_362F:
    VMStackPush 0x8009
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_3684
    FlagGet EVENT_FLAG_0x00bc, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3663
    ListMenuAdd 49, 65535, 22

L_3663:
    FlagGet EVENT_FLAG_0x00be, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3684
    ListMenuAdd 50, 65535, 23

L_3684:
    VMJump L_368A

L_368A:
    ListMenuAdd 51, 65535, 0
    ListMenuShow
    WorkSetConst 0x8054, 0
    VMReturn

L_369C:
    SurveyGetCurrentQuestionID 0x804d
    WorkCmpConst 0x804d, 24
    VMJumpIf CMP_EQ, L_36B3
    VMJump L_36BF

L_36B3:
    WorkSetConst 0x804e, 198
    VMJump L_3C34

L_36BF:
    WorkCmpConst 0x804d, 1
    VMJumpIf CMP_EQ, L_36D2
    VMJump L_36DE

L_36D2:
    WorkSetConst 0x804e, 198
    VMJump L_3C34

L_36DE:
    WorkCmpConst 0x804d, 25
    VMJumpIf CMP_EQ, L_36F1
    VMJump L_36FD

L_36F1:
    WorkSetConst 0x804e, 199
    VMJump L_3C34

L_36FD:
    WorkCmpConst 0x804d, 2
    VMJumpIf CMP_EQ, L_3710
    VMJump L_371C

L_3710:
    WorkSetConst 0x804e, 199
    VMJump L_3C34

L_371C:
    WorkCmpConst 0x804d, 26
    VMJumpIf CMP_EQ, L_372F
    VMJump L_373B

L_372F:
    WorkSetConst 0x804e, 200
    VMJump L_3C34

L_373B:
    WorkCmpConst 0x804d, 3
    VMJumpIf CMP_EQ, L_374E
    VMJump L_375A

L_374E:
    WorkSetConst 0x804e, 200
    VMJump L_3C34

L_375A:
    WorkCmpConst 0x804d, 27
    VMJumpIf CMP_EQ, L_376D
    VMJump L_3779

L_376D:
    WorkSetConst 0x804e, 201
    VMJump L_3C34

L_3779:
    WorkCmpConst 0x804d, 4
    VMJumpIf CMP_EQ, L_378C
    VMJump L_3798

L_378C:
    WorkSetConst 0x804e, 201
    VMJump L_3C34

L_3798:
    WorkCmpConst 0x804d, 28
    VMJumpIf CMP_EQ, L_37AB
    VMJump L_37B7

L_37AB:
    WorkSetConst 0x804e, 202
    VMJump L_3C34

L_37B7:
    WorkCmpConst 0x804d, 5
    VMJumpIf CMP_EQ, L_37CA
    VMJump L_37D6

L_37CA:
    WorkSetConst 0x804e, 202
    VMJump L_3C34

L_37D6:
    WorkCmpConst 0x804d, 29
    VMJumpIf CMP_EQ, L_37E9
    VMJump L_37F5

L_37E9:
    WorkSetConst 0x804e, 203
    VMJump L_3C34

L_37F5:
    WorkCmpConst 0x804d, 6
    VMJumpIf CMP_EQ, L_3808
    VMJump L_3814

L_3808:
    WorkSetConst 0x804e, 203
    VMJump L_3C34

L_3814:
    WorkCmpConst 0x804d, 30
    VMJumpIf CMP_EQ, L_3827
    VMJump L_3833

L_3827:
    WorkSetConst 0x804e, 204
    VMJump L_3C34

L_3833:
    WorkCmpConst 0x804d, 7
    VMJumpIf CMP_EQ, L_3846
    VMJump L_3852

L_3846:
    WorkSetConst 0x804e, 204
    VMJump L_3C34

L_3852:
    WorkCmpConst 0x804d, 31
    VMJumpIf CMP_EQ, L_3865
    VMJump L_3871

L_3865:
    WorkSetConst 0x804e, 205
    VMJump L_3C34

L_3871:
    WorkCmpConst 0x804d, 8
    VMJumpIf CMP_EQ, L_3884
    VMJump L_3890

L_3884:
    WorkSetConst 0x804e, 205
    VMJump L_3C34

L_3890:
    WorkCmpConst 0x804d, 32
    VMJumpIf CMP_EQ, L_38A3
    VMJump L_38AF

L_38A3:
    WorkSetConst 0x804e, 206
    VMJump L_3C34

L_38AF:
    WorkCmpConst 0x804d, 9
    VMJumpIf CMP_EQ, L_38C2
    VMJump L_38CE

L_38C2:
    WorkSetConst 0x804e, 206
    VMJump L_3C34

L_38CE:
    WorkCmpConst 0x804d, 33
    VMJumpIf CMP_EQ, L_38E1
    VMJump L_38ED

L_38E1:
    WorkSetConst 0x804e, 207
    VMJump L_3C34

L_38ED:
    WorkCmpConst 0x804d, 10
    VMJumpIf CMP_EQ, L_3900
    VMJump L_390C

L_3900:
    WorkSetConst 0x804e, 207
    VMJump L_3C34

L_390C:
    WorkCmpConst 0x804d, 34
    VMJumpIf CMP_EQ, L_391F
    VMJump L_392B

L_391F:
    WorkSetConst 0x804e, 208
    VMJump L_3C34

L_392B:
    WorkCmpConst 0x804d, 11
    VMJumpIf CMP_EQ, L_393E
    VMJump L_394A

L_393E:
    WorkSetConst 0x804e, 208
    VMJump L_3C34

L_394A:
    WorkCmpConst 0x804d, 35
    VMJumpIf CMP_EQ, L_395D
    VMJump L_3969

L_395D:
    WorkSetConst 0x804e, 209
    VMJump L_3C34

L_3969:
    WorkCmpConst 0x804d, 12
    VMJumpIf CMP_EQ, L_397C
    VMJump L_3988

L_397C:
    WorkSetConst 0x804e, 209
    VMJump L_3C34

L_3988:
    WorkCmpConst 0x804d, 36
    VMJumpIf CMP_EQ, L_399B
    VMJump L_39A7

L_399B:
    WorkSetConst 0x804e, 210
    VMJump L_3C34

L_39A7:
    WorkCmpConst 0x804d, 13
    VMJumpIf CMP_EQ, L_39BA
    VMJump L_39C6

L_39BA:
    WorkSetConst 0x804e, 210
    VMJump L_3C34

L_39C6:
    WorkCmpConst 0x804d, 37
    VMJumpIf CMP_EQ, L_39D9
    VMJump L_39E5

L_39D9:
    WorkSetConst 0x804e, 211
    VMJump L_3C34

L_39E5:
    WorkCmpConst 0x804d, 14
    VMJumpIf CMP_EQ, L_39F8
    VMJump L_3A04

L_39F8:
    WorkSetConst 0x804e, 211
    VMJump L_3C34

L_3A04:
    WorkCmpConst 0x804d, 38
    VMJumpIf CMP_EQ, L_3A17
    VMJump L_3A23

L_3A17:
    WorkSetConst 0x804e, 212
    VMJump L_3C34

L_3A23:
    WorkCmpConst 0x804d, 15
    VMJumpIf CMP_EQ, L_3A36
    VMJump L_3A42

L_3A36:
    WorkSetConst 0x804e, 212
    VMJump L_3C34

L_3A42:
    WorkCmpConst 0x804d, 39
    VMJumpIf CMP_EQ, L_3A55
    VMJump L_3A61

L_3A55:
    WorkSetConst 0x804e, 213
    VMJump L_3C34

L_3A61:
    WorkCmpConst 0x804d, 16
    VMJumpIf CMP_EQ, L_3A74
    VMJump L_3A80

L_3A74:
    WorkSetConst 0x804e, 213
    VMJump L_3C34

L_3A80:
    WorkCmpConst 0x804d, 40
    VMJumpIf CMP_EQ, L_3A93
    VMJump L_3A9F

L_3A93:
    WorkSetConst 0x804e, 214
    VMJump L_3C34

L_3A9F:
    WorkCmpConst 0x804d, 17
    VMJumpIf CMP_EQ, L_3AB2
    VMJump L_3ABE

L_3AB2:
    WorkSetConst 0x804e, 214
    VMJump L_3C34

L_3ABE:
    WorkCmpConst 0x804d, 41
    VMJumpIf CMP_EQ, L_3AD1
    VMJump L_3ADD

L_3AD1:
    WorkSetConst 0x804e, 215
    VMJump L_3C34

L_3ADD:
    WorkCmpConst 0x804d, 18
    VMJumpIf CMP_EQ, L_3AF0
    VMJump L_3AFC

L_3AF0:
    WorkSetConst 0x804e, 215
    VMJump L_3C34

L_3AFC:
    WorkCmpConst 0x804d, 42
    VMJumpIf CMP_EQ, L_3B0F
    VMJump L_3B1B

L_3B0F:
    WorkSetConst 0x804e, 216
    VMJump L_3C34

L_3B1B:
    WorkCmpConst 0x804d, 19
    VMJumpIf CMP_EQ, L_3B2E
    VMJump L_3B3A

L_3B2E:
    WorkSetConst 0x804e, 216
    VMJump L_3C34

L_3B3A:
    WorkCmpConst 0x804d, 43
    VMJumpIf CMP_EQ, L_3B4D
    VMJump L_3B59

L_3B4D:
    WorkSetConst 0x804e, 217
    VMJump L_3C34

L_3B59:
    WorkCmpConst 0x804d, 20
    VMJumpIf CMP_EQ, L_3B6C
    VMJump L_3B78

L_3B6C:
    WorkSetConst 0x804e, 217
    VMJump L_3C34

L_3B78:
    WorkCmpConst 0x804d, 44
    VMJumpIf CMP_EQ, L_3B8B
    VMJump L_3B97

L_3B8B:
    WorkSetConst 0x804e, 218
    VMJump L_3C34

L_3B97:
    WorkCmpConst 0x804d, 21
    VMJumpIf CMP_EQ, L_3BAA
    VMJump L_3BB6

L_3BAA:
    WorkSetConst 0x804e, 218
    VMJump L_3C34

L_3BB6:
    WorkCmpConst 0x804d, 45
    VMJumpIf CMP_EQ, L_3BC9
    VMJump L_3BD5

L_3BC9:
    WorkSetConst 0x804e, 219
    VMJump L_3C34

L_3BD5:
    WorkCmpConst 0x804d, 22
    VMJumpIf CMP_EQ, L_3BE8
    VMJump L_3BF4

L_3BE8:
    WorkSetConst 0x804e, 219
    VMJump L_3C34

L_3BF4:
    WorkCmpConst 0x804d, 46
    VMJumpIf CMP_EQ, L_3C07
    VMJump L_3C13

L_3C07:
    WorkSetConst 0x804e, 220
    VMJump L_3C34

L_3C13:
    WorkCmpConst 0x804d, 23
    VMJumpIf CMP_EQ, L_3C26
    VMJump L_3C32

L_3C26:
    WorkSetConst 0x804e, 220
    VMJump L_3C34

L_3C32:
    VMReturn

L_3C34:
    SEPlay SEQ_SE_SYS_80
    SystemMsg 0x804e, 2
    InfoMsgClose
    VMReturn

L_3C42:
    SurveyGetCurrentQuestionID 0x804d
    WorkCmpConst 0x804d, 24
    VMJumpIf CMP_EQ, L_3C59
    VMJump L_3C65

L_3C59:
    WorkSetConst 0x804e, 101
    VMJump L_41DA

L_3C65:
    WorkCmpConst 0x804d, 1
    VMJumpIf CMP_EQ, L_3C78
    VMJump L_3C84

L_3C78:
    WorkSetConst 0x804e, 101
    VMJump L_41DA

L_3C84:
    WorkCmpConst 0x804d, 25
    VMJumpIf CMP_EQ, L_3C97
    VMJump L_3CA3

L_3C97:
    WorkSetConst 0x804e, 102
    VMJump L_41DA

L_3CA3:
    WorkCmpConst 0x804d, 2
    VMJumpIf CMP_EQ, L_3CB6
    VMJump L_3CC2

L_3CB6:
    WorkSetConst 0x804e, 102
    VMJump L_41DA

L_3CC2:
    WorkCmpConst 0x804d, 26
    VMJumpIf CMP_EQ, L_3CD5
    VMJump L_3CE1

L_3CD5:
    WorkSetConst 0x804e, 103
    VMJump L_41DA

L_3CE1:
    WorkCmpConst 0x804d, 3
    VMJumpIf CMP_EQ, L_3CF4
    VMJump L_3D00

L_3CF4:
    WorkSetConst 0x804e, 103
    VMJump L_41DA

L_3D00:
    WorkCmpConst 0x804d, 27
    VMJumpIf CMP_EQ, L_3D13
    VMJump L_3D1F

L_3D13:
    WorkSetConst 0x804e, 104
    VMJump L_41DA

L_3D1F:
    WorkCmpConst 0x804d, 4
    VMJumpIf CMP_EQ, L_3D32
    VMJump L_3D3E

L_3D32:
    WorkSetConst 0x804e, 104
    VMJump L_41DA

L_3D3E:
    WorkCmpConst 0x804d, 28
    VMJumpIf CMP_EQ, L_3D51
    VMJump L_3D5D

L_3D51:
    WorkSetConst 0x804e, 105
    VMJump L_41DA

L_3D5D:
    WorkCmpConst 0x804d, 5
    VMJumpIf CMP_EQ, L_3D70
    VMJump L_3D7C

L_3D70:
    WorkSetConst 0x804e, 105
    VMJump L_41DA

L_3D7C:
    WorkCmpConst 0x804d, 29
    VMJumpIf CMP_EQ, L_3D8F
    VMJump L_3D9B

L_3D8F:
    WorkSetConst 0x804e, 106
    VMJump L_41DA

L_3D9B:
    WorkCmpConst 0x804d, 6
    VMJumpIf CMP_EQ, L_3DAE
    VMJump L_3DBA

L_3DAE:
    WorkSetConst 0x804e, 106
    VMJump L_41DA

L_3DBA:
    WorkCmpConst 0x804d, 30
    VMJumpIf CMP_EQ, L_3DCD
    VMJump L_3DD9

L_3DCD:
    WorkSetConst 0x804e, 107
    VMJump L_41DA

L_3DD9:
    WorkCmpConst 0x804d, 7
    VMJumpIf CMP_EQ, L_3DEC
    VMJump L_3DF8

L_3DEC:
    WorkSetConst 0x804e, 107
    VMJump L_41DA

L_3DF8:
    WorkCmpConst 0x804d, 31
    VMJumpIf CMP_EQ, L_3E0B
    VMJump L_3E17

L_3E0B:
    WorkSetConst 0x804e, 108
    VMJump L_41DA

L_3E17:
    WorkCmpConst 0x804d, 8
    VMJumpIf CMP_EQ, L_3E2A
    VMJump L_3E36

L_3E2A:
    WorkSetConst 0x804e, 108
    VMJump L_41DA

L_3E36:
    WorkCmpConst 0x804d, 32
    VMJumpIf CMP_EQ, L_3E49
    VMJump L_3E55

L_3E49:
    WorkSetConst 0x804e, 109
    VMJump L_41DA

L_3E55:
    WorkCmpConst 0x804d, 9
    VMJumpIf CMP_EQ, L_3E68
    VMJump L_3E74

L_3E68:
    WorkSetConst 0x804e, 109
    VMJump L_41DA

L_3E74:
    WorkCmpConst 0x804d, 33
    VMJumpIf CMP_EQ, L_3E87
    VMJump L_3E93

L_3E87:
    WorkSetConst 0x804e, 110
    VMJump L_41DA

L_3E93:
    WorkCmpConst 0x804d, 10
    VMJumpIf CMP_EQ, L_3EA6
    VMJump L_3EB2

L_3EA6:
    WorkSetConst 0x804e, 110
    VMJump L_41DA

L_3EB2:
    WorkCmpConst 0x804d, 34
    VMJumpIf CMP_EQ, L_3EC5
    VMJump L_3ED1

L_3EC5:
    WorkSetConst 0x804e, 111
    VMJump L_41DA

L_3ED1:
    WorkCmpConst 0x804d, 11
    VMJumpIf CMP_EQ, L_3EE4
    VMJump L_3EF0

L_3EE4:
    WorkSetConst 0x804e, 111
    VMJump L_41DA

L_3EF0:
    WorkCmpConst 0x804d, 35
    VMJumpIf CMP_EQ, L_3F03
    VMJump L_3F0F

L_3F03:
    WorkSetConst 0x804e, 112
    VMJump L_41DA

L_3F0F:
    WorkCmpConst 0x804d, 12
    VMJumpIf CMP_EQ, L_3F22
    VMJump L_3F2E

L_3F22:
    WorkSetConst 0x804e, 112
    VMJump L_41DA

L_3F2E:
    WorkCmpConst 0x804d, 36
    VMJumpIf CMP_EQ, L_3F41
    VMJump L_3F4D

L_3F41:
    WorkSetConst 0x804e, 113
    VMJump L_41DA

L_3F4D:
    WorkCmpConst 0x804d, 13
    VMJumpIf CMP_EQ, L_3F60
    VMJump L_3F6C

L_3F60:
    WorkSetConst 0x804e, 113
    VMJump L_41DA

L_3F6C:
    WorkCmpConst 0x804d, 37
    VMJumpIf CMP_EQ, L_3F7F
    VMJump L_3F8B

L_3F7F:
    WorkSetConst 0x804e, 114
    VMJump L_41DA

L_3F8B:
    WorkCmpConst 0x804d, 14
    VMJumpIf CMP_EQ, L_3F9E
    VMJump L_3FAA

L_3F9E:
    WorkSetConst 0x804e, 114
    VMJump L_41DA

L_3FAA:
    WorkCmpConst 0x804d, 38
    VMJumpIf CMP_EQ, L_3FBD
    VMJump L_3FC9

L_3FBD:
    WorkSetConst 0x804e, 115
    VMJump L_41DA

L_3FC9:
    WorkCmpConst 0x804d, 15
    VMJumpIf CMP_EQ, L_3FDC
    VMJump L_3FE8

L_3FDC:
    WorkSetConst 0x804e, 115
    VMJump L_41DA

L_3FE8:
    WorkCmpConst 0x804d, 39
    VMJumpIf CMP_EQ, L_3FFB
    VMJump L_4007

L_3FFB:
    WorkSetConst 0x804e, 116
    VMJump L_41DA

L_4007:
    WorkCmpConst 0x804d, 16
    VMJumpIf CMP_EQ, L_401A
    VMJump L_4026

L_401A:
    WorkSetConst 0x804e, 116
    VMJump L_41DA

L_4026:
    WorkCmpConst 0x804d, 40
    VMJumpIf CMP_EQ, L_4039
    VMJump L_4045

L_4039:
    WorkSetConst 0x804e, 117
    VMJump L_41DA

L_4045:
    WorkCmpConst 0x804d, 17
    VMJumpIf CMP_EQ, L_4058
    VMJump L_4064

L_4058:
    WorkSetConst 0x804e, 117
    VMJump L_41DA

L_4064:
    WorkCmpConst 0x804d, 41
    VMJumpIf CMP_EQ, L_4077
    VMJump L_4083

L_4077:
    WorkSetConst 0x804e, 118
    VMJump L_41DA

L_4083:
    WorkCmpConst 0x804d, 18
    VMJumpIf CMP_EQ, L_4096
    VMJump L_40A2

L_4096:
    WorkSetConst 0x804e, 118
    VMJump L_41DA

L_40A2:
    WorkCmpConst 0x804d, 42
    VMJumpIf CMP_EQ, L_40B5
    VMJump L_40C1

L_40B5:
    WorkSetConst 0x804e, 119
    VMJump L_41DA

L_40C1:
    WorkCmpConst 0x804d, 19
    VMJumpIf CMP_EQ, L_40D4
    VMJump L_40E0

L_40D4:
    WorkSetConst 0x804e, 119
    VMJump L_41DA

L_40E0:
    WorkCmpConst 0x804d, 43
    VMJumpIf CMP_EQ, L_40F3
    VMJump L_40FF

L_40F3:
    WorkSetConst 0x804e, 120
    VMJump L_41DA

L_40FF:
    WorkCmpConst 0x804d, 20
    VMJumpIf CMP_EQ, L_4112
    VMJump L_411E

L_4112:
    WorkSetConst 0x804e, 120
    VMJump L_41DA

L_411E:
    WorkCmpConst 0x804d, 44
    VMJumpIf CMP_EQ, L_4131
    VMJump L_413D

L_4131:
    WorkSetConst 0x804e, 121
    VMJump L_41DA

L_413D:
    WorkCmpConst 0x804d, 21
    VMJumpIf CMP_EQ, L_4150
    VMJump L_415C

L_4150:
    WorkSetConst 0x804e, 121
    VMJump L_41DA

L_415C:
    WorkCmpConst 0x804d, 45
    VMJumpIf CMP_EQ, L_416F
    VMJump L_417B

L_416F:
    WorkSetConst 0x804e, 122
    VMJump L_41DA

L_417B:
    WorkCmpConst 0x804d, 22
    VMJumpIf CMP_EQ, L_418E
    VMJump L_419A

L_418E:
    WorkSetConst 0x804e, 122
    VMJump L_41DA

L_419A:
    WorkCmpConst 0x804d, 46
    VMJumpIf CMP_EQ, L_41AD
    VMJump L_41B9

L_41AD:
    WorkSetConst 0x804e, 123
    VMJump L_41DA

L_41B9:
    WorkCmpConst 0x804d, 23
    VMJumpIf CMP_EQ, L_41CC
    VMJump L_41D8

L_41CC:
    WorkSetConst 0x804e, 123
    VMJump L_41DA

L_41D8:
    VMReturn

L_41DA:
    SEPlay SEQ_SE_SYS_80
    SystemMsg 0x804e, 2
    InfoMsgClose
    VMReturn

L_41E8:
    TrainerGameInfoCmd_020B 0x8040
    WorkCmpConst 0x8040, 1
    VMJumpIf CMP_EQ, L_41FF
    VMJump L_420B

L_41FF:
    WorkSetConst 0x8041, 249
    VMJump L_4287

L_420B:
    WorkCmpConst 0x8040, 2
    VMJumpIf CMP_EQ, L_421E
    VMJump L_422A

L_421E:
    WorkSetConst 0x8041, 249
    VMJump L_4287

L_422A:
    WorkCmpConst 0x8040, 3
    VMJumpIf CMP_EQ, L_423D
    VMJump L_4249

L_423D:
    WorkSetConst 0x8041, 250
    VMJump L_4287

L_4249:
    WorkCmpConst 0x8040, 4
    VMJumpIf CMP_EQ, L_425C
    VMJump L_4268

L_425C:
    WorkSetConst 0x8041, 251
    VMJump L_4287

L_4268:
    WorkCmpConst 0x8040, 5
    VMJumpIf CMP_EQ, L_427B
    VMJump L_4287

L_427B:
    WorkSetConst 0x8041, 252
    VMJump L_4287

L_4287:
    SEPlay SEQ_SE_SYS_82
    WordSetPlayerName 1
    SystemMsg 0x8041, 2
    InfoMsgClose
    VMReturn

L_4298:
    // "What are people's favorite things?\nWhat is popular right now?[f000]븁\u0000\nHave you ever wondered about\nthese things?[f000]븁\u0000\nWelcome to Passerby Analytics HQ![f000]븁\u0000\nThis is where you can find\nall the answers.[f000]븁\u0000\n...You have good eyes.\nEyes full of curiosity.[f000]븁\u0000\n...Good! I will specially appoint you\nas a statistician![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WhatPeoplesFavoriteThings, 0x8011, 2, 0
    ActorMsgClose
    SEPlay SEQ_SE_SYS_82
    WordSetPlayerName 1
    // "[f000]Ā\u0001\u0001 was appointed\nas a statistician![f000]븁\u0000"
    SystemMsg CasteliaCity13_Text_AppointedStatistician, 2
    InfoMsgClose
    WordSetPlayerName 1
    // "Statisticians have only one task!\nThey conduct requested surveys.[f000]븁\u0000\nFirst, you'll receive survey requests\nfrom me.[f000]븁\u0000\nThen, with the Survey Radar, you'll\nchoose the survey you want to conduct.[f000]븁\u0000\nThen, if you pass by a lot of people,\nthe radar will collect the information.[f000]븁\u0000\nOf course, we've prepared compensation\nfor the survey.[f000]븁\u0000\nStatistician [f000]Ā\u0001\u0001!\nI expect you to do a great job![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_StatisticiansHaveOnlyOne, 0x8011, 2, 0
    VMReturn

L_42C6:
    WordSetPlayerName 1
    // "Statistician [f000]Ā\u0001\u0001,\ngood to see you![f000]븁\u0000\nAre you surveying people vigorously?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_StatisticianGoodSeeSurveying, 0x8011, 2, 0
    VMReturn

L_42D7:
    // "You've finished all the requests.[f000]븁\u0000\nFrom now on, feel free to survey\nwhatever you want to know!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_YouveFinishedAllRequests, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_42E9:
    // "You've already accepted a survey\nrequest![f000]븁\u0000\nTry your best to conduct the survey.\nOr do you want to cancel the survey?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_YouveAlreadyAcceptedSurvey, 0x8011, 2, 0
    WorkSetConst 0x8008, 1
    VMCall L_017A
    VMReturn

L_4303:
    // "You don't seem to have accepted\na survey request.[f000]븁\u0000\nPlease come back if you want\nto accept a request."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_DontSeemHaveAccepted, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_4315:
    // "I see...[f000]븁\u0000\nI'm lonely, so\nplease come visit me sometimes."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_SeeImLonelyPlease, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_4327:
    // "Accept a survey request, right?[f000]븁\u0000\nFirst, choose a survey method."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_AcceptSurveyRequestRight, 0x8011, 2, 0
    VMCall L_2F78
    VMReturn

L_433B:
    WorkGet 0x8040, 0x8008
    TrainerGameInfoCmd_020B 0x8041
    WorkCmpConst 0x8040, 0
    VMJumpIf CMP_EQ, L_4358
    VMJump L_4376

L_4358:
    WorkGet 0x8008, 0x8040
    WorkGet 0x8009, 0x8041
    VMCall L_2F9D
    WorkGet 0x8042, 0x8010
    VMJump L_43AD

L_4376:
    WorkCmpConst 0x8040, 1
    VMJumpIf CMP_EQ, L_4389
    VMJump L_43A7

L_4389:
    WorkGet 0x8008, 0x8040
    WorkGet 0x8009, 0x8041
    VMCall L_2F9D
    WorkGet 0x8042, 0x8010
    VMJump L_43AD

L_43A7:
    WorkSetConst 0x8042, 3

L_43AD:
    WorkGet 0x8008, 0x8040
    WorkGet 0x8010, 0x8042
    VMReturn

L_43BB:
    // "Will you accept this survey request?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WillAcceptSurveyRequest, 0x8011, 2, 0
    WorkSetConst 0x8008, 0
    VMCall L_017A
    VMReturn

L_43D5:
    // "Accept another request?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_AcceptAnotherRequest, 0x8011, 2, 0
    WorkSetConst 0x8008, 0
    VMCall L_017A
    VMReturn

L_43EF:
    // "OK...[f000]븁\u0000\nPlease come back again\nif you want to accept a request."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OkPleaseComeBack, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_4401:
    // "OK...[f000]븁\u0000\nPlease come back again if you\nwant to accept a request."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OkPleaseComeBack_2, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_4413:
    // "Then, when you finish the survey,\nplease come and report it to me."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ThenWhenFinishSurvey, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_4425:
    // "Then, please continue the survey."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ThenPleaseContinueSurvey, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_4437:
    // "You have finished the survey!\nPlease report the survey result.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_HaveFinishedSurveyPlease, 0x8011, 2, 0
    ActorMsgClose
    VMReturn

L_4447:
    WordSetPlayerName 1
    // "It's an excellent survey![f000]븁\u0000\n[f000]Ā\u0001\u0001, you've come up to\nmy expectation.[f000]븁\u0000\nThis is the compensation for the survey.\nPlease accept this.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ItsExcellentSurveyYouve, 0x8011, 2, 0
    ActorMsgClose
    VMReturn

L_445A:
    WordSetPlayerName 1
    // "This completes all the survey requests.\n[f000]Ā\u0001\u0001, well done![f000]븀\u0000\nI'm incredibly moved![f000]븁\u0000\nThis is a token of my appreciation.\nPlease accept this.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_CompletesAllSurveyRequests, 0x8011, 2, 0
    ActorMsgClose
    VMReturn

L_446D:
    // "From now on, feel free to survey\nwhatever you want to know.[f000]븁\u0000\nI have high hopes for\nyour future success!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_FromNowFeelFree, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_447F:
    WordSetPlayerName 1
    // "I'm expecting great work\nfor the next survey, too![f000]븀\u0000\n[f000]Ā\u0001\u0001!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ImExpectingGreatWork, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_4494:
    // "A head-count survey, right?[f000]븁\u0000\nThis is a survey to collect data for\na specified number of people.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_HeadCountSurveyRight, 0x8011, 2, 0
    VMReturn

L_44A2:
    // "A timed survey, right?[f000]븁\u0000\nThis is a survey to collect data\nfor a specified time.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_TimedSurveyRightSurvey, 0x8011, 2, 0
    VMReturn

L_44B0:
    WordSetPlayerName 1
    // "...Oh?[f000]븁\u0000\nYou've already got the data\nfor this survey![f000]븁\u0000\nI knew you could do it, [f000]Ā\u0001\u0001.\nPlease report the survey result to me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OhYouveAlreadyGot, 0x8011, 2, 0
    ActorMsgClose
    VMReturn

L_44C3:
    WordSetPlayerName 1
    // "...Oh?[f000]븁\u0000\nYou've passed by so many people already!\nI knew you could do it, [f000]Ā\u0001\u0001.[f000]븁\u0000\nI think the current data is enough.\nPlease report the survey results to me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OhYouvePassedBy, 0x8011, 2, 0
    ActorMsgClose
    VMReturn

L_44D6:
    TrainerGameInfoCmd_020B 0x8040
    WorkCmpConst 0x8040, 1
    VMJumpIf CMP_EQ, L_44ED
    VMJump L_44F9

L_44ED:
    WorkSetConst 0x8041, 245
    VMJump L_4575

L_44F9:
    WorkCmpConst 0x8040, 2
    VMJumpIf CMP_EQ, L_450C
    VMJump L_4518

L_450C:
    WorkSetConst 0x8041, 245
    VMJump L_4575

L_4518:
    WorkCmpConst 0x8040, 3
    VMJumpIf CMP_EQ, L_452B
    VMJump L_4537

L_452B:
    WorkSetConst 0x8041, 246
    VMJump L_4575

L_4537:
    WorkCmpConst 0x8040, 4
    VMJumpIf CMP_EQ, L_454A
    VMJump L_4556

L_454A:
    WorkSetConst 0x8041, 247
    VMJump L_4575

L_4556:
    WorkCmpConst 0x8040, 5
    VMJumpIf CMP_EQ, L_4569
    VMJump L_4575

L_4569:
    WorkSetConst 0x8041, 248
    VMJump L_4575

L_4575:
    ActorMsg MSGFILE_SCRIPT, 0x8041, 0x8011, 2, 0
    ActorMsgClose
    VMReturn

L_4585:
    TrainerGameInfoCmd_020B 0x8032
    WorkCmpConst 0x8032, 1
    VMJumpIf CMP_EQ, L_459C
    VMJump L_45A8

L_459C:
    WorkSetConst 0x8033, 253
    VMJump L_4624

L_45A8:
    WorkCmpConst 0x8032, 2
    VMJumpIf CMP_EQ, L_45BB
    VMJump L_45C7

L_45BB:
    WorkSetConst 0x8033, 254
    VMJump L_4624

L_45C7:
    WorkCmpConst 0x8032, 3
    VMJumpIf CMP_EQ, L_45DA
    VMJump L_45E6

L_45DA:
    WorkSetConst 0x8033, 255
    VMJump L_4624

L_45E6:
    WorkCmpConst 0x8032, 4
    VMJumpIf CMP_EQ, L_45F9
    VMJump L_4605

L_45F9:
    WorkSetConst 0x8033, 256
    VMJump L_4624

L_4605:
    WorkCmpConst 0x8032, 5
    VMJumpIf CMP_EQ, L_4618
    VMJump L_4624

L_4618:
    WorkSetConst 0x8033, 256
    VMJump L_4624

L_4624:
    VMCall L_0C58
    WordSetNumber 1, 0x8010, 2
    ActorMsg MSGFILE_SCRIPT, 0x8033, 0x8011, 2, 0
    VMReturn

L_463F:
    WorkGet 0x802b, 0x8008
    WorkCmpConst 0x802b, 24
    VMJumpIf CMP_EQ, L_4658
    VMJump L_4664

L_4658:
    WorkSetConst 0x802a, 76
    VMJump L_4BD9

L_4664:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_4677
    VMJump L_4683

L_4677:
    WorkSetConst 0x802a, 52
    VMJump L_4BD9

L_4683:
    WorkCmpConst 0x802b, 25
    VMJumpIf CMP_EQ, L_4696
    VMJump L_46A2

L_4696:
    WorkSetConst 0x802a, 77
    VMJump L_4BD9

L_46A2:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_46B5
    VMJump L_46C1

L_46B5:
    WorkSetConst 0x802a, 53
    VMJump L_4BD9

L_46C1:
    WorkCmpConst 0x802b, 26
    VMJumpIf CMP_EQ, L_46D4
    VMJump L_46E0

L_46D4:
    WorkSetConst 0x802a, 78
    VMJump L_4BD9

L_46E0:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_46F3
    VMJump L_46FF

L_46F3:
    WorkSetConst 0x802a, 54
    VMJump L_4BD9

L_46FF:
    WorkCmpConst 0x802b, 27
    VMJumpIf CMP_EQ, L_4712
    VMJump L_471E

L_4712:
    WorkSetConst 0x802a, 79
    VMJump L_4BD9

L_471E:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_4731
    VMJump L_473D

L_4731:
    WorkSetConst 0x802a, 55
    VMJump L_4BD9

L_473D:
    WorkCmpConst 0x802b, 28
    VMJumpIf CMP_EQ, L_4750
    VMJump L_475C

L_4750:
    WorkSetConst 0x802a, 80
    VMJump L_4BD9

L_475C:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_476F
    VMJump L_477B

L_476F:
    WorkSetConst 0x802a, 56
    VMJump L_4BD9

L_477B:
    WorkCmpConst 0x802b, 29
    VMJumpIf CMP_EQ, L_478E
    VMJump L_479A

L_478E:
    WorkSetConst 0x802a, 81
    VMJump L_4BD9

L_479A:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_47AD
    VMJump L_47B9

L_47AD:
    WorkSetConst 0x802a, 57
    VMJump L_4BD9

L_47B9:
    WorkCmpConst 0x802b, 30
    VMJumpIf CMP_EQ, L_47CC
    VMJump L_47D8

L_47CC:
    WorkSetConst 0x802a, 82
    VMJump L_4BD9

L_47D8:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_47EB
    VMJump L_47F7

L_47EB:
    WorkSetConst 0x802a, 58
    VMJump L_4BD9

L_47F7:
    WorkCmpConst 0x802b, 31
    VMJumpIf CMP_EQ, L_480A
    VMJump L_4816

L_480A:
    WorkSetConst 0x802a, 83
    VMJump L_4BD9

L_4816:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_4829
    VMJump L_4835

L_4829:
    WorkSetConst 0x802a, 59
    VMJump L_4BD9

L_4835:
    WorkCmpConst 0x802b, 32
    VMJumpIf CMP_EQ, L_4848
    VMJump L_4854

L_4848:
    WorkSetConst 0x802a, 84
    VMJump L_4BD9

L_4854:
    WorkCmpConst 0x802b, 9
    VMJumpIf CMP_EQ, L_4867
    VMJump L_4873

L_4867:
    WorkSetConst 0x802a, 60
    VMJump L_4BD9

L_4873:
    WorkCmpConst 0x802b, 33
    VMJumpIf CMP_EQ, L_4886
    VMJump L_4892

L_4886:
    WorkSetConst 0x802a, 85
    VMJump L_4BD9

L_4892:
    WorkCmpConst 0x802b, 10
    VMJumpIf CMP_EQ, L_48A5
    VMJump L_48B1

L_48A5:
    WorkSetConst 0x802a, 61
    VMJump L_4BD9

L_48B1:
    WorkCmpConst 0x802b, 34
    VMJumpIf CMP_EQ, L_48C4
    VMJump L_48D0

L_48C4:
    WorkSetConst 0x802a, 86
    VMJump L_4BD9

L_48D0:
    WorkCmpConst 0x802b, 11
    VMJumpIf CMP_EQ, L_48E3
    VMJump L_48EF

L_48E3:
    WorkSetConst 0x802a, 62
    VMJump L_4BD9

L_48EF:
    WorkCmpConst 0x802b, 35
    VMJumpIf CMP_EQ, L_4902
    VMJump L_490E

L_4902:
    WorkSetConst 0x802a, 87
    VMJump L_4BD9

L_490E:
    WorkCmpConst 0x802b, 12
    VMJumpIf CMP_EQ, L_4921
    VMJump L_492D

L_4921:
    WorkSetConst 0x802a, 63
    VMJump L_4BD9

L_492D:
    WorkCmpConst 0x802b, 36
    VMJumpIf CMP_EQ, L_4940
    VMJump L_494C

L_4940:
    WorkSetConst 0x802a, 88
    VMJump L_4BD9

L_494C:
    WorkCmpConst 0x802b, 13
    VMJumpIf CMP_EQ, L_495F
    VMJump L_496B

L_495F:
    WorkSetConst 0x802a, 64
    VMJump L_4BD9

L_496B:
    WorkCmpConst 0x802b, 37
    VMJumpIf CMP_EQ, L_497E
    VMJump L_498A

L_497E:
    WorkSetConst 0x802a, 89
    VMJump L_4BD9

L_498A:
    WorkCmpConst 0x802b, 14
    VMJumpIf CMP_EQ, L_499D
    VMJump L_49A9

L_499D:
    WorkSetConst 0x802a, 65
    VMJump L_4BD9

L_49A9:
    WorkCmpConst 0x802b, 38
    VMJumpIf CMP_EQ, L_49BC
    VMJump L_49C8

L_49BC:
    WorkSetConst 0x802a, 90
    VMJump L_4BD9

L_49C8:
    WorkCmpConst 0x802b, 15
    VMJumpIf CMP_EQ, L_49DB
    VMJump L_49E7

L_49DB:
    WorkSetConst 0x802a, 66
    VMJump L_4BD9

L_49E7:
    WorkCmpConst 0x802b, 39
    VMJumpIf CMP_EQ, L_49FA
    VMJump L_4A06

L_49FA:
    WorkSetConst 0x802a, 91
    VMJump L_4BD9

L_4A06:
    WorkCmpConst 0x802b, 16
    VMJumpIf CMP_EQ, L_4A19
    VMJump L_4A25

L_4A19:
    WorkSetConst 0x802a, 67
    VMJump L_4BD9

L_4A25:
    WorkCmpConst 0x802b, 40
    VMJumpIf CMP_EQ, L_4A38
    VMJump L_4A44

L_4A38:
    WorkSetConst 0x802a, 92
    VMJump L_4BD9

L_4A44:
    WorkCmpConst 0x802b, 17
    VMJumpIf CMP_EQ, L_4A57
    VMJump L_4A63

L_4A57:
    WorkSetConst 0x802a, 68
    VMJump L_4BD9

L_4A63:
    WorkCmpConst 0x802b, 41
    VMJumpIf CMP_EQ, L_4A76
    VMJump L_4A82

L_4A76:
    WorkSetConst 0x802a, 93
    VMJump L_4BD9

L_4A82:
    WorkCmpConst 0x802b, 18
    VMJumpIf CMP_EQ, L_4A95
    VMJump L_4AA1

L_4A95:
    WorkSetConst 0x802a, 69
    VMJump L_4BD9

L_4AA1:
    WorkCmpConst 0x802b, 42
    VMJumpIf CMP_EQ, L_4AB4
    VMJump L_4AC0

L_4AB4:
    WorkSetConst 0x802a, 94
    VMJump L_4BD9

L_4AC0:
    WorkCmpConst 0x802b, 19
    VMJumpIf CMP_EQ, L_4AD3
    VMJump L_4ADF

L_4AD3:
    WorkSetConst 0x802a, 70
    VMJump L_4BD9

L_4ADF:
    WorkCmpConst 0x802b, 43
    VMJumpIf CMP_EQ, L_4AF2
    VMJump L_4AFE

L_4AF2:
    WorkSetConst 0x802a, 95
    VMJump L_4BD9

L_4AFE:
    WorkCmpConst 0x802b, 20
    VMJumpIf CMP_EQ, L_4B11
    VMJump L_4B1D

L_4B11:
    WorkSetConst 0x802a, 71
    VMJump L_4BD9

L_4B1D:
    WorkCmpConst 0x802b, 44
    VMJumpIf CMP_EQ, L_4B30
    VMJump L_4B3C

L_4B30:
    WorkSetConst 0x802a, 96
    VMJump L_4BD9

L_4B3C:
    WorkCmpConst 0x802b, 21
    VMJumpIf CMP_EQ, L_4B4F
    VMJump L_4B5B

L_4B4F:
    WorkSetConst 0x802a, 72
    VMJump L_4BD9

L_4B5B:
    WorkCmpConst 0x802b, 45
    VMJumpIf CMP_EQ, L_4B6E
    VMJump L_4B7A

L_4B6E:
    WorkSetConst 0x802a, 97
    VMJump L_4BD9

L_4B7A:
    WorkCmpConst 0x802b, 22
    VMJumpIf CMP_EQ, L_4B8D
    VMJump L_4B99

L_4B8D:
    WorkSetConst 0x802a, 73
    VMJump L_4BD9

L_4B99:
    WorkCmpConst 0x802b, 46
    VMJumpIf CMP_EQ, L_4BAC
    VMJump L_4BB8

L_4BAC:
    WorkSetConst 0x802a, 98
    VMJump L_4BD9

L_4BB8:
    WorkCmpConst 0x802b, 23
    VMJumpIf CMP_EQ, L_4BCB
    VMJump L_4BD7

L_4BCB:
    WorkSetConst 0x802a, 74
    VMJump L_4BD9

L_4BD7:
    VMReturn

L_4BD9:
    ActorMsg MSGFILE_SCRIPT, 0x802a, 0x8011, 2, 0
    VMReturn

L_4BE7:
    SurveyGetCurrentQuestionID 0x802b
    WorkCmpConst 0x802b, 24
    VMJumpIf CMP_EQ, L_4BFE
    VMJump L_4C0A

L_4BFE:
    WorkSetConst 0x802a, 76
    VMJump L_517F

L_4C0A:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_4C1D
    VMJump L_4C29

L_4C1D:
    WorkSetConst 0x802a, 52
    VMJump L_517F

L_4C29:
    WorkCmpConst 0x802b, 25
    VMJumpIf CMP_EQ, L_4C3C
    VMJump L_4C48

L_4C3C:
    WorkSetConst 0x802a, 77
    VMJump L_517F

L_4C48:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_4C5B
    VMJump L_4C67

L_4C5B:
    WorkSetConst 0x802a, 53
    VMJump L_517F

L_4C67:
    WorkCmpConst 0x802b, 26
    VMJumpIf CMP_EQ, L_4C7A
    VMJump L_4C86

L_4C7A:
    WorkSetConst 0x802a, 78
    VMJump L_517F

L_4C86:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_4C99
    VMJump L_4CA5

L_4C99:
    WorkSetConst 0x802a, 54
    VMJump L_517F

L_4CA5:
    WorkCmpConst 0x802b, 27
    VMJumpIf CMP_EQ, L_4CB8
    VMJump L_4CC4

L_4CB8:
    WorkSetConst 0x802a, 79
    VMJump L_517F

L_4CC4:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_4CD7
    VMJump L_4CE3

L_4CD7:
    WorkSetConst 0x802a, 55
    VMJump L_517F

L_4CE3:
    WorkCmpConst 0x802b, 28
    VMJumpIf CMP_EQ, L_4CF6
    VMJump L_4D02

L_4CF6:
    WorkSetConst 0x802a, 80
    VMJump L_517F

L_4D02:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_4D15
    VMJump L_4D21

L_4D15:
    WorkSetConst 0x802a, 56
    VMJump L_517F

L_4D21:
    WorkCmpConst 0x802b, 29
    VMJumpIf CMP_EQ, L_4D34
    VMJump L_4D40

L_4D34:
    WorkSetConst 0x802a, 81
    VMJump L_517F

L_4D40:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_4D53
    VMJump L_4D5F

L_4D53:
    WorkSetConst 0x802a, 57
    VMJump L_517F

L_4D5F:
    WorkCmpConst 0x802b, 30
    VMJumpIf CMP_EQ, L_4D72
    VMJump L_4D7E

L_4D72:
    WorkSetConst 0x802a, 82
    VMJump L_517F

L_4D7E:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_4D91
    VMJump L_4D9D

L_4D91:
    WorkSetConst 0x802a, 58
    VMJump L_517F

L_4D9D:
    WorkCmpConst 0x802b, 31
    VMJumpIf CMP_EQ, L_4DB0
    VMJump L_4DBC

L_4DB0:
    WorkSetConst 0x802a, 83
    VMJump L_517F

L_4DBC:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_4DCF
    VMJump L_4DDB

L_4DCF:
    WorkSetConst 0x802a, 59
    VMJump L_517F

L_4DDB:
    WorkCmpConst 0x802b, 32
    VMJumpIf CMP_EQ, L_4DEE
    VMJump L_4DFA

L_4DEE:
    WorkSetConst 0x802a, 84
    VMJump L_517F

L_4DFA:
    WorkCmpConst 0x802b, 9
    VMJumpIf CMP_EQ, L_4E0D
    VMJump L_4E19

L_4E0D:
    WorkSetConst 0x802a, 60
    VMJump L_517F

L_4E19:
    WorkCmpConst 0x802b, 33
    VMJumpIf CMP_EQ, L_4E2C
    VMJump L_4E38

L_4E2C:
    WorkSetConst 0x802a, 85
    VMJump L_517F

L_4E38:
    WorkCmpConst 0x802b, 10
    VMJumpIf CMP_EQ, L_4E4B
    VMJump L_4E57

L_4E4B:
    WorkSetConst 0x802a, 61
    VMJump L_517F

L_4E57:
    WorkCmpConst 0x802b, 34
    VMJumpIf CMP_EQ, L_4E6A
    VMJump L_4E76

L_4E6A:
    WorkSetConst 0x802a, 86
    VMJump L_517F

L_4E76:
    WorkCmpConst 0x802b, 11
    VMJumpIf CMP_EQ, L_4E89
    VMJump L_4E95

L_4E89:
    WorkSetConst 0x802a, 62
    VMJump L_517F

L_4E95:
    WorkCmpConst 0x802b, 35
    VMJumpIf CMP_EQ, L_4EA8
    VMJump L_4EB4

L_4EA8:
    WorkSetConst 0x802a, 87
    VMJump L_517F

L_4EB4:
    WorkCmpConst 0x802b, 12
    VMJumpIf CMP_EQ, L_4EC7
    VMJump L_4ED3

L_4EC7:
    WorkSetConst 0x802a, 63
    VMJump L_517F

L_4ED3:
    WorkCmpConst 0x802b, 36
    VMJumpIf CMP_EQ, L_4EE6
    VMJump L_4EF2

L_4EE6:
    WorkSetConst 0x802a, 88
    VMJump L_517F

L_4EF2:
    WorkCmpConst 0x802b, 13
    VMJumpIf CMP_EQ, L_4F05
    VMJump L_4F11

L_4F05:
    WorkSetConst 0x802a, 64
    VMJump L_517F

L_4F11:
    WorkCmpConst 0x802b, 37
    VMJumpIf CMP_EQ, L_4F24
    VMJump L_4F30

L_4F24:
    WorkSetConst 0x802a, 89
    VMJump L_517F

L_4F30:
    WorkCmpConst 0x802b, 14
    VMJumpIf CMP_EQ, L_4F43
    VMJump L_4F4F

L_4F43:
    WorkSetConst 0x802a, 65
    VMJump L_517F

L_4F4F:
    WorkCmpConst 0x802b, 38
    VMJumpIf CMP_EQ, L_4F62
    VMJump L_4F6E

L_4F62:
    WorkSetConst 0x802a, 90
    VMJump L_517F

L_4F6E:
    WorkCmpConst 0x802b, 15
    VMJumpIf CMP_EQ, L_4F81
    VMJump L_4F8D

L_4F81:
    WorkSetConst 0x802a, 66
    VMJump L_517F

L_4F8D:
    WorkCmpConst 0x802b, 39
    VMJumpIf CMP_EQ, L_4FA0
    VMJump L_4FAC

L_4FA0:
    WorkSetConst 0x802a, 91
    VMJump L_517F

L_4FAC:
    WorkCmpConst 0x802b, 16
    VMJumpIf CMP_EQ, L_4FBF
    VMJump L_4FCB

L_4FBF:
    WorkSetConst 0x802a, 67
    VMJump L_517F

L_4FCB:
    WorkCmpConst 0x802b, 40
    VMJumpIf CMP_EQ, L_4FDE
    VMJump L_4FEA

L_4FDE:
    WorkSetConst 0x802a, 92
    VMJump L_517F

L_4FEA:
    WorkCmpConst 0x802b, 17
    VMJumpIf CMP_EQ, L_4FFD
    VMJump L_5009

L_4FFD:
    WorkSetConst 0x802a, 68
    VMJump L_517F

L_5009:
    WorkCmpConst 0x802b, 41
    VMJumpIf CMP_EQ, L_501C
    VMJump L_5028

L_501C:
    WorkSetConst 0x802a, 93
    VMJump L_517F

L_5028:
    WorkCmpConst 0x802b, 18
    VMJumpIf CMP_EQ, L_503B
    VMJump L_5047

L_503B:
    WorkSetConst 0x802a, 69
    VMJump L_517F

L_5047:
    WorkCmpConst 0x802b, 42
    VMJumpIf CMP_EQ, L_505A
    VMJump L_5066

L_505A:
    WorkSetConst 0x802a, 94
    VMJump L_517F

L_5066:
    WorkCmpConst 0x802b, 19
    VMJumpIf CMP_EQ, L_5079
    VMJump L_5085

L_5079:
    WorkSetConst 0x802a, 70
    VMJump L_517F

L_5085:
    WorkCmpConst 0x802b, 43
    VMJumpIf CMP_EQ, L_5098
    VMJump L_50A4

L_5098:
    WorkSetConst 0x802a, 95
    VMJump L_517F

L_50A4:
    WorkCmpConst 0x802b, 20
    VMJumpIf CMP_EQ, L_50B7
    VMJump L_50C3

L_50B7:
    WorkSetConst 0x802a, 71
    VMJump L_517F

L_50C3:
    WorkCmpConst 0x802b, 44
    VMJumpIf CMP_EQ, L_50D6
    VMJump L_50E2

L_50D6:
    WorkSetConst 0x802a, 96
    VMJump L_517F

L_50E2:
    WorkCmpConst 0x802b, 21
    VMJumpIf CMP_EQ, L_50F5
    VMJump L_5101

L_50F5:
    WorkSetConst 0x802a, 72
    VMJump L_517F

L_5101:
    WorkCmpConst 0x802b, 45
    VMJumpIf CMP_EQ, L_5114
    VMJump L_5120

L_5114:
    WorkSetConst 0x802a, 97
    VMJump L_517F

L_5120:
    WorkCmpConst 0x802b, 22
    VMJumpIf CMP_EQ, L_5133
    VMJump L_513F

L_5133:
    WorkSetConst 0x802a, 73
    VMJump L_517F

L_513F:
    WorkCmpConst 0x802b, 46
    VMJumpIf CMP_EQ, L_5152
    VMJump L_515E

L_5152:
    WorkSetConst 0x802a, 98
    VMJump L_517F

L_515E:
    WorkCmpConst 0x802b, 23
    VMJumpIf CMP_EQ, L_5171
    VMJump L_517D

L_5171:
    WorkSetConst 0x802a, 74
    VMJump L_517F

L_517D:
    VMReturn

L_517F:
    ActorMsg MSGFILE_SCRIPT, 0x802a, 0x8011, 2, 0
    VMReturn

L_518D:
    SurveyGetCurrentQuestionID 0x802b
    SurveyGetCurrentAnswerIDs 0x802c, 0x802d, 0x802e
    VMStackPush 0x802c
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_51B2
    SurveyGetPopularOptionMsgID 0x802c, 0x802f

L_51B2:
    VMStackPush 0x802d
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_51CB
    SurveyGetPopularOptionMsgID 0x802d, 0x8030

L_51CB:
    VMStackPush 0x802e
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_51E4
    SurveyGetPopularOptionMsgID 0x802e, 0x8031

L_51E4:
    WorkCmpConst 0x802b, 24
    VMJumpIf CMP_EQ, L_51F7
    VMJump L_5203

L_51F7:
    WorkSetConst 0x802a, 221
    VMJump L_5778

L_5203:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_5216
    VMJump L_5222

L_5216:
    WorkSetConst 0x802a, 221
    VMJump L_5778

L_5222:
    WorkCmpConst 0x802b, 25
    VMJumpIf CMP_EQ, L_5235
    VMJump L_5241

L_5235:
    WorkSetConst 0x802a, 222
    VMJump L_5778

L_5241:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_5254
    VMJump L_5260

L_5254:
    WorkSetConst 0x802a, 222
    VMJump L_5778

L_5260:
    WorkCmpConst 0x802b, 26
    VMJumpIf CMP_EQ, L_5273
    VMJump L_527F

L_5273:
    WorkSetConst 0x802a, 223
    VMJump L_5778

L_527F:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_5292
    VMJump L_529E

L_5292:
    WorkSetConst 0x802a, 223
    VMJump L_5778

L_529E:
    WorkCmpConst 0x802b, 27
    VMJumpIf CMP_EQ, L_52B1
    VMJump L_52BD

L_52B1:
    WorkSetConst 0x802a, 224
    VMJump L_5778

L_52BD:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_52D0
    VMJump L_52DC

L_52D0:
    WorkSetConst 0x802a, 224
    VMJump L_5778

L_52DC:
    WorkCmpConst 0x802b, 28
    VMJumpIf CMP_EQ, L_52EF
    VMJump L_52FB

L_52EF:
    WorkSetConst 0x802a, 225
    VMJump L_5778

L_52FB:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_530E
    VMJump L_531A

L_530E:
    WorkSetConst 0x802a, 225
    VMJump L_5778

L_531A:
    WorkCmpConst 0x802b, 29
    VMJumpIf CMP_EQ, L_532D
    VMJump L_5339

L_532D:
    WorkSetConst 0x802a, 226
    VMJump L_5778

L_5339:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_534C
    VMJump L_5358

L_534C:
    WorkSetConst 0x802a, 226
    VMJump L_5778

L_5358:
    WorkCmpConst 0x802b, 30
    VMJumpIf CMP_EQ, L_536B
    VMJump L_5377

L_536B:
    WorkSetConst 0x802a, 227
    VMJump L_5778

L_5377:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_538A
    VMJump L_5396

L_538A:
    WorkSetConst 0x802a, 227
    VMJump L_5778

L_5396:
    WorkCmpConst 0x802b, 31
    VMJumpIf CMP_EQ, L_53A9
    VMJump L_53B5

L_53A9:
    WorkSetConst 0x802a, 228
    VMJump L_5778

L_53B5:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_53C8
    VMJump L_53D4

L_53C8:
    WorkSetConst 0x802a, 228
    VMJump L_5778

L_53D4:
    WorkCmpConst 0x802b, 32
    VMJumpIf CMP_EQ, L_53E7
    VMJump L_53F3

L_53E7:
    WorkSetConst 0x802a, 229
    VMJump L_5778

L_53F3:
    WorkCmpConst 0x802b, 9
    VMJumpIf CMP_EQ, L_5406
    VMJump L_5412

L_5406:
    WorkSetConst 0x802a, 229
    VMJump L_5778

L_5412:
    WorkCmpConst 0x802b, 33
    VMJumpIf CMP_EQ, L_5425
    VMJump L_5431

L_5425:
    WorkSetConst 0x802a, 230
    VMJump L_5778

L_5431:
    WorkCmpConst 0x802b, 10
    VMJumpIf CMP_EQ, L_5444
    VMJump L_5450

L_5444:
    WorkSetConst 0x802a, 230
    VMJump L_5778

L_5450:
    WorkCmpConst 0x802b, 34
    VMJumpIf CMP_EQ, L_5463
    VMJump L_546F

L_5463:
    WorkSetConst 0x802a, 231
    VMJump L_5778

L_546F:
    WorkCmpConst 0x802b, 11
    VMJumpIf CMP_EQ, L_5482
    VMJump L_548E

L_5482:
    WorkSetConst 0x802a, 231
    VMJump L_5778

L_548E:
    WorkCmpConst 0x802b, 35
    VMJumpIf CMP_EQ, L_54A1
    VMJump L_54AD

L_54A1:
    WorkSetConst 0x802a, 232
    VMJump L_5778

L_54AD:
    WorkCmpConst 0x802b, 12
    VMJumpIf CMP_EQ, L_54C0
    VMJump L_54CC

L_54C0:
    WorkSetConst 0x802a, 232
    VMJump L_5778

L_54CC:
    WorkCmpConst 0x802b, 36
    VMJumpIf CMP_EQ, L_54DF
    VMJump L_54EB

L_54DF:
    WorkSetConst 0x802a, 233
    VMJump L_5778

L_54EB:
    WorkCmpConst 0x802b, 13
    VMJumpIf CMP_EQ, L_54FE
    VMJump L_550A

L_54FE:
    WorkSetConst 0x802a, 233
    VMJump L_5778

L_550A:
    WorkCmpConst 0x802b, 37
    VMJumpIf CMP_EQ, L_551D
    VMJump L_5529

L_551D:
    WorkSetConst 0x802a, 234
    VMJump L_5778

L_5529:
    WorkCmpConst 0x802b, 14
    VMJumpIf CMP_EQ, L_553C
    VMJump L_5548

L_553C:
    WorkSetConst 0x802a, 234
    VMJump L_5778

L_5548:
    WorkCmpConst 0x802b, 38
    VMJumpIf CMP_EQ, L_555B
    VMJump L_5567

L_555B:
    WorkSetConst 0x802a, 235
    VMJump L_5778

L_5567:
    WorkCmpConst 0x802b, 15
    VMJumpIf CMP_EQ, L_557A
    VMJump L_5586

L_557A:
    WorkSetConst 0x802a, 235
    VMJump L_5778

L_5586:
    WorkCmpConst 0x802b, 39
    VMJumpIf CMP_EQ, L_5599
    VMJump L_55A5

L_5599:
    WorkSetConst 0x802a, 236
    VMJump L_5778

L_55A5:
    WorkCmpConst 0x802b, 16
    VMJumpIf CMP_EQ, L_55B8
    VMJump L_55C4

L_55B8:
    WorkSetConst 0x802a, 236
    VMJump L_5778

L_55C4:
    WorkCmpConst 0x802b, 40
    VMJumpIf CMP_EQ, L_55D7
    VMJump L_55E3

L_55D7:
    WorkSetConst 0x802a, 237
    VMJump L_5778

L_55E3:
    WorkCmpConst 0x802b, 17
    VMJumpIf CMP_EQ, L_55F6
    VMJump L_5602

L_55F6:
    WorkSetConst 0x802a, 237
    VMJump L_5778

L_5602:
    WorkCmpConst 0x802b, 41
    VMJumpIf CMP_EQ, L_5615
    VMJump L_5621

L_5615:
    WorkSetConst 0x802a, 238
    VMJump L_5778

L_5621:
    WorkCmpConst 0x802b, 18
    VMJumpIf CMP_EQ, L_5634
    VMJump L_5640

L_5634:
    WorkSetConst 0x802a, 238
    VMJump L_5778

L_5640:
    WorkCmpConst 0x802b, 42
    VMJumpIf CMP_EQ, L_5653
    VMJump L_565F

L_5653:
    WorkSetConst 0x802a, 239
    VMJump L_5778

L_565F:
    WorkCmpConst 0x802b, 19
    VMJumpIf CMP_EQ, L_5672
    VMJump L_567E

L_5672:
    WorkSetConst 0x802a, 239
    VMJump L_5778

L_567E:
    WorkCmpConst 0x802b, 43
    VMJumpIf CMP_EQ, L_5691
    VMJump L_569D

L_5691:
    WorkSetConst 0x802a, 240
    VMJump L_5778

L_569D:
    WorkCmpConst 0x802b, 20
    VMJumpIf CMP_EQ, L_56B0
    VMJump L_56BC

L_56B0:
    WorkSetConst 0x802a, 240
    VMJump L_5778

L_56BC:
    WorkCmpConst 0x802b, 44
    VMJumpIf CMP_EQ, L_56CF
    VMJump L_56DB

L_56CF:
    WorkSetConst 0x802a, 241
    VMJump L_5778

L_56DB:
    WorkCmpConst 0x802b, 21
    VMJumpIf CMP_EQ, L_56EE
    VMJump L_56FA

L_56EE:
    WorkSetConst 0x802a, 241
    VMJump L_5778

L_56FA:
    WorkCmpConst 0x802b, 45
    VMJumpIf CMP_EQ, L_570D
    VMJump L_5719

L_570D:
    WorkSetConst 0x802a, 242
    VMJump L_5778

L_5719:
    WorkCmpConst 0x802b, 22
    VMJumpIf CMP_EQ, L_572C
    VMJump L_5738

L_572C:
    WorkSetConst 0x802a, 242
    VMJump L_5778

L_5738:
    WorkCmpConst 0x802b, 46
    VMJumpIf CMP_EQ, L_574B
    VMJump L_5757

L_574B:
    WorkSetConst 0x802a, 243
    VMJump L_5778

L_5757:
    WorkCmpConst 0x802b, 23
    VMJumpIf CMP_EQ, L_576A
    VMJump L_5776

L_576A:
    WorkSetConst 0x802a, 243
    VMJump L_5778

L_5776:
    VMReturn

L_5778:
    VMStackPush 0x802c
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_5790
    WordSetSurveyAnswer 0, 0x802f

L_5790:
    VMStackPush 0x802d
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_57A8
    WordSetSurveyAnswer 1, 0x8030

L_57A8:
    VMStackPush 0x802e
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_57C0
    WordSetSurveyAnswer 2, 0x8031

L_57C0:
    ActorMsg MSGFILE_SCRIPT, 0x802a, 0x8011, 2, 0
    VMReturn

L_57CE:
    WorkGet 0x802e, 0x8008
    SurveyGetCurrentQuestionID 0x802b
    TrainerCardCmd_0203 0x802d
    WorkCmpConst 0x802e, 2
    VMJumpIf CMP_EQ, L_57EF
    VMJump L_57FC

L_57EF:
    WordSetNumber 1, 0x802d, 3
    VMJump L_5837

L_57FC:
    WorkCmpConst 0x802e, 3
    VMJumpIf CMP_EQ, L_580F
    VMJump L_581C

L_580F:
    WordSetNumber 2, 0x802d, 2
    VMJump L_5837

L_581C:
    WorkCmpConst 0x802e, 4
    VMJumpIf CMP_EQ, L_582F
    VMJump L_5835

L_582F:
    VMJump L_5837

L_5835:
    VMReturn

L_5837:
    WorkCmpConst 0x802e, 2
    VMJumpIf CMP_EQ, L_584A
    VMJump L_5B1B

L_584A:
    WorkCmpConst 0x802b, 24
    VMJumpIf CMP_EQ, L_585D
    VMJump L_5869

L_585D:
    WorkSetConst 0x802a, 174
    VMJump L_5B15

L_5869:
    WorkCmpConst 0x802b, 25
    VMJumpIf CMP_EQ, L_587C
    VMJump L_5888

L_587C:
    WorkSetConst 0x802a, 175
    VMJump L_5B15

L_5888:
    WorkCmpConst 0x802b, 26
    VMJumpIf CMP_EQ, L_589B
    VMJump L_58A7

L_589B:
    WorkSetConst 0x802a, 176
    VMJump L_5B15

L_58A7:
    WorkCmpConst 0x802b, 27
    VMJumpIf CMP_EQ, L_58BA
    VMJump L_58C6

L_58BA:
    WorkSetConst 0x802a, 177
    VMJump L_5B15

L_58C6:
    WorkCmpConst 0x802b, 28
    VMJumpIf CMP_EQ, L_58D9
    VMJump L_58E5

L_58D9:
    WorkSetConst 0x802a, 178
    VMJump L_5B15

L_58E5:
    WorkCmpConst 0x802b, 29
    VMJumpIf CMP_EQ, L_58F8
    VMJump L_5904

L_58F8:
    WorkSetConst 0x802a, 179
    VMJump L_5B15

L_5904:
    WorkCmpConst 0x802b, 30
    VMJumpIf CMP_EQ, L_5917
    VMJump L_5923

L_5917:
    WorkSetConst 0x802a, 180
    VMJump L_5B15

L_5923:
    WorkCmpConst 0x802b, 31
    VMJumpIf CMP_EQ, L_5936
    VMJump L_5942

L_5936:
    WorkSetConst 0x802a, 181
    VMJump L_5B15

L_5942:
    WorkCmpConst 0x802b, 32
    VMJumpIf CMP_EQ, L_5955
    VMJump L_5961

L_5955:
    WorkSetConst 0x802a, 182
    VMJump L_5B15

L_5961:
    WorkCmpConst 0x802b, 33
    VMJumpIf CMP_EQ, L_5974
    VMJump L_5980

L_5974:
    WorkSetConst 0x802a, 183
    VMJump L_5B15

L_5980:
    WorkCmpConst 0x802b, 34
    VMJumpIf CMP_EQ, L_5993
    VMJump L_599F

L_5993:
    WorkSetConst 0x802a, 184
    VMJump L_5B15

L_599F:
    WorkCmpConst 0x802b, 35
    VMJumpIf CMP_EQ, L_59B2
    VMJump L_59BE

L_59B2:
    WorkSetConst 0x802a, 185
    VMJump L_5B15

L_59BE:
    WorkCmpConst 0x802b, 36
    VMJumpIf CMP_EQ, L_59D1
    VMJump L_59DD

L_59D1:
    WorkSetConst 0x802a, 186
    VMJump L_5B15

L_59DD:
    WorkCmpConst 0x802b, 37
    VMJumpIf CMP_EQ, L_59F0
    VMJump L_59FC

L_59F0:
    WorkSetConst 0x802a, 187
    VMJump L_5B15

L_59FC:
    WorkCmpConst 0x802b, 38
    VMJumpIf CMP_EQ, L_5A0F
    VMJump L_5A1B

L_5A0F:
    WorkSetConst 0x802a, 188
    VMJump L_5B15

L_5A1B:
    WorkCmpConst 0x802b, 39
    VMJumpIf CMP_EQ, L_5A2E
    VMJump L_5A3A

L_5A2E:
    WorkSetConst 0x802a, 189
    VMJump L_5B15

L_5A3A:
    WorkCmpConst 0x802b, 40
    VMJumpIf CMP_EQ, L_5A4D
    VMJump L_5A59

L_5A4D:
    WorkSetConst 0x802a, 190
    VMJump L_5B15

L_5A59:
    WorkCmpConst 0x802b, 41
    VMJumpIf CMP_EQ, L_5A6C
    VMJump L_5A78

L_5A6C:
    WorkSetConst 0x802a, 191
    VMJump L_5B15

L_5A78:
    WorkCmpConst 0x802b, 42
    VMJumpIf CMP_EQ, L_5A8B
    VMJump L_5A97

L_5A8B:
    WorkSetConst 0x802a, 192
    VMJump L_5B15

L_5A97:
    WorkCmpConst 0x802b, 43
    VMJumpIf CMP_EQ, L_5AAA
    VMJump L_5AB6

L_5AAA:
    WorkSetConst 0x802a, 193
    VMJump L_5B15

L_5AB6:
    WorkCmpConst 0x802b, 44
    VMJumpIf CMP_EQ, L_5AC9
    VMJump L_5AD5

L_5AC9:
    WorkSetConst 0x802a, 194
    VMJump L_5B15

L_5AD5:
    WorkCmpConst 0x802b, 45
    VMJumpIf CMP_EQ, L_5AE8
    VMJump L_5AF4

L_5AE8:
    WorkSetConst 0x802a, 195
    VMJump L_5B15

L_5AF4:
    WorkCmpConst 0x802b, 46
    VMJumpIf CMP_EQ, L_5B07
    VMJump L_5B13

L_5B07:
    WorkSetConst 0x802a, 196
    VMJump L_5B15

L_5B13:
    VMReturn

L_5B15:
    VMJump L_60E5

L_5B1B:
    WorkCmpConst 0x802e, 3
    VMJumpIf CMP_EQ, L_5B2E
    VMJump L_5DFF

L_5B2E:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_5B41
    VMJump L_5B4D

L_5B41:
    WorkSetConst 0x802a, 128
    VMJump L_5DF9

L_5B4D:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_5B60
    VMJump L_5B6C

L_5B60:
    WorkSetConst 0x802a, 129
    VMJump L_5DF9

L_5B6C:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_5B7F
    VMJump L_5B8B

L_5B7F:
    WorkSetConst 0x802a, 130
    VMJump L_5DF9

L_5B8B:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_5B9E
    VMJump L_5BAA

L_5B9E:
    WorkSetConst 0x802a, 131
    VMJump L_5DF9

L_5BAA:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_5BBD
    VMJump L_5BC9

L_5BBD:
    WorkSetConst 0x802a, 132
    VMJump L_5DF9

L_5BC9:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_5BDC
    VMJump L_5BE8

L_5BDC:
    WorkSetConst 0x802a, 133
    VMJump L_5DF9

L_5BE8:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_5BFB
    VMJump L_5C07

L_5BFB:
    WorkSetConst 0x802a, 134
    VMJump L_5DF9

L_5C07:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_5C1A
    VMJump L_5C26

L_5C1A:
    WorkSetConst 0x802a, 135
    VMJump L_5DF9

L_5C26:
    WorkCmpConst 0x802b, 9
    VMJumpIf CMP_EQ, L_5C39
    VMJump L_5C45

L_5C39:
    WorkSetConst 0x802a, 136
    VMJump L_5DF9

L_5C45:
    WorkCmpConst 0x802b, 10
    VMJumpIf CMP_EQ, L_5C58
    VMJump L_5C64

L_5C58:
    WorkSetConst 0x802a, 137
    VMJump L_5DF9

L_5C64:
    WorkCmpConst 0x802b, 11
    VMJumpIf CMP_EQ, L_5C77
    VMJump L_5C83

L_5C77:
    WorkSetConst 0x802a, 138
    VMJump L_5DF9

L_5C83:
    WorkCmpConst 0x802b, 12
    VMJumpIf CMP_EQ, L_5C96
    VMJump L_5CA2

L_5C96:
    WorkSetConst 0x802a, 139
    VMJump L_5DF9

L_5CA2:
    WorkCmpConst 0x802b, 13
    VMJumpIf CMP_EQ, L_5CB5
    VMJump L_5CC1

L_5CB5:
    WorkSetConst 0x802a, 140
    VMJump L_5DF9

L_5CC1:
    WorkCmpConst 0x802b, 14
    VMJumpIf CMP_EQ, L_5CD4
    VMJump L_5CE0

L_5CD4:
    WorkSetConst 0x802a, 141
    VMJump L_5DF9

L_5CE0:
    WorkCmpConst 0x802b, 15
    VMJumpIf CMP_EQ, L_5CF3
    VMJump L_5CFF

L_5CF3:
    WorkSetConst 0x802a, 142
    VMJump L_5DF9

L_5CFF:
    WorkCmpConst 0x802b, 16
    VMJumpIf CMP_EQ, L_5D12
    VMJump L_5D1E

L_5D12:
    WorkSetConst 0x802a, 143
    VMJump L_5DF9

L_5D1E:
    WorkCmpConst 0x802b, 17
    VMJumpIf CMP_EQ, L_5D31
    VMJump L_5D3D

L_5D31:
    WorkSetConst 0x802a, 144
    VMJump L_5DF9

L_5D3D:
    WorkCmpConst 0x802b, 18
    VMJumpIf CMP_EQ, L_5D50
    VMJump L_5D5C

L_5D50:
    WorkSetConst 0x802a, 145
    VMJump L_5DF9

L_5D5C:
    WorkCmpConst 0x802b, 19
    VMJumpIf CMP_EQ, L_5D6F
    VMJump L_5D7B

L_5D6F:
    WorkSetConst 0x802a, 146
    VMJump L_5DF9

L_5D7B:
    WorkCmpConst 0x802b, 20
    VMJumpIf CMP_EQ, L_5D8E
    VMJump L_5D9A

L_5D8E:
    WorkSetConst 0x802a, 147
    VMJump L_5DF9

L_5D9A:
    WorkCmpConst 0x802b, 21
    VMJumpIf CMP_EQ, L_5DAD
    VMJump L_5DB9

L_5DAD:
    WorkSetConst 0x802a, 148
    VMJump L_5DF9

L_5DB9:
    WorkCmpConst 0x802b, 22
    VMJumpIf CMP_EQ, L_5DCC
    VMJump L_5DD8

L_5DCC:
    WorkSetConst 0x802a, 149
    VMJump L_5DF9

L_5DD8:
    WorkCmpConst 0x802b, 23
    VMJumpIf CMP_EQ, L_5DEB
    VMJump L_5DF7

L_5DEB:
    WorkSetConst 0x802a, 150
    VMJump L_5DF9

L_5DF7:
    VMReturn

L_5DF9:
    VMJump L_60E5

L_5DFF:
    WorkCmpConst 0x802e, 4
    VMJumpIf CMP_EQ, L_5E12
    VMJump L_60E3

L_5E12:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_5E25
    VMJump L_5E31

L_5E25:
    WorkSetConst 0x802a, 151
    VMJump L_60DD

L_5E31:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_5E44
    VMJump L_5E50

L_5E44:
    WorkSetConst 0x802a, 152
    VMJump L_60DD

L_5E50:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_5E63
    VMJump L_5E6F

L_5E63:
    WorkSetConst 0x802a, 153
    VMJump L_60DD

L_5E6F:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_5E82
    VMJump L_5E8E

L_5E82:
    WorkSetConst 0x802a, 154
    VMJump L_60DD

L_5E8E:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_5EA1
    VMJump L_5EAD

L_5EA1:
    WorkSetConst 0x802a, 155
    VMJump L_60DD

L_5EAD:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_5EC0
    VMJump L_5ECC

L_5EC0:
    WorkSetConst 0x802a, 156
    VMJump L_60DD

L_5ECC:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_5EDF
    VMJump L_5EEB

L_5EDF:
    WorkSetConst 0x802a, 157
    VMJump L_60DD

L_5EEB:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_5EFE
    VMJump L_5F0A

L_5EFE:
    WorkSetConst 0x802a, 158
    VMJump L_60DD

L_5F0A:
    WorkCmpConst 0x802b, 9
    VMJumpIf CMP_EQ, L_5F1D
    VMJump L_5F29

L_5F1D:
    WorkSetConst 0x802a, 159
    VMJump L_60DD

L_5F29:
    WorkCmpConst 0x802b, 10
    VMJumpIf CMP_EQ, L_5F3C
    VMJump L_5F48

L_5F3C:
    WorkSetConst 0x802a, 160
    VMJump L_60DD

L_5F48:
    WorkCmpConst 0x802b, 11
    VMJumpIf CMP_EQ, L_5F5B
    VMJump L_5F67

L_5F5B:
    WorkSetConst 0x802a, 161
    VMJump L_60DD

L_5F67:
    WorkCmpConst 0x802b, 12
    VMJumpIf CMP_EQ, L_5F7A
    VMJump L_5F86

L_5F7A:
    WorkSetConst 0x802a, 162
    VMJump L_60DD

L_5F86:
    WorkCmpConst 0x802b, 13
    VMJumpIf CMP_EQ, L_5F99
    VMJump L_5FA5

L_5F99:
    WorkSetConst 0x802a, 163
    VMJump L_60DD

L_5FA5:
    WorkCmpConst 0x802b, 14
    VMJumpIf CMP_EQ, L_5FB8
    VMJump L_5FC4

L_5FB8:
    WorkSetConst 0x802a, 164
    VMJump L_60DD

L_5FC4:
    WorkCmpConst 0x802b, 15
    VMJumpIf CMP_EQ, L_5FD7
    VMJump L_5FE3

L_5FD7:
    WorkSetConst 0x802a, 165
    VMJump L_60DD

L_5FE3:
    WorkCmpConst 0x802b, 16
    VMJumpIf CMP_EQ, L_5FF6
    VMJump L_6002

L_5FF6:
    WorkSetConst 0x802a, 166
    VMJump L_60DD

L_6002:
    WorkCmpConst 0x802b, 17
    VMJumpIf CMP_EQ, L_6015
    VMJump L_6021

L_6015:
    WorkSetConst 0x802a, 167
    VMJump L_60DD

L_6021:
    WorkCmpConst 0x802b, 18
    VMJumpIf CMP_EQ, L_6034
    VMJump L_6040

L_6034:
    WorkSetConst 0x802a, 168
    VMJump L_60DD

L_6040:
    WorkCmpConst 0x802b, 19
    VMJumpIf CMP_EQ, L_6053
    VMJump L_605F

L_6053:
    WorkSetConst 0x802a, 169
    VMJump L_60DD

L_605F:
    WorkCmpConst 0x802b, 20
    VMJumpIf CMP_EQ, L_6072
    VMJump L_607E

L_6072:
    WorkSetConst 0x802a, 170
    VMJump L_60DD

L_607E:
    WorkCmpConst 0x802b, 21
    VMJumpIf CMP_EQ, L_6091
    VMJump L_609D

L_6091:
    WorkSetConst 0x802a, 171
    VMJump L_60DD

L_609D:
    WorkCmpConst 0x802b, 22
    VMJumpIf CMP_EQ, L_60B0
    VMJump L_60BC

L_60B0:
    WorkSetConst 0x802a, 172
    VMJump L_60DD

L_60BC:
    WorkCmpConst 0x802b, 23
    VMJumpIf CMP_EQ, L_60CF
    VMJump L_60DB

L_60CF:
    WorkSetConst 0x802a, 173
    VMJump L_60DD

L_60DB:
    VMReturn

L_60DD:
    VMJump L_60E5

L_60E3:
    VMReturn

L_60E5:
    ActorMsg MSGFILE_SCRIPT, 0x802a, 0x8011, 2, 0
    WorkSetConst 0x8008, 1
    VMCall L_017A
    VMReturn

Script_2:
    ActorsPauseAll
    ActorCmdExec 10, Movement_7940
    ActorCmdWait
    VMCall L_791B
    ActorCmdExec 255, Movement_794C
    ActorCmdWait
    ActorGetGPos 255, 0x8008, 0x8009
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_6136
    VMJump L_6144

L_6136:
    ActorCmdExec 255, Movement_7958
    VMJump L_618E

L_6144:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_6157
    VMJump L_6165

L_6157:
    ActorCmdExec 255, Movement_7964
    VMJump L_618E

L_6165:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_6178
    VMJump L_6186

L_6178:
    ActorCmdExec 255, Movement_7978
    VMJump L_618E

L_6186:
    ActorCmdExec 255, Movement_7958

L_618E:
    ActorCmdWait
    ActorCmdExec 10, Movement_798C
    ActorCmdWait
    VMCall L_61E5
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    FlagGet EVENT_FLAG_0x00cc, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_61D3
    VMCall L_730F
    VMJump L_61DF

L_61D3:
    VMCall L_7321
    VMCall L_61E5

L_61DF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_61E5:
    FlagGet EVENT_FLAG_0x00cd, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6214
    FlagSet EVENT_FLAG_0x00cd
    VMCall L_01B8
    VMCall L_7332
    VMJump L_6235

L_6214:
    VMCall L_7340
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6235
    VMCall L_7352
    VMReturn

L_6235:
    WorkSetConst 0x8029, 1

L_623B:
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_634E
    VMCall L_6356
    WorkGet 0x8028, 0x8010
    VMStackPush 0x8028
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6275
    VMCall L_739C
    VMReturn

L_6275:
    WorkGet 0x8008, 0x8028
    VMCall L_73AE
    WorkGet 0x8008, 0x8028
    VMCall L_6403
    WorkGet 0x8008, 0x8028
    VMCall L_648C
    WorkGet 0x8008, 0x8028
    VMCall L_64FE
    VMCall L_78A0
    WorkGet 0x8008, 0x8028
    VMCall L_687C
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_62D2
    VMCall L_78DC
    VMReturn

L_62D2:
    VMCall L_78AE
    VMCall L_71E8
    WorkGet 0x8008, 0x8028
    VMCall L_668C
    VMCall L_684F
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6323
    VMCall L_78CE
    VMCall L_78BE
    WorkGet 0x8008, 0x8028
    VMCall L_6551
    VMCall L_78DC
    VMReturn

L_6323:
    VMCall L_78EE
    VMCall L_792B
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6348
    WorkSetConst 0x8029, 0

L_6348:
    VMJump L_623B

L_634E:
    VMCall L_7909
    VMReturn

L_6356:
    VMCall L_684F
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6377
    VMCall L_6ABC
    VMReturn

L_6377:
    VMCall L_7364
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6398
    WorkSetConst 0x8010, 9
    VMReturn

L_6398:
    VMCall L_7376
    WorkGet 0x8032, 0x8010
    VMStackPush 0x8032
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_63BF
    WorkSetConst 0x8010, 9
    VMReturn

L_63BF:
    VMStackPush 0x8032
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_63DA
    WorkSetConst 0x8010, 9
    VMReturn

L_63DA:
    VMCall L_738A
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_63FB
    WorkSetConst 0x8010, 9
    VMReturn

L_63FB:
    WorkGet 0x8010, 0x8032
    VMReturn

L_6403:
    WorkGet 0x802a, 0x8008
    WorkGet 0x8008, 0x802a
    VMCall L_6BED
    WorkGet 0x802b, 0x8009
    WorkGet 0x802c, 0x800a
    VMStackPush 0x802b
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_643C
    WorkSetConst 0x8010, 0
    VMReturn

L_643C:
    WorkGet 0x8008, 0x802a
    WorkGet 0x8009, 0x802b
    VMCall L_74D5
    Cmd_0209 0x802b, 0x8010
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_646B
    UnityTowerSetHobby 0x8010

L_646B:
    VMStackPush 0x802c
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_6484
    VMCall L_7892

L_6484:
    WorkSetConst 0x8010, 1
    VMReturn

L_648C:
    WorkGet 0x802a, 0x8008
    WorkGet 0x8008, 0x802a
    VMCall L_6BED
    WorkGet 0x802b, 0x800a
    WorkGet 0x802c, 0x800b
    VMStackPush 0x802b
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_64C5
    WorkSetConst 0x8010, 0
    VMReturn

L_64C5:
    WorkGet 0x8008, 0x802a
    WorkGet 0x8009, 0x802b
    VMCall L_7614
    Cmd_0209 0x802b, 0x8010
    VMStackPush 0x802c
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_64F6
    VMCall L_7892

L_64F6:
    WorkSetConst 0x8010, 1
    VMReturn

L_64FE:
    WorkGet 0x802a, 0x8008
    WorkGet 0x8008, 0x802a
    VMCall L_6BED
    WorkGet 0x802b, 0x800b
    VMStackPush 0x802b
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6531
    WorkSetConst 0x8010, 0
    VMReturn

L_6531:
    WorkGet 0x8008, 0x802a
    WorkGet 0x8009, 0x802b
    VMCall L_7753
    Cmd_0209 0x802b, 0x8010
    WorkSetConst 0x8010, 1
    VMReturn

L_6551:
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_6564
    VMJump L_6570

L_6564:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_6570:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_6583
    VMJump L_658F

L_6583:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_658F:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_65A2
    VMJump L_65AE

L_65A2:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_65AE:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_65C1
    VMJump L_65CD

L_65C1:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_65CD:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_65E0
    VMJump L_65EC

L_65E0:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_65EC:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_65FF
    VMJump L_660B

L_65FF:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_660B:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_661E
    VMJump L_662A

L_661E:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_662A:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_663D
    VMJump L_6649

L_663D:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_6649:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_665C
    VMJump L_6668

L_665C:
    WorkSetConst 0x804d, 31
    VMJump L_666A

L_6668:
    VMReturn

L_666A:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x804d
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMReturn

L_668C:
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_669F
    VMJump L_66A9

L_669F:
    FlagSet EVENT_FLAG_0x00bf
    VMJump L_6793

L_66A9:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_66BC
    VMJump L_66C6

L_66BC:
    FlagSet EVENT_FLAG_0x00c0
    VMJump L_6793

L_66C6:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_66D9
    VMJump L_66E3

L_66D9:
    FlagSet EVENT_FLAG_0x00c1
    VMJump L_6793

L_66E3:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_66F6
    VMJump L_6700

L_66F6:
    FlagSet EVENT_FLAG_0x00c2
    VMJump L_6793

L_6700:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_6713
    VMJump L_671D

L_6713:
    FlagSet EVENT_FLAG_0x00c3
    VMJump L_6793

L_671D:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_6730
    VMJump L_673A

L_6730:
    FlagSet EVENT_FLAG_0x00c4
    VMJump L_6793

L_673A:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_674D
    VMJump L_6757

L_674D:
    FlagSet EVENT_FLAG_0x00c5
    VMJump L_6793

L_6757:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_676A
    VMJump L_6774

L_676A:
    FlagSet EVENT_FLAG_0x00c6
    VMJump L_6793

L_6774:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_6787
    VMJump L_6791

L_6787:
    FlagSet EVENT_FLAG_0x00c7
    VMJump L_6793

L_6791:
    VMReturn

L_6793:
    VMReturn

L_6795:
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 269, 65535, 0
    ListMenuAdd 270, 65535, 1
    ListMenuAdd 271, 65535, 2
    ListMenuAdd 272, 65535, 3
    ListMenuAdd 273, 65535, 4
    ListMenuAdd 274, 65535, 5
    ListMenuAdd 275, 65535, 6
    ListMenuAdd 276, 65535, 7
    ListMenuAdd 277, 65535, 8
    ListMenuAdd 51, 65535, 9
    ListMenuShow
    VMReturn

L_67F2:
    VMCall L_6D84
    WorkGet 0x8040, 0x8009
    WorkGet 0x8041, 0x800a
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    WorkGet 0x8042, 0x8040
    WorkSetConst 0x8043, 1

L_6819:
    VMStackPush 0x8042
    VMStackPush 0x8041
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_684B
    WordSetSurveyAnswer 0, 0x8042
    ListMenuAdd 3, 65535, 0x8043
    WorkAdd 0x8042, 1
    WorkAdd 0x8043, 1
    VMJump L_6819

L_684B:
    ListMenuShow
    VMReturn

L_684F:
    VMCall L_6997
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6874
    WorkSetConst 0x8010, 1
    VMJump L_687A

L_6874:
    WorkSetConst 0x8010, 0

L_687A:
    VMReturn

L_687C:
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_688F
    VMJump L_689B

L_688F:
    FlagGet EVENT_FLAG_0x00bf, 0x8010
    VMJump L_6995

L_689B:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_68AE
    VMJump L_68BA

L_68AE:
    FlagGet EVENT_FLAG_0x00c0, 0x8010
    VMJump L_6995

L_68BA:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_68CD
    VMJump L_68D9

L_68CD:
    FlagGet EVENT_FLAG_0x00c1, 0x8010
    VMJump L_6995

L_68D9:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_68EC
    VMJump L_68F8

L_68EC:
    FlagGet EVENT_FLAG_0x00c2, 0x8010
    VMJump L_6995

L_68F8:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_690B
    VMJump L_6917

L_690B:
    FlagGet EVENT_FLAG_0x00c3, 0x8010
    VMJump L_6995

L_6917:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_692A
    VMJump L_6936

L_692A:
    FlagGet EVENT_FLAG_0x00c4, 0x8010
    VMJump L_6995

L_6936:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_6949
    VMJump L_6955

L_6949:
    FlagGet EVENT_FLAG_0x00c5, 0x8010
    VMJump L_6995

L_6955:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_6968
    VMJump L_6974

L_6968:
    FlagGet EVENT_FLAG_0x00c6, 0x8010
    VMJump L_6995

L_6974:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_6987
    VMJump L_6993

L_6987:
    FlagGet EVENT_FLAG_0x00c7, 0x8010
    VMJump L_6995

L_6993:
    VMReturn

L_6995:
    VMReturn

L_6997:
    WorkSetConst 0x804d, 0
    FlagGet EVENT_FLAG_0x00bf, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_69BC
    WorkAdd 0x804d, 1

L_69BC:
    FlagGet EVENT_FLAG_0x00c0, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_69DB
    WorkAdd 0x804d, 1

L_69DB:
    FlagGet EVENT_FLAG_0x00c1, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_69FA
    WorkAdd 0x804d, 1

L_69FA:
    FlagGet EVENT_FLAG_0x00c2, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6A19
    WorkAdd 0x804d, 1

L_6A19:
    FlagGet EVENT_FLAG_0x00c3, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6A38
    WorkAdd 0x804d, 1

L_6A38:
    FlagGet EVENT_FLAG_0x00c4, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6A57
    WorkAdd 0x804d, 1

L_6A57:
    FlagGet EVENT_FLAG_0x00c5, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6A76
    WorkAdd 0x804d, 1

L_6A76:
    FlagGet EVENT_FLAG_0x00c6, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6A95
    WorkAdd 0x804d, 1

L_6A95:
    FlagGet EVENT_FLAG_0x00c7, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6AB4
    WorkAdd 0x804d, 1

L_6AB4:
    WorkGet 0x8010, 0x804d
    VMReturn

L_6ABC:
    FlagGet EVENT_FLAG_0x00bf, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6ADD
    WorkSetConst 0x8010, 0
    VMReturn

L_6ADD:
    FlagGet EVENT_FLAG_0x00c0, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6AFE
    WorkSetConst 0x8010, 1
    VMReturn

L_6AFE:
    FlagGet EVENT_FLAG_0x00c1, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6B1F
    WorkSetConst 0x8010, 2
    VMReturn

L_6B1F:
    FlagGet EVENT_FLAG_0x00c2, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6B40
    WorkSetConst 0x8010, 3
    VMReturn

L_6B40:
    FlagGet EVENT_FLAG_0x00c3, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6B61
    WorkSetConst 0x8010, 4
    VMReturn

L_6B61:
    FlagGet EVENT_FLAG_0x00c4, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6B82
    WorkSetConst 0x8010, 5
    VMReturn

L_6B82:
    FlagGet EVENT_FLAG_0x00c5, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6BA3
    WorkSetConst 0x8010, 6
    VMReturn

L_6BA3:
    FlagGet EVENT_FLAG_0x00c6, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6BC4
    WorkSetConst 0x8010, 7
    VMReturn

L_6BC4:
    FlagGet EVENT_FLAG_0x00c7, 0x8008
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_6BE5
    WorkSetConst 0x8010, 8
    VMReturn

L_6BE5:
    WorkSetConst 0x8010, 9
    VMReturn

L_6BED:
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_6C00
    VMJump L_6C18

L_6C00:
    WorkSetConst 0x8009, 25
    WorkSetConst 0x800a, 26
    WorkSetConst 0x800b, 255
    VMJump L_6D82

L_6C18:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_6C2B
    VMJump L_6C43

L_6C2B:
    WorkSetConst 0x8009, 2
    WorkSetConst 0x800a, 7
    WorkSetConst 0x800b, 28
    VMJump L_6D82

L_6C43:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_6C56
    VMJump L_6C6E

L_6C56:
    WorkSetConst 0x8009, 3
    WorkSetConst 0x800a, 5
    WorkSetConst 0x800b, 11
    VMJump L_6D82

L_6C6E:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_6C81
    VMJump L_6C99

L_6C81:
    WorkSetConst 0x8009, 4
    WorkSetConst 0x800a, 6
    WorkSetConst 0x800b, 14
    VMJump L_6D82

L_6C99:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_6CAC
    VMJump L_6CC4

L_6CAC:
    WorkSetConst 0x8009, 12
    WorkSetConst 0x800a, 13
    WorkSetConst 0x800b, 24
    VMJump L_6D82

L_6CC4:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_6CD7
    VMJump L_6CEF

L_6CD7:
    WorkSetConst 0x8009, 20
    WorkSetConst 0x800a, 21
    WorkSetConst 0x800b, 22
    VMJump L_6D82

L_6CEF:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_6D02
    VMJump L_6D1A

L_6D02:
    WorkSetConst 0x8009, 10
    WorkSetConst 0x800a, 16
    WorkSetConst 0x800b, 17
    VMJump L_6D82

L_6D1A:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_6D2D
    VMJump L_6D45

L_6D2D:
    WorkSetConst 0x8009, 9
    WorkSetConst 0x800a, 19
    WorkSetConst 0x800b, 23
    VMJump L_6D82

L_6D45:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_6D58
    VMJump L_6D70

L_6D58:
    WorkSetConst 0x8009, 15
    WorkSetConst 0x800a, 18
    WorkSetConst 0x800b, 27
    VMJump L_6D82

L_6D70:
    WorkSetConst 0x8009, 255
    WorkSetConst 0x800a, 255
    WorkSetConst 0x800b, 255

L_6D82:
    VMReturn

L_6D84:
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_6D97
    VMJump L_6DA9

L_6D97:
    WorkSetConst 0x8009, 1
    WorkSetConst 0x800a, 2
    VMJump L_71E6

L_6DA9:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_6DBC
    VMJump L_6DCE

L_6DBC:
    WorkSetConst 0x8009, 3
    WorkSetConst 0x800a, 4
    VMJump L_71E6

L_6DCE:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_6DE1
    VMJump L_6DF3

L_6DE1:
    WorkSetConst 0x8009, 5
    WorkSetConst 0x800a, 6
    VMJump L_71E6

L_6DF3:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_6E06
    VMJump L_6E18

L_6E06:
    WorkSetConst 0x8009, 7
    WorkSetConst 0x800a, 8
    VMJump L_71E6

L_6E18:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_6E2B
    VMJump L_6E3D

L_6E2B:
    WorkSetConst 0x8009, 9
    WorkSetConst 0x800a, 10
    VMJump L_71E6

L_6E3D:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_6E50
    VMJump L_6E62

L_6E50:
    WorkSetConst 0x8009, 11
    WorkSetConst 0x800a, 12
    VMJump L_71E6

L_6E62:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_6E75
    VMJump L_6E87

L_6E75:
    WorkSetConst 0x8009, 13
    WorkSetConst 0x800a, 14
    VMJump L_71E6

L_6E87:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_6E9A
    VMJump L_6EAC

L_6E9A:
    WorkSetConst 0x8009, 15
    WorkSetConst 0x800a, 17
    VMJump L_71E6

L_6EAC:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_6EBF
    VMJump L_6ED1

L_6EBF:
    WorkSetConst 0x8009, 18
    WorkSetConst 0x800a, 20
    VMJump L_71E6

L_6ED1:
    WorkCmpConst 0x8008, 9
    VMJumpIf CMP_EQ, L_6EE4
    VMJump L_6EF6

L_6EE4:
    WorkSetConst 0x8009, 21
    WorkSetConst 0x800a, 22
    VMJump L_71E6

L_6EF6:
    WorkCmpConst 0x8008, 10
    VMJumpIf CMP_EQ, L_6F09
    VMJump L_6F1B

L_6F09:
    WorkSetConst 0x8009, 24
    WorkSetConst 0x800a, 26
    VMJump L_71E6

L_6F1B:
    WorkCmpConst 0x8008, 11
    VMJumpIf CMP_EQ, L_6F2E
    VMJump L_6F40

L_6F2E:
    WorkSetConst 0x8009, 27
    WorkSetConst 0x800a, 29
    VMJump L_71E6

L_6F40:
    WorkCmpConst 0x8008, 12
    VMJumpIf CMP_EQ, L_6F53
    VMJump L_6F65

L_6F53:
    WorkSetConst 0x8009, 30
    WorkSetConst 0x800a, 33
    VMJump L_71E6

L_6F65:
    WorkCmpConst 0x8008, 13
    VMJumpIf CMP_EQ, L_6F78
    VMJump L_6F8A

L_6F78:
    WorkSetConst 0x8009, 34
    WorkSetConst 0x800a, 35
    VMJump L_71E6

L_6F8A:
    WorkCmpConst 0x8008, 14
    VMJumpIf CMP_EQ, L_6F9D
    VMJump L_6FAF

L_6F9D:
    WorkSetConst 0x8009, 38
    WorkSetConst 0x800a, 41
    VMJump L_71E6

L_6FAF:
    WorkCmpConst 0x8008, 15
    VMJumpIf CMP_EQ, L_6FC2
    VMJump L_6FD4

L_6FC2:
    WorkSetConst 0x8009, 42
    WorkSetConst 0x800a, 45
    VMJump L_71E6

L_6FD4:
    WorkCmpConst 0x8008, 16
    VMJumpIf CMP_EQ, L_6FE7
    VMJump L_6FF9

L_6FE7:
    WorkSetConst 0x8009, 46
    WorkSetConst 0x800a, 49
    VMJump L_71E6

L_6FF9:
    WorkCmpConst 0x8008, 17
    VMJumpIf CMP_EQ, L_700C
    VMJump L_701E

L_700C:
    WorkSetConst 0x8009, 50
    WorkSetConst 0x800a, 53
    VMJump L_71E6

L_701E:
    WorkCmpConst 0x8008, 18
    VMJumpIf CMP_EQ, L_7031
    VMJump L_7043

L_7031:
    WorkSetConst 0x8009, 54
    WorkSetConst 0x800a, 57
    VMJump L_71E6

L_7043:
    WorkCmpConst 0x8008, 19
    VMJumpIf CMP_EQ, L_7056
    VMJump L_7068

L_7056:
    WorkSetConst 0x8009, 58
    WorkSetConst 0x800a, 62
    VMJump L_71E6

L_7068:
    WorkCmpConst 0x8008, 20
    VMJumpIf CMP_EQ, L_707B
    VMJump L_708D

L_707B:
    WorkSetConst 0x8009, 63
    WorkSetConst 0x800a, 67
    VMJump L_71E6

L_708D:
    WorkCmpConst 0x8008, 21
    VMJumpIf CMP_EQ, L_70A0
    VMJump L_70B2

L_70A0:
    WorkSetConst 0x8009, 68
    WorkSetConst 0x800a, 72
    VMJump L_71E6

L_70B2:
    WorkCmpConst 0x8008, 22
    VMJumpIf CMP_EQ, L_70C5
    VMJump L_70D7

L_70C5:
    WorkSetConst 0x8009, 73
    WorkSetConst 0x800a, 78
    VMJump L_71E6

L_70D7:
    WorkCmpConst 0x8008, 23
    VMJumpIf CMP_EQ, L_70EA
    VMJump L_70FC

L_70EA:
    WorkSetConst 0x8009, 79
    WorkSetConst 0x800a, 84
    VMJump L_71E6

L_70FC:
    WorkCmpConst 0x8008, 24
    VMJumpIf CMP_EQ, L_710F
    VMJump L_7121

L_710F:
    WorkSetConst 0x8009, 85
    WorkSetConst 0x800a, 89
    VMJump L_71E6

L_7121:
    WorkCmpConst 0x8008, 25
    VMJumpIf CMP_EQ, L_7134
    VMJump L_7146

L_7134:
    WorkSetConst 0x8009, 91
    WorkSetConst 0x800a, 98
    VMJump L_71E6

L_7146:
    WorkCmpConst 0x8008, 26
    VMJumpIf CMP_EQ, L_7159
    VMJump L_716B

L_7159:
    WorkSetConst 0x8009, 99
    WorkSetConst 0x800a, 106
    VMJump L_71E6

L_716B:
    WorkCmpConst 0x8008, 27
    VMJumpIf CMP_EQ, L_717E
    VMJump L_7190

L_717E:
    WorkSetConst 0x8009, 107
    WorkSetConst 0x800a, 114
    VMJump L_71E6

L_7190:
    WorkCmpConst 0x8008, 28
    VMJumpIf CMP_EQ, L_71A3
    VMJump L_71B5

L_71A3:
    WorkSetConst 0x8009, 118
    WorkSetConst 0x800a, 134
    VMJump L_71E6

L_71B5:
    WorkCmpConst 0x8008, 29
    VMJumpIf CMP_EQ, L_71C8
    VMJump L_71DA

L_71C8:
    WorkSetConst 0x8009, 135
    WorkSetConst 0x800a, 145
    VMJump L_71E6

L_71DA:
    WorkSetConst 0x8009, 0
    WorkSetConst 0x800a, 0

L_71E6:
    VMReturn

L_71E8:
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_71FB
    VMJump L_7207

L_71FB:
    WorkSetConst 0x804d, 319
    VMJump L_7301

L_7207:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_721A
    VMJump L_7226

L_721A:
    WorkSetConst 0x804d, 320
    VMJump L_7301

L_7226:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_7239
    VMJump L_7245

L_7239:
    WorkSetConst 0x804d, 321
    VMJump L_7301

L_7245:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_7258
    VMJump L_7264

L_7258:
    WorkSetConst 0x804d, 322
    VMJump L_7301

L_7264:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_7277
    VMJump L_7283

L_7277:
    WorkSetConst 0x804d, 323
    VMJump L_7301

L_7283:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_7296
    VMJump L_72A2

L_7296:
    WorkSetConst 0x804d, 324
    VMJump L_7301

L_72A2:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_72B5
    VMJump L_72C1

L_72B5:
    WorkSetConst 0x804d, 325
    VMJump L_7301

L_72C1:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_72D4
    VMJump L_72E0

L_72D4:
    WorkSetConst 0x804d, 326
    VMJump L_7301

L_72E0:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_72F3
    VMJump L_72FF

L_72F3:
    WorkSetConst 0x804d, 327
    VMJump L_7301

L_72FF:
    VMReturn

L_7301:
    SEPlay SEQ_SE_SYS_82
    SystemMsg 0x804d, 2
    InfoMsgClose
    VMReturn

L_730F:
    // "This is Passerby Analytics HQ.[f000]븁\u0000\nIf you want to join us,\nplease speak to the leader."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_PasserbyAnalyticsHqIf, 10, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_7321:
    WordSetPlayerName 1
    // "Hi, [f000]Ā\u0001\u0001.\nAre you conducting a survey vigorously?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_HiConductingSurveyVigorously, 10, 2, 0
    VMReturn

L_7332:
    // "Actually, I have a favor to ask you.[f000]븁\u0000\nI'm asking statisticians to answer\na questionnaire.[f000]븁\u0000\nOK, here we go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ActuallyHaveFavorAsk, 10, 2, 0
    VMReturn

L_7340:
    // "I'd like to ask you to answer\na questionnaire again.[f000]븁\u0000\nWill you?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_IdLikeAskAnswer, 10, 2, 0
    YesNoWin 0x8010
    VMReturn

L_7352:
    // "All right.[f000]븁\u0000\nIf you change your mind,\nplease speak to me."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_AllRightIfChange, 10, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_7364:
    // "Oh? It looks like you've answered\nall the questionnaires.[f000]븁\u0000\nWould you like to reanswer\nthe questionnaires you did before?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OhLooksLikeYouve, 10, 2, 0
    YesNoWin 0x8010
    VMReturn

L_7376:
    // "Then, will you choose a questionnaire\nyou'd like to answer?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ThenWillChooseQuestionnaire, 10, 2, 0
    VMCall L_6795
    VMReturn

L_738A:
    // "Choose this questionnaire?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ChooseQuestionnaire, 10, 2, 0
    YesNoWin 0x8010
    VMReturn

L_739C:
    // "You can answer the questionnaires\nas many times as you want.[f000]븁\u0000\nIf you want to change your answers,\nplease let me know."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_CanAnswerQuestionnairesMany, 10, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_73AE:
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_73C1
    VMJump L_73CD

L_73C1:
    WorkSetConst 0x804d, 280
    VMJump L_74C7

L_73CD:
    WorkCmpConst 0x8008, 1
    VMJumpIf CMP_EQ, L_73E0
    VMJump L_73EC

L_73E0:
    WorkSetConst 0x804d, 281
    VMJump L_74C7

L_73EC:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_73FF
    VMJump L_740B

L_73FF:
    WorkSetConst 0x804d, 282
    VMJump L_74C7

L_740B:
    WorkCmpConst 0x8008, 3
    VMJumpIf CMP_EQ, L_741E
    VMJump L_742A

L_741E:
    WorkSetConst 0x804d, 283
    VMJump L_74C7

L_742A:
    WorkCmpConst 0x8008, 4
    VMJumpIf CMP_EQ, L_743D
    VMJump L_7449

L_743D:
    WorkSetConst 0x804d, 284
    VMJump L_74C7

L_7449:
    WorkCmpConst 0x8008, 5
    VMJumpIf CMP_EQ, L_745C
    VMJump L_7468

L_745C:
    WorkSetConst 0x804d, 285
    VMJump L_74C7

L_7468:
    WorkCmpConst 0x8008, 6
    VMJumpIf CMP_EQ, L_747B
    VMJump L_7487

L_747B:
    WorkSetConst 0x804d, 286
    VMJump L_74C7

L_7487:
    WorkCmpConst 0x8008, 7
    VMJumpIf CMP_EQ, L_749A
    VMJump L_74A6

L_749A:
    WorkSetConst 0x804d, 287
    VMJump L_74C7

L_74A6:
    WorkCmpConst 0x8008, 8
    VMJumpIf CMP_EQ, L_74B9
    VMJump L_74C5

L_74B9:
    WorkSetConst 0x804d, 288
    VMJump L_74C7

L_74C5:
    VMReturn

L_74C7:
    ActorMsg MSGFILE_SCRIPT, 0x804d, 10, 2, 0
    VMReturn

L_74D5:
    WorkGet 0x8033, 0x8008
    WorkGet 0x8034, 0x8009
    WorkCmpConst 0x8033, 0
    VMJumpIf CMP_EQ, L_74F4
    VMJump L_7500

L_74F4:
    WorkSetConst 0x8032, 289
    VMJump L_75FA

L_7500:
    WorkCmpConst 0x8033, 1
    VMJumpIf CMP_EQ, L_7513
    VMJump L_751F

L_7513:
    WorkSetConst 0x8032, 290
    VMJump L_75FA

L_751F:
    WorkCmpConst 0x8033, 2
    VMJumpIf CMP_EQ, L_7532
    VMJump L_753E

L_7532:
    WorkSetConst 0x8032, 291
    VMJump L_75FA

L_753E:
    WorkCmpConst 0x8033, 3
    VMJumpIf CMP_EQ, L_7551
    VMJump L_755D

L_7551:
    WorkSetConst 0x8032, 292
    VMJump L_75FA

L_755D:
    WorkCmpConst 0x8033, 4
    VMJumpIf CMP_EQ, L_7570
    VMJump L_757C

L_7570:
    WorkSetConst 0x8032, 293
    VMJump L_75FA

L_757C:
    WorkCmpConst 0x8033, 5
    VMJumpIf CMP_EQ, L_758F
    VMJump L_759B

L_758F:
    WorkSetConst 0x8032, 294
    VMJump L_75FA

L_759B:
    WorkCmpConst 0x8033, 6
    VMJumpIf CMP_EQ, L_75AE
    VMJump L_75BA

L_75AE:
    WorkSetConst 0x8032, 295
    VMJump L_75FA

L_75BA:
    WorkCmpConst 0x8033, 7
    VMJumpIf CMP_EQ, L_75CD
    VMJump L_75D9

L_75CD:
    WorkSetConst 0x8032, 296
    VMJump L_75FA

L_75D9:
    WorkCmpConst 0x8033, 8
    VMJumpIf CMP_EQ, L_75EC
    VMJump L_75F8

L_75EC:
    WorkSetConst 0x8032, 297
    VMJump L_75FA

L_75F8:
    VMReturn

L_75FA:
    ActorMsg MSGFILE_SCRIPT, 0x8032, 10, 2, 0
    WorkGet 0x8008, 0x8034
    VMCall L_67F2
    VMReturn

L_7614:
    WorkGet 0x8033, 0x8008
    WorkGet 0x8034, 0x8009
    WorkCmpConst 0x8033, 0
    VMJumpIf CMP_EQ, L_7633
    VMJump L_763F

L_7633:
    WorkSetConst 0x8032, 298
    VMJump L_7739

L_763F:
    WorkCmpConst 0x8033, 1
    VMJumpIf CMP_EQ, L_7652
    VMJump L_765E

L_7652:
    WorkSetConst 0x8032, 299
    VMJump L_7739

L_765E:
    WorkCmpConst 0x8033, 2
    VMJumpIf CMP_EQ, L_7671
    VMJump L_767D

L_7671:
    WorkSetConst 0x8032, 300
    VMJump L_7739

L_767D:
    WorkCmpConst 0x8033, 3
    VMJumpIf CMP_EQ, L_7690
    VMJump L_769C

L_7690:
    WorkSetConst 0x8032, 301
    VMJump L_7739

L_769C:
    WorkCmpConst 0x8033, 4
    VMJumpIf CMP_EQ, L_76AF
    VMJump L_76BB

L_76AF:
    WorkSetConst 0x8032, 302
    VMJump L_7739

L_76BB:
    WorkCmpConst 0x8033, 5
    VMJumpIf CMP_EQ, L_76CE
    VMJump L_76DA

L_76CE:
    WorkSetConst 0x8032, 303
    VMJump L_7739

L_76DA:
    WorkCmpConst 0x8033, 6
    VMJumpIf CMP_EQ, L_76ED
    VMJump L_76F9

L_76ED:
    WorkSetConst 0x8032, 304
    VMJump L_7739

L_76F9:
    WorkCmpConst 0x8033, 7
    VMJumpIf CMP_EQ, L_770C
    VMJump L_7718

L_770C:
    WorkSetConst 0x8032, 305
    VMJump L_7739

L_7718:
    WorkCmpConst 0x8033, 8
    VMJumpIf CMP_EQ, L_772B
    VMJump L_7737

L_772B:
    WorkSetConst 0x8032, 306
    VMJump L_7739

L_7737:
    VMReturn

L_7739:
    ActorMsg MSGFILE_SCRIPT, 0x8032, 10, 2, 0
    WorkGet 0x8008, 0x8034
    VMCall L_67F2
    VMReturn

L_7753:
    WorkGet 0x8033, 0x8008
    WorkGet 0x8034, 0x8009
    WorkCmpConst 0x8033, 0
    VMJumpIf CMP_EQ, L_7772
    VMJump L_777E

L_7772:
    WorkSetConst 0x8032, 307
    VMJump L_7878

L_777E:
    WorkCmpConst 0x8033, 1
    VMJumpIf CMP_EQ, L_7791
    VMJump L_779D

L_7791:
    WorkSetConst 0x8032, 308
    VMJump L_7878

L_779D:
    WorkCmpConst 0x8033, 2
    VMJumpIf CMP_EQ, L_77B0
    VMJump L_77BC

L_77B0:
    WorkSetConst 0x8032, 309
    VMJump L_7878

L_77BC:
    WorkCmpConst 0x8033, 3
    VMJumpIf CMP_EQ, L_77CF
    VMJump L_77DB

L_77CF:
    WorkSetConst 0x8032, 310
    VMJump L_7878

L_77DB:
    WorkCmpConst 0x8033, 4
    VMJumpIf CMP_EQ, L_77EE
    VMJump L_77FA

L_77EE:
    WorkSetConst 0x8032, 311
    VMJump L_7878

L_77FA:
    WorkCmpConst 0x8033, 5
    VMJumpIf CMP_EQ, L_780D
    VMJump L_7819

L_780D:
    WorkSetConst 0x8032, 312
    VMJump L_7878

L_7819:
    WorkCmpConst 0x8033, 6
    VMJumpIf CMP_EQ, L_782C
    VMJump L_7838

L_782C:
    WorkSetConst 0x8032, 313
    VMJump L_7878

L_7838:
    WorkCmpConst 0x8033, 7
    VMJumpIf CMP_EQ, L_784B
    VMJump L_7857

L_784B:
    WorkSetConst 0x8032, 314
    VMJump L_7878

L_7857:
    WorkCmpConst 0x8033, 8
    VMJumpIf CMP_EQ, L_786A
    VMJump L_7876

L_786A:
    WorkSetConst 0x8032, 315
    VMJump L_7878

L_7876:
    VMReturn

L_7878:
    ActorMsg MSGFILE_SCRIPT, 0x8032, 10, 2, 0
    WorkGet 0x8008, 0x8034
    VMCall L_67F2
    VMReturn

L_7892:
    // "I see, I see...\nUh-huh.[f000]븁\u0000\nNext up...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_SeeSeeUhHuh, 10, 2, 0
    VMReturn

L_78A0:
    // "I see, I see...\nUh-huh.[f000]븁\u0000\n...OK. That's it![f000]븁\u0000\nThank you.\nI got a useful questionnaire.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_SeeSeeUhHuh_2, 10, 2, 0
    VMReturn

L_78AE:
    // "Oh, I will give this questionnaire sheet\nto you, too.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OhWillGiveQuestionnaire, 10, 2, 0
    ActorMsgClose
    VMReturn

L_78BE:
    // "And... This is a thank-you gift\nfor answering the questionnaire.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ThankGiftAnsweringQuestionnaire, 10, 2, 0
    ActorMsgClose
    VMReturn

L_78CE:
    // "This is the end of\nall the questionnaires.[f000]븁\u0000\nIt was fun to get to know you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_EndAllQuestionnairesFun, 10, 2, 0
    VMReturn

L_78DC:
    // "You can answer the questionnaires\nas many times as you want, so[f000]븀\u0000\nif you want to change your answers,[f000]븀\u0000\nplease let me know."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_CanAnswerQuestionnairesMany_2, 10, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_78EE:
    VMCall L_6997
    WordSetNumber 1, 0x8010, 1
    // "Now, you have...\nthis many questionnaires left: [f000]Ȁ\u0001\u0001.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_NowHaveManyQuestionnaires, 10, 2, 0
    VMReturn

L_7909:
    // "If you want to answer other\nquestionnaires, too, speak to me."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_IfWantAnswerOther, 10, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_791B:
    // "Excuse me, new statistician!\nPlease wait![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ExcuseNewStatisticianPlease, 10, 2, 0
    ActorMsgClose
    VMReturn

L_792B:
    // "Since you're here, why don't you\nanswer other questionnaires, too?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_SinceYoureHereWhy, 10, 2, 0
    YesNoWin 0x8010
    VMReturn
    .balign 4, 0

Movement_7940:
    Move 1, 1
    Move 49, 2
    MoveEnd

Movement_794C:
    Move 75, 1
    Move 0, 1
    MoveEnd

Movement_7958:
    Move 12, 7
    Move 2, 1
    MoveEnd

Movement_7964:
    Move 12, 3
    Move 14, 1
    Move 12, 4
    Move 2, 1
    MoveEnd

Movement_7978:
    Move 12, 3
    Move 14, 2
    Move 12, 4
    Move 2, 1
    MoveEnd

Movement_798C:
    Move 3, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello!\nGreeting is important, isn't it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_HelloGreetingImportantIsnt, 0x8011, 2, 0
    FlagGet EVENT_FLAG_0x00ce, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_79D7
    FlagSet EVENT_FLAG_0x00ce
    // "If you don't mind, would you tell me\nyour favorite greeting?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_IfDontMindWould, 0x8011, 2, 0
    VMJump L_79E3

L_79D7:
    // "If you don't mind, would you tell me\nyour favorite greeting again?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_IfDontMindWould_2, 0x8011, 2, 0

L_79E3:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7A10
    // "Well, OK. See you again soon!\nLet's always have a cheerful greeting!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WellOkSeeAgain, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_7A4F

L_7A10:
    ActorMsgClose
    CallGreetingPhraseInput 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7A3F
    // "Yes, it's a good greeting!\nIt cheers me up![f000]븁\u0000\nPlease feel free to come and say hi again!\nI look forward to seeing you again!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_YesItsGoodGreeting, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_7A4F

L_7A3F:
    // "Well, OK. See you again soon!\nLet's always have a cheerful greeting!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WellOkSeeAgain, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_7A4F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello! Do you always have a feeling\nof gratitude?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_HelloAlwaysHaveFeeling, 0x8011, 2, 0
    FlagGet EVENT_FLAG_0x00cf, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7AB1
    FlagSet EVENT_FLAG_0x00cf
    // "Oh, you dropped your Poké Ball.\nHere you are.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OhDroppedPokeBall, 0x8011, 2, 0
    ActorMsgClose
    WordSetPlayerName 1
    // "[f000]Ā\u0001\u0001 received a Poké Ball.[f000]븁\u0000"
    SystemMsg CasteliaCity13_Text_ReceivedPokeBall, 2
    InfoMsgClose
    // "Please remember to thank someone\nwho helps you![f000]븁\u0000\nIt may sound trivial, but it's important.[f000]븁\u0000\nNow, will you tell me your\nfeeling of gratitude?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_PleaseRememberThankSomeone, 0x8011, 2, 0
    VMJump L_7ABD

L_7AB1:
    // "If you don't mind, will you show me your\nfeeling of gratitude again?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_IfDontMindWill, 0x8011, 2, 0

L_7ABD:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7AEA
    // "Well, OK. See you again soon!\nDon't forget a feeling of gratitude!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WellOkSeeAgain_2, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_7B29

L_7AEA:
    ActorMsgClose
    CallThanksPhraseInput 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7B19
    // "What a wonderful phrase!\nIt warms my heart.[f000]븁\u0000\nIf somebody helps you,\nshow your feelings of gratitude.[f000]븁\u0000\nI am sure they will appreciate\nyour feelings."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WhatWonderfulPhraseWarms, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_7B29

L_7B19:
    // "Well, OK. See you again soon!\nDon't forget a feeling of gratitude!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WellOkSeeAgain_2, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_7B29:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    FlagGet EVENT_FLAG_0x0187, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7B66
    FlagSet EVENT_FLAG_0x0187
    // "When you're happy,\nwhat do you say?[f000]븁\u0000\nI say “Awesome!\"\nCould you teach me a new phrase?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WhenYoureHappyWhat, 0x8011, 2, 0
    VMJump L_7B72

L_7B66:
    // "Tell me something you just\nblurt out when you're happy!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_TellSomethingJustBlurt, 0x8011, 2, 0

L_7B72:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7B9F
    // "OK, then. See you next time.[f000]븁\u0000\nWhen you're happy, you have\nto express that joy, right?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OkThenSeeNext, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_7BDE

L_7B9F:
    ActorMsgClose
    CallHappyPhraseInput 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_7BCE
    // "What a wonderful phrase![f000]븁\u0000\nI registered it so I can\ninput it quickly in my[f000]븀\u0000\nTag Log comments!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_WhatWonderfulPhraseRegistered, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_7BDE

L_7BCE:
    // "OK, then. See you next time.[f000]븁\u0000\nWhen you're happy, you have\nto express that joy, right?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_OkThenSeeNext, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_7BDE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    TrainerGameInfoCmd_020B 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_7BFD
    VMJump L_7C09

L_7BFD:
    WorkSetConst 0x8008, 348
    VMJump L_7C8B

L_7C09:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_7C1C
    VMJump L_7C28

L_7C1C:
    WorkSetConst 0x8008, 349
    VMJump L_7C8B

L_7C28:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_7C3B
    VMJump L_7C47

L_7C3B:
    WorkSetConst 0x8008, 350
    VMJump L_7C8B

L_7C47:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_7C5A
    VMJump L_7C66

L_7C5A:
    WorkSetConst 0x8008, 351
    VMJump L_7C8B

L_7C66:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_7C79
    VMJump L_7C85

L_7C79:
    WorkSetConst 0x8008, 352
    VMJump L_7C8B

L_7C85:
    WorkSetConst 0x8008, 352

L_7C8B:
    WordSetPlayerName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    TrainerGameInfoCmd_020B 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_7CC3
    VMJump L_7CCF

L_7CC3:
    WorkSetConst 0x8008, 353
    VMJump L_7D51

L_7CCF:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_7CE2
    VMJump L_7CEE

L_7CE2:
    WorkSetConst 0x8008, 354
    VMJump L_7D51

L_7CEE:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_7D01
    VMJump L_7D0D

L_7D01:
    WorkSetConst 0x8008, 355
    VMJump L_7D51

L_7D0D:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_7D20
    VMJump L_7D2C

L_7D20:
    WorkSetConst 0x8008, 356
    VMJump L_7D51

L_7D2C:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_7D3F
    VMJump L_7D4B

L_7D3F:
    WorkSetConst 0x8008, 357
    VMJump L_7D51

L_7D4B:
    WorkSetConst 0x8008, 353

L_7D51:
    WordSetPlayerName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    TrainerGameInfoCmd_020B 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_7D89
    VMJump L_7D95

L_7D89:
    WorkSetConst 0x8008, 358
    VMJump L_7DF8

L_7D95:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_7DA8
    VMJump L_7DB4

L_7DA8:
    WorkSetConst 0x8008, 359
    VMJump L_7DF8

L_7DB4:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_7DC7
    VMJump L_7DD3

L_7DC7:
    WorkSetConst 0x8008, 360
    VMJump L_7DF8

L_7DD3:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_7DE6
    VMJump L_7DF2

L_7DE6:
    WorkSetConst 0x8008, 361
    VMJump L_7DF8

L_7DF2:
    WorkSetConst 0x8008, 358

L_7DF8:
    WordSetPlayerName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    TrainerGameInfoCmd_020B 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_7E30
    VMJump L_7E3C

L_7E30:
    WorkSetConst 0x8008, 362
    VMJump L_7E9F

L_7E3C:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_7E4F
    VMJump L_7E5B

L_7E4F:
    WorkSetConst 0x8008, 363
    VMJump L_7E9F

L_7E5B:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_7E6E
    VMJump L_7E7A

L_7E6E:
    WorkSetConst 0x8008, 364
    VMJump L_7E9F

L_7E7A:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_7E8D
    VMJump L_7E99

L_7E8D:
    WorkSetConst 0x8008, 365
    VMJump L_7E9F

L_7E99:
    WorkSetConst 0x8008, 362

L_7E9F:
    WordSetPlayerName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    TrainerGameInfoCmd_020B 0x8010
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_7ED7
    VMJump L_7EE3

L_7ED7:
    WorkSetConst 0x8008, 366
    VMJump L_7F27

L_7EE3:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_7EF6
    VMJump L_7F02

L_7EF6:
    WorkSetConst 0x8008, 367
    VMJump L_7F27

L_7F02:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_7F15
    VMJump L_7F21

L_7F15:
    WorkSetConst 0x8008, 368
    VMJump L_7F27

L_7F21:
    WorkSetConst 0x8008, 366

L_7F27:
    WordSetPlayerName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    TrainerGameInfoCmd_020B 0x8010
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_7F5F
    VMJump L_7F6B

L_7F5F:
    WorkSetConst 0x8008, 369
    VMJump L_7F90

L_7F6B:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_7F7E
    VMJump L_7F8A

L_7F7E:
    WorkSetConst 0x8008, 370
    VMJump L_7F90

L_7F8A:
    WorkSetConst 0x8008, 369

L_7F90:
    WordSetPlayerName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    TrainerGameInfoCmd_020B 0x8010
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_7FC8
    VMJump L_7FD4

L_7FC8:
    WorkSetConst 0x8008, 371
    VMJump L_7FDA

L_7FD4:
    WorkSetConst 0x8008, 371

L_7FDA:
    WordSetPlayerName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    Cmd_020A
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0139
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_8056
    SEPlay SEQ_SE_FLD_41
    // "I'm from the Castelia Harlequin Hunt![f000]븁\u0000\nYou found the Passerby Analytics HQ\nHarlequin! All riiight!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_ImFromCasteliaHarlequin, 0, 0
    FlagSet EVENT_FLAG_0x0139
    WorkAdd EVENT_WORK_0x40e2, 1
    SEWait
    LastKeyWait
    MsgWinCloseAll
    VMJump L_8064

L_8056:
    // "The statisticians research what's\npopular and what people like!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity13_Text_StatisticiansResearchWhatsPopular, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_8064:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    TrainerGameInfoCmd_020B 0x8010
    FlagSet EVENT_FLAG_0x027d
    FlagSet EVENT_FLAG_0x027e
    FlagSet EVENT_FLAG_0x027f
    FlagSet EVENT_FLAG_0x0280
    FlagSet EVENT_FLAG_0x0281
    FlagSet EVENT_FLAG_0x0282
    FlagSet EVENT_FLAG_0x0283
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_809D
    VMJump L_80AB

L_809D:
    FlagReset EVENT_FLAG_0x027d
    FlagReset EVENT_FLAG_0x027e
    VMJump L_8167

L_80AB:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_80BE
    VMJump L_80D4

L_80BE:
    FlagReset EVENT_FLAG_0x027d
    FlagReset EVENT_FLAG_0x027e
    FlagReset EVENT_FLAG_0x027f
    FlagReset EVENT_FLAG_0x0280
    VMJump L_8167

L_80D4:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_80E7
    VMJump L_8101

L_80E7:
    FlagReset EVENT_FLAG_0x027d
    FlagReset EVENT_FLAG_0x027e
    FlagReset EVENT_FLAG_0x027f
    FlagReset EVENT_FLAG_0x0280
    FlagReset EVENT_FLAG_0x0281
    VMJump L_8167

L_8101:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_8114
    VMJump L_8132

L_8114:
    FlagReset EVENT_FLAG_0x027d
    FlagReset EVENT_FLAG_0x027e
    FlagReset EVENT_FLAG_0x027f
    FlagReset EVENT_FLAG_0x0280
    FlagReset EVENT_FLAG_0x0281
    FlagReset EVENT_FLAG_0x0282
    VMJump L_8167

L_8132:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_8145
    VMJump L_8167

L_8145:
    FlagReset EVENT_FLAG_0x027d
    FlagReset EVENT_FLAG_0x027e
    FlagReset EVENT_FLAG_0x027f
    FlagReset EVENT_FLAG_0x0280
    FlagReset EVENT_FLAG_0x0281
    FlagReset EVENT_FLAG_0x0282
    FlagReset EVENT_FLAG_0x0283
    VMJump L_8167

L_8167:
    VMHalt
    .balign 4, 0
