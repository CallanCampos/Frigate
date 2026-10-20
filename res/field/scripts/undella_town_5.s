#include "asm/field_script.inc"
#include "text/script/undella_town_5.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9694, 0, 0xecfca, 0x68000, 0x53000, 0x58000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_02A8
    ActorCmdWait
    FadeInBlackQ
    EvCameraReturn 60
    EvCameraWait
    FadeWait
    // "Elesa: Skyla,\ncan I ask you a favor?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaSkylaCanAsk, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0290
    ActorCmdWait
    // "Skyla: Sure, what's up?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SkylaSureWhatsUp, 0, 0, 0
    MsgWinCloseAll
    // "Elesa: I've decided I need\na PR makeover![f000]븁\u0000\nI wanted to try out my new ideas\nwith you, Skyla![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaIveDecidedNeed, 1, 0, 0
    MsgWinCloseAll
    // "Skyla: A PR makeover?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SkylaPrMakeover, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02B0
    ActorCmdWait
    // "Elesa: That's right![f000]븁\u0000\nHere's the thing. I make what everyone\nthinks is cool into a reality, right?[f000]븁\u0000\nWell, I get to do what I want to do,\nand while being a model may be difficult,[f000]븀\u0000\nmost importantly, it's fun![f000]븁\u0000\nBut the hard parts are really hard.\nI mean, everyone judges me[f000]븀\u0000\nsimply based on my appearance.[f000]븁\u0000\nPeople say that I'm reserved,\nand that I don't look like the[f000]븀\u0000\ntype who would tell jokes![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaThatsRightHeres, 1, 0, 0
    MsgWinCloseAll
    MultiMsg 5, 8, 5, 1
    VMSleep 50
    MsgWinCloseNo 1
    ActorCmdExec 1, Movement_02B8
    ActorCmdWait
    // "Elesa: So, I've been thinking about\nit constantly since then![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaIveBeenThinking, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02C4
    ActorCmdWait
    // "A sophisticated joke![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SophisticatedJoke, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02CC
    ActorCmdWait
    // "How should I put it...[f000]븁\u0000\nMy looks suggest that\nI'm not a lot of fun.[f000]븁\u0000\nBut I'll tear down that mistaken\nimage with my own hands![f000]븁\u0000\nI'll say something silly and give someone\nan opportunity to call me out,[f000]븀\u0000\nso we can all have a good laugh![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_HowShouldPutLooks, 1, 0, 0
    MsgWinCloseAll
    // "Skyla: Uh... OK...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SkylaUhOk, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02B8
    ActorCmdWait
    // "Elesa: OK, get a load of this![f000]븁\u0000\nThrow that misbehaving Klink\nin the clink![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaOkGetLoad, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02C4
    ActorCmdWait
    // "Elesa: Cofagrigus is so cool!\nDon't you *cough* agree, Gus?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaCofagrigusCoolDont, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02C4
    ActorCmdWait
    // "Elesa: You bought those Fossils from\nClay? Did you buy them on Clay-away?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaBoughtThoseFossils, 1, 0, 0
    MsgWinCloseAll
    MultiMsg 5, 8, 5, 1
    VMSleep 50
    MsgWinCloseNo 1
    // "Skyla: Elesa...[f000]븁\u0000\nThose are just a bunch of bad puns.\nWhere's the joke?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SkylaElesaThoseJust, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0288
    ActorCmdWait
    // "Elesa: Exactly! If I make bad puns when\nI'm supposed to be telling a joke,[f000]븀\u0000\ndoesn't it give people even more[f000]븀\u0000\nof an opportunity to tease me[f000]븀\u0000\nand start a funny back-and-forth?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaExactlyIfMake, 1, 0, 0
    MsgWinCloseAll
    // "Skyla: Um...[f000]븁\u0000\nThat's a little too sophisticated for me.\nActually, it's really hard to understand.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SkylaUmThatsLittle, 0, 0, 0
    MsgWinCloseAll
    // "Elesa: Really?[f000]븁\u0000\nI guess I'll just rethink\nmy fashion first.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaReallyGuessIll, 1, 0, 0
    MsgWinCloseAll
    // "Skyla: Sounds good![f000]븁\u0000\nHey, as long as we're hanging out,\nsurely you wouldn't mind having[f000]븀\u0000\na Pokémon battle with me, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SkylaSoundsGoodHey, 0, 0, 0
    MsgWinCloseAll
    // "Elesa: You bet!\nAnd don't call me Shirley![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_ElesaBetDontCall, 1, 0, 0
    MsgWinCloseAll
    VMSleep 60
    // "Skyla: Oh, Elesa...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UndellaTown5_Text_SkylaOhElesa, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveTo 9694, 0, 0xecfca, 0x68000, 0x53000, 0x58000, 30
    FadeOutBlack
    FadeWait
    EvCameraWait
    RTReserveScript 10
    EvCameraRebind
    EvCameraEnd
    MapChangeCore ZONE_MISTRALTON_CITY, 78, 0, 271, 0
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
    Move 12, 6
    MoveEnd
    Move 9, 1
    MoveEnd
    Move 39, 1
    Move 19, 1
    MoveEnd
    Move 38, 1
    Move 18, 1
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

Movement_0288:
    Move 75, 1
    MoveEnd

Movement_0290:
    Move 159, 1
    MoveEnd
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_02A8:
    Move 69, 1
    MoveEnd

Movement_02B0:
    Move 11, 1
    MoveEnd

Movement_02B8:
    Move 1, 1
    Move 2, 1
    MoveEnd

Movement_02C4:
    Move 18, 1
    MoveEnd

Movement_02CC:
    Move 11, 2
    MoveEnd
