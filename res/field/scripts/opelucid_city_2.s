#include "asm/field_script.inc"
#include "text/script/opelucid_city_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_4:
    FlagSet EVENT_FLAG_0x01ee
    VMHalt

Script_5:
    VMStackPush EVENT_WORK_0x40d9
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003B
    ActorSetGPos 1, 14, 0, 10, 2

L_003B:
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0190
    VMSleep 32
    ActorCmdExec 0, Movement_01A4
    ActorCmdWait
    // "Drayden: Let me tell you the story.\nIt's a long story, but listen closely.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity2_Text_DraydenLetTellStory, 0, 0, 0
    MsgWinCloseAll
    // "It was two years ago when the two\ndragon Pokémon were awakened.[f000]븁\u0000\nThe white dragon Pokémon, Reshiram,\nsought what is true, with the desire[f000]븀\u0000\nto usher in a new world of goodness.[f000]븁\u0000\nAnd the black dragon Pokémon, Zekrom,\npursued what is ideal, with the desire[f000]븀\u0000\nto usher in a new world of hope.[f000]븁\u0000\nReshiram and Zekrom\nwere once a single Pokémon.[f000]븁\u0000"
    // "It was two years ago when the two\ndragon Pokémon were awakened.[f000]븁\u0000\nThe black dragon Pokémon, Zekrom,\npursued what is ideal, with the desire[f000]븀\u0000\nto usher in a new world of hope.[f000]븁\u0000\nAnd the white dragon Pokémon, Reshiram,\nsought what is true, with the desire[f000]븀\u0000\nto usher in a new world of goodness.[f000]븁\u0000\nZekrom and Reshiram\nwere once a single Pokémon.[f000]븁\u0000"
    ActorMsgVersioned 1024, OpelucidCity2_Text_TwoYearsAgoWhen_2, OpelucidCity2_Text_TwoYearsAgoWhen, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_025C
    ActorCmdWait
    // "You may wonder why it split in two.[f000]븁\u0000\nThe single dragon Pokémon had helped the\ntwin heroes bring a new region into being.[f000]븁\u0000\nBut the twin heroes--the younger\nbrother who sought ideals and the older[f000]븀\u0000\nbrother who sought the truth--sundered[f000]븀\u0000\nthe region in two as they fought to see[f000]븀\u0000\nwhich of them was right.[f000]븁\u0000\nIn that desperate hour, the single\ndragon Pokémon split its body into a[f000]븀\u0000\nwhite Pokémon and a black Pokémon,[f000]븀\u0000\neven though ideals and truth[f000]븀\u0000\ndon't need to be in opposition![f000]븁\u0000"
    // "You may wonder why it split in two.[f000]븁\u0000\nThe single dragon Pokémon had helped the\ntwin heroes bring a new region into being.[f000]븁\u0000\nBut the twin heroes--the older brother\nwho sought the truth and the[f000]븀\u0000\nyounger brother who sought ideals--[f000]븀\u0000\nsundered the region in two as they[f000]븀\u0000\nfought to see which of them was right.[f000]븁\u0000\nIn that desperate hour, the single\ndragon Pokémon split its body into a[f000]븀\u0000\nblack Pokémon and a white Pokémon,[f000]븀\u0000\neven though ideals and truth[f000]븀\u0000\ndon't need to be in opposition![f000]븁\u0000"
    ActorMsgVersioned 1024, OpelucidCity2_Text_MayWonderWhySplit_2, OpelucidCity2_Text_MayWonderWhySplit, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0204
    ActorCmdWait
    // "As the story goes, a third\ndragon Pokémon, [f000][ff00]\u0001\u0002Kyurem[f000][ff00]\u0001\u0000,[f000]븀\u0000\nalso came into existence in that era.[f000]븁\u0000\nAnd there may be proof of this to be\nfound in a treasure passed down in my[f000]븀\u0000\nfamily for generations: the DNA Splicers.[f000]븁\u0000\nProfessor Juniper's research determined\nthat the materials in the splicers date[f000]븀\u0000\nback to the same era as the materials[f000]븀\u0000\nused in building the Dragonspiral Tower.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity2_Text_StoryGoesThirdDragon, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_026C
    ActorCmdWait
    // "Oh, the DNA Splicers\nare stored very safely.[f000]븁\u0000\nI guard them because I don't know\nwhat kind of power might lie within them.[f000]븁\u0000\nBut here's what's been bothering me...\nCould there be one more dragon Pokémon?[f000]븁\u0000\nEven if Kyurem really exists,\nwe don't know what kind of Pokémon it is.[f000]븁\u0000\nFor starters, the two Pokémon\nthe ancient Pokémon split into[f000]븀\u0000\nare both overwhelmingly powerful.[f000]븁\u0000\nSo if Kyurem exists, could it be just\na husk--a shell that was left over?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity2_Text_OhDnaSplicersStored, 0, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_SW_SOURYU_RUMBLE
    EvCameraShake 0, 1, 3, 6, 0, 0, 0, 0
    // "Boom![f000]븁\u0000"
    ScreamMsg OpelucidCity2_Text_Boom, 2
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x09f9
    BGMChangeMap
    SEWait
    ActorCmdExec 0, Movement_027C
    ActorCmdWait
    ActorCmdExec 0, Movement_025C
    ActorCmdWait
    // "Drayden: Hm?[f000]븁\u0000\nWhat was that sound?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity2_Text_DraydenHmWhatSound, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_01BC
    ActorCmdExec 255, Movement_01B0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    ActorWalkRoute 255, 9, 14, 0, 8, 0
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40d9, 2
    FlagSet EVENT_FLAG_0x0320
    FlagReset EVENT_FLAG_0x031a
    FlagReset EVENT_FLAG_0x040a
    WorkSetConst EVENT_WORK_0x40d7, 1
    RTReserveScript 21
    WorkSetConst 0x8020, 0
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0179
    MapChangeWarp ZONE_OPELUCID_CITY, 418, 162, 1
    VMJump L_0183

L_0179:
    MapChangeWarp ZONE_OPELUCID_CITY, 418, 164, 1

L_0183:
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0190:
    Move 12, 3
    Move 15, 1
    Move 12, 2
    Move 34, 1
    MoveEnd

Movement_01A4:
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_01B0:
    Move 14, 1
    Move 13, 2
    MoveEnd

Movement_01BC:
    Move 17, 2
    Move 19, 1
    Move 17, 3
    MoveEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    RTCallGlobal 2280
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 610, 0
    // "Ax! Axew!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity2_Text_AxAxew, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0204:
    Move 10, 1
    Move 11, 1
    Move 30, 1
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

Movement_025C:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_026C:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd

Movement_027C:
    Move 159, 1
    MoveEnd
