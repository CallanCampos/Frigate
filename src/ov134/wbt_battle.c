#include "field/wbt_battle.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/trainer_classes.h"
#include "constants/trainers.h"
#include "constants/vars.h"
#include "field/battle_facility.h"
#include "field/bsubway_scr.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/wbt.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/str.h"
#include "nitro/mi.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "save/wbt_save.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/pms_data.h"
#include "system/wordset.h"
#include "battle/btl_setup.h"
#include "battle/trainer_data.h"
#include "gfl/bmpwin.h"

// The Pokémon World Tournament's rounds, opponents and battles, in overlay 134, which names this file

// The stadium's list of the player's Pokémon, which the script shows while the player picks which of them to send
typedef struct {
    FieldScriptEnv *env;
    WbtSystem *sys;
    void *window;
    void *msgBGSys;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *mainStrBuf;
    StrBuf *altStrBuf;
} WbtPartyList;

// The header of a file of a Trainer's Pokémon in the Battle Subway's arc
typedef struct {
    u16 unk0;
    u16 count;
    u16 files[];
} WbtTrainerFile;

// The message of the Trainers' lines that each of the 78 Trainers' sets starts at, for the tournaments 12 and 13
// and for the others
static const u16 sTrainerLines[78][2] = {
    { 0x000, 0x270 }, { 0x008, 0x278 }, { 0x010, 0x280 }, { 0x018, 0x288 }, { 0x020, 0x290 }, { 0x028, 0x298 },
    { 0x030, 0x2a0 }, { 0x038, 0x2a8 }, { 0x040, 0x2b0 }, { 0x048, 0x2b8 }, { 0x050, 0x2c0 }, { 0x058, 0x2c8 },
    { 0x060, 0x2d0 }, { 0x068, 0x2d8 }, { 0x070, 0x070 }, { 0x078, 0x078 }, { 0x080, 0x080 }, { 0x088, 0x088 },
    { 0x090, 0x090 }, { 0x098, 0x098 }, { 0x0a0, 0x2e0 }, { 0x0a8, 0x2e8 }, { 0x0b0, 0x2f0 }, { 0x0b8, 0x2f8 },
    { 0x0c0, 0x300 }, { 0x0c8, 0x308 }, { 0x0d0, 0x310 }, { 0x0d8, 0x318 }, { 0x0e0, 0x320 }, { 0x0e8, 0x328 },
    { 0x0f0, 0x330 }, { 0x0f8, 0x338 }, { 0x100, 0x340 }, { 0x108, 0x348 }, { 0x110, 0x350 }, { 0x118, 0x358 },
    { 0x120, 0x360 }, { 0x128, 0x368 }, { 0x130, 0x370 }, { 0x138, 0x378 }, { 0x140, 0x380 }, { 0x148, 0x388 },
    { 0x150, 0x390 }, { 0x158, 0x398 }, { 0x160, 0x3a0 }, { 0x168, 0x3a8 }, { 0x170, 0x3b0 }, { 0x178, 0x3b8 },
    { 0x180, 0x3c0 }, { 0x188, 0x3c8 }, { 0x190, 0x3d0 }, { 0x198, 0x3d8 }, { 0x1a0, 0x3e0 }, { 0x3e8, 0x3e8 },
    { 0x3e8, 0x3e8 }, { 0x1b8, 0x1b8 }, { 0x1c0, 0x1c0 }, { 0x1c8, 0x1c8 }, { 0x1d0, 0x1d0 }, { 0x1d8, 0x1d8 },
    { 0x1e0, 0x1e0 }, { 0x1e8, 0x1e8 }, { 0x1f0, 0x1f0 }, { 0x1f8, 0x1f8 }, { 0x200, 0x200 }, { 0x208, 0x208 },
    { 0x210, 0x210 }, { 0x218, 0x218 }, { 0x220, 0x220 }, { 0x228, 0x228 }, { 0x230, 0x230 }, { 0x238, 0x238 },
    { 0x240, 0x240 }, { 0x248, 0x248 }, { 0x250, 0x250 }, { 0x258, 0x258 }, { 0x260, 0x260 }, { 0x268, 0x268 },
};

static BOOL WbtBattle_IsDownloadedPokemonUsed(const WbtDownloadedPokemon *pkm);
static void WbtBattle_MakePartyPkm(PartyPkm *pkm, const WbtDownloadedPokemon *src, HeapID heapId);
static void WbtBattle_MakeDownloadedParty(PokeParty *party, WbtDownload *download, u8 round, u32 count, HeapID heapId);
static int WbtBattle_GetRivalTrainerId(GameSystem *gsys);
static void WbtBattle_MakeTournamentPokemon(BSubwayPokemon *dest, u32 tournament, WbtEntrant *entrant, u8 count, HeapID heapId);
static void WbtBattle_MakeTournamentParty(PokeParty *party, WbtEntrant *entrant, u32 tournament, u8 count, HeapID heapId);
static void WbtBattle_SetBattleTrainer(BtlSetupTrainer *trainer, WbtEntrant *entrant);
static int WbtBattle_CountRemainingPokemon(PokeParty *party, const u8 *slots);
static u8 WbtBattle_GetRoundSlot(u32 round);
static void WbtBattle_SetRoundRemaining(WbtSystem *sys, u32 round, u8 remaining);
static void WbtBattle_GetBattleMusic(WbtSystem *sys, u16 *bgm, u16 *winBgm);
static void WbtBattle_LoadTrainerLine(StrBuf *strbuf, HeapID heapId, u32 tournament, WbtEntrant *entrant, u32 index);
static u16 *WbtBattle_GetDownloadedLine(WbtSystem *sys, u8 round, u32 index, u16 *messageSet);

// Swaps the Pokémon that the player and the opponent have picked, in the rounds that the script decides
void WbtBattle_SwapPickedPokemon(WbtSystem *sys) {
    HeapID heapId = HEAPID_TAIL(sys->heapId);
    PartyPkm *tmp = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), FALSE, "wbt_battle.c", 251);
    PartyPkm *first = PokeParty_GetPkm(sys->partyBC, sys->unk13C8[0]);
    PartyPkm *second = PokeParty_GetPkm(sys->partyC0, sys->unk13C8[2]);

    copyPartyPkm(first, tmp);
    copyPartyPkm(second, first);
    copyPartyPkm(tmp, second);
    if (sys->style == 2) {
        first = PokeParty_GetPkm(sys->partyBC, sys->unk13C8[1]);
        second = PokeParty_GetPkm(sys->partyC0, sys->unk13C8[3]);
        copyPartyPkm(first, tmp);
        copyPartyPkm(second, first);
        copyPartyPkm(tmp, second);
    }
    GFL_HeapFree(tmp);
}

void WbtBattle_ResetPicks(WbtSystem *sys) {
    PokeParty_Copy(sys->partyC4, sys->partyBC);
    sys->unk13C8[0] = GFL_RandomLC(func_ov055_021e5d7c(sys));
    sys->unk13C8[1] = 0xff;
    sys->unk13C8[2] = 0xff;
    sys->unk13C8[3] = 0xff;
}

void WbtBattle_SetPick(WbtSystem *sys, u32 index, u8 value) {
    sys->unk13C8[index] = value;
}

void WbtBattle_RandomPick(WbtSystem *sys, u32 a1) {
    u32 count = func_ov055_021e5d7c(sys);
    u8 value;

    switch (a1) {
    case 0:
    case 2:
        value = GFL_RandomLC(count);
        break;
    case 1:
        value = GFL_RandomLC(count - 1);
        if (value >= sys->unk13C8[0]) {
            value = value + 1;
        }
        break;
    case 3:
        value = GFL_RandomLC(count - 1);
        if (value >= sys->unk13C8[2]) {
            value = value + 1;
        }
        break;
    }
    WbtBattle_SetPick(sys, a1, value);
}

u8 WbtBattle_GetPick(WbtSystem *sys, u32 index) {
    return sys->unk13C8[index];
}

void *WbtBattle_CreatePartyList(FieldScriptEnv *env, WbtSystem *sys) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    WbtPartyList *list = GFL_HeapAllocate(FieldScriptEnv_GetHeapID(env), sizeof(WbtPartyList), TRUE, "wbt_battle.c", 387);

    list->env = env;
    list->sys = sys;
    list->msgBGSys = func_ov012_0215518c(env);
    list->msgData = GetFieldScriptMsgData(env);
    list->wordSet = ScriptWork_GetWordSet(scriptWork);
    list->mainStrBuf = ScriptWork_GetMainStrBuf(scriptWork);
    list->altStrBuf = ScriptWork_GetAltStrBuf(scriptWork);
    return list;
}

// Shows the player's Pokémon, with those already picked in a different message
void WbtBattle_ShowPartyList(void *work) {
    WbtPartyList *list = work;
    PokeParty *party = func_ov055_021e5d68(list->sys);
    u16 size = PokeParty_GetPkmCount(party) * 14;
    int count;
    int picked1, picked2;
    int i;

    list->window = FieldMsgBG_CreateMoneyWin(list->msgBGSys, list->msgData, 1, 7, 11, ((size + 7) & ~7) / 8);
    count = func_ov055_021e5d7c(list->sys);
    picked1 = WbtBattle_GetPick(list->sys, 0);
    picked2 = WbtBattle_GetPick(list->sys, 1);
    for (i = 0; i < count; i++) {
        u16 message = 54;
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (i == picked1 || i == picked2) {
            message = 55;
        }
        GFL_MsgDataLoadStrbuf(list->msgData, message, list->altStrBuf);
        setPartyPokemonSpeciesNameToStrbuf(list->wordSet, 0, pkm);
        GFL_WordSetFormatStrbuf(list->wordSet, list->mainStrBuf, list->altStrBuf);
        func_ov036_02187c4c(list->window, 0, 14 * i, list->mainStrBuf);
    }
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(func_ov036_02187c9c(list->window)));
}

void WbtBattle_ClosePartyList(void *work) {
    WbtPartyList *list = work;

    func_ov036_02187c7c(list->window);
    func_ov036_02187c1c(list->window);
    GFL_BGSysLoadScr(1);
}

void WbtBattle_FreePartyList(void *work) {
    GFL_HeapFree(work);
}

static BOOL WbtBattle_IsDownloadedPokemonUsed(const WbtDownloadedPokemon *pkm) {
    if (pkm->species == 0 || pkm->level == 0) {
        return FALSE;
    }
    return TRUE;
}

// Makes a party Pokémon of a Pokémon of a downloaded tournament
static void WbtBattle_MakePartyPkm(PartyPkm *pkm, const WbtDownloadedPokemon *src, HeapID heapId) {
    u8 ability = src->ability;
    u32 ability1 = PML_PersonalGetParamSingle(src->species, src->form, PERSONAL_ABILITY_1);
    u32 ability2 = PML_PersonalGetParamSingle(src->species, src->form, PERSONAL_ABILITY_2);
    u32 hiddenAbility = PML_PersonalGetParamSingle(src->species, src->form, PERSONAL_ABILITY_HIDDEN);
    u32 abilityIndex;
    u32 id;
    u32 pid;
    StrBuf *nickname;
    int i;

    if (ability == ability1) {
        abilityIndex = 0;
    } else if (ability == ability2) {
        abilityIndex = 1;
    } else if (ability == hiddenAbility) {
        abilityIndex = 0;
    } else {
        abilityIndex = 0;
        ability = ability1;
    }
    PokeParty_ClearPkm(pkm);
    id = GFL_RandomLC(0);
    pid = PML_GenPID(id, src->species, src->form, src->ivs.sex, abilityIndex, src->unk1A_0);
    PokeParty_CreatePkm(pkm, src->species, src->level, id, PKM_IVS_RANDOM, pid);
    PokeParty_ChangeForme(pkm, src->form);
    if (ability == hiddenAbility) {
        PokeParty_SetHiddenAbil(pkm, src->species, src->form);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, src->item);
    for (i = 0; i < 4; i++) {
        u8 pp;

        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1 + i, src->moves[i]);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, (u8)((src->ppUps >> (i * 2)) & 3));
        pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_MAX_PP + i, NULL);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + i, pp);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_IV_HP, src->ivs.hp);
    PokeParty_SetParam(pkm, PKM_PARAM_IV_ATTACK, src->ivs.attack);
    PokeParty_SetParam(pkm, PKM_PARAM_IV_DEFENSE, src->ivs.defense);
    PokeParty_SetParam(pkm, PKM_PARAM_IV_SPEED, src->ivs.speed);
    PokeParty_SetParam(pkm, PKM_PARAM_IV_SP_ATTACK, src->ivs.spAttack);
    PokeParty_SetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, src->ivs.spDefense);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP, src->evs[0]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 1, src->evs[1]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 2, src->evs[2]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 3, src->evs[3]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 4, src->evs[4]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 5, src->evs[5]);
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, src->happiness);
    PokeParty_SetNature(pkm, src->nature);
    nickname = GFL_StrBufCreate(11, heapId);
    GFL_StrBufLoadFixedString(nickname, src->nickname, 11);
    PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (u32)nickname);
    GFL_StrBufFree(nickname);
    PokeParty_SetParam(pkm, PKM_PARAM_POKEBALL, src->ball);
    PokeParty_RecalcStats(pkm);
}

// Makes a party of the Pokémon of a downloaded tournament's Trainer, in a random order
static void WbtBattle_MakeDownloadedParty(PokeParty *party, WbtDownload *download, u8 round, u32 count, HeapID heapId) {
    WbtDownloadedPokemon *used[6];
    int usedCount = 0;
    int limit = download->tournament.unk1;
    PartyPkm *pkm;
    WbtDownloadedPokemon *candidates;
    int i;

    MI_CpuClear32(used, sizeof(used));
    candidates = download->trainers[round].pokemon;
    for (i = 0; i < 6; i++) {
        WbtDownloadedPokemon *candidate = &candidates[(u8)i];

        if (WbtBattle_IsDownloadedPokemonUsed(candidate)) {
            used[usedCount] = candidate;
            usedCount++;
        }
    }
    if (usedCount < limit) {
        limit = usedCount;
    }
    while (usedCount != 0) {
        int pick = GFL_RandomLC(usedCount);
        WbtDownloadedPokemon *swap;

        usedCount--;
        swap = used[pick];
        used[pick] = used[usedCount];
        used[usedCount] = swap;
    }
    PokeParty_InitCore(party, 6);
    pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), FALSE, "wbt_battle.c", 631);
    for (i = 0; i < limit; i++) {
        WbtDownloadedPokemon *candidate = used[i];

        if (WbtBattle_IsDownloadedPokemonUsed(candidate)) {
            WbtBattle_MakePartyPkm(pkm, candidate, heapId);
            PokeParty_AddPkm(party, pkm);
        }
    }
    GFL_HeapFree(pkm);
}

// The rival whom the player battles in the tournament, by the starter that the player picked
static int WbtBattle_GetRivalTrainerId(GameSystem *gsys) {
    u16 *starter = EventWork_GetWkPtr(GameData_GetEventWork(GSYS_GetGameData(gsys)), EVENT_WORK_0x4030);

    switch (*starter) {
    case 0:
    default:
        return TRAINER_RIVAL_7;
    case 1:
        return TRAINER_RIVAL_8;
    case 2:
        return TRAINER_RIVAL_9;
    }
}

// Makes the Pokémon that a Trainer of the tournament enters with. A Trainer with the flag first has one Pokémon of
// the tournament's special file, and the rest are picked at random from the files, as the filter allows
static void WbtBattle_MakeTournamentPokemon(BSubwayPokemon *dest, u32 tournament, WbtEntrant *entrant, u8 count, HeapID heapId) {
    WbtTrainerFile *trainerFile;
    u8 iv = func_ov055_021e5e18(entrant);
    u8 trainer = func_ov055_021e5e28(entrant);
    u32 arc = func_ov055_021e67b8(tournament);
    WbtPartyFilter *filter = func_ov055_021e6b58(heapId, tournament);
    u16 files[6];
    int i;

    if (func_ov055_021e5e4c(entrant) == 1) {
        u16 file;

        trainerFile = func_ov012_021627c0(arc, trainer, heapId);
        file = trainerFile->files[0];
        func_ov012_02162490(dest, arc + 2, file, trainer, 0, iv, 0, FALSE, heapId);
        func_ov055_021e6c20(filter, dest->species);
        func_ov055_021e6bfc(filter, dest->item);
        func_ov055_021e6c44(filter, file);
        count--;
        dest++;
        GFL_HeapFree(trainerFile);
    }
    MI_CpuClear16(files, sizeof(files));
    trainerFile = func_ov012_021627c0(arc + 1, trainer, heapId);
    func_ov055_021e6f34(files, filter, heapId, trainerFile->count, trainerFile->files, arc + 2, count, NULL);
    for (i = 0; i < count; i++) {
        func_ov012_02162490(&dest[i], arc + 2, files[i], trainer, 0, iv, i, FALSE, heapId);
    }
    GFL_HeapFree(trainerFile);
    func_ov055_021e6bdc(filter);
}

// Makes a party of a Trainer of the tournament
static void WbtBattle_MakeTournamentParty(PokeParty *party, WbtEntrant *entrant, u32 tournament, u8 count, HeapID heapId) {
    BSubwayPokemon *pokemon =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), 6 * sizeof(BSubwayPokemon), FALSE, "wbt_battle.c", 769);

    WbtBattle_MakeTournamentPokemon(pokemon, tournament, entrant, count, heapId);
    func_ov012_021621d4(party, pokemon, 50, count, heapId);
    GFL_HeapFree(pokemon);
}

// Makes the party of the player's opponent
void WbtBattle_MakeOpponentParty(WbtSystem *sys, GameSystem *gsys, WbtEntrant *opponent) {
    HeapID heapId = HEAPID_TAIL(sys->heapId);
    u32 count = func_ov055_021e5d7c(sys);

    switch (opponent->unk0_0) {
    case 0:
        if (opponent->unk6_0 == TRAINER_CLASS_PKMN_TRAINER_RIVAL) {
            TrainerUtil_LoadParty(WbtBattle_GetRivalTrainerId(gsys), sys->partyC0, heapId);
            return;
        }
        WbtBattle_MakeTournamentParty(sys->partyC0, opponent, func_ov055_021e5ca4(sys), count, heapId);
        return;
    case 1:
    case 2:
        WbtBattle_MakeDownloadedParty(sys->partyC0, &sys->download, opponent->unk3_6, count, heapId);
        break;
    case 3:
        break;
    }
}

static void WbtBattle_SetBattleTrainer(BtlSetupTrainer *trainer, WbtEntrant *entrant) {
    u32 aiFlags = 0;
    u32 flags = entrant->unk6_12;

    aiFlags |= flags & 7;
    if (flags & 8) {
        aiFlags |= 0x80;
    }
    trainer->trainerId = 0x800 + entrant->unk2;
    trainer->aiFlags = aiFlags;
    trainer->trainerClass = entrant->unk6_0;
    func_ov055_021e5e3c(entrant, trainer->name);
    MI_CpuClear16(trainer->items, sizeof(trainer->items));
    PMSData_Clear(&trainer->unk18);
    PMSData_Clear(&trainer->unk20);
}

// The battle of the current round
BtlSetup *WbtBattle_SetupBattle(WbtSystem *sys, GameSystem *gsys) {
    u32 mode = func_ov055_021e64d4(func_ov055_021e5cac(sys));
    WbtEntrant *opponent = WbtBattle_GetOpponent(sys);
    BtlSetup *setup = BtlSetup_SetTrainerRental(gsys, sys->partyBC, mode);

    if (func_ov055_021e5ca4(sys) != 11) {
        PokeRegulation_ApplyLevelToParty(func_ov055_021e5f78(sys), setup->party[0]);
    }
    WbtBattle_SetBattleTrainer(setup->trainers[1], opponent);
    PokeParty_Copy(sys->partyC0, setup->party[1]);
    PokeRegulation_ApplyLevelToParty(func_ov055_021e5f78(sys), setup->party[1]);
    setup->trainerNames[1] = opponent->unkC;
    sys->unk13D4 = GFL_StrBufCreate(0x130, sys->heapId);
    sys->unk13D8 = GFL_StrBufCreate(0x130, sys->heapId);
    WbtBattle_LoadOpponentLine(sys, 3, sys->unk13D4);
    WbtBattle_LoadOpponentLine(sys, 4, sys->unk13D8);
    setup->unk68 = sys->unk13D4;
    setup->unk6C = sys->unk13D8;
    WbtBattle_GetBattleMusic(sys, &setup->fieldSituation.bgm, &setup->fieldSituation.unk12);
    if (func_ov055_021e5ca4(sys) == 3 && sys->download.tournament.unkA8 == 1) {
        setup->fieldSituation.env.bgType = 0x12;
        setup->fieldSituation.env.terrain = 0xe;
    } else {
        setup->fieldSituation.env.bgType = 0x13;
        setup->fieldSituation.env.terrain = 0xe;
        if (func_ov055_021e5cc4(sys) == 4) {
            setup->unkDD_6 = TRUE;
        }
    }
    return setup;
}

void WbtBattle_FreeBattle(WbtSystem *sys, BtlSetup *setup) {
    if (sys->unk13D4 != NULL) {
        GFL_StrBufFree(sys->unk13D4);
    }
    if (sys->unk13D8 != NULL) {
        GFL_StrBufFree(sys->unk13D8);
    }
    BtlSetup_Free(setup);
}

// How many of the party's Pokémon the battle left standing, out of those it started with
static int WbtBattle_CountRemainingPokemon(PokeParty *party, const u8 *slots) {
    int fainted = 0;
    int i;

    for (i = 0; i < 6; i++) {
        if (slots[i] == 2) {
            fainted++;
        }
    }
    return PokeParty_GetPkmCount(party) - fainted;
}

// Which of the three results a round of the tournament keeps
static u8 WbtBattle_GetRoundSlot(u32 round) {
    u32 slot = 0;

    switch (round) {
    case 2:
        slot = 2;
        break;
    case 3:
        slot = 1;
        break;
    case 4:
        break;
    }
    return slot;
}

u8 WbtBattle_GetRoundRemaining(WbtSystem *sys, u16 round) {
    return sys->unk13DC[WbtBattle_GetRoundSlot(round)];
}

static void WbtBattle_SetRoundRemaining(WbtSystem *sys, u32 round, u8 remaining) {
    sys->unk13DC[WbtBattle_GetRoundSlot(round)] = remaining;
}

// Ends the battle: records its result, and the Trainer beaten, and whether the player may go on
void WbtBattle_EndBattle(WbtSystem *sys, GameData *gameData, BtlSetup *setup) {
    u16 *resultVar = func_ov055_021e5ce4(sys);
    u32 result = GameData_GetLastBtlResult(gameData);
    WbtEntrant *opponent;
    void *save;
    u8 i;

    WbtBattle_CountRemainingPokemon(setup->party[1], setup->unkE7[1]);
    WbtBattle_SetRoundRemaining(sys, sys->round, WbtBattle_CountRemainingPokemon(setup->party[0], setup->unkE7[0]));
    switch (result) {
    case 0:
    case 1:
    case 2:
        break;
    case 3:
    default:
        result = 0;
        break;
    }
    opponent = WbtBattle_GetOpponent(sys);
    save = func_020179f8(gameData);
    if (sys->tournament != 3 && opponent->unk2 < 54) {
        u8 slot = WbtBattle_GetRoundSlot(sys->round);

        if (opponent->unk2 == 53) {
            func_0200ff28(save, slot, 0xff);
        } else {
            func_0200ff28(save, slot, opponent->unk2);
        }
        func_0200fefc(save, opponent->unk2);
    } else if (sys->tournament == 11 && opponent->unk2 == 56) {
        func_0200fefc(save, 0);
    }
    if (result == 0 || sys->unk4 == 1) {
        for (i = 0; i < 3; i++) {
            func_0200ff28(save, i, 0xff);
        }
    }
    *resultVar = result;
}

// The player's opponent in the current round
WbtEntrant *WbtBattle_GetOpponent(WbtSystem *sys) {
    return func_ov055_021e5d20(sys, func_ov055_021e607c(sys));
}

u32 WbtBattle_GetOpponentFlag(WbtSystem *sys) {
    return WbtBattle_GetOpponent(sys)->unk0_3;
}

u16 WbtBattle_GetOpponentObjCode(WbtSystem *sys) {
    return WbtBattle_GetOpponent(sys)->objCode;
}

void WbtBattle_GetOpponentName(WbtSystem *sys, StrBuf *strbuf) {
    func_ov055_021e5e3c(WbtBattle_GetOpponent(sys), strbuf);
}

// The music of the battle against the opponent, and of winning it
static void WbtBattle_GetBattleMusic(WbtSystem *sys, u16 *bgm, u16 *winBgm) {
    WbtEntrant *opponent = WbtBattle_GetOpponent(sys);
    u32 kind = func_ov055_021e5e34(opponent);
    WbtTrainer trainer;

    if (sys->tournament == 3 && sys->download.tournament.unkA8 != 0) {
        *bgm = SEQ_BGM_VS_WCS;
        *winBgm = SEQ_BGM_WIN5;
        return;
    }
    switch (kind) {
    case 1:
    default:
        *bgm = SEQ_BGM_VS_WBT;
        *winBgm = SEQ_BGM_WIN_WBT;
        return;
    case 0:
    case 2:
        func_ov055_021e6440(sys->trainers, opponent->unk2, &trainer);
        *bgm = trainer.bgm;
        *winBgm = trainer.winBgm;
        if (sys->round == 4 && sys->tournament != 1) {
            *bgm = SEQ_BGM_VS_WBT;
            *winBgm = SEQ_BGM_WIN_WBT;
        }
        break;
    }
}

// Loads the line of the Trainer's set of lines, which the tournament decides the version of
static void WbtBattle_LoadTrainerLine(StrBuf *strbuf, HeapID heapId, u32 tournament, WbtEntrant *entrant, u32 index) {
    u32 trainer;
    u32 version = 0;
    MsgData *msgData;
    u16 message;

    if (tournament - 12 <= 1) {
        version = 1;
    }
    trainer = func_ov055_021e5e28(entrant);
    if (trainer >= 78) {
        trainer = 0;
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WBT_TRAINER_MESSAGES, heapId);
    message = sTrainerLines[trainer][version] + index;
    GFL_MsgDataLoadStrbuf(msgData, message, strbuf);
    GFL_MsgDataFree(msgData);
}

// A line of a downloaded tournament's Trainer, and which set of messages it is of unless it is written out
static u16 *WbtBattle_GetDownloadedLine(WbtSystem *sys, u8 round, u32 index, u16 *messageSet) {
    WbtDownloadedTrainer *trainer = &sys->download.trainers[round];
    u16 *line = NULL;

    *messageSet = 1;
    switch (index) {
    case 0:
    case 1:
    case 2:
        *messageSet = trainer->messageSets[0];
        line = trainer->lines[0];
        break;
    case 3:
        *messageSet = trainer->messageSets[3];
        break;
    case 4:
        *messageSet = trainer->messageSets[4];
        break;
    case 5:
        *messageSet = trainer->messageSets[1];
        line = trainer->lines[1];
        break;
    case 6:
        *messageSet = trainer->messageSets[2];
        line = trainer->lines[2];
        break;
    case 7:
        *messageSet = trainer->messageSets[5];
        line = trainer->lines[3];
        break;
    }
    if (*messageSet == 0 && line == NULL) {
        *messageSet = 1;
    }
    return line;
}

// The opponent's line of the kind: the Trainers say 8 kinds, and the ones that the tournament's Trainers have
// written, or chosen from the messages of the sets
void WbtBattle_LoadOpponentLine(WbtSystem *sys, int a1, StrBuf *strbuf) {
    HeapID heapId = sys->heapId;
    WbtEntrant *opponent = WbtBattle_GetOpponent(sys);
    u32 kind = func_ov055_021e5e34(opponent);

    if (a1 >= 8) {
        return;
    }
    switch (kind) {
    case 0:
    default:
        WbtBattle_LoadTrainerLine(strbuf, heapId, sys->tournament, opponent, a1);
        return;
    case 2: {
        u32 round = func_ov055_021e5e20(opponent);
        u16 messageSet = 1;
        u16 *line = WbtBattle_GetDownloadedLine(sys, round, a1, &messageSet);

        if (messageSet == 0) {
            GFL_StrBufLoadFixedString(strbuf, line, 0x4a);
        } else {
            WbtBattle_LoadTrainerLine(strbuf, heapId, sys->tournament, opponent, a1);
        }
        return;
    }
    case 1: {
        u32 round = func_ov055_021e5e20(opponent);
        u16 messageSet = 1;
        u32 set;
        u16 *line = WbtBattle_GetDownloadedLine(sys, round, a1, &messageSet);
        set = messageSet;
        if (set == 0) {
            GFL_StrBufLoadFixedString(strbuf, line, 0x4a);
        } else {
            MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WBT_TRAINER_MESSAGES_2, heapId);

            GFL_MsgDataLoadStrbuf(msgData, (u16)((set - 1) * 8 + a1), strbuf);
            GFL_MsgDataFree(msgData);
        }
        return;
    }
    }
}
