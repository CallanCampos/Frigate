#ifndef POKEBW2_APP_ZUKAN_COMMON_H
#define POKEBW2_APP_ZUKAN_COMMON_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// What the Pokédex's overlays share, in overlay 302 (zukan_common.c, by the ROM's string): the list of the species that
// it shows, the habitat lists and their data, and what the screens draw. The names are ours; swan has none

// How many areas the habitat list has
#define ZUKAN_HABITAT_COUNT 57
// An area has four seasons and three kinds of habitat
#define ZUKAN_SEASON_COUNT 4
#define ZUKAN_HABITAT_TYPE_COUNT 3

// An area of the habitat list, a table that zukan_common.c and zukan_habitat.c each hold a copy of: the flag that
// shows it, its zone and its file
typedef struct {
    u16 flag;
    u16 zone;
    u8 file;
    u8 padding;
} ZukanHabitatTableEntry;

// The habitats that the player has reached: the first is always true, and the others are flags of the story. An area
// is shown if one of the conditions that it names is true
typedef struct {
    BOOL conditions[3];
} ZukanHabitatConditions;

// A species in an area, in its file
typedef struct {
    u16 species;
    // Whether it is there, by season and kind of habitat
    u8 present[ZUKAN_SEASON_COUNT][ZUKAN_HABITAT_TYPE_COUNT];
    // The same, for the other half of the data
    u8 other[ZUKAN_SEASON_COUNT][ZUKAN_HABITAT_TYPE_COUNT];
    u16 unk1a;
} ZukanHabitatFileEntry;

// An area's file, from archive 296 and 0x352 bytes
typedef struct {
    u8 values[3];
    // The conditions that make the area shown, 1 for each
    u8 conditions[3];
    u16 unk6;
    u16 count;
    ZukanHabitatFileEntry entries[30];
} ZukanHabitatFile;

// A species of an area in the list that the habitat screen shows, 0x1c bytes
typedef struct {
    u16 species;
    u8 present[ZUKAN_SEASON_COUNT][ZUKAN_HABITAT_TYPE_COUNT];
    u8 other[ZUKAN_SEASON_COUNT][ZUKAN_HABITAT_TYPE_COUNT];
    // 1 if the species is not seen, 2 if it is only seen, 3 if it is caught
    u8 status;
    u8 unk1b;
} ZukanHabitatSpecies;

// An area in the list that the habitat screen shows, 0x364 bytes
typedef struct {
    u16 zone;
    u16 values[3];
    u32 unk8;
    ZukanHabitatSpecies species[30];
    // For each condition: 2 if all of its species are caught, 1 if all are seen, else 0
    u32 states[3];
    // 1 if every condition that applies is complete
    u32 complete;
} ZukanHabitatArea;

// What the habitat screen is given, 0xc150 bytes (overlay 303)
typedef struct {
    GameData *gameData;
    u32 count;
    ZukanHabitatArea areas[ZUKAN_HABITAT_COUNT];
    u32 result;
} ZukanHabitatListParam;

// An area in the list of the screen of the Pokédex's awards, 0x2c bytes
typedef struct {
    u16 zone;
    u8 conditions[3];
    u8 unk5;
    u32 states[3];
    // 4 for a condition that does not apply, else 1 to 3
    u32 display[3];
    u32 conditionsMet[3];
} ZukanHabitatSummary;

// What the screen of the Pokédex's awards is given, 0x9d8 bytes (overlay 304)
typedef struct {
    GameData *gameData;
    u8 count;
    ZukanHabitatSummary areas[ZUKAN_HABITAT_COUNT];
    u32 result;
} ZukanHabitatSummaryParam;

// What the Pokédex's list is searched with, 7 bytes: the order, the kind of list, and what to match: the initial's
// group, two types, a color and a shape, each 0xff for any
typedef struct {
    u8 regional;
    u8 order;
    u8 initial;
    u8 type1;
    u8 type2;
    u8 color;
    u8 shape;
} ZukanSearchCriteria;

// What an actor of the Pokédex's screens is created from: where it starts, its resources and its surface
typedef struct {
    ClActorSetup setup;
    u32 chars;
    u32 palette;
    u32 cells;
    u16 unk14;
    u16 surface;
} ZukanActorTemplate;

// Loads the species of the Pokédex's list, in the regional or the national order, and returns how many of its first
// entries to show: up to the last that the player has seen
u32 ZukanCommon_CreateList(PokeDexSave *pokedex, u16 **list, HeapID heapId);
// Makes the Pokédex's last species one that the list has seen, if it is not
void ZukanCommon_SelectSeen(PokeDexSave *pokedex, const u16 *list, u16 count);
// Whether the list starts with its first entry, the Pokédex's first species, which it hides before it is seen
BOOL ZukanCommon_IsFirstShown(PokeDexSave *pokedex);
void ZukanCommon_InitCriteria(PokeDexSave *pokedex, ZukanSearchCriteria *criteria);
// Wipe in and wipe out of the Pokédex's screens
void ZukanCommon_WipeIn(HeapID heapId);
void ZukanCommon_WipeOut(HeapID heapId);
// Scrolls two BGs by a pixel every four frames
void ZukanCommon_ScrollBGs(u8 bg1, u8 bg2, int *timer);
ClActor *ZukanCommon_CreateActor(ClActUnit *unit, const ZukanActorTemplate *template, HeapID heapId);
// The species of the habitat that the player chose, for the season and the kind of habitat that it shows. Returns
// their number
u32 ZukanCommon_LoadHabitatSpecies(PokeDexSave *pokedex, ZukanHabitatSave *habitat, u16 **list, HeapID heapId);
// Builds the list of the areas that the habitat screen shows, or if current only the one that the habitat is on
void ZukanCommon_BuildHabitatList(ZukanHabitatSave *habitat, ZukanHabitatListParam *param, BOOL current);
void ZukanCommon_BuildHabitatSummary(ZukanHabitatSave *habitat, ZukanHabitatSummaryParam *param);
// The habitat map's pulsing glow: starts it, and steps its phase and what it sets the blend to
void ZukanCommon_StartGlow(u16 *phase, int *ev);
void ZukanCommon_StepGlow(u16 *phase, int *ev);

#endif // POKEBW2_APP_ZUKAN_COMMON_H
