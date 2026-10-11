#include "asm/field_script.inc"
#include "text/script/chargestone_cave_5.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x98000, 0x67000, 0x138000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_0560
    ActorCmdWait
    FadeInBlackQ
    EvCameraReturn 60
    EvCameraWait
    FadeWait
    VMSleep 45
    ActorCmdExec 0, Movement_04D8
    ActorCmdWait
    // "N: [f000]븉\u0001\u0001Thank you, my friend.[f000]븁\u0000\nReturn to the peaceful\nlife you lived before.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_NThankFriendReturn, 0, 1, 0
    MsgWinCloseAll
    VMSleep 30
    ActorCmdExec 1, Movement_0528
    ActorCmdWait
    ActorCmdExec 1, Movement_0540
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 1, Movement_04D0
    ActorCmdWait
    // "Grunt: Lord N.\nWhy are you releasing your Pokémon?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_GruntLordNWhy, 1, 0, 0
    MsgWinCloseAll
    VMSleep 30
    ActorCmdExec 0, Movement_0570
    ActorCmdWait
    ActorCmdExec 1, Movement_0508
    ActorCmdWait
    VMSleep 30
    // "N: [f000]븉\u0001\u0001I can't...[f000]븁\u0000\nI just can't keep Pokémon\nconfined in Poké Balls![f000]븁\u0000\nAlso, if they stay with their Trainers,\nPokémon will battle,[f000]븀\u0000\nand they will be hurt...[f000]븁\u0000\nEven if it is for changing the\nworld to protect Pokémon...[f000]븁\u0000\nIt's too hard for me to put\nthem through such pain...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_NCantJustCant, 0, 1, 0
    MsgWinCloseAll
    // "Grunt: But...[f000]븁\u0000\nEver since we were young,\nwe've caught Pokémon and[f000]븀\u0000\nmade them battle.[f000]븁\u0000\nThat's just how the world\nworks, isn't it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_GruntButEverSince, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0518
    ActorCmdWait
    ActorCmdExec 0, Movement_0568
    ActorCmdWait
    // "N: [f000]븉\u0001\u0001Who decided that catching Pokémon\nand making them battle each other[f000]븀\u0000\nis how the world works?[f000]븁\u0000\nThat wasn't how things were\nbefore Poké Balls were invented...[f000]븁\u0000\nThe rules that govern\nthis world are wrong![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_NWhoDecidedCatching, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0530
    ActorCmdWait
    ActorCmdExec 1, Movement_0578
    ActorCmdWait
    // "Grunt: Th-that's true...[f000]븁\u0000\nWell, I guess I'll let\nmy Pokémon go, then.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_GruntThThatsTrue, 1, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 1, 9, 17, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0580
    ActorCmdWait
    // "N: [f000]븉\u0001\u0001Not yet!\nThe world hasn't changed yet![f000]븁\u0000\nThe time to free your Pokémon\nwill be when I befriend the Unova region's[f000]븀\u0000\nlegendary Dragon-type Pokémon,[f000]븀\u0000\nsurpass the Champion,[f000]븀\u0000\nand become the hero![f000]븉\u0001\u0000[f000]븁\u0000"
    // "N: [f000]븉\u0001\u0001Not yet!\nThe world hasn't changed yet![f000]븁\u0000\nThe time to free your Pokémon\nwill be when I befriend the Unova region's[f000]븀\u0000\nlegendary Dragon-type Pokémon,[f000]븀\u0000\nsurpass the Champion,[f000]븀\u0000\nand become the hero![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, ChargestoneCave5_Text_NNotYetWorld_2, ChargestoneCave5_Text_NNotYetWorld, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0518
    ActorCmdWait
    // "Grunt: Well then, I'll\nhead to the next destination.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_GruntWellThenIll, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0590
    VMSleep 8
    ActorCmdExec 0, Movement_0510
    ActorCmdWait
    ActorDelete 1
    // "[f000]븉\u0001\u0001I will separate Pokémon and people, and\nblack and white will be clearly distinct![f000]븁\u0000\nOnly then will Pokémon become\nperfect beings![f000]븁\u0000\nBut then why...[f000]븁\u0000\nWhy did those Pokémon\nseem so sad to leave me?[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_WillSeparatePokemonPeople, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_05A0
    ActorCmdWait
    Cmd_02B5 0, 0
    // "[f000]븉\u0001\u0001[f000]Ā\u0001\u0000![f000]븁\u0000\nIs it because of that Trainer\nthat my heart wavers now?[f000]븁\u0000\nWere the words of the Pokémon\nin Accumula Town really true?[f000]븁\u0000\nDoes that mean [f000]Ā\u0001\u0000\nis an ideal Trainer?[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_BecauseTrainerHeartWavers, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_05AC
    ActorCmdWait
    // "[f000]븉\u0001\u0001The cries of the suffering\nPokémon filling that room...[f000]븁\u0000\nThe borderline between Pokémon\nand humans...[f000]븁\u0000\nI exist on that line.\nI live in the margins between everyone,[f000]븀\u0000\nso I will save them![f000]븀\u0000\nI will change the world![f000]븁\u0000\nAnd to that end, I must\nfight to the finish with [f000]Ā\u0001\u0000![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ChargestoneCave5_Text_CriesSufferingPokemonFilling, 0, 1, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 16, 13, 1, 8, 1
    EvCameraMoveTo 9688, 0, 0xed000, 0x98000, 0x67000, 0x138000, 30
    FadeOutBlack
    FadeWait
    EvCameraWait
    ActorCmdWait
    EvCameraRebind
    EvCameraEnd
    RTReserveScript 2110
    WorkCmpConst EVENT_WORK_0x4192, 8
    VMJumpIf CMP_EQ, L_01EF
    VMJump L_0201

L_01EF:
    MapChangeCore ZONE_STRIATON_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0201:
    WorkCmpConst EVENT_WORK_0x4192, 20
    VMJumpIf CMP_EQ, L_0214
    VMJump L_0226

L_0214:
    MapChangeCore ZONE_NACRENE_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0226:
    WorkCmpConst EVENT_WORK_0x4192, 41
    VMJumpIf CMP_EQ, L_0239
    VMJump L_024B

L_0239:
    MapChangeCore ZONE_CASTELIA_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_024B:
    WorkCmpConst EVENT_WORK_0x4192, 65
    VMJumpIf CMP_EQ, L_025E
    VMJump L_0270

L_025E:
    MapChangeCore ZONE_NIMBASA_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0270:
    WorkCmpConst EVENT_WORK_0x4192, 99
    VMJumpIf CMP_EQ, L_0283
    VMJump L_0295

L_0283:
    MapChangeCore ZONE_DRIFTVEIL_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0295:
    WorkCmpConst EVENT_WORK_0x4192, 109
    VMJumpIf CMP_EQ, L_02A8
    VMJump L_02BA

L_02A8:
    MapChangeCore ZONE_MISTRALTON_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_02BA:
    WorkCmpConst EVENT_WORK_0x4192, 115
    VMJumpIf CMP_EQ, L_02CD
    VMJump L_02DF

L_02CD:
    MapChangeCore ZONE_ICIRRUS_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_02DF:
    WorkCmpConst EVENT_WORK_0x4192, 122
    VMJumpIf CMP_EQ, L_02F2
    VMJump L_0304

L_02F2:
    MapChangeCore ZONE_OPELUCID_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0304:
    WorkCmpConst EVENT_WORK_0x4192, 146
    VMJumpIf CMP_EQ, L_0317
    VMJump L_0329

L_0317:
    MapChangeCore ZONE_POKEMON_LEAGUE_11, 3, 0, 13, 0
    VMJump L_04C0

L_0329:
    WorkCmpConst EVENT_WORK_0x4192, 1
    VMJumpIf CMP_EQ, L_033C
    VMJump L_034E

L_033C:
    MapChangeCore ZONE_BLACK_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_034E:
    WorkCmpConst EVENT_WORK_0x4192, 425
    VMJumpIf CMP_EQ, L_0361
    VMJump L_0373

L_0361:
    MapChangeCore ZONE_WHITE_FOREST_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0373:
    WorkCmpConst EVENT_WORK_0x4192, 435
    VMJumpIf CMP_EQ, L_0386
    VMJump L_0398

L_0386:
    MapChangeCore ZONE_ASPERTIA_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0398:
    WorkCmpConst EVENT_WORK_0x4192, 454
    VMJumpIf CMP_EQ, L_03AB
    VMJump L_03BD

L_03AB:
    MapChangeCore ZONE_VIRBANK_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_03BD:
    WorkCmpConst EVENT_WORK_0x4192, 472
    VMJumpIf CMP_EQ, L_03D0
    VMJump L_03E2

L_03D0:
    MapChangeCore ZONE_HUMILAU_CITY_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_03E2:
    WorkCmpConst EVENT_WORK_0x4192, 398
    VMJumpIf CMP_EQ, L_03F5
    VMJump L_0407

L_03F5:
    MapChangeCore ZONE_ACCUMULA_TOWN_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0407:
    WorkCmpConst EVENT_WORK_0x4192, 407
    VMJumpIf CMP_EQ, L_041A
    VMJump L_042C

L_041A:
    MapChangeCore ZONE_LACUNOSA_TOWN_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_042C:
    WorkCmpConst EVENT_WORK_0x4192, 413
    VMJumpIf CMP_EQ, L_043F
    VMJump L_0451

L_043F:
    MapChangeCore ZONE_UNDELLA_TOWN_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0451:
    WorkCmpConst EVENT_WORK_0x4192, 443
    VMJumpIf CMP_EQ, L_0464
    VMJump L_0476

L_0464:
    MapChangeCore ZONE_FLOCCESY_TOWN_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_0476:
    WorkCmpConst EVENT_WORK_0x4192, 460
    VMJumpIf CMP_EQ, L_0489
    VMJump L_049B

L_0489:
    MapChangeCore ZONE_LENTIMAS_TOWN_POKEMON_CENTER, 3, 0, 13, 0
    VMJump L_04C0

L_049B:
    WorkCmpConst EVENT_WORK_0x4192, 602
    VMJumpIf CMP_EQ, L_04AE
    VMJump L_04C0

L_04AE:
    MapChangeCore ZONE_VICTORY_ROAD_27, 3, 0, 13, 0
    VMJump L_04C0

L_04C0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_04D0:
    Move 12, 1
    MoveEnd

Movement_04D8:
    Move 8, 1
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

Movement_0508:
    Move 3, 1
    MoveEnd

Movement_0510:
    Move 32, 1
    MoveEnd

Movement_0518:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0528:
    Move 35, 1
    MoveEnd

Movement_0530:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_0540:
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_0560:
    Move 69, 1
    MoveEnd

Movement_0568:
    Move 182, 1
    MoveEnd

Movement_0570:
    Move 11, 2
    MoveEnd

Movement_0578:
    Move 39, 4
    MoveEnd

Movement_0580:
    Move 10, 1
    Move 1, 1
    Move 182, 1
    MoveEnd

Movement_0590:
    Move 12, 4
    Move 15, 7
    Move 12, 5
    MoveEnd

Movement_05A0:
    Move 9, 1
    Move 182, 1
    MoveEnd

Movement_05AC:
    Move 10, 1
    MoveEnd
