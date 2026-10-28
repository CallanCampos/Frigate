#ifndef POKEBW2_APP_NAME_ENTRY_H
#define POKEBW2_APP_NAME_ENTRY_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "save/save_control_intr.h"
#include "struct_decls.h"

// The name entry, overlay 280

// What the name entry is for. The mode is also the number of the message that asks for the name (the text bank of
// name_input.c)
enum {
    NAME_ENTRY_PLAYER,
    NAME_ENTRY_POKEMON,
    NAME_ENTRY_BOX,
    NAME_ENTRY_RIVAL,
    NAME_ENTRY_FRIEND,
    NAME_ENTRY_GREETING,
    NAME_ENTRY_THANKS,
    NAME_ENTRY_PASSERBY_MESSAGE,
    NAME_ENTRY_AVENUE_NAME,
    NAME_ENTRY_AVENUE_TITLE,
    NAME_ENTRY_FAVORITE_PHRASE,
    // The Plasma Frigate's password
    NAME_ENTRY_PLASMA_PASSWORD,
    NAME_ENTRY_GAME_SYNC_ID,
    NAME_ENTRY_IMPRESSED,
    NAME_ENTRY_TRAINER_MESSAGE,
    NAME_ENTRY_HAPPY_MESSAGE,
    NAME_ENTRY_POKEMON_RENAME,
};

typedef struct {
    u32 mode;
    // The gender, or the Pokémon's species
    u16 gender;
    // The Pokémon's form, and its sex in the high byte
    u16 unk6;
    PartyPkm *pkm;
    // A StrBuf that the Pokémon name entry formats and prints once the name is given, if not NULL, and the box data and
    // the number of the box, which it names
    StrBuf *unkC;
    BoxSaveAccessor *unk10;
    u32 unk14;
    u32 maxLength;
    // Set when the name is to be left as it was, unchanged or empty (NameEntryParam_IsCanceled)
    u32 canceled;
    // The name, which starts as a default
    StrBuf *name;
    // The save data that the new game creates in the background
    SaveControlIntr *saveTask;
    TrainerGameInfoSave *gameInfo;
    // 1 to allow more than 4 digits, 2 to skip the check of profane words
    u32 unk2C;
    // The save block that a third preset name, for modes that offer some, is read from
    u16 *unk30;
    // Set to which preset name (1 to 3) the name that was entered is, if any
    u32 unk34;
} NameEntryParam;

// Overlay 12's name_input_setup.c
// The name starts as a copy of name, if given
NameEntryParam *setupNameEntry(HeapID heapId, u32 mode, u32 a2, u32 a3, u32 maxLength, const StrBuf *name,
                               TrainerGameInfoSave *gameInfo);
// The name entry for a Pokémon's nickname
NameEntryParam *setupPokemonNameEntry(HeapID heapId, PartyPkm *pkm, u32 maxLength, const StrBuf *name,
                                      TrainerGameInfoSave *gameInfo);
NameEntryParam *pokemonNameEntry(HeapID heapId, PartyPkm *pkm, u32 maxLength, const StrBuf *name, StrBuf *unkC,
                                 BoxSaveAccessor *unk10, u32 unk14, TrainerGameInfoSave *gameInfo);
void NameEntryParam_Free(NameEntryParam *param);
// Copies the entered name, and compares it with a string
void NameEntryParam_GetName(NameEntryParam *param, StrBuf *dest);
BOOL NameEntryParam_IsCanceled(NameEntryParam *param);
BOOL NameEntryParam_CompareName(NameEntryParam *param, const StrBuf *str);
// Overlay 12
void GameBeacon_BroadcastPhrase(StrBuf *name, u8 value);

// The keyboard that each mode last used, a byte of the trainer's info save (not declared in save_control.h)
u8 TrainerGameInfo_GetKeyboardType(TrainerGameInfoSave *info, u32 index);
void TrainerGameInfo_SetKeyboardType(TrainerGameInfoSave *info, u32 index, u8 value);

extern const GameProcFunctions NAME_ENTRY_PROC_FUNCTIONS;
// The sound sequences that the name entry plays, which the new game loads before it
extern const u32 NAME_ENTRY_SOUND_COUNT;
extern const u32 NAME_ENTRY_SOUNDS[];

#endif // POKEBW2_APP_NAME_ENTRY_H
