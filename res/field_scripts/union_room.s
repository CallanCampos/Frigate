#include "asm/field_script.inc"
#include "text/script/union_room.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0066
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0030
    // "Welcome to the Union Room.[f000]븁\u0000\nIf there is anything you need help\nwith, please let me know![f000]븁\u0000"
    InfoMsg UnionRoom_Text_WelcomeUnionRoomIf, 2
    FlagSet EVENT_FLAG_0x0066

L_0030:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

L_003C:
    VMStackPush 0x8020
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0136
    // "Which topic would you\nlike me to explain?"
    InfoMsg UnionRoom_Text_WhichTopicWouldLike, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32801
    ListMenuAdd 41, 65535, 4
    ListMenuAdd 36, 65535, 0
    ListMenuAdd 37, 65535, 1
    ListMenuAdd 39, 65535, 2
    ListMenuAdd 40, 65535, 3
    ListMenuAdd 42, 65535, 255
    ListMenuShow
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_00A2
    VMJump L_00B2

L_00A2:
    // "There are five Battle Formats.[f000]븁\u0000\nSingle Battle is for two Trainers\nwith one or more Pokémon each.[f000]븁\u0000\nEach Trainer can have one Pokémon\nin battle at a time.[f000]븁\u0000\nDouble Battle is for two Trainers\nwith two or more Pokémon each.[f000]븁\u0000\nEach Trainer will send out two\nPokémon to battle at a time.[f000]븁\u0000\nTriple Battle is for two Trainers\nwith three or more Pokémon each.[f000]븁\u0000\nEach Trainer will send out three\nPokémon to battle at a time.[f000]븁\u0000"
    InfoMsg UnionRoom_Text_ThereFiveBattleFormats, 2
    // "Rotation Battle is for two Trainers\nwith three or more Pokémon each.[f000]븁\u0000\nEach Trainer sends out three Pokémon at\na time, one in front and two in the back.[f000]븁\u0000\nMulti Battle is for four Trainers\nwith one or more Pokémon each.[f000]븁\u0000\nEach Trainer can have one Pokémon\nin battle at a time.[f000]븁\u0000"
    InfoMsg UnionRoom_Text_RotationBattleTwoTrainers, 2
    VMJump L_0130

L_00B2:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_00C5
    VMJump L_00D0

L_00C5:
    // "You may trade your Pokémon with\nother players.[f000]븁\u0000\nMeeting new people could be a\nshortcut to meeting rare Pokémon![f000]븁\u0000"
    InfoMsg UnionRoom_Text_MayTradePokemonOther, 2
    VMJump L_0130

L_00D0:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_00E3
    VMJump L_00EE

L_00E3:
    // "You may get together with others\nand draw a picture.[f000]븁\u0000\nYou all get to work on one sheet\nof paper at the same time.[f000]븁\u0000\nI'm sure a drawing made by friends\nwill be a memorable masterpiece![f000]븁\u0000\nUp to five players can take part,\nso try it with your friends![f000]븁\u0000"
    InfoMsg UnionRoom_Text_MayGetTogetherOthers, 2
    VMJump L_0130

L_00EE:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_0101
    VMJump L_010C

L_0101:
    // "In a Spin Trade, participants each bring\nan Egg for trading with others.[f000]븁\u0000\nWhich Egg will you end up with?\nWhat kind of Pokémon is in that Egg?[f000]븀\u0000\nIt's quite exciting and fun![f000]븁\u0000\nUp to five players can take part, so\ntry it with your friends![f000]븁\u0000"
    InfoMsg UnionRoom_Text_SpinTradeParticipantsEach, 2
    VMJump L_0130

L_010C:
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_011F
    VMJump L_012A

L_011F:
    // "If you touch an icon on the bar\non the Touch Screen below,[f000]븀\u0000\nyou can choose words for chatting[f000]븀\u0000\nor tell others what you want to do,[f000]븀\u0000\nsuch as battles or trades.[f000]븁\u0000\nAlso, players who have chosen the same\nicon as you will jump up and down[f000]븀\u0000\nso you can spot them easily.[f000]븁\u0000\nTouch any icon to find a person you want\nto play with.[f000]븁\u0000"
    InfoMsg UnionRoom_Text_IfTouchIconBar, 2
    VMJump L_0130

L_012A:
    WorkSetConst 0x8020, 255

L_0130:
    VMJump L_003C

L_0136:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
