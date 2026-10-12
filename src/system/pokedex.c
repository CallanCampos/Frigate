#include "types.h"
#include "save/pokedex.h"
#include "constants/language.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The Pokédex's save block: the species caught, the ones seen by sex, shininess and form, which of them the Pokédex
// shows, and the languages of the entries. The ROM has no string for the file, so the name is a guess. The names
// are swan's where there is one. Block 0x36.

#define SAVE_BLOCK_POKEDEX 0x36

#define NATIONAL_DEX_COMPLETE 636
#define UNOVA_DEX_COMPLETE 297
// A species that isn't in the regional Pokédex, in its table
#define REGIONAL_DEX_NONE 999

// A species with forms and the count of them
typedef struct {
    u16 species;
    u16 count;
} PokeDexFormEntry;

// The professor's evaluation for up to max Pokémon. The list ends with a max of 0, for any count
typedef struct {
    u16 max;
    u16 value;
} PokeDexEvaluation;

static BOOL PML_PkmIsBadMonsNo(u16 species);
static void func_0200cea4(PokeDexSave *pokedex, u32 species, u32 language);
static BOOL PML_PkmIsNationalDexExclude(u16 species);
static BOOL PML_PkmIsRegionalDexExclude(u16 species);
static int getIndexPokemonWithForms(u16 species);
static int getIndexNumOfPkmForm(u16 species);
static void func_0200cf88(PokeDexSave *pokedex, u32 species, u32 sex, BOOL rare);
static void getForm(PokeDexSave *pokedex, u32 species, u32 sex, BOOL rare, u32 form);
static u16 PokeDex_GetCaughtNo(PokeDexSave *pokedex, HeapID heapId);
static u16 PokeDex_GetCaughtNoPermissive(PokeDexSave *pokedex);
static u16 PokeDex_GetCaughtNoUnovaPermissive(PokeDexSave *pokedex, HeapID heapId);
static u16 PokeDex_GetSeenNoUnovaPermissive(PokeDexSave *pokedex, HeapID heapId);
static u16 func_0200dbb4(const PokeDexEvaluation *table, int count);

static const u16 UNOVA_DEX_COMPLETION_EXCLUSIONS[] = {
    SPECIES_KELDEO,
    SPECIES_MELOETTA,
    SPECIES_GENESECT,
    SPECIES_VICTINI,
};

static const PokeDexEvaluation data_0208f814[] = {
    { 39, 1311 }, { 99, 1312 }, { 159, 1313 }, { 229, 1314 }, { 296, 1315 }, { 0, 1316 },
};

static const PokeDexEvaluation data_0208f82c[] = {
    { 159, 1311 }, { 349, 1312 }, { 449, 1313 }, { 549, 1314 }, { 635, 1315 }, { 0, 1316 },
};

static const u16 NATIONAL_DEX_COMPLETION_EXCLUSIONS[] = {
    SPECIES_MEW,     SPECIES_CELEBI,  SPECIES_JIRACHI, SPECIES_DEOXYS,   SPECIES_PHIONE,
    SPECIES_MANAPHY, SPECIES_DARKRAI, SPECIES_SHAYMIN, SPECIES_ARCEUS,   SPECIES_KELDEO,
    SPECIES_MELOETTA, SPECIES_GENESECT, SPECIES_VICTINI,
};

static const PokeDexEvaluation data_0208f85e[] = {
    { 14, 4 },   { 29, 5 },   { 44, 6 },   { 64, 7 },   { 84, 8 },   { 104, 9 },  { 134, 10 }, { 164, 11 },
    { 194, 12 }, { 224, 13 }, { 244, 14 }, { 264, 15 }, { 284, 16 }, { 296, 17 }, { 0, 17 },
};

static const PokeDexEvaluation data_0208f89a[] = {
    { 14, 19 },  { 29, 20 },  { 44, 21 },  { 64, 22 },  { 84, 23 },  { 104, 24 }, { 134, 25 }, { 164, 26 },
    { 194, 27 }, { 224, 28 }, { 244, 29 }, { 264, 30 }, { 284, 31 }, { 296, 32 }, { 0, 32 },
};

static const PokeDexEvaluation data_0208f8d8[] = {
    { 49, 44 },  { 104, 45 }, { 164, 46 }, { 229, 47 }, { 289, 48 }, { 344, 49 }, { 394, 50 }, { 444, 51 }, { 494, 52 },
    { 529, 53 }, { 559, 54 }, { 579, 55 }, { 599, 56 }, { 614, 57 }, { 629, 58 }, { 635, 59 }, { 0, 59 },
};

static const PokeDexFormEntry pokemonFormList[] = {
    { SPECIES_UNOWN, 28 },     { SPECIES_DEOXYS, 4 },   { SPECIES_SHAYMIN, 2 },  { SPECIES_GIRATINA, 2 },
    { SPECIES_ROTOM, 6 },      { SPECIES_SHELLOS, 2 },  { SPECIES_GASTRODON, 2 }, { SPECIES_BURMY, 3 },
    { SPECIES_WORMADAM, 3 },   { SPECIES_CASTFORM, 4 }, { SPECIES_CHERRIM, 2 },  { SPECIES_DEERLING, 4 },
    { SPECIES_SAWSBUCK, 4 },   { SPECIES_MELOETTA, 2 }, { SPECIES_DARMANITAN, 2 }, { SPECIES_BASCULIN, 2 },
    { SPECIES_KYUREM, 3 },     { SPECIES_KELDEO, 2 },   { SPECIES_THUNDURUS, 2 }, { SPECIES_TORNADUS, 2 },
    { SPECIES_LANDORUS, 2 },   { 0, 0 },
};

static inline void SetBit(u8 *bits, u16 index) {
    bits[index >> 3] |= (u8)(1 << (index & 7));
}

static inline BOOL CheckBit(const u8 *bits, u16 index) {
    if (bits[index >> 3] & (1 << (index & 7))) {
        return TRUE;
    }
    return FALSE;
}

// The same by species
static inline void SetSpeciesBit(u8 *bits, u16 species) {
    u16 bit = species - 1;

    bits[bit >> 3] |= (u8)(1 << (bit & 7));
}

static inline BOOL CheckSpeciesBit(const u8 *bits, u16 species) {
    u16 bit = species - 1;

    if (bits[bit >> 3] & (1 << (bit & 7))) {
        return TRUE;
    }
    return FALSE;
}

// The list of the species seen of a kind
static inline const u8 *SeenList(const PokeDexSave *pokedex, u16 kind) {
    return pokedex->seen + kind * POKEDEX_SPECIES_BYTES;
}

static inline void SetSeen(PokeDexSave *pokedex, u16 kind, u16 species) {
    SetSpeciesBit(pokedex->seen + kind * POKEDEX_SPECIES_BYTES, species);
}

static inline BOOL CheckSeen(const PokeDexSave *pokedex, u16 kind, u16 species) {
    return CheckSpeciesBit(SeenList(pokedex, kind), species);
}

static inline BOOL CheckShown(const PokeDexSave *pokedex, u16 kind, u16 species) {
    return CheckSpeciesBit(pokedex->shown + kind * POKEDEX_SPECIES_BYTES, species);
}

static BOOL PML_PkmIsBadMonsNo(u16 species) {
    if (species == 0 || species > SPECIES_GENESECT) {
        return TRUE;
    }
    return FALSE;
}

// Adds a language to the ones a species was caught in
static void func_0200cea4(PokeDexSave *pokedex, u32 species, u32 language) {
    if (language > LANGUAGE_KOREAN || language == 6 || language == 0) {
        return;
    }
    if (species > POKEDEX_LANGUAGE_SPECIES_MAX) {
        return;
    }
    if (language >= LANGUAGE_SPANISH) {
        language--;
    }
    SetBit(pokedex->languages, (species - 1) * POKEDEX_LANGUAGE_COUNT + (language - 1));
}

// Whether a species counts for the national Pokédex's completion, and for the regional one's
static BOOL PML_PkmIsNationalDexExclude(u16 species) {
    u32 i;

    for (i = 0; i < NELEMS(NATIONAL_DEX_COMPLETION_EXCLUSIONS); i++) {
        if (species == NATIONAL_DEX_COMPLETION_EXCLUSIONS[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL PML_PkmIsRegionalDexExclude(u16 species) {
    u32 i;

    for (i = 0; i < NELEMS(UNOVA_DEX_COMPLETION_EXCLUSIONS); i++) {
        if (species == UNOVA_DEX_COMPLETION_EXCLUSIONS[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

// The bit of a species' first form in the lists of forms, or -1 for a species without forms
static int getIndexPokemonWithForms(u16 species) {
    int index = 0;
    int i = 0;

    do {
        if (species == pokemonFormList[i].species) {
            return index;
        }
        index += pokemonFormList[i].count;
        i++;
    } while (pokemonFormList[i].species != 0);
    return -1;
}

// The species' entry in pokemonFormList, or -1
static int getIndexNumOfPkmForm(u16 species) {
    int i = 0;

    do {
        if (species == pokemonFormList[i].species) {
            return i;
        }
        i++;
    } while (pokemonFormList[i].species != 0);
    return -1;
}

// Sets a species as seen in a sex and shininess
static void func_0200cf88(PokeDexSave *pokedex, u32 species, u32 sex, BOOL rare) {
    if (sex == 0) {
        if (rare == TRUE) {
            SetSeen(pokedex, POKEDEX_KIND_RARE_MALE, species);
        } else {
            SetSeen(pokedex, POKEDEX_KIND_MALE, species);
        }
    } else if (sex == 1) {
        if (rare == TRUE) {
            SetSeen(pokedex, POKEDEX_KIND_RARE_FEMALE, species);
        } else {
            SetSeen(pokedex, POKEDEX_KIND_FEMALE, species);
        }
    } else {
        if (rare == TRUE) {
            SetSeen(pokedex, POKEDEX_KIND_RARE_MALE, species);
        } else {
            SetSeen(pokedex, POKEDEX_KIND_MALE, species);
        }
    }
}

// Sets a form of a species as seen, and all the forms of the species that change form in battle
static void getForm(PokeDexSave *pokedex, u32 species, u32 sex, BOOL rare, u32 form) {
    int index = getIndexPokemonWithForms(species);
    u32 count;
    u16 i;

    if (index == -1) {
        return;
    }
    if (species == SPECIES_CASTFORM) {
        form = 0;
        count = 4;
    } else if (species == SPECIES_CHERRIM) {
        form = 0;
        count = 2;
    } else if (species == SPECIES_DARMANITAN) {
        form = 0;
        count = 2;
    } else if (species == SPECIES_MELOETTA) {
        form = 0;
        count = 2;
    } else {
        if (form >= getNumberOfForms(species)) {
            form = 0;
        }
        count = 1;
    }
    if (rare == TRUE) {
        for (i = 0; i < count; i++) {
            SetBit(pokedex->seenForms + POKEDEX_FORM_BYTES, i + (index + form));
        }
    } else {
        for (i = 0; i < count; i++) {
            SetBit(pokedex->seenForms, i + (index + form));
        }
    }
}

u32 getSizeofPokedexData(void) {
    return sizeof(PokeDexSave);
}

void initPokedex(PokeDexSave *pokedex) {
    sys_memset(pokedex, 0, sizeof(PokeDexSave));
    pokedex->magic = POKEDEX_MAGIC;
    givePlayerPokedex(pokedex);
}

PokeDexSave *getPokedexSaveAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_POKEDEX);
}

PokeDexSave *GameData_GetPokedex(GameData *gameData) {
    return getPokedexSaveAddress(GameData_GetSaveControl(gameData));
}

void PokeDex_SetNationalObtained(PokeDexSave *pokedex) {
    pokedex->nationalObtained = TRUE;
}

BOOL PokeDex_IsNationalObtained(PokeDexSave *pokedex) {
    return pokedex->nationalObtained;
}

void PokeDex_EnableHabitatList(PokeDexSave *pokedex) {
    pokedex->habitatListEnabled = TRUE;
}

BOOL PokeDex_IsHabitatListEnabled(PokeDexSave *pokedex) {
    return pokedex->habitatListEnabled;
}

void givePlayerPokedex(PokeDexSave *pokedex) {
    pokedex->unk4_2 = 1;
}

u32 func_0200d1dc(PokeDexSave *pokedex) {
    return pokedex->unk4_2;
}

void func_0200d1e4(PokeDexSave *pokedex, u32 mode) {
    pokedex->mode = mode;
}

u32 func_0200d1f8(PokeDexSave *pokedex) {
    return pokedex->mode;
}

void func_0200d200(PokeDexSave *pokedex, u16 species) {
    pokedex->lastSpecies = species;
}

u16 func_0200d214(PokeDexSave *pokedex) {
    return pokedex->lastSpecies;
}

void addToDex(PokeDexSave *pokedex, u16 species, u32 sex, BOOL rare, u32 form) {
    int index = 0;
    u32 i = 0;
    u32 j;
    u8 *shown;
    u16 bit;
    u32 byte;
    u32 flag;
    u8 mask;

    do {
        if (species == pokemonFormList[i].species) {
            u32 count = pokemonFormList[i].count;
            u8 *normalForms = pokedex->shownForms;
            u8 *rareForms = pokedex->shownForms + POKEDEX_FORM_BYTES;

            for (j = 0; j < count; j++) {
                u16 formBit = index + j;
                u8 formMask = (1 << (formBit & 7)) ^ 0xff;

                normalForms[formBit >> 3] &= formMask;
                rareForms[formBit >> 3] &= formMask;
            }
            if (form >= count) {
                form = 0;
            }
            if (rare == TRUE) {
                SetBit(pokedex->shownForms + POKEDEX_FORM_BYTES, index + form);
            } else {
                SetBit(pokedex->shownForms, index + form);
            }
            break;
        }
        index += pokemonFormList[i].count;
        i++;
    } while (pokemonFormList[i].species != 0);

    bit = species - 1;
    shown = pokedex->shown;
    byte = bit >> 3;
    flag = 1 << (bit & 7);
    mask = flag ^ 0xff;
    for (i = 0; i < POKEDEX_KIND_COUNT; i++) {
        shown[i * POKEDEX_SPECIES_BYTES + byte] &= mask;
    }
    if (sex == 0) {
        if (rare == TRUE) {
            pokedex->shown[POKEDEX_KIND_RARE_MALE * POKEDEX_SPECIES_BYTES + byte] |= (u8)flag;
        } else {
            pokedex->shown[POKEDEX_KIND_MALE * POKEDEX_SPECIES_BYTES + byte] |= (u8)flag;
        }
    } else if (sex == 1) {
        if (rare == TRUE) {
            pokedex->shown[POKEDEX_KIND_RARE_FEMALE * POKEDEX_SPECIES_BYTES + byte] |= (u8)flag;
        } else {
            pokedex->shown[POKEDEX_KIND_FEMALE * POKEDEX_SPECIES_BYTES + byte] |= (u8)flag;
        }
    } else {
        if (rare == TRUE) {
            pokedex->shown[POKEDEX_KIND_RARE_MALE * POKEDEX_SPECIES_BYTES + byte] |= (u8)flag;
        } else {
            pokedex->shown[POKEDEX_KIND_MALE * POKEDEX_SPECIES_BYTES + byte] |= (u8)flag;
        }
    }
}

u16 getNumberOfForms(u16 species) {
    int entry = getIndexNumOfPkmForm(species);

    if (entry != -1) {
        return pokemonFormList[entry].count;
    }
    return 1;
}

void func_0200d3c8(PokeDexSave *pokedex, u16 species, u32 *sex, u32 *rare, u32 *form, HeapID heapId) {
    int entry = getIndexNumOfPkmForm(species);
    void *personal;

    if (entry != -1) {
        int index = getIndexPokemonWithForms(species);
        u32 i;

        for (i = 0; i < pokemonFormList[entry].count; i++) {
            if (CheckBit(pokedex->shownForms, index + i)) {
                *form = i;
                *rare = FALSE;
                break;
            }
            if (CheckBit(pokedex->shownForms + POKEDEX_FORM_BYTES, index + i)) {
                *form = i;
                *rare = TRUE;
                break;
            }
        }
        if (CheckShown(pokedex, POKEDEX_KIND_MALE, species)
            || CheckShown(pokedex, POKEDEX_KIND_RARE_MALE, species)) {
            *sex = 0;
        }
        if (CheckShown(pokedex, POKEDEX_KIND_FEMALE, species)
            || CheckShown(pokedex, POKEDEX_KIND_RARE_FEMALE, species)) {
            *sex = 1;
        }
    } else {
        *form = 0;
        if (CheckShown(pokedex, POKEDEX_KIND_MALE, species)) {
            *sex = 0;
            *rare = FALSE;
        }
        if (CheckShown(pokedex, POKEDEX_KIND_FEMALE, species)) {
            *sex = 1;
            *rare = FALSE;
        }
        if (CheckShown(pokedex, POKEDEX_KIND_RARE_MALE, species)) {
            *sex = 0;
            *rare = TRUE;
        }
        if (CheckShown(pokedex, POKEDEX_KIND_RARE_FEMALE, species)) {
            *sex = 1;
            *rare = TRUE;
        }
    }
    personal = PML_PersonalLoad(species, *form, heapId);
    if (PML_PersonalGetParam(personal, PERSONAL_SEX_RATIO) == 0xff) {
        *sex = 2;
    }
    PML_PersonalFree(personal);
}

void addPkmToDex(PokeDexSave *pokedex, PartyPkm *pkm) {
    BOOL wasEncrypted = PokeParty_DecryptPkm(pkm);
    BOOL rare;
    u32 personality;
    u32 language;
    u32 sex;
    u32 form;
    u16 species;

    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
        PokeParty_EncryptPkm(pkm, wasEncrypted);
        return;
    }
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    personality = PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    language = PokeParty_GetParam(pkm, PKM_PARAM_REGION, NULL);
    sex = PokeParty_GetSex(pkm);
    rare = PokeParty_IsRare(pkm);
    PokeParty_EncryptPkm(pkm, wasEncrypted);
    if (PML_PkmIsBadMonsNo(species)) {
        return;
    }
    if (!PokeDex_IsSeen(pokedex, species)) {
        addToDex(pokedex, species, sex, rare, form);
    }
    if (species == SPECIES_SPINDA && pokedex->spindaPersonality == 0) {
        pokedex->spindaPersonality = personality;
    }
    func_0200cf88(pokedex, species, sex, rare);
    getForm(pokedex, species, sex, rare, form);
    func_0200cea4(pokedex, species, language);
    SetSpeciesBit(pokedex->caught, species);
}

BOOL PokeDex_IsCaught(PokeDexSave *pokedex, u16 species) {
    if (PML_PkmIsBadMonsNo(species)) {
        return FALSE;
    }
    return CheckSpeciesBit(pokedex->caught, species);
}

static u16 PokeDex_GetCaughtNo(PokeDexSave *pokedex, HeapID heapId) {
    if (PokeDex_IsNationalObtained(pokedex)) {
        return PokeDex_GetCaughtNoNational(pokedex);
    }
    return PokeDex_GetCaughtNoUnova(pokedex, heapId);
}

u16 PokeDex_GetCaughtNoNational(PokeDexSave *pokedex) {
    int count = 0;
    int i;

    for (i = 1; i <= SPECIES_GENESECT; i++) {
        if (PokeDex_IsCaught(pokedex, i) == TRUE) {
            count++;
        }
    }
    return count;
}

u16 PokeDex_GetCaughtNoUnova(PokeDexSave *pokedex, HeapID heapId) {
    u16 i;
    u16 count = 0;
    u16 *table = PML_PersonalLoadRegionalDexTable(heapId, 0);

    for (i = 1; i <= SPECIES_GENESECT; i++) {
        if (PokeDex_IsCaught(pokedex, i) == TRUE && table[i] != REGIONAL_DEX_NONE) {
            count++;
        }
    }
    GFL_HeapFree(table);
    return count;
}

void PokeDex_RegistPkm(PokeDexSave *pokedex, PartyPkm *pkm) {
    BOOL wasEncrypted = PokeParty_DecryptPkm(pkm);
    BOOL rare;
    u32 personality;
    u32 sex;
    u32 form;
    u16 species;

    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
        PokeParty_EncryptPkm(pkm, wasEncrypted);
        return;
    }
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    personality = PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    sex = PokeParty_GetSex(pkm);
    rare = PokeParty_IsRare(pkm);
    PokeParty_EncryptPkm(pkm, wasEncrypted);
    if (PML_PkmIsBadMonsNo(species)) {
        return;
    }
    if (!PokeDex_IsSeen(pokedex, species)) {
        addToDex(pokedex, species, sex, rare, form);
    }
    if (species == SPECIES_SPINDA && pokedex->spindaPersonality == 0) {
        pokedex->spindaPersonality = personality;
    }
    func_0200cf88(pokedex, species, sex, rare);
    getForm(pokedex, species, sex, rare, form);
}

BOOL PokeDex_IsSeen(PokeDexSave *pokedex, u16 species) {
    u32 i;

    if (PML_PkmIsBadMonsNo(species)) {
        return FALSE;
    }
    for (i = 0; i < POKEDEX_KIND_COUNT; i++) {
        if (CheckSeen(pokedex, i, species)) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 countSeenDexPokes(PokeDexSave *pokedex, HeapID heapId) {
    if (PokeDex_IsNationalObtained(pokedex)) {
        return PokeDex_GetSeenNoNational(pokedex);
    }
    return PokeDex_GetSeenNoUnova(pokedex, heapId);
}

u16 PokeDex_GetSeenNoNational(PokeDexSave *pokedex) {
    int count = 0;
    int i;

    for (i = 1; i <= SPECIES_GENESECT; i++) {
        if (PokeDex_IsSeen(pokedex, i) == TRUE) {
            count++;
        }
    }
    return count;
}

u16 PokeDex_GetSeenNoUnova(PokeDexSave *pokedex, HeapID heapId) {
    u16 i;
    u16 count = 0;
    u16 *table = PML_PersonalLoadRegionalDexTable(heapId, 0);

    for (i = 1; i <= SPECIES_GENESECT; i++) {
        if (PokeDex_IsSeen(pokedex, i) == TRUE && table[i] != REGIONAL_DEX_NONE) {
            count++;
        }
    }
    GFL_HeapFree(table);
    return count;
}

BOOL func_0200d8d4(PokeDexSave *pokedex, u16 species, u32 sex, BOOL rare, u32 form) {
    int index;

    if (PML_PkmIsBadMonsNo(species)) {
        return -1;
    }
    index = getIndexPokemonWithForms(species);
    if (index != -1) {
        return CheckBit(pokedex->seenForms + rare * POKEDEX_FORM_BYTES, index + form);
    }
    if (sex == 0) {
        if (rare == TRUE) {
            return CheckSeen(pokedex, POKEDEX_KIND_RARE_MALE, species);
        } else {
            return CheckSeen(pokedex, POKEDEX_KIND_MALE, species);
        }
    } else if (sex == 1) {
        if (rare == TRUE) {
            return CheckSeen(pokedex, POKEDEX_KIND_RARE_FEMALE, species);
        } else {
            return CheckSeen(pokedex, POKEDEX_KIND_FEMALE, species);
        }
    } else {
        if (rare == TRUE) {
            if (CheckSeen(pokedex, POKEDEX_KIND_RARE_MALE, species)
                || CheckSeen(pokedex, POKEDEX_KIND_RARE_FEMALE, species)) {
                return TRUE;
            }
        } else {
            if (CheckSeen(pokedex, POKEDEX_KIND_MALE, species)
                || CheckSeen(pokedex, POKEDEX_KIND_FEMALE, species)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

u32 func_0200da18(PokeDexSave *pokedex, u32 index) {
    if (index == 0) {
        return pokedex->spindaPersonality;
    }
    return 0;
}

BOOL PokeDex_IsCompleteNational(PokeDexSave *pokedex) {
    if (PokeDex_GetCaughtNoPermissive(pokedex) >= NATIONAL_DEX_COMPLETE) {
        return TRUE;
    }
    return FALSE;
}

BOOL PokeDex_IsCompleteUnova(PokeDexSave *pokedex, HeapID heapId) {
    if (PokeDex_GetCaughtNoUnovaPermissive(pokedex, heapId) >= UNOVA_DEX_COMPLETE) {
        return TRUE;
    }
    return FALSE;
}

BOOL PokeDex_IsSeenAllUnova(PokeDexSave *pokedex, HeapID heapId) {
    if (PokeDex_GetSeenNoUnovaPermissive(pokedex, heapId) >= UNOVA_DEX_COMPLETE) {
        return TRUE;
    }
    return FALSE;
}

// The counts without the mythical species, which the Pokédex is complete without
static u16 PokeDex_GetCaughtNoPermissive(PokeDexSave *pokedex) {
    int i;
    u16 count = 0;

    for (i = 1; i <= SPECIES_GENESECT; i++) {
        if (PokeDex_IsCaught(pokedex, i) == TRUE && PML_PkmIsNationalDexExclude(i) == TRUE) {
            count++;
        }
    }
    return count;
}

static u16 PokeDex_GetCaughtNoUnovaPermissive(PokeDexSave *pokedex, HeapID heapId) {
    u16 i;
    u16 count = 0;
    u16 *table = PML_PersonalLoadRegionalDexTable(heapId, 0);

    for (i = 1; i <= SPECIES_GENESECT; i++) {
        if (PokeDex_IsCaught(pokedex, i) == TRUE && table[i] != REGIONAL_DEX_NONE
            && PML_PkmIsRegionalDexExclude(i) == TRUE) {
            count++;
        }
    }
    GFL_HeapFree(table);
    return count;
}

static u16 PokeDex_GetSeenNoUnovaPermissive(PokeDexSave *pokedex, HeapID heapId) {
    u16 i;
    u16 count = 0;
    u16 *table = PML_PersonalLoadRegionalDexTable(heapId, 0);

    for (i = 1; i <= SPECIES_GENESECT; i++) {
        if (PokeDex_IsSeen(pokedex, i) == TRUE && table[i] != REGIONAL_DEX_NONE
            && PML_PkmIsRegionalDexExclude(i) == TRUE) {
            count++;
        }
    }
    GFL_HeapFree(table);
    return count;
}

BOOL func_0200db5c(PokeDexSave *pokedex, u16 species, u32 language) {
    if (language > LANGUAGE_KOREAN || language == 6 || language == 0) {
        return FALSE;
    }
    if (species > POKEDEX_LANGUAGE_SPECIES_MAX) {
        return FALSE;
    }
    if (language >= LANGUAGE_SPANISH) {
        language--;
    }
    return CheckBit(pokedex->languages, (species - 1) * POKEDEX_LANGUAGE_COUNT + (language - 1));
}

static u16 func_0200dbb4(const PokeDexEvaluation *table, int count) {
    int i;

    for (i = 0;; i++) {
        if (table[i].max == 0) {
            return table[i].value;
        }
        if (count <= table[i].max) {
            return table[i].value;
        }
    }
}

void PokeDex_GetEvaluationParams(PokeDexSave *pokedex, u16 mode, HeapID heapId, u16 *message, u16 *count, u16 *unk) {
    PokeDex_IsNationalObtained(pokedex);
    *message = 5;
    *count = 0;
    *unk = 1311;
    switch (mode) {
    case 3:
        *count = PokeDex_GetCaughtNoUnova(pokedex, heapId);
        if (PokeDex_IsCompleteUnova(pokedex, heapId) == TRUE) {
            *message = 33;
            *unk = 1316;
            return;
        }
        *message = func_0200dbb4(data_0208f89a, *count);
        *unk = func_0200dbb4(data_0208f814, PokeDex_GetCaughtNoUnovaPermissive(pokedex, heapId));
        break;
    case 2:
        *count = PokeDex_GetSeenNoUnova(pokedex, heapId);
        if (PokeDex_IsSeenAllUnova(pokedex, heapId) == TRUE) {
            *message = 18;
            *unk = 1316;
            return;
        }
        *message = func_0200dbb4(data_0208f85e, *count);
        *unk = func_0200dbb4(data_0208f814, PokeDex_GetSeenNoUnovaPermissive(pokedex, heapId));
        break;
    case 1:
        *count = PokeDex_GetCaughtNo(pokedex, heapId);
        if (PokeDex_IsCompleteNational(pokedex) == TRUE) {
            *message = 60;
            *unk = 1316;
            return;
        }
        *message = func_0200dbb4(data_0208f8d8, *count);
        *unk = func_0200dbb4(data_0208f82c, PokeDex_GetCaughtNoPermissive(pokedex));
        break;
    }
}
