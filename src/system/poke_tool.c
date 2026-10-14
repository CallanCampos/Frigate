#include "types.h"
#include "pml/poke_party.h"
#include "constants/abilities.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/types.h"
#include "constants/version.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/mail.h"
#include "pml/met_data.h"
#include "pml/personal.h"
#include "pml/species_names.h"
#include "pml/waza.h"
#include "save/player_info.h"
#include "system/str_tool.h"
#include "system/version.h"
#include "text/system/species_names.h"

// Pokémon: creating them, their forms, moves and stats, their encryption and their fields, and the cache of the
// species data read last. The file's name is the ROM's string; the functions' names are swan's
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) unless said

// The kind of PML_UtilSetupMetData for a hatched egg
#define MET_DATA_HATCHED 2

// The species of the personal cache when it holds nothing
#define PERSONAL_CACHE_NONE 686

#define NATURE_COUNT 25
// The stats a nature changes
#define NATURE_STAT_COUNT 5
// The flavors a nature likes and dislikes, the columns of stat_nature_adjust
#define FLAVOR_COUNT 5
#define IV_BITS 5
#define IV_MASK 0x1f
#define IV_RANGE 32
// The PID bit that picks the ability, and the one that picked it before Black and White
#define PID_ABILITY_BIT 0x10000
#define PID_ABILITY_BIT_OLD 1

// The friendship a hatched Pokémon starts with
#define HATCHED_FRIENDSHIP 120

#define ROTOM_FORM_COUNT 6

#define MAX_LEVEL 100
#define MOVE_SLOTS 4
#define MAX_PP_UPS 3
// The experience of each level, 0 to MAX_LEVEL, of a growth rate
#define GROWTH_TABLE_SIZE (MAX_LEVEL + 1)
// The ability slots of getPkmAbilSlotNumber
#define ABILITY_SLOT_1 0
#define ABILITY_SLOT_2 1
#define ABILITY_SLOT_HIDDEN 2
// How PML_GenPID treats shininess: never shiny, always shiny, or as the random value falls
#define PID_RARE_NEVER 0
#define PID_RARE_ALWAYS 1
#define PID_RARE_RANDOM 2

// The most effort values a Pokémon has in all, and in one stat
#define PKM_EV_TOTAL_MAX 510
#define PKM_EV_MAX 255

// The sizes of the encrypted data: the four blocks, and the fields that only a Pokémon of the party has
#define PKM_BOX_DATA_SIZE sizeof(PkmBuffer)
#define PKM_PARTY_DATA_SIZE (sizeof(PartyPkm) - sizeof(BoxPkm))

#define LEARNSET_MAX 26
#define LEARNSET_END 0xffff

// A move a species learns at a level
typedef struct {
    u16 move;
    u16 level;
} LearnsetEntry;

static inline BOOL LearnsetEntry_IsEnd(LearnsetEntry entry) {
    return entry.move == LEARNSET_END && entry.level == LEARNSET_END;
}

static inline u16 LearnsetEntry_GetMove(LearnsetEntry entry) {
    return entry.move;
}

static inline u16 LearnsetEntry_GetLevel(LearnsetEntry entry) {
    return entry.level;
}

// The species and form whose data g_PersonalDataCache holds
typedef struct {
    u16 form;
    u16 species;
} PersonalNowLoad;

// The species whose forms share one sprite
static const u16 sSharedSpriteFormSpecies[] = {
    SPECIES_CASTFORM,
    SPECIES_CHERRIM,
    SPECIES_DARMANITAN,
    SPECIES_MELOETTA,
};

// The types of Hidden Power, by the value its IVs give
static const u8 hpTypes[16] = {
    TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON,   TYPE_GROUND,  TYPE_ROCK, TYPE_BUG,    TYPE_GHOST, TYPE_STEEL,
    TYPE_FIRE,     TYPE_WATER,  TYPE_GRASS,    TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON, TYPE_DARK,
};

// Whether each nature likes (1) or dislikes (-1) a flavor: spicy, dry, sweet, bitter and sour, which go with attack,
// special attack, speed, special defense and defense
static const s8 stat_nature_adjust[][FLAVOR_COUNT] = {
    { 0, 0, 0, 0, 0 },  // NATURE_HARDY
    { 1, 0, 0, 0, -1 }, // NATURE_LONELY
    { 1, 0, -1, 0, 0 }, // NATURE_BRAVE
    { 1, -1, 0, 0, 0 }, // NATURE_ADAMANT
    { 1, 0, 0, -1, 0 }, // NATURE_NAUGHTY
    { -1, 0, 0, 0, 1 }, // NATURE_BOLD
    { 0, 0, 0, 0, 0 },  // NATURE_DOCILE
    { 0, 0, -1, 0, 1 }, // NATURE_RELAXED
    { 0, -1, 0, 0, 1 }, // NATURE_IMPISH
    { 0, 0, 0, -1, 1 }, // NATURE_LAX
    { -1, 0, 1, 0, 0 }, // NATURE_TIMID
    { 0, 0, 1, 0, -1 }, // NATURE_HASTY
    { 0, 0, 0, 0, 0 },  // NATURE_SERIOUS
    { 0, -1, 1, 0, 0 }, // NATURE_JOLLY
    { 0, 0, 1, -1, 0 }, // NATURE_NAIVE
    { -1, 1, 0, 0, 0 }, // NATURE_MODEST
    { 0, 1, 0, 0, -1 }, // NATURE_MILD
    { 0, 1, -1, 0, 0 }, // NATURE_QUIET
    { 0, 0, 0, 0, 0 },  // NATURE_BASHFUL
    { 0, 1, 0, -1, 0 }, // NATURE_RASH
    { -1, 0, 0, 1, 0 }, // NATURE_CALM
    { 0, 0, 0, 1, -1 }, // NATURE_GENTLE
    { 0, 0, -1, 1, 0 }, // NATURE_SASSY
    { 0, -1, 0, 1, 0 }, // NATURE_CAREFUL
    { 0, 0, 0, 0, 0 },  // NATURE_QUIRKY
};

// How each nature changes attack, defense, special attack, special defense and speed: 1 raises it, -1 lowers it
static const s8 natureStat[NATURE_COUNT][NATURE_STAT_COUNT] = {
    { 0, 0, 0, 0, 0 },  // NATURE_HARDY
    { 1, -1, 0, 0, 0 }, // NATURE_LONELY
    { 1, 0, 0, 0, -1 }, // NATURE_BRAVE
    { 1, 0, -1, 0, 0 }, // NATURE_ADAMANT
    { 1, 0, 0, -1, 0 }, // NATURE_NAUGHTY
    { -1, 1, 0, 0, 0 }, // NATURE_BOLD
    { 0, 0, 0, 0, 0 },  // NATURE_DOCILE
    { 0, 1, 0, 0, -1 }, // NATURE_RELAXED
    { 0, 1, -1, 0, 0 }, // NATURE_IMPISH
    { 0, 1, 0, -1, 0 }, // NATURE_LAX
    { -1, 0, 0, 0, 1 }, // NATURE_TIMID
    { 0, -1, 0, 0, 1 }, // NATURE_HASTY
    { 0, 0, 0, 0, 0 },  // NATURE_SERIOUS
    { 0, 0, -1, 0, 1 }, // NATURE_JOLLY
    { 0, 0, 0, -1, 1 }, // NATURE_NAIVE
    { -1, 0, 1, 0, 0 }, // NATURE_MODEST
    { 0, -1, 1, 0, 0 }, // NATURE_MILD
    { 0, 0, 1, 0, -1 }, // NATURE_QUIET
    { 0, 0, 0, 0, 0 },  // NATURE_BASHFUL
    { 0, 0, 1, -1, 0 }, // NATURE_RASH
    { -1, 0, 0, 1, 0 }, // NATURE_CALM
    { 0, -1, 0, 1, 0 }, // NATURE_GENTLE
    { 0, 0, 0, 1, -1 }, // NATURE_SASSY
    { 0, 0, -1, 1, 0 }, // NATURE_CAREFUL
    { 0, 0, 0, 0, 0 },  // NATURE_QUIRKY
};

// The offsets of blocks A to D in a Pokémon's data, by bits 13 to 17 of its PID
static const u8 blockShufLUT[32][4] = {
    { 0x00, 0x20, 0x40, 0x60 },
    { 0x00, 0x20, 0x60, 0x40 },
    { 0x00, 0x40, 0x20, 0x60 },
    { 0x00, 0x60, 0x20, 0x40 },
    { 0x00, 0x40, 0x60, 0x20 },
    { 0x00, 0x60, 0x40, 0x20 },
    { 0x20, 0x00, 0x40, 0x60 },
    { 0x20, 0x00, 0x60, 0x40 },
    { 0x40, 0x00, 0x20, 0x60 },
    { 0x60, 0x00, 0x20, 0x40 },
    { 0x40, 0x00, 0x60, 0x20 },
    { 0x60, 0x00, 0x40, 0x20 },
    { 0x20, 0x40, 0x00, 0x60 },
    { 0x20, 0x60, 0x00, 0x40 },
    { 0x40, 0x20, 0x00, 0x60 },
    { 0x60, 0x20, 0x00, 0x40 },
    { 0x40, 0x60, 0x00, 0x20 },
    { 0x60, 0x40, 0x00, 0x20 },
    { 0x20, 0x40, 0x60, 0x00 },
    { 0x20, 0x60, 0x40, 0x00 },
    { 0x40, 0x20, 0x60, 0x00 },
    { 0x60, 0x20, 0x40, 0x00 },
    { 0x40, 0x60, 0x20, 0x00 },
    { 0x60, 0x40, 0x20, 0x00 },
    { 0x00, 0x20, 0x40, 0x60 },
    { 0x00, 0x20, 0x60, 0x40 },
    { 0x00, 0x40, 0x20, 0x60 },
    { 0x00, 0x60, 0x20, 0x40 },
    { 0x00, 0x40, 0x60, 0x20 },
    { 0x00, 0x60, 0x40, 0x20 },
    { 0x20, 0x00, 0x40, 0x60 },
    { 0x20, 0x00, 0x60, 0x40 },
};

static PersonalNowLoad g_PMLPersonalNowLoadForme;
// Buffers for names: a species' or the player's, and an original trainer's to compare with the player's. Defined in
// this order, MWCC puts the second before the first, as the ROM has them
static u16 g_PMLPkmStringTemp[32];
static u16 g_OTNameCompTemp[32];
// The personal data that PML_PersonalLoadBW2 and PML_PersonalLoadBW1 read last
static PersonalData g_PersonalDataCache;
// The level-up moves of the Pokémon that PML_UtilPkmSetDefaultMoves gives its moves
static LearnsetEntry sDefaultMovesLearnset[LEARNSET_MAX];
// The experience table of a growth rate, by level
static u32 g_PMLGrowTblCache[GROWTH_TABLE_SIZE];

static void PML_CreatePkm(BoxPkm *pkm, u16 species, u16 level, u64 trainerId, u32 ivs, u64 pid);
static void setNameChangedPkm(BoxPkm *pkm, u16 species);
static void fillDataForHatchedEgg(BoxPkm *pkm, PlayerInfo *playerInfo, u32 placeName, HeapID heapId);
static void shuffleMovesUpWriteLastSlot(PartyPkm *pkm, u32 slot);
static void setAbilityForForm(BoxPkm *pkm, u16 species, u16 oldSpecies);
static void UpdateGenderOnMonsChange(BoxPkm *pkm, u16 species, u16 oldSpecies);
static BOOL isKamiTrioTransformed(u16 species, u8 form);
static BOOL isKeldeoTransformed(u16 species, u8 form);
static BOOL isNewFormTransformed(u16 species, u8 form);
static u8 getPkmAbilSlotNumber(BoxPkm *pkm);
static void PML_UtilPkmSetDefaultMoves(BoxPkm *pkm);
static u16 PML_PkmLearnMove(BoxPkm *pkm, u16 move);
static void PML_PkmSetLastMove(BoxPkm *pkm, u16 move);
static u16 PML_PkmLearnMoveTutor(BoxPkm *pkm, u16 move);
static void PML_PkmSetMoveKeepPP(BoxPkm *pkm, u16 move, u8 slot);
static u32 getExpForPkm(BoxPkm *pkm);
static u32 getGenderValue(u16 species, u16 form, u8 sex);
static BOOL canPkmLearnTM(BoxPkm *pkm, u8 tm);
static BOOL isTMValidForPkm(u16 species, u16 form, s32 tm);
static BOOL PML_UtilCheckMoveTutorNormal(u16 species, u16 form, s32 moveIndex);
static BOOL PML_UtilCheckMoveTutorPaid(u16 species, u16 form, int moveIndex, u32 tutor);
static s8 isStatAdjustedByPokeNature(BoxPkm *pkm, u32 flavor);
static s8 isStatAdjustedForNature(u8 nature, u32 flavor);
static void PML_PkmSetHiddenAbil(BoxPkm *pkm, u32 species, u32 form);
static void PokeParty_RecalcStatsCore(PartyPkm *pkm);
static BOOL PokeParty_PkmDecryptCheck(PartyPkm *pkm);
static BOOL PML_PkmDecryptCheck(BoxPkm *pkm);
static void PokeParty_PkmEncrypt(PartyPkm *pkm);
static void PML_PkmEncryptForce(BoxPkm *pkm);
static u32 PokeParty_GetParamCore(PartyPkm *pkm, PkmField field, void *buffer);
static u32 PML_PkmGetParamCore(BoxPkm *pkm, u32 field, void *buffer);
static void PokeParty_SetParamCore(PartyPkm *pkm, PkmField field, u32 value);
static void PML_PkmSetParamCore(BoxPkm *pkm, u32 field, u32 value);
static void writePkmBattleData(PartyPkm *pkm, int param, int value);
static void PML_PkmAddParam(BoxPkm *pkm, int param, int value);
static void addClampedU8(u8 *field, int value, int max);
static u16 PML_CryptoNextKey(u32 *key);
static BOOL PML_PkmIsNicknamed(BoxPkm *pkm);
static u16 adjustStatForNature(u32 nature, u16 value, u32 stat);
static void setNameToNick(BoxPkm *pkm);
static void PML_GrowTblRead(u32 growthRate, u32 *dest);
static u32 PML_GrowTblGetLvExp(u32 growthRate, u32 level);
static u16 PML_ItemGetArceusPlateType(u16 item);
static PersonalData *PML_PersonalLoadBW2(u16 species, u16 form);
static u32 GetPersonalAbilCount(PersonalData *personal);
static PersonalData *PML_PersonalLoadBW1(u16 species, u16 form);

void PML_PersonalInit(HeapID heapId) {
    PML_PersonalArcInit(heapId);
    g_PMLPersonalNowLoadForme.species = PERSONAL_CACHE_NONE;
}

u32 PML_GetPkmRawSize(void) {
    return sizeof(BoxPkm);
}

u32 PokeParty_GetPkmRawSize(void) {
    return sizeof(PartyPkm);
}

PartyPkm *PokeParty_NewTempPkm(u16 species, u16 level, u64 trainerId, HeapID heapId) {
    PartyPkm *pkm = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), TRUE, "poke_tool.c", 160);

    if (pkm != NULL) {
        PokeParty_CreateTempPkm(pkm, species, level, trainerId);
    }
    return pkm;
}

PartyPkm *PokeParty_NewPkm(u16 species, u16 level, u64 trainerId, s32 ivs, u64 pid, HeapID heapId) {
    PartyPkm *pkm = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), TRUE, "poke_tool.c", 186);

    if (pkm != NULL) {
        PokeParty_CreatePkm(pkm, species, level, trainerId, ivs, pid);
    }
    return pkm;
}

PartyPkm *boxPkmRegenToPartyPkm(BoxPkm *pkm, HeapID heapId) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    u16 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 level = PML_PkmGetParam(pkm, PKM_PARAM_LEVEL, NULL);
    u32 id = PML_PkmGetParam(pkm, PKM_PARAM_ID, NULL);
    PartyPkm *partyPkm = PokeParty_NewTempPkm(species, level, id, heapId);
    MailData *mail;

    PML_PkmReEncrypt(pkm, wasEncrypted);
    partyPkm->base = *pkm;
    PokeParty_SetParam(partyPkm, PKM_PARAM_STATUS, 0);
    PokeParty_SetParam(partyPkm, PKM_PARAM_HP, 0);
    PokeParty_SetParam(partyPkm, PKM_PARAM_MAX_HP, 0);
    mail = CreateMailData(HEAPID_TAIL(heapId));
    PokeParty_SetParam(partyPkm, PKM_PARAM_MAIL, (u32)mail);
    GFL_HeapFree(mail);
    PokeParty_RecalcStats(partyPkm);
    return partyPkm;
}

void PokeParty_ClearPkm(PartyPkm *pkm) {
    sys_memset32_fast(0, pkm, sizeof(PartyPkm));
    PML_CryptoRun(&pkm->base.contentBuffer, sizeof(PkmBuffer), pkm->base.checksum);
    PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), pkm->base.pid);
}

void PML_PkmInit(BoxPkm *pkm) {
    sys_memset32_fast(0, pkm, sizeof(BoxPkm));
    PML_CryptoRun(&pkm->contentBuffer, sizeof(PkmBuffer), pkm->checksum);
}

void PokeParty_CreateTempPkm(PartyPkm *pkm, u16 species, u16 level, u64 id) {
    PokeParty_CreatePkm(pkm, species, level, id, PKM_IVS_RANDOM, PKM_PID_RANDOM);
}

void PokeParty_CreatePkm(PartyPkm *pkm, u16 species, u16 level, u64 trainerId, s32 ivs, u64 pid) {
    MailData *mail;

    PokeParty_ClearPkm(pkm);
    PML_CreatePkm(&pkm->base, species, level, trainerId, ivs, pid);
    // The party fields were encrypted with the cleared Pokémon's PID
    PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), 0);
    PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), pkm->base.pid);
    PokeParty_SetParam(pkm, PKM_PARAM_LEVEL, level);
    mail = CreateMailData(HEAPID_TAIL(HEAPID_SYSTEM));
    PokeParty_SetParam(pkm, PKM_PARAM_MAIL, (u32)mail);
    GFL_HeapFree(mail);
    PokeParty_RecalcStats(pkm);
}

void PML_CreateTempPkm(BoxPkm *pkm, u16 species, u16 level, u64 id) {
    PML_CreatePkm(pkm, species, level, id, PKM_IVS_RANDOM, PKM_PID_RANDOM);
}

static void PML_CreatePkm(BoxPkm *pkm, u16 species, u16 level, u64 trainerId, u32 ivs, u64 pid) {
    PersonalData *personal;
    BOOL wasEncrypted;
    u32 ability;

    PML_PkmInit(pkm);
    wasEncrypted = PML_PkmDecrypt(pkm);
    personal = PML_PersonalLoadBW2(species, 0);

    if (pid == PKM_PID_RANDOM) {
        pid = GFL_RandomLC(0);
        GFL_RandomMT();
    } else if (pid == PKM_PID_FROM_ID) {
        pid = trainerId;
    }
    PML_PkmSetParam(pkm, PKM_PARAM_PID, pid);

    if (trainerId == PKM_ID_RANDOM) {
        trainerId = GFL_RandomLC(0);
    } else if (trainerId == PKM_ID_NOT_SHINY) {
        trainerId = (u32)(pid + 1);
        while (PML_UtilPIDIsRare(trainerId, pid)) {
            trainerId = (trainerId + 1) & 0xffffffff;
        }
    }
    PML_PkmSetParam(pkm, PKM_PARAM_ID, trainerId);

    PML_PkmSetParam(pkm, PKM_PARAM_REGION, region);
    PML_PkmSetParam(pkm, PKM_PARAM_SPECIES, species);
    GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, species, g_PMLPkmStringTemp, NELEMS(g_PMLPkmStringTemp));
    PML_PkmSetParam(pkm, PKM_PARAM_NICKNAME_RAW, (u32)g_PMLPkmStringTemp);
    PML_PkmSetParam(pkm, PKM_PARAM_EXP, PML_UtilGetPkmLvExp(species, 0, level));
    PML_PkmSetParam(pkm, PKM_PARAM_HAPPINESS, PML_PersonalGetParam(personal, PERSONAL_FRIENDSHIP));
    PML_PkmSetParam(pkm, PKM_PARAM_MET_LEVEL, level);
    PML_PkmSetParam(pkm, PKM_PARAM_ORIGIN_GAME, game_version);
    PML_PkmSetParam(pkm, PKM_PARAM_POKEBALL, ITEM_POKE_BALL);

    if (ivs == PKM_IVS_RANDOM) {
        PML_PkmSetParam(pkm, PKM_PARAM_IV_HP, GFL_RandomMTRange(IV_RANGE));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_ATTACK, GFL_RandomMTRange(IV_RANGE));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_DEFENSE, GFL_RandomMTRange(IV_RANGE));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_SP_ATTACK, GFL_RandomMTRange(IV_RANGE));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, GFL_RandomMTRange(IV_RANGE));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_SPEED, GFL_RandomMTRange(IV_RANGE));
    } else {
        PML_PkmSetParam(pkm, PKM_PARAM_IV_HP, (u8)(ivs & IV_MASK));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_ATTACK, (u8)((ivs >> (IV_BITS * 1)) & IV_MASK));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_DEFENSE, (u8)((ivs >> (IV_BITS * 2)) & IV_MASK));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_SP_ATTACK, (u8)((ivs >> (IV_BITS * 3)) & IV_MASK));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, (u8)((ivs >> (IV_BITS * 4)) & IV_MASK));
        PML_PkmSetParam(pkm, PKM_PARAM_IV_SPEED, (u8)((ivs >> (IV_BITS * 5)) & IV_MASK));
    }

    ability = PERSONAL_ABILITY_1;
    if (GetPersonalAbilCount(personal) == 2 && (pid & PID_ABILITY_BIT) != 0) {
        ability = PERSONAL_ABILITY_2;
    }
    PML_PkmSetParam(pkm, PKM_PARAM_ABILITY, PML_PersonalGetParam(personal, ability));
    PML_PkmSetParam(pkm, PKM_PARAM_SEX, PML_PkmGetSex(pkm));
    PML_PkmSetParam(pkm, PKM_PARAM_NATURE, GFL_RandomLC(NATURE_COUNT));
    PML_UtilPkmSetDefaultMoves(pkm);
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

void setChangedPkmSpecies(PartyPkm *pkm, u16 species) {
    u8 wasEncrypted = PokeParty_DecryptPkm(pkm);

    setNameChangedPkm(&pkm->base, species);
    PokeParty_RecalcStats(pkm);
    PokeParty_EncryptPkm(pkm, wasEncrypted);
}

// Changes the species, with its ability and sex, and its name unless the Pokémon has a nickname
static void setNameChangedPkm(BoxPkm *pkm, u16 species) {
    u8 wasEncrypted = PML_PkmDecrypt(pkm);
    u16 oldSpecies = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 isNicknamed;

    if (oldSpecies != species) {
        PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
        isNicknamed = PML_PkmGetParam(pkm, PKM_PARAM_IS_NICKNAMED, NULL);
        PML_PkmSetParam(pkm, PKM_PARAM_SPECIES, species);
        setAbilityForForm(pkm, species, oldSpecies);
        UpdateGenderOnMonsChange(pkm, species, oldSpecies);
        if (!isNicknamed) {
            GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, species, g_PMLPkmStringTemp,
                                  NELEMS(g_PMLPkmStringTemp));
            PML_PkmSetParam(pkm, PKM_PARAM_NICKNAME_RAW, (u32)g_PMLPkmStringTemp);
        }
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

BOOL PokeParty_ChangeForme(PartyPkm *pkm, u16 forme) {
    u8 wasEncrypted = PokeParty_DecryptPkm(pkm);
    BOOL changed = PML_PkmChangeForme(&pkm->base, forme);

    if (changed == TRUE) {
        PokeParty_RecalcStats(pkm);
    }
    PokeParty_EncryptPkm(pkm, wasEncrypted);
    return changed;
}

BOOL PML_PkmChangeForme(BoxPkm *pkm, u32 forme) {
    BOOL changed = FALSE;
    u8 wasEncrypted = PML_PkmDecrypt(pkm);
    u16 oldForme = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    u16 species;

    if (oldForme != forme) {
        PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
        species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
        PML_PkmSetParam(pkm, PKM_PARAM_FORM, forme);
        setAbilityForForm(pkm, species, species);
        UpdateGenderOnMonsChange(pkm, species, species);
        changed = TRUE;
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
    return changed;
}

BOOL PML_PkmChangeRotomForme(PartyPkm *pkm, u32 forme, u32 moveSlot) {
    int i = 0;
    u32 j;
    u32 move;
    u32 newMove;

    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) != SPECIES_ROTOM) {
        return FALSE;
    }
    u16 formMoves[ROTOM_FORM_COUNT] = {
        MOVE_NONE, MOVE_OVERHEAT, MOVE_HYDRO_PUMP, MOVE_BLIZZARD, MOVE_AIR_SLASH, MOVE_LEAF_STORM,
    };
    newMove = formMoves[forme];
    // Replace the first form move with the new form's, and forget the others
    for (i = 0; i < 4; i++) {
        move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);
        for (j = 1; j < ROTOM_FORM_COUNT; j++) {
            if (move != MOVE_NONE && move == formMoves[j]) {
                if (newMove != MOVE_NONE) {
                    PokeParty_SetMove(pkm, newMove, i);
                    newMove = MOVE_NONE;
                } else {
                    shuffleMovesUpWriteLastSlot(pkm, i);
                    i--;
                }
                break;
            }
        }
    }
    if (newMove != MOVE_NONE) {
        for (i = 0; i < 4; i++) {
            if (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL) == MOVE_NONE) {
                PokeParty_SetMove(pkm, newMove, i);
                break;
            }
        }
        if (i == 4) {
            PokeParty_SetMove(pkm, newMove, moveSlot);
        }
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1, NULL) == MOVE_NONE) {
        PokeParty_SetMove(pkm, MOVE_THUNDER_SHOCK, 0);
    }
    PokeParty_ChangeForme(pkm, forme);
    return TRUE;
}

void hatchEgg(PartyPkm *pkm, PlayerInfo *playerInfo, u16 placeName, HeapID heapId) {
    u8 wasEncrypted = PokeParty_DecryptPkm(pkm);

    fillDataForHatchedEgg(&pkm->base, playerInfo, placeName, heapId);
    PokeParty_EncryptPkm(pkm, wasEncrypted);
}

static void fillDataForHatchedEgg(BoxPkm *pkm, PlayerInfo *playerInfo, u32 placeName, HeapID heapId) {
    u8 wasEncrypted = PML_PkmDecrypt(pkm);
    u16 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);

    PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    PML_PkmSetParam(pkm, PKM_PARAM_IS_EGG, FALSE);
    PML_PkmSetParam(pkm, PKM_PARAM_HAPPINESS, HATCHED_FRIENDSHIP);
    GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, species, g_PMLPkmStringTemp, NELEMS(g_PMLPkmStringTemp));
    PML_PkmSetParam(pkm, PKM_PARAM_NICKNAME_RAW, (u32)g_PMLPkmStringTemp);
    PML_UtilSetupMetData(pkm, MET_DATA_HATCHED, playerInfo, placeName, heapId);
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

// Forgets the move of a slot: the moves after it move up, and the last slot becomes empty
static void shuffleMovesUpWriteLastSlot(PartyPkm *pkm, u32 slot) {
    u32 i;
    u16 move;
    u8 pp;
    u8 ppUp;

    for (i = slot; i < 3; i++) {
        move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i + 1, NULL);
        pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + i + 1, NULL);
        ppUp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i + 1, NULL);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1 + i, move);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + i, pp);
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, ppUp);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE1 + 3, MOVE_NONE);
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + 3, 0);
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP_UP + 3, 0);
}

// Sets the ability the species and form have in the Pokémon's ability slot
static void setAbilityForForm(BoxPkm *pkm, u16 species, u16 oldSpecies) {
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    PersonalData *personal = PML_PersonalLoadBW2(species, form);
    u32 pid = PML_PkmGetParam(pkm, PKM_PARAM_PID, NULL);
    u32 ability = PERSONAL_ABILITY_1;
    u32 abilityBit = PID_ABILITY_BIT;

    if (PML_PkmGetParam(pkm, PKM_PARAM_ORIGIN_GAME, NULL) < VERSION_WHITE) {
        abilityBit = PID_ABILITY_BIT_OLD;
    }
    if (PML_PkmGetParam(pkm, PKM_PARAM_HIDDEN_ABILITY, NULL)) {
        ability = PERSONAL_ABILITY_HIDDEN;
    } else if (GetPersonalAbilCount(personal) == 2 && (pid & abilityBit) != 0) {
        ability = PERSONAL_ABILITY_2;
    }
    PML_PkmSetParam(pkm, PKM_PARAM_ABILITY, PML_PersonalGetParam(personal, ability));
}

// Derives the sex again, unless both the old and the new species leave no choice of it
static void UpdateGenderOnMonsChange(BoxPkm *pkm, u16 species, u16 oldSpecies) {
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    u32 sexRatio = PML_PersonalGetParam(PML_PersonalLoadBW2(species, form), PERSONAL_SEX_RATIO);
    u32 oldSexRatio = PML_PersonalGetParam(PML_PersonalLoadBW2(oldSpecies, form), PERSONAL_SEX_RATIO);

    if (isGenderlessOrSetGender(sexRatio) != TRUE || oldSexRatio == 0xff) {
        PML_PkmSetParam(pkm, PKM_PARAM_SEX,
                        PML_UtilDerivePkmSex(species, form, PML_PkmGetParam(pkm, PKM_PARAM_PID, NULL)));
    }
}

BOOL PokeParty_DecryptPkm(PartyPkm *pkm) {
    BOOL decrypted = FALSE;

    if (!pkm->base.partyDecrypted) {
        decrypted = TRUE;
        pkm->base.partyDecrypted = TRUE;
        pkm->base.boxDecrypted = TRUE;
        PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), pkm->base.pid);
        PML_CryptoRun(&pkm->base.contentBuffer, sizeof(PkmBuffer), pkm->base.checksum);
    }
    return decrypted;
}

BOOL PokeParty_EncryptPkm(PartyPkm *pkm, BOOL wasEncrypted) {
    BOOL encrypted = FALSE;

    if (pkm->base.partyDecrypted == TRUE && wasEncrypted == TRUE) {
        encrypted = TRUE;
        pkm->base.partyDecrypted = FALSE;
        pkm->base.boxDecrypted = FALSE;
        PML_CryptoRun(&pkm->statusCond, sizeof(PartyPkm) - sizeof(BoxPkm), pkm->base.pid);
        pkm->base.checksum = PML_CryptoGenKey(&pkm->base.contentBuffer, sizeof(PkmBuffer));
        PML_CryptoRun(&pkm->base.contentBuffer, sizeof(PkmBuffer), pkm->base.checksum);
    }
    return encrypted;
}

BOOL PML_PkmDecrypt(BoxPkm *pkm) {
    BOOL decrypted = FALSE;

    if (!pkm->boxDecrypted) {
        decrypted = TRUE;
        pkm->boxDecrypted = TRUE;
        PML_CryptoRun(&pkm->contentBuffer, sizeof(PkmBuffer), pkm->checksum);
    }
    return decrypted;
}

BOOL PML_PkmReEncrypt(BoxPkm *pkm, BOOL wasEncrypted) {
    BOOL encrypted = FALSE;

    if (pkm->boxDecrypted == TRUE && wasEncrypted == TRUE) {
        encrypted = TRUE;
        pkm->boxDecrypted = FALSE;
        pkm->checksum = PML_CryptoGenKey(&pkm->contentBuffer, sizeof(PkmBuffer));
        PML_CryptoRun(&pkm->contentBuffer, sizeof(PkmBuffer), pkm->checksum);
    }
    return encrypted;
}

u32 PokeParty_GetParam(const PartyPkm *pkm, PkmField param, void *buffer) {
    u32 value;

    PokeParty_PkmDecryptCheck((PartyPkm *)pkm);
    value = PokeParty_GetParamCore((PartyPkm *)pkm, param, buffer);
    PokeParty_PkmEncrypt((PartyPkm *)pkm);
    return value;
}

void PokeParty_SetParam(PartyPkm *pkm, PkmField param, u32 value) {
    if (PokeParty_PkmDecryptCheck(pkm) == TRUE) {
        PokeParty_SetParamCore(pkm, param, value);
    }
    PokeParty_PkmEncrypt(pkm);
}

void PML_PkmSetParam(BoxPkm *pkm, u32 param, u32 value) {
    if (PML_PkmDecryptCheck(pkm) == TRUE) {
        PML_PkmSetParamCore(pkm, param, value);
    }
    PML_PkmEncryptForce(pkm);
}

void setPkmBattleData(PartyPkm *pkm, u32 param, u32 value) {
    if (PokeParty_PkmDecryptCheck(pkm) == TRUE) {
        writePkmBattleData(pkm, param, value);
    }
    PokeParty_PkmEncrypt(pkm);
}

u32 PML_PkmGetParam(BoxPkm *pkm, u32 param, void *buffer) {
    u32 value;

    PML_PkmDecryptCheck(pkm);
    value = PML_PkmGetParamCore(pkm, param, buffer);
    PML_PkmEncryptForce(pkm);
    return value;
}

BOOL PokeParty_IsRare(PartyPkm *pkm) {
    return PML_PkmIsRare(&pkm->base);
}

BOOL PML_PkmIsRare(BoxPkm *pkm) {
    u32 id = PML_PkmGetParam(pkm, PKM_PARAM_ID, NULL);
    u32 pid = PML_PkmGetParam(pkm, PKM_PARAM_PID, NULL);

    return PML_UtilPIDIsRare(id, pid);
}

BOOL PML_UtilPIDIsRare(u32 id, u32 pid) {
    return (((id & 0xffff0000) >> 16) ^ (id & 0xffff) ^ ((pid & 0xffff0000) >> 16) ^ (pid & 0xffff)) < 8;
}

BOOL isKyuremTransformed(u16 species, u8 form) {
    if (species == SPECIES_KYUREM && form != 0) {
        return TRUE;
    }
    return FALSE;
}

// Whether the species and form are Tornadus, Thundurus or Landorus in its Therian form
static BOOL isKamiTrioTransformed(u16 species, u8 form) {
    if ((species == SPECIES_TORNADUS || species == SPECIES_THUNDURUS || species == SPECIES_LANDORUS) && form != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL isKeldeoTransformed(u16 species, u8 form) {
    if (species == SPECIES_KELDEO && form != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL isNewFormTransformed(u16 species, u8 form) {
    if (isKyuremTransformed(species, form)) {
        return TRUE;
    }
    if (isKamiTrioTransformed(species, form)) {
        return TRUE;
    }
    if (isKeldeoTransformed(species, form)) {
        return TRUE;
    }
    return FALSE;
}

BOOL hasPokemonChangedForm(BoxPkm *pkm) {
    u32 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);

    return isNewFormTransformed(species, form);
}

u8 PokeParty_GetSex(PartyPkm *pkm) {
    return PML_PkmGetSex(&pkm->base);
}

u8 PML_PkmGetSex(BoxPkm *pkm) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    u16 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    u32 pid = PML_PkmGetParam(pkm, PKM_PARAM_PID, NULL);

    PML_PkmReEncrypt(pkm, wasEncrypted);
    return PML_UtilDerivePkmSex(species, form, pid);
}

u8 PML_UtilDerivePkmSex(u16 species, u16 form, u32 pid) {
    u8 ratio = PML_PersonalGetParam(PML_PersonalLoadBW2(species, form), PERSONAL_SEX_RATIO);

    switch (ratio) {
    case GENDER_RATIO_MALE_ONLY:
        return GENDER_MALE;
    case GENDER_RATIO_FEMALE_ONLY:
        return GENDER_FEMALE;
    case GENDER_RATIO_GENDERLESS:
        return GENDER_UNKNOWN;
    }
    return (u8)pid < ratio ? GENDER_FEMALE : GENDER_MALE;
}

u32 GetStatusCond(PartyPkm *pkm) {
    return PokeParty_GetParam(pkm, PKM_PARAM_STATUS, NULL);
}

u8 getParentAbilSlotNumber(PartyPkm *pkm) {
    return getPkmAbilSlotNumber(&pkm->base);
}

// Which of its species' abilities the Pokémon has. Pokémon of the earlier generations have theirs by the lowest bit
// of the PID
static u8 getPkmAbilSlotNumber(BoxPkm *pkm) {
    int slot = ABILITY_SLOT_1;
    u32 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    void *personal = PML_PersonalLoadBW2(species, form);
    u32 pid = PML_PkmGetParam(pkm, PKM_PARAM_PID, NULL);
    u32 mask = 0x10000;

    if (PML_PkmGetParam(pkm, PKM_PARAM_ORIGIN_GAME, NULL) < VERSION_WHITE) {
        mask = 1;
    }
    if (PML_PkmGetParam(pkm, PKM_PARAM_HIDDEN_ABILITY, NULL)) {
        slot = ABILITY_SLOT_HIDDEN;
    } else if (GetPersonalAbilCount(personal) == 2) {
        if (pid & mask) {
            slot = ABILITY_SLOT_2;
        }
    }
    return slot;
}

void PokeParty_SetStatusCond(PartyPkm *pkm, u32 status) {
    PokeParty_SetParam(pkm, PKM_PARAM_STATUS, status);
}

void PokeParty_SetDefaultMoves(PartyPkm *pkm) {
    PML_UtilPkmSetDefaultMoves(&pkm->base);
}

// Gives the Pokémon the last four moves its species learns up to its level
static void PML_UtilPkmSetDefaultMoves(BoxPkm *pkm) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    int i = 0;
    int n;
    u16 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    u8 level = PML_PkmGetLevel(pkm);
    u16 move;

    PML_LearnsetLvUpLoad(species, form, sDefaultMovesLearnset);
    for (; i < MOVE_SLOTS; i++) {
        PML_PkmSetParam(pkm, PKM_PARAM_MOVE1 + i, 0);
        PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP + i, 0);
        PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, 0);
    }
    for (n = 0; !LearnsetEntry_IsEnd(sDefaultMovesLearnset[n]); n++) {
        if (LearnsetEntry_GetLevel(sDefaultMovesLearnset[n]) > level) {
            break;
        }
        move = LearnsetEntry_GetMove(sDefaultMovesLearnset[n]);
        if (PML_PkmLearnMove(pkm, move) == 0xffff) {
            PML_PkmSetLastMove(pkm, move);
        }
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

u16 PokeParty_LearnMove(PartyPkm *pkm, u16 move) {
    return PML_PkmLearnMove(&pkm->base, move);
}

// Teaches a move in the first free slot. Returns the move, LEARN_MOVE_KNOWN if the Pokémon already knows it, and
// 0xffff if it has no free slot
static u16 PML_PkmLearnMove(BoxPkm *pkm, u16 move) {
    u16 result = 0xffff;
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    int i;

    for (i = 0; i < MOVE_SLOTS; i++) {
        u16 known = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

        if (known == 0) {
            PML_PkmSetMove(pkm, move, i);
            result = move;
            break;
        }
        if (known == move) {
            result = LEARN_MOVE_KNOWN;
            break;
        }
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
    return result;
}

void PokeParty_SetLastMove(PartyPkm *pkm, u16 move) {
    PML_PkmSetLastMove(&pkm->base, move);
}

// Drops the first move, moves the others up and puts the move in the last slot
static void PML_PkmSetLastMove(BoxPkm *pkm, u16 move) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    int i;
    u16 moves[MOVE_SLOTS];
    u8 pp[MOVE_SLOTS];
    u8 ppUps[MOVE_SLOTS];

    for (i = 0; i < MOVE_SLOTS - 1; i++) {
        moves[i] = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + 1 + i, NULL);
        pp[i] = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP + 1 + i, NULL);
        ppUps[i] = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP_UP + 1 + i, NULL);
    }
    moves[MOVE_SLOTS - 1] = move;
    pp[MOVE_SLOTS - 1] = PML_MoveGetParam(move, MOVE_PARAM_PP);
    ppUps[MOVE_SLOTS - 1] = 0;
    for (i = 0; i < MOVE_SLOTS; i++) {
        PML_PkmSetParam(pkm, PKM_PARAM_MOVE1 + i, moves[i]);
        PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP + i, pp[i]);
        PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, ppUps[i]);
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

void PokeParty_PPRecover(PartyPkm *pkm) {
    PML_PkmPPRecover(&pkm->base);
}

// Restores the PP of the Pokémon's moves
void PML_PkmPPRecover(BoxPkm *pkm) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    u32 i;

    for (i = 0; i < MOVE_SLOTS; i++) {
        u32 move = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

        if (move != 0) {
            u32 ppUps = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, NULL);

            PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP + i, PML_MoveGetMaxPP(move, ppUps));
        }
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

void PokeParty_SetMove(PartyPkm *pkm, u16 move, u8 slot) {
    PML_PkmSetMove(&pkm->base, move, slot);
}

void PML_PkmSetMove(BoxPkm *pkm, u16 move, u8 slot) {
    u32 pp = 0;

    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1 + slot, move);
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slot, 0);
    if (move != 0) {
        pp = PML_MoveGetMaxPP(move, 0);
    }
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP + slot, pp);
}

u16 PokeParty_LearnMoveTutor(PartyPkm *pkm, u16 move) {
    return PML_PkmLearnMoveTutor(&pkm->base, move);
}

// PML_PkmLearnMove for the move tutors
static u16 PML_PkmLearnMoveTutor(BoxPkm *pkm, u16 move) {
    u16 result = 0xffff;
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    int i;

    for (i = 0; i < MOVE_SLOTS; i++) {
        u16 known = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

        if (known == 0) {
            PML_PkmSetMoveKeepPP(pkm, move, i);
            result = move;
            break;
        }
        if (known == move) {
            result = LEARN_MOVE_KNOWN;
            break;
        }
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
    return result;
}

void PokeParty_SetMoveKeepPP(PartyPkm *pkm, u16 move, u8 slot) {
    PML_PkmSetMoveKeepPP(&pkm->base, move, slot);
}

// Puts a move in a slot without PP Ups. Over another move, the new move has at most the PP the old one had left
static void PML_PkmSetMoveKeepPP(BoxPkm *pkm, u16 move, u8 slot) {
    u8 pp = 0;
    BOOL hadMove;
    u8 oldPP;

    if (PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL) != 0) {
        hadMove = TRUE;
    } else {
        hadMove = FALSE;
    }
    oldPP = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP + slot, NULL);
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1 + slot, move);
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slot, 0);
    if (move != 0) {
        pp = PML_MoveGetMaxPP(move, 0);
        if (hadMove) {
            pp = pp > oldPP ? oldPP : pp;
        }
    }
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP + slot, pp);
}

u16 PokeParty_LearnNextLevelMove(PartyPkm *pkm, u32 *index, HeapID heapId) {
    u16 result = 0;
    LearnsetEntry *learnset =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), LEARNSET_MAX * sizeof(LearnsetEntry), FALSE, "poke_tool.c", 1737);
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    u8 level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);

    PML_LearnsetLvUpLoad(species, form, learnset);
    while (!LearnsetEntry_IsEnd(learnset[*index])) {
        if (LearnsetEntry_GetLevel(learnset[*index]) == level) {
            result = PokeParty_LearnMove(pkm, LearnsetEntry_GetMove(learnset[*index]));
            if (result == 0xffff) {
                result = LearnsetEntry_GetMove(learnset[*index]) | LEARN_MOVE_NO_SLOT;
                (*index)++;
                break;
            }
            if (result != LEARN_MOVE_KNOWN) {
                (*index)++;
                break;
            }
        }
        (*index)++;
    }
    GFL_HeapFree(learnset);
    return result;
}

u32 PokeParty_GetLevel(PartyPkm *pkm) {
    return PML_PkmGetLevel(&pkm->base);
}

u32 PML_PkmGetLevel(BoxPkm *pkm) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    u32 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    u32 exp = PML_PkmGetParam(pkm, PKM_PARAM_EXP, NULL);

    PML_PkmReEncrypt(pkm, wasEncrypted);
    return CalcLevelByExp(species, form, exp);
}

BOOL IsTrainerOT(PartyPkm *pkm, PlayerInfo *player) {
    return PML_UtilCheckForeignOT(&pkm->base, player);
}

// Whether the Pokémon's original trainer has the player's ID, gender and name
BOOL PML_UtilCheckForeignOT(BoxPkm *pkm, PlayerInfo *playerInfo) {
    u8 wasEncrypted = PML_PkmDecrypt(pkm);
    BOOL result = FALSE;
    u32 playerId = getIDAsUInt(playerInfo);
    u32 otId = PML_PkmGetParam(pkm, PKM_PARAM_ID, NULL);
    u32 playerGender = getTrainerGender(playerInfo);
    u32 otGender = PML_PkmGetParam(pkm, PKM_PARAM_OT_GENDER, NULL);

    copyName(playerInfo->name, g_PMLPkmStringTemp, NELEMS(g_PMLPkmStringTemp));
    PML_PkmGetParam(pkm, PKM_PARAM_OT_NAME_RAW, g_OTNameCompTemp);
    if (playerId == otId && playerGender == otGender && wcharscmp(g_PMLPkmStringTemp, g_OTNameCompTemp)) {
        result = TRUE;
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
    return result;
}

// The level a Pokémon of the species has with the experience
u32 CalcLevelByExp(u16 species, u16 form, u32 exp) {
    u16 growthRate = PML_PersonalGetParam(PML_PersonalLoadBW2(species, form), PERSONAL_GROWTH_RATE);
    u16 level;

    PML_GrowTblRead(growthRate, g_PMLGrowTblCache);
    for (level = 1; level < MAX_LEVEL + 1; level++) {
        if (g_PMLGrowTblCache[level] > exp) {
            break;
        }
    }
    return level - 1;
}

u32 getExpForPkm_Wrapper(PartyPkm *pkm) {
    return getExpForPkm(&pkm->base);
}

static u32 getExpForPkm(BoxPkm *pkm) {
    u32 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    u32 level = PML_PkmGetParam(pkm, PKM_PARAM_LEVEL, NULL);

    return PML_UtilGetPkmLvExp(species, form, level);
}

u32 PML_UtilGetPkmLvExp(u16 species, u16 form, u16 level) {
    return PML_GrowTblGetLvExp(PML_PersonalGetParamSingle(species, form, PERSONAL_GROWTH_RATE), level);
}

u32 PokeParty_GetNature(PartyPkm *pkm) {
    return PML_PkmGetNature(&pkm->base);
}

u32 PML_PkmGetNature(BoxPkm *pkm) {
    u8 nature = PML_PkmGetParam(pkm, PKM_PARAM_NATURE, NULL);

    return nature;
}

void PokeParty_SetNature(PartyPkm *pkm, u32 nature) {
    PokeParty_SetParam(pkm, PKM_PARAM_NATURE, nature);
    PokeParty_RecalcStats(pkm);
}

void PokeParty_RecalcStats(PartyPkm *pkm) {
    BOOL wasEncrypted = PokeParty_DecryptPkm(pkm);

    PokeParty_SetParam(pkm, PKM_PARAM_LEVEL, PokeParty_GetLevel(pkm));
    PokeParty_RecalcStatsCore(pkm);
    PokeParty_EncryptPkm(pkm, wasEncrypted);
}

BoxPkm *func_0201d620(PartyPkm *pkm) {
    return &pkm->base;
}

BoxPkm *func_0201d624(PartyPkm *pkm) {
    return &pkm->base;
}

// The lowest byte of a PID that makes a Pokémon of the species male, or the highest that makes it female
static u32 getGenderValue(u16 species, u16 form, u8 sex) {
    u32 value = 8;
    u8 ratio = PML_PersonalGetParam(PML_PersonalLoadBW2(species, form), PERSONAL_SEX_RATIO);

    if (isGenderlessOrSetGender(ratio) != TRUE) {
        if (sex == GENDER_MALE) {
            value = ratio;
        } else {
            value = ratio - 1;
            if (ratio == 0) {
                value = 0;
            }
        }
    }
    return value;
}

// A PID that is shiny or not for the trainer ID, with the sex and the ability bit asked for
u32 makeSpecialPID(u32 id, u16 species, u16 form, u8 sex, u8 ability, BOOL rare) {
    u8 ratio;
    u32 pid;

    if (rare) {
        u32 value = getGenderValue(species, form, sex);
        u32 idXor = ((id & 0xffff0000) >> 16) ^ (id & 0xffff);

        pid = value | ((idXor ^ (value & 0xffff)) << 16);
        if ((pid & 0x10000) != (ability << 16)) {
            pid ^= 0x10000;
        }
    } else {
        ratio = PML_PersonalGetParam(PML_PersonalLoadBW2(species, form), PERSONAL_SEX_RATIO);
        pid = (((((id & 0xffff0000) >> 16) ^ (id & 0xffff)) & 0xff00) ^ 0xff00) << 16;
        if ((pid & 0x10000) != (ability << 16)) {
            pid ^= 0x10000;
        }
        if (isGenderlessOrSetGender(ratio) != TRUE) {
            pid |= ratio;
            if (sex == GENDER_FEMALE) {
                pid--;
            }
        }
    }
    return pid;
}

u32 PML_GenPID(u32 id, u16 species, u16 form, u32 sex, u32 ability, u32 rare) {
    u32 pid;

    if (sex == GENDER_UNKNOWN) {
        pid = GFL_RandomLC(0);
    } else {
        u32 high = GFL_RandomLC(0) & ~0xff;
        u32 value = getGenderValue(species, form, sex);

        if (sex == GENDER_MALE && !isGenderlessOrSetGender(value)) {
            value += GFL_RandomLC(254 - value);
        } else if (sex == GENDER_FEMALE && !isGenderlessOrSetGender(value)) {
            value = GFL_RandomLC(value) + 1;
        }
        pid = value | high;
    }
    switch (rare) {
    case PID_RARE_ALWAYS:
        pid = (pid & 0xff) | ((((id & 0xffff0000) >> 16) ^ (id & 0xffff) ^ (pid & 0xff)) << 16);
        if ((pid & 0x10000) != (ability << 16)) {
            pid ^= 0x10000;
        }
        break;
    case PID_RARE_NEVER:
        if (PML_UtilPIDIsRare(id, pid) == TRUE) {
            pid ^= 0x10000000;
        }
        // fallthrough
    case PID_RARE_RANDOM:
        if ((pid & 0x10000) != (ability << 16)) {
            pid ^= 0x10000;
        }
        break;
    }
    return pid;
}

u8 getHiddenPowerType(PartyPkm *pkm) {
    u32 value = 0;

    if (PokeParty_GetParam(pkm, PKM_PARAM_IV_HP, NULL) & 1) {
        value += 1;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IV_ATTACK, NULL) & 1) {
        value += 2;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IV_DEFENSE, NULL) & 1) {
        value += 4;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IV_SPEED, NULL) & 1) {
        value += 8;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_ATTACK, NULL) & 1) {
        value += 16;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, NULL) & 1) {
        value += 32;
    }
    value = value * 15 / 63;
    while (value > NELEMS(hpTypes)) {
        value -= NELEMS(hpTypes);
    }
    return hpTypes[value];
}

u32 getHiddenPowerBasePwr(PartyPkm *pkm) {
    u32 value = 0;

    if ((PokeParty_GetParam(pkm, PKM_PARAM_IV_HP, NULL) & 3) > 1) {
        value += 1;
    }
    if ((PokeParty_GetParam(pkm, PKM_PARAM_IV_ATTACK, NULL) & 3) > 1) {
        value += 2;
    }
    if ((PokeParty_GetParam(pkm, PKM_PARAM_IV_DEFENSE, NULL) & 3) > 1) {
        value += 4;
    }
    if ((PokeParty_GetParam(pkm, PKM_PARAM_IV_SPEED, NULL) & 3) > 1) {
        value += 8;
    }
    if ((PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_ATTACK, NULL) & 3) > 1) {
        value += 16;
    }
    if ((PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, NULL) & 3) > 1) {
        value += 32;
    }
    return value * 40 / 63 + 30;
}

BOOL canPkmLearnTM_Wrapper(PartyPkm *pkm, u8 tm) {
    return canPkmLearnTM(&pkm->base, tm);
}

static BOOL canPkmLearnTM(BoxPkm *pkm, u8 tm) {
    u16 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);

    return isTMValidForPkm(species, form, tm);
}

static BOOL isTMValidForPkm(u16 species, u16 form, s32 tm) {
    if (species == SPECIES_EGG) {
        return FALSE;
    }
    return (PML_PersonalGetParamSingle(species, form, PERSONAL_MACHINES_1 + tm / 32) & (1 << (tm % 32))) != 0;
}

// Whether the Pokémon can learn the move at moveIndex of the list of the tutors that teach for free
BOOL PokeParty_CheckMoveTutorNormal(PartyPkm *pkm, u32 moveIndex) {
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);

    return PML_UtilCheckMoveTutorNormal(species, form, moveIndex);
}

static BOOL PML_UtilCheckMoveTutorNormal(u16 species, u16 form, s32 moveIndex) {
    if (species == SPECIES_EGG) {
        return FALSE;
    }
    return (PML_PersonalGetParamSingle(species, form, PERSONAL_TUTORS) & (1 << (moveIndex % 32))) != 0;
}

BOOL PokeParty_CheckMoveTutorPaid(PartyPkm *pkm, u32 moveIndex, u32 tutor) {
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);

    return PML_UtilCheckMoveTutorPaid(species, form, moveIndex, tutor);
}

// The personal data has a word of flags for each of the four paid tutors, after the free tutors' word
static BOOL PML_UtilCheckMoveTutorPaid(u16 species, u16 form, int moveIndex, u32 tutor) {
    BOOL result;

    if (species == SPECIES_EGG) {
        result = FALSE;
    } else {
        u32 flags = PML_PersonalGetParamSingle(species, form, PERSONAL_TUTOR_PAID_1 + tutor);

        result = (flags & (1 << (moveIndex % 32))) != 0;
    }
    return result;
}

BOOL PokeParty_CheckAnyRibbon(PartyPkm *pkm) {
    BOOL result;
    PkmBlockA *blockA;
    PkmBlockB *blockB;
    PkmBlockC *blockC;

    PokeParty_PkmDecryptCheck(pkm);
    result = FALSE;
    blockA = PML_PkmGetParamBlockCore(&pkm->base, pkm->base.pid, 0);
    blockB = PML_PkmGetParamBlockCore(&pkm->base, pkm->base.pid, 1);
    blockC = PML_PkmGetParamBlockCore(&pkm->base, pkm->base.pid, 2);
    if (blockA->ribbons != 0 || blockB->ribbons != 0 || blockC->ribbons != 0) {
        result = TRUE;
    }
    PokeParty_PkmEncrypt(pkm);
    return result;
}

s8 doesNatureAffectStat(PartyPkm *pkm, u32 stat) {
    return isStatAdjustedByPokeNature(&pkm->base, stat);
}

static s8 isStatAdjustedByPokeNature(BoxPkm *pkm, u32 flavor) {
    u8 nature = PML_PkmGetParam(pkm, PKM_PARAM_NATURE, NULL);

    return isStatAdjustedForNature(nature, flavor);
}

static s8 isStatAdjustedForNature(u8 nature, u32 flavor) {
    return stat_nature_adjust[nature][flavor];
}

void PokeParty_SetHiddenAbil(PartyPkm *pkm, u32 species, u32 form) {
    PML_PkmSetHiddenAbil(&pkm->base, species, form);
}

// Gives the Pokémon its species' hidden ability, or, when the species has none, the ability its PID picks
static void PML_PkmSetHiddenAbil(BoxPkm *pkm, u32 species, u32 form) {
    u32 ability = PML_PersonalGetParamSingle(species, form, PERSONAL_ABILITY_HIDDEN);

    if (ability != 0) {
        PML_PkmSetParam(pkm, PKM_PARAM_ABILITY, ability);
        PML_PkmSetParam(pkm, PKM_PARAM_HIDDEN_ABILITY, TRUE);
    } else {
        PersonalData *personal = PML_PersonalLoadBW2(species, form);
        u32 pid = PML_PkmGetParam(pkm, PKM_PARAM_PID, NULL);
        u32 param = PERSONAL_ABILITY_1;

        if (GetPersonalAbilCount(personal) == 2 && (pid & 0x10000)) {
            param = PERSONAL_ABILITY_2;
        }
        PML_PkmSetParam(pkm, PKM_PARAM_ABILITY, PML_PersonalGetParam(personal, param));
    }
}

// Calculates the stats from the species, level, IVs, EVs and nature. The HP keep their distance from the maximum
static void PokeParty_RecalcStatsCore(PartyPkm *pkm) {
    int hp;
    int level;
    int oldMaxHP;
    int hpIV;
    int hpEV;
    int atkIV;
    int defIV;
    int speIV;
    int spaIV;
    int spdIV;
    int atkEV;
    int defEV;
    int speEV;
    int spaEV;
    int spdEV;
    int form;
    int species;
    PersonalData *personal;
    int maxHP;
    int stat;

    level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    oldMaxHP = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
    hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
    hpIV = PokeParty_GetParam(pkm, PKM_PARAM_IV_HP, NULL);
    hpEV = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP, NULL);
    atkIV = PokeParty_GetParam(pkm, PKM_PARAM_IV_ATTACK, NULL);
    atkEV = PokeParty_GetParam(pkm, PKM_PARAM_EV_ATTACK, NULL);
    defIV = PokeParty_GetParam(pkm, PKM_PARAM_IV_DEFENSE, NULL);
    defEV = PokeParty_GetParam(pkm, PKM_PARAM_EV_DEFENSE, NULL);
    speIV = PokeParty_GetParam(pkm, PKM_PARAM_IV_SPEED, NULL);
    speEV = PokeParty_GetParam(pkm, PKM_PARAM_EV_SPEED, NULL);
    spaIV = PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_ATTACK, NULL);
    spaEV = PokeParty_GetParam(pkm, PKM_PARAM_EV_SP_ATTACK, NULL);
    spdIV = PokeParty_GetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, NULL);
    spdEV = PokeParty_GetParam(pkm, PKM_PARAM_EV_SP_DEFENSE, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    personal = PML_PersonalLoadBW2(species, form);
    if (species == SPECIES_SHEDINJA) {
        maxHP = 1;
    } else {
        maxHP = (2 * personal->hp + hpIV + hpEV / 4) * level / 100 + level + 10;
    }
    PokeParty_SetParam(pkm, PKM_PARAM_MAX_HP, maxHP);

    stat = (2 * personal->attack + atkIV + atkEV / 4) * level / 100 + 5;
    stat = adjustStatForNature(PokeParty_GetNature(pkm), stat, 1);
    PokeParty_SetParam(pkm, PKM_PARAM_ATTACK, stat);

    stat = (2 * personal->defense + defIV + defEV / 4) * level / 100 + 5;
    stat = adjustStatForNature(PokeParty_GetNature(pkm), stat, 2);
    PokeParty_SetParam(pkm, PKM_PARAM_DEFENSE, stat);

    stat = (2 * personal->speed + speIV + speEV / 4) * level / 100 + 5;
    stat = adjustStatForNature(PokeParty_GetNature(pkm), stat, 5);
    PokeParty_SetParam(pkm, PKM_PARAM_SPEED, stat);

    stat = (2 * personal->spAttack + spaIV + spaEV / 4) * level / 100 + 5;
    stat = adjustStatForNature(PokeParty_GetNature(pkm), stat, 3);
    PokeParty_SetParam(pkm, PKM_PARAM_SP_ATTACK, stat);

    stat = (2 * personal->spDefense + spdIV + spdEV / 4) * level / 100 + 5;
    stat = adjustStatForNature(PokeParty_GetNature(pkm), stat, 4);
    PokeParty_SetParam(pkm, PKM_PARAM_SP_DEFENSE, stat);

    if (hp != 0 || oldMaxHP == 0) {
        if (species == SPECIES_SHEDINJA) {
            hp = 1;
        } else if (hp == 0) {
            hp = maxHP;
        } else {
            int change = maxHP - oldMaxHP;

            if (change < 0) {
                if (hp > maxHP) {
                    hp = maxHP;
                }
            } else {
                hp += change;
            }
        }
    }
    if (hp != 0) {
        PokeParty_SetParam(pkm, PKM_PARAM_HP, hp);
    }
}

// Decrypts a Pokémon of the party that isn't kept decrypted, and returns whether its checksum is right
static BOOL PokeParty_PkmDecryptCheck(PartyPkm *pkm) {
    BOOL result = TRUE;

    if (!pkm->base.partyDecrypted) {
        PML_CryptoRun(&pkm->statusCond, PKM_PARTY_DATA_SIZE, pkm->base.pid);
        result = PML_PkmDecryptCheck(&pkm->base);
    }
    return result;
}

// A Pokémon whose checksum is wrong becomes a bad egg
static BOOL PML_PkmDecryptCheck(BoxPkm *pkm) {
    BOOL result = TRUE;

    if (!pkm->boxDecrypted) {
        PML_CryptoRun(&pkm->contentBuffer, PKM_BOX_DATA_SIZE, pkm->checksum);
        u16 checksum = PML_CryptoGenKey(&pkm->contentBuffer, PKM_BOX_DATA_SIZE);

        if (checksum != pkm->checksum) {
            result = FALSE;
            pkm->badEgg = TRUE;
        }
    }
    return result;
}

static void PokeParty_PkmEncrypt(PartyPkm *pkm) {
    if (!pkm->base.partyDecrypted) {
        PML_CryptoRun(&pkm->statusCond, PKM_PARTY_DATA_SIZE, pkm->base.pid);
        PML_PkmEncryptForce(&pkm->base);
    }
}

static void PML_PkmEncryptForce(BoxPkm *pkm) {
    if (!pkm->boxDecrypted) {
        pkm->checksum = PML_CryptoGenKey(&pkm->contentBuffer, PKM_BOX_DATA_SIZE);
        PML_CryptoRun(&pkm->contentBuffer, PKM_BOX_DATA_SIZE, pkm->checksum);
    }
}

// The fields that only a Pokémon of the party has. The rest are the box Pokémon's
static u32 PokeParty_GetParamCore(PartyPkm *pkm, PkmField field, void *buffer) {
    switch (field) {
    case PKM_PARAM_STATUS:
        return pkm->statusCond;
    case PKM_PARAM_LEVEL:
        return pkm->level;
    case PKM_PARAM_UNK9F:
        return pkm->unk8D;
    case PKM_PARAM_HP:
        return pkm->nowHP;
    case PKM_PARAM_MAX_HP:
        return pkm->maxHP;
    case PKM_PARAM_ATTACK:
        return pkm->atk;
    case PKM_PARAM_DEFENSE:
        return pkm->def;
    case PKM_PARAM_SPEED:
        return pkm->spe;
    case PKM_PARAM_SP_ATTACK:
        return pkm->spa;
    case PKM_PARAM_SP_DEFENSE:
        return pkm->spd;
    case PKM_PARAM_MAIL:
        copyMailMsg(&pkm->mail, buffer);
        return TRUE;
    case PKM_PARAM_UNKA8:
        return TRUE;
    default:
        return PML_PkmGetParamCore(&pkm->base, field, buffer);
    }
}

// Reads a field of a decrypted Pokémon
static u32 PML_PkmGetParamCore(BoxPkm *pkm, u32 field, void *buffer) {
    u32 result = 0;
    u64 bit = 1;
    PkmBlockA *blockA = PML_PkmGetParamBlockCore(pkm, pkm->pid, 0);
    PkmBlockB *blockB = PML_PkmGetParamBlockCore(pkm, pkm->pid, 1);
    PkmBlockC *blockC = PML_PkmGetParamBlockCore(pkm, pkm->pid, 2);
    PkmBlockD *blockD = PML_PkmGetParamBlockCore(pkm, pkm->pid, 3);

    switch (field) {
    default:
        result = 0;
        break;
    case PKM_PARAM_PID:
        result = pkm->pid;
        break;
    case PKM_PARAM_PARTY_DECRYPTED:
        result = pkm->partyDecrypted;
        break;
    case PKM_PARAM_BOX_DECRYPTED:
        result = pkm->boxDecrypted;
        break;
    case PKM_PARAM_BAD_EGG:
        result = pkm->badEgg;
        break;
    case PKM_PARAM_CHECKSUM:
        result = pkm->checksum;
        break;
    case PKM_PARAM_SPECIES_VALID:
        result = blockA->species != SPECIES_NONE;
        break;
    case PKM_PARAM_EGG_OR_BAD_EGG:
        result = pkm->badEgg;
        if (result == FALSE) {
            result = blockB->isEgg;
        }
        break;
    case PKM_PARAM_LEGAL_SPECIES:
        result = blockA->species;
        if (result == SPECIES_NONE) {
            break;
        }
        if (blockB->isEgg || pkm->badEgg) {
            result = SPECIES_EGG;
        }
        break;
    case PKM_PARAM_LEVEL:
        result = CalcLevelByExp(blockA->species, blockB->form, blockA->exp);
        break;
    case PKM_PARAM_SPECIES:
        if (pkm->badEgg) {
            result = SPECIES_EGG;
        } else {
            result = blockA->species;
        }
        break;
    case PKM_PARAM_ITEM:
        result = blockA->item;
        if (result > ITEM_LAST) {
            result = ITEM_NONE;
        }
        break;
    case PKM_PARAM_ID:
        result = blockA->id;
        break;
    case PKM_PARAM_EXP:
        result = blockA->exp;
        break;
    case PKM_PARAM_HAPPINESS:
        result = blockA->friendship;
        break;
    case PKM_PARAM_ABILITY:
        result = blockA->ability;
        break;
    case PKM_PARAM_MARKINGS:
        result = blockA->markings;
        break;
    case PKM_PARAM_REGION:
        result = blockA->language;
        break;
    case PKM_PARAM_EV_HP:
        result = blockA->hpEV;
        break;
    case PKM_PARAM_EV_ATTACK:
        result = blockA->atkEV;
        break;
    case PKM_PARAM_EV_DEFENSE:
        result = blockA->defEV;
        break;
    case PKM_PARAM_EV_SPEED:
        result = blockA->speEV;
        break;
    case PKM_PARAM_EV_SP_ATTACK:
        result = blockA->spaEV;
        break;
    case PKM_PARAM_EV_SP_DEFENSE:
        result = blockA->spdEV;
        break;
    case PKM_PARAM_CONTEST_COOL:
        result = blockA->contest[0];
        break;
    case PKM_PARAM_CONTEST_BEAUTY:
        result = blockA->contest[1];
        break;
    case PKM_PARAM_CONTEST_CUTE:
        result = blockA->contest[2];
        break;
    case PKM_PARAM_CONTEST_SMART:
        result = blockA->contest[3];
        break;
    case PKM_PARAM_CONTEST_TOUGH:
        result = blockA->contest[4];
        break;
    case PKM_PARAM_CONTEST_SHEEN:
        result = blockA->contest[5];
        break;
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 1:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 2:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 3:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 4:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 5:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 6:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 7:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 8:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 9:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 10:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 11:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 12:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 13:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 14:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 15:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 16:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 17:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 18:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 19:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 20:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 21:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 22:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 23:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 24:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 25:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 26:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 27:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 28:
        result = (blockA->ribbons & (bit << (field - PKM_PARAM_RIBBON_CHAMPION_SINNOH))) != 0;
        break;
    case PKM_PARAM_MOVE1:
    case PKM_PARAM_MOVE2:
    case PKM_PARAM_MOVE3:
    case PKM_PARAM_MOVE4:
        result = blockB->moves[field - PKM_PARAM_MOVE1];
        break;
    case PKM_PARAM_MOVE1_PP:
    case PKM_PARAM_MOVE2_PP:
    case PKM_PARAM_MOVE3_PP:
    case PKM_PARAM_MOVE4_PP:
        result = blockB->pp[field - PKM_PARAM_MOVE1_PP];
        break;
    case PKM_PARAM_MOVE1_PP_UP:
    case PKM_PARAM_MOVE2_PP_UP:
    case PKM_PARAM_MOVE3_PP_UP:
    case PKM_PARAM_MOVE4_PP_UP:
        result = blockB->ppUps[field - PKM_PARAM_MOVE1_PP_UP];
        break;
    case PKM_PARAM_MOVE1_MAX_PP:
    case PKM_PARAM_MOVE2_MAX_PP:
    case PKM_PARAM_MOVE3_MAX_PP:
    case PKM_PARAM_MOVE4_MAX_PP:
        if (blockB->moves[field - PKM_PARAM_MOVE1_MAX_PP] != MOVE_NONE) {
            result = PML_MoveGetMaxPP(blockB->moves[field - PKM_PARAM_MOVE1_MAX_PP],
                                      blockB->ppUps[field - PKM_PARAM_MOVE1_MAX_PP]);
        } else {
            result = 0;
        }
        break;
    case PKM_PARAM_IV_HP:
        result = blockB->hpIV;
        break;
    case PKM_PARAM_IV_ATTACK:
        result = blockB->atkIV;
        break;
    case PKM_PARAM_IV_DEFENSE:
        result = blockB->defIV;
        break;
    case PKM_PARAM_IV_SPEED:
        result = blockB->speIV;
        break;
    case PKM_PARAM_IV_SP_ATTACK:
        result = blockB->spaIV;
        break;
    case PKM_PARAM_IV_SP_DEFENSE:
        result = blockB->spdIV;
        break;
    case PKM_PARAM_IS_EGG:
        result = pkm->badEgg;
        if (result == FALSE) {
            result = blockB->isEgg;
        }
        break;
    case PKM_PARAM_IS_NICKNAMED:
        result = blockB->isNicknamed;
        break;
    case PKM_PARAM_RIBBON_G3_COOL:
    case PKM_PARAM_RIBBON_G3_COOL + 1:
    case PKM_PARAM_RIBBON_G3_COOL + 2:
    case PKM_PARAM_RIBBON_G3_COOL + 3:
    case PKM_PARAM_RIBBON_G3_COOL + 4:
    case PKM_PARAM_RIBBON_G3_COOL + 5:
    case PKM_PARAM_RIBBON_G3_COOL + 6:
    case PKM_PARAM_RIBBON_G3_COOL + 7:
    case PKM_PARAM_RIBBON_G3_COOL + 8:
    case PKM_PARAM_RIBBON_G3_COOL + 9:
    case PKM_PARAM_RIBBON_G3_COOL + 10:
    case PKM_PARAM_RIBBON_G3_COOL + 11:
    case PKM_PARAM_RIBBON_G3_COOL + 12:
    case PKM_PARAM_RIBBON_G3_COOL + 13:
    case PKM_PARAM_RIBBON_G3_COOL + 14:
    case PKM_PARAM_RIBBON_G3_COOL + 15:
    case PKM_PARAM_RIBBON_G3_COOL + 16:
    case PKM_PARAM_RIBBON_G3_COOL + 17:
    case PKM_PARAM_RIBBON_G3_COOL + 18:
    case PKM_PARAM_RIBBON_G3_COOL + 19:
    case PKM_PARAM_RIBBON_G3_COOL + 20:
    case PKM_PARAM_RIBBON_G3_COOL + 21:
    case PKM_PARAM_RIBBON_G3_COOL + 22:
    case PKM_PARAM_RIBBON_G3_COOL + 23:
    case PKM_PARAM_RIBBON_G3_COOL + 24:
    case PKM_PARAM_RIBBON_G3_COOL + 25:
    case PKM_PARAM_RIBBON_G3_COOL + 26:
    case PKM_PARAM_RIBBON_G3_COOL + 27:
    case PKM_PARAM_RIBBON_G3_COOL + 28:
    case PKM_PARAM_RIBBON_G3_COOL + 29:
    case PKM_PARAM_RIBBON_G3_COOL + 30:
    case PKM_PARAM_RIBBON_G3_COOL + 31:
        result = (blockB->ribbons & (bit << (field - PKM_PARAM_RIBBON_G3_COOL))) != 0;
        break;
    case PKM_PARAM_FATEFUL_ENCOUNTER:
        result = blockB->fatefulEncounter;
        break;
    // Reading the sex also stores it, as the species and the PID give it, and renews the checksum
    case PKM_PARAM_SEX:
        result = PML_UtilDerivePkmSex(blockA->species, blockB->form, pkm->pid);
        blockB->gender = result;
        pkm->checksum = PML_CryptoGenKey(&pkm->contentBuffer, PKM_BOX_DATA_SIZE);
        break;
    case PKM_PARAM_FORM:
        result = blockB->form;
        break;
    case PKM_PARAM_NATURE:
        result = blockB->nature;
        break;
    case PKM_PARAM_HIDDEN_ABILITY:
        result = blockB->hiddenAbility;
        break;
    case PKM_PARAM_N_POKEMON:
        result = blockB->nPoke;
        break;
    case PKM_PARAM_UNK72:
        result = blockB->unk1C;
        break;
    case PKM_PARAM_NICKNAME:
        if (pkm->badEgg) {
            GFL_MsgDataLoadStrbuf(g_PMLSpeciesNamesResident, SpeciesNames_Text_BadEgg, buffer);
        } else {
            GFL_StrBufLoadString(buffer, blockC->nickname);
        }
        break;
    case PKM_PARAM_NICKNAME_RAW:
        if (pkm->badEgg) {
            GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, SpeciesNames_Text_BadEgg, buffer, 11);
        } else {
            wcharsncpy(blockC->nickname, buffer, 11);
        }
        break;
    case PKM_PARAM_UNK76:
        result = blockC->unk16;
        break;
    case PKM_PARAM_ORIGIN_GAME:
        result = blockC->originGame;
        break;
    case PKM_PARAM_RIBBON_G4_COOL:
    case PKM_PARAM_RIBBON_G4_COOL + 1:
    case PKM_PARAM_RIBBON_G4_COOL + 2:
    case PKM_PARAM_RIBBON_G4_COOL + 3:
    case PKM_PARAM_RIBBON_G4_COOL + 4:
    case PKM_PARAM_RIBBON_G4_COOL + 5:
    case PKM_PARAM_RIBBON_G4_COOL + 6:
    case PKM_PARAM_RIBBON_G4_COOL + 7:
    case PKM_PARAM_RIBBON_G4_COOL + 8:
    case PKM_PARAM_RIBBON_G4_COOL + 9:
    case PKM_PARAM_RIBBON_G4_COOL + 10:
    case PKM_PARAM_RIBBON_G4_COOL + 11:
    case PKM_PARAM_RIBBON_G4_COOL + 12:
    case PKM_PARAM_RIBBON_G4_COOL + 13:
    case PKM_PARAM_RIBBON_G4_COOL + 14:
    case PKM_PARAM_RIBBON_G4_COOL + 15:
    case PKM_PARAM_RIBBON_G4_COOL + 16:
    case PKM_PARAM_RIBBON_G4_COOL + 17:
    case PKM_PARAM_RIBBON_G4_COOL + 18:
    case PKM_PARAM_RIBBON_G4_COOL + 19:
    case PKM_PARAM_RIBBON_G4_COOL + 20:
        result = (blockC->ribbons & (bit << (field - PKM_PARAM_RIBBON_G4_COOL))) != 0;
        break;
    case PKM_PARAM_OT_NAME:
        GFL_StrBufLoadString(buffer, blockD->otName);
        break;
    case PKM_PARAM_OT_NAME_RAW:
        wcharsncpy(blockD->otName, buffer, 8);
        break;
    case PKM_PARAM_EGG_YEAR:
        result = blockD->eggDate[0];
        break;
    case PKM_PARAM_EGG_MONTH:
        result = blockD->eggDate[1];
        break;
    case PKM_PARAM_EGG_DAY:
        result = blockD->eggDate[2];
        break;
    case PKM_PARAM_MET_YEAR:
        result = blockD->metDate[0];
        break;
    case PKM_PARAM_MET_MONTH:
        result = blockD->metDate[1];
        break;
    case PKM_PARAM_MET_DAY:
        result = blockD->metDate[2];
        break;
    case PKM_PARAM_EGG_LOCATION:
        result = blockD->eggLocation;
        break;
    case PKM_PARAM_MET_LOCATION:
        result = blockD->metLocation;
        break;
    case PKM_PARAM_POKERUS:
        result = blockD->pokerus;
        break;
    case PKM_PARAM_POKEBALL:
        result = blockD->ball;
        break;
    case PKM_PARAM_MET_LEVEL:
        result = blockD->metLevel;
        break;
    case PKM_PARAM_OT_GENDER:
        result = blockD->otGender;
        break;
    case PKM_PARAM_UNK9B:
        result = blockD->unk1D;
        break;
    case PKM_PARAM_UNK9C:
        result = blockD->unk1E;
        break;
    case PKM_PARAM_IVS_ALL:
        result = blockB->hpIV | (blockB->atkIV << 5) | (blockB->defIV << 10) | (blockB->speIV << 15)
                 | (blockB->spaIV << 20) | (blockB->spdIV << 25);
        break;
    case PKM_PARAM_NIDORAN_NICKNAME:
        if ((blockA->species == SPECIES_NIDORAN_F || blockA->species == SPECIES_NIDORAN_M)
            && !blockB->isNicknamed) {
            result = FALSE;
        } else {
            result = TRUE;
        }
        break;
    // Arceus's types are its plate's
    case PKM_PARAM_TYPE1:
    case PKM_PARAM_TYPE2:
        if (blockA->species == SPECIES_ARCEUS && blockA->ability == ABILITY_MULTITYPE) {
            result = PML_ItemGetArceusPlateType(blockA->item);
        } else {
            result = PML_PersonalGetParamSingle(blockA->species, blockB->form,
                                                field - PKM_PARAM_TYPE1 + PERSONAL_TYPE_1);
        }
        break;
    case PKM_PARAM_POKESTAR_FAME:
        result = blockD->pokestarFame;
        break;
    }
    return result;
}

static void PokeParty_SetParamCore(PartyPkm *pkm, PkmField field, u32 value) {
    switch (field) {
    case PKM_PARAM_STATUS:
        pkm->statusCond = value;
        break;
    case PKM_PARAM_LEVEL:
        pkm->level = value;
        break;
    case PKM_PARAM_UNK9F:
        pkm->unk8D = value;
        break;
    case PKM_PARAM_HP:
        pkm->nowHP = value;
        break;
    case PKM_PARAM_MAX_HP:
        pkm->maxHP = value;
        break;
    case PKM_PARAM_ATTACK:
        pkm->atk = value;
        break;
    case PKM_PARAM_DEFENSE:
        pkm->def = value;
        break;
    case PKM_PARAM_SPEED:
        pkm->spe = value;
        break;
    case PKM_PARAM_SP_ATTACK:
        pkm->spa = value;
        break;
    case PKM_PARAM_SP_DEFENSE:
        pkm->spd = value;
        break;
    case PKM_PARAM_MAIL:
        copyMailMsg((const MailData *)value, &pkm->mail);
        break;
    default:
        PML_PkmSetParamCore(&pkm->base, field, value);
        break;
    case PKM_PARAM_UNKA8:
        break;
    }
}

// Writes a field of a decrypted Pokémon. An effort value is cut down to what the total and the stat allow
static void PML_PkmSetParamCore(BoxPkm *pkm, u32 field, u32 value) {
    u64 bit;
    PkmBlockA *blockA = PML_PkmGetParamBlockCore(pkm, pkm->pid, 0);
    PkmBlockB *blockB = PML_PkmGetParamBlockCore(pkm, pkm->pid, 1);
    PkmBlockC *blockC = PML_PkmGetParamBlockCore(pkm, pkm->pid, 2);
    PkmBlockD *blockD = PML_PkmGetParamBlockCore(pkm, pkm->pid, 3);

    switch (field) {
    case PKM_PARAM_PID:
        pkm->pid = value;
        break;
    case PKM_PARAM_PARTY_DECRYPTED:
        pkm->partyDecrypted = value;
        break;
    case PKM_PARAM_BOX_DECRYPTED:
        pkm->boxDecrypted = value;
        break;
    case PKM_PARAM_BAD_EGG:
        pkm->badEgg = value;
        break;
    case PKM_PARAM_CHECKSUM:
        pkm->checksum = value;
        break;
    case PKM_PARAM_SPECIES:
        blockA->species = value;
        break;
    case PKM_PARAM_ITEM:
        if (value <= ITEM_LAST) {
            blockA->item = value;
        }
        break;
    case PKM_PARAM_ID:
        blockA->id = value;
        break;
    case PKM_PARAM_EXP:
        blockA->exp = value;
        break;
    case PKM_PARAM_HAPPINESS:
        blockA->friendship = value;
        break;
    case PKM_PARAM_ABILITY:
        blockA->ability = value;
        break;
    case PKM_PARAM_MARKINGS:
        blockA->markings = value;
        break;
    case PKM_PARAM_REGION:
        blockA->language = value;
        break;
    case PKM_PARAM_EV_HP: {
        int ev = blockA->hpEV;
        int room = PKM_EV_TOTAL_MAX
                   - (blockA->hpEV + blockA->atkEV + blockA->defEV + blockA->speEV + blockA->spaEV + blockA->spdEV);
        int change = value - ev;

        if (change > room) {
            value = ev + room;
        }
        if (value > PKM_EV_MAX) {
            value = PKM_EV_MAX;
        }
        blockA->hpEV = value;
        break;
    }
    case PKM_PARAM_EV_ATTACK: {
        int ev = blockA->atkEV;
        int room = PKM_EV_TOTAL_MAX
                   - (blockA->hpEV + blockA->atkEV + blockA->defEV + blockA->speEV + blockA->spaEV + blockA->spdEV);
        int change = value - ev;

        if (change > room) {
            value = ev + room;
        }
        if (value > PKM_EV_MAX) {
            value = PKM_EV_MAX;
        }
        blockA->atkEV = value;
        break;
    }
    case PKM_PARAM_EV_DEFENSE: {
        int ev = blockA->defEV;
        int room = PKM_EV_TOTAL_MAX
                   - (blockA->hpEV + blockA->atkEV + blockA->defEV + blockA->speEV + blockA->spaEV + blockA->spdEV);
        int change = value - ev;

        if (change > room) {
            value = ev + room;
        }
        if (value > PKM_EV_MAX) {
            value = PKM_EV_MAX;
        }
        blockA->defEV = value;
        break;
    }
    case PKM_PARAM_EV_SPEED: {
        int ev = blockA->speEV;
        int room = PKM_EV_TOTAL_MAX
                   - (blockA->hpEV + blockA->atkEV + blockA->defEV + blockA->speEV + blockA->spaEV + blockA->spdEV);
        int change = value - ev;

        if (change > room) {
            value = ev + room;
        }
        if (value > PKM_EV_MAX) {
            value = PKM_EV_MAX;
        }
        blockA->speEV = value;
        break;
    }
    case PKM_PARAM_EV_SP_ATTACK: {
        int ev = blockA->spaEV;
        int room = PKM_EV_TOTAL_MAX
                   - (blockA->hpEV + blockA->atkEV + blockA->defEV + blockA->speEV + blockA->spaEV + blockA->spdEV);
        int change = value - ev;

        if (change > room) {
            value = ev + room;
        }
        if (value > PKM_EV_MAX) {
            value = PKM_EV_MAX;
        }
        blockA->spaEV = value;
        break;
    }
    case PKM_PARAM_EV_SP_DEFENSE: {
        int ev = blockA->spdEV;
        int room = PKM_EV_TOTAL_MAX
                   - (blockA->hpEV + blockA->atkEV + blockA->defEV + blockA->speEV + blockA->spaEV + blockA->spdEV);
        int change = value - ev;

        if (change > room) {
            value = ev + room;
        }
        if (value > PKM_EV_MAX) {
            value = PKM_EV_MAX;
        }
        blockA->spdEV = value;
        break;
    }
    case PKM_PARAM_CONTEST_COOL:
        blockA->contest[0] = value;
        break;
    case PKM_PARAM_CONTEST_BEAUTY:
        blockA->contest[1] = value;
        break;
    case PKM_PARAM_CONTEST_CUTE:
        blockA->contest[2] = value;
        break;
    case PKM_PARAM_CONTEST_SMART:
        blockA->contest[3] = value;
        break;
    case PKM_PARAM_CONTEST_TOUGH:
        blockA->contest[4] = value;
        break;
    case PKM_PARAM_CONTEST_SHEEN:
        blockA->contest[5] = value;
        break;
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 1:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 2:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 3:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 4:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 5:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 6:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 7:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 8:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 9:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 10:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 11:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 12:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 13:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 14:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 15:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 16:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 17:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 18:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 19:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 20:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 21:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 22:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 23:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 24:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 25:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 26:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 27:
    case PKM_PARAM_RIBBON_CHAMPION_SINNOH + 28:
        bit = 1 << (field - PKM_PARAM_RIBBON_CHAMPION_SINNOH);
        if (value) {
            blockA->ribbons |= bit;
        } else {
            blockA->ribbons &= bit ^ 0xffffffff;
        }
        break;
    case PKM_PARAM_MOVE1:
    case PKM_PARAM_MOVE2:
    case PKM_PARAM_MOVE3:
    case PKM_PARAM_MOVE4:
        blockB->moves[field - PKM_PARAM_MOVE1] = value;
        break;
    case PKM_PARAM_MOVE1_PP:
    case PKM_PARAM_MOVE2_PP:
    case PKM_PARAM_MOVE3_PP:
    case PKM_PARAM_MOVE4_PP:
        blockB->pp[field - PKM_PARAM_MOVE1_PP] = value;
        break;
    case PKM_PARAM_MOVE1_PP_UP:
    case PKM_PARAM_MOVE2_PP_UP:
    case PKM_PARAM_MOVE3_PP_UP:
    case PKM_PARAM_MOVE4_PP_UP:
        blockB->ppUps[field - PKM_PARAM_MOVE1_PP_UP] = value;
        break;
    case PKM_PARAM_IV_HP:
        blockB->hpIV = value;
        break;
    case PKM_PARAM_IV_ATTACK:
        blockB->atkIV = value;
        break;
    case PKM_PARAM_IV_DEFENSE:
        blockB->defIV = value;
        break;
    case PKM_PARAM_IV_SPEED:
        blockB->speIV = value;
        break;
    case PKM_PARAM_IV_SP_ATTACK:
        blockB->spaIV = value;
        break;
    case PKM_PARAM_IV_SP_DEFENSE:
        blockB->spdIV = value;
        break;
    case PKM_PARAM_IS_EGG:
        blockB->isEgg = value;
        break;
    case PKM_PARAM_RIBBON_G3_COOL:
    case PKM_PARAM_RIBBON_G3_COOL + 1:
    case PKM_PARAM_RIBBON_G3_COOL + 2:
    case PKM_PARAM_RIBBON_G3_COOL + 3:
    case PKM_PARAM_RIBBON_G3_COOL + 4:
    case PKM_PARAM_RIBBON_G3_COOL + 5:
    case PKM_PARAM_RIBBON_G3_COOL + 6:
    case PKM_PARAM_RIBBON_G3_COOL + 7:
    case PKM_PARAM_RIBBON_G3_COOL + 8:
    case PKM_PARAM_RIBBON_G3_COOL + 9:
    case PKM_PARAM_RIBBON_G3_COOL + 10:
    case PKM_PARAM_RIBBON_G3_COOL + 11:
    case PKM_PARAM_RIBBON_G3_COOL + 12:
    case PKM_PARAM_RIBBON_G3_COOL + 13:
    case PKM_PARAM_RIBBON_G3_COOL + 14:
    case PKM_PARAM_RIBBON_G3_COOL + 15:
    case PKM_PARAM_RIBBON_G3_COOL + 16:
    case PKM_PARAM_RIBBON_G3_COOL + 17:
    case PKM_PARAM_RIBBON_G3_COOL + 18:
    case PKM_PARAM_RIBBON_G3_COOL + 19:
    case PKM_PARAM_RIBBON_G3_COOL + 20:
    case PKM_PARAM_RIBBON_G3_COOL + 21:
    case PKM_PARAM_RIBBON_G3_COOL + 22:
    case PKM_PARAM_RIBBON_G3_COOL + 23:
    case PKM_PARAM_RIBBON_G3_COOL + 24:
    case PKM_PARAM_RIBBON_G3_COOL + 25:
    case PKM_PARAM_RIBBON_G3_COOL + 26:
    case PKM_PARAM_RIBBON_G3_COOL + 27:
    case PKM_PARAM_RIBBON_G3_COOL + 28:
    case PKM_PARAM_RIBBON_G3_COOL + 29:
    case PKM_PARAM_RIBBON_G3_COOL + 30:
    case PKM_PARAM_RIBBON_G3_COOL + 31:
        bit = 1 << (field - PKM_PARAM_RIBBON_G3_COOL);
        if (value) {
            blockB->ribbons |= bit;
        } else {
            blockB->ribbons &= bit ^ 0xffffffff;
        }
        break;
    case PKM_PARAM_FATEFUL_ENCOUNTER:
        blockB->fatefulEncounter = value;
        break;
    // The sex is always what the species and the PID give
    case PKM_PARAM_SEX:
        blockB->gender = PML_UtilDerivePkmSex(blockA->species, blockB->form, pkm->pid);
        break;
    case PKM_PARAM_FORM:
        blockB->form = value;
        break;
    case PKM_PARAM_NATURE:
        blockB->nature = value;
        break;
    case PKM_PARAM_HIDDEN_ABILITY:
        blockB->hiddenAbility = value;
        break;
    case PKM_PARAM_N_POKEMON:
        blockB->nPoke = value;
        break;
    case PKM_PARAM_UNK72:
        blockB->unk1C = value;
        break;
    case PKM_PARAM_NICKNAME:
        GFL_StrBufStoreString((const StrBuf *)value, blockC->nickname, 11);
        blockB->isNicknamed = PML_PkmIsNicknamed(pkm);
        break;
    case PKM_PARAM_NICKNAME_RAW:
        wcharsncpy((const u16 *)value, blockC->nickname, 11);
        blockB->isNicknamed = PML_PkmIsNicknamed(pkm);
        break;
    case PKM_PARAM_NICKNAME_KEEP_FLAG:
        GFL_StrBufStoreString((const StrBuf *)value, blockC->nickname, 11);
        break;
    case PKM_PARAM_NICKNAME_RAW_KEEP_FLAG:
        wcharsncpy((const u16 *)value, blockC->nickname, 11);
        break;
    case PKM_PARAM_UNK76:
        blockC->unk16 = value;
        break;
    case PKM_PARAM_ORIGIN_GAME:
        blockC->originGame = value;
        break;
    case PKM_PARAM_RIBBON_G4_COOL:
    case PKM_PARAM_RIBBON_G4_COOL + 1:
    case PKM_PARAM_RIBBON_G4_COOL + 2:
    case PKM_PARAM_RIBBON_G4_COOL + 3:
    case PKM_PARAM_RIBBON_G4_COOL + 4:
    case PKM_PARAM_RIBBON_G4_COOL + 5:
    case PKM_PARAM_RIBBON_G4_COOL + 6:
    case PKM_PARAM_RIBBON_G4_COOL + 7:
    case PKM_PARAM_RIBBON_G4_COOL + 8:
    case PKM_PARAM_RIBBON_G4_COOL + 9:
    case PKM_PARAM_RIBBON_G4_COOL + 10:
    case PKM_PARAM_RIBBON_G4_COOL + 11:
    case PKM_PARAM_RIBBON_G4_COOL + 12:
    case PKM_PARAM_RIBBON_G4_COOL + 13:
    case PKM_PARAM_RIBBON_G4_COOL + 14:
    case PKM_PARAM_RIBBON_G4_COOL + 15:
    case PKM_PARAM_RIBBON_G4_COOL + 16:
    case PKM_PARAM_RIBBON_G4_COOL + 17:
    case PKM_PARAM_RIBBON_G4_COOL + 18:
    case PKM_PARAM_RIBBON_G4_COOL + 19:
    case PKM_PARAM_RIBBON_G4_COOL + 20:
        bit = 1 << (field - PKM_PARAM_RIBBON_G4_COOL);
        if (value) {
            blockC->ribbons |= bit;
        } else {
            blockC->ribbons &= bit ^ 0xffffffffffffffff;
        }
        break;
    case PKM_PARAM_OT_NAME:
        GFL_StrBufStoreString((const StrBuf *)value, blockD->otName, 8);
        break;
    case PKM_PARAM_OT_NAME_RAW:
        wcharsncpy((const u16 *)value, blockD->otName, 8);
        break;
    case PKM_PARAM_EGG_YEAR:
        blockD->eggDate[0] = value;
        break;
    case PKM_PARAM_EGG_MONTH:
        blockD->eggDate[1] = value;
        break;
    case PKM_PARAM_EGG_DAY:
        blockD->eggDate[2] = value;
        break;
    case PKM_PARAM_MET_YEAR:
        blockD->metDate[0] = value;
        break;
    case PKM_PARAM_MET_MONTH:
        blockD->metDate[1] = value;
        break;
    case PKM_PARAM_MET_DAY:
        blockD->metDate[2] = value;
        break;
    case PKM_PARAM_EGG_LOCATION:
        blockD->eggLocation = value;
        break;
    case PKM_PARAM_MET_LOCATION:
        blockD->metLocation = value;
        break;
    case PKM_PARAM_POKERUS:
        blockD->pokerus = value;
        break;
    case PKM_PARAM_POKEBALL:
        blockD->ball = value;
        break;
    case PKM_PARAM_MET_LEVEL:
        blockD->metLevel = value;
        break;
    case PKM_PARAM_OT_GENDER:
        blockD->otGender = value;
        break;
    case PKM_PARAM_UNK9B:
        blockD->unk1D = value;
        break;
    case PKM_PARAM_UNK9C:
        blockD->unk1E = value;
        break;
    case PKM_PARAM_POKESTAR_FAME:
        blockD->pokestarFame = value;
        break;
    // The packed value has speed last, after the special stats, unlike the one PML_PkmGetParamCore returns
    case PKM_PARAM_IVS_ALL:
        blockB->hpIV = (u8)(value & 0x1f);
        blockB->atkIV = (u8)((value >> 5) & 0x1f);
        blockB->defIV = (u8)((value >> 10) & 0x1f);
        blockB->spaIV = (u8)((value >> 15) & 0x1f);
        blockB->spdIV = (u8)((value >> 20) & 0x1f);
        blockB->speIV = (u8)((value >> 25) & 0x1f);
        break;
    }
}

// Adds value, which may be negative, to a field of a party Pokémon and keeps it in the field's range
static void writePkmBattleData(PartyPkm *pkm, int param, int value) {
    switch (param) {
    case PKM_PARAM_STATUS:
    case PKM_PARAM_LEVEL:
    case PKM_PARAM_LEVEL + 1:
        break;
    case PKM_PARAM_HP:
        if (pkm->nowHP + value > pkm->maxHP) {
            pkm->nowHP = pkm->maxHP;
        } else if (pkm->nowHP + value < 0) {
            pkm->nowHP = 0;
        } else {
            pkm->nowHP += value;
        }
        break;
    case PKM_PARAM_HP + 1:
    case PKM_PARAM_HP + 2:
    case PKM_PARAM_HP + 3:
    case PKM_PARAM_HP + 4:
    case PKM_PARAM_HP + 5:
    case PKM_PARAM_HP + 6:
    case PKM_PARAM_HP + 7:
        break;
    default:
        PML_PkmAddParam(&pkm->base, param, value);
        break;
    }
}

// The same for the fields of a boxed Pokémon: experience, friendship, the contest conditions, PP and PP Ups
static void PML_PkmAddParam(BoxPkm *pkm, int param, int value) {
    PkmBlockA *blockA = PML_PkmGetParamBlockCore(pkm, pkm->pid, 0);
    PkmBlockB *blockB = PML_PkmGetParamBlockCore(pkm, pkm->pid, 1);
    PkmBlockC *blockC = PML_PkmGetParamBlockCore(pkm, pkm->pid, 2);
    PkmBlockD *blockD = PML_PkmGetParamBlockCore(pkm, pkm->pid, 3);

    switch (param) {
    case PKM_PARAM_EXP: {
        u32 maxExp = PML_UtilGetPkmLvExp(blockA->species, blockB->form, MAX_LEVEL);

        if (blockA->exp + value > maxExp) {
            blockA->exp = maxExp;
        } else {
            blockA->exp += value;
        }
        break;
    }
    case PKM_PARAM_HAPPINESS:
        addClampedU8(&blockA->friendship, value, 255);
        break;
    case PKM_PARAM_CONTEST_COOL:
        addClampedU8(&blockA->contest[0], value, 255);
        break;
    case PKM_PARAM_CONTEST_BEAUTY:
        addClampedU8(&blockA->contest[1], value, 255);
        break;
    case PKM_PARAM_CONTEST_CUTE:
        addClampedU8(&blockA->contest[2], value, 255);
        break;
    case PKM_PARAM_CONTEST_SMART:
        addClampedU8(&blockA->contest[3], value, 255);
        break;
    case PKM_PARAM_CONTEST_TOUGH:
        addClampedU8(&blockA->contest[4], value, 255);
        break;
    case PKM_PARAM_CONTEST_SHEEN:
        addClampedU8(&blockA->contest[5], value, 255);
        break;
    case PKM_PARAM_MOVE1_PP:
    case PKM_PARAM_MOVE1_PP + 1:
    case PKM_PARAM_MOVE1_PP + 2:
    case PKM_PARAM_MOVE1_PP + 3:
        addClampedU8(&blockB->pp[param - PKM_PARAM_MOVE1_PP], value,
                      PML_MoveGetMaxPP(blockB->moves[param - PKM_PARAM_MOVE1_PP],
                                       blockB->ppUps[param - PKM_PARAM_MOVE1_PP]));
        break;
    case PKM_PARAM_MOVE1_PP_UP:
    case PKM_PARAM_MOVE1_PP_UP + 1:
    case PKM_PARAM_MOVE1_PP_UP + 2:
    case PKM_PARAM_MOVE1_PP_UP + 3:
        addClampedU8(&blockB->ppUps[param - PKM_PARAM_MOVE1_PP_UP], value, MAX_PP_UPS);
        break;
    }
}

// Adds value to a byte, keeping it between 0 and max
static void addClampedU8(u8 *field, int value, int max) {
    int sum = *field + value;
    int result;

    if (sum > max) {
        result = max;
    } else if (sum < 0) {
        result = 0;
    } else {
        result = sum;
    }
    *field = result;
}

void PML_CryptoRun(void *data, u32 size, u32 key) {
    u16 *words = data;
    u32 i;

    for (i = 0; i < size / 2; i++) {
        words[i] ^= PML_CryptoNextKey(&key);
    }
}

static u16 PML_CryptoNextKey(u32 *key) {
    *key = *key * 0x41c64e6d + 0x6073;
    return *key >> 16;
}

u16 PML_CryptoGenKey(void *data, u32 size) {
    u16 *words = data;
    u16 sum = 0;
    u32 i;

    for (i = 0; i < size / 2; i++) {
        sum += words[i];
    }
    return sum;
}

// Whether the nickname differs from the species' name
static BOOL PML_PkmIsNicknamed(BoxPkm *pkm) {
    BOOL result = FALSE;
    PkmBlockA *blockA = PML_PkmGetParamBlockCore(pkm, pkm->pid, 0);
    PkmBlockC *blockC = PML_PkmGetParamBlockCore(pkm, pkm->pid, 2);

    GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, blockA->species, g_PMLPkmStringTemp, NELEMS(g_PMLPkmStringTemp));
    if (!wcharscmp(g_PMLPkmStringTemp, blockC->nickname)) {
        result = TRUE;
    }
    return result;
}

// A stat, in the order attack, defense, special attack, special defense and speed from 1, with the nature's 10% more
// or less
static u16 adjustStatForNature(u32 nature, u16 value, u32 stat) {
    if (stat >= 6) {
        return value;
    }
    switch (natureStat[nature][stat - 1]) {
    case 1:
        value = value * 110;
        value /= 100;
        break;
    case -1:
        value = value * 90;
        value /= 100;
        break;
    }
    return value;
}

s8 statAffectedByNature(u8 nature, u32 stat) {
    return natureStat[nature][stat - 1];
}

void setNicknameToNick(PartyPkm *pkm) {
    setNameToNick(func_0201d620(pkm));
}

static void setNameToNick(BoxPkm *pkm) {
    GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL), g_PMLPkmStringTemp,
                          NELEMS(g_PMLPkmStringTemp));
    PML_PkmSetParam(pkm, PKM_PARAM_NICKNAME_RAW, (u32)g_PMLPkmStringTemp);
}

static void PML_GrowTblRead(u32 growthRate, u32 *dest) {
    GFL_ArcSysRead(dest, ARCID_GROWTBL, growthRate);
}

static u32 PML_GrowTblGetLvExp(u32 growthRate, u32 level) {
    PML_GrowTblRead(growthRate, g_PMLGrowTblCache);
    return g_PMLGrowTblCache[level];
}

static u16 PML_ItemGetArceusPlateType(u16 item) {
    switch (item) {
    case ITEM_FLAME_PLATE:
        return TYPE_FIRE;
    case ITEM_SPLASH_PLATE:
        return TYPE_WATER;
    case ITEM_ZAP_PLATE:
        return TYPE_ELECTRIC;
    case ITEM_MEADOW_PLATE:
        return TYPE_GRASS;
    case ITEM_ICICLE_PLATE:
        return TYPE_ICE;
    case ITEM_FIST_PLATE:
        return TYPE_FIGHTING;
    case ITEM_TOXIC_PLATE:
        return TYPE_POISON;
    case ITEM_EARTH_PLATE:
        return TYPE_GROUND;
    case ITEM_SKY_PLATE:
        return TYPE_FLYING;
    case ITEM_MIND_PLATE:
        return TYPE_PSYCHIC;
    case ITEM_INSECT_PLATE:
        return TYPE_BUG;
    case ITEM_STONE_PLATE:
        return TYPE_ROCK;
    case ITEM_SPOOKY_PLATE:
        return TYPE_GHOST;
    case ITEM_DRACO_PLATE:
        return TYPE_DRAGON;
    case ITEM_DREAD_PLATE:
        return TYPE_DARK;
    case ITEM_IRON_PLATE:
        return TYPE_STEEL;
    default:
        return TYPE_NORMAL;
    }
}

void *PML_PkmGetParamBlockCore(BoxPkm *pkm, u32 pid, u32 block) {
    return (u8 *)&pkm->contentBuffer + blockShufLUT[(pid & 0x3e000) >> 13][block];
}

// A species' data, read only when the species or the form changes
static PersonalData *PML_PersonalLoadBW2(u16 species, u16 form) {
    if (species != g_PMLPersonalNowLoadForme.species || form != g_PMLPersonalNowLoadForme.form) {
        g_PMLPersonalNowLoadForme.form = form;
        g_PMLPersonalNowLoadForme.species = species;
        PML_PersonalFetchBW2(species, form, &g_PersonalDataCache);
    }
    return &g_PersonalDataCache;
}

// How many abilities a species chooses from, not counting the hidden one
static u32 GetPersonalAbilCount(PersonalData *personal) {
    if (PML_PersonalGetParam(personal, PERSONAL_ABILITY_2) != 0) {
        return 2;
    }
    return 1;
}

u32 PML_PersonalGetParamSingle(u16 species, u16 form, u32 param) {
    return PML_PersonalGetParam(PML_PersonalLoadBW2(species, form), param);
}

void setLevel(PartyPkm *pkm, u32 level) {
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);

    PokeParty_SetParam(pkm, PKM_PARAM_EXP, PML_UtilGetPkmLvExp(species, form, level));
    PokeParty_RecalcStats(pkm);
}

void copyPartyPkm(const PartyPkm *src, PartyPkm *dest) {
    *dest = *src;
}

u16 _getTypeForPlate(u16 item) {
    return PML_ItemGetArceusPlateType(item);
}

u32 PML_ItemGetGenesectDriveForme(u16 item) {
    switch (item) {
    case ITEM_DOUSE_DRIVE:
        return 1;
    case ITEM_SHOCK_DRIVE:
        return 2;
    case ITEM_BURN_DRIVE:
        return 3;
    case ITEM_CHILL_DRIVE:
        return 4;
    default:
        return 0;
    }
}

u8 PML_PkmSanitizeForme(u16 species, u8 form) {
    if (species != SPECIES_EGG) {
        if (form >= (int)PML_PersonalGetParamSingle(species, form, PERSONAL_FORM_COUNT)) {
            form = 0;
        }
        return form;
    } else {
        if (form >= 2) {
            form = 0;
        }
        return form;
    }
}

u8 PML_PkmGetSpriteForme(u16 species, u8 form) {
    u16 i;

    if (form != 0) {
        for (i = 0; i < NELEMS(sSharedSpriteFormSpecies); i++) {
            if (species == sSharedSpriteFormSpecies[i]) {
                return 0;
            }
        }
    }
    return form;
}

int PML_GetPokestarFameLevel(u8 fame) {
    int level = 0;

    if (fame < 100) {
        level = 0;
    } else if (fame >= 100 && fame < 150) {
        level = 1;
    } else if (fame >= 150 && fame < 200) {
        level = 2;
    } else if (fame >= 200 && fame < 250) {
        level = 3;
    } else if (fame >= 250) {
        level = 4;
    }
    return level;
}

// The same cache as PML_PersonalLoadBW2's, filled from Black and White's data
static PersonalData *PML_PersonalLoadBW1(u16 species, u16 form) {
    if (species != g_PMLPersonalNowLoadForme.species || form != g_PMLPersonalNowLoadForme.form) {
        g_PMLPersonalNowLoadForme.form = form;
        g_PMLPersonalNowLoadForme.species = species;
        PML_PersonalFetchBW1(species, form, &g_PersonalDataCache);
    }
    return &g_PersonalDataCache;
}

u32 PML_PersonalGetParamSingleBW1(u16 species, u16 form, u32 param) {
    return PML_PersonalGetParam(PML_PersonalLoadBW1(species, form), param);
}
