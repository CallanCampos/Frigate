// The battle facilities' tools: their trainers, Pokémon and battles, for the Battle Subway and the Trial House.
// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "battle/btl_setup.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/obj_codes.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "constants/trainer_classes.h"
#include "constants/version.h"
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
static void BtlInst_SetSetupTrainer(u32 mode, u32 trainerId, const BSubwayTrainer *trainer, BtlSetupTrainer *dest,
                                    u32 aiFlags, BOOL clearWords, BOOL copyWords);
static BtlSetup *BtlSetup_SetTrainerTrialHouse(GameSystem *gsys, u16 mode, BtlFieldStatus *status, BOOL record,
                                               HeapID heapId);
static void RestrictPlayerParty(PokeParty *src, PokeParty *dest, int count, u16 level, HeapID heapId);
static void LoadTrialHouseParty(BSubwayTrainer *trainer, PokeParty *party, u16 level, int count, HeapID heapId);
static void genSubwayBtlInstitutePoke(const BSubwayPokemon *src, PartyPkm *pkm, u16 level);
static void BtlInst_ReadPokemonData(u32 arcId, BSubwayPokemonData *data, u32 file);
static u16 BtlInst_Random16(MATHRandContext32 *rand);
static u8 BtlInst_GetTrainerIV(u32 trainerId);
static BOOL BtlInst_TrainerHasWords(const BSubwayTrainer *trainer);

// The items of the rental Pokémon
static const u16 sRentalItems[4] = { ITEM_BRIGHT_POWDER, ITEM_LUM_BERRY, ITEM_LEFTOVERS, ITEM_QUICK_CLAW };

// The OBJ code of each trainer class's actor
static const struct {
    u16 trainerClass;
    u16 objCode;
} sTrainerClassObjCodes[] = {
    { TRAINER_CLASS_YOUNGSTER, OBJ_CODE_YOUNGSTER },
    { TRAINER_CLASS_LASS, OBJ_CODE_LASS },
    { TRAINER_CLASS_SCHOOL_KID_M, OBJ_CODE_SCHOOL_KID_M },
    { TRAINER_CLASS_SCHOOL_KID_F, OBJ_CODE_SCHOOL_KID_F },
    { TRAINER_CLASS_RICH_BOY, OBJ_CODE_RICH_BOY },
    { TRAINER_CLASS_LADY, OBJ_CODE_LADY },
    { TRAINER_CLASS_PRESCHOOLER_M, OBJ_CODE_PRESCHOOLER_M },
    { TRAINER_CLASS_PRESCHOOLER_F, OBJ_CODE_PRESCHOOLER_F },
    { TRAINER_CLASS_BACKPACKER_M, OBJ_CODE_BACKPACKER_M },
    { TRAINER_CLASS_BACKPACKER_F, OBJ_CODE_BACKPACKER_F },
    { TRAINER_CLASS_WAITER, OBJ_CODE_WAITER },
    { TRAINER_CLASS_WAITRESS, OBJ_CODE_WAITRESS },
    { TRAINER_CLASS_POKEFAN_M, OBJ_CODE_POKEFAN_M },
    { TRAINER_CLASS_POKEFAN_F, OBJ_CODE_POKEFAN_F },
    { TRAINER_CLASS_DOCTOR, OBJ_CODE_DOCTOR },
    { TRAINER_CLASS_NURSE, OBJ_CODE_NURSE },
    { TRAINER_CLASS_CYCLIST_M, OBJ_CODE_CYCLIST_M },
    { TRAINER_CLASS_CYCLIST_F, OBJ_CODE_CYCLIST_F },
    { TRAINER_CLASS_GENTLEMAN, OBJ_CODE_GENTLEMAN },
    { TRAINER_CLASS_SOCIALITE, OBJ_CODE_SOCIALITE },
    { TRAINER_CLASS_PKMN_BREEDER_M, OBJ_CODE_PKMN_BREEDER_M },
    { TRAINER_CLASS_PKMN_BREEDER_F, OBJ_CODE_PKMN_BREEDER_F },
    { TRAINER_CLASS_SCIENTIST_M_52, OBJ_CODE_SCIENTIST_M_52 },
    { TRAINER_CLASS_SCIENTIST_F_46, OBJ_CODE_SCIENTIST_F_46 },
    { TRAINER_CLASS_PSYCHIC_M, OBJ_CODE_PSYCHIC_M },
    { TRAINER_CLASS_PSYCHIC_F, OBJ_CODE_PSYCHIC_F },
    { TRAINER_CLASS_BLACK_BELT, OBJ_CODE_BLACK_BELT },
    { TRAINER_CLASS_BATTLE_GIRL, OBJ_CODE_BATTLE_GIRL },
    { TRAINER_CLASS_PKMN_RANGER_M, OBJ_CODE_PKMN_RANGER_M },
    { TRAINER_CLASS_PKMN_RANGER_F, OBJ_CODE_PKMN_RANGER_F },
    { TRAINER_CLASS_ACE_TRAINER_M, OBJ_CODE_ACE_TRAINER_M },
    { TRAINER_CLASS_ACE_TRAINER_F, OBJ_CODE_ACE_TRAINER_F },
    { TRAINER_CLASS_VETERAN_M, OBJ_CODE_VETERAN_M },
    { TRAINER_CLASS_VETERAN_F, OBJ_CODE_VETERAN_F },
    { TRAINER_CLASS_FISHERMAN, OBJ_CODE_FISHERMAN },
    { TRAINER_CLASS_HIKER, OBJ_CODE_HIKER },
    { TRAINER_CLASS_WORKER_44, OBJ_CODE_WORKER_44 },
    { TRAINER_CLASS_WORKER_26, OBJ_CODE_WORKER_26 },
    { TRAINER_CLASS_HARLEQUIN_32, OBJ_CODE_HARLEQUIN_233 },
    { TRAINER_CLASS_ARTIST, OBJ_CODE_ARTIST },
    { TRAINER_CLASS_POLICEMAN, OBJ_CODE_POLICEMAN },
    { TRAINER_CLASS_ROUGHNECK, OBJ_CODE_ROUGHNECK },
    { TRAINER_CLASS_BIKER, OBJ_CODE_BIKER },
    { TRAINER_CLASS_JANITOR, OBJ_CODE_JANITOR },
    { TRAINER_CLASS_DEPOT_AGENT, OBJ_CODE_DEPOT_AGENT },
    { TRAINER_CLASS_PILOT, OBJ_CODE_PILOT },
    { TRAINER_CLASS_CLERK_M_66, OBJ_CODE_CLERK_M_67 },
    { TRAINER_CLASS_CLERK_M_67, OBJ_CODE_CLERK_M_67 },
    { TRAINER_CLASS_PARASOL_LADY, OBJ_CODE_PARASOL_LADY },
    { TRAINER_CLASS_BAKER, OBJ_CODE_BAKER },
    { TRAINER_CLASS_NURSERY_AIDE, OBJ_CODE_NURSERY_AIDE },
    { TRAINER_CLASS_MAID, OBJ_CODE_65 },
    { TRAINER_CLASS_CLERK_F, OBJ_CODE_CLERK_F },
};

static void BtlSetup_SetTrialHouseParty(BtlSetup *setup, BSubwayTrainer *trainer, int client, u32 mode, int count,
                                        HeapID heapId) {
    BtlSetupTrainer *dest = setup->trainers[client];
    BOOL clearWords, copyWords;

    switch (client) {
    case CLIENT_ENEMY:
        clearWords = TRUE;
        copyWords = TRUE;
        break;
    case CLIENT_ENEMY_2:
        clearWords = FALSE;
        copyWords = FALSE;
        break;
    case CLIENT_PARTNER:
        clearWords = TRUE;
        copyWords = FALSE;
        break;
    }
    BtlInst_SetSetupTrainer(mode, trainer->unk00, trainer, dest, 0x87, clearWords, copyWords);
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

    setup->playerInfo[client] = info;
    RestrictPlayerParty(party, setup->party[client], count, 50, 4);
    BtlSetup_SetTrialHouseParty(setup, trainers, CLIENT_ENEMY, type, count, 4);
    if (setup->fieldSituation.multiMode != 0) {
        BtlSetup_SetTrialHouseParty(setup, &trainers[1], CLIENT_ENEMY_2, type, count, 4);
    }
    if (setup->fieldSituation.multiMode == 3) {
        BtlSetup_SetTrialHouseParty(setup, partner, CLIENT_PARTNER, type, count, 4);
    }
    BtlSetup_PostProcessTrialHouse(setup);
    return setup;
}

// Sets up the client's trainer from trainer, numbered from trainerIdBase
static inline void SetTrialHouseTrainer(BtlSetup *setup, SetupClient client, const BSubwayTrainer *trainer,
                                        u32 trainerIdBase) {
    setup->trainers[client]->trainerId = trainer->unk00 - 1;
    BtlInst_SetSetupTrainer(0, trainerIdBase + setup->trainers[client]->trainerId, trainer, setup->trainers[client],
                            0x87, TRUE, FALSE);
}

// A Trial House battle against one trainer, of levels between minLevel and maxLevel
BtlSetup *SetupInfinityDungeonBattle(GameSystem *gsys, PokeParty *party, int partyCount, int mode, int count,
                                     BSubwayTrainer *trainer, u32 a6, u32 trainerIdBase, u16 maxLevel, u16 minLevel,
                                     u32 a10, u8 version, u32 bgType, Field *field, MATHRandContext32 *rand) {
    GameData *gameData = GSYS_GetGameData(gsys);
    BtlFieldStatus status;
    BtlSetup *setup;
    SetupClient client;
    PlayerInfo *info;
    u16 level;

    // Only mode 0 makes the setup, so the others use it unset
    if (mode <= 0) {
        setup = BtlSetup_SetTrainerTrialHouse(gsys, 0, &status, FALSE, 4);
    }
    client = CLIENT_PLAYER;
    info = GetGameDataPlayerInfo(gameData);
    setup->playerInfo[client] = info;
    RestrictPlayerParty(party, setup->party[client], partyCount, 0, 4);
    client = CLIENT_ENEMY;
    SetTrialHouseTrainer(setup, client, trainer, trainerIdBase);
    level = minLevel + BtlInst_Random16(rand) % (maxLevel - minLevel + 1);
    LoadTrialHouseParty(trainer, setup->party[client], level, count, 4);
    if (version == VERSION_BLACK2) {
        setup->fieldSituation.env.bgType = bgType;
        setup->fieldSituation.env.terrain = 0x12;
    } else {
        setup->fieldSituation.env.bgType = bgType;
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
    setup->playerInfo[client] = GetGameDataPlayerInfo(gameData);
    PokeParty_Copy(party, setup->party[client]);
    if (setup->battleType == 1) {
        setup->battleType = 2;
    }
    return setup;
}

void BtlInst_MakeParty(PokeParty *party, const BSubwayPokemon *pkms, u16 level, int count, HeapID heapId) {
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

static BtlSetup *BtlSetup_SetTrainerTrialHouse(GameSystem *gsys, u16 mode, BtlFieldStatus *status, BOOL record,
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
        handle = NetHandle_GetOwn();
        netId = 0;
        if (NetSystem_GetNetId()) {
            netId = 2;
        }
        BtlSetup_SetNetMultiVsAI(setup, gameData, handle, 1, netId, 0, 0, heapId);
        break;
    }
    if (record == TRUE) {
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

static void BtlInst_SetSetupTrainer(u32 mode, u32 trainerId, const BSubwayTrainer *trainer, BtlSetupTrainer *dest,
                                    u32 aiFlags, BOOL clearWords, BOOL copyWords) {
    dest->trainerId = trainerId;
    dest->trainerClass = trainer->trainerId;
    dest->aiFlags = aiFlags;
    GFL_StrBufLoadString(dest->name, trainer->name);
    if (clearWords == TRUE) {
        PMSData_Clear(&dest->winWords);
        PMSData_Clear(&dest->loseWords);
    }
    if (copyWords == TRUE) {
        // The trainer keeps its phrases as four words each, which BtlInst_TrainerHasWords checks one by one
        if (mode == 4) {
            dest->winWords = *(const PMSData *)trainer->winWords;
            dest->loseWords = *(const PMSData *)trainer->loseWords;
        } else if (BtlInst_TrainerHasWords(trainer)) {
            dest->winWords = *(const PMSData *)trainer->winWords;
            dest->loseWords = *(const PMSData *)trainer->loseWords;
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

u32 BtlInst_MakeRomPokemon(BSubwayPokemon *pkm, u32 arcId, u16 file, u32 id, u32 pid, u8 iv, u8 index, BOOL rentalItem,
                           HeapID heapId) {
    BSubwayPokemonData data;
    u32 personality;
    int i;
    int count;
    int ev;
    u8 happiness;
    u32 ability;
    MsgData *msgData;

    MI_CpuFill8(pkm, 0, sizeof(BSubwayPokemon));
    BtlInst_ReadPokemonData(arcId, &data, file);
    pkm->species = data.species;
    pkm->form = data.form;
    if (rentalItem) {
        pkm->item = sRentalItems[index];
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
        personality = PML_GenPID(id, data.species, data.form, 2, 2, PID_RARE_NEVER);
        pkm->personality = personality;
    } else {
        pkm->personality = personality;
    }
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
    ability = PML_PersonalGetParamSingle(pkm->species, 0, PERSONAL_ABILITY_2);
    if (ability != 0) {
        if (pkm->personality & 1) {
            pkm->ability = ability;
        } else {
            pkm->ability = PML_PersonalGetParamSingle(pkm->species, 0, PERSONAL_ABILITY_1);
        }
    } else {
        pkm->ability = PML_PersonalGetParamSingle(pkm->species, 0, PERSONAL_ABILITY_1);
    }
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
    PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (uptr)strbuf);
    GFL_StrBufFree(strbuf);
    PokeParty_SetParam(pkm, PKM_PARAM_REGION, src->region);
    PokeParty_SetParam(pkm, PKM_PARAM_LEVEL, PokeParty_GetLevel(pkm));
}

static void BtlInst_ReadPokemonData(u32 arcId, BSubwayPokemonData *data, u32 file) {
    GFL_ArcSysRead(data, arcId, file);
}

void *BtlInst_LoadTrainerData(u32 arcId, u16 file, HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(arcId, (u16)(file + 1), FALSE, heapId);
}

static u16 BtlInst_Random16(MATHRandContext32 *rand) {
    if (rand == NULL) {
        return GFL_RandomLC(0xffffffff) / (0xffffffff / 0x10000);
    }
    return MATH_Rand32(rand, 0xffffffff) / (0xffffffff / 0x10000);
}

static u8 BtlInst_GetTrainerIV(u32 trainerId) {
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

BOOL BtlInst_MakeRomTrainerAndTeam(BSubwayTrainer *trainer, u16 trainerId, u32 count, const u16 *species,
                                   const u16 *items, BSubwayTeamConfig *config, HeapID heapId) {
    void *trainerData = BtlInst_MakeRomTrainer(trainer, 0xd4, trainerId, 0xf, heapId);
    BOOL result = BtlInst_MakeRomTeam(trainerData, trainerId, trainer->pokemon, count, 0xd3, species, items, config,
                                      NULL, BtlInst_GetTrainerIV(trainerId), heapId);

    GFL_HeapFree(trainerData);
    return result;
}

void *BtlInst_MakeRomTrainer(BSubwayTrainer *trainer, u32 arcId, u16 trainerId, u16 msgFile, HeapID heapId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, msgFile, heapId);
    u16 *trainerData;
    StrBuf *name;

    MI_CpuFill8(trainer, 0, sizeof(BSubwayTrainer));
    trainerData = BtlInst_LoadTrainerData(arcId, trainerId, heapId);
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

BOOL BtlInst_MakeRomTeam(const u16 *trainerData, u16 trainerId, BSubwayPokemon *pkms, u8 count, u32 arcId,
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
        index = BtlInst_Random16(rand) % trainerData[1];
        file = trainerData[2 + index];
        BtlInst_ReadPokemonData(arcId, &data, file);
        for (i = 0; i < n; i++) {
            BtlInst_ReadPokemonData(arcId, &other, files[i]);
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
                BtlInst_ReadPokemonData(arcId, &other, files[i]);
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
    seed = BtlInst_Random16(rand) | (BtlInst_Random16(rand) << 16);
    if (retries >= 50) {
        failed = TRUE;
    }
    for (i = 0; i < n; i++) {
        pids[i] = BtlInst_MakeRomPokemon(&pkms[i], arcId, files[i], seed, 0, iv, i, failed, heapId);
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

static BOOL BtlInst_TrainerHasWords(const BSubwayTrainer *trainer) {
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

u16 BtlInst_GetRomTrainerObjCode(u32 arcId, u16 file, HeapID heapId) {
    u16 *trainerData = BtlInst_LoadTrainerData(arcId, file, heapId);
    u16 value = BtlInst_GetTrainerClassObjCode(trainerData[0]);

    GFL_HeapFree(trainerData);
    return value;
}

u16 BtlInst_GetRomTrainerClass(u32 arcId, u16 file, HeapID heapId) {
    u16 *trainerData = BtlInst_LoadTrainerData(arcId, file, heapId);
    u16 trainerClass = trainerData[0];

    GFL_HeapFree(trainerData);
    return trainerClass;
}

u16 BtlInst_GetTrainerClassObjCode(u32 trainerClass) {
    u32 i;

    for (i = 0; i < NELEMS(sTrainerClassObjCodes); i++) {
        if (trainerClass == sTrainerClassObjCodes[i].trainerClass) {
            return sTrainerClassObjCodes[i].objCode;
        }
    }
    return OBJ_CODE_10;
}
