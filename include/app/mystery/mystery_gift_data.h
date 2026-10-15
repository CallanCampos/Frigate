#ifndef POKEBW2_APP_MYSTERY_MYSTERY_GIFT_DATA_H
#define POKEBW2_APP_MYSTERY_MYSTERY_GIFT_DATA_H

// What a received gift gives, read from its card. Overlay 197's mystery.c, mystery_album.c and mystery_check.c and
// overlay 12's mystery_gift_pokemon.c each compile their own copy of the ribbon table and inline the Pokémon maker, so
// these are static in a header they all include. Our names; swan has none for these overlays

#include "types.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/str.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/mystery_gift.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/version.h"

// The ribbons a gift can give, by bit
static const u32 sRibbonParams[15] = {
    0x69, 0x6a, 0x6b, 0x6c, 0x33, 0x34, 0x2c, 0x2f, 0x30, 0x31, 0x32, 0x66, 0x67, 0x68, 0x2e,
};

// The Pokémon of a Pokémon gift, as overlay 12's func_ov012_02153160 makes it. NULL when the gift's Pokémon is not
// valid
static inline PartyPkm *Mystery_CreateGiftPokemon(const MysteryGift *gift, HeapID heapId, GameData *gameData) {
    const MysteryGiftPokemon *poke = (const MysteryGiftPokemon *)gift;
    // The copy keeps a computed heap ID, such as a tail heap, in a slot of its own before the other locals, as the
    // original's callers in overlay 197 have it
    HeapID heap = heapId;
    u16 level = poke->level;
    u32 trainerId = poke->trainerId;
    u32 species = poke->species;
    u64 pid;
    u8 gender = poke->gender;
    u32 abilityType = poke->abilityType;
    u32 ability;
    u32 ivs;
    u8 stats[6];
    u16 moves[4];
    PartyPkm *pkm;
    StrBuf *name;
    int i;
    u32 bit;
    u16 move;
    u8 value;
    u16 location;
    u32 year;
    u32 month;
    u32 day;

    stats[0] = poke->ivs[0];
    stats[1] = poke->ivs[1];
    stats[2] = poke->ivs[2];
    stats[3] = poke->ivs[4];
    stats[4] = poke->ivs[5];
    stats[5] = poke->ivs[3];
    moves[0] = poke->moves[0];
    moves[1] = poke->moves[1];
    moves[2] = poke->moves[2];
    moves[3] = poke->moves[3];
    if (gift->kind != 1) {
        return NULL;
    }
    if (species > 649) {
        return NULL;
    }
    if (species == 0) {
        return NULL;
    }
    if (level > 100) {
        return NULL;
    }
    if (poke->heldItem > 638) {
        return NULL;
    }
    if (level == 0) {
        level = GFL_RandomLC(100) + 1;
    }
    if (poke->trainerId == 0) {
        trainerId = getIDAsUInt(GetGameDataPlayerInfo(gameData));
    }
    if (gender == 0xff) {
        gender = 2;
    }
    for (i = 0; i < 6; i++) {
        if (stats[i] == 0xff) {
            stats[i] = GFL_RandomLC(32);
        }
    }
    pkm = PokeParty_NewTempPkm(species, level, trainerId, heap);
    ivs = (stats[0] & 0x1f) | ((stats[1] & 0x1f) << 5) | ((stats[2] & 0x1f) << 10) | ((stats[3] & 0x1f) << 15) |
          ((stats[4] & 0x1f) << 20) | ((stats[5] & 0x1f) << 25);
    if (abilityType == 4) {
        abilityType = (u8)GFL_RandomLC(3);
    }
    switch (abilityType) {
    case 0:
        ability = 0;
        break;
    case 1:
        ability = 1;
        break;
    case 2:
        ability = 0;
        break;
    case 3:
        ability = 2;
        break;
    default:
        return NULL;
    }
    if (poke->personality != 0) {
        pid = poke->personality;
    } else if (poke->shininess == 0) {
        pid = PML_GenPID(trainerId, species, poke->form, gender, ability, 0);
    } else if (poke->shininess == 1) {
        pid = PML_GenPID(trainerId, species, poke->form, gender, ability, 2);
    } else if (poke->shininess == 2) {
        pid = PML_GenPID(trainerId, species, poke->form, gender, ability, 1);
    }
    PokeParty_CreatePkm(pkm, poke->species, level, trainerId, ivs, pid);
    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, poke->heldItem);
    PokeParty_SetDefaultMoves(pkm);
    for (i = 0; i < 4; i++) {
        move = moves[i];
        if (move != 0 && move < 560) {
            if (PokeParty_LearnMove(pkm, move) == 0xffff) {
                PokeParty_SetLastMove(pkm, move);
            }
        }
    }
    if (poke->personality == 0 && abilityType == 2) {
        PokeParty_SetHiddenAbil(pkm, species, poke->form);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_FORM, poke->form);
    if (poke->ball != 0 && poke->ball <= 25) {
        PokeParty_SetParam(pkm, PKM_PARAM_POKEBALL, poke->ball);
    } else {
        PokeParty_SetParam(pkm, PKM_PARAM_POKEBALL, 4);
    }
    if (poke->metLevel != 0) {
        PokeParty_SetParam(pkm, PKM_PARAM_MET_LEVEL, poke->metLevel);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_COOL, poke->contest[0]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_BEAUTY, poke->contest[1]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_CUTE, poke->contest[2]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_SMART, poke->contest[3]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_TOUGH, poke->contest[4]);
#ifdef BUGFIX
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_SHEEN, poke->contest[5]);
#else
    // BUG: The sheen is set to the toughness, and the gift's sheen is never used
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_SHEEN, poke->contest[4]);
#endif
    for (bit = 0; bit < 15; bit++) {
        if (poke->ribbons & (1 << bit)) {
            PokeParty_SetParam(pkm, sRibbonParams[bit], TRUE);
        }
    }
    if (poke->version == 0) {
        PokeParty_SetParam(pkm, PKM_PARAM_ORIGIN_GAME, game_version);
    } else {
        PokeParty_SetParam(pkm, PKM_PARAM_ORIGIN_GAME, poke->version);
    }
    if (poke->language == 0) {
        PokeParty_SetParam(pkm, PKM_PARAM_REGION, region);
    } else {
        PokeParty_SetParam(pkm, PKM_PARAM_REGION, poke->language);
    }
    if (poke->nickname[0] == 0) {
        GFL_HeapFree(pkm);
        return NULL;
    }
    if (poke->nickname[0] != GFL_StrBufGetTerminator()) {
        PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME_RAW, (u32)poke->nickname);
    }
    value = poke->nature;
    if (value == 0xff) {
        value = GFL_RandomLC(25);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_NATURE, value);
    PokeParty_SetParam(pkm, PKM_PARAM_EGG_LOCATION, poke->eggLocation);
    PokeParty_SetParam(pkm, PKM_PARAM_MET_LOCATION, poke->metLocation);
    if (poke->otName[0] == 0) {
        GFL_HeapFree(pkm);
        return NULL;
    }
    if (poke->otName[0] == GFL_StrBufGetTerminator()) {
        PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME_RAW, (u32)GetPlayerName(GetGameDataPlayerInfo(gameData)));
    } else {
        PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME_RAW, (u32)poke->otName);
    }
    if (poke->otGender <= 2) {
        PokeParty_SetParam(pkm, PKM_PARAM_OT_GENDER, poke->otGender);
    } else {
        PokeParty_SetParam(pkm, PKM_PARAM_OT_GENDER, getTrainerGender(GetGameDataPlayerInfo(gameData)));
    }
    if (poke->isEgg) {
        PokeParty_SetParam(pkm, PKM_PARAM_IS_EGG, TRUE);
        name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, SPECIES_EGG);
        PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (u32)name);
        GFL_StrBufFree(name);
        PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, 10);
    }
    year = (poke->date >> 16) & 0xffff;
    month = (poke->date >> 8) & 0xff;
    day = poke->date & 0xff;
    if (poke->isEgg) {
        location = poke->eggLocation;
    } else {
        location = poke->metLocation;
    }
    setFatefulEncounterPkmData(func_0201d620(pkm), location, year - 2000, month, day);
    PokeParty_RecalcStats(pkm);
    if (PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL)) {
        GFL_HeapFree(pkm);
        return NULL;
    }
    return pkm;
}

// The values the getters below give for a gift that doesn't give one
#define MYSTERY_GIFT_ITEM_NONE 0
#define MYSTERY_GIFT_PASS_POWER_NONE 0x30
#define MYSTERY_GIFT_ONE_SHOT_NONE 0x7f

// The item of an item gift
static inline u16 Mystery_GetGiftItem(const MysteryGift *gift) {
    s32 value;

    if (gift->kind != 2) {
        value = MYSTERY_GIFT_ITEM_NONE;
    } else {
        value = gift->value;
        if (value <= 0 || value > 638) {
            value = MYSTERY_GIFT_ITEM_NONE;
        }
    }
    return value;
}

// The Pass Power of a gift of kind 3
static inline u16 Mystery_GetGiftPassPower(const MysteryGift *gift) {
    s32 value;

    if (gift->kind != 3) {
        value = MYSTERY_GIFT_PASS_POWER_NONE;
    } else {
        value = gift->value;
        if (value < 0x21 || value > 0x26) {
            value = MYSTERY_GIFT_PASS_POWER_NONE;
        }
    }
    return value;
}

// The one-shot power of a gift of kind 4, as overlay 33's func_ov033_021782d0 reads it
static inline u32 Mystery_GetGiftOneShotPower(const MysteryGift *gift) {
    s32 value;

    if (gift->kind != 4) {
        value = MYSTERY_GIFT_ONE_SHOT_NONE;
    } else {
        value = gift->value;
        if (value < 0x33 || value > 0x36) {
            value = MYSTERY_GIFT_ONE_SHOT_NONE;
        }
    }
    return value;
}

#endif // POKEBW2_APP_MYSTERY_MYSTERY_GIFT_DATA_H
