#ifndef POKEBW2_SAVE_MUSICAL_SAVEDATA_H
#define POKEBW2_SAVE_MUSICAL_SAVEDATA_H

#include "types.h"
#include "struct_decls.h"

// The musical's save block 0x2a (swan's getAddressOfMusicalDataInfo): the photo of the last finale, the props
// owned and the ones not yet looked at, and the musical's counters

#define MUSICAL_PROP_MAX 100
#define MUSICAL_PROP_BYTES (MUSICAL_PROP_MAX / 8 + 20)

// A prop a Pokémon wears in a musical shot, by the slot it is worn on
typedef struct {
    u16 itemId;
    s16 unk2;
    u8 unk4;
    u8 unk5[3];
} MusicalShotEquip;

typedef struct {
    u16 species;
    u16 sex : 2;
    u16 rare : 1;
    u16 form : 5;
    u32 personality;
    u16 name[8];
    MusicalShotEquip equips[8];
} MusicalShotPoke;

// The photo of a musical's finale, which the musical event fills in and the save keeps. A month of 0 means no photo
typedef struct {
    u32 unk0_0 : 5;
    // A bit for each Pokémon that got the most points
    u32 tops : 4;
    u32 year : 7;
    u32 month : 5;
    u32 day : 6;
    // The player's Pokémon
    u32 player : 2;
    u32 unk0_29 : 3;
    MusicalShotPoke pokes[4];
    u16 title[0x25];
    u8 unk1AE;
    u8 unk1AF;
} MusicalShot;

typedef struct {
    u8 unk0;
    u8 unk1;
    u16 unk2;
} MusicalSaveUnk1E0;

// A prop that the Pokémon wears: its position (10 for none), the prop and its rotation
typedef struct {
    u8 unk0;
    u8 unk1;
    u16 itemId;
    s16 rotation;
} MusicalSaveUnk1B0;

struct MusicalSave {
    MusicalShot shot;
    MusicalSaveUnk1B0 unk1B0[8];
    MusicalSaveUnk1E0 unk1E0[10];
    // A name of 0x25 characters
    u16 name[0x25];
    u16 unk252;
    u16 unk254;
    u16 unk256;
    // A bit per prop owned, and per prop not yet looked at
    u8 props[MUSICAL_PROP_BYTES];
    u8 newProps[MUSICAL_PROP_BYTES];
    u8 unk298[4];
    u8 unk29C;
    u8 unk29D;
    u8 unk29E;
    // Whether the dressing room has been visited
    u8 dressingRoomVisited;
    // Whether the downloaded props were added
    u8 downloadedProps;
    u8 padding2A1[3];
}; // size: 0x2a4

// The size of the block and its init
u32 func_0200ac58(void);
void func_0200ac60(MusicalSave *musical);
MusicalSave *getAddressOfMusicalDataInfo(SaveControl *save);
void func_0200ad20(MusicalSave *musical);
MusicalSaveUnk1B0 *func_0200ad44(MusicalSave *musical);
// Whether a musical photo is saved
BOOL func_0200ad4c(MusicalSave *musical);
MusicalShot *func_0200ad5c(MusicalSave *musical);
// Whether a prop is owned, whether it is flagged as new, and giving one
BOOL func_0200ad60(MusicalSave *musical, u8 prop);
BOOL func_0200ad9c(MusicalSave *musical, u8 prop);
void func_0200add8(MusicalSave *musical, u8 prop);
// Forgets which props are new
void func_0200ae14(MusicalSave *musical);
// How many props are owned, and whether all of them are
u32 func_0200ae30(MusicalSave *musical);
BOOL func_0200ae58(MusicalSave *musical);
MusicalSaveUnk1E0 *func_0200ae6c(MusicalSave *musical, u8 index);
// The counters at 0x252 and 0x254, and adding one to them, up to 0xffff
u16 func_0200ae78(MusicalSave *musical);
void func_0200ae84(MusicalSave *musical);
u16 func_0200ae9c(MusicalSave *musical);
void func_0200aea4(MusicalSave *musical);
u8 func_0200aebc(MusicalSave *musical, u8 index);
void func_0200aec8(MusicalSave *musical, u8 index, u8 value);
u8 func_0200aed4(MusicalSave *musical);
void func_0200aedc(MusicalSave *musical, u8 value);
u8 func_0200aee4(MusicalSave *musical);
void func_0200aef0(MusicalSave *musical, u8 value);
u8 func_0200aefc(MusicalSave *musical);
void func_0200af08(MusicalSave *musical, u8 value);
const u16 *func_0200af14(MusicalSave *musical);
// The counter at 0x256, and adding to it, up to 0xffff
void func_0200af1c(MusicalSave *musical, u16 value);
u16 func_0200af38(MusicalSave *musical);
BOOL func_0200af44(MusicalSave *musical);
void func_0200af50(MusicalSave *musical, BOOL value);
// Whether the downloaded props were added, which func_0200af64 sets
u8 func_0200af5c(MusicalSave *musical);
void func_0200af64(MusicalSave *musical, u8 value);
u8 *func_0200af6c(MusicalSave *musical);

#endif // POKEBW2_SAVE_MUSICAL_SAVEDATA_H
