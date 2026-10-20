#include "types.h"
#include "app/research_radar/percentage.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

enum {
    ACTOR_PERCENT,
    ACTOR_HUNDREDS,
    ACTOR_TENS,
    ACTOR_ONES,
    ACTOR_COUNT,
};

struct Percentage {
    HeapID heapId;
    u8 value;
    int x;
    int y;
    ClActUnit *unit;
    ClActor *actors[ACTOR_COUNT];
    PercentageResources resources;
};

static void Percentage_SetVisibleCore(Percentage *percentage, BOOL visible);
static void Percentage_UpdateActors(Percentage *percentage);
static void Percentage_SetValueCore(Percentage *percentage, u8 value);
static void Percentage_SetPosCore(Percentage *percentage, int x, int y);
static u8 Percentage_GetOnes(Percentage *percentage);
static u8 Percentage_GetTens(Percentage *percentage);
static u8 Percentage_GetHundreds(Percentage *percentage);
static void Percentage_GetOnesPos(const Percentage *percentage, s16 *x, s16 *y);
static void Percentage_GetTensPos(const Percentage *percentage, s16 *x, s16 *y);
static void Percentage_GetHundredsPos(const Percentage *percentage, s16 *x, s16 *y);
static Percentage *Percentage_Alloc(HeapID heapId);
static void Percentage_Free(Percentage *percentage);
static void Percentage_InitWork(Percentage *percentage, HeapID heapId);
static void Percentage_Setup(Percentage *percentage, const PercentageResources *resources, ClActUnit *unit);
static void Percentage_Teardown(Percentage *percentage);
static void Percentage_SetUnit(Percentage *percentage, ClActUnit *unit);
static void Percentage_CreateActors(Percentage *percentage);
static void Percentage_DeleteActors(Percentage *percentage);
static void Percentage_SetResources(Percentage *percentage, const PercentageResources *resources);

Percentage *Percentage_Create(HeapID heapId, const PercentageResources *resources, ClActUnit *unit) {
    Percentage *percentage = Percentage_Alloc(heapId);

    Percentage_InitWork(percentage, heapId);
    Percentage_Setup(percentage, resources, unit);
    return percentage;
}

void Percentage_Delete(Percentage *percentage) {
    Percentage_Teardown(percentage);
    Percentage_Free(percentage);
}

void Percentage_SetPos(Percentage *percentage, int x, int y) {
    Percentage_SetPosCore(percentage, x, y);
    Percentage_UpdateActors(percentage);
}

void Percentage_SetValue(Percentage *percentage, u32 value) {
    Percentage_SetValueCore(percentage, value);
    Percentage_UpdateActors(percentage);
}

void Percentage_SetVisible(Percentage *percentage, BOOL visible) {
    Percentage_SetVisibleCore(percentage, visible);
}

// The hundreds are shown only when they aren't 0, and the tens only when they or the hundreds aren't
static void Percentage_SetVisibleCore(Percentage *percentage, BOOL visible) {
    ClActor_SetVisible(percentage->actors[ACTOR_PERCENT], visible);
    if (Percentage_GetHundreds(percentage)) {
        ClActor_SetVisible(percentage->actors[ACTOR_HUNDREDS], visible);
    }
    if (Percentage_GetHundreds(percentage) || Percentage_GetTens(percentage)) {
        ClActor_SetVisible(percentage->actors[ACTOR_TENS], visible);
    }
    ClActor_SetVisible(percentage->actors[ACTOR_ONES], visible);
}

static void Percentage_UpdateActors(Percentage *percentage) {
    ClActorPos pos;
    ClActor *actor = percentage->actors[ACTOR_PERCENT];
    PercentageResources *resources = &percentage->resources;

    pos.x = percentage->x;
    pos.y = percentage->y;
    ClActor_SetSurfacePos(actor, &pos, resources->surface);
    ClActor_SetSequence(actor, resources->percentSequence);
    ClActor_SetAnimTime(actor, 0);
    ClActor_StopAnim(actor);
    ClActor_SetVisible(actor, FALSE);

    actor = percentage->actors[ACTOR_HUNDREDS];
    Percentage_GetHundredsPos(percentage, &pos.x, &pos.y);
    ClActor_SetSurfacePos(actor, &pos, resources->surface);
    ClActor_SetSequence(actor, resources->digitSequence);
    ClActor_SetAnimTime(actor, Percentage_GetHundreds(percentage) * FX32_ONE);
    ClActor_StopAnim(actor);
    ClActor_SetVisible(actor, FALSE);

    actor = percentage->actors[ACTOR_TENS];
    Percentage_GetTensPos(percentage, &pos.x, &pos.y);
    ClActor_SetSurfacePos(actor, &pos, resources->surface);
    ClActor_SetSequence(actor, resources->digitSequence);
    ClActor_SetAnimTime(actor, Percentage_GetTens(percentage) * FX32_ONE);
    ClActor_StopAnim(actor);
    ClActor_SetVisible(actor, FALSE);

    actor = percentage->actors[ACTOR_ONES];
    Percentage_GetOnesPos(percentage, &pos.x, &pos.y);
    ClActor_SetSurfacePos(actor, &pos, resources->surface);
    ClActor_SetSequence(actor, resources->digitSequence);
    ClActor_SetAnimTime(actor, Percentage_GetOnes(percentage) * FX32_ONE);
    ClActor_StopAnim(actor);
    ClActor_SetVisible(actor, FALSE);
}

static void Percentage_SetValueCore(Percentage *percentage, u8 value) {
    percentage->value = value;
}

static void Percentage_SetPosCore(Percentage *percentage, int x, int y) {
    percentage->x = x;
    percentage->y = y;
}

static u8 Percentage_GetOnes(Percentage *percentage) {
    return percentage->value % 10;
}

static u8 Percentage_GetTens(Percentage *percentage) {
    return percentage->value % 100 / 10;
}

static u8 Percentage_GetHundreds(Percentage *percentage) {
    return percentage->value / 100;
}

static void Percentage_GetOnesPos(const Percentage *percentage, s16 *x, s16 *y) {
    *x = percentage->x + 20;
    *y = percentage->y - 8;
}

static void Percentage_GetTensPos(const Percentage *percentage, s16 *x, s16 *y) {
    *x = percentage->x + 15;
    *y = percentage->y - 8;
}

static void Percentage_GetHundredsPos(const Percentage *percentage, s16 *x, s16 *y) {
    *x = percentage->x + 10;
    *y = percentage->y - 8;
}

static Percentage *Percentage_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(Percentage), FALSE, "percentage.c", 420);
}

static void Percentage_Free(Percentage *percentage) {
    GFL_HeapFree(percentage);
}

static void Percentage_InitWork(Percentage *percentage, HeapID heapId) {
    int i;

    percentage->heapId = heapId;
    percentage->value = 0;
    percentage->unit = NULL;
    for (i = 0; i < ACTOR_COUNT; i++) {
        percentage->actors[i] = NULL;
    }
}

static void Percentage_Setup(Percentage *percentage, const PercentageResources *resources, ClActUnit *unit) {
    Percentage_SetUnit(percentage, unit);
    Percentage_SetResources(percentage, resources);
    Percentage_CreateActors(percentage);
}

static void Percentage_Teardown(Percentage *percentage) {
    Percentage_DeleteActors(percentage);
}

static void Percentage_SetUnit(Percentage *percentage, ClActUnit *unit) {
    percentage->unit = unit;
}

static void Percentage_CreateActors(Percentage *percentage) {
    ClActorSetup setup;
    PercentageResources *resources = &percentage->resources;
    int i;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    for (i = 0; i < ACTOR_COUNT; i++) {
        percentage->actors[i] = ClActor_Create(percentage->unit, resources->chars, resources->palette,
                                               resources->cellAnims, &setup, resources->surface, percentage->heapId);
    }
}

static void Percentage_DeleteActors(Percentage *percentage) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActor_Delete(percentage->actors[i]);
    }
}

static void Percentage_SetResources(Percentage *percentage, const PercentageResources *resources) {
    percentage->resources = *resources;
}
