#include "types.h"
#include "app/mailbox_local.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "nitro/gx.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"

// The mailbox screen's cell actors. The ROM doesn't name this file; the name is a guess, after box2_obj.c

typedef struct {
    s16 x;
    s16 y;
    s16 unk4;
    u16 sequence;
    u32 unk8;
    u32 unkC;
    u32 surface;
    u32 charIndex;
    u32 paletteIndex;
    u32 cellAnimIndex;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 bgPriority;
    u32 unk30;
} MailBoxActorTemplate;

static void MailBox_InitClAct(MailBoxView *view);
static void MailBox_FreeClAct(MailBoxView *view);
static ClActor *MailBox_CreateActor(MailBoxView *view, const MailBoxActorTemplate *template);
static void MailBox_FreeActor(MailBoxView *view, u32 index);
static void MailBox_FreeActors(MailBoxView *view);
static void MailBox_OpenIconArc(MailBoxView *view);
static void MailBox_CloseIconArc(MailBoxView *view);
static void MailBox_LoadActorResources(MailBoxView *view);
static void MailBox_LoadButtonResources(MailBoxView *view);

static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 64, 4, 64, 0, 6, 4, 4, 32, 16, 16 };

static const MailBoxActorTemplate sActorTemplates[MAILBOX_ACTOR_COUNT] = {
    { 8, 168, 0, 4, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },   { 32, 168, 0, 5, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },
    { 64, 16, 0, 5, 0, 1, 0, 3, 1, 1, 0, 0, 0, 1, 0 },   { 224, 168, 0, 1, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },
    { 128, 160, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0 }, { 88, 160, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0 },
    { 48, 160, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 1, 0 },  { 188, 168, 0, 0, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },
};

void MailBox_CreateOBJs(MailBoxWork *work) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    for (i = 0; i < 8; i++) {
        work->view->chars[i] = 0;
        work->view->palettes[i] = 0;
        work->view->cellAnims[i] = 0;
    }
    MailBox_OpenIconArc(work->view);
    MailBox_InitClAct(work->view);
    MailBox_LoadButtonResources(work->view);
    MailBox_LoadActorResources(work->view);
    for (i = 0; i < 8; i++) {
        work->view->actors[i] = MailBox_CreateActor(work->view, &sActorTemplates[i]);
        func_0204c520(work->view->actors[i], TRUE);
    }
    if (work->view->mailCount <= MAILBOX_PAGE_SIZE) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    } else if (work->page == 0) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
    } else {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    }
    MailBox_SetActorVisible(work->view, 4, FALSE);
    MailBox_SetActorVisible(work->view, 5, FALSE);
    MailBox_SetActorVisible(work->view, 6, FALSE);
}

void MailBox_FreeOBJs(MailBoxWork *work) {
    MailBox_FreeActors(work->view);
    MailBox_FreeClAct(work->view);
    MailBox_CloseIconArc(work->view);
}

void MailBox_UpdateClActUnit(MailBoxView *view) {
    if (view->clActUnit != NULL) {
        func_0204b794();
    }
}

static void MailBox_InitClAct(MailBoxView *view) {
    ClActSys_Create(&sClActSetup, &MailBox_VRAMConfig, HEAPID_MAILBOX_VIEW);
    view->clActUnit = func_0204bf1c(8, 0, HEAPID_MAILBOX_VIEW);
    func_0204c028(view->clActUnit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MailBox_FreeClAct(MailBoxView *view) {
    // BUG: the palette and the cell animations are freed with each other's function. Both are resource 0, so it
    // works
#ifdef BUGFIX
    func_0204be64(view->cellAnims[0]);
    func_0204bcd0(view->palettes[0]);
#else
    func_0204be64(view->palettes[0]);
    func_0204bcd0(view->cellAnims[0]);
#endif
    func_0204bf98(view->clActUnit);
    func_0204b758();
}

static ClActor *MailBox_CreateActor(MailBoxView *view, const MailBoxActorTemplate *template) {
    ClActorSetup setup;

    setup.x = template->x;
    setup.y = template->y;
    setup.sequence = template->sequence;
    setup.priority = 0;
    setup.bgPriority = template->bgPriority;
    return func_0204c040(view->clActUnit, view->chars[template->charIndex], view->palettes[template->paletteIndex],
                         view->cellAnims[template->cellAnimIndex], &setup, template->surface,
                         HEAPID_MAILBOX_VIEW);
}

static void MailBox_FreeActor(MailBoxView *view, u32 index) {
    if (view->actors[index] != NULL) {
        func_0204c108(view->actors[index]);
        view->actors[index] = NULL;
    }
}

static void MailBox_FreeActors(MailBoxView *view) {
    u32 i;

    for (i = 0; i < MAILBOX_ACTOR_COUNT; i++) {
        MailBox_FreeActor(view, i);
    }
}

void MailBox_SetActorAnim(MailBoxView *view, u32 index, u32 sequence) {
    func_0204c488(view->actors[index], sequence);
}

void MailBox_SetActorVisible(MailBoxView *view, u32 index, BOOL visible) {
    func_0204c124(view->actors[index], visible);
}

void MailBox_SetActorPos(MailBoxView *view, u32 index, int x, int y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    if (y < 256) {
        func_0204c140(view->actors[index], &pos, 0);
    } else {
        func_0204c140(view->actors[index], &pos, 1);
    }
    func_0204c4d4(view->actors[index], 0);
}

void MailBox_SetActorBlend(MailBoxView *view, u32 index, BOOL blend) {
    if (blend == TRUE) {
        func_0204c318(view->actors[index], GX_OAM_MODE_XLU);
    } else {
        func_0204c318(view->actors[index], GX_OAM_MODE_NORMAL);
    }
}

static void MailBox_OpenIconArc(MailBoxView *view) {
    view->iconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_MAILBOX_VIEW);
}

static void MailBox_CloseIconArc(MailBoxView *view) {
    GFL_ArcToolFree(view->iconArc);
}

static void MailBox_LoadActorResources(MailBoxView *view) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_MAILBOX_VIEW);
    u32 i;

    for (i = 0; i <= 2; i++) {
        view->chars[i] = func_0204b81c(arc, func_0202d894(), FALSE, TRUE, HEAPID_MAILBOX_VIEW);
    }
    view->palettes[0] = func_0204bba0(arc, PokeIcon_GetPaletteFile(), 1, 0, HEAPID_MAILBOX_VIEW);
    view->cellAnims[0] = func_0204bde0(arc, func_0202d898(0), func_0202d89c(0), HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
}

static void MailBox_LoadButtonResources(MailBoxView *view) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MAILBOX, HEAPID_MAILBOX_VIEW);

    view->chars[3] = func_0204b81c(arc, 44, TRUE, 0, HEAPID_MAILBOX_VIEW);
    view->palettes[1] = func_0204bba0(arc, 47, 0, 0, HEAPID_MAILBOX_VIEW);
    view->cellAnims[1] = func_0204bde0(arc, 45, 46, HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_MAILBOX_VIEW);
    view->chars[4] = func_0204b81c(arc, func_0202d814(), FALSE, 0, HEAPID_MAILBOX_VIEW);
    view->palettes[2] = func_0204bba0(arc, func_0202d810(), 0, 0x80, HEAPID_MAILBOX_VIEW);
    view->cellAnims[2] = func_0204bde0(arc, func_0202d818(0), func_0202d81c(0), HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
}

void MailBox_ShowReadCursor(MailBoxView *view) {
    MailBox_SetActorPos(view, MAILBOX_ACTOR_CURSOR, 224, 176);
    MailBox_SetActorAnim(view, MAILBOX_ACTOR_CURSOR, 4);
}
