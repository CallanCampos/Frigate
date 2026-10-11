#include "asm/field_script.inc"
#include "text/script/castelia_city_27.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8021, 0
    // "Me oh my, the Badges you can get in the\nUnova region! Want to hear about them?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_OhBadgesCanGet, 1, 4, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0236

L_004B:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0230
    // "Which Badge do you want to know about?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_WhichBadgeWantKnow, 1, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 11, 65535, 0
    ListMenuAdd 12, 65535, 1
    ListMenuAdd 13, 65535, 2
    ListMenuAdd 14, 65535, 3
    ListMenuAdd 15, 65535, 4
    ListMenuAdd 16, 65535, 5
    ListMenuAdd 17, 65535, 6
    ListMenuAdd 18, 65535, 7
    ListMenuAdd 19, 65535, 8
    ListMenuShow
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_00D0
    VMJump L_00E2

L_00D0:
    // "With the Basic Badge, Pokémon up to\nLv. 20 will obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_BasicBadgePokemonUp, 1, 4, 0
    VMJump L_022A

L_00E2:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_00F5
    VMJump L_0107

L_00F5:
    // "With the Toxic Badge, Pokémon up to\nLv. 30 will obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_ToxicBadgePokemonUp, 1, 4, 0
    VMJump L_022A

L_0107:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_011A
    VMJump L_012C

L_011A:
    // "With the Insect Badge, Pokémon up to\nLv. 40 will obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_InsectBadgePokemonUp, 1, 4, 0
    VMJump L_022A

L_012C:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_013F
    VMJump L_0151

L_013F:
    // "With the Bolt Badge, Pokémon up to Lv. 50\nwill obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_BoltBadgePokemonUp, 1, 4, 0
    VMJump L_022A

L_0151:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_0164
    VMJump L_0176

L_0164:
    // "With the Quake Badge, Pokémon up to\nLv. 60 will obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_QuakeBadgePokemonUp, 1, 4, 0
    VMJump L_022A

L_0176:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_0189
    VMJump L_019B

L_0189:
    // "With the Jet Badge, Pokémon up to Lv. 70\nwill obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_JetBadgePokemonUp, 1, 4, 0
    VMJump L_022A

L_019B:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_01AE
    VMJump L_01C0

L_01AE:
    // "With the Legend Badge, Pokémon up to\nLv. 80 will obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_LegendBadgePokemonUp, 1, 4, 0
    VMJump L_022A

L_01C0:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_01D3
    VMJump L_01E5

L_01D3:
    // "With the Wave Badge, all Pokémon\nwill obey you without question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_WaveBadgeAllPokemon, 1, 4, 0
    VMJump L_022A

L_01E5:
    WorkCmpConst 0x8020, 8
    VMJumpIf CMP_EQ, L_01F8
    VMJump L_0214

L_01F8:
    // "Okey dokey. If you want to know\nabout them, please come back."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_OkeyDokeyIfWant, 1, 4, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8021, 1
    VMJump L_022A

L_0214:
    // "Okey dokey. If you want to know\nabout them, please come back."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_OkeyDokeyIfWant, 1, 4, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8021, 1

L_022A:
    VMJump L_004B

L_0230:
    VMJump L_0246

L_0236:
    // "Okey dokey. If you want to know\nabout them, please come back."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_OkeyDokeyIfWant, 1, 4, 0
    LastKeyWait
    MsgWinCloseAll

L_0246:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When I gaze down at the city from a\ntall building, I tremble.[f000]븁\u0000\nBecause...\nI-I-I'm scared of heights..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_WhenGazeDownCity, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Hwoof hwoof!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity27_Text_HwoofHwoof, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
