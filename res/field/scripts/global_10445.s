#include "asm/field_script.inc"
#include "text/script/global_10445.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Cmd_01F6 0, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0045
    // "My name is Loblolly.\nI'm a designer of Décor items.[f000]븁\u0000\nBut I'm stuck now... I just can't come\nup with a good design.[f000]븁\u0000\nI've been thinking about Décor\nso great that it will appear in a dream..."
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_NameLoblollyImDesigner, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00E8

L_0045:
    Cmd_01F7 0, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0078
    // "Loblolly: Hmmm... I still can't think of a\ngreat new Décor item.[f000]븁\u0000\nWill you come back again?"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_LoblollyHmmmStillCant, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00E8

L_0078:
    VMStackPush EVENT_WORK_0x408a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B1
    // "Everybody is talking about Décor\nthey dream about. Do you know it?"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_EverybodyTalkingAboutDecor, 0x8011, 2, 0
    YesNoWin 0x8010
    // "Hello! My name is Loblolly, and I'm\na Décor designer.[f000]븁\u0000\nI am trying to design Décor items that\neverybody loves and dreams about.[f000]븁\u0000\nTables and chairs so great you can't\nstop thinking about them, even in[f000]븀\u0000\nyour sleep...[f000]븁\u0000\nThat's the kind of Décor I'm\ntalking about.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_HelloNameLoblollyIm, 0x8011, 2, 0
    RTCallGlobal 10446
    VMJump L_00E8

L_00B1:
    Cmd_01F7 1, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E4
    // "Loblolly: When I come up with some good\ndesigns, please help me again!"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_LoblollyWhenComeUp, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00E8

L_00E4:
    RTCallGlobal 10446

L_00E8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    WorkSetConst 0x8020, 0
    Cmd_01F7 2, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012D
    // "Loblolly: I finally came up with some new\ndesigns. I'm really proud of them![f000]븁\u0000\nBut I'd like your opinion. Would you\ntake a look at these Décor items, and[f000]븀\u0000\ntell me which one you'd dream about?"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_LoblollyFinallyCameUp, 0x8011, 2, 0
    Cmd_01F7 3, 0, 0, 0
    VMJump L_0164

L_012D:
    VMStackPush EVENT_WORK_0x408a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0158
    // "I think I've finally designed Décor\nitems so great that people will dream[f000]븀\u0000\nabout them![f000]븁\u0000\nHere! Which one would you dream about?"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_ThinkIveFinallyDesigned, 0x8011, 2, 0
    WorkSetConst EVENT_WORK_0x408a, 1
    VMJump L_0164

L_0158:
    // "Loblolly: Oh, are you going to help me\nwith my dream Décor design?[f000]븁\u0000\nGreat! Which of these Décor items would\nyou dream about?"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_LoblollyOhGoingHelp, 0x8011, 2, 0

L_0164:
    Cmd_01F8 0, 0
    Cmd_01F8 1, 1
    Cmd_01F8 2, 2
    Cmd_01F8 3, 3
    Cmd_01F8 4, 4
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 10, 65535, 0
    ListMenuAdd 11, 65535, 1
    ListMenuAdd 12, 65535, 2
    ListMenuAdd 13, 65535, 3
    ListMenuAdd 14, 65535, 4
    ListMenuAdd 15, 65535, 5
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_01F6
    // "Well, if you'd like to give me your opinion\non my dream Décor, please come back!"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_WellIfYoudLike, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0210

L_01F6:
    Cmd_01F7 4, 0x8020, 0, 0
    // "Oh, you have impeccable taste![f000]븁\u0000\nThat's it. This Décor is so great that\npeople will dream about it![f000]븁\u0000\nYour advice was very helpful. Next time I\ncome up with great designs, I'd love to[f000]븀\u0000\nbounce my ideas off you again!"
    ActorMsg MSGFILE_SCRIPT, Global10445_Text_OhHaveImpeccableTaste, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_0210:
    RTEndGlobal
    VMHalt
