#include "asm/field_script.inc"
#include "text/script/nimbasa_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004D
    // "Professional athletes look so attractive\nduring a game!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_ProfessionalAthletesLookAttractive, 0, 0
    VMJump L_0057

L_004D:
    // "We can tell how good these professionals\nare just by watching them practice."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_WeCanTellHow, 0, 0

L_0057:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4160, 3
    VMJumpIf CMP_EQ, L_007C
    VMJump L_008C

L_007C:
    // "I want to be a person who is good at\nfootball and Pokémon battles!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_WantPersonWhoGood, 0, 0
    VMJump L_00B9

L_008C:
    WorkCmpConst EVENT_WORK_0x4160, 1
    VMJumpIf CMP_EQ, L_009F
    VMJump L_00AF

L_009F:
    // "I want to be a person who is good at\nbaseball and Pokémon battles!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_WantPersonWhoGood_2, 0, 0
    VMJump L_00B9

L_00AF:
    // "I want to be a person who is good at\nsoccer and Pokémon battles!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_WantPersonWhoGood_3, 0, 0

L_00B9:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "On the field, they play games in earnest!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_FieldTheyPlayGames, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Just one ball can make people and\nPokémon smile.[f000]븁\u0000\nSports are wonderful things!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_JustOneBallCan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Eeeee! Turn this waaaaay!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_EeeeeTurnWaaaaay, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There!\nThere, turn like that!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_ThereThereTurnLike, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Following a ball right and left\nmakes me feel woozy."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_FollowingBallRightLeft, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Yahooooooo!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity4_Text_Yahooooooo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
