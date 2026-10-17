#include "asm/field_script.inc"
#include "text/script/opelucid_city_12.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9694, 0, 0xecf8c, 0x98000, 0x54000, 0x98000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_02C0
    ActorCmdWait
    ActorNew 10, 9, 2, 251, 337, 0
    FadeInBlackQ
    EvCameraReturn 60
    EvCameraWait
    FadeWait
    ActorCmdExec 251, Movement_02D0
    ActorCmdWait
    // "Iris: Yay! I did it!\nHaxorus, thank you![f000]븀\u0000\nI won because of you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisYayDidHaxorus, 251, 0, 0
    MsgWinCloseAll
    // "Drayden: Well done, Iris.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenWellDoneIris, 0, 0, 0
    MsgWinCloseAll
    // "Iris: What did you want\nto talk to me about, Grandpa?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisWhatDidWant, 251, 0, 0
    MsgWinCloseAll
    // "Drayden: Ah yes...\nI was thinking about the past.[f000]븁\u0000\nI traveled all over the world\nlooking for a successor.[f000]븁\u0000\nI even went to remote places\nsuch as the Village of Dragons.[f000]븁\u0000\nThat's where I met you, Iris.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenAhYesThinking, 0, 0, 0
    MsgWinCloseAll
    // "Iris: Yep!\nI was the strongest![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisYepStrongest, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0288
    ActorCmdWait
    // "Drayden: Yes.\nI was surprised.[f000]븁\u0000\nYou were one with your Pokémon\nand battling with so much joy.[f000]븁\u0000\nJust watching you made me smile.[f000]븁\u0000\nYour opponents felt disappointed by\ntheir defeat, but at the same time[f000]븀\u0000\nthey enjoyed the battle.[f000]븁\u0000\nIt was completely natural for me\nto decide I wanted to make you[f000]븀\u0000\nmy successor.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenYesSurprisedWere, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0258
    ActorCmdWait
    // "Iris: I'm so glad I came to Unova![f000]븁\u0000\nThere are many different people\nand so many different Pokémon![f000]븁\u0000\nAnd you know what...[f000]븁\u0000\nIn the Village of Dragons, people take\nliving alongside Pokémon for granted.[f000]븁\u0000\nI was surprised some people\nin Unova didn't think that way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisImGladCame, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0298
    ActorCmdWait
    // "Drayden: I found your reactions\nto be a breath of fresh air.[f000]븁\u0000\nAnd as a condition to leave the\nVillage of Dragons, you wanted[f000]븀\u0000\nto broaden your experiences[f000]븀\u0000\nand become the Champion...[f000]븁\u0000\nSo as I promised,\nI've been training you as a Trainer[f000]븀\u0000\nand as a Gym Leader.[f000]븁\u0000\nAn order directly from Alder\ncame as I was training you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenFoundReactionsBreath, 0, 0, 0
    MsgWinCloseAll
    // "Iris: Alder was smiling, though.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisAlderSmilingThough, 251, 0, 0
    MsgWinCloseAll
    // "Drayden: He lost to N\nand retrained himself.[f000]븁\u0000\nHe asked me to help him with his special\ntraining, but it was ghastly.[f000]븁\u0000\nHe wanted to become an immense\nobstacle for you, the new Champion...[f000]븁\u0000\nFor that alone, he pushed himself\nincredibly hard.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenHeLostN, 0, 0, 0
    MsgWinCloseAll
    // "Iris: Alder was really, really strong![f000]븁\u0000\nAnd even when I felt a bit weak,\nhe encouraged us![f000]븁\u0000\nHe said that even when their backs\nare against the wall, my Pokémon[f000]븀\u0000\nhave an intense look in their eyes.[f000]븁\u0000\nSo, that's why I won, but I feel\nlike I still have a lot to learn.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisAlderReallyReally, 251, 0, 0
    MsgWinCloseAll
    // "Drayden: Don't forget that feeling.[f000]븁\u0000\nAnd now, if I may change the subject...[f000]븁\u0000\nWhen you came to Opelucid City,\nI gave you those clothes, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenDontForgetFeeling, 0, 0, 0
    // "You're the Champion, now.\nIt's all right to dress up a little.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_YoureChampionNowIts, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0338
    ActorCmdWait
    ActorCmdExec 0, Movement_0344
    ActorCmdWait
    ActorCmdExec 251, Movement_02A0
    ActorCmdWait
    // "Iris: Are these new clothes?\nOK. I'll change into them right now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisTheseNewClothes, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_02D8
    ActorCmdWait
    ActorDelete 251
    ActorNew 10, 9, 2, 251, 338, 0
    ActorCmdExec 251, Movement_02D8
    ActorCmdWait
    // "Iris: Wooow!\nIt's such a flowing dress![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisWooowItsSuch, 251, 0, 0
    MsgWinCloseAll
    // "Drayden: What is important is\nyour mental preparation[f000]븀\u0000\nas the Champion, Iris.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenWhatImportantMental, 0, 0, 0
    // "When I was little,\nPoké Balls didn't exist yet.[f000]븁\u0000\nSometimes Pokémon would run away\nfrom awful Trainers who didn't try[f000]븀\u0000\nto understand them.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_WhenLittlePokeBalls, 0, 0, 0
    MsgWinCloseAll
    // "Iris: But you were fine, right?[f000]븁\u0000\nI can tell![f000]븁\u0000\nYour Haxorus loves you very much![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IrisButWereFine, 251, 0, 0
    ActorCmdExec 251, Movement_0318
    ActorCmdWait
    // "I've already decided!\nI know what kind of Champion I will be![f000]븁\u0000\nThere's a myth in Sinnoh that says\nthe reason why Pokémon jump out[f000]븀\u0000\nis because they want to thank people.[f000]븁\u0000\nI'm sure that we and Pokémon have helped\neach other and enriched the world[f000]븀\u0000\nsince ancient times.[f000]븁\u0000\nThese memories have been engraved in\neach Pokémon's heart![f000]븁\u0000\nSo, I want Pokémon and people\nto get closer and closer![f000]븁\u0000\nAs the Champion,\nI want to tell that to everybody![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_IveAlreadyDecidedKnow, 251, 0, 0
    MsgWinCloseAll
    // "Drayden: Good!\nIf anyone can do it, you can.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity12_Text_DraydenGoodIfAnyone, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveTo 9694, 0, 0xecf8c, 0x98000, 0x54000, 0x98000, 30
    FadeOutBlack
    FadeWait
    EvCameraWait
    RTReserveScript 30
    EvCameraRebind
    EvCameraEnd
    MapChangeCore ZONE_OPELUCID_CITY, 418, 0, 170, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 12, 6
    MoveEnd
    Move 9, 1
    MoveEnd
    Move 11, 1
    MoveEnd
    Move 48, 1
    MoveEnd
    Move 49, 1
    MoveEnd
    Move 51, 1
    MoveEnd

Movement_0258:
    Move 50, 1
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

Movement_0288:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0298:
    Move 35, 1
    MoveEnd

Movement_02A0:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_02C0:
    Move 69, 1
    MoveEnd
    Move 13, 4
    MoveEnd

Movement_02D0:
    Move 50, 2
    MoveEnd

Movement_02D8:
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 2, 1
    MoveEnd

Movement_0318:
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 2, 1
    MoveEnd

Movement_0338:
    Move 11, 1
    Move 35, 1
    MoveEnd

Movement_0344:
    Move 10, 1
    Move 3, 1
    MoveEnd
