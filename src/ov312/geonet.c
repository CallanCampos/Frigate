#include "types.h"
#include "app/geonet.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/unity_tower.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/math_util.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/app_taskmenu.h"
#include "system/bmp_menu.h"
#include "system/bmp_menulist.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_winframe.h"
#include "system/country_region.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/version.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/event_mapchange_2.h"

// Geonet, the globe of the Pokémon Centers: the player registers where they live, and the globe shows the places of
// the people they have traded and battled with. The ROM has no name for this file; "geonet.c" is a guess. It is the
// Generation 4 globe's code (pokeplatinum's wifi_earth) moved to this game's libraries

#define HEAPID_GEONET 0x7c
// The globe's models and the screens' graphics
#define ARCID_GEONET 194
// The places of the globe: member 18 has those of the countries, and each country with regions has a member of its
// own
#define ARCID_GEONET_PLACE 52
#define NARC_GEONET_PLACE_COUNTRIES 18

#define NARC_GEONET_EARTH_NSBMD 0
#define NARC_GEONET_BG_NCGR 5
#define NARC_GEONET_BG_NCLR 6
#define NARC_GEONET_BG_NSCR 7

#define COUNTRY_USA 220

#define PLACE_MAX 1024

// The marks: one for each UNITY_TOWER_REGION_* state but none, and the one at the middle of the screen
#define MARK_CURSOR 4
#define MARK_COUNT 5

#define BUTTON_VIEW 0
#define BUTTON_QUIT 1
#define BUTTON_COUNT 2

#define WINDOW_INFO 0
#define WINDOW_PLACE 3
#define WINDOW_COUNT 4

// The frames' characters and palettes
#define FRAME_MESSAGE_CHAR 4
#define FRAME_MESSAGE_PALETTE 6
#define FRAME_MENU_CHAR 0x22
#define FRAME_MENU_PALETTE 7

// How far the camera is from the globe, and how fast it moves between the two
#define CAMERA_DISTANCE_NEAR (FX32_ONE * 80)
#define CAMERA_DISTANCE_FAR (FX32_ONE * 296)
#define CAMERA_SPEED (FX32_ONE * 8)

// The globe's rotation in the Japanese game, which shows only Japan until the player has met a foreign place
#define JAPAN_ROTATION_X_MIN 0x1300
#define JAPAN_ROTATION_X_MAX 0x2020
#define JAPAN_ROTATION_Y_MIN (-0x3380)
#define JAPAN_ROTATION_Y_MAX (-0x27e0)
#define WORLD_ROTATION_X_MAX 0x3e00

enum {
    SEQ_INIT,
    SEQ_FADE_IN,
    SEQ_WELCOME,
    SEQ_MENU_MESSAGE,
    SEQ_MENU,
    SEQ_REGISTER_MESSAGE,
    SEQ_REGISTER_CONFIRM,
    SEQ_COUNTRY_LIST,
    SEQ_COUNTRY_MESSAGE,
    SEQ_COUNTRY_SELECT,
    SEQ_REGION_LIST,
    SEQ_REGION_MESSAGE,
    SEQ_REGION_SELECT,
    SEQ_PLACE_SHOW,
    SEQ_PLACE_MESSAGE,
    SEQ_PLACE_CONFIRM,
    SEQ_EARTH_INIT,
    SEQ_EARTH,
    SEQ_EARTH_ZOOM,
    SEQ_EARTH_QUIT,
    SEQ_FADE_OUT,
    SEQ_END,
};

enum {
    DRAW_NONE,
    DRAW_EARTH,
    DRAW_CLEAR,
};

typedef struct {
    s16 x;
    s16 y;
    MtxFx33 rotation;
    // UNITY_TOWER_REGION_*
    u16 state;
    u16 country;
    u16 region;
} GeonetPlace;

// A country's entry in the places' file, and a region's in its country's file
typedef struct {
    u16 state;
    s16 x;
    s16 y;
} GeonetCountryPlace;

typedef struct {
    s16 x;
    s16 y;
} GeonetRegionPlace;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u8 charOffset;
} GeonetWindowSetup;

typedef struct {
    BmpWin *window;
    PrintWindow print;
} GeonetWindow;

typedef struct {
    u32 message;
    u32 value;
} GeonetMenuItem;

typedef struct {
    u16 mark;
    u16 file;
} GeonetMarkFile;

typedef struct {
    int x;
    int y;
    int width;
} GeonetButtonPos;

typedef struct {
    int x;
    int y;
} GeonetAngles;

typedef struct {
    int heapId;
    GameData *gameData;
    SaveControl *save;
    PlayerInfo *playerInfo;
    UnityTowerSurveySave *survey;
    u32 placeCount;
    GeonetPlace places[PLACE_MAX];
    BmpWin *listWindow;
    GeonetWindow windows[WINDOW_COUNT];
    BmpMenuList *list;
    ListMenuOption *options;
    BmpMenu *yesNo;
    AppTaskMenuRes *buttonRes;
    AppTaskMenuWin *buttons[BUTTON_COUNT];
    StrBuf *buttonText[BUTTON_COUNT];
    Font *font;
    MsgData *msgData;
    PrintStream *stream;
    PrintWindow listPrint;
    PrintQueue *queue;
    u8 textColors[3];
    KeyCursor *keyCursor;
    AppTaskMenuItem buttonItems[BUTTON_COUNT];
    TCBExManager *tcbManager;
    WordSet *wordSet;
    StrBuf *message;
    int messageState;
    u8 unusedC0B4[0x228];
    VecFx32 translation;
    VecFx32 scale;
    VecFx32 rotation;
    VecFx32 markScale;
    // Whether the camera is near the globe
    u16 zoom;
    int drawState;
    int unusedC314;
    int isJapan;
    // Whether the whole globe can be seen
    u16 worldMode;
    // The keys that the touch screen stands for
    int touchKeys;
    int touchState;
    int touchPrevX;
    int touchPrevY;
    int touchMoveX;
    int touchMoveY;
    int tapTimer;
    int myCountry;
    int myRegion;
    int selCountry;
    int selRegion;
    int foreignRegionMet;
    // Whether the window shows the place at the middle of the screen
    int infoMode;
    u32 keysPressed;
    u32 keysHeld;
    BOOL touchPressed;
    BOOL touchHeld;
    u32 touchX;
    u32 touchY;
    G3DCamera *camera;
    G3DLight *light;
    void *earthRes;
    void *markRes[MARK_COUNT];
    G3DModel *earthModel;
    G3DModel *markModels[MARK_COUNT];
    G3DActor *earthActor;
    G3DActor *markActors[MARK_COUNT];
} GeonetWork;

static BOOL GeonetProc_Init(GameProc *proc, u32 *state, void *param, void *procWork);
static BOOL GeonetProc_Main(GameProc *proc, u32 *state, void *param, void *procWork);
static void Geonet_PressButton(GeonetWork *work, int button);
static void Geonet_UpdateButtons(GeonetWork *work);
static BOOL Geonet_Main(GeonetWork *work, u32 *state);
static BOOL GeonetProc_End(GameProc *proc, u32 *state, void *param, void *procWork);
static void Geonet_SetVRAMBanks(void);
static void Geonet_InitGraphics(HeapID heapId);
static void Geonet_FreeGraphics(void);
static void Geonet_LoadPlaces(GeonetWork *work);
static void Geonet_AddPlace(GeonetWork *work, int index, s16 x, s16 y, u16 country, u16 region);
static void Geonet_MarkOwnPlace(GeonetWork *work);
static u32 Geonet_GetSubdividedIndex(u32 country);
static void Geonet_ReadTouch(GeonetWork *work);
static void Geonet_GetTouchMove(GeonetWork *work, int prevX, int prevY, int *keysX, int *moveX, int *keysY,
                                int *moveY);
static BmpWin *Geonet_CreateWindow(const GeonetWindowSetup *setup, int show);
static void Geonet_LoadBG(GeonetWork *work, ArcTool *arc);
static void Geonet_InitButtons(GeonetWork *work);
static void Geonet_CreateButton(GeonetWork *work, int button);
static void Geonet_FreeButton(GeonetWork *work, int button);
static void Geonet_FreeBG(GeonetWork *work);
static void Geonet_Print(GeonetWork *work, PrintWindow *print, BmpWin *window, StrBuf *strbuf, u8 x, u8 y);
static BOOL Geonet_PrintMessage(GeonetWork *work, u32 message, BOOL close);
static void Geonet_ListCursorCallback(BmpMenuList *list, s32 value, u8 init);
static void Geonet_CreateMenu(GeonetWork *work, const GeonetWindowSetup *setup, const BmpMenuListHeader *header,
                              const GeonetMenuItem *items);
static void Geonet_CreateList(GeonetWork *work, const BmpMenuListHeader *header, u32 count);
static void Geonet_CreateCountryList(GeonetWork *work, const GeonetWindowSetup *setup,
                                     const BmpMenuListHeader *header, u32 textBank);
static void Geonet_CreateRegionList(GeonetWork *work, const GeonetWindowSetup *setup,
                                    const BmpMenuListHeader *header, u32 textBank, const u8 *order, u32 count);
static void Geonet_FreeList(GeonetWork *work);
static void Geonet_ShowRegisteredPlace(GeonetWork *work);
static void Geonet_ShowSelectedPlace(GeonetWork *work, u32 country, u32 region);
static void Geonet_ClosePlaceWindow(GeonetWork *work);
static BOOL Geonet_FindNearestPlace(GeonetWork *work, u32 *index);
static void Geonet_UpdateInfo(GeonetWork *work);
static void Geonet_Load3D(GeonetWork *work, ArcTool *arc);
static void Geonet_Free3D(GeonetWork *work);
static void Geonet_InitEarth(GeonetWork *work);
static void Geonet_CreateCamera(GeonetWork *work);
static void Geonet_ResetCamera(GeonetWork *work);
static void Geonet_CreateLight(GeonetWork *work);
static void Geonet_GetEarthRotation(MtxFx33 *mtx, const VecFx32 *rotation);
static void Geonet_GetPlaceRotation(MtxFx33 *mtx, const VecFx32 *rotation);
static BOOL Geonet_Rotate(GeonetWork *work, u32 pressed, u32 held);
static BOOL Geonet_MoveCamera(GeonetWork *work);
static void Geonet_Draw(GeonetWork *work);
static BOOL Geonet_LoadPlaceNames(u32 country, u32 region, StrBuf *countryName, StrBuf *regionName, int heapId);
static BOOL Geonet_IsSubdivided(u32 country);
static void Geonet_WrapAngles(GeonetAngles *angles);
static u32 Geonet_GetDistance(const GeonetAngles *a, const GeonetAngles *b);

static const ConfirmDialogSetup sGeonetYesNoSetup = {6, 24, 13, 4, 0x174};

static const GeonetWindowSetup sGeonetListWindow = {6, 3, 2, 26, 13, 4, 0x3e};
static const GeonetWindowSetup sGeonetPlaceWindow = {6, 2, 1, 27, 6, 4, 0xd2};
static const GeonetWindowSetup sGeonetMenuWindow = {6, 18, 11, 13, 6, 4, 0x42};
static const GeonetWindowSetup sGeonetInfoWindow = {6, 2, 19, 28, 4, 4, 0x90};

static const LightSetup sGeonetLights[] = {
    {0, {{0, 0, -FX16_ONE + 1}, GX_RGB(31, 31, 31)}},
};

static const LightSetupList sGeonetLightList = {sGeonetLights, NELEMS(sGeonetLights)};

static const VecFx32 sGeonetCameraPos = {0, 0, CAMERA_DISTANCE_FAR};
static const VecFx32 sGeonetCameraTarget = {0, 0, 0};

// The View and Quit buttons
static const TouchRect sGeonetButtonRects[] = {
    {168, 191, 0, 71},
    {168, 191, 184, 255},
    {TOUCH_RECT_END, 0, 0, 0},
};

// Where the globe is dragged: the screen above the buttons, and the gap between them
static const TouchRect sGeonetTouchRects[] = {
    {0, 167, 0, 239},
    {168, 191, 72, 183},
    {TOUCH_RECT_END, 0, 0, 0},
};

static const GeonetMarkFile sGeonetMarkFiles[] = {
    {UNITY_TOWER_REGION_OWN, 1},
    {MARK_CURSOR, 2},
    {UNITY_TOWER_REGION_TODAY, 3},
    {UNITY_TOWER_REGION_BEFORE, 4},
};

static const GeonetMenuItem sGeonetMenuItems[] = {
    {EventMapchange2_Text_SeeList, 0},
    {EventMapchange2_Text_Register, 1},
    {EventMapchange2_Text_Cancel, 2},
};

static const GeonetButtonPos sGeonetButtonPos[BUTTON_COUNT] = {
    {0, 21, 9},
    {23, 21, 9},
};

static const BGSetup sGeonetTextBGSetup = {0, 0, 0x800, 0, 1, 0, 14, 0, 0x4000, 0, 0, 0, 0};
static const BGSetup sGeonetBackBGSetup = {0, 0, 0x800, 0, 1, 0, 15, 1, 0x4000, 0, 3, 0, 0};

static const BmpMenuListHeader sGeonetMenuHeader = {
    NULL, NULL, NULL, 3, 3, 0, 12, 0, 0, 1, 15, 2, 0, 0, BMPMENULIST_SKIP_NONE, 0, BMPMENULIST_CURSOR_SHOW,
    NULL, 0, 0, NULL, NULL, NULL, NULL, 20,
};

static const BmpMenuListHeader sGeonetListHeader = {
    NULL, NULL, NULL, 0, 7, 0, 12, 0, 0, 1, 15, 2, 0, 0, BMPMENULIST_SKIP_LR_KEY, 0, BMPMENULIST_CURSOR_SHOW,
    NULL, 0, 0, NULL, NULL, NULL, NULL, 20,
};

const GameProcFunctions GEONET_PROC_FUNCTIONS = {GeonetProc_Init, GeonetProc_Main, GeonetProc_End};

static BOOL GeonetProc_Init(GameProc *proc, u32 *state, void *param, void *procWork) {
    GeonetWork *work;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GEONET, 0x80000);
    work = GFL_ProcInitSubsystem(proc, sizeof(GeonetWork), HEAPID_GEONET);
    sys_memset(work, 0, sizeof(GeonetWork));
    work->heapId = HEAPID_GEONET;
    work->gameData = param;
    work->save = GameData_GetSaveControl(param);
    work->playerInfo = GetGameDataPlayerInfo(work->gameData);
    work->survey = getUnityTower_SurveySaveBlkAddrress(work->save);
    if (region == 1) {
        work->isJapan = TRUE;
    } else {
        work->isJapan = FALSE;
    }
    work->myCountry = UnityTowerVisitor_GetCountry(work->playerInfo);
    work->myRegion = UnityTowerVisitor_GetProvince(work->playerInfo);
    work->foreignRegionMet = func_02009c38(work->survey);
    work->buttons[BUTTON_VIEW] = NULL;
    work->buttons[BUTTON_QUIT] = NULL;
    Geonet_SetVRAMBanks();
    Geonet_InitGraphics(work->heapId);
    work->wordSet = GFL_WordSetSystemCreate(8, 64, work->heapId);
    work->drawState = DRAW_NONE;
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_SUB_MAIN);
    work->tcbManager = GFL_TCBExMgrCreate(work->heapId, work->heapId, 32, 32);
    work->keyCursor = KeyCursor_Create(15, TRUE, FALSE, work->heapId);
    work->queue = func_02021998(work->heapId);
    func_02042ba8(TRUE, work->heapId);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    Geonet_LoadPlaces(work);
    return TRUE;
}

static BOOL GeonetProc_Main(GameProc *proc, u32 *state, void *param, void *procWork) {
    GeonetWork *work = procWork;
    BOOL result;

    GFL_TCBExMgrUpdate(work->tcbManager);
    result = Geonet_Main(work, state);
    if (work->earthActor != NULL) {
        Geonet_Draw(work);
    }
    Geonet_UpdateButtons(work);
    func_02021a3c(work->queue);
    return result;
}

// Shows a button as the cursor's, or makes the quit button flash
static void Geonet_PressButton(GeonetWork *work, int button) {
    if (work->buttons[button] != NULL) {
        if (button == BUTTON_VIEW) {
            AppTaskMenuWin_SetActive(work->buttons[BUTTON_VIEW], TRUE);
        } else if (button == BUTTON_QUIT) {
            AppTaskMenuWin_SetFlashing(work->buttons[BUTTON_QUIT], TRUE);
        }
    }
}

static void Geonet_UpdateButtons(GeonetWork *work) {
    if (work->buttons[BUTTON_VIEW] != NULL) {
        AppTaskMenuWin_Update(work->buttons[BUTTON_VIEW]);
    }
    if (work->buttons[BUTTON_QUIT] != NULL) {
        AppTaskMenuWin_Update(work->buttons[BUTTON_QUIT]);
    }
}

static BOOL Geonet_Main(GeonetWork *work, u32 *state) {
    BOOL result = FALSE;

    work->keysPressed = GCTX_HIDGetPressedKeys();
    work->keysHeld = GCTX_HIDGetHeldKeys();
    if (func_0203d9c8(sGeonetTouchRects) != TOUCH_RECT_NONE) {
        work->touchHeld = func_0203da84(&work->touchX, &work->touchY);
    } else {
        work->touchHeld = FALSE;
    }
    if (work->touchHeld == TRUE) {
        work->touchPressed = func_0203da48();
    } else {
        work->touchPressed = func_0203dac8(&work->touchX, &work->touchY);
    }

    switch (*state) {
    case SEQ_INIT: {
        ArcTool *arc;

        work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_MAPCHANGE_2, work->heapId);
        arc = GFL_ArcSysCreateFileHandle(ARCID_GEONET, work->heapId);
        Geonet_Load3D(work, arc);
        Geonet_LoadBG(work, arc);
        GFL_ArcToolFree(arc);
        work->unusedC314 = 0;
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, work->heapId);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysSetBGEnabled(3, TRUE);
        *state = SEQ_FADE_IN;
        break;
    }
    case SEQ_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            *state = SEQ_WELCOME;
        }
        break;
    case SEQ_WELCOME:
        if (Geonet_PrintMessage(work, EventMapchange2_Text_WelcomeGeonetHighTech, TRUE) == TRUE) {
            if (work->myCountry == COUNTRY_NONE) {
                *state = SEQ_MENU_MESSAGE;
            } else {
                *state = SEQ_EARTH_INIT;
            }
        }
        break;
    case SEQ_MENU_MESSAGE:
        if (Geonet_PrintMessage(work, EventMapchange2_Text_WhatWouldLike, TRUE) == TRUE) {
            Geonet_CreateMenu(work, &sGeonetMenuWindow, &sGeonetMenuHeader, sGeonetMenuItems);
            *state = SEQ_MENU;
        }
        break;
    case SEQ_MENU:
        if (func_02021c0c(work->queue)) {
            u32 choice = BmpMenuList_Update(work->list);

            if (choice != BMPMENULIST_NULL) {
                Geonet_FreeList(work);
                switch (choice) {
                default:
                case 0:
                    *state = SEQ_EARTH_INIT;
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    break;
                case 1:
                    *state = SEQ_REGISTER_MESSAGE;
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    break;
                case 2:
                case BMPMENULIST_CANCEL:
                    *state = SEQ_FADE_OUT;
                    GFL_SndSEPlay(SEQ_SE_CANCEL1);
                    break;
                }
            }
        }
        break;
    case SEQ_REGISTER_MESSAGE:
        if (Geonet_PrintMessage(work, EventMapchange2_Text_CanRegisterLocationInformation, TRUE) == TRUE) {
            work->yesNo =
                ShopUI_CreateConfirmDialog(&sGeonetYesNoSetup, FRAME_MENU_CHAR, FRAME_MENU_PALETTE, 0, work->heapId);
            *state = SEQ_REGISTER_CONFIRM;
        }
        break;
    case SEQ_REGISTER_CONFIRM:
        switch (ConfirmDialog_Update(work->yesNo)) {
        case 0:
            if (work->isJapan == TRUE) {
                work->selCountry = Country_GetJapan();
                *state = SEQ_REGION_LIST;
            } else {
                *state = SEQ_COUNTRY_LIST;
            }
            break;
        case BMPMENU_CANCEL:
            *state = SEQ_MENU_MESSAGE;
            break;
        }
        break;
    case SEQ_COUNTRY_LIST:
        work->selCountry = COUNTRY_NONE;
        Geonet_CreateCountryList(work, &sGeonetListWindow, &sGeonetListHeader,
                                 TEXT_BANK_EVENT_MAPCHANGE_LOAD_COUNTRY_TO_STRBUF);
        *state = SEQ_COUNTRY_MESSAGE;
        break;
    case SEQ_COUNTRY_MESSAGE:
        if (Geonet_PrintMessage(work, EventMapchange2_Text_PleaseSelectLocationWhere, TRUE) == TRUE) {
            *state = SEQ_COUNTRY_SELECT;
        }
        break;
    case SEQ_COUNTRY_SELECT:
        if (func_02021c0c(work->queue)) {
            u32 choice = BmpMenuList_Update(work->list);

            if (choice != BMPMENULIST_NULL) {
                Geonet_FreeList(work);
                switch (choice) {
                default:
                    work->selCountry = choice;
                    if (Geonet_IsSubdivided(choice) == TRUE) {
                        *state = SEQ_REGION_LIST;
                    } else {
                        work->selRegion = 0;
                        *state = SEQ_PLACE_SHOW;
                    }
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    break;
                case BMPMENULIST_CANCEL:
                    *state = SEQ_MENU_MESSAGE;
                    GFL_SndSEPlay(SEQ_SE_CANCEL1);
                    break;
                }
            }
        }
        break;
    case SEQ_REGION_LIST: {
        const u8 *order;
        u32 count;

        work->selRegion = 0;
        Country_GetRegionOrder(work->selCountry, &order, &count);
        Geonet_CreateRegionList(work, &sGeonetListWindow, &sGeonetListHeader,
                                fetchFileNumOfDividedCountry(work->selCountry), order, count);
        *state = SEQ_REGION_MESSAGE;
        break;
    }
    case SEQ_REGION_MESSAGE:
        if (Geonet_PrintMessage(work, EventMapchange2_Text_PleaseSelectAreaWhere, TRUE) == TRUE) {
            *state = SEQ_REGION_SELECT;
        }
        break;
    case SEQ_REGION_SELECT:
        if (func_02021c0c(work->queue)) {
            u32 choice = BmpMenuList_Update(work->list);

            if (choice != BMPMENULIST_NULL) {
                Geonet_FreeList(work);
                switch (choice) {
                default:
                    work->selRegion = choice;
                    *state = SEQ_PLACE_SHOW;
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    break;
                case BMPMENULIST_CANCEL:
                    GFL_SndSEPlay(SEQ_SE_CANCEL1);
                    if (work->isJapan == TRUE) {
                        *state = SEQ_MENU_MESSAGE;
                    } else {
                        *state = SEQ_COUNTRY_LIST;
                    }
                    break;
                }
            }
        }
        break;
    case SEQ_PLACE_SHOW:
        Geonet_ShowSelectedPlace(work, work->selCountry, work->selRegion);
        *state = SEQ_PLACE_MESSAGE;
        break;
    case SEQ_PLACE_MESSAGE:
        if (Geonet_PrintMessage(work, EventMapchange2_Text_MayNotChangeLocation, TRUE) == TRUE) {
            work->yesNo =
                ShopUI_CreateConfirmDialog(&sGeonetYesNoSetup, FRAME_MENU_CHAR, FRAME_MENU_PALETTE, 0, work->heapId);
            *state = SEQ_PLACE_CONFIRM;
        }
        break;
    case SEQ_PLACE_CONFIRM:
        switch (ConfirmDialog_Update(work->yesNo)) {
        case 0:
            Geonet_ClosePlaceWindow(work);
            func_02009b84(work->survey, work->playerInfo, work->selCountry, work->selRegion);
            GameBeaconSys_SetCountryRegion(work->selCountry, work->selRegion);
            work->myCountry = work->selCountry;
            work->myRegion = work->selRegion;
            *state = SEQ_EARTH_INIT;
            break;
        case BMPMENU_CANCEL:
            Geonet_ClosePlaceWindow(work);
            *state = SEQ_MENU_MESSAGE;
            break;
        }
        break;
    case SEQ_EARTH_INIT:
        if (work->isJapan == TRUE && work->foreignRegionMet == 0) {
            work->worldMode = FALSE;
        } else {
            work->worldMode = TRUE;
        }
        Geonet_InitEarth(work);
        Geonet_MarkOwnPlace(work);
        Geonet_ResetCamera(work);
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_INFO].window), 15);
        BmpWin_FlushChar(work->windows[WINDOW_INFO].window);
        Geonet_CreateButton(work, BUTTON_QUIT);
        if (work->myCountry != COUNTRY_NONE) {
            Geonet_ShowRegisteredPlace(work);
        }
        Geonet_UpdateInfo(work);
        work->infoMode = FALSE;
        work->drawState = DRAW_EARTH;
        *state = SEQ_EARTH;
        break;
    case SEQ_EARTH: {
        u16 zoom = work->zoom;
        u32 index;

        if (!func_02021c0c(work->queue)) {
            break;
        }
        Geonet_ReadTouch(work);
        if (Geonet_FindNearestPlace(work, &index)) {
            Geonet_CreateButton(work, BUTTON_VIEW);
        } else {
            Geonet_FreeButton(work, BUTTON_VIEW);
        }
        if ((work->keysPressed & PAD_BUTTON_B) || (work->touchKeys & PAD_BUTTON_B)) {
            Geonet_FreeButton(work, BUTTON_VIEW);
            Geonet_PressButton(work, BUTTON_QUIT);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_INFO].window), 15);
            BmpWin_FlushChar(work->windows[WINDOW_INFO].window);
            *state = SEQ_EARTH_QUIT;
            break;
        }
        if (((work->keysPressed & PAD_BUTTON_X) || (work->touchKeys & PAD_BUTTON_X)) && work->infoMode == FALSE) {
            work->infoMode = TRUE;
            Geonet_PressButton(work, BUTTON_VIEW);
            Geonet_UpdateInfo(work);
            if (work->infoMode == TRUE) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            break;
        }
        if (((work->keysPressed & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A)) || (work->touchKeys & PAD_BUTTON_X))
            && work->infoMode == TRUE) {
            work->infoMode = FALSE;
            Geonet_UpdateInfo(work);
            break;
        }
        if (Geonet_Rotate(work, work->keysPressed, work->keysHeld) == TRUE && work->infoMode == TRUE) {
            work->infoMode = FALSE;
            Geonet_UpdateInfo(work);
        }
        if (zoom != work->zoom) {
            *state = SEQ_EARTH_ZOOM;
            if (work->zoom == FALSE) {
                GFL_SndSEPlay(SEQ_SE_SYS_10);
            } else {
                GFL_SndSEPlay(SEQ_SE_SYS_09);
            }
        }
        break;
    }
    case SEQ_EARTH_QUIT:
        if (AppTaskMenuWin_IsFlashFinished(work->buttons[BUTTON_QUIT])) {
            if (work->myCountry == COUNTRY_NONE) {
                work->drawState = DRAW_CLEAR;
                *state = SEQ_MENU_MESSAGE;
            } else {
                Geonet_ClosePlaceWindow(work);
                *state = SEQ_FADE_OUT;
            }
            Geonet_FreeButton(work, BUTTON_QUIT);
        }
        break;
    case SEQ_EARTH_ZOOM:
        if (Geonet_MoveCamera(work) == TRUE) {
            *state = SEQ_EARTH;
        }
        break;
    case SEQ_FADE_OUT:
        work->unusedC314 = 0;
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, work->heapId);
        *state = SEQ_END;
        break;
    case SEQ_END:
        if (GFL_WipeIsFinished() == TRUE) {
            work->drawState = DRAW_EARTH;
            result = TRUE;
            Geonet_FreeBG(work);
            Geonet_Free3D(work);
            GFL_MsgDataFree(work->msgData);
            *state = SEQ_INIT;
        }
        break;
    }

    PrintWindow_Flush(&work->listPrint, work->queue);
    PrintWindow_Flush(&work->windows[WINDOW_PLACE].print, work->queue);
    PrintWindow_Flush(&work->windows[2].print, work->queue);
    PrintWindow_Flush(&work->windows[1].print, work->queue);
    PrintWindow_Flush(&work->windows[WINDOW_INFO].print, work->queue);
    return result;
}

static BOOL GeonetProc_End(GameProc *proc, u32 *state, void *param, void *procWork) {
    GeonetWork *work = procWork;
    int heapId = work->heapId;

    func_02021c44(work->queue);
    func_02021a18(work->queue);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(2, FALSE);
    GFL_BGSysSetBGEnabled(7, FALSE);
    GFL_BGSysSetBGEnabled(3, FALSE);
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_MAIN_SUB);
    KeyCursor_Free(work->keyCursor);
    GFL_TCBExMgrFree(work->tcbManager);
    GFL_WordSetSystemFree(work->wordSet);
    Geonet_FreeGraphics();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    return TRUE;
}

static void Geonet_SetVRAMBanks(void) {
    BGSysVRAMConfig config = {
        GX_VRAM_C, GX_VRAM_NONE, GX_VRAM_H, GX_VRAM_NONE, GX_VRAM_F, GX_VRAM_NONE,
        GX_VRAM_I, GX_VRAM_NONE, GX_VRAM_A | GX_VRAM_B, GX_VRAM_E, 0x10, 0x10,
    };

    gfxSetLCDCBanks(GX_VRAM_LCDC_ALL);
    sys_memset32_fast(0, (void *)HW_LCDC_VRAM, HW_LCDC_VRAM_SIZE);
    gfxDisableLCDCBanks();
    sys_memset32_fast(0xc0, (void *)HW_OAM, HW_OAM_SIZE);
    sys_memset32_fast(0, (void *)HW_BG_PLTT, HW_PLTT_SIZE);
    sys_memset32_fast(0xc0, (void *)HW_DB_OAM, HW_OAM_SIZE);
    sys_memset32_fast(0, (void *)HW_DB_BG_PLTT, HW_PLTT_SIZE);
    GFL_BGSysSetVRAMBanks(&config);
}

static void Geonet_InitGraphics(HeapID heapId) {
    BGSysLCDConfig config = {GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, TRUE};

    GFL_BGSysCreate(heapId);
    GFL_BGSysSetLCDConfig(&config);
    GFL_G3DSysCreate(FALSE, 3, FALSE, 2, 0, heapId, NULL);
    GFL_BGSysSet3DBGPriority(1);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysEnableEngines();
}

static void Geonet_FreeGraphics(void) {
    BmpWin_FreeAllocator();
    GFL_G3DSysFree();
    GFL_BGSysFree();
}

// Reads where every country and region is on the globe. The first entry of each file is not a place
static void Geonet_LoadPlaces(GeonetWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GEONET_PLACE, work->heapId);
    u32 size;
    u32 regionSize;
    GeonetCountryPlace *countries;
    GeonetRegionPlace *regions;
    GeonetCountryPlace *country;
    GeonetRegionPlace *region;
    int subdividedCount;
    int regionCount;
    int count;
    int i;
    int j;

    work->placeCount = 0;
    countries = GFL_ArcToolReadHeapNewLZGetLen(arc, NARC_GEONET_PLACE_COUNTRIES, FALSE, work->heapId, &size);
    count = size / sizeof(GeonetCountryPlace);
    country = countries + 1;
    for (i = 1; i < count; i++) {
        // A country with regions has its regions' places instead
        if (country->state != 2) {
            Geonet_AddPlace(work, work->placeCount, country->x, country->y, i, 0);
            work->placeCount++;
        }
        country++;
    }
    GFL_HeapFree(countries);

    for (j = 1, subdividedCount = Country_GetSubdividedCount(); j < subdividedCount; j++) {
        regions = GFL_ArcToolReadHeapNewLZGetLen(arc, Country_GetPlaceDataID(j), FALSE, work->heapId, &regionSize);
        regionCount = regionSize / sizeof(GeonetRegionPlace);
        region = regions + 1;
        for (i = 1; i < regionCount; i++) {
            Geonet_AddPlace(work, work->placeCount, region->x, region->y, Country_GetSubdividedCountryCode(j), i);
            work->placeCount++;
            region++;
        }
        GFL_HeapFree(regions);
    }
    GFL_ArcToolFree(arc);
}

static void Geonet_AddPlace(GeonetWork *work, int index, s16 x, s16 y, u16 country, u16 region) {
    MtxFx33 rotation = {FX32_ONE, 0, 0, 0, FX32_ONE, 0, 0, 0, FX32_ONE};
    VecFx32 angles;

    work->places[index].x = x;
    work->places[index].y = y;
    angles.x = x;
    angles.y = y;
    angles.z = 0;
    Geonet_GetPlaceRotation(&rotation, &angles);
    work->places[index].rotation = rotation;
    work->places[index].state = func_02009ba4(work->survey, country, region);
    work->places[index].country = country;
    work->places[index].region = region;
}

// Marks the player's place as their own, and turns the globe to it
static void Geonet_MarkOwnPlace(GeonetWork *work) {
    u32 i;

    for (i = 0; i < work->placeCount; i++) {
        if (work->myCountry == work->places[i].country && work->myRegion == work->places[i].region) {
            work->places[i].state = UNITY_TOWER_REGION_OWN;
            work->rotation.x = work->places[i].x;
            work->rotation.y = work->places[i].y;
        }
    }
}

static u32 Geonet_GetSubdividedIndex(u32 country) {
    return getSubdividedCountryArrayNum(country);
}

// Turns the touch screen into keys: the buttons into X and B, dragging into the directions, and a tap into A
static void Geonet_ReadTouch(GeonetWork *work) {
    int keys = 0;
    s32 button;
    int keysX;
    int moveX;
    int keysY;
    int moveY;

    work->touchKeys = 0;
    button = func_0203da0c(sGeonetButtonRects);
    if (button == BUTTON_VIEW) {
        keys = PAD_BUTTON_X;
    } else if (button == BUTTON_QUIT) {
        keys = PAD_BUTTON_B;
    }

    if (work->touchPressed) {
        if (keys != 0) {
            work->touchKeys = keys;
            return;
        }
        work->touchState = 0;
        work->touchMoveX = 0;
        work->touchMoveY = 0;
        work->tapTimer = 0;
        work->touchKeys = 0;
        work->touchPrevX = work->touchX;
        work->touchPrevY = work->touchY;
        work->tapTimer = 4;
    }

    if (work->touchHeld) {
        switch (work->touchState) {
        case 0:
            if (work->tapTimer == 0) {
                work->touchState++;
            } else {
                work->tapTimer--;
            }
            // fallthrough
        case 1:
            if (keys == 0) {
                Geonet_GetTouchMove(work, work->touchPrevX, work->touchPrevY, &keysX, &moveX, &keysY, &moveY);
                work->touchKeys = keysX | keysY;
                work->touchMoveX = moveX;
                work->touchMoveY = moveY;
                work->touchPrevX = work->touchX;
                work->touchPrevY = work->touchY;
            }
            break;
        }
    } else {
        if (keys == 0 && work->tapTimer != 0 && work->touchMoveX < 4 && work->touchMoveY < 4) {
            work->touchKeys = PAD_BUTTON_A;
        }
        work->touchState = 0;
        work->touchMoveX = 0;
        work->touchMoveY = 0;
        work->tapTimer = 0;
    }
}

static void Geonet_GetTouchMove(GeonetWork *work, int prevX, int prevY, int *keysX, int *moveX, int *keysY,
                                int *moveY) {
    int dirX = 0;
    int dirY = 0;
    int x = 0;
    int y = 0;

    if (work->touchX != 0xffff) {
        x = work->touchX - prevX;
        if (x < 0) {
            x ^= -1;
            dirX = PAD_KEY_RIGHT;
        } else if (x > 0) {
            dirX = PAD_KEY_LEFT;
        }
    }
    *keysX = dirX;
    *moveX = x & 0x3f;

    if (work->touchY != 0xffff) {
        y = work->touchY - prevY;
        if (y < 0) {
            y ^= -1;
            dirY = PAD_KEY_DOWN;
        } else if (y > 0) {
            dirY = PAD_KEY_UP;
        }
    }
    *keysY = dirY;
    *moveY = y & 0x3f;
}

static BmpWin *Geonet_CreateWindow(const GeonetWindowSetup *setup, int show) {
    BmpWin *window =
        BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);

    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    if (show == TRUE) {
        BmpWin_FlushMap(window);
    }
    return window;
}

static void Geonet_LoadBG(GeonetWork *work, ArcTool *arc) {
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    GFL_TextRndGetGlobalColors(&work->textColors[0], &work->textColors[1], &work->textColors[2]);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);

    GFL_BGSysCreateBG(6, &sGeonetTextBGSetup, 0);
    GFL_BGSysClearScr(6);
    GFL_BGSysCreateBG(7, &sGeonetBackBGSetup, 0);
    GFL_BGSysLoadArcNCGRStatic(arc, NARC_GEONET_BG_NCGR, 7, 0, 0, FALSE, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, NARC_GEONET_BG_NCLR, PALTYPE_SUB_BG, 0, 0x80, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, NARC_GEONET_BG_NSCR, 7, 0, 0, FALSE, work->heapId);
    LoadSysMsgBox(6, FRAME_MESSAGE_CHAR, FRAME_MESSAGE_PALETTE, 0, work->heapId);
    LoadSysMsgBox(6, FRAME_MENU_CHAR, FRAME_MENU_PALETTE, 0, work->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x80, 0x20, work->heapId);
    GFL_BGSysClearCharCore(6, 0x20, 0, work->heapId);
    GFL_BGSysResetStdPalette(6, GX_RGB(19, 26, 17));
    work->windows[WINDOW_INFO].window = Geonet_CreateWindow(&sGeonetInfoWindow, TRUE);
    BmpWin_DrawFrame(work->windows[WINDOW_INFO].window, 0, FRAME_MESSAGE_CHAR, FRAME_MESSAGE_PALETTE);
    BmpWin_FlushChar(work->windows[WINDOW_INFO].window);
    work->messageState = 0;

    GFL_BGSysCreateBG(2, &sGeonetTextBGSetup, 0);
    GFL_BGSysClearScr(2);
    GFL_BGSysSet3DBGPriority(1);
    GFL_BGSysCreateBG(3, &sGeonetBackBGSetup, 0);
    GFL_BGSysLoadArcNCGRStatic(arc, NARC_GEONET_BG_NCGR, 3, 0, 0, FALSE, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, NARC_GEONET_BG_NCLR, PALTYPE_MAIN_BG, 0, 0x80, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, NARC_GEONET_BG_NSCR, 3, 0, 0, FALSE, work->heapId);
    LoadSysMsgBox(2, FRAME_MENU_CHAR, FRAME_MENU_PALETTE, 0, work->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x80, 0x20, work->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x100, 0x20, work->heapId);
    GFL_BGSysClearCharCore(2, 0x20, 0, work->heapId);
    GFL_BGSysResetStdPalette(2, GX_RGB(0, 0, 0));
    Geonet_InitButtons(work);
}

static void Geonet_InitButtons(GeonetWork *work) {
    work->buttonText[BUTTON_VIEW] = GFL_MsgDataLoadStrbufNew(work->msgData, EventMapchange2_Text_View);
    work->buttonText[BUTTON_QUIT] = GFL_MsgDataLoadStrbufNew(work->msgData, EventMapchange2_Text_Quit);
    work->buttonRes = AppTaskMenuRes_Create(2, 9, work->font, work->queue, work->heapId);
    work->buttonItems[BUTTON_VIEW].str = work->buttonText[BUTTON_VIEW];
    work->buttonItems[BUTTON_VIEW].color = PRINT_COLOR(14, 15, 3);
    work->buttonItems[BUTTON_VIEW].type = 0;
    work->buttonItems[BUTTON_QUIT].str = work->buttonText[BUTTON_QUIT];
    work->buttonItems[BUTTON_QUIT].color = PRINT_COLOR(14, 15, 3);
    work->buttonItems[BUTTON_QUIT].type = APP_TASKMENU_ITEM_RETURN;
}

static void Geonet_CreateButton(GeonetWork *work, int button) {
    if (work->buttons[button] == NULL) {
        work->buttons[button] =
            AppTaskMenuWin_Create(work->buttonRes, &work->buttonItems[button], sGeonetButtonPos[button].x,
                                  sGeonetButtonPos[button].y, sGeonetButtonPos[button].width, work->heapId);
    }
}

static void Geonet_FreeButton(GeonetWork *work, int button) {
    if (work->buttons[button] != NULL) {
        AppTaskMenuWin_Free(work->buttons[button]);
        GFL_BGSysQueueScrLoad(2);
        work->buttons[button] = NULL;
    }
}

static void Geonet_FreeBG(GeonetWork *work) {
    Geonet_FreeButton(work, BUTTON_VIEW);
    Geonet_FreeButton(work, BUTTON_QUIT);
    AppTaskMenuRes_Free(work->buttonRes);
    GFL_StrBufFree(work->buttonText[BUTTON_VIEW]);
    GFL_StrBufFree(work->buttonText[BUTTON_QUIT]);
    work->buttons[BUTTON_VIEW] = NULL;
    work->buttons[BUTTON_QUIT] = NULL;
    BmpWin_Free(work->windows[WINDOW_INFO].window);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(7);
    GFL_FontFree(work->font);
    GFL_TextRndUpdateColorIndexLUT(work->textColors[0], work->textColors[1], work->textColors[2]);
}

static void Geonet_Print(GeonetWork *work, PrintWindow *print, BmpWin *window, StrBuf *strbuf, u8 x, u8 y) {
    PrintWindow_Init(print, window);
    PrintWindow_PrintNoColor(print, work->queue, x, y, strbuf, work->font);
}

// Prints a message a character at a time. Returns whether it is done, which is once A is pressed unless `close`
static BOOL Geonet_PrintMessage(GeonetWork *work, u32 message, BOOL close) {
    BOOL done = FALSE;

    switch (work->messageState) {
    case 0:
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_INFO].window), 15);
        work->message = GFL_StrBufCreate(0x400, work->heapId);
        GFL_MsgDataLoadStrbuf(work->msgData, message, work->message);
        work->stream = func_02022268(work->windows[WINDOW_INFO].window, 0, 0, work->message, work->font,
                                     func_02017bcc(), work->tcbManager, 0, work->heapId, 15);
        work->messageState = 1;
        break;
    case 1: {
        u32 streamState = func_020223b4(work->stream);

        KeyCursor_Update(work->keyCursor, work->stream, work->windows[WINDOW_INFO].window);
        switch (streamState) {
        case PRINT_STREAM_RUNNING:
            if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_A) {
                func_020223e0(work->stream, 0);
            }
            break;
        case PRINT_STREAM_PAUSED:
            if ((work->keysPressed & PAD_BUTTON_A) || (work->keysPressed & PAD_BUTTON_B)) {
                func_020223bc(work->stream);
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            break;
        case PRINT_STREAM_DONE:
            work->messageState = 2;
            break;
        }
        break;
    }
    case 2:
        if (close || (work->keysPressed & PAD_BUTTON_A)) {
            GFL_StrBufFree(work->message);
            func_020223cc(work->stream);
            work->messageState = 0;
            done = TRUE;
        }
        break;
    }
    return done;
}

static void Geonet_ListCursorCallback(BmpMenuList *list, s32 value, u8 init) {
    if (!init) {
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
    }
}

static void Geonet_CreateMenu(GeonetWork *work, const GeonetWindowSetup *setup, const BmpMenuListHeader *header,
                              const GeonetMenuItem *items) {
    BmpMenuListHeader listHeader;
    int i;

    work->listWindow = Geonet_CreateWindow(setup, TRUE);
    work->options = ListMenuCore_CreateOptionList(header->count, work->heapId);
    for (i = 0; i < header->count; i++) {
        ListMenuCore_AppendMsgOption(work->options, work->msgData, items[i].message, items[i].value, work->heapId);
    }
    PrintWindow_Init(&work->listPrint, work->listWindow);
    listHeader = *header;
    listHeader.options = work->options;
    listHeader.cursorCallback = Geonet_ListCursorCallback;
    listHeader.fontSizeX = 12;
    listHeader.fontSizeY = GFL_FontGetCharHeight(work->font);
    listHeader.msgData = work->msgData;
    listHeader.printWindow = &work->listPrint;
    listHeader.queue = work->queue;
    listHeader.font = work->font;
    work->list = BmpMenuList_Create(&listHeader, 0, 0, work->heapId);
    BmpMenuList_LoadCursor(work->list, work->heapId);
    BmpWin_DrawFrame(work->listWindow, 0, FRAME_MENU_CHAR, FRAME_MENU_PALETTE);
    BmpWin_FlushChar(work->listWindow);
}

// Makes the list of the options that a country or region list has gathered
static void Geonet_CreateList(GeonetWork *work, const BmpMenuListHeader *header, u32 count) {
    BmpMenuListHeader listHeader;

    PrintWindow_Init(&work->listPrint, work->listWindow);
    listHeader = *header;
    listHeader.options = work->options;
    listHeader.count = count;
    listHeader.cursorCallback = Geonet_ListCursorCallback;
    listHeader.fontSizeX = 12;
    listHeader.fontSizeY = GFL_FontGetCharHeight(work->font);
    listHeader.printWindow = &work->listPrint;
    listHeader.queue = work->queue;
    listHeader.font = work->font;
    work->list = BmpMenuList_Create(&listHeader, 0, 0, work->heapId);
    BmpMenuList_LoadCursor(work->list, work->heapId);
    BmpWin_DrawFrame(work->listWindow, 0, FRAME_MENU_CHAR, FRAME_MENU_PALETTE);
    BmpWin_FlushChar(work->listWindow);
}

static void Geonet_CreateCountryList(GeonetWork *work, const GeonetWindowSetup *setup,
                                     const BmpMenuListHeader *header, u32 textBank) {
    MsgData *msgData;
    int count;
    int i;

    work->listWindow = Geonet_CreateWindow(setup, TRUE);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, textBank, work->heapId);
    count = gSelectableCountryCount;
    work->options = ListMenuCore_CreateOptionList(count, work->heapId);
    for (i = 0; i < count; i++) {
        ListMenuCore_AppendMsgOption(work->options, msgData, gSelectableCountries[i], gSelectableCountries[i],
                                     work->heapId);
    }
    GFL_MsgDataFree(msgData);
    Geonet_CreateList(work, header, count);
}

static void Geonet_CreateRegionList(GeonetWork *work, const GeonetWindowSetup *setup,
                                    const BmpMenuListHeader *header, u32 textBank, const u8 *order, u32 count) {
    MsgData *msgData;
    u32 i;

    work->listWindow = Geonet_CreateWindow(setup, TRUE);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, textBank, work->heapId);
    work->options = ListMenuCore_CreateOptionList(gSelectableCountryCount, work->heapId);
    for (i = 0; i < count; i++) {
        ListMenuCore_AppendMsgOption(work->options, msgData, order[i], order[i], work->heapId);
    }
    GFL_MsgDataFree(msgData);
    Geonet_CreateList(work, header, count);
}

static void Geonet_FreeList(GeonetWork *work) {
    BmpWin_ClearFrame(work->listWindow, 0);
    BmpWin_Free(work->listWindow);
    BmpMenuList_Free(work->list, NULL, NULL);
    ListMenuCore_FreeOptionList(work->options);
}

static void Geonet_ShowRegisteredPlace(GeonetWork *work) {
    StrBuf *strbuf = GFL_StrBufCreate(0x400, work->heapId);
    StrBuf *format = GFL_StrBufCreate(0x400, work->heapId);

    work->windows[WINDOW_PLACE].window = Geonet_CreateWindow(&sGeonetPlaceWindow, TRUE);
    loadCountryToStrbuf(work->wordSet, 0, work->myCountry);
    loadCountryAreaToStrbuf(work->wordSet, 1, work->myCountry, work->myRegion);
    GFL_MsgDataLoadStrbuf(work->msgData, EventMapchange2_Text_RegisteredLocation, format);
    GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
    Geonet_Print(work, &work->windows[WINDOW_PLACE].print, work->windows[WINDOW_PLACE].window, strbuf, 0, 0);
    GFL_StrBufFree(format);
    GFL_StrBufFree(strbuf);
    BmpWin_DrawFrame(work->windows[WINDOW_PLACE].window, 0, FRAME_MENU_CHAR, FRAME_MENU_PALETTE);
    BmpWin_FlushChar(work->windows[WINDOW_PLACE].window);
}

static void Geonet_ShowSelectedPlace(GeonetWork *work, u32 country, u32 region) {
    StrBuf *countryName = GFL_StrBufCreate(64, work->heapId);
    StrBuf *regionName = GFL_StrBufCreate(64, work->heapId);

    work->windows[WINDOW_PLACE].window = Geonet_CreateWindow(&sGeonetPlaceWindow, TRUE);
    Geonet_LoadPlaceNames(country, region, countryName, regionName, work->heapId);
    if (region != 0) {
        Geonet_Print(work, &work->windows[WINDOW_PLACE].print, work->windows[WINDOW_PLACE].window, regionName, 0,
                     16);
    }
    Geonet_Print(work, &work->windows[WINDOW_PLACE].print, work->windows[WINDOW_PLACE].window, countryName, 0, 0);
    GFL_StrBufFree(regionName);
    GFL_StrBufFree(countryName);
    BmpWin_DrawFrame(work->windows[WINDOW_PLACE].window, 0, FRAME_MENU_CHAR, FRAME_MENU_PALETTE);
    BmpWin_Transfer(work->windows[WINDOW_PLACE].window);
}

static void Geonet_ClosePlaceWindow(GeonetWork *work) {
    BmpWin_ClearFrame(work->windows[WINDOW_PLACE].window, 0);
    BmpWin_Free(work->windows[WINDOW_PLACE].window);
}

// Finds the marked place nearest the middle of the screen, among those within 0x80 of it. Returns whether there is
// one
static BOOL Geonet_FindNearestPlace(GeonetWork *work, u32 *index) {
    BOOL found = FALSE;
    s16 minX = work->rotation.x - 0x80;
    s16 maxX = work->rotation.x + 0x80;
    int minY = (s16)(work->rotation.y - 0x80);
    int maxY = (s16)(work->rotation.y + 0x80);
    int wrapMinY;
    int wrapMaxY;
    u32 nearest = work->placeCount;
    u32 nearestDistance = 0x100;
    GeonetAngles center;
    GeonetAngles angles;
    u32 i;

    center.x = work->rotation.x;
    center.y = work->rotation.y;
    Geonet_WrapAngles(&center);

    // The range crosses the 180th meridian
    if (MATH_ABS(maxY - minY) > 0x100) {
        wrapMaxY = wrapMinY = (s16)work->rotation.y;
        if (wrapMinY > 0) {
            maxY = minY = wrapMinY - 0x10000;
        } else {
            maxY = minY = wrapMinY + 0x10000;
        }
        wrapMinY -= 0x80;
        wrapMaxY += 0x80;
        minY -= 0x80;
        maxY += 0x80;
    } else {
        wrapMaxY = maxY;
        wrapMinY = minY;
    }

    for (i = 0; i < work->placeCount; i++) {
        if (work->places[i].x > minX && work->places[i].x < maxX
            && ((work->places[i].y > minY && work->places[i].y < maxY)
                || (work->places[i].y > wrapMinY && work->places[i].y < wrapMaxY))
            && work->places[i].state != UNITY_TOWER_REGION_NONE) {
            u32 distance;

            angles.x = work->places[i].x;
            angles.y = work->places[i].y;
            Geonet_WrapAngles(&angles);
            distance = Geonet_GetDistance(&center, &angles);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearest = i;
            }
        }
    }

    *index = nearest;
    if (nearest != work->placeCount) {
        found = TRUE;
    }
    return found;
}

// Shows how to use the globe, or the name of the place at the middle of the screen, which the globe turns to
static void Geonet_UpdateInfo(GeonetWork *work) {
    u32 index;

    if (work->infoMode == FALSE) {
        StrBuf *strbuf = GFL_StrBufCreate(0x400, work->heapId);

        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_INFO].window), 15);
        GFL_MsgDataLoadStrbuf(work->msgData, EventMapchange2_Text_DisplayRegisteredLocationsLine, strbuf);
        Geonet_Print(work, &work->windows[WINDOW_INFO].print, work->windows[WINDOW_INFO].window, strbuf, 0, 0);
        GFL_StrBufFree(strbuf);
    } else if (Geonet_FindNearestPlace(work, &index)) {
        StrBuf *countryName = GFL_StrBufCreate(64, work->heapId);
        StrBuf *regionName = GFL_StrBufCreate(64, work->heapId);

        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_INFO].window), 15);
        Geonet_LoadPlaceNames(work->places[index].country, work->places[index].region, countryName, regionName,
                              work->heapId);
        if (work->places[index].region != 0) {
            Geonet_Print(work, &work->windows[WINDOW_INFO].print, work->windows[WINDOW_INFO].window, regionName, 0,
                         16);
        }
        Geonet_Print(work, &work->windows[WINDOW_INFO].print, work->windows[WINDOW_INFO].window, countryName, 0, 0);
        GFL_StrBufFree(regionName);
        GFL_StrBufFree(countryName);
        work->rotation.x = work->places[index].x;
        work->rotation.y = work->places[index].y;
    } else {
        work->infoMode = FALSE;
    }
    BmpWin_FlushChar(work->windows[WINDOW_INFO].window);
}

static void Geonet_Load3D(GeonetWork *work, ArcTool *arc) {
    u32 i;

    work->earthRes = GFL_G3DSysReadArcSysResource(ARCID_GEONET, NARC_GEONET_EARTH_NSBMD);
    GFL_G3DResUploadTexData(work->earthRes);
    work->earthModel = GFL_G3DMdlCreate(work->earthRes, 0, work->earthRes);
    work->earthActor = GFL_G3DActorCreate(work->earthModel, NULL, 0);
    for (i = 0; i < NELEMS(sGeonetMarkFiles); i++) {
        u16 mark = sGeonetMarkFiles[i].mark;

        work->markRes[mark] = GFL_G3DSysReadArcSysResource(ARCID_GEONET, sGeonetMarkFiles[i].file);
        GFL_G3DResUploadTexData(work->markRes[mark]);
        work->markModels[mark] = GFL_G3DMdlCreate(work->markRes[mark], 0, work->markRes[mark]);
        work->markActors[mark] = GFL_G3DActorCreate(work->markModels[mark], NULL, 0);
    }
    Geonet_CreateCamera(work);
    Geonet_CreateLight(work);
}

static void Geonet_Free3D(GeonetWork *work) {
    u32 i;

    GFL_G3DCameraFree(work->camera);
    GFL_G3DLightFree(work->light);
    GFL_G3DActorFree(work->earthActor);
    GFL_G3DMdlFree(work->earthModel);
    GFL_G3DResFreeTexData(work->earthRes);
    GFL_G3DResFree(work->earthRes);
    for (i = 0; i < NELEMS(sGeonetMarkFiles); i++) {
        u16 mark = sGeonetMarkFiles[i].mark;

        GFL_G3DActorFree(work->markActors[mark]);
        GFL_G3DMdlFree(work->markModels[mark]);
        GFL_G3DResFreeTexData(work->markRes[mark]);
        GFL_G3DResFree(work->markRes[mark]);
    }
    work->earthActor = NULL;
}

static void Geonet_InitEarth(GeonetWork *work) {
    work->translation.x = 0;
    work->translation.y = 0;
    work->translation.z = 0;
    work->scale.x = FX32_ONE;
    work->scale.y = FX32_ONE;
    work->scale.z = FX32_ONE;
    work->rotation.x = 0x1a40;
    work->rotation.y = 0x7c00;
    work->rotation.z = 0;
    work->markScale.x = FX32_ONE;
    work->markScale.y = FX32_ONE;
    work->markScale.z = FX32_ONE;
}

static void Geonet_CreateCamera(GeonetWork *work) {
    HeapID heapId = work->heapId;
    VecFx32 up = {0, FX32_ONE, 0};

    work->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                       FX_CosIdx(DEG_TO_IDX(20)), FX32_ONE * 4 / 3, 0, FX32_ONE, FX32_ONE * 1024, 0,
                                       &sGeonetCameraPos, &up, &sGeonetCameraTarget, heapId);
    GFL_G3DCameraFlush(work->camera);
}

// Puts the camera where it starts: far from the globe if the player has no place or lives in the United States, and
// near it otherwise
static void Geonet_ResetCamera(GeonetWork *work) {
    fx32 near = 0;
    fx32 far = FX32_ONE * 100;

    GFL_G3DCameraSetLookatPos(work->camera, &sGeonetCameraPos);
    GFL_G3DCameraSetLookatTarget(work->camera, &sGeonetCameraTarget);
    GFL_G3DCameraPerspectiveSetFOVSin(work->camera, FX_SinIdx(0x5c1));
    GFL_G3DCameraPerspectiveSetFOVCos(work->camera, FX_CosIdx(0x5c1));
    GFL_G3DCameraSetProjectionZNear(work->camera, &near);
    GFL_G3DCameraSetProjectionZFar(work->camera, &far);

    if (work->worldMode == FALSE) {
        work->zoom = TRUE;
    } else if (work->myCountry == COUNTRY_NONE) {
        work->zoom = FALSE;
    } else if (work->myCountry == COUNTRY_USA) {
        work->zoom = FALSE;
    } else {
        work->zoom = TRUE;
    }
    while (Geonet_MoveCamera(work) != TRUE) {
    }
}

static void Geonet_CreateLight(GeonetWork *work) {
    work->light = GFL_G3DLightCreate(&sGeonetLightList, work->heapId);
    GFL_G3DLightFlush(work->light);
}

static void Geonet_GetEarthRotation(MtxFx33 *mtx, const VecFx32 *rotation) {
    MtxFx33 tmp;

    MTX_RotY33_(mtx, FX_SinIdx((u16)rotation->y), FX_CosIdx((u16)rotation->y));
    MTX_RotX33_(&tmp, FX_SinIdx((u16)rotation->x), FX_CosIdx((u16)rotation->x));
    MTX_Concat33(mtx, &tmp, mtx);
    MTX_RotZ33_(&tmp, FX_SinIdx((u16)rotation->z), FX_CosIdx((u16)rotation->z));
    MTX_Concat33(mtx, &tmp, mtx);
}

// The rotation that puts a mark at a place. The sine and cosine of the last rotation are in each other's place,
// which turns the mark a quarter of the way around
static void Geonet_GetPlaceRotation(MtxFx33 *mtx, const VecFx32 *rotation) {
    MtxFx33 tmp;

    MTX_RotY33_(mtx, FX_SinIdx((u16)rotation->x), FX_CosIdx((u16)rotation->x));
    MTX_RotX33_(&tmp, FX_SinIdx((u16)-rotation->y), FX_CosIdx((u16)-rotation->y));
    MTX_Concat33(mtx, &tmp, mtx);
    MTX_RotZ33_(&tmp, FX_CosIdx((u16)rotation->z), FX_SinIdx((u16)rotation->z));
    MTX_Concat33(mtx, &tmp, mtx);
}

// Turns the globe with the keys or the touch screen, and A moves the camera. Returns whether a key did anything
static BOOL Geonet_Rotate(GeonetWork *work, u32 pressed, u32 held) {
    u16 speedX;
    u16 speedY;
    BOOL moved = FALSE;
    s16 rotationX = work->rotation.x;
    s16 rotationY = work->rotation.y;

    if ((pressed & PAD_BUTTON_A) || (work->touchKeys & PAD_BUTTON_A)) {
        if (work->worldMode == TRUE) {
            if (work->zoom == FALSE) {
                work->zoom = TRUE;
            } else {
                work->zoom = FALSE;
            }
        }
        return TRUE;
    }

    if (work->zoom == FALSE) {
        if (work->touchMoveX != 0 || work->touchMoveY != 0) {
            speedX = work->touchMoveX * 0x55;
            speedY = work->touchMoveY * 0x55;
        } else {
            speedX = 0x200;
            speedY = 0x200;
        }
    } else {
        if (work->touchMoveX != 0 || work->touchMoveY != 0) {
            speedX = work->touchMoveX * 10;
            speedY = work->touchMoveY * 10;
        } else {
            speedX = 0x20;
            speedY = 0x20;
        }
    }

    if ((held & PAD_KEY_LEFT) || (work->touchKeys & PAD_KEY_LEFT)) {
        if (work->worldMode == TRUE) {
            work->rotation.y += speedX;
        } else if (rotationY < JAPAN_ROTATION_Y_MAX) {
            work->rotation.y += speedX;
        }
        moved = TRUE;
    }
    if ((held & PAD_KEY_RIGHT) || (work->touchKeys & PAD_KEY_RIGHT)) {
        if (work->worldMode == TRUE) {
            work->rotation.y -= speedX;
        } else if (rotationY > JAPAN_ROTATION_Y_MIN) {
            work->rotation.y -= speedX;
        }
        moved = TRUE;
    }
    if ((held & PAD_KEY_UP) || (work->touchKeys & PAD_KEY_UP)) {
        if (work->worldMode == TRUE) {
            if (rotationX + speedY < WORLD_ROTATION_X_MAX) {
                work->rotation.x += speedY;
            } else {
                work->rotation.x = WORLD_ROTATION_X_MAX;
            }
        } else if (rotationX < JAPAN_ROTATION_X_MAX) {
            work->rotation.x += speedY;
        }
        moved = TRUE;
    }
    if ((held & PAD_KEY_DOWN) || (work->touchKeys & PAD_KEY_DOWN)) {
        if (work->worldMode == TRUE) {
            if (rotationX - speedY > -WORLD_ROTATION_X_MAX) {
                work->rotation.x -= speedY;
            } else {
                work->rotation.x = -WORLD_ROTATION_X_MAX;
            }
        } else if (rotationX > JAPAN_ROTATION_X_MIN) {
            work->rotation.x -= speedY;
        }
        moved = TRUE;
    }
    return moved;
}

// Moves the camera a step toward where the zoom puts it, the marks growing as it moves away. Returns whether it is
// there
static BOOL Geonet_MoveCamera(GeonetWork *work) {
    VecFx32 pos;
    VecFx32 target;
    VecFx32 direction;
    fx32 distance;
    BOOL done = FALSE;

    GFL_G3DCameraGetLookatPos(work->camera, &pos);
    GFL_G3DCameraGetLookatTarget(work->camera, &target);
    VEC_Subtract(&pos, &target, &direction);
    distance = VEC_Mag(&direction);
    VEC_Normalize(&direction, &direction);

    switch (work->zoom) {
    case TRUE:
        if (distance > CAMERA_DISTANCE_NEAR + CAMERA_SPEED) {
            distance -= CAMERA_SPEED;
            work->markScale.x -= 0x80;
            work->markScale.y = work->markScale.x;
        } else {
            distance = CAMERA_DISTANCE_NEAR;
            done = TRUE;
        }
        break;
    case FALSE:
        if (distance < CAMERA_DISTANCE_FAR - CAMERA_SPEED) {
            distance += CAMERA_SPEED;
            work->markScale.x += 0x80;
            work->markScale.y = work->markScale.x;
        } else {
            distance = CAMERA_DISTANCE_FAR;
            done = TRUE;
        }
        break;
    }

    VEC_MultAdd(distance, &direction, &target, &pos);
    GFL_G3DCameraSetLookatPos(work->camera, &pos);
    return done;
}

static void Geonet_Draw(GeonetWork *work) {
    SRTMatrix status;
    SRTMatrix markStatus;
    u32 i;

    switch (work->drawState) {
    case DRAW_NONE:
        break;
    case DRAW_CLEAR:
        GFL_G3DSysReset();
        GFL_G3DSysReqSwapBuffers();
        work->drawState = DRAW_NONE;
        break;
    case DRAW_EARTH:
        GFL_G3DLightFlush(work->light);
        GFL_G3DCameraFlush(work->camera);
        GFL_G3DSysReset();
        GFL_G3DSysMtxViewFlush();

        status.translation = work->translation;
        MTX_Identity33_(&status.rotation);
        status.scale = work->markScale;
        GFL_G3DSysDrawObj(work->markActors[MARK_CURSOR], &status);

        status.scale = work->scale;
        Geonet_GetEarthRotation(&status.rotation, &work->rotation);
        GFL_G3DSysDrawObj(work->earthActor, &status);

        markStatus.translation = work->translation;
        markStatus.scale = work->markScale;
        for (i = 0; i < work->placeCount; i++) {
            MTX_Concat33(&work->places[i].rotation, &status.rotation, &markStatus.rotation);
            if (work->places[i].state != UNITY_TOWER_REGION_NONE) {
                GFL_G3DSysDrawObj(work->markActors[work->places[i].state], &markStatus);
            }
        }
        GFL_G3DSysReqSwapBuffers();
        break;
    }
}

// Loads the names of a country and of its region. Returns whether the country has regions
static BOOL Geonet_LoadPlaceNames(u32 country, u32 region, StrBuf *countryName, StrBuf *regionName, int heapId) {
    u32 subdivided = Geonet_GetSubdividedIndex(country);
    BOOL hasRegions = FALSE;
    MsgData *msgData;

    msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_MAPCHANGE_LOAD_COUNTRY_TO_STRBUF, heapId);
    GFL_MsgDataLoadStrbuf(msgData, country, countryName);
    GFL_MsgDataFree(msgData);
    if (subdivided == 0) {
        subdivided = 1;
        region = 0;
    } else {
        hasRegions = TRUE;
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, getFileNumForSubdividedRegion(subdivided), heapId);
    GFL_MsgDataLoadStrbuf(msgData, region, regionName);
    GFL_MsgDataFree(msgData);
    return hasRegions;
}

static BOOL Geonet_IsSubdivided(u32 country) {
    if (Geonet_GetSubdividedIndex(country) != 0) {
        return TRUE;
    }
    return FALSE;
}

// Brings angles into 0 to 0xfffe
static void Geonet_WrapAngles(GeonetAngles *angles) {
    if (angles->x >= 0) {
        angles->x %= 0xffff;
    } else {
        angles->x += (MATH_ABS(angles->x) / 0xffff + 1) * 0xffff;
    }
    if (angles->y >= 0) {
        angles->y %= 0xffff;
    } else {
        angles->y += (MATH_ABS(angles->y) / 0xffff + 1) * 0xffff;
    }
}

static u32 Geonet_GetDistance(const GeonetAngles *a, const GeonetAngles *b) {
    int x = MATH_ABS(a->x - b->x);
    int y = MATH_ABS(a->y - b->y);

    if (x > func_020443b4(180)) {
        x = 0xffff - x;
    }
    if (y > func_020443b4(180)) {
        y = 0xffff - y;
    }
    return FX_Sqrt((x * x + y * y) << FX32_SHIFT) >> FX32_SHIFT;
}
