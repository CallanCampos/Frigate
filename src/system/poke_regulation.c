#include "types.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "constants/types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "system/wordset.h"
#include "text/system/0356.h"

// poke_regulation.c, which the ROM names: checks of Pokémon and parties against a regulation, and the regulation's
// rules as text. Names are not swan's, except PML_PkmIsLegendNational.

#define ARCID_REGULATION 106

#define PRINT_MSG_RULE_COUNT 6
#define PRINT_MSG_STRBUF_SIZE 64

typedef StrBuf *(*PrintMsgValueFunc)(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                     StrBuf *temp, HeapID heapId);

static u32 checkPokemon(Regulation *regulation, PartyPkm *pkm);
static BOOL checkPokemonFlags(Regulation *regulation, PartyPkm *pkm, u32 *flags);
static u32 getLevel(Regulation *regulation, u32 level);
static StrBuf *makeCountText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg, StrBuf *temp,
                             HeapID heapId);
static StrBuf *makeLevelText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg, StrBuf *temp,
                             HeapID heapId);
static StrBuf *makeSpecialPokemonText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                      StrBuf *temp, HeapID heapId);
static StrBuf *makeSamePokemonText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                   StrBuf *temp, HeapID heapId);
static StrBuf *makeSameItemsText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                 StrBuf *temp, HeapID heapId);
static StrBuf *makeLauncherText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                StrBuf *temp, HeapID heapId);
static BOOL checkPicked(Regulation *regulation, PokeParty *party, const u8 *picked, u8 *out, u32 *flags);
static BOOL nextOrder(u8 *order);

static const u16 sNationalLegendaries[] = {
    SPECIES_MEW,     SPECIES_CELEBI, SPECIES_JIRACHI, SPECIES_DEOXYS,   SPECIES_MANAPHY,  SPECIES_DARKRAI,
    SPECIES_SHAYMIN, SPECIES_ARCEUS, SPECIES_KELDEO,  SPECIES_MELOETTA, SPECIES_GENESECT, SPECIES_VICTINI,
};

// The moves an extra rule can ban
static const u16 sExtraRuleMoves[] = {
    MOVE_METRONOME,
    MOVE_TORMENT,
    MOVE_TRANSFORM,
};

static const PrintMsgValueFunc sPrintMsgValueFuncs[PRINT_MSG_RULE_COUNT] = {
    makeCountText, makeLevelText, makeSpecialPokemonText, makeSamePokemonText, makeSameItemsText, makeLauncherText,
};

BOOL PML_PkmIsLegendNational(u16 species) {
    u32 i;

    for (i = 0; i < NELEMS(sNationalLegendaries); i++) {
        if (species == sNationalLegendaries[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

// The REGULATION_FAIL_* bit of the first reason the Pokémon may not join, or 0
static u32 checkPokemon(Regulation *regulation, PartyPkm *pkm) {
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    u32 result;
    int level, levelRule, levelRange;

    if (regulation == NULL) {
        return REGULATION_FAIL_COUNT;
    }
    levelRule = Regulation_GetParam(regulation, REGULATION_PARAM_4);
    levelRange = Regulation_GetParam(regulation, REGULATION_PARAM_5);
    level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    switch (levelRange) {
    case 1:
        if (levelRule > level) {
            return REGULATION_FAIL_LEVEL;
        }
        break;
    case 2:
        if (levelRule < level) {
            return REGULATION_FAIL_LEVEL;
        }
        break;
    }
    result = 0;
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        return REGULATION_FAIL_EGG;
    }
    if (Regulation_CheckParamBit(regulation, REGULATION_PARAM_BITS_A, species)) {
        return REGULATION_FAIL_SPECIES;
    }
    if (Regulation_CheckParamBit(regulation, REGULATION_PARAM_BITS_5C, item)) {
        result = REGULATION_FAIL_ITEM;
    }
    return result;
}

u32 PokeRegulation_CheckPokemonExtraRule(Regulation *regulation, RegulationExtraRule *rule, PartyPkm *pkm) {
    u32 result = checkPokemon(regulation, pkm);
    int i;
    u32 j;

    if (result != 0) {
        return result;
    }
    if (rule != NULL) {
        if (rule->move != MOVE_NONE || rule->banMoves) {
            for (i = 0; i < 4; i++) {
                u16 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

                if (move != MOVE_NONE) {
                    if (move == rule->move) {
                        return REGULATION_FAIL_MOVE;
                    }
                    if (rule->banMoves) {
                        for (j = 0; j < NELEMS(sExtraRuleMoves); j++) {
                            if (move == sExtraRuleMoves[j]) {
                                return REGULATION_FAIL_MOVE;
                            }
                        }
                    }
                }
            }
        }
        if (rule->type != TYPE_NULL) {
            if (rule->type != PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL)
                && rule->type != PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL)) {
                return REGULATION_FAIL_TYPE;
            }
        }
    }
    return 0;
}

u32 PokeRegulation_CheckPicked(Regulation *regulation, PokeParty *party, const u8 *picked) {
    u8 out[PARTY_SIZE];
    u32 flags;

    if (regulation == NULL) {
        return REGULATION_RESULT_OK;
    }
    checkPicked(regulation, party, picked, out, &flags);
    if (flags == 0) {
        return REGULATION_RESULT_OK;
    }
    if ((flags & REGULATION_FAIL_LEVEL) || (flags & REGULATION_FAIL_SPECIES) || (flags & REGULATION_FAIL_EGG)
        || (flags & REGULATION_FAIL_ITEM)) {
        return REGULATION_RESULT_POKEMON;
    }
    if (flags & REGULATION_FAIL_SAME_SPECIES) {
        return REGULATION_RESULT_SAME_SPECIES;
    }
    if (flags & REGULATION_FAIL_SAME_ITEM) {
        return REGULATION_RESULT_SAME_ITEM;
    }
    if (flags & REGULATION_FAIL_LEVEL_TOTAL) {
        return REGULATION_RESULT_LEVEL_TOTAL;
    }
    if (flags & REGULATION_FAIL_REQUIRED) {
        return REGULATION_RESULT_REQUIRED;
    }
    if (flags & REGULATION_FAIL_COUNT) {
        return REGULATION_RESULT_COUNT;
    }
    return REGULATION_RESULT_OK;
}

u32 PokeRegulation_CheckPartyCanEnter(Regulation *regulation, PokeParty *party) {
    u16 species[PARTY_SIZE];
    u16 levels[PARTY_SIZE];
    u16 items[PARTY_SIZE];
    int count = PokeParty_GetPkmCount(party);
    int valid = count;
    int i, j;
    u32 result;

    sys_memset(items, 0, sizeof(items));
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        species[i] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        levels[i] = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
        if (checkPokemon(regulation, pkm) != 0) {
            species[i] = SPECIES_NONE;
            valid--;
        }
    }
    if (Regulation_GetParam(regulation, REGULATION_PARAM_7) == 0 && count > 1) {
        // Of two of a species, only the one of the lower level counts
        for (i = 0; i < count - 1; i++) {
            for (j = i + 1; j < count; j++) {
                if (species[i] == species[j] && species[i] != SPECIES_NONE) {
                    if (levels[i] > levels[j]) {
                        species[i] = SPECIES_NONE;
                    } else {
                        species[j] = SPECIES_NONE;
                    }
                    valid--;
                }
            }
        }
    }
    if (valid < (int)Regulation_GetParam(regulation, REGULATION_PARAM_2)) {
        return REGULATION_RESULT_COUNT;
    }
    result = REGULATION_RESULT_OK;
    if (PokeRegulation_PickTeam(regulation, party, NULL) != TRUE) {
        result = REGULATION_RESULT_LEVEL_TOTAL;
    }
    return result;
}

// Sets the bits of every reason the Pokémon may not join, and returns whether it may
static BOOL checkPokemonFlags(Regulation *regulation, PartyPkm *pkm, u32 *flags) {
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    BOOL ok = TRUE;
    int level, levelRule, levelRange;

    if (regulation == NULL) {
        return ok;
    }
    levelRule = Regulation_GetParam(regulation, REGULATION_PARAM_4);
    levelRange = Regulation_GetParam(regulation, REGULATION_PARAM_5);
    level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    switch (levelRange) {
    case 1:
        if (levelRule > level) {
            ok = FALSE;
            *flags |= REGULATION_FAIL_LEVEL;
        }
        break;
    case 2:
        if (levelRule < level) {
            ok = FALSE;
            *flags |= REGULATION_FAIL_LEVEL;
        }
        break;
    }
    if (Regulation_CheckParamBit(regulation, REGULATION_PARAM_BITS_A, species)) {
        if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
            ok = FALSE;
            *flags |= REGULATION_FAIL_SPECIES;
        }
    }
    if (Regulation_CheckParamBit(regulation, REGULATION_PARAM_BITS_5C, item)) {
        ok = FALSE;
        *flags |= REGULATION_FAIL_ITEM;
    }
    return ok;
}

u32 PokeRegulation_CheckParty(Regulation *regulation, PokeParty *party, u32 *flags) {
    u16 species[PARTY_SIZE];
    u16 items[PARTY_SIZE];
    u16 forms[PARTY_SIZE];
    int count;
    int levelTotal = 0;
    u32 result = REGULATION_RESULT_OK;
    int i, j;
    u32 required, requiredForm;

    if (regulation == NULL) {
        return result;
    }
    for (i = 0; i < PARTY_SIZE; i++) {
        species[i] = SPECIES_NONE;
        forms[i] = 0;
        items[i] = 0;
    }
    count = howManyPokesAreAbleToFight(party);
    if (count < (int)Regulation_GetParam(regulation, REGULATION_PARAM_2)) {
        *flags |= REGULATION_FAIL_COUNT;
        result = REGULATION_RESULT_COUNT;
    }
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (!checkPokemonFlags(regulation, pkm, flags)) {
            result = REGULATION_RESULT_POKEMON;
        }
        if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
            species[i] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            items[i] = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
            forms[i] = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
            levelTotal += PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
        }
    }
    if (Regulation_GetParam(regulation, REGULATION_PARAM_7) == 0 && count > 1) {
        for (i = 0; i < PARTY_SIZE - 1; i++) {
            for (j = i + 1; j < PARTY_SIZE; j++) {
                if (species[i] == species[j] && species[i] != SPECIES_NONE) {
                    *flags |= REGULATION_FAIL_SAME_SPECIES;
                    result = REGULATION_RESULT_SAME_SPECIES;
                }
            }
        }
    }
    if (Regulation_GetParam(regulation, REGULATION_PARAM_8) == 0 && count > 1) {
        for (i = 0; i < PARTY_SIZE - 1; i++) {
            for (j = i + 1; j < PARTY_SIZE; j++) {
                if (items[i] == items[j] && species[i] != SPECIES_NONE && items[i] != 0) {
                    *flags |= REGULATION_FAIL_SAME_ITEM;
                    result = REGULATION_RESULT_SAME_ITEM;
                }
            }
        }
    }
    required = Regulation_GetParam(regulation, REGULATION_PARAM_11);
    requiredForm = Regulation_GetParam(regulation, REGULATION_PARAM_12);
    i = 0;
    if (required != SPECIES_NONE && count >= 1) {
        for (i = 0; i < PARTY_SIZE; i++) {
            if (required == species[i] && requiredForm == forms[i]) {
                break;
            }
        }
    }
    if (i == PARTY_SIZE) {
        *flags |= REGULATION_FAIL_REQUIRED;
        result = REGULATION_RESULT_REQUIRED;
    }
    if (result == REGULATION_RESULT_OK) {
        if (!PokeRegulation_PickTeam(regulation, party, NULL)) {
            result = REGULATION_RESULT_LEVEL_TOTAL;
        }
    }
    return result;
}

// The level the regulation gives a Pokémon of a level
static u32 getLevel(Regulation *regulation, u32 level) {
    u32 levelRule = Regulation_GetParam(regulation, REGULATION_PARAM_4);

    switch (Regulation_GetParam(regulation, REGULATION_PARAM_5)) {
    case 3:
        if (level > levelRule) {
            return levelRule;
        }
        break;
    case 4:
        return levelRule;
    case 5:
        if (level < levelRule) {
            return levelRule;
        }
        break;
    }
    return level;
}

void PokeRegulation_ApplyLevelToParty(Regulation *regulation, PokeParty *party) {
    u32 levelRule = Regulation_GetParam(regulation, REGULATION_PARAM_4);
    int i;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            u32 level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);

            if (level != getLevel(regulation, level)) {
                setLevel(pkm, levelRule);
            }
        }
    }
}

void PokeRegulation_ApplyNicknamesToParty(Regulation *regulation, PokeParty *party, HeapID heapId) {
    int i;

    if (Regulation_GetParam(regulation, REGULATION_PARAM_17) == 0) {
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);

            if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                setNicknameToNick(pkm);
            }
        }
    }
}

BOOL PokeRegulation_PartyHasEgg(Regulation *regulation, PokeParty *party) {
    int i;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) == TRUE
            && PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

Regulation *PokeRegulation_LoadNew(u32 fileId, HeapID heapId) {
    return GFL_ArcSysReadHeapNew(ARCID_REGULATION, fileId, heapId);
}

void PokeRegulation_Load(u32 fileId, Regulation *regulation) {
    GFL_ArcSysRead(regulation, ARCID_REGULATION, fileId);
}

static StrBuf *makeCountText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg, StrBuf *temp,
                             HeapID heapId) {
    StrBuf *strbuf;
    u32 messageId;

    if (regulation->unk2 == 1 && regulation->unk3 == PARTY_SIZE) {
        messageId = Bank0356_Text_NoRestrictions;
    } else if (regulation->unk2 == regulation->unk3) {
        messageId = Bank0356_Text_Empty;
    } else if (regulation->unk2 > 1 && regulation->unk3 == PARTY_SIZE) {
        messageId = Bank0356_Text_More;
    } else {
        messageId = Bank0356_Text_To;
    }
    WordSetNumber(wordset, 0, regulation->unk2, 1, 0, TRUE);
    WordSetNumber(wordset, 1, regulation->unk3, 1, 0, TRUE);
    strbuf = GFL_StrBufCreate(PRINT_MSG_STRBUF_SIZE, heapId);
    GFL_MsgDataLoadStrbuf(printMsg->msgData, messageId, temp);
    GFL_WordSetFormatStrbuf(wordset, strbuf, temp);
    return strbuf;
}

static StrBuf *makeLevelText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg, StrBuf *temp,
                             HeapID heapId) {
    StrBuf *strbuf;
    u32 messageId;

    if (regulation->unk5 == 0) {
        messageId = Bank0356_Text_NoRestrictions;
    } else if (regulation->unk5 == 4) {
        messageId = Bank0356_Text_Becomes;
    } else if (regulation->unk5 == 1) {
        messageId = Bank0356_Text_Up_2;
    } else if (regulation->unk5 == 2) {
        messageId = Bank0356_Text_Up;
    } else if (regulation->unk5 == 3) {
        messageId = Bank0356_Text_Becomes_2;
    } else if (regulation->unk5 == 5) {
        messageId = Bank0356_Text_Becomes_3;
    } else {
        messageId = Bank0356_Text_NoRestrictions;
    }
    WordSetNumber(wordset, 0, regulation->unk4, 3, 0, TRUE);
    strbuf = GFL_StrBufCreate(PRINT_MSG_STRBUF_SIZE, heapId);
    GFL_MsgDataLoadStrbuf(printMsg->msgData, messageId, temp);
    GFL_WordSetFormatStrbuf(wordset, strbuf, temp);
    return strbuf;
}

static StrBuf *makeSpecialPokemonText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                      StrBuf *temp, HeapID heapId) {
    int i;
    u32 messageId;

    for (i = 0; i < (int)sizeof(regulation->unkA); i++) {
        if (regulation->unkA[i] != 0) {
            break;
        }
    }
    if (i == sizeof(regulation->unkA)) {
        messageId = Bank0356_Text_Permitted;
    } else {
        messageId = Bank0356_Text_Banned;
    }
    return GFL_MsgDataLoadStrbufNew(printMsg->msgData, messageId);
}

static StrBuf *makeSamePokemonText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                   StrBuf *temp, HeapID heapId) {
    u32 messageId;

    if (regulation->unk8 == 1) {
        messageId = Bank0356_Text_Permitted_2;
    } else {
        messageId = Bank0356_Text_Banned_2;
    }
    return GFL_MsgDataLoadStrbufNew(printMsg->msgData, messageId);
}

static StrBuf *makeSameItemsText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                 StrBuf *temp, HeapID heapId) {
    u32 messageId;

    if (regulation->unk9 == 1) {
        messageId = Bank0356_Text_Permitted_3;
    } else {
        messageId = Bank0356_Text_Banned_3;
    }
    return GFL_MsgDataLoadStrbufNew(printMsg->msgData, messageId);
}

static StrBuf *makeLauncherText(const Regulation *regulation, WordSet *wordset, RegulationPrintMsg *printMsg,
                                StrBuf *temp, HeapID heapId) {
    u32 messageId;

    if (printMsg->launcher == 1) {
        messageId = Bank0356_Text_On;
    } else if (printMsg->launcher == 0) {
        messageId = Bank0356_Text_Off;
    } else {
        messageId = Bank0356_Text_On;
    }
    return GFL_MsgDataLoadStrbufNew(printMsg->msgData, messageId);
}

void *PokeRegulation_CreatePrintMsg(Regulation *regulation, WordSet *wordset, HeapID heapId, BOOL launcher) {
    RegulationPrintMsg *printMsg = GFL_HeapAllocate(heapId, sizeof(RegulationPrintMsg), TRUE, "poke_regulation.c", 871);
    StrBuf *temp;
    int i;

    printMsg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0356, heapId);
    temp = GFL_StrBufCreate(PRINT_MSG_STRBUF_SIZE, heapId);
    // A regulation with 2 leaves the Wonder Launcher to the players
    if (regulation->unkAB == 2) {
        printMsg->launcher = launcher;
    } else {
        printMsg->launcher = regulation->unkAB;
    }
    for (i = 0; i < PRINT_MSG_RULE_COUNT; i++) {
        printMsg->rule[i] = GFL_MsgDataLoadStrbufNew(printMsg->msgData, Bank0356_Text_NoPokemon + i);
        printMsg->value[i] = sPrintMsgValueFuncs[i](regulation, wordset, printMsg, temp, heapId);
    }
    GFL_StrBufFree(temp);
    return printMsg;
}

void PokeRegulation_FreePrintMsg(void *data) {
    RegulationPrintMsg *printMsg = data;
    int i;

    for (i = 0; i < PRINT_MSG_RULE_COUNT; i++) {
        GFL_StrBufFree(printMsg->rule[i]);
        GFL_StrBufFree(printMsg->value[i]);
    }
    GFL_MsgDataFree(printMsg->msgData);
    GFL_HeapFree(printMsg);
}

BOOL PokeRegulation_PickTeam(Regulation *regulation, PokeParty *party, u8 *picked) {
    // Every order of the party's slots is tried, starting after this one
    u8 order[PARTY_SIZE] = {1, 2, 3, 4, 5, 5};

    while (nextOrder(order)) {
        if (checkPicked(regulation, party, order, picked, NULL) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

// Goes through the Pokémon picked, by their slots from 1 with 0 for none, and takes those the regulation allows,
// until the team is full. out gets the slots taken and flags the reasons others were not. Returns whether the team
// is one the regulation allows
static BOOL checkPicked(Regulation *regulation, PokeParty *party, const u8 *picked, u8 *out, u32 *flags) {
    u32 species[PARTY_SIZE] = {0};
    u32 forms[PARTY_SIZE] = {0};
    u32 items[PARTY_SIZE] = {0};
    u8 count = 0;
    u8 i, j, k;
    BOOL ok;
    u16 levelTotal = 0;
    PartyPkm *pkm;
    u32 pkmSpecies, pkmItem, level;
    u32 levelLimit = Regulation_GetParam(regulation, REGULATION_PARAM_6);

    if (out != NULL) {
        out[0] = 0;
    }
    if (flags != NULL) {
        *flags = 0;
    }
    for (i = 0; i < PARTY_SIZE; i++) {
        u8 slot;
        u32 fail;

        if (picked[i] == 0) {
            continue;
        }
        slot = picked[i] - 1;
        if (slot >= PokeParty_GetPkmCount(party)) {
            continue;
        }
        pkm = PokeParty_GetPkm(party, slot);
        pkmSpecies = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        pkmItem = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
        level = getLevel(regulation, PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL));
        fail = checkPokemon(regulation, pkm);
        if (fail != 0) {
            if (flags != NULL) {
                *flags |= fail;
            }
            continue;
        }
        ok = TRUE;
        for (j = 0; j < count; j++) {
            if (Regulation_GetParam(regulation, REGULATION_PARAM_7) == 0 && pkmSpecies == species[j]) {
                ok = FALSE;
                if (flags != NULL) {
                    *flags |= REGULATION_FAIL_SAME_SPECIES;
                }
                break;
            }
            if (Regulation_GetParam(regulation, REGULATION_PARAM_8) == 0 && pkmItem != 0 && pkmItem == items[j]) {
                ok = FALSE;
                if (flags != NULL) {
                    *flags |= REGULATION_FAIL_SAME_ITEM;
                }
                break;
            }
        }
        if (!ok) {
            continue;
        }
        if (levelLimit != 0 && levelTotal + level > levelLimit) {
            if (flags != NULL) {
                *flags |= REGULATION_FAIL_LEVEL_TOTAL;
            }
            continue;
        }
        if (out != NULL) {
            out[count] = slot + 1;
        }
        species[count] = pkmSpecies;
        forms[count] = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        items[count] = pkmItem;
        levelTotal += (u16)level;
        count++;
        if (count >= (int)Regulation_GetParam(regulation, REGULATION_PARAM_3)) {
            // A Pokémon picked beyond a full team
            if (flags != NULL && count < PARTY_SIZE && picked[count] != 0) {
                *flags |= REGULATION_FAIL_COUNT;
            }
            break;
        }
    }
    if (count >= (int)Regulation_GetParam(regulation, REGULATION_PARAM_2)) {
        u32 required = Regulation_GetParam(regulation, REGULATION_PARAM_11);
        BOOL found = TRUE;

        if (required != SPECIES_NONE) {
            u32 requiredForm = Regulation_GetParam(regulation, REGULATION_PARAM_12);

            found = FALSE;
            for (k = 0; k < count; k++) {
                if (required == species[k] && requiredForm == forms[k]) {
                    found = TRUE;
                    break;
                }
            }
        }
        if (found == TRUE) {
            return TRUE;
        }
        if (flags != NULL) {
            *flags |= REGULATION_FAIL_REQUIRED;
        }
    } else {
        if (flags != NULL) {
            *flags |= REGULATION_FAIL_COUNT;
        }
    }
    return FALSE;
}

// The next arrangement of six slots, each from 1 to 6, that has every slot once. FALSE after the last
static BOOL nextOrder(u8 *order) {
    order[5]++;
    for (; order[0] <= PARTY_SIZE; order[0]++) {
        for (; order[1] <= PARTY_SIZE; order[1]++) {
            for (; order[2] <= PARTY_SIZE; order[2]++) {
                for (; order[3] <= PARTY_SIZE; order[3]++) {
                    for (; order[4] <= PARTY_SIZE; order[4]++) {
                        for (; order[5] <= PARTY_SIZE; order[5]++) {
                            if (order[0] != order[1] && order[0] != order[2] && order[0] != order[3]
                                && order[0] != order[4] && order[0] != order[5] && order[1] != order[2]
                                && order[1] != order[3] && order[1] != order[4] && order[1] != order[5]
                                && order[2] != order[3] && order[2] != order[4] && order[2] != order[5]
                                && order[3] != order[4] && order[3] != order[5] && order[4] != order[5]) {
                                return TRUE;
                            }
                        }
                        order[5] = 1;
                    }
                    order[4] = 1;
                }
                order[3] = 1;
            }
            order[2] = 1;
        }
        order[1] = 1;
    }
    return FALSE;
}
