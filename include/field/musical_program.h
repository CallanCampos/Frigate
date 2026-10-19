#ifndef POKEBW2_FIELD_MUSICAL_PROGRAM_H
#define POKEBW2_FIELD_MUSICAL_PROGRAM_H

// Overlay 12's musical_program.c: the program of a show, which decides the points of each kind of prop and the
// Pokémon of the other performers

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

MusicalProgram *MusicalProgram_Create(HeapID heapId, Ov210Work *ov210, u16 shots);
void MusicalProgram_Free(MusicalProgram *program);
void MusicalProgram_CalcPoints(HeapID heapId, MusicalProgram *program, MusicalStageParam *stage);
u8 MusicalProgram_GetKindPoints(MusicalProgram *program, u8 kind);
u32 MusicalProgram_PackPoints(MusicalProgram *program);
void MusicalProgram_UnpackPoints(MusicalProgram *program, u32 value);
u32 MusicalProgram_PackNpcs(MusicalProgram *program);
void MusicalProgram_UnpackNpcs(MusicalProgram *program, u32 value);
u8 MusicalProgram_GetTopKind(MusicalProgram *program);
void MusicalProgram_PutNpcOnStage(MusicalProgram *program, MusicalStageParam *stage, u8 pos, u8 cpu, HeapID heapId);
u8 func_ov012_02152614(MusicalProgram *program);
u8 MusicalProgram_GetPropList(MusicalProgram *program);
u8 MusicalProgram_GetNpcTrainerClass(MusicalProgram *program, u8 cpu);
u8 MusicalProgram_GetNpcNameMsg(MusicalProgram *program, u8 cpu);
u8 MusicalProgram_GetPropPoints(MusicalProgram *program, u8 kind);

#endif // POKEBW2_FIELD_MUSICAL_PROGRAM_H
