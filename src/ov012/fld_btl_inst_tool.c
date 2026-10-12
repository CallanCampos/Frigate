// The battle facilities' tools: their trainers, Pokémon and battles, for the Battle Subway and the Trial House.
// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/arc.h"
#include "battle/btl_setup.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "field/battle_facility.h"
#include "field/bsubway_scr.h"
#include "field/field.h"
#include "field/field_event.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "system/game_data.h"
#include "system/game_system.h"

#define MOVE_FRUSTRATION 218

// The clients of a battle setup: the player, the trainer, the partner and the second trainer
typedef enum {
    CLIENT_PLAYER,
    CLIENT_ENEMY,
    CLIENT_PARTNER,
    CLIENT_ENEMY_2,
} SetupClient;

static void BtlSetup_SetTrialHouseParty(BtlSetup *setup, BSubwayTrainer *trainer, int client, u32 mode, int count,
                                        HeapID heapId);
static void func_ov012_02162394(u32 mode, u32 trainerId, const BSubwayTrainer *trainer, BtlSetupTrainer *dest,
                                u32 aiFlags, BOOL clearWords, BOOL copyWords);
static BtlSetup *BtlSetup_SetTrainerTrialHouse(GameSystem *gsys, u16 mode, BtlFieldStatus *status, BOOL a3,
                                               HeapID heapId);
static void RestrictPlayerParty(PokeParty *src, PokeParty *dest, int count, u16 level, HeapID heapId);
static void LoadTrialHouseParty(BSubwayTrainer *trainer, PokeParty *party, u16 level, int count, HeapID heapId);
static void genSubwayBtlInstitutePoke(const BSubwayPokemon *src, PartyPkm *pkm, u16 level);
static void func_ov012_021627b0(u32 arcId, BSubwayPokemonData *data, u32 file);
static u16 func_ov012_021627d4(MATHRandContext32 *rand);
static u8 func_ov012_02162828(u32 trainerId);
static BOOL func_ov012_02162ae8(const BSubwayTrainer *trainer);

// The items of the rental Pokémon
static const u16 data_ov012_0216d9ac[4] = {213, 157, 234, 217};

// What each trainer class gives
static const struct {
    u16 trainerClass;
    u16 value;
} data_ov012_0216d9b4[] = {
    {0x02, 0x0b}, {0x03, 0x0f}, {0x04, 0x0c}, {0x05, 0x10}, {0x29, 0x0d}, {0x2a, 0x11}, {0x0f, 0x99}, {0x0e, 0x9a},
    {0x1b, 0x30}, {0x1c, 0x31}, {0x08, 0x2a}, {0x09, 0x2b}, {0x3b, 0x18}, {0x3c, 0x1a}, {0x3d, 0x32}, {0x3e, 0x33},
    {0x5a, 0x26}, {0x5b, 0x27}, {0x4c, 0x2c}, {0x4b, 0x2d}, {0x11, 0x22}, {0x12, 0x23}, {0x34, 0x49}, {0x2e, 0x4a},
    {0x23, 0xb7}, {0x24, 0xb8}, {0x33, 0x2e}, {0x40, 0x2f}, {0x18, 0x24}, {0x19, 0x25}, {0x32, 0x1e}, {0x31, 0x1f},
    {0x46, 0x20}, {0x47, 0x21}, {0x1d, 0x3f}, {0x4a, 0x40}, {0x2c, 0x43}, {0x1a, 0x9b}, {0x20, 0x36}, {0x21, 0x1c},
    {0x56, 0x48}, {0x39, 0x3d}, {0x48, 0x3e}, {0x3a, 0x45}, {0x53, 0x44}, {0x2b, 0x47}, {0x42, 0x34}, {0x43, 0x34},
    {0x41, 0x54}, {0x22, 0x42}, {0x0d, 0x17}, {0x57, 0x41}, {0x30, 0x35},
};

static void BtlSetup_SetTrialHouseParty(BtlSetup *setup, BSubwayTrainer *trainer, int client, u32 mode, int count,
                                        HeapID heapId) {
    BtlSetupTrainer *dest = setup->trainers[client];
    BOOL clearWords, copyWords;

    switch (client) {
    case 1:
        clearWords = TRUE;
        copyWords = TRUE;
        break;
    case 3:
        clearWords = FALSE;
        copyWords = FALSE;
        break;
    case 2:
        clearWords = TRUE;
        copyWords = FALSE;
        break;
    }
    func_ov012_02162394(mode, trainer->unk00, trainer, dest, 0x87, clearWords, copyWords);
    LoadTrialHouseParty(trainer, setup->party[client], 50, count, heapId);
}

BtlSetup *SetupTrialHouseBattle(GameSystem *gsys, PokeParty *party, u32 mode, BSubwayTrainer *trainers,
                                BSubwayTrainer *partner, int count) {
    GameData *gameData = GSYS_GetGameData(gsys);
    u16 type = mode;
    BtlFieldStatus status;
    BtlSetup *setup = BtlSetup_SetTrainerTrialHouse(gsys, type, &status, TRUE, 4);
    SetupClient client = CLIENT_PLAYER;
    PlayerInfo *info = GetGameDataPlayerInfo(gameData);

    setup->unk34[client] = info;
    RestrictPlayerParty(party, setup->party[client], count, 50, 4);
    BtlSetup_SetTrialHouseParty(setup, trainers, 1, type, count, 4);
    if (setup->fieldSituation.unk1a != 0) {
        BtlSetup_SetTrialHouseParty(setup, &trainers[1], 3, type, count, 4);
    }
    if (setup->fieldSituation.unk1a == 3) {
        BtlSetup_SetTrialHouseParty(setup, partner, 2, type, count, 4);
    }
    BtlSetup_PostProcessTrialHouse(setup);
    return setup;
}

// A Trial House battle against one trainer, of levels between minLevel and maxLevel
BtlSetup *func_ov012_02162068(GameSystem *gsys, PokeParty *party, int partyCount, int mode, int count,
                              BSubwayTrainer *trainer, u32 a6, u32 trainerIdBase, u16 maxLevel, u16 minLevel, u32 a10,
                              u8 a11, u32 a12, Field *field, MATHRandContext32 *rand) {
    GameData *gameData = GSYS_GetGameData(gsys);
    BtlFieldStatus status;
    BtlSetup *setup;
    SetupClient client;
    u16 level;

    // Only mode 0 makes the setup, so the others use it unset
    if (mode <= 0) {
        setup = BtlSetup_SetTrainerTrialHouse(gsys, 0, &status, FALSE, 4);
    }
    client = CLIENT_PLAYER;
    setup->unk34[client] = GetGameDataPlayerInfo(gameData);
    RestrictPlayerParty(party, setup->party[client], partyCount, 0, 4);
    client = CLIENT_ENEMY;
    setup->trainers[client]->trainerId = trainer->unk00 - 1;
    func_ov012_02162394(0, trainerIdBase + setup->trainers[client]->trainerId, trainer, setup->trainers[client], 0x87,
                        TRUE, FALSE);
    level = minLevel + func_ov012_021627d4(rand) % (maxLevel - minLevel + 1);
    LoadTrialHouseParty(trainer, setup->party[client], level, count, 4);
    if (a11 == 0x17) {
        setup->fieldSituation.env.bgType = a12;
        setup->fieldSituation.env.terrain = 0x12;
    } else {
        setup->fieldSituation.env.bgType = a12;
        setup->fieldSituation.env.terrain = 0x13;
    }
    adjustPkmLvForChallengeKeys(setup, gameData, Field_GetPlayerStateZoneID(field));
    return setup;
}

// A battle with the player's rental Pokémon
BtlSetup *BtlSetup_SetTrainerRental(GameSystem *gsys, PokeParty *party, int mode) {
    GameData *gameData = GSYS_GetGameData(gsys);
    BtlFieldStatus status;
    BtlSetup *setup;
    SetupClient client;

    SaveBtlFieldStatus(&status, gameData, GSYS_GetField(gsys));
    setup = BtlSetup_Create(4);
    switch (mode) {
    case 0:
        BtlSetup_SetTrainer1v1Single(setup, gameData, &status, 0, 4);
        break;
    case 1:
        BtlSetup_SetTrainer1v1Double(setup, gameData, &status, 0, 4);
        break;
    case 2:
        BtlSetup_SetTrainer3v3(setup, gameData, &status, 0, 4);
        break;
    case 3:
        BtlSetup_SetTrainerRotation(setup, gameData, &status, 0, 4);
        break;
    }
    client = CLIENT_PLAYER;
    setup->unk34[client] = GetGameDataPlayerInfo(gameData);
    PokeParty_Copy(party, setup->party[client]);
    if (setup->battleType == 1) {
        setup->battleType = 2;
    }
    return setup;
}

void func_ov012_021621d4(PokeParty *party, const BSubwayPokemon *pkms, u16 level, int count, HeapID heapId) {
    int i;
    PartyPkm *pkm;

    PokeParty_InitCore(party, 6);
    pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), FALSE, "fld_btl_inst_tool.c", 796);
    for (i = 0; i < count; i++) {
        genSubwayBtlInstitutePoke(&pkms[i], pkm, level);
        PokeParty_AddPkm(party, pkm);
    }
    GFL_HeapFree(pkm);
}

static BtlSetup *BtlSetup_SetTrainerTrialHouse(GameSystem *gsys, u16 mode, BtlFieldStatus *status, BOOL a3,
                                               HeapID heapId) {
    GameData *gameData = GSYS_GetGameData(gsys);
    BtlSetup *setup;
    NetHandle *handle;
    int netId;

    SaveBtlFieldStatus(status, gameData, GSYS_GetField(gsys));
    setup = BtlSetup_Create(heapId);
    switch (mode) {
    case 0:
    case 4:
    case 5:
        BtlSetup_SetTrainer1v1Single(setup, gameData, status, 0, heapId);
        break;
    case 1:
    case 6:
        BtlSetup_SetTrainer1v1Double(setup, gameData, status, 0, heapId);
        break;
    case 2:
    case 7:
        BtlSetup_SetTrainer2v2(setup, gameData, status, 0, 0, 0, heapId);
        break;
    case 3:
    case 8:
        handle = func_02040440();
        netId = 0;
        if (func_0203ffc4()) {
            netId = 2;
        }
        BtlSetup_SetNetMultiVsAI(setup, gameData, handle, 1, netId, 0, 0, heapId);
        break;
    }
    if (a3 == TRUE) {
        BtlSetup_AllocRecBuffer(setup, heapId);
    }
    return setup;
}

static void RestrictPlayerParty(PokeParty *src, PokeParty *dest, int count, u16 level, HeapID heapId) {
    PartyPkm *pkm;
    PartyPkm *srcPkm;
    int i;

    PokeParty_InitCore(dest, 6);
    pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), FALSE, "fld_btl_inst_tool.c", 921);
    PokeParty_ClearPkm(pkm);
    for (i = 0; i < count; i++) {
        srcPkm = PokeParty_GetPkm(src, i);
        copyPartyPkm(srcPkm, pkm);
        if (level != 0 && level != PokeParty_GetParam(srcPkm, PKM_PARAM_LEVEL, NULL)) {
            setLevel(pkm, level);
        }
        PokeParty_AddPkm(dest, pkm);
    }
    GFL_HeapFree(pkm);
}

static void func_ov012_02162394(u32 mode, u32 trainerId, const BSubwayTrainer *trainer, BtlSetupTrainer *dest,
                                u32 aiFlags, BOOL clearWords, BOOL copyWords) {
    dest->trainerId = trainerId;
    dest->trainerClass = trainer->trainerId;
    dest->aiFlags = aiFlags;
    GFL_StrBufLoadString(dest->name, trainer->name);
    if (clearWords == TRUE) {
        PMSData_Clear(&dest->unk18);
        PMSData_Clear(&dest->unk20);
    }
    if (copyWords == TRUE) {
        // The trainer keeps its phrases as four words each, which func_ov012_02162ae8 checks one by one
        if (mode == 4) {
            dest->unk18 = *(const PMSData *)trainer->winWords;
            dest->unk20 = *(const PMSData *)trainer->loseWords;
        } else if (func_ov012_02162ae8(trainer)) {
            dest->unk18 = *(const PMSData *)trainer->winWords;
            dest->unk20 = *(const PMSData *)trainer->loseWords;
        }
    }
}

static void LoadTrialHouseParty(BSubwayTrainer *trainer, PokeParty *party, u16 level, int count, HeapID heapId) {
    int i;
    PartyPkm *pkm;

    PokeParty_InitCore(party, 6);
    pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), FALSE, "fld_btl_inst_tool.c", 1020);
    for (i = 0; i < count; i++) {
        genSubwayBtlInstitutePoke(&trainer->pokemon[i], pkm, level);
        PokeParty_AddPkm(party, pkm);
    }
    GFL_HeapFree(pkm);
}

u32 func_ov012_02162490(BSubwayPokemon *pkm, u32 arcId, u16 file, u32 id, u32 pid, u8 iv, u8 index, BOOL rentalItem,
                        HeapID heapId) {
    BSubwayPokemonData data;
    u32 personality;
    int i;
    int count;
    int ev;
    u8 happiness;
    u32 ability;
    MsgData *msgData;

    sys_memset(pkm, 0, sizeof(BSubwayPokemon));
    func_ov012_021627b0(arcId, &data, file);
    pkm->species = data.species;
    pkm->form = data.form;
    if (rentalItem) {
        pkm->item = data_ov012_0216d9ac[index];
    } else {
        pkm->item = data.item;
    }
    happiness = 0xff;
    for (i = 0; i < 4; i++) {
        pkm->moves[i] = data.moves[i];
        if (data.moves[i] == MOVE_FRUSTRATION) {
            happiness = 0;
        }
    }
    pkm->id = id;
    personality = pid;
    if (personality == 0) {
        personality = PML_GenPID(id, data.species, data.form, 2, 2, 0);
    }
    pkm->personality = personality;
    pkm->nature = data.nature;
    pkm->ivs.stat.hp = iv;
    pkm->ivs.stat.attack = iv;
    pkm->ivs.stat.defense = iv;
    pkm->ivs.stat.speed = iv;
    pkm->ivs.stat.spAttack = iv;
    pkm->ivs.stat.spDefense = iv;
    count = 0;
    for (i = 0; i < 6; i++) {
        if (data.evFlags & (1 << i)) {
            count++;
        }
    }
    ev = 510 / count;
    if (ev > 255) {
        ev = 255;
    }
    for (i = 0; i < 6; i++) {
        if (data.evFlags & (1 << i)) {
            pkm->evs[i] = ev;
        }
    }
    pkm->ppUps = 0;
    pkm->region = 0;
    ability = PML_PersonalGetParamSingle(pkm->species, 0, 0x1b);
    if (ability != 0) {
        if (pkm->personality & 1) {
        } else {
            ability = PML_PersonalGetParamSingle(pkm->species, 0, 0x1a);
        }
    } else {
        ability = PML_PersonalGetParamSingle(pkm->species, 0, 0x1a);
    }
    pkm->ability = ability;
    pkm->happiness = happiness;
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SPECIES_NAMES, heapId);
    GFL_MsgDataLoadRawStr(msgData, pkm->species, pkm->nickname, NELEMS(pkm->nickname));
    GFL_MsgDataFree(msgData);
    return personality;
}

static void genSubwayBtlInstitutePoke(const BSubwayPokemon *src, PartyPkm *pkm, u16 level) {
    StrBuf *strbuf;
    const u16 *name;
    u16 nickname[11];
    int i;
    u8 pp;
    u16 terminator;

    PokeParty_ClearPkm(pkm);
    PokeParty_CreatePkm(pkm, src->species, level, PKM_ID_RANDOM, src->ivs.all & 0x3fffffff, src->personality);
    PokeParty_SetParam(pkm, PKM_PARAM_FORM, (u8)src->form);
    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, src->item);
    for (i = 0; i < 4; i++) {
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1 + i, src->moves[i]);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, (u8)((src->ppUps >> (i * 2)) & 3));
        pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_MAX_PP + i, NULL);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + i, pp);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_ID, src->id);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP, src->evs[0]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 1, src->evs[1]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 2, src->evs[2]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 3, src->evs[3]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 4, src->evs[4]);
    PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 5, src->evs[5]);
    PokeParty_SetParam(pkm, PKM_PARAM_ABILITY, src->ability);
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, src->happiness);
    PokeParty_SetNature(pkm, src->nature);
    strbuf = GFL_StrBufCreate(NELEMS(nickname), HEAPID_GAMEEVENT);
    terminator = GFL_StrBufGetTerminator();
    name = src->nickname;
    for (i = 0; i < 11; i++) {
        nickname[i] = name[i];
    }
    nickname[i - 1] = terminator;
    GFL_StrBufLoadString(strbuf, nickname);
    PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (u32)strbuf);
    GFL_StrBufFree(strbuf);
    PokeParty_SetParam(pkm, PKM_PARAM_REGION, src->region);
    PokeParty_SetParam(pkm, PKM_PARAM_LEVEL, PokeParty_GetLevel(pkm));
}

static void func_ov012_021627b0(u32 arcId, BSubwayPokemonData *data, u32 file) {
    GFL_ArcSysRead(data, arcId, file);
}

void *func_ov012_021627c0(u32 arcId, u16 file, HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(arcId, (u16)(file + 1), FALSE, heapId);
}

static u16 func_ov012_021627d4(MATHRandContext32 *rand) {
    if (rand == NULL) {
        return GFL_RandomLC(0xffffffff) / (0xffffffff / 0x10000);
    }
    return MATH_Rand32(rand, 0xffffffff) / (0xffffffff / 0x10000);
}

static u8 func_ov012_02162828(u32 trainerId) {
    if (trainerId < 50) {
        return 3;
    }
    if (trainerId < 70) {
        return 6;
    }
    if (trainerId < 90) {
        return 9;
    }
    if (trainerId < 110) {
        return 12;
    }
    if (trainerId < 160) {
        return 15;
    }
    if (trainerId < 180) {
        return 18;
    }
    if (trainerId < 200) {
        return 21;
    }
    return 31;
}

BOOL func_ov012_02162864(BSubwayTrainer *trainer, u16 trainerId, u32 count, const u16 *species, const u16 *items,
                         BSubwayTeamConfig *config, HeapID heapId) {
    void *trainerData = func_ov012_021628c0(trainer, 0xd4, trainerId, 0xf, heapId);
    BOOL result = func_ov012_0216292c(trainerData, trainerId, trainer->pokemon, count, 0xd3, species, items, config,
                                      NULL, func_ov012_02162828(trainerId), heapId);

    GFL_HeapFree(trainerData);
    return result;
}

void *func_ov012_021628c0(BSubwayTrainer *trainer, u32 arcId, u16 trainerId, u16 msgFile, HeapID heapId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, msgFile, heapId);
    u16 *trainerData;
    StrBuf *name;

    sys_memset(trainer, 0, sizeof(BSubwayTrainer));
    trainerData = func_ov012_021627c0(arcId, trainerId, heapId);
    trainer->unk00 = trainerId + 1;
    trainer->message.type = 0xffff;
    trainer->message.id = trainerId * 3;
    trainer->trainerId = trainerData[0];
    name = GFL_MsgDataLoadStrbufNew(msgData, trainerId);
    // BUG: the size is in bytes, but GFL_StrBufStoreString counts characters
#ifdef BUGFIX
    GFL_StrBufStoreString(name, trainer->name, NELEMS(trainer->name));
#else
    GFL_StrBufStoreString(name, trainer->name, sizeof(trainer->name));
#endif
    GFL_StrBufFree(name);
    GFL_MsgDataFree(msgData);
    return trainerData;
}

BOOL func_ov012_0216292c(const u16 *trainerData, u16 trainerId, BSubwayPokemon *pkms, u8 count, u32 arcId,
                         const u16 *species, const u16 *items, BSubwayTeamConfig *config, MATHRandContext32 *rand,
                         u8 iv, HeapID heapId) {
    u32 files[4];
    u32 pids[4];
    BSubwayPokemonData other;
    BSubwayPokemonData data;
    u8 natures[4];
    u32 seed;
    u32 file;
    u8 index;
    int i;
    int n;
    int retries;
    BOOL failed;

    failed = FALSE;
    retries = 0;
    n = 0;
    while (n != count) {
        index = func_ov012_021627d4(rand) % trainerData[1];
        file = trainerData[2 + index];
        func_ov012_021627b0(arcId, &data, file);
        for (i = 0; i < n; i++) {
            func_ov012_021627b0(arcId, &other, files[i]);
            if (other.species == data.species) {
                break;
            }
        }
        if (i != n) {
            continue;
        }
        if (species != NULL) {
            for (i = 0; i < count; i++) {
                if (data.species == species[i]) {
                    break;
                }
            }
            if (i != count) {
                continue;
            }
        }
        if (retries < 50) {
            for (i = 0; i < n; i++) {
                func_ov012_021627b0(arcId, &other, files[i]);
                if (other.item != 0 && other.item == data.item) {
                    break;
                }
            }
            if (i != n) {
                retries++;
                continue;
            }
            if (items != NULL) {
                for (i = 0; i < count; i++) {
                    if (data.item == items[i] && items[i] != 0) {
                        break;
                    }
                }
                if (i != count) {
                    retries++;
                    continue;
                }
            }
        }
        files[n] = file;
        natures[n] = data.nature;
        n++;
    }
    seed = func_ov012_021627d4(rand) | (func_ov012_021627d4(rand) << 16);
    if (retries >= 50) {
        failed = TRUE;
    }
    for (i = 0; i < n; i++) {
        pids[i] = func_ov012_02162490(&pkms[i], arcId, files[i], seed, 0, iv, i, failed, heapId);
    }
    if (config == NULL) {
        return failed;
    }
    config->id = seed;
    for (i = 0; i < 2; i++) {
        config->files[i] = files[i];
        config->pids[i] = pids[i];
        config->natures[i] = natures[i];
    }
    return failed;
}

static BOOL func_ov012_02162ae8(const BSubwayTrainer *trainer) {
    int i;

    for (i = 0; i < 4; i++) {
        if (trainer->winWords[i] != 0) {
            return TRUE;
        }
        if (trainer->loseWords[i] != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 func_ov012_02162b0c(u32 arcId, u16 file, HeapID heapId) {
    u16 *trainerData = func_ov012_021627c0(arcId, file, heapId);
    u32 value = func_ov012_02162b38(trainerData[0]);

    GFL_HeapFree(trainerData);
    return value;
}

u16 func_ov012_02162b28(u32 arcId, u16 file, HeapID heapId) {
    u16 *trainerData = func_ov012_021627c0(arcId, file, heapId);
    u16 trainerClass = trainerData[0];

    GFL_HeapFree(trainerData);
    return trainerClass;
}

u32 func_ov012_02162b38(u32 trainerClass) {
    u32 i;

    for (i = 0; i < NELEMS(data_ov012_0216d9b4); i++) {
        if (trainerClass == data_ov012_0216d9b4[i].trainerClass) {
            return data_ov012_0216d9b4[i].value;
        }
    }
    return 10;
}
