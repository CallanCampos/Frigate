#ifndef POKEBW2_FIELD_SP_POKE_GIMMICK_H
#define POKEBW2_FIELD_SP_POKE_GIMMICK_H

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// sp_poke_gimmick.c, overlay 129, which the ROM names: the field gimmick of the special Pokémon's zones (53 and 614),
// which script plugin 16 drives. Its sound effects are Keldeo's form change (SEQ_SE_SW_KERTIO_FORM_*)

typedef struct SpPokeGimmickCurve SpPokeGimmickCurve;

// Which NPC the event of sp_poke_event.c's SpPokeEvent_CreateAppear replaces with the zone's own definition of it
typedef struct {
    u16 actorId;
    u16 uid;
} SpPokeGimmickNpc;

// The work block 1 of all four gimmicks, 0x30 bytes. The jump event of sp_poke_event.c keeps its path in it
typedef struct {
    FieldExpObjSystem *system;
    Field *field;
    s32 mode;
    VecFx32 start;
    VecFx32 end;
    fx32 height;
    u16 frames;
    u16 frame;
    union {
        SpPokeGimmickCurve *curve;
        SpPokeGimmickNpc *npc;
    } data;
} SpPokeGimmickWork;

// The gimmicks' callbacks, which overlay 36 loads: initialize, finish and update, each taking the field
void SpPokeGimmickA_Init(Field *field);
void SpPokeGimmickA_End(Field *field);
void SpPokeGimmickA_Update(Field *field);
void SpPokeGimmickB_Init(Field *field);
void SpPokeGimmickB_End(Field *field);
void SpPokeGimmickB_Update(Field *field);
void SpPokeGimmickC_Init(Field *field);
void SpPokeGimmickC_End(Field *field);
void SpPokeGimmickC_Update(Field *field);
void SpPokeGimmickD_Init(Field *field);
void SpPokeGimmickD_End(Field *field);
void SpPokeGimmickD_Update(Field *field);

void SpPokeGimmick_StartChase(Field *field, u16 a1);
BOOL SpPokeGimmick_IsChaseDone(Field *field);
void SpPokeGimmick_DropActor(Field *field);
GameEvent *SpPokeGimmick_CallFormEvent(GameSystem *gsys);
void SpPokeGimmick_HideActor2(GameSystem *gsys);

#endif // POKEBW2_FIELD_SP_POKE_GIMMICK_H
