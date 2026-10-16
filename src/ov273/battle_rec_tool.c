// battle_rec_tool.c (overlay 273, the name its allocations pass): stores a battle's setup in the battle video that is
// loaded, or in Pokéstar Studios' block, and loads it back, converting the parties, the clients and the setup's
// parameters to the form they are kept in

#include "types.h"
#include "battle/battle_rec_tool.h"
#include "battle/btl_setup.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/config.h"
#include "save/player_info.h"

static void BattleRecTool_StorePkm(PartyPkm *pkm, BattleRecPkm *rec);
static void BattleRecTool_StoreParty(PokeParty *party, BattleRecParty *rec);
static void BattleRecTool_LoadParty(BattleRecParty *rec, PokeParty *party, HeapID heapId);
static void BattleRecTool_StoreParties(const BtlSetup *setup, BattleRecBody *body);
static void BattleRecTool_LoadParties(BtlSetup *setup, BattleRecBody *body, HeapID heapId);
static void BattleRecTool_StoreClients(const BtlSetup *setup, BattleRecClient *clients, u32 count);
static void BattleRecTool_StorePokewoodClients(const BtlSetup *setup, BattleRecClient *clients, u32 count,
                                               u16 *trainerClass);
static void BattleRecTool_LoadClients(BtlSetup *setup, BattleRecClient *clients, u32 count);
static void BattleRecTool_LoadPokewoodClients(BtlSetup *setup, const BattleRecClient *clients, u32 count,
                                              u16 trainerClass);
static void BattleRecTool_StoreTrainer(const BtlSetupTrainer *trainer, BattleRecTrainer *rec);
static void BattleRecTool_LoadTrainer(BtlSetupTrainer *trainer, const BattleRecTrainer *rec);
static BOOL BattleRecTool_StoreRecData(const BtlSetup *setup, BattleRecBody *body);
static BOOL BattleRecTool_LoadRecData(BtlSetup *setup, const BattleRecBody *body);
static BOOL BattleRecTool_StoreSetupParams(const BtlSetup *setup, BattleRecSetup *rec);
static BOOL BattleRecTool_LoadSetupParams(BtlSetup *setup, const BattleRecSetup *rec);
static void BattleRecTool_StorePokewoodParty(const BtlSetup *setup, PokewoodBlock *block);
static void BattleRecTool_LoadPokewoodParty(BtlSetup *setup, PokewoodBlock *block, HeapID heapId);
static BOOL BattleRecTool_StorePokewoodRecData(const BtlSetup *setup, PokewoodBlock *block);
static BOOL BattleRecTool_LoadPokewoodRecData(BtlSetup *setup, const PokewoodBlock *block);
static void BattleRecTool_StorePokewoodPartyCore(PokeParty *party, BattleRecParty *rec);
static void BattleRecTool_LoadPokewoodPartyCore(BattleRecParty *rec, PokeParty *party, HeapID heapId);
static void BattleRecTool_StorePokewoodInfo(const BtlSetup *setup, PokewoodBattleInfo *info);
static void BattleRecTool_LoadPokewoodInfo(BtlSetup *setup, const PokewoodBattleInfo *info);
static void BattleRecTool_StorePokewoodExtra(const BtlSetup *setup, PokewoodBlock *block);
static void BattleRecTool_LoadPokewoodExtra(BtlSetup *setup, const PokewoodBlock *block);

// The smaller of two numbers, a on a tie
static inline int BattleRecTool_Min(int a, int b) {
    return a <= b ? a : b;
}

static void BattleRecTool_StorePkm(PartyPkm *pkm, BattleRecPkm *rec) {
    PkmBlockA *blockA;
    PkmBlockB *blockB;
    PkmBlockC *blockC;
    PkmBlockD *blockD;
    int i;

    MI_CpuFill8(rec, 0, sizeof(BattleRecPkm));
    if (!pkm->base.partyDecrypted) {
        PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), pkm->base.pid);
        PML_CryptoRun(&pkm->base.contentBuffer, sizeof(PkmBuffer), pkm->base.checksum);
    }
    blockA = PML_PkmGetParamBlockCore(&pkm->base, pkm->base.pid, 0);
    blockB = PML_PkmGetParamBlockCore(&pkm->base, pkm->base.pid, 1);
    blockC = PML_PkmGetParamBlockCore(&pkm->base, pkm->base.pid, 2);
    blockD = PML_PkmGetParamBlockCore(&pkm->base, pkm->base.pid, 3);

    rec->pid = pkm->base.pid;
    rec->partyDecrypted = FALSE;
    rec->boxDecrypted = FALSE;
    rec->badEgg = pkm->base.badEgg;
    rec->nature = blockB->nature;
    rec->nPoke = blockB->nPoke;

    rec->species = blockA->species;
    rec->item = blockA->item;
    rec->id = blockA->id;
    rec->exp = blockA->exp;
    rec->friendship = blockA->friendship;
    rec->ability = blockA->ability;
    rec->hpEV = blockA->hpEV;
    rec->atkEV = blockA->atkEV;
    rec->defEV = blockA->defEV;
    rec->speEV = blockA->speEV;
    rec->spaEV = blockA->spaEV;
    rec->spdEV = blockA->spdEV;
    rec->language = blockA->language;

    for (i = 0; i < 4; i++) {
        rec->moves[i] = blockB->moves[i];
        rec->pp[i] = blockB->pp[i];
        rec->ppUps[i] = blockB->ppUps[i];
    }
    rec->hpIV = blockB->hpIV;
    rec->atkIV = blockB->atkIV;
    rec->defIV = blockB->defIV;
    rec->speIV = blockB->speIV;
    rec->spaIV = blockB->spaIV;
    rec->spdIV = blockB->spdIV;
    rec->isEgg = blockB->isEgg;
    rec->isNicknamed = blockB->isNicknamed;
    rec->fatefulEncounter = blockB->fatefulEncounter;
    rec->gender = blockB->gender;
    rec->form = blockB->form;

    for (i = 0; i < 11; i++) {
        rec->nickname[i] = blockC->nickname[i];
    }

    for (i = 0; i < 8; i++) {
        rec->otName[i] = blockD->otName[i];
    }
    rec->ball = blockD->ball;
    rec->pokestarFame = blockD->pokestarFame;

    rec->statusCond = pkm->statusCond;
    rec->level = pkm->level;
    rec->nowHP = pkm->nowHP;
    rec->maxHP = pkm->maxHP;
    rec->atk = pkm->atk;
    rec->def = pkm->def;
    rec->spe = pkm->spe;
    rec->spa = pkm->spa;
    rec->spd = pkm->spd;

    if (!pkm->base.partyDecrypted) {
        PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), pkm->base.pid);
        PML_CryptoRun(&pkm->base.contentBuffer, sizeof(PkmBuffer), pkm->base.checksum);
    }
}

void BattleRecTool_LoadPkm(BattleRecPkm *rec, PartyPkm *pkm) {
    PkmBlockA *blockA;
    PkmBlockB *blockB;
    PkmBlockC *blockC;
    PkmBlockD *blockD;
    int i;

    MI_CpuFill8(pkm, 0, sizeof(PartyPkm));
    blockA = PML_PkmGetParamBlockCore(&pkm->base, rec->pid, 0);
    blockB = PML_PkmGetParamBlockCore(&pkm->base, rec->pid, 1);
    blockC = PML_PkmGetParamBlockCore(&pkm->base, rec->pid, 2);
    blockD = PML_PkmGetParamBlockCore(&pkm->base, rec->pid, 3);

    pkm->base.pid = rec->pid;
    pkm->base.partyDecrypted = FALSE;
    pkm->base.boxDecrypted = FALSE;
    pkm->base.badEgg = rec->badEgg;

    blockA->species = rec->species;
    blockA->item = rec->item;
    blockA->id = rec->id;
    blockA->exp = rec->exp;
    blockA->friendship = rec->friendship;
    blockA->ability = rec->ability;
    blockA->hpEV = rec->hpEV;
    blockA->atkEV = rec->atkEV;
    blockA->defEV = rec->defEV;
    blockA->speEV = rec->speEV;
    blockA->spaEV = rec->spaEV;
    blockA->spdEV = rec->spdEV;
    blockA->language = rec->language;

    for (i = 0; i < 4; i++) {
        blockB->moves[i] = rec->moves[i];
        blockB->pp[i] = rec->pp[i];
        blockB->ppUps[i] = rec->ppUps[i];
    }
    blockB->hpIV = rec->hpIV;
    blockB->atkIV = rec->atkIV;
    blockB->defIV = rec->defIV;
    blockB->speIV = rec->speIV;
    blockB->spaIV = rec->spaIV;
    blockB->spdIV = rec->spdIV;
    blockB->isEgg = rec->isEgg;
    blockB->isNicknamed = rec->isNicknamed;
    blockB->fatefulEncounter = rec->fatefulEncounter;
    blockB->gender = rec->gender;
    blockB->form = rec->form;
    blockB->nature = rec->nature;
    blockB->nPoke = rec->nPoke;

    for (i = 0; i < 11; i++) {
        blockC->nickname[i] = rec->nickname[i];
    }

    for (i = 0; i < 8; i++) {
        blockD->otName[i] = rec->otName[i];
    }
    blockD->ball = rec->ball;
    blockD->pokestarFame = rec->pokestarFame;

    pkm->statusCond = rec->statusCond;
    pkm->level = rec->level;
    pkm->nowHP = rec->nowHP;
    pkm->maxHP = rec->maxHP;
    pkm->atk = rec->atk;
    pkm->def = rec->def;
    pkm->spe = rec->spe;
    pkm->spa = rec->spa;
    pkm->spd = rec->spd;

    PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), pkm->base.pid);
    pkm->base.checksum = PML_CryptoGenKey(&pkm->base.contentBuffer, sizeof(PkmBuffer));
    PML_CryptoRun(&pkm->base.contentBuffer, sizeof(PkmBuffer), pkm->base.checksum);
}

void BattleRecTool_StoreSetup(BtlSetup *setup) {
    BattleRecBody *body;
    PokewoodBlock *block;

    switch (setup->unkDD_3) {
    case 0:
    default:
        body = &data_02140f64->body;
        BattleRecTool_StoreParties(setup, body);
        BattleRecTool_StoreClients(setup, body->clients, 4);
        BattleRecTool_StoreRecData(setup, body);
        BattleRecTool_StoreSetupParams(setup, &body->setup);
        break;
    case 1:
    case 2:
        block = getPokewoodBlock();
        BattleRecTool_StorePokewoodParty(setup, block);
        BattleRecTool_StorePokewoodClients(setup, &block->client, 1, &block->trainerClass);
        BattleRecTool_StorePokewoodRecData(setup, block);
        BattleRecTool_StoreSetupParams(setup, &block->setup);
        BattleRecTool_StorePokewoodInfo(setup, &block->info);
        BattleRecTool_StorePokewoodExtra(setup, block);
        break;
    }
}

void BattleRecTool_LoadSetup(BtlSetup *setup, u32 mode, HeapID heapId) {
    BattleRecBody *body;
    PokewoodBlock *block;

    switch (mode) {
    case 0:
    default:
        body = &data_02140f64->body;
        BattleRecTool_LoadParties(setup, body, heapId);
        BattleRecTool_LoadClients(setup, body->clients, 4);
        BattleRecTool_LoadRecData(setup, body);
        BattleRecTool_LoadSetupParams(setup, &body->setup);
        break;
    case 1:
        block = getPokewoodBlock();
        BattleRecTool_LoadPokewoodParty(setup, block, heapId);
        BattleRecTool_LoadPokewoodClients(setup, &block->client, 1, block->trainerClass);
        BattleRecTool_LoadPokewoodRecData(setup, block);
        BattleRecTool_LoadSetupParams(setup, &block->setup);
        BattleRecTool_LoadPokewoodInfo(setup, &block->info);
        BattleRecTool_LoadPokewoodExtra(setup, block);
        setup->unkDD_3 = 2;
        break;
    }
}

static void BattleRecTool_StoreParty(PokeParty *party, BattleRecParty *rec) {
    int i;

    MI_CpuFill8(rec, 0, sizeof(BattleRecParty));
    if (PokeParty_GetPkmCount(party) != 0) {
        rec->capacity = PokeParty_GetCapacity(party);
        rec->count = PokeParty_GetPkmCount(party);
        for (i = 0; i < rec->count; i++) {
            BattleRecTool_StorePkm(PokeParty_GetPkm(party, i), &rec->pkm[i]);
        }
    }
}

static void BattleRecTool_LoadParty(BattleRecParty *rec, PokeParty *party, HeapID heapId) {
    int i;
    PartyPkm *pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), TRUE, "battle_rec_tool.c", 266);

    PokeParty_InitCore(party, rec->capacity);
    for (i = 0; i < rec->count; i++) {
        BattleRecTool_LoadPkm(&rec->pkm[i], pkm);
        PokeParty_SetParam(pkm, 0x9f, 0);
        PokeParty_AddPkm(party, pkm);
    }
    GFL_HeapFree(pkm);
}

static void BattleRecTool_StoreParties(const BtlSetup *setup, BattleRecBody *body) {
    u32 i;

    MI_CpuFill8(body->parties, 0, sizeof(body->parties));
    for (i = 0; i < 4; i++) {
        if (setup->party[i] != NULL) {
            BattleRecTool_StoreParty(setup->party[i], &body->parties[i]);
        }
    }
}

static void BattleRecTool_LoadParties(BtlSetup *setup, BattleRecBody *body, HeapID heapId) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (setup->party[i] != NULL) {
            BattleRecTool_LoadParty(&body->parties[i], setup->party[i], heapId);
        }
    }
}

static void BattleRecTool_StoreClients(const BtlSetup *setup, BattleRecClient *clients, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        if (setup->trainers[i] != NULL) {
            BattleRecTool_StoreTrainer(setup->trainers[i], &clients[i].info.trainer);
            clients[i].type = BATTLE_REC_CLIENT_TRAINER;
        } else if (setup->unk34[i] != NULL) {
            func_02008b34(setup->unk34[i], &clients[i].info.player);
            clients[i].type = BATTLE_REC_CLIENT_PLAYER;
        } else {
            clients[i].type = BATTLE_REC_CLIENT_NONE;
        }
        clients[i].unk2 = setup->unk44[i];
    }
}

static void BattleRecTool_StorePokewoodClients(const BtlSetup *setup, BattleRecClient *clients, u32 count,
                                               u16 *trainerClass) {
    u32 i;

    for (i = 0; i < count; i++) {
        switch (i) {
        case 0:
            func_02008b34(setup->unk34[i], &clients[i].info.player);
            if (setup->trainers[i] != NULL) {
                *trainerClass = setup->trainers[i]->trainerClass;
            } else if (getTrainerGender(setup->unk34[i]) == GENDER_MALE) {
                *trainerClass = 0;
            } else {
                *trainerClass = 1;
            }
            clients[i].type = BATTLE_REC_CLIENT_TRAINER;
            break;
        case 1:
            BattleRecTool_StoreTrainer(setup->trainers[i], &clients[i].info.trainer);
            clients[i].type = BATTLE_REC_CLIENT_TRAINER;
            break;
        }
        clients[i].unk2 = setup->unk44[i];
    }
}

static void BattleRecTool_LoadClients(BtlSetup *setup, BattleRecClient *clients, u32 count) {
    u32 i;
    BattleRecClient *client;

    for (i = 0; i < count; i++) {
        client = &clients[i];
        switch (clients[i].type) {
        case BATTLE_REC_CLIENT_PLAYER:
            func_02008b34(&client->info.player, setup->unk34[i]);
            break;
        case BATTLE_REC_CLIENT_TRAINER:
            BattleRecTool_LoadTrainer(setup->trainers[i], &client->info.trainer);
            break;
        }
        setup->unk44[i] = client->unk2;
    }
}

static void BattleRecTool_LoadPokewoodClients(BtlSetup *setup, const BattleRecClient *clients, u32 count,
                                              u16 trainerClass) {
    u32 i;

    for (i = 0; i < count; i++) {
        switch (i) {
        case 0:
            func_02008b34(&clients[i].info.player, setup->unk34[i]);
            setup->trainers[i]->trainerId = 1;
            setup->trainers[i]->trainerClass = trainerClass;
            break;
        case 1:
            BattleRecTool_LoadTrainer(setup->trainers[i], &clients[i].info.trainer);
            break;
        }
        setup->unk44[i] = clients[i].unk2;
    }
}

static void BattleRecTool_StoreTrainer(const BtlSetupTrainer *trainer, BattleRecTrainer *rec) {
    u32 i;

    rec->trainerId = trainer->trainerId;
    rec->trainerClass = trainer->trainerClass;
    rec->aiFlags = trainer->aiFlags;
    rec->unk30 = trainer->unk18;
    rec->unk38 = trainer->unk20;
    for (i = 0; i < 4; i++) {
        rec->items[i] = trainer->items[i];
    }
    GFL_StrBufStoreString(trainer->name, rec->name, 16);
}

static void BattleRecTool_LoadTrainer(BtlSetupTrainer *trainer, const BattleRecTrainer *rec) {
    u32 i;
    const u16 *str;

    trainer->trainerId = rec->trainerId;
    trainer->trainerClass = rec->trainerClass;
    trainer->aiFlags = rec->aiFlags;
    trainer->unk18 = rec->unk30;
    trainer->unk20 = rec->unk38;
    for (i = 0; i < 4; i++) {
        trainer->items[i] = rec->items[i];
    }
    GFL_StrBufLoadString(trainer->name, rec->name);
    str = GFL_StrBufGetStringPtr(trainer->name);
    while (*str != GFL_StrBufGetTerminator()) {
        str++;
    }
}

static BOOL BattleRecTool_StoreRecData(const BtlSetup *setup, BattleRecBody *body) {
    void *data = setup->unkB0;
    u32 size;

    if (data != NULL) {
        size = setup->unkB4;
        if (size < sizeof(body->data)) {
            body->dataSize = size;
            MI_CpuCopy8(data, body->data, size);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL BattleRecTool_LoadRecData(BtlSetup *setup, const BattleRecBody *body) {
    setup->unkB4 = body->dataSize;
    MI_CpuCopy8(body->data, setup->unkB0, setup->unkB4);
    return TRUE;
}

static BOOL BattleRecTool_StoreSetupParams(const BtlSetup *setup, BattleRecSetup *rec) {
    rec->env = setup->fieldSituation.env;
    rec->rand = setup->rand;
    rec->bgm = setup->fieldSituation.bgm;
    rec->unk2E = setup->fieldSituation.unk12;
    rec->battleType = setup->battleType;
    rec->battleStyle = setup->battleStyle;
    rec->unk33_4 = setup->fieldSituation.unk1a;
    rec->unk30 = setup->unkA2;
    rec->unk32_5 = setup->fieldSituation.unk19;
    rec->unk33_7 = setup->unk97 ? TRUE : FALSE;
    initConfig(setup->config, (Config *)rec->config);
    return TRUE;
}

static BOOL BattleRecTool_LoadSetupParams(BtlSetup *setup, const BattleRecSetup *rec) {
    setup->fieldSituation.env = rec->env;
    setup->rand = rec->rand;
    setup->fieldSituation.bgm = rec->bgm;
    setup->fieldSituation.unk12 = rec->unk2E;
    setup->battleType = rec->battleType;
    setup->battleStyle = rec->battleStyle;
    setup->fieldSituation.unk1a = rec->unk33_4;
    setup->unkA2 = rec->unk30;
    setup->fieldSituation.unk19 = rec->unk32_5;
    setup->unk97 = rec->unk33_7;
    return TRUE;
}

static void BattleRecTool_StorePokewoodParty(const BtlSetup *setup, PokewoodBlock *block) {
    MI_CpuFill8(&block->party, 0, sizeof(BattleRecParty));
    if (setup->party[0] != NULL) {
        BattleRecTool_StorePokewoodPartyCore(setup->party[0], &block->party);
    }
}

static void BattleRecTool_LoadPokewoodParty(BtlSetup *setup, PokewoodBlock *block, HeapID heapId) {
    if (setup->party[0] != NULL) {
        BattleRecTool_LoadPokewoodPartyCore(&block->party, setup->party[0], heapId);
    }
}

static BOOL BattleRecTool_StorePokewoodRecData(const BtlSetup *setup, PokewoodBlock *block) {
    void *data = setup->unkB0;
    u32 size;

    if (data != NULL) {
        size = setup->unkB4;
        if (size < sizeof(block->data)) {
            block->dataSize = size;
            MI_CpuCopy8(data, block->data, size);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL BattleRecTool_LoadPokewoodRecData(BtlSetup *setup, const PokewoodBlock *block) {
    setup->unkB4 = block->dataSize;
    MI_CpuCopy8(block->data, setup->unkB0, setup->unkB4);
    return TRUE;
}

static void BattleRecTool_StorePokewoodPartyCore(PokeParty *party, BattleRecParty *rec) {
    int i;

    MI_CpuFill8(rec, 0, sizeof(BattleRecParty));
    if (PokeParty_GetPkmCount(party) != 0) {
        rec->capacity = BattleRecTool_Min(6, PokeParty_GetCapacity(party));
        rec->count = BattleRecTool_Min(PokeParty_GetPkmCount(party), rec->capacity);
        for (i = 0; i < rec->count; i++) {
            BattleRecTool_StorePkm(PokeParty_GetPkm(party, i), &rec->pkm[i]);
        }
    }
}

static void BattleRecTool_LoadPokewoodPartyCore(BattleRecParty *rec, PokeParty *party, HeapID heapId) {
    int i;
    PartyPkm *pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), TRUE, "battle_rec_tool.c", 742);

    PokeParty_InitCore(party, 6);
    for (i = 0; i < rec->count; i++) {
        BattleRecTool_LoadPkm(&rec->pkm[i], pkm);
        PokeParty_SetParam(pkm, 0x9f, 0);
        PokeParty_AddPkm(party, pkm);
    }
    GFL_HeapFree(pkm);
}

static void BattleRecTool_StorePokewoodInfo(const BtlSetup *setup, PokewoodBattleInfo *info) {
    info->unkDF = setup->unkDF;
    info->unk124 = setup->unk124;
    MI_CpuCopy8(setup->unk110, info->unk110, sizeof(info->unk110));
}

static void BattleRecTool_LoadPokewoodInfo(BtlSetup *setup, const PokewoodBattleInfo *info) {
    setup->unkDF = info->unkDF;
    setup->unk124 = info->unk124;
    MI_CpuCopy8(info->unk110, setup->unk110, sizeof(setup->unk110));
}

static void BattleRecTool_StorePokewoodExtra(const BtlSetup *setup, PokewoodBlock *block) {
    block->unk128 = setup->unk128;
    block->unkE0 = setup->unkE0;
    block->unk130 = setup->unk130;
}

static void BattleRecTool_LoadPokewoodExtra(BtlSetup *setup, const PokewoodBlock *block) {
    setup->unk128 = block->unk128;
    setup->unkE0 = block->unkE0;
}
