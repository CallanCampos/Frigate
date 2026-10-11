#ifndef POKEBW2_APP_UI_UI_SCENE_H
#define POKEBW2_APP_UI_UI_SCENE_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"

// Overlay 139 holds the parts of the lower screen that applications share, each its own original file: OBJ
// resources and their actors (ui_scene.c), the bar of icons at the bottom (touchbar.c), printing (print_msg.c), a
// search of message files (msgsearch.c), the menu of two choices (yesno_menu.c) and a scrolling list (frame_list.c).
// Load it with GFL_OvlLoad(OVERLAY_APP_UI) before using any of them
#define OVERLAY_APP_UI OVERLAY_ID(139)

// ui_scene.c: A scene steps through a table of steps, each with callbacks for its parts. A callback that returns a
// BOOL says whether its part is done; the steps go on to the step set with UIScene_SetNext, or end with UI_SCENE_END
typedef struct UIScene UIScene;

// The next step that ends the scene
#define UI_SCENE_END 0xfffe

typedef BOOL (*UISceneFunc)(UIScene *scene, void *param);
typedef void (*UISceneHook)(UIScene *scene, void *param);

typedef struct {
    UISceneFunc begin;
    UISceneHook enter;
    UISceneFunc run;
    UISceneHook leave;
    UISceneFunc end;
} UISceneStep;

struct UIScene {
    const UISceneStep *steps;
    void *param;
    u16 step;
    u16 next;
    u8 flag;
    u8 state;
    u8 counter;
};

UIScene *UIScene_Create(HeapID heapId, const UISceneStep *steps, u8 flag, u16 first, void *param);
void UIScene_Free(UIScene *scene);
// Runs the current step, and returns TRUE once the scene has ended
BOOL UIScene_Main(UIScene *scene);
void UIScene_SetNext(UIScene *scene, u16 next);
u8 UIScene_GetCounter(UIScene *scene);
void UIScene_IncCounter(UIScene *scene);

// The OBJ resources of a set of files, and where to load them from
typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u32 vramType;
} UIObjRes;

typedef struct {
    u32 vramType;
    // Bit 0 loads the whole palette, bit 1 means the characters are compressed
    u32 flags;
    u32 arcId;
    u32 paletteFile;
    u32 charFile;
    u32 cellFile;
    u32 animFile;
    u8 paletteOffset;
    u8 paletteStart;
    u8 paletteCount;
} UIObjResSetup;

void UIObjRes_Load(UIObjRes *res, UIObjResSetup *setup, ClActUnit *unit, HeapID heapId);
void UIObjRes_Free(UIObjRes *res);
// Creates an actor of the resources at (x, y), playing an animation
ClActor *UIObjRes_CreateActor(UIObjRes *res, ClActUnit *unit, u8 x, u8 y, u8 anim, HeapID heapId);

#endif // POKEBW2_APP_UI_UI_SCENE_H
