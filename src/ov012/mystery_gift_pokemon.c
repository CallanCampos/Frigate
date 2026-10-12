#include "types.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "field/mystery_gift_pokemon.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/str.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/mystery_gift.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/version.h"

// The ribbons a gift can give, by bit
static const u32 data_ov012_0216af5c[15] = {
    0x69, 0x6a, 0x6b, 0x6c, 0x33, 0x34, 0x2c, 0x2f, 0x30, 0x31, 0x32, 0x66, 0x67, 0x68, 0x2e,
};

PartyPkm *func_ov012_02153160(MysteryGift *gift, HeapID heapId, GameData *gameData) {
    MysteryGiftPokemon *poke = (MysteryGiftPokemon *)gift;
    u16 level = poke->level;
    u32 trainerId = poke->trainerId;
    u8 abilityType = poke->abilityType;
    u8 gender = poke->gender;
    u16 species = poke->species;
    u32 ability;
    u64 pid;
    u32 ivs;
    u16 moves[4];
    u8 stats[6];
    PartyPkm *pkm;
    StrBuf *name;
    int i;
    u32 bit;
    u16 move;
    u8 value;
    u16 location;
    u16 year;
    u8 month;
    u8 day;

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
    if (gift->kind == 1 && species <= 649 && species != 0 && level <= 100 && poke->heldItem <= 638) {
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
        pkm = PokeParty_NewTempPkm(species, level, trainerId, heapId);
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
                PokeParty_SetParam(pkm, data_ov012_0216af5c[bit], TRUE);
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
        value = poke->otGender;
        if (value > 2) {
            value = getTrainerGender(GetGameDataPlayerInfo(gameData));
        }
        PokeParty_SetParam(pkm, PKM_PARAM_OT_GENDER, value);
        if (poke->isEgg) {
            PokeParty_SetParam(pkm, PKM_PARAM_IS_EGG, TRUE);
            name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, SPECIES_EGG);
            PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (u32)name);
            GFL_StrBufFree(name);
            PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, 10);
        }
        year = poke->date >> 16;
        month = poke->date >> 8;
        day = poke->date;
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
    return NULL;
}
