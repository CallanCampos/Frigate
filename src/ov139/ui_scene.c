// ui_scene.c: scenes that step through a table of steps, and the OBJ resources of the lower screen's parts. The names
// are ours

#include "app/ui/ui_scene.h"
#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"

enum {
    UI_SCENE_STATE_BEGIN,
    UI_SCENE_STATE_ENTER,
    UI_SCENE_STATE_RUN,
    UI_SCENE_STATE_LEAVE,
    UI_SCENE_STATE_NEXT,
};

// No next step set
#define UI_SCENE_NONE 0xffff

UIScene *UIScene_Create(HeapID heapId, const UISceneStep *steps, u8 flag, u16 first, void *param) {
    UIScene *scene = GFL_HeapAllocate(heapId, sizeof(UIScene), FALSE, "ui_scene.c", 78);

    sys_memset(scene, 0, sizeof(UIScene));
    scene->steps = steps;
    scene->flag = flag;
    scene->param = param;
    scene->step = first;
    scene->next = UI_SCENE_NONE;
    return scene;
}

void UIScene_Free(UIScene *scene) {
    GFL_HeapFree(scene);
}

BOOL UIScene_Main(UIScene *scene) {
    const UISceneStep *step = &scene->steps[scene->step];

    switch (scene->state) {
    case UI_SCENE_STATE_BEGIN:
        if (step->begin == NULL) {
            scene->state = UI_SCENE_STATE_ENTER;
        } else {
            if (!step->begin(scene, scene->param)) {
                break;
            }
            scene->counter = 0;
            scene->state = UI_SCENE_STATE_ENTER;
            break;
        }
        // fall through
    case UI_SCENE_STATE_ENTER:
        if (step->enter != NULL) {
            step->enter(scene, scene->param);
        }
        scene->state = UI_SCENE_STATE_RUN;
        // fall through
    case UI_SCENE_STATE_RUN:
        if (step->run(scene, scene->param)) {
            scene->counter = 0;
            if (step->leave != NULL) {
                step->leave(scene, scene->param);
            }
            scene->state = UI_SCENE_STATE_LEAVE;
        }
        break;
    case UI_SCENE_STATE_LEAVE:
        if (step->end == NULL) {
            scene->state = UI_SCENE_STATE_NEXT;
        } else {
            if (!step->end(scene, scene->param)) {
                break;
            }
            scene->counter = 0;
            scene->state = UI_SCENE_STATE_NEXT;
            break;
        }
        // fall through
    case UI_SCENE_STATE_NEXT: {
        u16 next = scene->next;

        scene->next = UI_SCENE_NONE;
        scene->step = next;
        if (next == UI_SCENE_END) {
            return TRUE;
        }
        scene->state = UI_SCENE_STATE_BEGIN;
        scene->counter = 0;
        break;
    }
    }
    return FALSE;
}

void UIScene_SetNext(UIScene *scene, u16 next) {
    scene->next = next;
}

u8 UIScene_GetCounter(UIScene *scene) {
    return scene->counter;
}

void UIScene_IncCounter(UIScene *scene) {
    scene->counter++;
}

void UIObjRes_Load(UIObjRes *res, UIObjResSetup *setup, ClActUnit *unit, HeapID heapId) {
    u32 compressed = setup->flags & 2;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(setup->arcId, heapId);

    if (setup->flags & 1) {
        res->palette = func_0204bc48(arc, setup->paletteFile, setup->vramType, setup->paletteOffset << 5, heapId);
    } else {
        res->palette = func_0204bbb8(arc, setup->paletteFile, setup->vramType, setup->paletteOffset << 5,
                                     setup->paletteStart, setup->paletteCount, heapId);
    }
    res->chars = func_0204b81c(arc, setup->charFile, compressed, setup->vramType, heapId);
    res->cellAnims = func_0204bde0(arc, setup->cellFile, setup->animFile, heapId);
    res->vramType = setup->vramType;
    GFL_ArcToolFree(arc);
}

void UIObjRes_Free(UIObjRes *res) {
    func_0204bcd0(res->palette);
    func_0204b98c(res->chars);
    func_0204be64(res->cellAnims);
}

ClActor *UIObjRes_CreateActor(UIObjRes *res, ClActUnit *unit, u8 x, u8 y, u8 anim, HeapID heapId) {
    ClActorSetup setup = {0};
    ClActor *actor;

    setup.x = x;
    setup.y = y;
    actor = func_0204c040(unit, res->chars, res->palette, res->cellAnims, &setup, res->vramType, heapId);
    func_0204c488(actor, anim);
    return actor;
}
