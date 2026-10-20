#include "types.h"
#include "constants/pokemon.h"
#include "field/pokewood_system.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/medal_box.h"
#include "save/pokedex.h"
#include "save/pokewood.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The regulation of the movies' battles, in arc 106
#define POKEWOOD_REGULATION 5
// The movies of a series, until 0xff, and the medal for making them all
#define SERIES_COUNT 13

typedef struct {
    u8 movies[5];
    u8 medal;
} PokewoodSeries;

// The battle's results, which PokewoodSystem_SetRecord keeps
typedef struct {
    u8 unk0[0x24];
    PokeParty *party;
    u8 unk28[0xfc];
    u32 result;
    u8 unk128[8];
    s32 score;
} PokewoodBattleResult;

// What PokewoodSystem_AddGradePoints adds for each result
static s32 sAmounts[] = { 1, 3, 15 };
// The change to the cast's Pokéstar fame for each result
static s32 sFameChanges[] = { -50, 25, 255 };

PokewoodSystem *PokewoodSystem_Create(HeapID heapId) {
    PokewoodSystem *sys = GFL_HeapAllocate(heapId, sizeof(PokewoodSystem), TRUE, "pokewood_system.c", 51);

    sys->heapId = heapId;
    sys->pokewoodBlk = GFL_HeapAllocate(heapId, getSizeOfPokewoodBlock(), TRUE, "pokewood_system.c", 55);
    return sys;
}

void PokewoodSystem_Free(PokewoodSystem *sys) {
    GFL_HeapFree(sys->pokewoodBlk);
    GFL_HeapFree(sys);
}

void PokewoodSystem_SetCastFromPokeList(PokewoodSystem *sys, const PokeListParam *param) {
    MI_CpuCopy8(param->picked, sys->castSlots, sizeof(sys->castSlots));
    sys->unkE = param->index;
    sys->unk10 = param->result;
    if (sys->unk10 != 0 || sys->unkE == 7 || sys->unkE == 8) {
        *sys->resultVar = 0;
    } else {
        *sys->resultVar = 1;
    }
}

void PokewoodSystem_SetupPokeList(PokewoodSystem *sys, GameData *gameData, PokeListParam *param) {
    PokewoodMovie *movie;

    PokeListParam_Setup(param, gameData, 0x16, GameData_GetParty(gameData));
    PokeRegulation_Load(POKEWOOD_REGULATION, &sys->regulation);
    movie = GFL_ArcSysReadHeapNew(ARCID_POKEWOOD_MOVIE, sys->movie, HEAPID_GAMEEVENT);
    sys->regulation.unk2 = movie->castCount;
    sys->regulation.unk3 = movie->castCount;
    GFL_HeapFree(movie);
    sys->unkCE = 0x11;
    sys->unkCF = 1;
    sys->unkD0 = 0x90;
    param->regulation = &sys->regulation;
    param->unk18 = &sys->unkCE;
    param->unk48 = 0;
}

void PokewoodSystem_SetupPStatus(PokewoodSystem *sys, GameData *gameData, PStatusParam *param) {
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);

    MIi_CpuClear32(0, param, sizeof(PStatusParam));
    param->party = GameData_GetParty(gameData);
    param->dataType = PSTATUS_DATA_PARTY;
    param->partyCount = PokeParty_GetPkmCount(param->party);
    param->mode = PSTATUS_MODE_NORMAL;
    param->page = PSTATUS_PAGE_INFO;
    param->gameData = gameData;
    param->isNationalDex = PokeDex_IsNationalObtained(pokedex);
}

void PokewoodSystem_SetResultVar(PokewoodSystem *sys, u16 *var) {
    sys->resultVar = var;
}

u16 *PokewoodSystem_GetResultVar(PokewoodSystem *sys) {
    return sys->resultVar;
}

void PokewoodSystem_SetUsePlayerCast(PokewoodSystem *sys, u16 value) {
    sys->unkD8 = value;
}

u16 PokewoodSystem_GetUsePlayerCast(PokewoodSystem *sys) {
    return sys->unkD8;
}

void PokewoodSystem_SetMovie(PokewoodSystem *sys, int movie) {
    sys->movie = movie;
}

u16 PokewoodSystem_GetMovie(PokewoodSystem *sys) {
    return sys->movie;
}

void PokewoodSystem_SetSaveSlot(PokewoodSystem *sys, u16 value) {
    sys->unkDA = value;
}

u16 PokewoodSystem_GetSaveSlot(PokewoodSystem *sys) {
    return sys->unkDA;
}

void *PokewoodSystem_GetPokewoodBlk(PokewoodSystem *sys) {
    return sys->pokewoodBlk;
}

// Fills party with healed copies of the cast
void PokewoodSystem_CopyCast(PokewoodSystem *sys, GameData *gameData, PokeParty *party) {
    PokeParty *playerParty;
    PartyPkm *copy;
    PartyPkm *pkm;
    u32 i;
    u32 count;
    HeapID heapId;
    u8 slot;

    PokeParty_InitCore(party, 6);
    playerParty = GameData_GetParty(gameData);
    heapId = HEAPID_TAIL(sys->heapId);
    copy = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), FALSE, "pokewood_system.c", 269);
    PokeParty_ClearPkm(copy);
    count = sys->regulation.unk3;
    for (i = 0; i < count; i++) {
        slot = sys->castSlots[i];
        if (slot == 0) {
            slot = 1;
        }
        pkm = PokeParty_GetPkm(playerParty, slot - 1);
        PokeParty_Recover(pkm);
        copyPartyPkm(pkm, copy);
        PokeParty_AddPkm(party, copy);
    }
    GFL_HeapFree(copy);
}

void PokewoodSystem_SetResultFlag(PokewoodSystem *sys, GameData *gameData, u32 movie, u32 result) {
    PokewoodSave *save = PokewoodSave_Get(gameData);

    if (result == 1) {
        PokewoodSave_SetFlag(save, 2, movie);
    } else if (result == 2) {
        PokewoodSave_SetFlag(save, 4, movie);
    } else if (result == 0) {
        PokewoodSave_SetFlag(save, 6, movie);
    }
}

static const PokewoodSeries sSeries[SERIES_COUNT] = {
    { { 0, 0xff }, 0xcb },           { { 1, 2, 0xff }, 0xcc },           { { 3, 4, 5, 6, 0xff }, 0xcd },
    { { 7, 8, 9, 10, 0xff }, 0xce }, { { 11, 12, 13, 14, 0xff }, 0xcf }, { { 15, 16, 17, 0xff }, 0xd0 },
    { { 18, 19, 20, 0xff }, 0xd1 },  { { 21, 22, 23, 0xff }, 0xd2 },     { { 24, 25, 26, 27, 0xff }, 0xd3 },
    { { 28, 29, 30, 0xff }, 0xd4 },  { { 31, 32, 33, 34, 0xff }, 0xd5 }, { { 35, 36, 37, 38, 0xff }, 0xd6 },
    { { 39, 0xff }, 0xd7 },
};

// Whether every movie of a series is in flag list 1
static BOOL Pokewood_IsSeriesComplete(PokewoodSave *save, u32 series) {
    u32 i;

    for (i = 0; sSeries[series].movies[i] != 0xff; i++) {
        if (PokewoodSave_CheckFlag(save, 1, sSeries[series].movies[i]) == FALSE) {
            break;
        }
    }
    if (sSeries[series].movies[i] == 0xff) {
        return TRUE;
    }
    return FALSE;
}

void Pokewood_CheckMedals(GameData *gameData) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(gameData));
    PokewoodSave *save = PokewoodSave_Get(gameData);
    u32 i;
    u32 count;

    for (i = 0; i < SERIES_COUNT; i++) {
        if (Pokewood_IsSeriesComplete(save, i)) {
            MedalBox_GiveMedal(box, sSeries[i].medal);
        }
    }
    if (PokewoodSave_CountFlags(save, 1) == POKEWOOD_MOVIE_COUNT) {
        MedalBox_GiveMedal(box, 0xd8);
        MedalBox_DiscoverMedal(box, 0xdc);
    }
    if (PokewoodSave_CountFlags(save, 2) == POKEWOOD_MOVIE_COUNT) {
        MedalBox_GiveMedal(box, 0xd9);
    }
    if (func_020111ec(func_020111b0(save)) >= 2) {
        MedalBox_DiscoverMedal(box, 0xd9);
    }
    count = PokewoodSave_CountFlags(save, 4);
    if (count != 0) {
        MedalBox_GiveMedal(box, 0xdb);
    }
    if (count == POKEWOOD_MOVIE_COUNT) {
        MedalBox_GiveMedal(box, 0xdc);
    }
}

void PokewoodSystem_SetRecord(PokewoodSystem *sys, const void *battleResult, u32 movie) {
    const PokewoodBattleResult *result = battleResult;
    PokeParty *party = result->party;
    PokewoodRecord record;
    PartyPkm *pkm;
    u32 species;
    u32 form;
    int i = 0;

    MI_CpuFill8(&record, 0, sizeof(record));
    record.score = result->score;
    for (; i < PokeParty_GetPkmCount(party); i++) {
        pkm = PokeParty_GetPkm(party, i);
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        record.cast[i] = PokeIcon_PackSpecies(species, form, PokeParty_GetParam(pkm, 0x6e, NULL));
        if (PML_GetPokestarFameLevel(PokeParty_GetParam(pkm, PKM_PARAM_POKESTAR_FAME, NULL)) >= 2) {
            record.famousCast |= (u8)(1 << i);
        }
    }
    sys->record = record;
    sys->unkF4 = result->result;
}

void PokewoodSystem_KeepBestRecord(GameData *gameData, PokewoodSystem *sys) {
    PokewoodSave *save = PokewoodSave_Get(gameData);

    if (sys->record.score >= PokewoodSave_GetRecord(save, sys->movie)->score) {
        PokewoodSave_UpdateRecord(save, sys->movie, &sys->record);
    }
}

void PokewoodSystem_RecordScore(GameData *gameData, PokewoodSystem *sys) {
    GameRecords *records = getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData));

    if (sys->record.score > 0) {
        RecordSetIfHigher(records, 0x33, sys->record.score);
    }
    if (sys->record.score > 0) {
        RecordAdd(records, 0x34, sys->record.score);
    }
}

void PokewoodSystem_AddGradePoints(GameData *gameData, PokewoodSystem *sys, u32 result, u32 slot) {
    PokewoodSave *save = PokewoodSave_Get(gameData);

    if (func_020111a0(save, slot) == 0) {
        func_020111d0(save, sAmounts[result]);
    }
}

void PokewoodSystem_UpdateCastFame(GameData *gameData, PokewoodSystem *sys) {
    PokeParty *party = GameData_GetParty(gameData);
    PartyPkm *pkm;
    int fame;
    int i;
    int count;

    if (sys->unkD8 != 0) {
        count = sys->regulation.unk3;
        for (i = 0; i < count; i++) {
            pkm = PokeParty_GetPkm(party, sys->castSlots[i] - 1);
            fame = PokeParty_GetParam(pkm, PKM_PARAM_POKESTAR_FAME, NULL);
            fame += sFameChanges[sys->unkF4];
            if (fame < 0) {
                fame = 0;
            }
            if (fame > 255) {
                fame = 255;
            }
            PokeParty_SetParam(pkm, PKM_PARAM_POKESTAR_FAME, fame);
        }
    }
}

void PokewoodSystem_PushLastSeries(GameData *gameData, PokewoodSystem *sys, u32 slot) {
    // The series of each movie. The table has room for 4 more movies, left 0
    static const u8 sMovieSeries[POKEWOOD_MOVIE_COUNT + 4] = {
        0, 11, 11, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4,  4,  4,  5,  5,
        5, 6,  6,  6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 12,
    };
    PokewoodSave *save = PokewoodSave_Get(gameData);

    if (sys->movie < POKEWOOD_MOVIE_COUNT) {
        PokewoodSave_PushLastSeries(save, slot, sMovieSeries[sys->movie] + 1, sys->unkF8);
    }
}

void PokewoodSystem_SetSlotInfo(PokewoodSystem *sys, const u8 *src) {
    int i;

    for (i = 0; i < 8; i++) {
        sys->unkF8[i] = src[i];
    }
}
