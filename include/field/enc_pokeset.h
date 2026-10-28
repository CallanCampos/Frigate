#ifndef POKEBW2_FIELD_ENC_POKESET_H
#define POKEBW2_FIELD_ENC_POKESET_H

// Overlay 36's enc_pokeset.c: the wild Pokémon of an encounter. The file name is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/encounter.h"
#include "save/encounter.h"
#include "struct_decls.h"

// The kinds of encounter, which pick the zone's slots
enum {
    ENCTYPE_GRASS,
    ENCTYPE_GRASS_RARE,
    ENCTYPE_GRASS_SHAKING,
    ENCTYPE_SURF,
    ENCTYPE_SURF_RARE,
    ENCTYPE_FISHING,
    ENCTYPE_FISHING_RARE,
    ENCTYPE_MAX,
};

// What an encounter knows of the player's lead Pokémon and how it rolls the wild ones
typedef struct {
    GameData *gameData;
    EncountSave *encountSave;
    int encType;
    u32 mode;
    u32 slotFuncIndex;
    u8 slotCount;
    u8 pokeCount;
    u16 leadSpecies;
    u16 leadItem;
    u8 leadIsEgg;
    u8 leadAbility;
    u8 leadSex;
    u8 leadNature;
    u8 leadLevel;
    u32 trainerId;
    PlayerInfo *playerInfo;
    u32 swarmsEnabled : 1;
    u32 isFishing : 1;
    u32 repelActive : 1;
    u32 forced : 1;
    u32 pairActive : 1;
    u32 doubleBattle : 1;
    u32 rateUp : 1;
    u32 rateDown : 1;
    u32 fishingRateUp : 1;
    u32 compoundEyes : 1;
    u32 synchronize : 1;
    u32 cuteCharm : 1;
    u32 magnetPull : 1;
    u32 staticAbility : 1;
    u32 higherLevel : 1;
    u32 repelsWeaker : 1;
} EncountManager;

// A swarm: its zone, species and form, and level range
typedef struct {
    u16 zoneId;
    u16 species;
    u8 form;
    u8 minLevel;
    u8 maxLevel;
} SwarmData;

// A wild Pokémon to make
typedef struct {
    u16 species;
    u16 item;
    u8 level;
    u8 form;
    // A WILD_SHINY_ value
    u8 shinyLock;
    u8 hiddenAbility;
    u8 unk08[8];
    // A WILD_SEX_ value
    u8 sex;
    u8 unk11[3];
} WildPkmParam;

// Whether a wild Pokémon is shiny: by chance, always or never
enum {
    WILD_SHINY_RANDOM,
    WILD_SHINY_FORCED,
    WILD_SHINY_NEVER,
};

// A wild Pokémon's sex: by chance and the lead's Cute Charm, male or female
enum {
    WILD_SEX_RANDOM,
    WILD_SEX_MALE,
    WILD_SEX_FEMALE,
};

void CreateEncountManager(EncountManager *manager, GameData *gameData, int encType, u32 mode, u16 weather);
u32 FieldEncount_CalcEncountRate(EncountManager *manager, GameData *gameData, u32 rate);
int FieldEncount_GenWilds(EncData *encData, EncountManager *manager, u16 zoneId, WildPkmParam *params);
RoamingPkm *FieldEncount_RndCheckRoaming(EncountManager *manager, u32 zoneId);
// What FieldEncount_RndCheckNPoke returns when none of N's Pokémon appears
#define N_POKE_NONE 0xff

u8 FieldEncount_RndCheckNPoke(EncountManager *manager, u32 zoneId);
void FieldEncount_CreateWildPkm(PartyPkm *pkm, const EncountManager *manager, WildPkmParam *param);
void FieldEncount_GenRoamingPkm(PartyPkm *pkm, const EncountManager *manager, const RoamingPkm *roamingPkm);
void makeNPokeFromData(PartyPkm *pkm, u8 index);
const SwarmData *getSwarmDataPtr(GameData *gameData);

#endif // POKEBW2_FIELD_ENC_POKESET_H
