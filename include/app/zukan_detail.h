#ifndef POKEBW2_APP_ZUKAN_DETAIL_H
#define POKEBW2_APP_ZUKAN_DETAIL_H

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
#define OVERLAY_ZUKAN_DETAIL OVERLAY_ID(298)

// The pages
enum {
    ZUKAN_DETAIL_PAGE_NONE,
    ZUKAN_DETAIL_PAGE_INFO,
    ZUKAN_DETAIL_PAGE_MAP,
    ZUKAN_DETAIL_PAGE_VOICE,
    ZUKAN_DETAIL_PAGE_FORM,
};

// What the screen returns to the list
enum {
    // The player picked a place on the habitat map
    ZUKAN_DETAIL_RESULT_PLACE,
    // Close the Pokédex, or return to its list
    ZUKAN_DETAIL_RESULT_CLOSE,
    ZUKAN_DETAIL_RESULT_RETURN,
};

typedef struct {
    GameData *gameData;
    // The ZUKAN_DETAIL_PAGE_* to start on
    int page;
    // The Pokémon that the screen pages through, and where in the list it is
    const u16 *list;
    u16 count;
    u16 index;
    // A ZUKAN_DETAIL_RESULT_*
    int result;
    u32 mode;
    u16 unk18;
    // With ZUKAN_DETAIL_RESULT_PLACE
    u16 place;
} ZukanDetailParam;

extern const GameProcFunctions ZUKAN_DETAIL_PROC_FUNCTIONS;

typedef struct ZukanDetailProcSys ZukanDetailProcSys;
typedef struct ZukanDetailCommon ZukanDetailCommon;
typedef struct ZukanDetailBlend ZukanDetailBlend;
typedef struct ZukanDetailPalFade ZukanDetailPalFade;
typedef struct ZukanDetailBackground ZukanDetailBackground;
typedef struct ZukanDetailGraphic ZukanDetailGraphic;
typedef struct ZukanDetailTouchbar ZukanDetailTouchbar;
typedef struct ZukanDetailHeadbar ZukanDetailHeadbar;

// zukan_detail_procsys.c: runs one of the pages

typedef BOOL (*ZukanDetailProcFunc)(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                    ZukanDetailCommon *common);
typedef void (*ZukanDetailProcCommandFunc)(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                           ZukanDetailCommon *common, int command);
typedef void (*ZukanDetailProcDrawFunc)(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                        ZukanDetailCommon *common);

typedef struct {
    ZukanDetailProcFunc init;
    ZukanDetailProcFunc main;
    ZukanDetailProcFunc exit;
    // Called every frame with the touch bar's command
    ZukanDetailProcCommandFunc command;
    ZukanDetailProcDrawFunc draw;
} ZukanDetailProcFuncs;

struct ZukanDetailProcSys {
    const ZukanDetailProcFuncs *funcs;
    int seq;
    int subSeq;
    void *param;
    void *work;
};

ZukanDetailProcSys *ZukanDetailProcSys_Create(const ZukanDetailProcFuncs *funcs, void *param, HeapID heapId);
void ZukanDetailProcSys_Free(ZukanDetailProcSys *sys);
// Returns TRUE once the page has exited
BOOL ZukanDetailProcSys_Main(ZukanDetailProcSys *sys, ZukanDetailCommon *common);
void ZukanDetailProcSys_Command(ZukanDetailProcSys *sys, ZukanDetailCommon *common, int command);
void ZukanDetailProcSys_Draw(ZukanDetailProcSys *sys, ZukanDetailCommon *common);
void *ZukanDetailProcSys_AllocWork(ZukanDetailProcSys *sys, u32 size, HeapID heapId);
void ZukanDetailProcSys_FreeWork(ZukanDetailProcSys *sys);

// zukan_detail_common.c: what the pages share

struct ZukanDetailCommon {
    GameData *gameData;
    ZukanDetailGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    ZukanDetailTouchbar *touchbar;
    ZukanDetailHeadbar *headbar;
    // The Pokémon that the screen pages through, and where in the list it is
    const u16 *list;
    u16 count;
    u16 *index;
};

ZukanDetailCommon *ZukanDetailCommon_Create(HeapID heapId, GameData *gameData, ZukanDetailGraphic *graphic, Font *font,
                                            PrintQueue *printQueue, ZukanDetailTouchbar *touchbar,
                                            ZukanDetailHeadbar *headbar, const u16 *list, u16 count, u16 *index);
void ZukanDetailCommon_Free(ZukanDetailCommon *common);
GameData *ZukanDetailCommon_GetGameData(ZukanDetailCommon *common);
ZukanDetailGraphic *ZukanDetailCommon_GetGraphic(ZukanDetailCommon *common);
Font *ZukanDetailCommon_GetFont(ZukanDetailCommon *common);
PrintQueue *ZukanDetailCommon_GetPrintQueue(ZukanDetailCommon *common);
ZukanDetailTouchbar *ZukanDetailCommon_GetTouchbar(ZukanDetailCommon *common);
ZukanDetailHeadbar *ZukanDetailCommon_GetHeadbar(ZukanDetailCommon *common);
u16 ZukanDetailCommon_GetCount(ZukanDetailCommon *common);
u16 ZukanDetailCommon_GetSpecies(ZukanDetailCommon *common);
void ZukanDetailCommon_GoNext(ZukanDetailCommon *common);
void ZukanDetailCommon_GoPrev(ZukanDetailCommon *common);

// A screen's blending, which fades layers in or out by 2 of 16 every frame
struct ZukanDetailBlend {
    u32 plane1;
    u32 plane2;
    int ev;
    int step;
    int wait;
};

ZukanDetailBlend *ZukanDetailBlend_Create(HeapID heapId);
void ZukanDetailBlend_Free(ZukanDetailBlend *blend);
void ZukanDetailBlend_Update(ZukanDetailBlend *main, ZukanDetailBlend *sub);
BOOL ZukanDetailBlend_IsActive(ZukanDetailBlend *blend);
void ZukanDetailBlend_StartIn(ZukanDetailBlend *blend);
void ZukanDetailBlend_StartOut(ZukanDetailBlend *blend);
// screen is 0 for the main screen and 1 for the sub screen
void ZukanDetailBlend_SetIn(u32 screen, ZukanDetailBlend *blend);
void ZukanDetailBlend_SetOut(u32 screen, ZukanDetailBlend *blend);
void ZukanDetailBlend_InitPlanes(ZukanDetailBlend *blend);

// Fades the palettes of the screens to black and back
enum {
    ZUKAN_DETAIL_PALFADE_MAIN_BG = 1 << 0,
    ZUKAN_DETAIL_PALFADE_SUB_BG = 1 << 1,
    ZUKAN_DETAIL_PALFADE_MAIN_OBJ = 1 << 2,
    ZUKAN_DETAIL_PALFADE_SUB_OBJ = 1 << 3,
};

enum {
    ZUKAN_DETAIL_PALFADE_SHOWN,
    ZUKAN_DETAIL_PALFADE_HIDDEN,
    ZUKAN_DETAIL_PALFADE_FADING_IN,
    ZUKAN_DETAIL_PALFADE_FADING_OUT,
};

struct ZukanDetailPalFade {
    TCBManager *tcbMgr;
    void *tcbBuffer;
    PaletteFade *palette;
    // The ZUKAN_DETAIL_PALFADE_* palettes that it fades
    u16 buffers;
    int state;
};

ZukanDetailPalFade *ZukanDetailPalFade_CreateEx(HeapID heapId, u16 buffers);
ZukanDetailPalFade *ZukanDetailPalFade_Create(HeapID heapId);
void ZukanDetailPalFade_Free(ZukanDetailPalFade *fade);
void ZukanDetailPalFade_Update(ZukanDetailPalFade *fade);
void ZukanDetailPalFade_VBlank(ZukanDetailPalFade *fade);
BOOL ZukanDetailPalFade_IsFading(ZukanDetailPalFade *fade);
void ZukanDetailPalFade_StartIn(ZukanDetailPalFade *fade);
void ZukanDetailPalFade_StartOut(ZukanDetailPalFade *fade);
void ZukanDetailPalFade_SetHidden(ZukanDetailPalFade *fade);
void ZukanDetailPalFade_LoadPalette(ZukanDetailPalFade *fade, ArcTool *arc, u32 fileId, HeapID heapId, u32 buffer,
                                    u32 size, u16 offset, u16 srcOffset);
void ZukanDetailPalFade_ReadPalettes(ZukanDetailPalFade *fade);

// A page's scrolling background
struct ZukanDetailBackground {
    u32 chars;
    u8 bg;
    u8 wait;
};

ZukanDetailBackground *ZukanDetailBackground_Create(HeapID heapId, u32 page, u8 bg, u8 bottomPalette, u8 topPalette);
void ZukanDetailBackground_Free(ZukanDetailBackground *background);
void ZukanDetailBackground_Update(ZukanDetailBackground *background);
u32 ZukanDetail_LoadBG(BOOL loaded, HeapID heapId, u8 bg, u32 palettes, u8 palette, u8 srcPalette, u32 arcId, u32 nclr,
                       u32 ncgr, u32 nscr, u32 chars);
void ZukanDetail_FreeBG(u32 bg, u32 chars);

// zukan_detail_graphic.c: the screens' BGs, OBJs and 3D

// A BG to create, as the graphics and the map page set them up
typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ZukanDetailBGSetup;

// A window to create, as BmpWin_CreateDynamic takes it, in the pages' tables
typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u8 fromEnd;
} ZukanDetailWindowData;

ZukanDetailGraphic *ZukanDetailGraphic_Create(u32 layout, HeapID heapId, BOOL with3D);
void ZukanDetailGraphic_Free(ZukanDetailGraphic *graphic);
void ZukanDetailGraphic_Update(ZukanDetailGraphic *graphic);
// Draw the 3D system's frame, if it has been created
void ZukanDetailGraphic_Begin3D(ZukanDetailGraphic *graphic);
void ZukanDetailGraphic_End3D(ZukanDetailGraphic *graphic);
ClActUnit *ZukanDetailGraphic_GetClActUnit(ZukanDetailGraphic *graphic);
void ZukanDetailGraphic_Create3D(ZukanDetailGraphic *graphic, HeapID heapId);
void ZukanDetailGraphic_Free3D(ZukanDetailGraphic *graphic);

// zknd_tbar.c: the Pokédex's copy of the touch bar, a bar of icons at the bottom of a screen that are touched or
// pressed with a key

typedef struct ZkndTbar ZkndTbar;

// The icons that the bar draws itself. The others are the app's own
enum {
    ZKND_TBAR_ICON_CLOSE,
    ZKND_TBAR_ICON_RETURN,
    ZKND_TBAR_ICON_CUR_D,
    ZKND_TBAR_ICON_CUR_U,
    ZKND_TBAR_ICON_CUR_L,
    ZKND_TBAR_ICON_CUR_R,
    ZKND_TBAR_ICON_CHECK,
    ZKND_TBAR_ICON_CUSTOM,
};

// What touching an icon does: plays its pushed animation, or flips it on or off
enum {
    ZKND_TBAR_TYPE_PUSH,
    ZKND_TBAR_TYPE_FLIP,
};

typedef struct {
    int icon;
    ClActorPos pos;
    u16 width;
    // For a ZKND_TBAR_ICON_CUSTOM icon, its resources and animations, its key and its sound
    u16 chars;
    u16 palette;
    u16 cellAnims;
    u16 activeAnim;
    u16 inactiveAnim;
    u16 pushedAnim;
    u32 key;
    u32 se;
} ZkndTbarIcon;

typedef struct {
    const ZkndTbarIcon *icons;
    u32 iconCount;
    ClActUnit *unit;
    // The bar's BG, the palettes for the bar and its icons, and the OBJ VRAM mapping mode
    u32 bg;
    u32 bgPalette;
    u32 objPalette;
    u32 mapping;
    // Whether the app has loaded the bar's BG itself
    BOOL noBG;
} ZkndTbarParam;

ZkndTbar *ZkndTbar_Create(ZkndTbarParam *param, HeapID heapId);
void ZkndTbar_Free(ZkndTbar *tbar);
void ZkndTbar_Main(ZkndTbar *tbar);
// The icon that was touched, once its animation has played, and as it is touched, or -1
int ZkndTbar_GetTrigger(ZkndTbar *tbar);
int ZkndTbar_GetTouch(ZkndTbar *tbar);
void ZkndTbar_SetVisibleAll(ZkndTbar *tbar, BOOL visible);
void ZkndTbar_SetActiveAll(ZkndTbar *tbar, BOOL active);
BOOL ZkndTbar_GetActiveAll(ZkndTbar *tbar);
void ZkndTbar_Unlock(ZkndTbar *tbar);
void ZkndTbar_SetActive(ZkndTbar *tbar, int icon, BOOL active);
void ZkndTbar_SetVisible(ZkndTbar *tbar, int icon, BOOL visible);
BOOL ZkndTbar_GetVisible(ZkndTbar *tbar, int icon);
void ZkndTbar_SetKey(ZkndTbar *tbar, int icon, u32 key);
void ZkndTbar_SetFlip(ZkndTbar *tbar, int icon, BOOL flip);
BOOL ZkndTbar_GetFlip(ZkndTbar *tbar, int icon);
ClActor *ZkndTbar_GetActor(ZkndTbar *tbar, int icon);
void ZkndTbar_SetPos(ZkndTbar *tbar, int icon, const ClActorPos *pos);
// Acts as if the icon were touched
void ZkndTbar_Push(ZkndTbar *tbar, int icon);
BOOL ZkndTbar_IsTriggered(ZkndTbar *tbar, int icon);

// zukan_detail_touchbar.c: the bar at the bottom of the touch screen

// The commands of the bar's icons: the first once the icon's animation has played, the _TOUCH ones as it is touched
enum {
    ZUKAN_DETAIL_CMD_NONE,
    ZUKAN_DETAIL_CMD_CLOSE,
    ZUKAN_DETAIL_CMD_RETURN,
    ZUKAN_DETAIL_CMD_CUR_D,
    ZUKAN_DETAIL_CMD_CUR_U,
    ZUKAN_DETAIL_CMD_CHECK,
    ZUKAN_DETAIL_CMD_INFO,
    ZUKAN_DETAIL_CMD_MAP,
    ZUKAN_DETAIL_CMD_VOICE,
    ZUKAN_DETAIL_CMD_FORM,
    ZUKAN_DETAIL_CMD_CLOSE_TOUCH,
    ZUKAN_DETAIL_CMD_RETURN_TOUCH,
    ZUKAN_DETAIL_CMD_CUR_D_TOUCH,
    ZUKAN_DETAIL_CMD_CUR_U_TOUCH,
    ZUKAN_DETAIL_CMD_CHECK_TOUCH,
    ZUKAN_DETAIL_CMD_INFO_TOUCH,
    ZUKAN_DETAIL_CMD_MAP_TOUCH,
    ZUKAN_DETAIL_CMD_VOICE_TOUCH,
    ZUKAN_DETAIL_CMD_FORM_TOUCH,
    // The map's and the forms' bars
    ZUKAN_DETAIL_CMD_MAP_RETURN,
    ZUKAN_DETAIL_CMD_MAP_RETURN_TOUCH,
    ZUKAN_DETAIL_CMD_FORM_RETURN,
    ZUKAN_DETAIL_CMD_FORM_CUR_R,
    ZUKAN_DETAIL_CMD_FORM_CUR_L,
    ZUKAN_DETAIL_CMD_FORM_CUR_D,
    ZUKAN_DETAIL_CMD_FORM_CUR_U,
    ZUKAN_DETAIL_CMD_FORM_BUTTON,
    ZUKAN_DETAIL_CMD_MAP_PLACE,
    ZUKAN_DETAIL_CMD_FORM_RETURN_TOUCH,
    ZUKAN_DETAIL_CMD_FORM_CUR_R_TOUCH,
    ZUKAN_DETAIL_CMD_FORM_CUR_L_TOUCH,
    ZUKAN_DETAIL_CMD_FORM_CUR_D_TOUCH,
    ZUKAN_DETAIL_CMD_FORM_CUR_U_TOUCH,
    ZUKAN_DETAIL_CMD_FORM_BUTTON_TOUCH,
    ZUKAN_DETAIL_CMD_MAP_PLACE_TOUCH,
};

// The bar's icons: the pages' tabs, or the map's or the forms' buttons
enum {
    ZUKAN_DETAIL_TOUCHBAR_GENERAL,
    ZUKAN_DETAIL_TOUCHBAR_MAP,
    ZUKAN_DETAIL_TOUCHBAR_FORM,
};

// ZukanDetailTouchbar_GetState
enum {
    ZUKAN_DETAIL_TOUCHBAR_HIDDEN,
    ZUKAN_DETAIL_TOUCHBAR_APPEARING,
    ZUKAN_DETAIL_TOUCHBAR_SHOWN,
    ZUKAN_DETAIL_TOUCHBAR_DISAPPEARING,
};

// showFormTab is a setting of the Pokédex's save (PokeDex_IsGiven), and mode the screen's ZukanDetailParam mode, which
// hides the map's tab and the check box
ZukanDetailTouchbar *ZukanDetailTouchbar_Create(HeapID heapId, BOOL showFormTab, u32 mode);
void ZukanDetailTouchbar_Free(ZukanDetailTouchbar *touchbar);
void ZukanDetailTouchbar_Update(ZukanDetailTouchbar *touchbar);
// page is a ZUKAN_DETAIL_PAGE_* less 1
void ZukanDetailTouchbar_SetType(ZukanDetailTouchbar *touchbar, int type, int page, BOOL showArrows);
int ZukanDetailTouchbar_GetState(ZukanDetailTouchbar *touchbar);
// Slides the bar in or out
void ZukanDetailTouchbar_Appear(ZukanDetailTouchbar *touchbar, u32 speed);
void ZukanDetailTouchbar_Disappear(ZukanDetailTouchbar *touchbar, u32 speed);
int ZukanDetailTouchbar_GetTrigger(ZukanDetailTouchbar *touchbar);
int ZukanDetailTouchbar_GetTouch(ZukanDetailTouchbar *touchbar);
void ZukanDetailTouchbar_Unlock(ZukanDetailTouchbar *touchbar);
void ZukanDetailTouchbar_SetVisibleAll(ZukanDetailTouchbar *touchbar, BOOL visible);
void ZukanDetailTouchbar_SetPage(ZukanDetailTouchbar *touchbar, int page);
void ZukanDetailTouchbar_SetFormArrowsVisible(ZukanDetailTouchbar *touchbar, BOOL visible);
void ZukanDetailTouchbar_SetCheck(ZukanDetailTouchbar *touchbar, BOOL check);
BOOL ZukanDetailTouchbar_GetCheck(ZukanDetailTouchbar *touchbar);
void ZukanDetailTouchbar_SetActive(ZukanDetailTouchbar *touchbar, BOOL active);
u32 ZukanDetailTouchbar_GetIconPalette(ZukanDetailTouchbar *touchbar);
void ZukanDetailTouchbar_SetBGPriority(ZukanDetailTouchbar *touchbar, u8 priority);
void ZukanDetailTouchbar_SetMapPlaceActive(ZukanDetailTouchbar *touchbar, BOOL active);
void ZukanDetailTouchbar_SetMapPlaceVisible(ZukanDetailTouchbar *touchbar, BOOL visible);
void ZukanDetailTouchbar_PushMapPlace(ZukanDetailTouchbar *touchbar);
BOOL ZukanDetailTouchbar_IsMapPlaceTriggered(ZukanDetailTouchbar *touchbar);
BOOL ZukanDetailTouchbar_IsArrowTriggered(ZukanDetailTouchbar *touchbar);
BOOL ZukanDetailTouchbar_IsFormButtonTriggered(ZukanDetailTouchbar *touchbar);

// zukan_detail_headbar.c: the bar at the top of the screen

// The titles, messages 179 on of system message file 441
#define ZUKAN_DETAIL_HEADBAR_TITLE_COUNT 5

ZukanDetailHeadbar *ZukanDetailHeadbar_Create(HeapID heapId, Font *font);
void ZukanDetailHeadbar_Free(ZukanDetailHeadbar *headbar);
void ZukanDetailHeadbar_Update(ZukanDetailHeadbar *headbar);
void ZukanDetailHeadbar_SetTitle(ZukanDetailHeadbar *headbar, int title);
// The same states as the touch bar's
int ZukanDetailHeadbar_GetState(ZukanDetailHeadbar *headbar);
// Slides the bar in or out
void ZukanDetailHeadbar_Appear(ZukanDetailHeadbar *headbar);
void ZukanDetailHeadbar_Disappear(ZukanDetailHeadbar *headbar);

// zukan_detail_info.c, a guessed name, as the ROM names neither this file nor the map's: the Pokémon's info

typedef struct {
    HeapID heapId;
} ZukanDetailInfoParam;

extern const ZukanDetailProcFuncs ZUKAN_DETAIL_INFO_PROC_FUNCS;

void ZukanDetailInfo_InitParam(ZukanDetailInfoParam *param, HeapID heapId);

// zukan_detail_map.c, a guessed name: where the Pokémon lives

typedef struct {
    HeapID heapId;
    // The zone of the place picked on the map for the habitat list, or ZUKAN_DETAIL_MAP_NO_PLACE
    u16 place;
} ZukanDetailMapParam;

// One past the last zone
#define ZUKAN_DETAIL_MAP_NO_PLACE 615

extern const ZukanDetailProcFuncs ZUKAN_DETAIL_MAP_PROC_FUNCS;

void ZukanDetailMap_InitParam(ZukanDetailMapParam *param, HeapID heapId);

// zukan_detail_voice.c: the Pokémon's cry

typedef struct {
    HeapID heapId;
} ZukanDetailVoiceParam;

extern const ZukanDetailProcFuncs ZUKAN_DETAIL_VOICE_PROC_FUNCS;

void ZukanDetailVoice_InitParam(ZukanDetailVoiceParam *param, HeapID heapId);

// zukan_detail_form.c: the Pokémon's forms

typedef struct {
    HeapID heapId;
} ZukanDetailFormParam;

extern const ZukanDetailProcFuncs ZUKAN_DETAIL_FORM_PROC_FUNCS;

// zukan_detail_form_data.c: the forms page's tables that the linker placed ahead of its file

// The resources of the forms page's actors and buttons
enum {
    ZUKAN_DETAIL_FORM_RES_MAIN_CHARS,
    ZUKAN_DETAIL_FORM_RES_MAIN_PALETTE,
    ZUKAN_DETAIL_FORM_RES_MAIN_CELL_ANIMS,
    ZUKAN_DETAIL_FORM_RES_COLOR_CHARS,
    ZUKAN_DETAIL_FORM_RES_COLOR_PALETTE,
    ZUKAN_DETAIL_FORM_RES_COLOR_CELL_ANIMS,
    ZUKAN_DETAIL_FORM_RES_ARROW_CHARS,
    ZUKAN_DETAIL_FORM_RES_ARROW_PALETTE,
    ZUKAN_DETAIL_FORM_RES_ARROW_CELL_ANIMS,
    ZUKAN_DETAIL_FORM_RES_COUNT,
};

typedef struct {
    f32 x;
    f32 y;
    f32 z;
} ZukanDetailFormPos;

typedef struct {
    u8 x;
    u8 y;
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    // ZUKAN_DETAIL_FORM_RES_*
    u8 chars;
    u8 palette;
    u8 cellAnims;
} ZukanDetailFormActorData;

extern const u16 ZUKAN_DETAIL_FORM_STRBUF_MESSAGES[];
extern const ZukanDetailFormActorData ZUKAN_DETAIL_FORM_ACTORS[];
extern const ZukanDetailFormPos ZUKAN_DETAIL_FORM_DEFAULT_POSITIONS[];

void ZukanDetailForm_InitParam(ZukanDetailFormParam *param, HeapID heapId);

#endif // POKEBW2_APP_ZUKAN_DETAIL_H
