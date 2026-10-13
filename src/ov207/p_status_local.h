#ifndef POKEBW2_OV207_P_STATUS_LOCAL_H
#define POKEBW2_OV207_P_STATUS_LOCAL_H

#include "types.h"
#include "app/p_status.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/msg.h"
#include "gfl/tcb.h"
#include "struct_decls.h"
#include "system/gf_font.h"
#include "system/mcss.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The summary screen's work and the functions its files share. The ROM names p_status.c and the pages' files
// (p_sta_sub.c, p_sta_info.c, p_sta_ribbon.c, p_sta_skill.c and p_sta_oam.c); p_sta_sys.c, the screen's core, and
// ribbon.c, the ribbon table, are guessed names. So are the names of the fields and functions

// The palettes, characters and cell animations in PStatusWork's clResources
#define PSTA_RES_PLTT(i) (i)
#define PSTA_RES_CHAR(i) (12 + (i))
#define PSTA_RES_CELL(i) (25 + (i))
#define PSTA_RES_COUNT 38

#define PSTA_TYPE_COUNT 17

// PStatusWork.shownPage before the first page is drawn
#define PSTA_PAGE_NONE 4

// Which way PStatus_FindPokemon looks
enum {
    PSTA_DIR_UP,
    PSTA_DIR_DOWN,
};

// The buttons of the bottom screen
enum {
    PSTA_BUTTON_INFO,
    PSTA_BUTTON_SKILL,
    PSTA_BUTTON_RIBBON,
    // Registers the page to the shortcut menu
    PSTA_BUTTON_SHORTCUT,
    PSTA_BUTTON_UP,
    PSTA_BUTTON_DOWN,
    PSTA_BUTTON_CLOSE,
    PSTA_BUTTON_BACK,
    PSTA_BUTTON_COUNT,
};

// What the proc's main function does
enum {
    PSTA_SEQ_FADE_IN,
    PSTA_SEQ_WAIT_FADE_IN,
    PSTA_SEQ_EXIT,
    PSTA_SEQ_WAIT_FADE_OUT,
    PSTA_SEQ_MAIN,
};

// The mosaic that hides a change of page or Pokémon
enum {
    PSTA_MOSAIC_NONE,
    PSTA_MOSAIC_IN,
    PSTA_MOSAIC_REDRAW,
    PSTA_MOSAIC_OUT,
};

// A screen file and its data
typedef struct {
    NNSG2dScreenData *screen;
    void *file;
} PStaScreen;

// A text window of a page, in tiles
typedef struct {
    u8 x;
    u8 y;
    u8 width;
    u8 height;
} PStaWindowSetup;

// What PStaOam_CreateActor makes the sprites of a bitmap from: the bitmap is cut into 64x32 actors, which take the
// palette at its offset and the surface's OBJ mapping
typedef struct {
    GFLBitmap *bitmap;
    s16 x;
    s16 y;
    u32 palette;
    u32 paletteOffset;
    u8 priority;
    u8 bgPriority;
    u16 surface;
    // CLACT_VRAM_MAIN or CLACT_VRAM_SUB
    u32 vramType;
} PStaOamSetup;

struct PStatusWork {
    HeapID heapId;
    TCB *vblankTcb;
    PStatusParam *param;
    BOOL isRedrawing;
    BOOL playCry;
    BOOL canForgetHm;
    u16 partyIndex;
    // The Pokémon on screen, 0xff for none
    u8 shownPartyIndex;
    u8 bgScrollFrames;
    u8 isTouch;
    u32 lastVBlankCount;
    s32 touchHit;
    u32 touchX;
    u32 touchY;
    u32 prevTouchX;
    u32 prevTouchY;
    BOOL isEgg;
    u32 happiness;
    BOOL hasRibbon;
    BOOL isInputEnabled;
    BOOL upPressed;
    BOOL downPressed;
    int seq;
    int exitResult;
    int page;
    int shownPage;
    BOOL shortcutRegistered[3];
    u32 unk6C;
    G3DCamera *camera;
    u32 unk74;
    MCSSSystem *mcssSys;
    MsgData *msgData;
    Font *font;
    PrintQueue *printQueue;
    PStaSubWork *sub;
    PStaInfoWork *info;
    PStaSkillWork *skill;
    PStaRibbonWork *ribbon;
    u32 clResources[PSTA_RES_COUNT];
    u32 typeIconChars[PSTA_TYPE_COUNT];
    ClActUnit *actorUnit;
    ClActor *buttons[PSTA_BUTTON_COUNT];
    ClActor *typeIcons[2];
    ClActor *pressedButton;
    u16 plttAnimPhase;
    // Six colors are copied here from OBJ palette RAM at the start, which runs over into cursorPltt. Never read
    u16 cursorPlttSrc[4];
    // The cursor's four animated colors
    u16 cursorPltt[4];
    u16 tabPlttSrc[2][16];
    u16 tabPltt[16];
    u16 buttonPltt[16];
    u16 buttonPlttSrc[16];
    // A party copy of the boxed Pokémon shown
    PartyPkm *boxPartyPkm;
    int mosaicState;
    u8 mosaicLevel;
};

// p_sta_sys.c
BOOL PStatus_Init(PStatusWork *wk);
BOOL PStatus_Exit(PStatusWork *wk);
int PStatus_Main(PStatusWork *wk);
void PStatus_EnableInput(PStatusWork *wk, BOOL enable);
BoxPkm *PStatus_GetBoxPkm(PStatusWork *wk);
PartyPkm *PStatus_GetPartyPkm(PStatusWork *wk);
void PStatus_SetDecrypted(PStatusWork *wk, BOOL decrypt);
void PStatus_Print(PStatusWork *wk, GFLBitmap *bitmap, u32 msgId, u16 x, u16 y, u16 color);
void PStatus_PrintToWindow(PStatusWork *wk, BmpWin *window, u32 msgId, u16 x, u16 y, u16 color);
void PStatus_PrintCentered(PStatusWork *wk, GFLBitmap *bitmap, u32 msgId, u16 x, u16 y, u16 color);
void PStatus_PrintFormatted(PStatusWork *wk, GFLBitmap *bitmap, WordSet *wordSet, u32 msgId, u16 x, u16 y, u16 color);
void PStatus_PrintFormattedToWindow(PStatusWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, u16 x, u16 y,
                                    u16 color);
void PStatus_PrintFormattedRight(PStatusWork *wk, GFLBitmap *bitmap, WordSet *wordSet, u32 msgId, u16 x, u16 y,
                                 u16 color);
void PStatus_PrintFormattedRightToWindow(PStatusWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, u16 x, u16 y,
                                         u16 color);

// p_sta_sub.c
PStaSubWork *PStaSub_Create(PStatusWork *wk);
void PStaSub_Free(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_Main(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_LoadResources(PStatusWork *wk, PStaSubWork *sub, ArcTool *arc);
void PStaSub_FreeResources(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_CreateActors(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_FreeActors(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_Load(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_Draw(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_Unload(PStatusWork *wk, PStaSubWork *sub);
void PStaSub_Clear(PStatusWork *wk, PStaSubWork *sub);

// p_sta_info.c
PStaInfoWork *PStaInfo_Create(PStatusWork *wk);
void PStaInfo_Free(PStatusWork *wk, PStaInfoWork *info);
void PStaInfo_Main(PStatusWork *wk, PStaInfoWork *info);
void PStaInfo_LoadResources(PStatusWork *wk, PStaInfoWork *info, ArcTool *arc);
void PStaInfo_FreeResources(PStatusWork *wk, PStaInfoWork *info);
void PStaInfo_Load(PStatusWork *wk, PStaInfoWork *info);
void PStaInfo_Draw(PStatusWork *wk, PStaInfoWork *info);
void PStaInfo_Unload(PStatusWork *wk, PStaInfoWork *info);
void PStaInfo_Clear(PStatusWork *wk, PStaInfoWork *info);

// p_sta_ribbon.c
PStaRibbonWork *PStaRibbon_Create(PStatusWork *wk);
void PStaRibbon_Free(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_Main(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_LoadResources(PStatusWork *wk, PStaRibbonWork *ribbon, ArcTool *arc);
void PStaRibbon_FreeResources(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_CreateActors(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_FreeActors(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_Load(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_Draw(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_Unload(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_Clear(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_LoadPokemon(PStatusWork *wk, PStaRibbonWork *ribbon);
void PStaRibbon_UnloadPokemon(PStatusWork *wk, PStaRibbonWork *ribbon);

// p_sta_skill.c
PStaSkillWork *PStaSkill_Create(PStatusWork *wk);
void PStaSkill_Free(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_Main(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_LoadResources(PStatusWork *wk, PStaSkillWork *skill, ArcTool *arc);
void PStaSkill_FreeResources(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_CreateActors(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_FreeActors(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_Load(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_Draw(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_Unload(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_Clear(PStatusWork *wk, PStaSkillWork *skill);
// The same for the page that picks a move to forget
void PStaSkill_LoadForget(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_DrawForget(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_UnloadForget(PStatusWork *wk, PStaSkillWork *skill);
void PStaSkill_ClearForget(PStatusWork *wk, PStaSkillWork *skill);

// p_sta_oam.c
PStaOam *PStaOam_Create(HeapID heapId, ClActUnit *unit);
void PStaOam_Free(PStaOam *oam);
PStaOamActor *PStaOam_CreateActor(PStaOam *oam, const PStaOamSetup *setup);
void PStaOam_FreeActor(PStaOamActor *actor);
void PStaOam_SetVisible(PStaOamActor *actor, BOOL visible);
void PStaOam_Upload(PStaOamActor *actor);
void PStaOam_SetPosition(PStaOamActor *actor, s16 x, s16 y);
void PStaOam_SwapBitmaps(PStaOamActor *a, PStaOamActor *b);

// ribbon.c: the table of the 80 ribbons
enum {
    RIBBON_DATA_PARAM,
    RIBBON_DATA_ICON,
    RIBBON_DATA_PALETTE,
    RIBBON_DATA_NAME,
    RIBBON_DATA_DESCRIPTION,
    RIBBON_DATA_CATEGORY,
};

#define RIBBON_COUNT 80

// The groups that number the ribbons, named after their headings in the ribbon text, which follow the descriptions
enum {
    RIBBON_CATEGORY_LEAGUE,
    RIBBON_CATEGORY_CONTEST,
    RIBBON_CATEGORY_TOWER,
    RIBBON_CATEGORY_MEMORIAL,
    RIBBON_CATEGORY_GIFT,
    RIBBON_CATEGORY_COUNT,
};

u32 Ribbon_GetData(u32 ribbon, u32 field);
u32 Ribbon_GetDescription(u32 ribbon);

#endif // POKEBW2_OV207_P_STATUS_LOCAL_H
