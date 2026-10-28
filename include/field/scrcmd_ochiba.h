#ifndef POKEBW2_FIELD_SCRCMD_OCHIBA_H
#define POKEBW2_FIELD_SCRCMD_OCHIBA_H

// Overlay 36's scrcmd_ochiba.c: the fallen leaves effect, the Habitat List checks and the Unity Tower commands

#include "types.h"
#include "gfl/tcb.h"
#include "save/pokedex.h"
#include "struct_decls.h"

// Where the fallen leaves effect plays
typedef struct {
    Field *field;
    FieldPlayer *player;
} OchibaEffectArgs;

typedef struct {
    struct FieldG3DObjSystem *sys;
    u16 resGroup;
    u16 obj;
} OchibaEffectWork;

// A Pokémon of a Habitat List page, and where it appears in each part of the day
typedef struct {
    u16 species;
    u8 encounters[4][3];
    u8 unk0E[14];
} HabitatListEntry;

// A Habitat List page, a file of archive 0x128
typedef struct {
    u8 unk00[8];
    u16 count;
    HabitatListEntry entries[30];
} HabitatList;

// The page of each zone
typedef struct {
    u16 unk0;
    u16 zoneId;
    u8 fileId;
    u8 unk5;
} HabitatListHeader;

extern const HabitatListHeader HABITAT_LIST_HEADERS[57];

void OchibaEffect_Start(OchibaEffectArgs *args);
void OchibaEffect_UpdateTCB(TCB *tcb, void *data);
u8 GetPWTSeason(const WbtSystem *wbt);
BOOL CheckHabitatList(const HabitatList *list, PokeDexSave *pokedex, u8 time, u32 caught);

#endif // POKEBW2_FIELD_SCRCMD_OCHIBA_H
