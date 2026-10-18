#include "asm/field_script.inc"
#include "text/script/castelia_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_2:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40b1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0150
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 6917, 0, 0xcc000, 0x178000, 0, 0x308000, 40
    EvCameraWait
    // "If you're looking for the Gym Leader,\nBurgh, he said there might be trouble[f000]븀\u0000\nand then he took off![f000]븁\u0000\nYou can go look for him if you'd like![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_IfYoureLookingGym, 0, 0, 0
    MsgWinCloseAll
    ActorNew 20, 56, 0, 251, 355, 0
    // "???: Huh? Burgh vanished again?[f000]븁\u0000"
    InfoMsg CasteliaCity3_Text_HuhBurghVanishedAgain, 2
    MsgWinCloseAll
    ActorWalkRoute 251, 21, 48, 0, 8, 1
    ActorCmdExec 0, Movement_0288
    ActorCmdExec 255, Movement_0210
    ActorCmdWait
    // "Clyde: Oh! Hello, Iris.[f000]븁\u0000\nSomething came up, and Burgh\nisn't here right now.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_ClydeOhHelloIris, 0, 0, 0
    MsgWinCloseAll
    // "Iris: Hmm...[f000]븁\u0000\nIsn't Burgh always vanishing, though?[f000]븁\u0000\nHe always says he's got artist's block\nand just goes wandering out of the Gym.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_IrisHmmIsntBurgh, 251, 0, 0
    // "Hi there! Who are you?[f000]븁\u0000\n...[f000]븁\u0000\nLooking for Team Plasma?[f000]븁\u0000\nBut Team Plasma disbanded\ntwo years ago![f000]븁\u0000\nI guess that doesn't matter!\nYou're having problems,[f000]븀\u0000\nso I'll help you out![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_HiThereWhoLooking, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0270
    ActorCmdWait
    // "Hmm... Now where would\nsuspicious people go to hide?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_HmmNowWhereWould, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0288
    ActorCmdWait
    // "That's it!\nThat might be where they are![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_ThatsMightWhereThey, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0220
    VMSleep 6
    ActorCmdExec 0, Movement_0270
    ActorCmdExec 255, Movement_0270
    ActorCmdWait
    ActorDelete 251
    ActorCmdExec 0, Movement_0278
    ActorCmdExec 255, Movement_0280
    ActorCmdWait
    // "Clyde: Good grief...\nBurgh and Iris are so similar.[f000]븁\u0000\nIt looks like she went around the corner,\ntoward the Pokémon Center.[f000]븁\u0000\nDo you know where the Pokémon Center is?[f000]븁\u0000\nIf you keep following the street\nthat goes around Castelia City,[f000]븀\u0000\nit's right there!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_ClydeGoodGriefBurgh, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x40b1, 1
    FlagReset EVENT_FLAG_0x02ef
    VMJump L_0164

L_0150:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Clyde: Good grief...\nBurgh and Iris are so similar.[f000]븁\u0000\nIt looks like she went around the corner,\ntoward the Pokémon Center.[f000]븁\u0000\nDo you know where the Pokémon Center is?[f000]븁\u0000\nIf you keep following the street\nthat goes around Castelia City,[f000]븀\u0000\nit's right there!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_ClydeGoodGriefBurgh, 0, 0
    LastKeyWait
    ActorMsgClose

L_0164:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When it comes to loving my Pokémon,\nI won't lose to anybody![f000]븁\u0000\nI hope that gets through\nto my Pokémon..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_WhenComesLovingPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've got plenty of Potions!\nWith them, I'll bet I can[f000]븀\u0000\nbeat the Gym Leader!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_IveGotPlentyPotions, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wanting to become stronger\nas a Pokémon Trainer is good![f000]븁\u0000\nYou can have a good time playing with\nPokémon or put them to work instead!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_WantingBecomeStrongerPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If I only had five Badges,\nI could buy Ultra Balls..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_IfOnlyHadFive, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My little Palpitoad\nis utterly charming![f000]븁\u0000\nWhen I come home tired,\nit makes me feel better!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity3_Text_LittlePalpitoadUtterlyCharming, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Castelia City Pokémon Gym\nLeader: Burgh[f000]븀\u0000\nPremier Insect Artist"
    MsgPlaceSign CasteliaCity3_Text_CasteliaCityPokemonGym, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0210:
    Move 75, 1
    Move 63, 1
    Move 34, 1
    MoveEnd

Movement_0220:
    Move 17, 8
    MoveEnd
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
    Move 32, 1
    MoveEnd

Movement_0270:
    Move 33, 1
    MoveEnd

Movement_0278:
    Move 34, 1
    MoveEnd

Movement_0280:
    Move 35, 1
    MoveEnd

Movement_0288:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
