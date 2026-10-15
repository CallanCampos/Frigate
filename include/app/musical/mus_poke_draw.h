#ifndef POKEBW2_APP_MUSICAL_MUS_POKE_DRAW_H
#define POKEBW2_APP_MUSICAL_MUS_POKE_DRAW_H

// Overlay 209's mus_poke_draw.c: draws the musical's Pokémon with musical_mcss.c, and keeps where each sprite's marker
// cells were drawn, which is where its props go. The dressing room (overlay 208) and the stage share it

#include "types.h"
#include "app/musical/musical_mcss.h"
#include "field/musical.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// Where a prop goes on a Pokémon, from the marker cell of its position
typedef struct {
    // Whether the cell was drawn
    BOOL valid;
    MusicalMcssCellInfo info;
} MusPokeDrawEquipPos;

// A prop position on a Pokémon, one for each marker cell from kind MUS_POKE_DRAW_CELL_EQUIP
typedef enum {
    MUS_POKE_DRAW_EQUIP_MAX = 9,
} MusPokeDrawEquipNo;

// A Pokémon being drawn
typedef struct {
    BOOL active;
    // Whether it shows its front rather than its back
    BOOL isFront;
    MusicalMcss *mcss;
    MusicalMcss *front;
    // NULL when it has no back
    MusicalMcss *back;
    MusPokeDrawEquipPos equips[MUS_POKE_DRAW_EQUIP_MAX];
    // The offsets of its marker cells of kinds 4 and 5, and whether they were drawn
    VecFx32 markPos4;
    VecFx32 markPos5;
    BOOL markValid4;
    BOOL markValid5;
} MusPokeDraw;

struct MusPokeDrawSys {
    HeapID heapId;
    MusicalMcssSys *mcssSys;
    MusPokeDraw pokes[8];
};

MusPokeDrawSys *MusPokeDraw_InitSystem(HeapID heapId);
void MusPokeDraw_TermSystem(MusPokeDrawSys *sys);
void MusPokeDraw_UpdateSystem(MusPokeDrawSys *sys);
void MusPokeDraw_DrawSystem(MusPokeDrawSys *sys);
// Adds a Pokémon, and its back as well when withBack is set
MusPokeDraw *MusPokeDraw_AddPoke(MusPokeDrawSys *sys, MusicalPoke *musPoke, BOOL withBack);
void MusPokeDraw_DelPoke(MusPokeDrawSys *sys, MusPokeDraw *poke);
void MusPokeDraw_SetPosition(MusPokeDraw *poke, VecFx32 *pos);
void MusPokeDraw_SetScale(MusPokeDraw *poke, VecFx32 *scale);
void MusPokeDraw_SetRotation(MusPokeDraw *poke, u16 rotation);
void MusPokeDraw_SetVisible(MusPokeDraw *poke, BOOL visible);
BOOL MusPokeDraw_IsVisible(MusPokeDraw *poke);
void MusPokeDraw_StartAnime(MusPokeDraw *poke);
void MusPokeDraw_StopAnime(MusPokeDraw *poke);
void MusPokeDraw_ChangeAnime(MusPokeDraw *poke, u16 anime);
// Turns the Pokémon around, to its back or its front
void MusPokeDraw_TurnAround(MusPokeDraw *poke);
void MusPokeDraw_SetFront(MusPokeDraw *poke, BOOL front);
void MusPokeDraw_SetFlip(MusPokeDraw *poke, BOOL flip);
void MusPokeDraw_SetTexBase(MusPokeDrawSys *sys, u32 base);
void MusPokeDraw_SetPlttBase(MusPokeDrawSys *sys, u32 base);
MusPokeDrawEquipPos *MusPokeDraw_GetEquipPos(MusPokeDraw *poke, u32 pos);
VecFx32 *MusPokeDraw_GetMarkPos4(MusPokeDraw *poke);
VecFx32 *MusPokeDraw_GetMarkPos5(MusPokeDraw *poke);

#endif // POKEBW2_APP_MUSICAL_MUS_POKE_DRAW_H
