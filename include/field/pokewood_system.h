#ifndef POKEBW2_FIELD_POKEWOOD_SYSTEM_H
#define POKEBW2_FIELD_POKEWOOD_SYSTEM_H

#include "types.h"
#include "app/p_status.h"
#include "app/pkwd_book.h"
#include "app/pkwd_record_monitor.h"
#include "app/pkwd_selectscreen.h"
#include "app/pokelist.h"
#include "battle/regulation.h"
#include "gfl/heap.h"
#include "save/pokewood.h"
#include "struct_decls.h"

// Pokéstar Studios' state while the player makes a movie, in overlay 62 with its script plugin (plugin 10). Overlay
// 23's event_pokewood.c runs the movie

// The movies, a file each
#define ARCID_POKEWOOD_MOVIE 269

typedef struct {
    u32 unk0;
    u16 unk4;
    // How many Pokémon take part in the movie's battle
    s16 castCount;
    u16 unk8;
    u8 unkA[0xe];
    u16 unk18;
    u16 unk1a;
    u16 unk1c;
} PokewoodMovie;

struct PokewoodSystem {
    HeapID heapId;
    u8 unk2[6];
    // The party slots, from 1, of the Pokémon picked for the movie
    u8 castSlots[6];
    u16 unkE;
    u16 unk10;
    // The movie's rules, whose unk3 is how many Pokémon take part
    Regulation regulation;
    u8 unkCE;
    u8 unkCF;
    u16 unkD0;
    u16 movie;
    u16 *resultVar;
    u16 unkD8;
    u16 unkDA;
    void *pokewoodBlk;
    PokewoodRecord record;
    u32 unkF4;
    u8 unkF8[8];
};

// pokewood_system.c
PokewoodSystem *PokewoodSystem_Create(HeapID heapId);
void PokewoodSystem_Free(PokewoodSystem *sys);
void PokewoodSystem_SetCastFromPokeList(PokewoodSystem *sys, const PokeListParam *param);
void PokewoodSystem_SetupPokeList(PokewoodSystem *sys, GameData *gameData, PokeListParam *param);
void PokewoodSystem_SetupPStatus(PokewoodSystem *sys, GameData *gameData, PStatusParam *param);
void PokewoodSystem_SetResultVar(PokewoodSystem *sys, u16 *var);
u16 *PokewoodSystem_GetResultVar(PokewoodSystem *sys);
void PokewoodSystem_SetUsePlayerCast(PokewoodSystem *sys, u16 value);
u16 PokewoodSystem_GetUsePlayerCast(PokewoodSystem *sys);
void PokewoodSystem_SetMovie(PokewoodSystem *sys, int movie);
u16 PokewoodSystem_GetMovie(PokewoodSystem *sys);
void PokewoodSystem_SetSaveSlot(PokewoodSystem *sys, u16 value);
u16 PokewoodSystem_GetSaveSlot(PokewoodSystem *sys);
void *PokewoodSystem_GetPokewoodBlk(PokewoodSystem *sys);
void PokewoodSystem_CopyCast(PokewoodSystem *sys, GameData *gameData, PokeParty *party);
void PokewoodSystem_SetResultFlag(PokewoodSystem *sys, GameData *gameData, u32 movie, u32 result);
void Pokewood_CheckMedals(GameData *gameData);
void PokewoodSystem_SetRecord(PokewoodSystem *sys, const BtlSetup *setup, u32 movie);
void PokewoodSystem_KeepBestRecord(GameData *gameData, PokewoodSystem *sys);
void PokewoodSystem_RecordScore(GameData *gameData, PokewoodSystem *sys);
void PokewoodSystem_AddGradePoints(GameData *gameData, PokewoodSystem *sys, u32 result, u32 slot);
void PokewoodSystem_UpdateCastFame(GameData *gameData, PokewoodSystem *sys);
void PokewoodSystem_PushLastSeries(GameData *gameData, PokewoodSystem *sys, u32 slot);
void PokewoodSystem_SetSlotInfo(PokewoodSystem *sys, const u8 *src);

// pokewood_setup.c: the parameters of overlay 23's events, which the caller frees once the event has them
PkwdBookParam *PokewoodSetup_CreateBookParam(HeapID heapId, GameSystem *gsys, u16 *var);
void PokewoodSetup_FreeBookParam(PkwdBookParam *param);
// mode is a PKWD_SELECTSCREEN_MODE_*
PkwdSelectScreenParam *PokewoodSetup_CreateMenuParam(HeapID heapId, GameSystem *gsys, u16 mode, u16 *var);
void PokewoodSetup_FreeMenuParam(PkwdSelectScreenParam *param);
PkwdRecordMonitorParam *PokewoodSetup_CreateRecordMonitorParam(HeapID heapId, GameSystem *gsys);
void PokewoodSetup_FreeRecordMonitorParam(PkwdRecordMonitorParam *param);
GameEvent *EventPokewoodSave_Create(GameSystem *gsys, u16 *var);

#endif // POKEBW2_FIELD_POKEWOOD_SYSTEM_H
