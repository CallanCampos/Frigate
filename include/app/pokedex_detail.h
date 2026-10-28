#ifndef POKEBW2_APP_POKEDEX_DETAIL_H
#define POKEBW2_APP_POKEDEX_DETAIL_H

#include "types.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The Pokédex's detail screens, overlay 298: a Pokémon's info, its habitat map, its cry and its forms, with the bars
// at the top and bottom of the screens to switch between them. Overlay 302, the Pokédex's list, runs it
#define OVERLAY_POKEDEX_DETAIL OVERLAY_ID(298)

// The pages
enum {
    POKEDEX_DETAIL_PAGE_NONE,
    POKEDEX_DETAIL_PAGE_INFO,
    POKEDEX_DETAIL_PAGE_MAP,
    POKEDEX_DETAIL_PAGE_VOICE,
    POKEDEX_DETAIL_PAGE_FORM,
};

// What the screen returns to the list
enum {
    // The player picked a place on the habitat map
    POKEDEX_DETAIL_RESULT_PLACE,
    // Close the Pokédex, or return to its list
    POKEDEX_DETAIL_RESULT_CLOSE,
    POKEDEX_DETAIL_RESULT_RETURN,
};

typedef struct {
    GameData *gameData;
    // The POKEDEX_DETAIL_PAGE_* to start on
    int page;
    // The Pokémon that the screen pages through, and where in the list it is
    const u16 *list;
    u16 count;
    u16 index;
    // A POKEDEX_DETAIL_RESULT_*
    int result;
    u32 mode;
    u16 unk18;
    // With POKEDEX_DETAIL_RESULT_PLACE
    u16 place;
} PokedexDetailParam;

extern const GameProcFunctions POKEDEX_DETAIL_PROC_FUNCTIONS;

typedef struct PokedexDetailProcSys PokedexDetailProcSys;
typedef struct PokedexDetailCommon PokedexDetailCommon;
typedef struct PokedexDetailBlend PokedexDetailBlend;
typedef struct PokedexDetailPalFade PokedexDetailPalFade;
typedef struct PokedexDetailBackground PokedexDetailBackground;
typedef struct PokedexDetailGraphic PokedexDetailGraphic;
typedef struct PokedexDetailTouchbar PokedexDetailTouchbar;
typedef struct PokedexDetailHeadbar PokedexDetailHeadbar;

// pokedex_detail_procsys.c: runs one of the pages

typedef BOOL (*PokedexDetailProcFunc)(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                      PokedexDetailCommon *common);
typedef void (*PokedexDetailProcCommandFunc)(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                             PokedexDetailCommon *common, int command);
typedef void (*PokedexDetailProcDrawFunc)(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                          PokedexDetailCommon *common);

typedef struct {
    PokedexDetailProcFunc init;
    PokedexDetailProcFunc main;
    PokedexDetailProcFunc exit;
    // Called every frame with the touch bar's command
    PokedexDetailProcCommandFunc command;
    PokedexDetailProcDrawFunc draw;
} PokedexDetailProcFuncs;

struct PokedexDetailProcSys {
    const PokedexDetailProcFuncs *funcs;
    int seq;
    int subSeq;
    void *param;
    void *work;
};

PokedexDetailProcSys *PokedexDetailProcSys_Create(const PokedexDetailProcFuncs *funcs, void *param, HeapID heapId);
void PokedexDetailProcSys_Free(PokedexDetailProcSys *sys);
// Returns TRUE once the page has exited
BOOL PokedexDetailProcSys_Main(PokedexDetailProcSys *sys, PokedexDetailCommon *common);
void PokedexDetailProcSys_Command(PokedexDetailProcSys *sys, PokedexDetailCommon *common, int command);
void PokedexDetailProcSys_Draw(PokedexDetailProcSys *sys, PokedexDetailCommon *common);
void *PokedexDetailProcSys_AllocWork(PokedexDetailProcSys *sys, u32 size, HeapID heapId);
void PokedexDetailProcSys_FreeWork(PokedexDetailProcSys *sys);

// pokedex_detail_common.c: what the pages share

struct PokedexDetailCommon {
    GameData *gameData;
    PokedexDetailGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    PokedexDetailTouchbar *touchbar;
    PokedexDetailHeadbar *headbar;
    // The Pokémon that the screen pages through, and where in the list it is
    const u16 *list;
    u16 count;
    u16 *index;
};

PokedexDetailCommon *PokedexDetailCommon_Create(HeapID heapId, GameData *gameData, PokedexDetailGraphic *graphic,
                                                Font *font, PrintQueue *printQueue, PokedexDetailTouchbar *touchbar,
                                                PokedexDetailHeadbar *headbar, const u16 *list, u16 count, u16 *index);
void PokedexDetailCommon_Free(PokedexDetailCommon *common);
GameData *PokedexDetailCommon_GetGameData(PokedexDetailCommon *common);
PokedexDetailGraphic *PokedexDetailCommon_GetGraphic(PokedexDetailCommon *common);
Font *PokedexDetailCommon_GetFont(PokedexDetailCommon *common);
PrintQueue *PokedexDetailCommon_GetPrintQueue(PokedexDetailCommon *common);
PokedexDetailTouchbar *PokedexDetailCommon_GetTouchbar(PokedexDetailCommon *common);
PokedexDetailHeadbar *PokedexDetailCommon_GetHeadbar(PokedexDetailCommon *common);
u16 PokedexDetailCommon_GetCount(PokedexDetailCommon *common);
u16 PokedexDetailCommon_GetSpecies(PokedexDetailCommon *common);
void PokedexDetailCommon_GoNext(PokedexDetailCommon *common);
void PokedexDetailCommon_GoPrev(PokedexDetailCommon *common);

// A screen's blending, which fades layers in or out by 2 of 16 every frame
struct PokedexDetailBlend {
    u32 plane1;
    u32 plane2;
    int ev;
    int step;
    int wait;
};

PokedexDetailBlend *PokedexDetailBlend_Create(HeapID heapId);
void PokedexDetailBlend_Free(PokedexDetailBlend *blend);
void PokedexDetailBlend_Update(PokedexDetailBlend *main, PokedexDetailBlend *sub);
BOOL PokedexDetailBlend_IsActive(PokedexDetailBlend *blend);
void PokedexDetailBlend_StartIn(PokedexDetailBlend *blend);
void PokedexDetailBlend_StartOut(PokedexDetailBlend *blend);
// screen is 0 for the main screen and 1 for the sub screen
void PokedexDetailBlend_SetIn(u32 screen, PokedexDetailBlend *blend);
void PokedexDetailBlend_SetOut(u32 screen, PokedexDetailBlend *blend);
void PokedexDetailBlend_InitPlanes(PokedexDetailBlend *blend);

// Fades the palettes of the screens to black and back
enum {
    POKEDEX_DETAIL_PALFADE_MAIN_BG = 1 << 0,
    POKEDEX_DETAIL_PALFADE_SUB_BG = 1 << 1,
    POKEDEX_DETAIL_PALFADE_MAIN_OBJ = 1 << 2,
    POKEDEX_DETAIL_PALFADE_SUB_OBJ = 1 << 3,
};

enum {
    POKEDEX_DETAIL_PALFADE_SHOWN,
    POKEDEX_DETAIL_PALFADE_HIDDEN,
    POKEDEX_DETAIL_PALFADE_FADING_IN,
    POKEDEX_DETAIL_PALFADE_FADING_OUT,
};

struct PokedexDetailPalFade {
    TCBManager *tcbMgr;
    void *tcbBuffer;
    PaletteFade *palette;
    // The POKEDEX_DETAIL_PALFADE_* palettes that it fades
    u16 buffers;
    int state;
};

PokedexDetailPalFade *PokedexDetailPalFade_CreateEx(HeapID heapId, u16 buffers);
PokedexDetailPalFade *PokedexDetailPalFade_Create(HeapID heapId);
void PokedexDetailPalFade_Free(PokedexDetailPalFade *fade);
void PokedexDetailPalFade_Update(PokedexDetailPalFade *fade);
void PokedexDetailPalFade_VBlank(PokedexDetailPalFade *fade);
BOOL PokedexDetailPalFade_IsFading(PokedexDetailPalFade *fade);
void PokedexDetailPalFade_StartIn(PokedexDetailPalFade *fade);
void PokedexDetailPalFade_StartOut(PokedexDetailPalFade *fade);
void PokedexDetailPalFade_SetHidden(PokedexDetailPalFade *fade);
void PokedexDetailPalFade_LoadPalette(PokedexDetailPalFade *fade, ArcTool *arc, u32 fileId, HeapID heapId, u32 buffer,
                                      u32 size, u16 offset, u16 srcOffset);
void PokedexDetailPalFade_ReadPalettes(PokedexDetailPalFade *fade);

// A page's scrolling background
struct PokedexDetailBackground {
    u32 chars;
    u8 bg;
    u8 wait;
};

PokedexDetailBackground *PokedexDetailBackground_Create(HeapID heapId, u32 page, u8 bg, u8 bottomPalette,
                                                        u8 topPalette);
void PokedexDetailBackground_Free(PokedexDetailBackground *background);
void PokedexDetailBackground_Update(PokedexDetailBackground *background);
u32 PokedexDetail_LoadBG(BOOL loaded, HeapID heapId, u8 bg, u32 palettes, u8 palette, u8 srcPalette, u32 arcId,
                         u32 nclr, u32 ncgr, u32 nscr, u32 chars);
void PokedexDetail_FreeBG(u32 bg, u32 chars);

// pokedex_detail_graphic.c: the screens' BGs, OBJs and 3D

// A BG to create, as the graphics and the map page set them up
typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PokedexDetailBGSetup;

// A window to create, as BmpWin_CreateDynamic takes it, in the pages' tables
typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u8 fromEnd;
} PokedexDetailWindowData;

PokedexDetailGraphic *PokedexDetailGraphic_Create(u32 layout, HeapID heapId, BOOL with3D);
void PokedexDetailGraphic_Free(PokedexDetailGraphic *graphic);
void PokedexDetailGraphic_Update(PokedexDetailGraphic *graphic);
// Draw the 3D system's frame, if it has been created
void PokedexDetailGraphic_Begin3D(PokedexDetailGraphic *graphic);
void PokedexDetailGraphic_End3D(PokedexDetailGraphic *graphic);
ClActUnit *PokedexDetailGraphic_GetClActUnit(PokedexDetailGraphic *graphic);
void PokedexDetailGraphic_Create3D(PokedexDetailGraphic *graphic, HeapID heapId);
void PokedexDetailGraphic_Free3D(PokedexDetailGraphic *graphic);

// pokedex_detail_tbar.c: the Pokédex's copy of the touch bar, a bar of icons at the bottom of a screen that are touched
// or pressed with a key

typedef struct PokedexDetailTbar PokedexDetailTbar;

// The icons that the bar draws itself. The others are the app's own
enum {
    POKEDEX_DETAIL_TBAR_ICON_CLOSE,
    POKEDEX_DETAIL_TBAR_ICON_RETURN,
    POKEDEX_DETAIL_TBAR_ICON_CUR_D,
    POKEDEX_DETAIL_TBAR_ICON_CUR_U,
    POKEDEX_DETAIL_TBAR_ICON_CUR_L,
    POKEDEX_DETAIL_TBAR_ICON_CUR_R,
    POKEDEX_DETAIL_TBAR_ICON_CHECK,
    POKEDEX_DETAIL_TBAR_ICON_CUSTOM,
};

// What touching an icon does: plays its pushed animation, or flips it on or off
enum {
    POKEDEX_DETAIL_TBAR_TYPE_PUSH,
    POKEDEX_DETAIL_TBAR_TYPE_FLIP,
};

typedef struct {
    int icon;
    ClActorPos pos;
    u16 width;
    // For a POKEDEX_DETAIL_TBAR_ICON_CUSTOM icon, its resources and animations, its key and its sound
    u16 chars;
    u16 palette;
    u16 cellAnims;
    u16 activeAnim;
    u16 inactiveAnim;
    u16 pushedAnim;
    u32 key;
    u32 se;
} PokedexDetailTbarIcon;

typedef struct {
    const PokedexDetailTbarIcon *icons;
    u32 iconCount;
    ClActUnit *unit;
    // The bar's BG, the palettes for the bar and its icons, and the OBJ VRAM mapping mode
    u32 bg;
    u32 bgPalette;
    u32 objPalette;
    u32 mapping;
    // Whether the app has loaded the bar's BG itself
    BOOL noBG;
} PokedexDetailTbarParam;

PokedexDetailTbar *PokedexDetailTbar_Create(PokedexDetailTbarParam *param, HeapID heapId);
void PokedexDetailTbar_Free(PokedexDetailTbar *tbar);
void PokedexDetailTbar_Main(PokedexDetailTbar *tbar);
// The icon that was touched, once its animation has played, and as it is touched, or -1
int PokedexDetailTbar_GetTrigger(PokedexDetailTbar *tbar);
int PokedexDetailTbar_GetTouch(PokedexDetailTbar *tbar);
void PokedexDetailTbar_SetVisibleAll(PokedexDetailTbar *tbar, BOOL visible);
void PokedexDetailTbar_SetActiveAll(PokedexDetailTbar *tbar, BOOL active);
BOOL PokedexDetailTbar_GetActiveAll(PokedexDetailTbar *tbar);
void PokedexDetailTbar_Unlock(PokedexDetailTbar *tbar);
void PokedexDetailTbar_SetActive(PokedexDetailTbar *tbar, int icon, BOOL active);
void PokedexDetailTbar_SetVisible(PokedexDetailTbar *tbar, int icon, BOOL visible);
BOOL PokedexDetailTbar_GetVisible(PokedexDetailTbar *tbar, int icon);
void PokedexDetailTbar_SetKey(PokedexDetailTbar *tbar, int icon, u32 key);
void PokedexDetailTbar_SetFlip(PokedexDetailTbar *tbar, int icon, BOOL flip);
BOOL PokedexDetailTbar_GetFlip(PokedexDetailTbar *tbar, int icon);
ClActor *PokedexDetailTbar_GetActor(PokedexDetailTbar *tbar, int icon);
void PokedexDetailTbar_SetPos(PokedexDetailTbar *tbar, int icon, const ClActorPos *pos);
// Acts as if the icon were touched
void PokedexDetailTbar_Push(PokedexDetailTbar *tbar, int icon);
BOOL PokedexDetailTbar_IsTriggered(PokedexDetailTbar *tbar, int icon);

// pokedex_detail_touchbar.c: the bar at the bottom of the touch screen

// The commands of the bar's icons: the first once the icon's animation has played, the _TOUCH ones as it is touched
enum {
    POKEDEX_DETAIL_CMD_NONE,
    POKEDEX_DETAIL_CMD_CLOSE,
    POKEDEX_DETAIL_CMD_RETURN,
    POKEDEX_DETAIL_CMD_CUR_D,
    POKEDEX_DETAIL_CMD_CUR_U,
    POKEDEX_DETAIL_CMD_CHECK,
    POKEDEX_DETAIL_CMD_INFO,
    POKEDEX_DETAIL_CMD_MAP,
    POKEDEX_DETAIL_CMD_VOICE,
    POKEDEX_DETAIL_CMD_FORM,
    POKEDEX_DETAIL_CMD_CLOSE_TOUCH,
    POKEDEX_DETAIL_CMD_RETURN_TOUCH,
    POKEDEX_DETAIL_CMD_CUR_D_TOUCH,
    POKEDEX_DETAIL_CMD_CUR_U_TOUCH,
    POKEDEX_DETAIL_CMD_CHECK_TOUCH,
    POKEDEX_DETAIL_CMD_INFO_TOUCH,
    POKEDEX_DETAIL_CMD_MAP_TOUCH,
    POKEDEX_DETAIL_CMD_VOICE_TOUCH,
    POKEDEX_DETAIL_CMD_FORM_TOUCH,
    // The map's and the forms' bars
    POKEDEX_DETAIL_CMD_MAP_RETURN,
    POKEDEX_DETAIL_CMD_MAP_RETURN_TOUCH,
    POKEDEX_DETAIL_CMD_FORM_RETURN,
    POKEDEX_DETAIL_CMD_FORM_CUR_R,
    POKEDEX_DETAIL_CMD_FORM_CUR_L,
    POKEDEX_DETAIL_CMD_FORM_CUR_D,
    POKEDEX_DETAIL_CMD_FORM_CUR_U,
    POKEDEX_DETAIL_CMD_FORM_BUTTON,
    POKEDEX_DETAIL_CMD_MAP_PLACE,
    POKEDEX_DETAIL_CMD_FORM_RETURN_TOUCH,
    POKEDEX_DETAIL_CMD_FORM_CUR_R_TOUCH,
    POKEDEX_DETAIL_CMD_FORM_CUR_L_TOUCH,
    POKEDEX_DETAIL_CMD_FORM_CUR_D_TOUCH,
    POKEDEX_DETAIL_CMD_FORM_CUR_U_TOUCH,
    POKEDEX_DETAIL_CMD_FORM_BUTTON_TOUCH,
    POKEDEX_DETAIL_CMD_MAP_PLACE_TOUCH,
};

// The bar's icons: the pages' tabs, or the map's or the forms' buttons
enum {
    POKEDEX_DETAIL_TOUCHBAR_GENERAL,
    POKEDEX_DETAIL_TOUCHBAR_MAP,
    POKEDEX_DETAIL_TOUCHBAR_FORM,
};

// PokedexDetailTouchbar_GetState
enum {
    POKEDEX_DETAIL_TOUCHBAR_HIDDEN,
    POKEDEX_DETAIL_TOUCHBAR_APPEARING,
    POKEDEX_DETAIL_TOUCHBAR_SHOWN,
    POKEDEX_DETAIL_TOUCHBAR_DISAPPEARING,
};

// showFormTab is a setting of the Pokédex's save (PokeDex_IsGiven), and mode the screen's PokedexDetailParam mode,
// which hides the map's tab and the check box
PokedexDetailTouchbar *PokedexDetailTouchbar_Create(HeapID heapId, BOOL showFormTab, u32 mode);
void PokedexDetailTouchbar_Free(PokedexDetailTouchbar *touchbar);
void PokedexDetailTouchbar_Update(PokedexDetailTouchbar *touchbar);
// page is a POKEDEX_DETAIL_PAGE_* less 1
void PokedexDetailTouchbar_SetType(PokedexDetailTouchbar *touchbar, int type, int page, BOOL showArrows);
int PokedexDetailTouchbar_GetState(PokedexDetailTouchbar *touchbar);
// Slides the bar in or out
void PokedexDetailTouchbar_Appear(PokedexDetailTouchbar *touchbar, u32 speed);
void PokedexDetailTouchbar_Disappear(PokedexDetailTouchbar *touchbar, u32 speed);
int PokedexDetailTouchbar_GetTrigger(PokedexDetailTouchbar *touchbar);
int PokedexDetailTouchbar_GetTouch(PokedexDetailTouchbar *touchbar);
void PokedexDetailTouchbar_Unlock(PokedexDetailTouchbar *touchbar);
void PokedexDetailTouchbar_SetVisibleAll(PokedexDetailTouchbar *touchbar, BOOL visible);
void PokedexDetailTouchbar_SetPage(PokedexDetailTouchbar *touchbar, int page);
void PokedexDetailTouchbar_SetFormArrowsVisible(PokedexDetailTouchbar *touchbar, BOOL visible);
void PokedexDetailTouchbar_SetCheck(PokedexDetailTouchbar *touchbar, BOOL check);
BOOL PokedexDetailTouchbar_GetCheck(PokedexDetailTouchbar *touchbar);
void PokedexDetailTouchbar_SetActive(PokedexDetailTouchbar *touchbar, BOOL active);
u32 PokedexDetailTouchbar_GetIconPalette(PokedexDetailTouchbar *touchbar);
void PokedexDetailTouchbar_SetBGPriority(PokedexDetailTouchbar *touchbar, u8 priority);
void PokedexDetailTouchbar_SetMapPlaceActive(PokedexDetailTouchbar *touchbar, BOOL active);
void PokedexDetailTouchbar_SetMapPlaceVisible(PokedexDetailTouchbar *touchbar, BOOL visible);
void PokedexDetailTouchbar_PushMapPlace(PokedexDetailTouchbar *touchbar);
BOOL PokedexDetailTouchbar_IsMapPlaceTriggered(PokedexDetailTouchbar *touchbar);
BOOL PokedexDetailTouchbar_IsArrowTriggered(PokedexDetailTouchbar *touchbar);
BOOL PokedexDetailTouchbar_IsFormButtonTriggered(PokedexDetailTouchbar *touchbar);

// pokedex_detail_headbar.c: the bar at the top of the screen

// The titles, messages 179 on of system message file 441
#define POKEDEX_DETAIL_HEADBAR_TITLE_COUNT 5

PokedexDetailHeadbar *PokedexDetailHeadbar_Create(HeapID heapId, Font *font);
void PokedexDetailHeadbar_Free(PokedexDetailHeadbar *headbar);
void PokedexDetailHeadbar_Update(PokedexDetailHeadbar *headbar);
void PokedexDetailHeadbar_SetTitle(PokedexDetailHeadbar *headbar, int title);
// The same states as the touch bar's
int PokedexDetailHeadbar_GetState(PokedexDetailHeadbar *headbar);
// Slides the bar in or out
void PokedexDetailHeadbar_Appear(PokedexDetailHeadbar *headbar);
void PokedexDetailHeadbar_Disappear(PokedexDetailHeadbar *headbar);

// pokedex_detail_info.c, a guessed name, as the ROM names neither this file nor the map's: the Pokémon's info

typedef struct {
    HeapID heapId;
} PokedexDetailInfoParam;

extern const PokedexDetailProcFuncs POKEDEX_DETAIL_INFO_PROC_FUNCS;

void PokedexDetailInfo_InitParam(PokedexDetailInfoParam *param, HeapID heapId);

// pokedex_detail_map.c, a guessed name: where the Pokémon lives

typedef struct {
    HeapID heapId;
    // The zone of the place picked on the map for the habitat list, or ZONE_MAX
    u16 place;
} PokedexDetailMapParam;

extern const PokedexDetailProcFuncs POKEDEX_DETAIL_MAP_PROC_FUNCS;

void PokedexDetailMap_InitParam(PokedexDetailMapParam *param, HeapID heapId);

// pokedex_detail_voice.c: the Pokémon's cry

typedef struct {
    HeapID heapId;
} PokedexDetailVoiceParam;

extern const PokedexDetailProcFuncs POKEDEX_DETAIL_VOICE_PROC_FUNCS;

void PokedexDetailVoice_InitParam(PokedexDetailVoiceParam *param, HeapID heapId);

// pokedex_detail_form.c: the Pokémon's forms

typedef struct {
    HeapID heapId;
} PokedexDetailFormParam;

extern const PokedexDetailProcFuncs POKEDEX_DETAIL_FORM_PROC_FUNCS;

// pokedex_detail_form_data.c: the forms page's tables that the linker placed ahead of its file

// The resources of the forms page's actors and buttons
enum {
    POKEDEX_DETAIL_FORM_RES_MAIN_CHARS,
    POKEDEX_DETAIL_FORM_RES_MAIN_PALETTE,
    POKEDEX_DETAIL_FORM_RES_MAIN_CELL_ANIMS,
    POKEDEX_DETAIL_FORM_RES_COLOR_CHARS,
    POKEDEX_DETAIL_FORM_RES_COLOR_PALETTE,
    POKEDEX_DETAIL_FORM_RES_COLOR_CELL_ANIMS,
    POKEDEX_DETAIL_FORM_RES_ARROW_CHARS,
    POKEDEX_DETAIL_FORM_RES_ARROW_PALETTE,
    POKEDEX_DETAIL_FORM_RES_ARROW_CELL_ANIMS,
    POKEDEX_DETAIL_FORM_RES_COUNT,
};

typedef struct {
    f32 x;
    f32 y;
    f32 z;
} PokedexDetailFormPos;

typedef struct {
    u8 x;
    u8 y;
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    // POKEDEX_DETAIL_FORM_RES_*
    u8 chars;
    u8 palette;
    u8 cellAnims;
} PokedexDetailFormActorData;

extern const u16 POKEDEX_DETAIL_FORM_STRBUF_MESSAGES[];
extern const PokedexDetailFormActorData POKEDEX_DETAIL_FORM_ACTORS[];
extern const PokedexDetailFormPos POKEDEX_DETAIL_FORM_DEFAULT_POSITIONS[];

void PokedexDetailForm_InitParam(PokedexDetailFormParam *param, HeapID heapId);

#endif // POKEBW2_APP_POKEDEX_DETAIL_H
