#include "asm/field_script.inc"
#include "text/script/striaton_city_10.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9694, 0, 0xecf27, 0x78000, 0x48000, 0x38000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_03C4
    ActorCmdExec 3, Movement_03C4
    ActorCmdExec 4, Movement_03C4
    ActorCmdExec 5, Movement_03C4
    ActorCmdWait
    FadeInBlackQ
    EvCameraMoveTo 9694, 0, 0xed02b, 0x78000, 0, 0x38000, 60
    EvCameraWait
    FadeWait
    // "Cilan: Chili, are you listening to me?[f000]븁\u0000\nWe have to study hard\nand become full-fledged Gym Leaders[f000]븀\u0000\nas soon as possible...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CilanChiliListeningWe, 1, 0, 0
    MsgWinCloseAll
    // "Chili: About that.[f000]븁\u0000\nIf we had gone to N's Castle a year\nago, the Seven Sages and the[f000]븀\u0000\nShadow Triad wouldn't have escaped![f000]븁\u0000\nThen, nobody would say something like\n“Three of you together make...\"[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ChiliAboutIfWe, 0, 0, 0
    MsgWinCloseAll
    // "Cress: I guess you're right.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CressGuessYoureRight, 2, 0, 0
    MsgWinCloseAll
    // "Cilan: You think so?\nBut it isn't that big of a deal.[f000]븁\u0000\nWe may not have made it in time,\nbut Team Plasma is still gone.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CilanThinkButIsnt, 1, 0, 0
    MsgWinCloseAll
    // "Chili: Argh![f000]븁\u0000\nIt's because you say things like that.\nThat's why people treat us like a joke![f000]븁\u0000\nLike it takes all three of us together\nto make one great Trainer![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ChiliArghItsBecause, 0, 0, 0
    MsgWinCloseAll
    // "Cress: You're overthinking it,\nbut I can't say the possibility is zero.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CressYoureOverthinkingBut, 2, 0, 0
    MsgWinCloseAll
    // "Cilan: Come on, you two...[f000]븁\u0000\nDon't you think it's a bit strange\nto get recognition and respect as[f000]븀\u0000\nGym Leaders just for beating bad guys?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CilanComeTwoDont, 1, 0, 0
    MsgWinCloseAll
    // "Chili: I understand what you're saying.[f000]븁\u0000\nBut if things go on like this,\nnothing will ever change.[f000]븁\u0000\nEven if we train, the other Gym Leaders\nwill become that much stronger as well.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ChiliUnderstandWhatYoure, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_03F8
    ActorCmdWait
    // "???: Then...\nWe'll battle you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ThenWellBattle, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_03F8
    ActorCmdExec 5, Movement_03F8
    ActorCmdWait
    EvCameraMoveTo 9688, 0, 0xed000, 0xa3000, 0, 0x38000, 30
    ActorCmdExec 0, Movement_03A4
    ActorCmdExec 2, Movement_03A4
    VMSleep 10
    ActorCmdExec 1, Movement_03AC
    ActorCmdWait
    ActorCmdExec 0, Movement_039C
    ActorCmdExec 2, Movement_039C
    ActorCmdExec 1, Movement_03CC
    ActorCmdWait
    EvCameraWait
    // "Cress: Based on your appearance,\nyou must be the Shadow Triad.[f000]븀\u0000\nBut why are you here?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CressBasedAppearanceMust, 2, 0, 0
    MsgWinCloseAll
    // "Shadow Triad: We came on a whim...[f000]븁\u0000\nTo vent...[f000]븁\u0000\nTo pass time...[f000]븁\u0000\nIt doesn't matter.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ShadowTriadWeCame, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_039C
    ActorCmdWait
    // "Chili: Three on three![f000]븁\u0000\nThe first one to beat them\nis the strongest among us![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ChiliThreeThreeFirst, 0, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    ActorCmdExec 0, Movement_0354
    ActorCmdExec 2, Movement_0354
    ActorCmdExec 3, Movement_035C
    ActorCmdExec 4, Movement_035C
    ActorCmdExec 5, Movement_035C
    ActorCmdExec 1, Movement_0354
    ActorCmdWait
    FadeExWait
    VMSleep 90
    FadeEx 3, 16, 0, 2
    FadeExWait
    ActorCmdExec 0, Movement_03B4
    ActorCmdExec 2, Movement_03B4
    ActorCmdExec 1, Movement_03B4
    ActorCmdWait
    VMSleep 30
    // "Shadow Triad: Too easy...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ShadowTriadTooEasy, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_038C
    ActorCmdWait
    // "Shadow Triad: These three aren't worth\nworrying about.[f000]븁\u0000\nWe must focus on the other\nGym Leaders so that next time[f000]븀\u0000\nthey won't get in our way...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ShadowTriadTheseThree, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0384
    ActorCmdWait
    // "Shadow Triad: Well...\nLord Ghetsis is waiting.[f000]븁\u0000\nWe must catch that Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ShadowTriadWellLord, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_0404
    ActorCmdExec 4, Movement_0404
    ActorCmdExec 5, Movement_0404
    ActorCmdWait
    EvCameraMoveTo 9694, 0, 0xed02b, 0x78000, 0, 0x38000, 30
    EvCameraWait
    VMSleep 30
    ActorCmdExec 0, Movement_0394
    ActorCmdWait
    // "Chili: C'mon![f000]븁\u0000\nWhat was that?\nCome back here and say that again![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ChiliCmonWhatCome, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0384
    ActorCmdWait
    // "Cilan: I mean...\nThey were overwhelmingly strong.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CilanMeanTheyWere, 1, 0, 0
    MsgWinCloseAll
    // "Cress: We have to admit it.[f000]븁\u0000\nThe three Leaders of Striaton City\nmake one full-fledged Trainer.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CressWeHaveAdmit, 2, 0, 0
    MsgWinCloseAll
    // "Cilan: You're right.[f000]븁\u0000\nChili. Cress.[f000]븁\u0000\nI think as long as the three of us\nact like the Gym Leaders we are now,[f000]븀\u0000\nnothing will ever change.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CilanYoureRightChili, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_038C
    ActorCmdExec 2, Movement_0394
    ActorCmdWait
    // "Chili: I didn't think\nyou would be the first to say that.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ChiliDidntThinkWould, 0, 0, 0
    MsgWinCloseAll
    // "Cress: Of course, I, Cress,\nwas thinking about that, too.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CressCourseCressThinking, 2, 0, 0
    MsgWinCloseAll
    // "Cilan: All right.[f000]븁\u0000\nLet's resign as Gym Leaders\nand start our training over[f000]븀\u0000\nfrom the ground up.[f000]븁\u0000\nThen, I'm sure one of us\nwill become a Trainer[f000]븀\u0000\neveryone recognizes as great.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CilanAllRightLets, 1, 0, 0
    MsgWinCloseAll
    // "Chili: We're triplets.\nWhat we think is the same.[f000]븁\u0000\nSo we just have to become\nas strong as each other,[f000]븀\u0000\nand then we can be[f000]븀\u0000\nGym Leaders together again![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_ChiliWereTripletsWhat, 0, 0, 0
    MsgWinCloseAll
    // "Cress: OK. So as for the replacement\nGym Leader...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity10_Text_CressOkReplacementGym, 2, 0, 0
    MsgWinCloseAll
    EvCameraMoveTo 9694, 0, 0xecf27, 0x78000, 0x48000, 0x38000, 30
    FadeOutBlack
    FadeWait
    EvCameraWait
    RTReserveScript 15
    EvCameraRebind
    EvCameraEnd
    MapChangeCore ZONE_STRIATON_CITY_2, 11, 0, 3, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0354:
    Move 15, 1
    MoveEnd

Movement_035C:
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

Movement_0384:
    Move 32, 1
    MoveEnd

Movement_038C:
    Move 33, 1
    MoveEnd

Movement_0394:
    Move 34, 1
    MoveEnd

Movement_039C:
    Move 35, 1
    MoveEnd

Movement_03A4:
    Move 75, 1
    MoveEnd

Movement_03AC:
    Move 159, 1
    MoveEnd

Movement_03B4:
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_03C4:
    Move 69, 1
    MoveEnd

Movement_03CC:
    Move 13, 1
    Move 15, 1
    MoveEnd
    Move 10, 1
    MoveEnd
    Move 14, 1
    Move 32, 1
    MoveEnd
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_03F8:
    Move 70, 1
    Move 184, 1
    MoveEnd

Movement_0404:
    Move 185, 1
    Move 69, 1
    MoveEnd
