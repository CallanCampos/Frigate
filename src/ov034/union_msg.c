#include "field/union_msg.h"
#include "types.h"
#include "constants/language.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/unity_tower.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "system/bmp_menulist.h"
#include "system/game_data.h"
#include "system/pms_word.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/0399.h"

// The Union Room's messages and menus: the lookups of message IDs, the message system the field talks through (the
// message data, the word set, the stream window), the menus of the battle rules and the regulation's rules printed.
// Names are not swan's

#define UNION_HEAPID 0x41

// A row of a menu: the message, and its value or the menu it opens
typedef struct UnionMenuRow {
    u32 msg;
    union {
        u32 value;
        const struct UnionMenuRow *sub;
    };
} UnionMenuRow;

typedef struct {
    const UnionMenuRow *rows;
    u8 count;
} UnionMenuInfo;

static void UnionMsg_SetupWordSet(UnionSystem *unisys);
static void UnionMsg_SetLanguageFlag(GameData *gameData, u32 language);
static void UnionMsg_CreateWordSet(UnionSystem *unisys);
static void UnionMsg_FreeWordSet(UnionSystem *unisys);
static void UnionMsg_FreeMsgSystem(UnionSystem *unisys);
static void UnionMsg_CreateMenu(UnionSystem *unisys, Field *field, const UnionMenuRow *rows, u32 count,
                                ListMenuRequest *request);
static void UnionMsg_FreeMenu(UnionSystem *unisys);
static s32 UnionMsg_UpdateMenu(UnionSystem *unisys, Field *field);

// The menus of the battle rules, each row a message and a value or the menu it opens
static const UnionMenuRow sMenu_029c[3];
static const UnionMenuRow sMenu_02b4[3];
static const UnionMenuRow sMenu_02cc[3];
static const UnionMenuRow sMenu_02e4[3];
static const UnionMenuRow sMenu_02fc[3];
static const UnionMenuRow sMenu_0374[5];
static const UnionMenuRow sMenu_039c[5];
static const UnionMenuRow sMenu_03f4[6];

// The tables are declared in the order that gives the ROM's .rodata: MWCC heapsorts the objects of equal size by where
// they are declared. The pairs are the messages of a gender, or of a language, by the lookups above
static const u16 sMsgPair_0214[2] = { Bank0399_Text_ThanksWaitingLetsGet, Bank0399_Text_SorryMadeWaitLets };

static const u16 sMsgPair_0210[2] = { Bank0399_Text_ThereEggCannotTraded_3, Bank0399_Text_ThereEggCannotTraded_4 };

static const u16 sMsgPair_0208[2] = { Bank0399_Text_CantTradeIfHave, Bank0399_Text_CantTradeIfHave_2 };

static const u16 sMsgPair_01f0[2] = { Bank0399_Text_OhAllRightCome_3, Bank0399_Text_OhPleaseComeBy_3 };

static const u16 sMsgPair_0218[2] = { Bank0399_Text_IfWantSomethingJust, Bank0399_Text_IfWantSomethingDont };

static const u16 sMsgPair_0220[2] = { Bank0399_Text_ThereEggCannotTraded, Bank0399_Text_ThereEggCannotTraded_2 };

static const u16 sMsgPair_021c[2] = { Bank0399_Text_OkJustGiveSecond_3, Bank0399_Text_OkJustGiveSecond_4 };

static const u16 sMsgPair_0228[2] = { Bank0399_Text_WereDrawingRightNow, Bank0399_Text_WereDrawingRightNow_2 };

static const u16 sMsgPair_01f4[2] = { Bank0399_Text_SorryMakeWaitOk, Bank0399_Text_SorryKeepWaitingOk };

static const u16 sMsgPair_01e0[2] = { Bank0399_Text_OkJustGiveSecond, Bank0399_Text_OkJustGiveSecond_2 };

static const u16 sMsgPair_0224[2] = { Bank0399_Text_TrainerAppearsBusy, Bank0399_Text_TrainerAppearsBusy_2 };

static const u16 sMsgPair_0204[2] = { Bank0399_Text_DidNotReply, Bank0399_Text_DidNotReply_2 };

static const u16 sMsgPair_01f8[2] = { Bank0399_Text_WereGoingSpinTrade, Bank0399_Text_WeWereDoingSpin };

static const u16 sMsgPair_0230[2] = { Bank0399_Text_WhoopsSorryButHave, Bank0399_Text_WhoopsSorryButHave_2 };

static const u16 sMsgPair_020c[2] = { Bank0399_Text_ThereEggCannotTraded, Bank0399_Text_ThereEggCannotTraded_2 };

static const u16 sMsgPair_01ec[2] = { Bank0399_Text_OhAllRightCome_2, Bank0399_Text_OhPleaseComeBy_2 };

static const u16 sMsgPair_0200[2] = { Bank0399_Text_NeedEggIfWant, Bank0399_Text_NeedEggIfWant_2 };

static const u16 sMsgPair_01e8[2] = { Bank0399_Text_WhatWouldLike, Bank0399_Text_WhatWouldLike_2 };

static const u16 sMsgPair_022c[2] = { Bank0399_Text_OhAllRightCome, Bank0399_Text_OhPleaseComeBy };

static const u16 sMsgPair_01fc[2] = { Bank0399_Text_WereAboutHaveMulti, Bank0399_Text_WereAboutHaveMulti_2 };

static const u16 sMsgPair_01e4[2] = { Bank0399_Text_OhImSorryHave_3, Bank0399_Text_OhSorryLetsAnother };

static const u16 sMsgPair_01d8[2] = { Bank0399_Text_OkLetsSomeDrawing, Bank0399_Text_OkThanksWaitingLets };

static const u16 sMsgPair_01dc[2] = { Bank0399_Text_OhImSorryHave, Bank0399_Text_OhImSorryHave_2 };

static const u16 sMsgPair_0244[2] = { Bank0399_Text_OkJustGiveSecond_5, Bank0399_Text_OkJustGiveSecond_6 };

static const u16 sMsgPair_0240[2] = { Bank0399_Text_OkLetsSpinTrade, Bank0399_Text_OkLetsSpinTrade_2 };

static const u16 sMsgPair_023c[2] = { Bank0399_Text_OhImSorryHave_5, Bank0399_Text_OhSorryLetsAnother_3 };

static const u16 sMsgPair_0238[2] = { Bank0399_Text_WhatsLooksLikeDont, Bank0399_Text_WhatsLooksLikeDont_2 };

static const u16 sMsgPair_0234[2] = { Bank0399_Text_ThereSomethingWanted, Bank0399_Text_HelloItsWouldLike };

static const u16 sMsgPairs_0248[2][2] = {
    { Bank0399_Text_OhYeahReallyWanted, Bank0399_Text_ImTradingPokemonRight },
    { Bank0399_Text_FinallyGotPokemonIve, Bank0399_Text_FinallyGotPokemonWanted },
};

static const u16 sMsgPairs_0250[2][2] = {
    { Bank0399_Text_ImJustShowingTrainer, Bank0399_Text_WereShowingEachOther },
    { Bank0399_Text_HopeGetKnowBetter, Bank0399_Text_GladMeetPleaseDont },
};

static const u16 sMsgPairs_0258[4][2] = {
    { Bank0399_Text_LastLetterWhatDraw, Bank0399_Text_WowYoureReallyGood },
    { Bank0399_Text_StopDrawingFunnyFaces, Bank0399_Text_AhahaWhat },
    { Bank0399_Text_WowYoureGoodIll, Bank0399_Text_LoveDrawingCanDraw },
    { Bank0399_Text_UhHuhWhatSupposed, Bank0399_Text_OopsCantStayInside },
};

static const u16 sMsgPairs_0268[4][2] = {
    { Bank0399_Text_WhoaCanTellYoure, Bank0399_Text_PokemonItsBeenRaised },
    { Bank0399_Text_UsedMoveThatsGood, Bank0399_Text_ThatsRightMoveNow },
    { Bank0399_Text_WayGoEyeOpener, Bank0399_Text_ThatsAwesomeIveNever },
    { Bank0399_Text_OhHowCouldUse, Bank0399_Text_HaveExquisiteTimingSwitching },
};

static const u16 sMsgPairs_0278[4][2] = {
    { Bank0399_Text_SpinTradesMuchFun, Bank0399_Text_WereDoingSpinTrades },
    { Bank0399_Text_ThereStopThere, Bank0399_Text_WahMovedAgain },
    { Bank0399_Text_EggsAwesome, Bank0399_Text_ThereGoesEggWanted },
    { Bank0399_Text_YesGotEggWanted, Bank0399_Text_IfStoppedThereWouldve },
};

static const u16 sMsgPairs_04cc[25][2] = {
    { Bank0399_Text_SureThingLetShow, Bank0399_Text_HiLetIntroduceMyself },
    { Bank0399_Text_OhDidWantDraw, Bank0399_Text_WantDrawSureJust },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_BattleCourseButNeed, Bank0399_Text_BattleCourseButNeed_2 },
    { Bank0399_Text_WantTradeHuhSure, Bank0399_Text_TradeCourseButNeed },
    { Bank0399_Text_SpinTradeSureOk, Bank0399_Text_SpinTradeCourseOk },
    { Bank0399_Text_SpinTradeSureOk, Bank0399_Text_SpinTradeCourseOk },
};

static const u16 sMsgPairs_0530[25][2] = {
    { Bank0399_Text_HuhTrainerCardWhered, Bank0399_Text_HuhTrainerCardWhered_2 },
    { Bank0399_Text_OhSorryCantNow, Bank0399_Text_OhImSorryLets },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryPokemonDontSeem, Bank0399_Text_ImTerriblySorryBut },
    { Bank0399_Text_SorryDontHavePokemon, Bank0399_Text_ImSorryButDont },
    { Bank0399_Text_OhImSorryHave_4, Bank0399_Text_OhSorryLetsAnother_2 },
    { Bank0399_Text_Empty_20, Bank0399_Text_Empty_21 },
};

static const u16 sTable_0424[26] = {
    11, 18, 22, 22, 23, 23, 25, 25, 26, 26, 28, 28, 29, 29, 31, 31, 32, 32, 33, 33, 34, 34, 37, 44, 41, 0
};

static const u16 sTable_0458[26] = {
    17, 20, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 39, 46, 46, 0
};

static const UnionMenuRow sMenu_029c[3] = {
    { Bank0399_Text_NoRestrictions, { 0x0 } },
    { Bank0399_Text_FlatBattle, { 0x1 } },
    { Bank0399_Text_Cancel_6, { .sub = sMenu_03f4 } },
};

static const UnionMenuRow sMenu_02b4[3] = {
    { Bank0399_Text_WonderLauncher, { 0x0 } },
    { Bank0399_Text_WonderLauncherOff, { 0x1 } },
    { Bank0399_Text_Cancel_6, { .sub = sMenu_03f4 } },
};

static const UnionMenuRow sMenu_02cc[3] = {
    { Bank0399_Text_BattlesTwo, { .sub = sMenu_03f4 } },
    { Bank0399_Text_BattlesFour, { .sub = sMenu_039c } },
    { Bank0399_Text_Cancel_4, { BMPMENULIST_CANCEL } },
};

static const UnionMenuRow sMenu_02e4[3] = {
    { Bank0399_Text_NoRestrictions, { 0x0 } },
    { Bank0399_Text_FlatBattle, { 0x1 } },
    { Bank0399_Text_Cancel_6, { .sub = sMenu_039c } },
};

static const UnionMenuRow sMenu_02fc[3] = {
    { Bank0399_Text_WonderLauncher, { 0x0 } },
    { Bank0399_Text_WonderLauncherOff, { 0x1 } },
    { Bank0399_Text_Cancel_6, { .sub = sMenu_039c } },
};

static const UnionMenuRow sMenu_0374[5] = {
    { Bank0399_Text_SingleBattle, { 0x0 } },
    { Bank0399_Text_DoubleBattle, { 0x1 } },
    { Bank0399_Text_TripleBattle, { 0x2 } },
    { Bank0399_Text_RotationBattle, { 0x3 } },
    { Bank0399_Text_Cancel_5, { .sub = sMenu_03f4 } },
};

static const UnionMenuRow sMenu_039c[5] = {
    { Bank0399_Text_DoubleBattle, { .sub = sMenu_02e4 } },
    { Bank0399_Text_TripleBattle, { .sub = sMenu_02fc } },
    { Bank0399_Text_Confirm, { 0x64 } },
    { Bank0399_Text_Rules, { 0x96 } },
    { Bank0399_Text_Cancel_5, { .sub = sMenu_02cc } },
};

static const UnionMenuRow sMenu_03f4[6] = {
    { Bank0399_Text_SingleBattle, { .sub = sMenu_0374 } },
    { Bank0399_Text_DoubleBattle, { .sub = sMenu_029c } },
    { Bank0399_Text_TripleBattle, { .sub = sMenu_02b4 } },
    { Bank0399_Text_Confirm, { 0x64 } },
    { Bank0399_Text_Rules, { 0x96 } },
    { Bank0399_Text_Cancel_5, { .sub = sMenu_02cc } },
};

static const UnionMenuInfo sMenuInfo[8] = {
    { sMenu_02cc, 3 }, { sMenu_03f4, 6 }, { sMenu_0374, 5 }, { sMenu_029c, 3 },
    { sMenu_02b4, 3 }, { sMenu_039c, 5 }, { sMenu_02e4, 3 }, { sMenu_02fc, 3 },
};

// The party and the rules a menu's choices lead to, by the battle format and the two options
static const u8 sRuleResult[5][2][2] = {
    { { 4, 5 }, { 6, 7 } },     { { 8, 9 }, { 10, 11 } },   { { 12, 13 }, { 14, 15 } },
    { { 16, 17 }, { 18, 19 } }, { { 20, 21 }, { 22, 23 } },
};

static const UnionMenuRow sMenuMain[6] = {
    { Bank0399_Text_Greet, { 2 } },  { Bank0399_Text_Battle, { 0x1b } }, { Bank0399_Text_Trade, { 0x18 } },
    { Bank0399_Text_Draw, { 3 } },   { Bank0399_Text_Spin, { 0x19 } },   { Bank0399_Text_Cancel, { 0 } },
};

static const u32 sLanguageFlags[8] = { 0, 1, 2, 3, 4, 7, 5, 6 };

static const ListMenuRequest sMenuMainRequest = { 6, 6, 0, 0x0d, 0x1000, 0x2f, 0, 0, 0, 1, 0x0c, 0x10, 1, 1, 0x0d, 0x0c, 0 };

static const ListMenuRequest sMenuRulesRequest = { 3, 3, 0, 0x0d, 0x1000, 0x2f, 0, 0, 0, 1, 0x0c, 0x10, 1, 1, 0x13, 0x0e, 0 };

static const struct {
    u8 value;
    u8 unk01[3];
} sMsgIndexTable[27] = {
    { 2 }, { 2 }, { 2 }, { 5 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 4 }, { 4 }, { 4 }, { 4 }, { 2 }, { 5 }, { 5 },
};

u16 func_ov034_0217aa7c(UnionSystem *unisys)
{
    return sMsgPair_0204[getTrainerGender(unisys->param->playerInfo)];
}

u16 func_ov034_0217aa94(u8 gender)
{
    return sMsgPair_0234[gender];
}

u16 func_ov034_0217aaa0(u8 gender)
{
    return sMsgPair_0218[gender];
}

u16 func_ov034_0217aaac(u8 gender)
{
    return sMsgPair_0230[gender];
}

u16 func_ov034_0217aab8(u32 row, u8 gender)
{
    return sMsgPairs_04cc[row - 2][gender];
}

u16 func_ov034_0217aacc(u32 row, u8 gender)
{
    return sMsgPairs_0530[row - 2][gender];
}

u16 func_ov034_0217aae0(u32 row)
{
    return sTable_0424[row - 2];
}

u16 func_ov034_0217aaf0(u32 row)
{
    return sTable_0458[row - 2];
}

u16 func_ov034_0217ab00(u8 gender)
{
    return sMsgPair_01e8[gender];
}

u16 func_ov034_0217ab0c(u8 gender)
{
    return sMsgPair_0214[gender];
}

u16 func_ov034_0217ab18(u8 gender)
{
    return sMsgPair_0208[gender];
}

u16 func_ov034_0217ab24(u8 gender)
{
    return sMsgPair_0200[gender];
}

u16 func_ov034_0217ab30(UnionSystem *unisys)
{
    return sMsgPair_0210[getTrainerGender(unisys->param->playerInfo)];
}

u16 func_ov034_0217ab48(UnionSystem *unisys)
{
    return sMsgPair_020c[getTrainerGender(unisys->param->playerInfo)];
}

u16 func_ov034_0217ab60(u8 gender)
{
    return sMsgPairs_0268[GFL_RandomLC(4)][gender];
}

u16 func_ov034_0217ab7c(u8 gender)
{
    return sMsgPair_0224[gender];
}

u16 func_ov034_0217ab88(u8 gender)
{
    return sMsgPairs_0250[GFL_RandomLC(2)][gender];
}

u16 func_ov034_0217aba4(u8 gender)
{
    return sMsgPairs_0258[GFL_RandomLC(4)][gender];
}

u16 func_ov034_0217abc0(u8 gender)
{
    return sMsgPairs_0248[GFL_RandomLC(2)][gender];
}

u16 func_ov034_0217abdc(u8 gender)
{
    return sMsgPairs_0278[GFL_RandomLC(4)][gender];
}

u16 func_ov034_0217abf8(u8 gender)
{
    return sMsgPair_01fc[gender];
}

u16 func_ov034_0217ac04(u8 gender)
{
    return sMsgPair_01f8[gender];
}

u16 func_ov034_0217ac10(u8 gender)
{
    return sMsgPair_0228[gender];
}

u16 func_ov034_0217ac1c(u8 gender)
{
    return sMsgPair_01ec[gender];
}

u16 func_ov034_0217ac28(u8 gender)
{
    return sMsgPair_01f0[gender];
}

u16 func_ov034_0217ac34(u8 gender)
{
    return sMsgPair_022c[gender];
}

u16 func_ov034_0217ac40(u8 gender)
{
    return sMsgPair_0238[gender];
}

u16 func_ov034_0217ac4c(u8 gender)
{
    return sMsgPair_0220[gender];
}

u16 func_ov034_0217ac58(u8 gender)
{
    return sMsgPair_0244[gender];
}

u16 func_ov034_0217ac64(u8 gender)
{
    return sMsgPair_0240[gender];
}

u16 func_ov034_0217ac70(u8 gender)
{
    return sMsgPair_023c[gender];
}

u16 func_ov034_0217ac7c(u8 gender)
{
    return sMsgPair_01e0[gender];
}

u16 func_ov034_0217ac88(u8 gender)
{
    return sMsgPair_01d8[gender];
}

u16 func_ov034_0217ac94(u8 gender)
{
    return sMsgPair_01dc[gender];
}

u16 func_ov034_0217aca0(u8 gender)
{
    return sMsgPair_021c[gender];
}

u16 func_ov034_0217acac(u8 gender)
{
    return sMsgPair_01f4[gender];
}

u16 func_ov034_0217acb8(u8 gender)
{
    return sMsgPair_01e4[gender];
}

u32 UnionMsg_GetOriginMessage(UnionSystem *unisys, u8 country, u8 province)
{
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(unisys->param->gameData);
    u8 myCountry = UnityTowerVisitor_GetCountry(playerInfo);
    u8 myProvince = UnityTowerVisitor_GetProvince(playerInfo);

    if (country == 0) {
        return Bank0399_Text_STrainerCard;
    }
    if (myCountry != country) {
        if (province == 0) {
            return Bank0399_Text_TrainerCardWhoLives_2;
        }
        return Bank0399_Text_TrainerCardWhoLives;
    }
    if (province == 0 || myProvince == province) {
        return Bank0399_Text_STrainerCard;
    }
    return Bank0399_Text_TrainerCardWhoLives_3;
}

// The words of the message system: the player or the one spoken to, their greeting by language and their origin
static void UnionMsg_SetupWordSet(UnionSystem *unisys)
{
    UnionSelf *situ = &unisys->situ;
    PlayerInfo *playerInfo = unisys->param->playerInfo;
    const u16 *name;
    u8 language;
    u8 gender;
    StrBuf *strbuf;
    u32 messageId;
    UnionBeaconEntry *target;

    GFL_ASSERT(unisys->msgdata != NULL);
    GFL_ASSERT(unisys->wordset != NULL);

    target = situ->mycomm.target;
    if (target != NULL) {
        name = target->beacon.name;
        language = target->beacon.language;
        gender = target->beacon.gender;
    } else {
        name = GetPlayerName(playerInfo);
        language = LANGUAGE_ENGLISH;
        gender = getTrainerGender(playerInfo);
    }

    strbuf = GFL_StrBufCreate(0x20, UNION_HEAPID);
    GFL_StrBufLoadString(strbuf, name);
    func_0202437c(unisys->wordset, 0, strbuf, gender, TRUE, language);
    GFL_StrBufFree(strbuf);
    copyVarForText(unisys->wordset, 1, playerInfo);

    switch (language) {
    case LANGUAGE_JAPANESE:
        messageId = Bank0399_Text_Empty_71;
        break;
    case LANGUAGE_ENGLISH:
        messageId = Bank0399_Text_Hello;
        break;
    case LANGUAGE_FRENCH:
        messageId = Bank0399_Text_Bonjour;
        break;
    case LANGUAGE_ITALIAN:
        messageId = Bank0399_Text_Ciao;
        break;
    case LANGUAGE_GERMAN:
        messageId = Bank0399_Text_Hallo;
        break;
    case LANGUAGE_SPANISH:
        messageId = Bank0399_Text_Hola;
        break;
    case LANGUAGE_KOREAN:
        messageId = Bank0399_Text_Empty_72;
        break;
    default:
        messageId = Bank0399_Text_HELL;
        break;
    }
    strbuf = GFL_MsgDataLoadStrbufNew(unisys->msgdata, messageId);
    func_0202437c(unisys->wordset, 2, strbuf, gender, TRUE, language);
    GFL_StrBufFree(strbuf);

    if (situ->mycomm.target != NULL && situ->mycomm.target->beacon.country != 0) {
        loadCountryToStrbuf(unisys->wordset, 3, situ->mycomm.target->beacon.country);
        if (situ->mycomm.target->beacon.province != 0) {
            loadCountryAreaToStrbuf(unisys->wordset, 4, situ->mycomm.target->beacon.country,
                                    situ->mycomm.target->beacon.province);
        }
    }
    UnionMsg_SetLanguageFlag(unisys->param->gameData, language);
}

// Marks the language as seen in the player's Pokemon Wonder word list
static void UnionMsg_SetLanguageFlag(GameData *gameData, u32 language)
{
    PMSWordSave *save = getDexBlkAddress(GameData_GetSaveControl(gameData));
    u16 index = language - 1;

    if (language >= 1 && language <= 8) {
        if (index < 8) {
            PMSWordSave_SetLanguageFlag(save, sLanguageFlags[index]);
        } else {
            PMSWordSave_SetLanguageFlag(save, 7);
        }
    } else {
        PMSWordSave_SetLanguageFlag(save, 7);
    }
}

static void UnionMsg_CreateWordSet(UnionSystem *unisys)
{
    if (unisys->wordset == NULL) {
        unisys->wordset = GFL_WordSetSystemCreate(5, 0x40, UNION_HEAPID);
        UnionMsg_SetupWordSet(unisys);
    }
}

static void UnionMsg_FreeWordSet(UnionSystem *unisys)
{
    if (unisys->wordset != NULL) {
        GFL_WordSetSystemFree(unisys->wordset);
        unisys->wordset = NULL;
    }
}

s32 UnionMsg_GetTableByte(s32 index)
{
    if (index >= 27) {
        return 0;
    }
    return sMsgIndexTable[index].value;
}

void UnionMsg_FreeAll(UnionSystem *unisys)
{
    UnionMsg_FreeRulesWindow(unisys);
    UnionMsg_FreeInfoWindow(unisys);
    UnionMsg_FreeYesNo(unisys);
    UnionMsg_FreeMenu(unisys);
    UnionMsg_FreeMsgSystem(unisys);
}

void UnionMsg_CreateMsgSystem(UnionSystem *unisys, Field *field)
{
    void *fldmsg_bg;

    if (unisys->fld_msgwin_stream == NULL) {
        GFL_ASSERT(unisys->msgdata == NULL);
        fldmsg_bg = Field_GetMsgBGSys(field);
        unisys->msgdata = func_ov036_021879a0(fldmsg_bg, TEXT_BANK_0399);
        unisys->msgbuf = GFL_StrBufCreate(0x100, UNION_HEAPID);
        unisys->fmtbuf = GFL_StrBufCreate(0x100, UNION_HEAPID);
        UnionMsg_CreateWordSet(unisys);
        unisys->fld_msgwin_stream = func_ov036_02188a54(fldmsg_bg, (u32)unisys->msgdata, 0, 1, 0x13, 0x1e, 4);
    } else {
        func_ov036_02188ae8(unisys->fld_msgwin_stream);
    }
}

static void UnionMsg_FreeMsgSystem(UnionSystem *unisys)
{
    if (unisys->fld_msgwin_stream != NULL) {
        func_ov036_02188b08(unisys->fld_msgwin_stream);
        func_ov036_02188ab0(unisys->fld_msgwin_stream);
        unisys->fld_msgwin_stream = NULL;
        unisys->msgPrinting = 0;
        GFL_StrBufFree(unisys->msgbuf);
        GFL_StrBufFree(unisys->fmtbuf);
        unisys->msgbuf = NULL;
        unisys->fmtbuf = NULL;
    }
    if (unisys->msgdata != NULL) {
        UnionMsg_FreeWordSet(unisys);
        func_ov036_021879b8(unisys->msgdata);
        unisys->msgdata = NULL;
    }
}

void UnionMsg_PrintMessage(UnionSystem *unisys, u32 messageId)
{
    GFL_ASSERT(unisys->fld_msgwin_stream != NULL);
    GFL_MsgDataLoadStrbuf(unisys->msgdata, messageId, unisys->msgbuf);
    GFL_WordSetFormatStrbuf(unisys->wordset, unisys->fmtbuf, unisys->msgbuf);
    func_ov036_02188ba4(unisys->fld_msgwin_stream, 0, 0, unisys->fmtbuf);
    unisys->msgPrinting = 1;
}

BOOL UnionMsg_IsPrintDone(UnionSystem *unisys)
{
    if (unisys->fld_msgwin_stream == NULL || unisys->msgPrinting == 0) {
        return TRUE;
    }
    return func_ov036_02188bdc(unisys->fld_msgwin_stream);
}

void UnionMsg_CreateAndPrint(UnionSystem *unisys, Field *field, u32 messageId)
{
    UnionMsg_CreateMsgSystem(unisys, field);
    UnionMsg_SetupWordSet(unisys);
    UnionMsg_PrintMessage(unisys, messageId);
}

void UnionMsg_CreateYesNo(UnionSystem *unisys, Field *field)
{
    if (unisys->fldmenu_yesno_func == NULL) {
        unisys->fldmenu_yesno_func = func_ov036_021880d4(Field_GetMsgBGSys(field), 0);
    }
}

void UnionMsg_FreeYesNo(UnionSystem *unisys)
{
    if (unisys->fldmenu_yesno_func != NULL) {
        func_ov036_02187ea0(unisys->fldmenu_yesno_func);
        unisys->fldmenu_yesno_func = NULL;
    }
}

BOOL UnionMsg_UpdateYesNo(UnionSystem *unisys, BOOL *yes)
{
    u32 answer;

    GFL_ASSERT(unisys->fldmenu_yesno_func != NULL);
    answer = func_ov036_0218816c(unisys->fldmenu_yesno_func);
    if (answer == 2) {
        return FALSE;
    }
    *yes = answer == 0;
    return TRUE;
}

static void UnionMsg_CreateMenu(UnionSystem *unisys, Field *field, const UnionMenuRow *rows, u32 count,
                                ListMenuRequest *request)
{
    void *msgBGSys = Field_GetMsgBGSys(field);

    if (unisys->fldmenu_func == NULL) {
        ListMenuOption *options = func_ov036_02187f0c(rows, count, unisys->msgdata, UNION_HEAPID);

        unisys->fldmenu_func = ListMenuUI_Create(msgBGSys, request, options, NULL, NULL, 0, 0, TRUE);
    }
}

static void UnionMsg_FreeMenu(UnionSystem *unisys)
{
    if (unisys->fldmenu_func != NULL) {
        func_ov036_02187ea0(unisys->fldmenu_func);
        unisys->fldmenu_func = NULL;
    }
}

static s32 UnionMsg_UpdateMenu(UnionSystem *unisys, Field *field)
{
    void *fldmsg_bg = Field_GetMsgBGSys(field);

    GFL_ASSERT(fldmsg_bg != NULL);
    if (fldmsg_bg != NULL && !func_ov036_0218798c(fldmsg_bg)) {
        return BMPMENULIST_NULL;
    }
    GFL_ASSERT(unisys->fldmenu_func != NULL);
    return ListMenuUI_Update(unisys->fldmenu_func);
}

void UnionMsg_CreateMainMenu(UnionSystem *unisys, Field *field)
{
    UnionMsg_CreateMenu(unisys, field, sMenuMain, 6, (ListMenuRequest *)&sMenuMainRequest);
}

void UnionMsg_FreeMainMenu(UnionSystem *unisys)
{
    UnionMsg_FreeMenu(unisys);
}

s32 UnionMsg_UpdateMainMenu(UnionSystem *unisys, Field *field)
{
    return UnionMsg_UpdateMenu(unisys, field);
}

// Opens the menu of the battle rules: the kind says which of sMenuInfo's, and which choices the rows show
void UnionMsg_CreateRuleMenu(UnionSystem *unisys, Field *field, int kind, u8 *ruleSel)
{
    UnionMenuRow *rows;
    u8 count;

    ruleSel[3] = kind;
    ListMenuRequest request = sMenuRulesRequest;
    count = sMenuInfo[kind].count;
    request.count = count;
    request.unk02 = count;
    request.height = request.count * 2;
    if (kind == 1 || kind == 5) {
        request.width = 30;
    }
    if (kind >= 5) {
        request.y = 5;
    }

    if (kind == 1) {
        rows = GFL_HeapAllocate(UNION_HEAPID, sizeof(UnionMenuRow) * 6, TRUE, "union_msg.c", 0x647);
        sys_memcpy(sMenu_03f4, rows, sizeof(UnionMenuRow) * 6);
        rows[0].msg = Bank0399_Text_BattleFormatSingleBattle + ruleSel[0];
        rows[1].msg = Bank0399_Text_BattleRulesNoRestrictions + ruleSel[1];
        rows[2].msg = Bank0399_Text_WonderLauncher_2 + ruleSel[2];
        UnionMsg_CreateMenu(unisys, field, rows, count, &request);
        GFL_HeapFree(rows);
    } else if (kind == 5) {
        rows = GFL_HeapAllocate(UNION_HEAPID, sizeof(UnionMenuRow) * 5, TRUE, "union_msg.c", 0x657);
        sys_memcpy(sMenu_039c, rows, sizeof(UnionMenuRow) * 5);
        rows[0].msg = Bank0399_Text_BattleRulesNoRestrictions + ruleSel[1];
        rows[1].msg = Bank0399_Text_WonderLauncher_2 + ruleSel[2];
        UnionMsg_CreateMenu(unisys, field, rows, count, &request);
        GFL_HeapFree(rows);
    } else {
        UnionMsg_CreateMenu(unisys, field, sMenuInfo[kind].rows, count, &request);
    }
}

void UnionMsg_FreeRuleMenu(UnionSystem *unisys)
{
    UnionMsg_FreeMenu(unisys);
}

// Whether the number is one of sMenuInfo's menus
static inline BOOL UnionMsg_IsMenuKind(u32 kind)
{
    if (kind < NELEMS(sMenuInfo)) {
        return TRUE;
    }
    return FALSE;
}

// The entry of sMenuInfo for a menu kind
static inline const UnionMenuInfo *UnionMsg_GetMenuInfo(int kind)
{
    return &sMenuInfo[kind];
}

// Runs the menu of the battle rules and takes the choice: the value is a submenu or a rule. picked says the choice
// changed the rules or opened a menu, isFinal that the rules are confirmed
u32 UnionMsg_UpdateRuleMenu(UnionSystem *unisys, BOOL *picked, u8 *ruleSel, BOOL *isFinal, Field *field)
{
    u32 ret;
    u32 i;

    *picked = FALSE;
    *isFinal = FALSE;
    ret = UnionMsg_UpdateMenu(unisys, field);

    switch (ret) {
    case BMPMENULIST_NULL:
        return ret;
    case BMPMENULIST_CANCEL:
        if (ruleSel[3] == 0) {
            return ret;
        }
        // The last row goes back to the menu that opened this one
        ret = sMenuInfo[ruleSel[3]].rows[UnionMsg_GetMenuInfo(ruleSel[3])->count - 1].value;
        break;
    case 0x64:
        return sRuleResult[ruleSel[0]][ruleSel[1]][ruleSel[2]];
    case 0x96:
        *isFinal = TRUE;
        return sRuleResult[ruleSel[0]][ruleSel[1]][ruleSel[2]];
    }

    if (ruleSel[3] == 0) {
        if (ret == (u32)sMenu_03f4) {
            if (ruleSel[0] == 4) {
                ruleSel[0] = 0;
            }
        } else if (ret == (u32)sMenu_039c) {
            ruleSel[0] = 4;
        }
    }

    for (i = 0; UnionMsg_IsMenuKind(i); i++) {
        if (UnionMsg_GetMenuInfo(i)->rows == (const UnionMenuRow *)ret) {
            *picked = TRUE;
            return i;
        }
        switch (ruleSel[3]) {
        case 2:
            if (ret < 5) {
                ruleSel[0] = ret;
                *picked = TRUE;
                return TRUE;
            }
            break;
        case 3:
        case 6:
            if (ret < 2) {
                ruleSel[1] = ret;
                *picked = TRUE;
                if (ruleSel[3] == 6) {
                    return 5;
                }
                return TRUE;
            }
            break;
        case 4:
        case 7:
            if (ret < 2) {
                ruleSel[2] = ret;
                *picked = TRUE;
                if (ruleSel[3] == 7) {
                    return 5;
                }
                return TRUE;
            }
            break;
        }
    }
    GFL_ASSERT_MSG(FALSE, "menu_ret=%d\n", ret);
    return 0;
}

void UnionMsg_CreateInfoWindow(UnionSystem *unisys, Field *field)
{
    if (unisys->fldmsgwin == NULL) {
        void *msgBGSys = Field_GetMsgBGSys(field);

        unisys->fldmsgwin = func_ov036_02188a54(msgBGSys, (u32)unisys->msgdata, 0, 1, 1, 0x1e, 2);
        func_ov036_02188b18(unisys->fldmsgwin, 0, 0, Bank0399_Text_BattleFormatMultiBattle);
    }
}

void UnionMsg_FreeInfoWindow(UnionSystem *unisys)
{
    if (unisys->fldmsgwin != NULL) {
        func_ov036_02188ab0(unisys->fldmsgwin);
        unisys->fldmsgwin = NULL;
    }
}

// Opens the window that lists the regulation's rules, each in the color of whether the party meets it
void UnionMsg_CreateRulesWindow(UnionSystem *unisys, Field *field, u32 okMask, u32 battleType, u32 mode)
{
    void *msgBGSys = Field_GetMsgBGSys(field);
    BOOL isMulti;
    int i;

    GFL_ASSERT(unisys->wordset != NULL);
    GFL_ASSERT(unisys->fldmsgwin == NULL);
    GFL_ASSERT(unisys->alloc.rpm == NULL);

    isMulti = TRUE;
    if (battleType != 1) {
        isMulti = FALSE;
    }
    unisys->fldmsgwin = func_ov036_02188a54(msgBGSys, (u32)unisys->msgdata, 0, 1, 1, 0x1a, 0x10);
    unisys->alloc.rpm = PokeRegulation_CreatePrintMsg(unisys->alloc.regulation, unisys->wordset, UNION_HEAPID, isMulti);
    UnionMsg_SetupWordSet(unisys);

    if (mode == 0) {
        StrBuf *title = makeStrbufLoadBattleTypeText(unisys->alloc.regulation, UNION_HEAPID);

        func_ov036_02188b44(unisys->fldmsgwin, 0, 0, title);
        GFL_StrBufFree(title);
    } else {
        func_ov036_02188b18(unisys->fldmsgwin, 0, 0, mode == 1 ? Bank0399_Text_Party : Bank0399_Text_BattleBox);
    }

    for (i = 0; i < 6; i++) {
        RegulationRulesText *rpm;
        u32 color;

        if (okMask & (1 << i)) {
            color = PRINT_COLOR(4, 2, 0);
        } else {
            color = PRINT_COLOR(1, 2, 0);
        }

        rpm = unisys->alloc.rpm;
        func_ov036_02188b70(unisys->fldmsgwin, 0, (i * 2 + 4) * 8, rpm->rule[i], color);
        rpm = unisys->alloc.rpm;
        func_ov036_02188b70(unisys->fldmsgwin, 0x70, (i * 2 + 4) * 8, rpm->value[i], color);
    }
}

BOOL UnionMsg_IsRulesPrinted(UnionSystem *unisys, Field *field)
{
    void *queue = func_ov036_02187998(Field_GetMsgBGSys(field));

    if (unisys->alloc.rpm == NULL) {
        return TRUE;
    }
    return func_02021c0c(queue);
}

void UnionMsg_FreeRulesWindow(UnionSystem *unisys)
{
    if (unisys->alloc.rpm != NULL) {
        func_ov036_02188ab0(unisys->fldmsgwin);
        unisys->fldmsgwin = NULL;
        PokeRegulation_FreePrintMsg(unisys->alloc.rpm);
        unisys->alloc.rpm = NULL;
    }
}
