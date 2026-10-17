#include "asm/field_script.inc"
#include "text/script/black_city.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    VMHalt

Script_2:
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0086
    VMStackPushFlag EVENT_FLAG_0x03ca
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0080
    FlagReset EVENT_FLAG_0x03ca
    ActorAdd 3

L_0080:
    VMJump L_00CF

L_0086:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00CF
    VMStackPushFlag EVENT_FLAG_0x03ca
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B4
    FlagReset EVENT_FLAG_0x03ca
    ActorAdd 3

L_00B4:
    VMStackPushFlag EVENT_FLAG_0x03cb
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CF
    FlagReset EVENT_FLAG_0x03cb
    ActorAdd 6

L_00CF:
    VMHalt

Script_3:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 59750, 0, 0x239000, 0x1b8000, 0x134000, 0xa9000, 1
    ActorCmdExec 255, Movement_0578
    ActorCmdWait
    EvCameraWait
    MapChangeWarp ZONE_BLACK_TOWER, 7, 14, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0570
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Black City\nProsperous and Vibrant"
    MsgPlaceSign BlackCity_Text_BlackCityProsperousVibrant, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_016A
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Dave: People who've got it goin' on\nalways get what they want![f000]븁\u0000\nIf there's a Pokémon you want\nto catch, keep on goin' on![f000]븁\u0000\nYou gotta get whatcha want\nthe right way--honestly and thoroughly!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_DavePeopleWhoveGot, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01CE

L_016A:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01A7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Dave: There's a shop where those\nwho've got it goin' on go.[f000]븁\u0000\nI heard they got some\nnew items in recently."
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_DaveTheresShopWhere, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01CE

L_01A7:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_01CE
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Dave: Used to be a lot of thugs\nhangin' around here looking for cash.[f000]븀\u0000\nI drove most of them off![f000]븁\u0000\nYeah! That's right! Black City\ngot its peace on all because of me!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_DaveUsedLotThugs, 0, 0
    LastKeyWait
    ActorMsgClose

L_01CE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0207
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Karenna: I heard strong people gather\nto train somewhere around here,[f000]븀\u0000\nso I brought my Pokémon![f000]븁\u0000\nYou're a Trainer too, right?\nLet's cheer each other on!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_KarennaHeardStrongPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0271

L_0207:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0247
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Karenna: You're [f000]Ā\u0001\u0000, right?[f000]븁\u0000\nIt sounds like you've conquered\na lot of the Black Tower![f000]븀\u0000\nI won't lose either!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_KarennaYoureRightSounds, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0271

L_0247:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0271
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Karenna: Congratulations on clearing\nthe Black Tower, [f000]Ā\u0001\u0000![f000]븀\u0000\nEveryone's talking about you![f000]븁\u0000\nI even heard the shops have new items\ncommemorating your victory!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_KarennaCongratulationsClearingBlack, 0, 0
    LastKeyWait
    ActorMsgClose

L_0271:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_02AA
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Marie: I came to Black City\nto write my thesis.[f000]븁\u0000\nThe theme of my research is\nPokémon that live in cities.[f000]븁\u0000\nI came to research the soothing\neffect Pokémon have on tired[f000]븀\u0000\nurban dwellers!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_MarieCameBlackCity, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02D1

L_02AA:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_02D1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Marie: I've finally organized\nthe research I've been[f000]븀\u0000\ndoing in Black City![f000]븁\u0000\nI knew the best way to relieve\nstress is to interact with Pokémon,[f000]븀\u0000\neven in the big city!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_MarieIveFinallyOrganized, 0, 0
    LastKeyWait
    ActorMsgClose

L_02D1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_030A
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Piper: I managed to get a job at one\nof the best companies in Black City,[f000]븀\u0000\nwhich is full of amazing businesses![f000]븁\u0000\nI'm going to work really hard\nand move up through the ranks!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_PiperManagedGetJob, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_036E

L_030A:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0347
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Piper: I can't take it! I'm done!\nWaaah![f000]븁\u0000\nSigh... I keep making mistakes at work,\nI got dumped...[f000]븁\u0000\nI want to run away to White Forest\nand relax in the woods with Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_PiperCantTakeIm, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_036E

L_0347:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_036E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Piper: The other day, I solved\na big problem at work![f000]븀\u0000\nEveryone complimented me! ♪[f000]븁\u0000\nA lot has happened recently,\nbut I'm glad I stuck with my job![f000]븁\u0000\nI'll keep making money\nhere in Black City!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_PiperOtherDaySolved, 0, 0
    LastKeyWait
    ActorMsgClose

L_036E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Eliza: I came to this town with\nthe desire to become rich and famous![f000]븁\u0000\nFirst, I'm going to clear the Black Tower\nfaster than anyone else[f000]븀\u0000\nand become really famous![f000]븁\u0000\n...What? You're kidding, right?\nYou cleared it already?[f000]븀\u0000\nNo! My plans are ruined!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_ElizaCameTownDesire, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_03C3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Collin: There's something I want,\nbut it is never in stock at the shops.[f000]븁\u0000\nThe guy at one of the shops said they\nwould have it in stock soon.[f000]븀\u0000\nBut I just can't wait!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_CollinTheresSomethingWant, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0427

L_03C3:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0400
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Collin: Have you been to the shop?\nThey got a new item in stock![f000]븁\u0000\nBut it's still not the item I want...[f000]븁\u0000\nIf I don't get my hands on it soon,\nI'm going to be in a lot of trouble!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_CollinHaveBeenShop, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0427

L_0400:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0427
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Collin: They finally got\nwhat I wanted in stock![f000]븀\u0000\nI've waited so long for this![f000]븁\u0000\n...Huh? What did I want?[f000]븁\u0000\nThat's kind of a nosy question!\nI'll never tell you!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_CollinTheyFinallyGot, 0, 0
    LastKeyWait
    ActorMsgClose

L_0427:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WordSetPlayerName 0
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0463
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ken: Unova's Challenge...[f000]븁\u0000\nTrainers come from all over the\nworld to challenge the Black Tower.[f000]븁\u0000\nAre you taking it on too, [f000]Ā\u0001\u0000?[f000]븁\u0000\nA difficult battle lies ahead!\nTake some amazing Pokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_KenUnovasChallengeTrainers, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04C7

L_0463:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04A0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ken: I heard about your exploits in\nUnova's Challenge, the Black Tower![f000]븁\u0000\nBut don't get cocky!\nThe higher the area,[f000]븀\u0000\nthe odder the Trainers get...[f000]븁\u0000\nWell, that was my experience anyway.\nDo your best and aim for the top!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_KenHeardAboutExploits, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04C7

L_04A0:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04C7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ken: You finally overcame Unova's\nChallenge, the Black Tower![f000]븁\u0000\n[f000]Ā\u0001\u0000...\nYou are truly amazing![f000]븁\u0000\nI've tried many times myself,\nbut I failed every time,[f000]븀\u0000\nand before long, I gave up.[f000]븁\u0000\nI respect you for believing in\nyour Pokémon and yourself!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_KenFinallyOvercameUnovas, 0, 0
    LastKeyWait
    ActorMsgClose

L_04C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0500
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Emi: When I'm not at my part-time job,\nI make piles of prize money by[f000]븀\u0000\nbattling in the Black Tower![f000]븁\u0000\nI'm glad I came here.\nI like quiet places, but...[f000]븁\u0000\nI couldn't make money like\nthis in White Forest."
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_EmiWhenImNot, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0567

L_0500:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0540
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Emi: I hear you made it pretty\nfar up in the Black Tower?[f000]븁\u0000\nThat's really amazing!\nI couldn't get past the first area![f000]븁\u0000\nStill, I can make a lot of money\nthere, so I don't really mind."
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_EmiHearMadePretty, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0567

L_0540:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0567
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Emi: Even if I'm making good money,\nI've never lived in the same place[f000]븀\u0000\nfor this long before...[f000]븁\u0000\nI've met a lot of people and\nfound some shops I really like.[f000]븁\u0000\nBut it's about time for me to leave.\nMaybe it would be nice to go to[f000]븀\u0000\nWhite Forest and relax for a while."
    ParentActorMsg MSGFILE_SCRIPT, BlackCity_Text_EmiEvenIfIm, 0, 0
    LastKeyWait
    ActorMsgClose

L_0567:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0570:
    Move 13, 1
    MoveEnd

Movement_0578:
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
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
