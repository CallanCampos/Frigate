#include "asm/field_script.inc"
#include "text/script/castelia_city_15.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_1:
    ActorsPauseAll
    // "This is Battle Company.\nResearch and Development of Items!"
    InfoMsg CasteliaCity15_Text_BattleCompanyResearchDevelopment, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    // "Burgh's signature is scrawled in the\ncorner of the painting."
    InfoMsg CasteliaCity15_Text_BurghsSignatureScrawledCorner, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome![f000]븁\u0000\nIf you use the elevator, please use the\nbuttons on the door or next to the door."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_WelcomeIfUseElevator, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Every morning, my Pokémon wakes me\nwith Uproar, so I always look like[f000]븀\u0000\na wreck.[f000]븁\u0000\nBut I appreciate its good intentions.\nI'll work my hardest to provide for it[f000]븀\u0000\ntoday, as always."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_EveryMorningPokemonWakes, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0169
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E9
    // "That thing you have is a Pokédex,\nisn't it?[f000]븁\u0000\nWow! Coooooool! You collect Pokémon!\nOK! I'll help you.[f000]븁\u0000\nWhich Pokémon did you choose at the\nbeginning of your journey?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_ThingHavePokedexIsnt, 2, 2, 0
    WorkSetConst 0x8024, 0

L_00C7:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E3
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32803
    ListMenuAdd 11, 65535, 0
    ListMenuAdd 12, 65535, 1
    ListMenuAdd 13, 65535, 2
    ListMenuAdd 14, 65535, 3
    ListMenuShow
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019B
    // "Do you have Snivy?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_HaveSnivy, 2, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0189
    // "You have Snivy! Then I will give you this![f000]븁\u0000\nWhen you have your Pokémon hold it, it\ncan raise the power of Grass-type moves![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_HaveSnivyThenWill, 2, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 239
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0169
    // "A lot of items have effects when Pokémon\nhold them, so be on the lookout for[f000]븀\u0000\nthese items![f000]븁\u0000\nWell, work hard to fill up your Pokédex!\nGood luck!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_LotItemsHaveEffects, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8024, 1
    VMJump L_0195

L_0189:
    // "Then, what is the Pokémon you chose\nat the beginning of your journey?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_ThenWhatPokemonChose, 2, 2, 0

L_0195:
    VMJump L_02DD

L_019B:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0231
    // "Do you have Oshawott?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_HaveOshawott, 2, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_021F
    // "You have Oshawott! Then I will give\nyou this![f000]븁\u0000\nWhen you have your Pokémon hold it, it\ncan raise the power of Water-type moves![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_HaveOshawottThenWill, 2, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 243
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0169
    // "A lot of items have effects when Pokémon\nhold them, so be on the lookout for[f000]븀\u0000\nthese items![f000]븁\u0000\nWell, work hard to fill up your Pokédex!\nGood luck!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_LotItemsHaveEffects, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8024, 1
    VMJump L_022B

L_021F:
    // "Then, what is the Pokémon you chose\nat the beginning of your journey?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_ThenWhatPokemonChose, 2, 2, 0

L_022B:
    VMJump L_02DD

L_0231:
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C7
    // "Do you have Tepig?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_HaveTepig, 2, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B5
    // "You have Tepig! Then I will give you this![f000]븁\u0000\nWhen you have your Pokémon hold it, it\ncan raise the power of Fire-type moves![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_HaveTepigThenWill, 2, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 249
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0169
    // "A lot of items have effects when Pokémon\nhold them, so be on the lookout for[f000]븀\u0000\nthese items![f000]븁\u0000\nWell, work hard to fill up your Pokédex!\nGood luck!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_LotItemsHaveEffects, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8024, 1
    VMJump L_02C1

L_02B5:
    // "Then, what is the Pokémon you chose\nat the beginning of your journey?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_ThenWhatPokemonChose, 2, 2, 0

L_02C1:
    VMJump L_02DD

L_02C7:
    // "If you want to tell me, please speak\nto me!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_IfWantTellPlease, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8024, 1

L_02DD:
    VMJump L_00C7

L_02E3:
    VMJump L_02F9

L_02E9:
    // "A lot of items have effects when Pokémon\nhold them, so be on the lookout for[f000]븀\u0000\nthese items![f000]븁\u0000\nWell, work hard to fill up your Pokédex!\nGood luck!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_LotItemsHaveEffects, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_02F9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am a janitorial man. ♪\nI make everything spick and span. ♪[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_AmJanitorialManMake, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    ActorCmdExec 3, Movement_0484
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 1
    ActorWalkRoute 3, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 3, Movement_047C
    ActorCmdWait
    // "People who work in this building have\nPokémon battles, not opinion battles.[f000]븁\u0000\nYou appear to be strong, but if you go\nupstairs, please be extra careful.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_PeopleWhoWorkBuilding, 3, 0, 0
    // "Oh, yes! If you'd like, you should\nhave your Pokémon hold this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_OhYesIfYoud, 3, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 216
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "That's the Exp. Share![f000]븁\u0000\nA Pokémon holding an Exp. Share gets\nsome of the Exp. Points from every[f000]븀\u0000\nbattle, even if it's not involved.[f000]븁\u0000\nIt may be useful for\nraising weak Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_ThatsExpSharePokemon, 3, 0, 0
    // "I am a janitorial man. ♪\nI make everything spick and span. ♪[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_AmJanitorialManMake, 3, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9777, 0, 0xecba0, 0x78000, 0, 0x59000, 30
    ActorWalkRoute 3, 7, 2, 1, 8, 1
    EvCameraWait
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_03EC
    ActorCmdExec 3, Movement_0474
    ActorCmdWait

L_03EC:
    VMSleep 12
    SEPlay SEQ_SE_FLD_87
    SEWait
    WorkSetConst 0x8025, 0
    BMCreateHandleByGPos 0x8025, 1, 7, 1
    BMHndAudioVisualAnmPlay 0x8025, 0
    BMHndAnmWait 0x8025
    ActorCmdExec 3, Movement_0448
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8025, 1
    BMHndAnmWait 0x8025
    BMReleaseHandle 0x8025
    ActorDelete 3
    EvCameraReturn 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FlagSet EVENT_FLAG_0x0354
    WorkSetConst EVENT_WORK_0x40fc, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0448:
    Move 12, 2
    Move 33, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_0474:
    Move 32, 1
    MoveEnd

Movement_047C:
    Move 33, 1
    MoveEnd

Movement_0484:
    Move 75, 1
    MoveEnd

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x013a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04DB
    SEPlay SEQ_SE_FLD_41
    // "I'm from the Castelia Harlequin Hunt![f000]븁\u0000\nYou found the Battle Company\nHarlequin! All riiight!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_ImFromCasteliaHarlequin, 0, 0
    FlagSet EVENT_FLAG_0x013a
    WorkAdd EVENT_WORK_0x40e2, 1
    SEWait
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04E9

L_04DB:
    // "Battle Company develops many\ndifferent items for Pokémon and Trainers!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity15_Text_BattleCompanyDevelopsMany, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_04E9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
