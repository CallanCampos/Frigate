// Overlay 151: the encounter effects that ripple the captured screen like water. The file's name is a guess; the ROM
// has no string for it

#include "field/encounter_effect_ripple.h"
#include "types.h"
#include "field/encounter_effect.h"
#include "field/encounter_effect_capture.h"
#include "field/field.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The middle vertex, where the drops land
#define CENTER_VERTEX (ENCEFF_RIPPLE_ROWS / 2 * ENCEFF_RIPPLE_COLUMNS + ENCEFF_RIPPLE_COLUMNS / 2)

static GameEvent *EncEffRipple_InitWhite(GameSystem *gsys);
static GameEventReturnCode EncEffRipple_EventCallback(GameEvent *event, u32 *state, void *data);
static void EncEffRipple_Draw(EncEffRipple *ripple);
static BOOL EncEffRipple_Update(EncEffRipple *ripple);
static GameEvent *EncEffRipple_InitBlack(GameSystem *gsys);

GameEvent *EncEffRipple_CreateWhite(GameSystem *gsys, Field *field) {
    VecFx32 pos = { 0, 0, FX32_CONST(33) };

    func_ov036_021c5ebc(Field_GetEncEff(field));
    return EncEffCapture_CreateFlashEvent(gsys, &pos, EncEffRipple_InitWhite,
                                          (void (*)(EncEffGrid *))EncEffRipple_Draw);
}

GameEvent *EncEffRipple_CreateBlack(GameSystem *gsys, Field *field) {
    VecFx32 pos = { 0, 0, FX32_CONST(33) };

    func_ov036_021c5ebc(Field_GetEncEff(field));
    return EncEffCapture_CreateEvent(gsys, &pos, EncEffRipple_InitBlack, (void (*)(EncEffGrid *))EncEffRipple_Draw);
}

static GameEvent *EncEffRipple_InitWhite(GameSystem *gsys) {
    EncEffRipple *ripple = EncEff_AllocWorkArea(Field_GetEncEff(GSYS_GetField(gsys)), sizeof(EncEffRipple), 0x50);
    GameEvent *event = GameEvent_Create(gsys, NULL, EncEffRipple_EventCallback, 0);
    int i;

    ripple->drops = 7;
    ripple->timer = 0;
    ripple->dropInterval = 8;
    ripple->dropHeight = FX32_CONST(0.25);
    ripple->settleTime = 0;
    ripple->fadeDelay = 50;
    ripple->fadeStarted = FALSE;
    for (i = 0; i < ENCEFF_RIPPLE_VERTICES; i++) {
        ripple->vertices[i].x = i % ENCEFF_RIPPLE_COLUMNS * FX32_ONE / 16;
        ripple->vertices[i].y = i / ENCEFF_RIPPLE_COLUMNS * FX32_ONE / 16;
        ripple->vertices[i].z = 0;
    }
    ripple->fadeMode = FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE;
    return event;
}

static GameEventReturnCode EncEffRipple_EventCallback(GameEvent *event, u32 *state, void *data) {
    EncEffRipple *ripple = EncEff_GetWorkArea(Field_GetEncEff(GSYS_GetField(GameEvent_GetGameSystem(event))));

    switch (*state) {
    case 0:
        if (ripple->fadeDelay != 0) {
            ripple->fadeDelay--;
        } else if (!ripple->fadeStarted) {
            GFL_FadeSet(ripple->fadeMode, 0, 16, 0);
            ripple->fadeStarted = TRUE;
        }
        if (EncEffRipple_Update(ripple)) {
            ripple->timer = 6;
            (*state)++;
        }
        break;
    case 1:
        if (ripple->timer != 0) {
            ripple->timer--;
        } else if (!GFL_FadeIsRunning()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

void EncEffRipple_ClearAndDraw(EncEff *effect) {
    void *eventData = EncEff_GetEventData(effect);

    gfxClearColor(GX_RGB(16, 16, 16), 31, 0x7fff, 0, FALSE);
    EncEffCapture_Draw(eventData);
}

// Draws the mesh as a quad strip per row, each vertex showing its part of the screen
static void EncEffRipple_Draw(EncEffRipple *ripple) {
    int x, y;

    G3_PushMtx();
    G3_MaterialColorDiffAmb(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    G3_MaterialColorSpecEmi(GX_RGB(16, 16, 16), GX_RGB(0, 0, 0), FALSE);
    G3_LightColor(GX_LIGHTID_0, GX_RGB(31, 31, 31));
    G3_PolygonAttr(GX_LIGHTMASK_0, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 31, 0);
    {
        VecFx32 trans = { -FX32_CONST(16), FX32_CONST(12), 0 };
        VecFx32 scale = { FX32_CONST(16), FX32_CONST(16), FX32_CONST(64) };

        G3_Translate(trans.x, trans.y, trans.z);
        G3_Scale(scale.x, scale.y, scale.z);
    }
    for (y = 0; y < ENCEFF_RIPPLE_ROWS - 1; y++) {
        VecFx16 *row = &ripple->vertices[y * ENCEFF_RIPPLE_COLUMNS];

        G3_Begin(GX_BEGIN_QUAD_STRIP);
        for (x = 0; x < ENCEFF_RIPPLE_COLUMNS; x++) {
            fx16 x0 = row[x].x;
            fx16 x1 = row[x + ENCEFF_RIPPLE_COLUMNS].x;
            fx16 y0 = row[x].y;
            fx16 y1 = row[x + ENCEFF_RIPPLE_COLUMNS].y;
            fx16 z0 = row[x].z;
            fx16 z1 = row[x + ENCEFF_RIPPLE_COLUMNS].z;

            G3_Normal(0, 0, -FX16_ONE);
            G3_TexCoord(x * 8 * FX32_ONE, y * 8 * FX32_ONE);
            G3_Vtx(x0, -y0, z0);
            G3_TexCoord(x * 8 * FX32_ONE, y * 8 * FX32_ONE + 8 * FX32_ONE);
            G3_Vtx(x1, -y1, z1);
        }
        G3_End();
    }
    G3_PopMtx(1);
}

// Drops the next drop when it is time, then moves each vertex toward the average of its neighbors. Returns TRUE once
// the drops are over and the water has had its time to settle
static BOOL EncEffRipple_Update(EncEffRipple *ripple) {
    s16 heights[ENCEFF_RIPPLE_VERTICES];
    int i;
    VecFx16 *vertex;

    for (i = 0; i < ENCEFF_RIPPLE_VERTICES; i++) {
        heights[i] = ripple->vertices[i].z;
    }
    if (ripple->drops > 0) {
        if (ripple->timer == 0) {
            ripple->vertices[CENTER_VERTEX].z = ripple->dropHeight;
            ripple->timer = ripple->dropInterval;
            ripple->drops--;
        } else {
            ripple->timer--;
        }
        if (ripple->drops <= 0) {
            if (ripple->settleTime == 0) {
                ripple->timer = 6;
                return TRUE;
            }
            ripple->timer = ripple->settleTime;
        }
    } else {
        ripple->timer--;
        if (ripple->timer <= 0) {
            ripple->timer = 6;
            return TRUE;
        }
    }
    for (i = 0; i < ENCEFF_RIPPLE_VERTICES; i++) {
        int x = i % ENCEFF_RIPPLE_COLUMNS;
        int y = i / ENCEFF_RIPPLE_COLUMNS;
        int left, right, up, down;
        // The neighbor's column or row
        int n;

        n = x - 1;
        left = n >= 0 ? heights[y * ENCEFF_RIPPLE_COLUMNS + n] : 0;
        n = x + 1;
        right = n < ENCEFF_RIPPLE_COLUMNS ? heights[y * ENCEFF_RIPPLE_COLUMNS + n] : 0;
        n = y - 1;
        up = n >= 0 ? heights[n * ENCEFF_RIPPLE_COLUMNS + x] : 0;
        n = y + 1;
        down = n < ENCEFF_RIPPLE_ROWS ? heights[n * ENCEFF_RIPPLE_COLUMNS + x] : 0;
        vertex = &ripple->vertices[i];

        ripple->speeds[i] += (left + right + up + down) / 4 - vertex->z;
        vertex->z += ripple->speeds[i];
        if (vertex->z > FX16_ONE) {
            vertex->z = FX16_ONE;
        }
        if (vertex->z < -FX16_ONE) {
            vertex->z = -FX16_ONE;
        }
        // The edges stay flat
        if (x == 0 || x == ENCEFF_RIPPLE_COLUMNS - 1 || y == 0 || y == ENCEFF_RIPPLE_ROWS - 1) {
            vertex->z = 0;
        }
    }
    return FALSE;
}

static GameEvent *EncEffRipple_InitBlack(GameSystem *gsys) {
    EncEffRipple *ripple = EncEff_AllocWorkArea(Field_GetEncEff(GSYS_GetField(gsys)), sizeof(EncEffRipple), 0x50);
    GameEvent *event = GameEvent_Create(gsys, NULL, EncEffRipple_EventCallback, 0);
    int i;

    ripple->drops = 7;
    ripple->timer = 0;
    ripple->dropInterval = 2;
    ripple->dropHeight = FX32_CONST(1) / 3;
    ripple->settleTime = 45;
    ripple->fadeDelay = 55;
    ripple->fadeStarted = FALSE;
    for (i = 0; i < ENCEFF_RIPPLE_VERTICES; i++) {
        ripple->vertices[i].x = i % ENCEFF_RIPPLE_COLUMNS * FX32_ONE / 16;
        ripple->vertices[i].y = i / ENCEFF_RIPPLE_COLUMNS * FX32_ONE / 16;
        ripple->vertices[i].z = 0;
    }
    ripple->fadeMode = FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK;
    return event;
}
