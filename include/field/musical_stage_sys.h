#ifndef POKEBW2_FIELD_MUSICAL_STAGE_SYS_H
#define POKEBW2_FIELD_MUSICAL_STAGE_SYS_H

// Overlay 12's musical_stage_sys.c: the proc that runs overlay 209's stage, and the Pokémon on it

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

struct MusicalStageParam {
    // Overlay 211's communication work
    void *comm;
    MusicalPoke *pokes[4];
    MusicalProgram *program;
    Ov210Work *ov210;
    u32 unk1C;
};

MusicalStageParam *MusicalStageParam_Create(HeapID heapId, void *comm);
void MusicalStageParam_Free(MusicalStageParam *param);
// Puts a Pokémon on the stage
void MusicalStageParam_SetPlayerPoke(MusicalStageParam *param, u8 pos, MusicalPoke *poke);
void MusicalStageParam_CreateNpcPoke(MusicalStageParam *param, u8 pos, u16 species, u8 form, u32 personality, u16 a5,
                                     HeapID heapId);
void MusicalStageParam_SetCommPoke(MusicalStageParam *param, u8 pos, MusicalPoke *poke);
void MusicalStageParam_SetEquip(MusicalStageParam *param, u8 pos, u8 slot, u16 itemId, s16 a4, u8 a5);

extern GameProcFunctions data_ov012_0216dfe8;

// Overlay 209's stage (sta_acting.c), whose main function returns TRUE once the show has faded out
StaActing *StaActing_Init(MusicalStageParam *param, HeapID heapId);
void StaActing_Term(StaActing *stage);
BOOL StaActing_Main(StaActing *stage);

#endif // POKEBW2_FIELD_MUSICAL_STAGE_SYS_H
