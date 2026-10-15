// Overlay 150: the encounter effects that split the screen into a grid of cells, each drawn as a textured quad that
// the effect's overlay (152 to 154) moves, turns and hides. Overlay 148 runs the event around them. The file's name
// is a guess; the ROM has no string for it

#include "types.h"
#include "field/encounter_effect.h"
#include "field/field.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/game_event.h"
#include "system/game_system.h"

static GameEventReturnCode EncEffGrid_EventCallback(GameEvent *event, u32 *state, void *data);
static BOOL EncEffGrid_Update(EncEffGrid *grid);

static const VecFx32 sCellScale = { FX32_ONE, FX32_ONE, FX32_ONE };

// mode picks the fade at the end: to white, or to black
GameEvent *EncEffGrid_CreateEvent(GameSystem *gsys, const EncEffGridParam *param, u32 mode) {
    EncEffGrid *grid = EncEff_AllocWorkArea(Field_GetEncEff(GSYS_GetField(gsys)), sizeof(EncEffGrid), 0x50);
    GameEvent *event = GameEvent_Create(gsys, NULL, EncEffGrid_EventCallback, 0);
    s32 cellWidth = param->cellWidth;
    s32 cellHeight = param->cellHeight;
    s32 width = cellWidth * 8;
    s32 height = cellHeight * 8;
    s32 columns = 32 / cellWidth;
    s32 rows = 24 / cellHeight;
    s32 left, top, i;

    grid->columns = columns;
    grid->rows = rows;
    left = columns / 2 * width - width / 2;
    top = rows / 2 * height - height / 2;
    for (i = 0; i < columns * rows; i++) {
        EncEffCell *cell = &grid->cells[i];
        s32 column = i % columns;
        s32 row = i / columns;

        cell->pos.x = (column * width - left) * FX32_ONE;
        cell->pos.y = (top - row * height) * FX32_ONE;
        cell->pos.z = 0;
        cell->unk18 = 0;
        cell->offset = 0;
        cell->unk20 = 0;
        cell->visible = FALSE;
        cell->done = FALSE;
        cell->update = param->cellUpdate;
    }
    grid->update = param->update;
    if (param->init != NULL) {
        param->init(grid);
    }
    if (mode) {
        grid->unk3074 = 12;
        grid->unk3078 = GX_RGB(31, 31, 31);
    } else {
        grid->unk3074 = 3;
        grid->unk3078 = GX_RGB(0, 0, 0);
    }
    return event;
}

static GameEventReturnCode EncEffGrid_EventCallback(GameEvent *event, u32 *state, void *data) {
    EncEffGrid *grid = EncEff_GetWorkArea(Field_GetEncEff(GSYS_GetField(GameEvent_GetGameSystem(event))));

    switch (*state) {
    case 0:
        if (EncEffGrid_Update(grid)) {
            GFL_FadeSet(grid->unk3074, 0, 16, 0);
            (*state)++;
        }
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

// Updates the grid and its visible cells, and returns whether every cell is done
static BOOL EncEffGrid_Update(EncEffGrid *grid) {
    s32 i, count, doneCount;

    grid->update(grid);
    count = grid->columns * grid->rows;
    doneCount = 0;
    for (i = 0; i < count; i++) {
        EncEffCell *cell = &grid->cells[i];
        BOOL done = FALSE;

        if (!cell->done) {
            if (cell->visible) {
                if (cell->update == NULL) {
                    cell->done = TRUE;
                } else {
                    done = cell->update(cell);
                }
            }
        } else {
            done = TRUE;
        }
        if (done) {
            doneCount++;
        }
    }
    return doneCount >= count;
}

void EncEffGrid_ClearAndDraw(EncEff *effect) {
    void *eventData = EncEff_GetEventData(effect);
    EncEffGrid *grid = EncEff_GetWorkArea(effect);

    gfxClearColor(grid->unk3078, 31, 0x7fff, 0, FALSE);
    EncEffCapture_Draw(eventData);
}

// Draws each cell as a quad that shows its part of the screen
void EncEffGrid_Draw(EncEffGrid *grid) {
    G3_PushMtx();
    G3_MaterialColorDiffAmb(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    G3_MaterialColorSpecEmi(GX_RGB(16, 16, 16), GX_RGB(0, 0, 0), FALSE);
    G3_LightColor(GX_LIGHTID_0, GX_RGB(31, 31, 31));
    G3_PolygonAttr(GX_LIGHTMASK_0, GX_POLYGONMODE_MODULATE, GX_CULL_BACK, 0, 31, 0);
    {
        VecFx32 trans = { 0, 0, 0 };
        VecFx32 scale = sCellScale;
        MtxFx33 rot;
        fx32 s, t;
        s32 count = grid->columns * grid->rows;
        s32 width = 32 / grid->columns * 8;
        s32 height = 24 / grid->rows * 8;
        s32 i;

        scale.x = (width / 8) * FX32_ONE;
        scale.y = (height / 8) * FX32_ONE;
        for (i = 0; i < count; i++) {
            EncEffCell *cell = &grid->cells[i];
            s32 column, row;
            fx32 s1, t1;

            G3_PushMtx();
            column = i % grid->columns;
            row = i / grid->columns;
            trans = cell->pos;
            s = column * width * FX32_ONE;
            t = row * height * FX32_ONE;
            s1 = s + width * FX32_ONE;
            t1 = t + height * FX32_ONE;
            G3_Translate(trans.x, trans.y, trans.z);
            G3_Scale(scale.x, scale.y, scale.z);
            MTX_RotY33_(&rot, FX_SinIdx(cell->offset), FX_CosIdx(cell->offset));
            gfxMultMatrix3x3(&rot);
            MTX_RotZ33_(&rot, FX_SinIdx(cell->unk20), FX_CosIdx(cell->unk20));
            gfxMultMatrix3x3(&rot);
            G3_Begin(GX_BEGIN_QUADS);
            G3_Normal(0, 0, FX16_ONE);
            G3_TexCoord(s, t);
            G3_Vtx(-FX16_CONST(4), FX16_CONST(4), 0);
            G3_TexCoord(s, t1);
            G3_Vtx(-FX16_CONST(4), -FX16_CONST(4), 0);
            G3_TexCoord(s1, t1);
            G3_Vtx(FX16_CONST(4), -FX16_CONST(4), 0);
            G3_TexCoord(s1, t);
            G3_Vtx(FX16_CONST(4), FX16_CONST(4), 0);
            G3_End();
            G3_PopMtx(1);
        }
    }
    G3_PopMtx(1);
}
