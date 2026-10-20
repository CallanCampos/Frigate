#include "asm/field_script.inc"
#include "text/script/musical_theater_3.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xe8000, 0x5d000, 0xf8000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_0214
    ActorCmdWait
    FadeInBlackQ
    EvCameraReturn 60
    EvCameraWait
    FadeWait
    Cmd_02B5 1, 0
    // "Leaving the Prop Case behind...\nHuh. Did [f000]Ā\u0001\u0000 do that[f000]븀\u0000\naccidentally or on purpose?[f000]븁\u0000\nWhatever the reason, there's no doubt\nthat [f000]Ā\u0001\u0000 had a great talent[f000]븀\u0000\nfor coordinating Pokémon Props![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_LeavingPropCaseBehind, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0194
    ActorCmdWait
    // "Open the Prop Case--and voilà!\nColorful Props for Pokémon![f000]븁\u0000\nAw, yeah! It is time to play Dress Up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_OpenPropCaseVoil, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_021C
    ActorCmdWait
    // "The Top Hat is an elegant Prop that\nadds class to any Pokémon's head![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_TopHatElegantProp, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0240
    ActorCmdWait
    // "How about popping some cute\nBlue Barrettes on a Pokémon's ears?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_HowAboutPoppingSome, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0264
    ActorCmdWait
    // "The Square Glasses are eye catching.\nThey nicely frame a Pokémon's face![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_SquareGlassesEyeCatching, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_029C
    ActorCmdWait
    // "Maraca, Maraca, Maraca!\nIt's a sharp look for a Pokémon's arm.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_MaracaMaracaMaracaIts, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_02B8
    ActorCmdWait
    // "The Umber Belt accentuates a waistline!\nBelts are decorative as well as useful.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_UmberBeltAccentuatesWaistline, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_02E0
    ActorCmdWait
    // "Putting a Tie on a Pokémon's body makes\nit look dignified or charming--or both![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_PuttingTiePokemonsBody, 0, 0, 0
    // "There's nothing to worry about.[f000]븁\u0000\nIf [f000]Ā\u0001\u0000 comes back, we'll return\nthis trusty Prop Case in a trice.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_TheresNothingWorryAbout, 0, 0, 0
    // "In the meantime, we see in you a\nworthy successor to [f000]Ā\u0001\u0000.[f000]븁\u0000\nYes! You'll do!\nWith your talent and this Prop Case,[f000]븀\u0000\nwe foresee the rising of a future star![f000]븁\u0000\nProps to you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater3_Text_MeantimeWeSeeWorthy, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0320
    EvCameraMoveTo 9688, 0, 0xed000, 0xe8000, 0x5d000, 0xf8000, 30
    FadeOutBlack
    FadeWait
    EvCameraWait
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4087
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0153
    RTReserveScript 17
    VMJump L_0157

L_0153:
    RTReserveScript 18

L_0157:
    EvCameraRebind
    EvCameraEnd
    VMStackPush EVENT_WORK_0x4087
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0180
    MapChangeCore ZONE_MUSICAL_THEATER, 14, 0, 16, 0
    VMJump L_018C

L_0180:
    MapChangeCore ZONE_MUSICAL_THEATER, 17, 0, 4, 0

L_018C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0194:
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
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_0214:
    Move 69, 1
    MoveEnd

Movement_021C:
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    MoveEnd

Movement_0240:
    Move 15, 2
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    MoveEnd

Movement_0264:
    Move 12, 2
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 14, 2
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    MoveEnd

Movement_029C:
    Move 14, 2
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    MoveEnd

Movement_02B8:
    Move 13, 2
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    MoveEnd

Movement_02E0:
    Move 15, 2
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 12, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    MoveEnd

Movement_0320:
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    Move 3, 1
    Move 0, 1
    Move 2, 1
    Move 1, 1
    MoveEnd
