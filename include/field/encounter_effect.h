#ifndef POKEBW2_FIELD_ENCOUNTER_EFFECT_H
#define POKEBW2_FIELD_ENCOUNTER_EFFECT_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

// Overlay 150 runs the encounter effects that split the screen into a grid of cells, which overlays 152 to 154 set
// up. Overlay 148 runs the event around such an effect

// A cell of the screen, 0x40 bytes
struct EncEffCell {
    VecFx32 pos;
    u8 unk0C[0xc];
    u32 unk18;
    s32 offset;
    u32 unk20;
    // Added to pos.x each frame
    fx32 speed;
    u8 unk28[0x8];
    s32 frame;
    BOOL visible;
    BOOL done;
    // Called for each visible cell, until it returns TRUE
    BOOL (*update)(EncEffCell *cell);
};

struct EncEffGrid {
    EncEffCell cells[192];
    // For the grid's update function
    s32 work[2];
    u16 columns;
    u16 rows;
    u8 unk300C[0x64];
    void (*update)(EncEffGrid *grid);
    u32 unk3074;
    u32 unk3078;
};

struct EncEffGridParam {
    // The size of a cell, in tiles
    u32 cellWidth;
    u32 cellHeight;
    void (*init)(EncEffGrid *grid);
    void (*update)(EncEffGrid *grid);
    BOOL (*cellUpdate)(EncEffCell *cell);
};

void func_ov036_021c5ea0(EncEff *effect);
// Loads overlay 148 alone, for the effects that draw the captured screen themselves
void func_ov036_021c5ebc(EncEff *effect);
void *EncEff_AllocWorkArea(EncEff *effect, u32 size, HeapID heapId);
void *EncEff_GetWorkArea(EncEff *effect);
void *EncEff_GetEventData(EncEff *effect);
void *Field_Get3DCi(Field *field);
void EncEff_CallRenderFunc(EncEff *effect);
GameEvent *EventFieldEffect_CreatePokeSprite(GameSystem *gsys, Fld3DCi *ci, u8 partySlot);
GameEvent *EventFieldEffect_CreateAlderFly(GameSystem *gsys, Fld3DCi *ci, u32 actorId);
// The event of an effect: init makes the effect's event, and render draws it
GameEvent *EncEffCapture_CreateFlashEvent(GameSystem *gsys, const VecFx32 *pos, GameEvent *(*init)(GameSystem *gsys),
                                          void (*render)(EncEffGrid *grid));
void EncEffCapture_Draw(void *data);
GameEvent *EncEffGrid_CreateEvent(GameSystem *gsys, const EncEffGridParam *param, u32 mode);
// Clears the screen to the grid's color, then draws the effect with overlay 148
void EncEffGrid_ClearAndDraw(EncEff *effect);
void EncEffGrid_Draw(EncEffGrid *grid);

GameEvent *func_ov152_021f6200(GameSystem *gsys, Field *field);
GameEvent *func_ov152_021f623c(GameSystem *gsys);
void func_ov152_021f6264(EncEffGrid *grid);
BOOL func_ov152_021f62a4(EncEffCell *cell);

GameEvent *func_ov153_021f6200(GameSystem *gsys, Field *field);
GameEvent *func_ov153_021f623c(GameSystem *gsys);
void func_ov153_021f6268(EncEffGrid *grid);
BOOL func_ov153_021f62c4(EncEffCell *cell);
EncEff *EncEff_Create(HeapID heapId, Field *field);
void EncEff_Free(EncEff *encEff);

#endif // POKEBW2_FIELD_ENCOUNTER_EFFECT_H
