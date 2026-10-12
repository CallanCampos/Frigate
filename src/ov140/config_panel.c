#include "types.h"
#include "app/config_panel.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "save/config.h"
#include "save/save_control.h"
#include "save/trainer_data_savedata.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "text/script/global_10520.h"
#include "text/system/title_2.h"

// The options screen of the menu: five options in rows that scroll, each with two or three values to pick with the
// keys or the touch screen, an explanation of the row under the cursor, and the buttons that confirm the changes or
// quit. The ROM has no name for this file; "config_panel.c" is a guess

#define HEAPID_CONFIG_PANEL 0x1c
// The screen's graphics
#define ARCID_CONFIG_PANEL 27
// The options screen's shortcut
#define SHORTCUT_CONFIG_PANEL 27

#define BG_COUNT 6
#define BG_BAR 0
#define BG_TEXT 1
#define BG_ROWS 2
#define BG_CONFIRM 3
#define BG_MESSAGE 4

// The windows: the values, the options' names, the title, the explanation and the question
#define WINDOW_COUNT 19
#define LABEL_COUNT 16
#define WINDOW_OPTION 11
#define WINDOW_TITLE 16
#define WINDOW_MESSAGE 17
#define WINDOW_CONFIRM 18

// The actors: the values' frames from ACTOR_ITEM on
#define ACTOR_COUNT 14
#define ACTOR_ITEM 2

// The options and all their values
#define OPTION_COUNT 5
#define OPTION_TEXT_SPEED 0
#define OPTION_BATTLE_SCENE 1
#define OPTION_BATTLE_STYLE 2
#define OPTION_SOUND 3
#define OPTION_SAVE_BEFORE_IR 4
#define ITEM_COUNT 11

// The cursor: an option, or one of the buttons
#define CURSOR_CONFIRM 5
#define CURSOR_QUIT 6
#define CURSOR_MAX 6

// The top and bottom of the rows on the screen
#define ROWS_TOP 24
#define ROWS_BOTTOM 168

// What the player did this frame
enum {
    INPUT_NONE,
    INPUT_SLIDE,
    INPUT_2,
    INPUT_TOUCH,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_RIGHT,
    INPUT_LEFT,
    INPUT_DECIDE,
    INPUT_CANCEL,
    INPUT_UP_HELD,
    INPUT_DOWN_HELD,
    INPUT_Y,
    INPUT_X,
    // The first key after the touch screen, which only brings the cursor back
    INPUT_KEY_MODE,
};

// The points the input keeps: where the touch began, how far it moved this frame, and where it is
enum {
    POINT_START,
    POINT_SLIDE,
    POINT_NOW,
};

// The buttons of the bar. The touch bar's close icon is BUTTON_CLOSE
enum {
    BUTTON_CONFIRM,
    BUTTON_QUIT,
    BUTTON_CLOSE,
    BUTTON_NONE,
};

// How the message window prints
enum {
    MSG_MODE_NONE,
    MSG_MODE_STREAM,
    MSG_MODE_QUEUE,
};

// The answer to the question
enum {
    CONFIRM_NONE,
    CONFIRM_YES,
    CONFIRM_NO,
};

// The touch bar's own icons that the screen uses
#define TOUCHBAR_ICON_CLOSE 0
#define TOUCHBAR_ICON_CHECK 6

typedef struct {
    s32 x;
    s32 y;
} ConfigPanelPoint;

typedef struct {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
} ConfigPanelRect;

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
} ConfigPanelBgSetup;

// A text of the screen: its message and its window, in tiles. The width is set from the text
typedef struct {
    u32 msgId;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
} ConfigPanelLabel;

// An option: its explanation and its values
typedef struct {
    u16 msgId;
    u16 count;
    u8 items[4];
} ConfigPanelOption;

// A value of an option: where its frame is in the rows, and the area that picks it
typedef struct {
    u32 option;
    u32 value;
    s32 x;
    s32 y;
    s16 hitX;
    s16 hitY;
    s16 hitWidth;
    s16 hitHeight;
} ConfigPanelItem;

// The options as the screen edits them. changed is the value last picked, ITEM_COUNT when none
typedef struct {
    u8 values[OPTION_COUNT];
    u8 changed;
    Config *config;
} ConfigPanelOptions;

typedef struct {
    BmpWin *windows[WINDOW_COUNT];
    u16 pulseAngle;
    u16 pad4E;
    u16 pulseColor;
    void *titleCharBuf;
    NNSG2dCharacterData *titleChars;
    u16 cursorPalette[16];
} ConfigPanelBg;

typedef struct {
    ClActUnit *unit;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    u32 knobChars;
    u32 knobCellAnims;
    ClActor *actors[ACTOR_COUNT];
    u16 frame;
} ConfigPanelObj;

typedef struct {
    ConfigPanelBg bg;
    ConfigPanelObj obj;
    TCB *vblankTcb;
} ConfigPanelGraphic;

typedef struct ConfigPanelSeq ConfigPanelSeq;
typedef void (*ConfigPanelSeqFunc)(ConfigPanelSeq *seq, u32 *state, void *data);

struct ConfigPanelSeq {
    ConfigPanelSeqFunc func;
    BOOL ended;
    u32 state;
    void *data;
};

typedef struct {
    u32 input;
    ConfigPanelPoint start;
    ConfigPanelPoint slide;
    ConfigPanelPoint now;
    BOOL touching;
} ConfigPanelUi;

typedef struct {
    MsgData *msgData;
    Font *font;
    PrintStream *stream;
    TCBExManager *tcbManager;
    BmpWin *window;
    StrBuf *strbuf;
    u16 clearColor;
    u16 heapId;
    PrintWindow printWindow;
    PrintQueue *queue;
    u32 mode;
    AppPrintsysCommon common;
    u32 unk34;
} ConfigPanelMsg;

typedef struct {
    Font *font;
    PrintQueue *queue;
    TouchBar *touchBar;
    s32 button;
    BOOL decided;
    AppTaskMenuRes *res;
    MsgData *msgData;
    AppTaskMenuItem items[2];
    AppTaskMenuWin *wins[2];
    GameData *gameData;
    // Whether a button or an icon was touched this frame, which hides the cursor
    BOOL touched;
} ConfigPanelAppBar;

typedef struct {
    u16 textBg;
    u16 rowsBg;
    s32 cursor;
    ClActor *actors[ITEM_COUNT];
    s32 y;
    u32 repeat;
    u32 unk3C;
    u32 unk40;
    ConfigPanelOptions options;
    BOOL update;
    BOOL cursorVisible;
} ConfigPanelScroll;

typedef struct {
    ConfigPanelMsg msg;
    AppTaskMenu *menu;
    AppTaskMenuItem items[2];
    u16 result;
    u16 state;
    AppTaskMenuInit init;
    AppTaskMenuRes *res;
} ConfigPanelConfirm;

typedef struct {
    ConfigPanelGraphic graphic;
    ConfigPanelSeq seq;
    ConfigPanelUi ui;
    ConfigPanelAppBar appBar;
    ConfigPanelScroll scroll;
    ConfigPanelMsg msg;
    ConfigPanelConfirm confirm;
    // The options as they were
    ConfigPanelOptions saved;
    Font *font;
    PrintQueue *queue;
    MsgData *msgData;
    AppTaskMenuRes *buttonRes;
    AppTaskMenuRes *confirmRes;
    TCBExManager *tcbManager;
    ConfigPanelParam *param;
} ConfigPanelWork;

static BOOL ConfigPanel_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ConfigPanel_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ConfigPanel_Main(GameProc *proc, u32 *state, void *param, void *work);
static void ConfigPanelGraphic_Init(ConfigPanelGraphic *graphic, HeapID heapId);
static void ConfigPanelGraphic_Exit(ConfigPanelGraphic *graphic);
// Not static: overlay 169's data has two words equal to this function's address that dsd took for relocations
// (`overlays(140,167,...)`, really overlay 167's), and config_fixes.py can't resolve them in White 2, so the symbol
// must exist for the link.
void ConfigPanelGraphic_Main(ConfigPanelGraphic *graphic);
static ClActor *ConfigPanelGraphic_GetActor(ConfigPanelGraphic *graphic, u32 index);
static ClActUnit *ConfigPanelGraphic_GetUnit(ConfigPanelGraphic *graphic);
static BmpWin *ConfigPanelGraphic_GetWindow(ConfigPanelGraphic *graphic, u32 index);
static void ConfigPanelGraphic_ResetPulse(ConfigPanelGraphic *graphic);
static void ConfigPanelGraphic_VBlank(TCB *tcb, void *data);
static void ConfigPanelBg_Init(ConfigPanelBg *bg, HeapID heapId);
static void ConfigPanelBg_Exit(ConfigPanelBg *bg);
static void ConfigPanelBg_Main(ConfigPanelBg *bg);
static BmpWin *ConfigPanelBg_GetWindow(ConfigPanelBg *bg, u32 index);
static void ConfigPanelBg_PrintLabels(ConfigPanelBg *bg);
static void ConfigPanelBg_ResetPulse(ConfigPanelBg *bg);
static void ConfigPanelBg_VBlank(ConfigPanelBg *bg);
static void ConfigPanel_PulseColor(u16 *dest, u16 *angle, u16 speed, u8 palette, u8 index, u16 from, u16 to);
static void ConfigPanelObj_Init(ConfigPanelObj *obj, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void ConfigPanelObj_Exit(ConfigPanelObj *obj);
static void ConfigPanelObj_Main(ConfigPanelObj *obj);
static void ConfigPanelObj_VBlank(ConfigPanelObj *obj);
static ClActor *ConfigPanelObj_GetActor(ConfigPanelObj *obj, u32 index);
static ClActUnit *ConfigPanelObj_GetUnit(ConfigPanelObj *obj);
static void ConfigPanelUi_Init(ConfigPanelUi *ui, HeapID heapId);
static void ConfigPanelUi_Exit(ConfigPanelUi *ui);
static void ConfigPanelUi_Main(ConfigPanelUi *ui);
static u32 ConfigPanelUi_GetInput(ConfigPanelUi *ui);
static void ConfigPanelUi_GetPoint(ConfigPanelUi *ui, u32 kind, ConfigPanelPoint *point);
static void ConfigPanelMsg_Init(ConfigPanelMsg *msg, BmpWin *window, Font *font, MsgData *msgData,
                                TCBExManager *tcbManager, PrintQueue *queue, u16 heapId);
static void ConfigPanelMsg_Exit(ConfigPanelMsg *msg);
static void ConfigPanelMsg_Print(ConfigPanelMsg *msg, u32 msgId, s32 wait, BOOL stream);
static void ConfigPanelMsg_PrintFrom(ConfigPanelMsg *msg, MsgData *msgData, u32 msgId, s32 wait, BOOL stream);
static BOOL ConfigPanelMsg_IsDone(ConfigPanelMsg *msg);
static void ConfigPanelAppBar_Init(ConfigPanelAppBar *appBar, ClActUnit *unit, Font *font, PrintQueue *queue,
                                   MsgData *msgData, AppTaskMenuRes *res, GameData *gameData, u16 heapId);
static void ConfigPanelAppBar_Exit(ConfigPanelAppBar *appBar);
static void ConfigPanelAppBar_Main(ConfigPanelAppBar *appBar, ConfigPanelUi *ui, ConfigPanelScroll *scroll);
static void ConfigPanelAppBar_UpdateButtons(ConfigPanelAppBar *appBar);
static BOOL ConfigPanelAppBar_GetDecided(ConfigPanelAppBar *appBar, s32 *button);
static BOOL ConfigPanelAppBar_IsFlashFinished(ConfigPanelAppBar *appBar);
static void ConfigPanelAppBar_ResetButtons(ConfigPanelAppBar *appBar);
static void ConfigPanelAppBar_CreateButtons(ConfigPanelAppBar *appBar, u16 heapId);
static BOOL ConfigPanelAppBar_IsTouched(ConfigPanelAppBar *appBar);
static void ConfigPanelScroll_Init(ConfigPanelScroll *scroll, u16 textBg, u16 rowsBg, ConfigPanelGraphic *graphic,
                                   const ConfigPanelOptions *options, HeapID heapId);
static void ConfigPanelScroll_Exit(ConfigPanelScroll *scroll);
static void ConfigPanelScroll_Main(ConfigPanelScroll *scroll, ConfigPanelUi *ui, ConfigPanelMsg *msg,
                                   ConfigPanelGraphic *graphic, ConfigPanelAppBar *appBar);
static void ConfigPanelScroll_HideCursor(ConfigPanelScroll *scroll, ConfigPanelGraphic *graphic);
static s32 ConfigPanelScroll_GetCursor(ConfigPanelScroll *scroll);
static void ConfigPanelScroll_GetOptions(ConfigPanelScroll *scroll, ConfigPanelOptions *options);
static void ConfigPanelScroll_Redraw(ConfigPanelScroll *scroll, BOOL unused);
static void ConfigPanelScroll_SetSaveBeforeIr(ConfigPanelScroll *scroll, u8 value);
static void ConfigPanelScroll_Draw(ConfigPanelScroll *scroll, BOOL cursorVisible);
static void ConfigPanelScroll_Move(ConfigPanelScroll *scroll, int dy, int min, int max);
static void ConfigPanelScroll_TouchItems(ConfigPanelScroll *scroll, const ConfigPanelPoint *point);
static void ConfigPanelScroll_ApplySound(ConfigPanelScroll *scroll, s8 sound, BOOL force);
static void ConfigPanelScroll_Step(ConfigPanelScroll *scroll, int dir);
static s32 ConfigPanelScroll_GetTouchedRow(ConfigPanelScroll *scroll, const ConfigPanelPoint *point);
static void ConfigPanelScroll_Tick(ConfigPanelScroll *scroll);
static void ConfigPanelConfirm_Init(ConfigPanelConfirm *confirm, BmpWin *window, Font *font, MsgData *msgData,
                                    TCBExManager *tcbManager, AppTaskMenuRes *res, PrintQueue *queue, u16 heapId);
static void ConfigPanelConfirm_Exit(ConfigPanelConfirm *confirm);
static void ConfigPanelConfirm_Main(ConfigPanelConfirm *confirm);
static void ConfigPanelConfirm_Start(ConfigPanelConfirm *confirm, s32 wait);
static u32 ConfigPanelConfirm_GetResult(ConfigPanelConfirm *confirm);
static void ConfigPanelConfirm_PrintScriptMsg(ConfigPanelConfirm *confirm, u32 msgId, s32 wait);
static void ConfigPanelConfirm_PrintMsg(ConfigPanelConfirm *confirm, u32 msgId, s32 wait);
static BOOL ConfigPanelConfirm_IsMsgDone(ConfigPanelConfirm *confirm);
static void ConfigPanelOptions_Init(ConfigPanelOptions *options, Config *config);
static void ConfigPanelOptions_Exit(ConfigPanelOptions *options);
static void ConfigPanelOptions_Apply(const ConfigPanelOptions *options);
static void ConfigPanelOptions_SetItem(ConfigPanelOptions *options, u32 item);
static void ConfigPanelOptions_Step(ConfigPanelOptions *options, u32 option, int dir);
static BOOL ConfigPanelOptions_IsSet(const ConfigPanelOptions *options, u32 item);
static s32 ConfigPanelOptions_GetTextWait(const ConfigPanelOptions *options);
static u32 ConfigPanelOptions_PopChanged(ConfigPanelOptions *options);
static BOOL ConfigPanelOptions_Equals(const ConfigPanelOptions *a, const ConfigPanelOptions *b);
static void ConfigPanelSeq_Init(ConfigPanelSeq *seq, void *data, ConfigPanelSeqFunc func);
static void ConfigPanelSeq_Exit(ConfigPanelSeq *seq);
static void ConfigPanelSeq_Main(ConfigPanelSeq *seq);
static BOOL ConfigPanelSeq_IsEnded(ConfigPanelSeq *seq);
static void ConfigPanelSeq_SetNext(ConfigPanelSeq *seq, ConfigPanelSeqFunc func);
static void ConfigPanelSeq_End(ConfigPanelSeq *seq);
static void ConfigPanelSeq_FadeIn(ConfigPanelSeq *seq, u32 *state, void *data);
static void ConfigPanelSeq_FadeOut(ConfigPanelSeq *seq, u32 *state, void *data);
static void ConfigPanelSeq_Input(ConfigPanelSeq *seq, u32 *state, void *data);
static void ConfigPanelSeq_Confirm(ConfigPanelSeq *seq, u32 *state, void *data);
static void ConfigPanelSeq_Quit(ConfigPanelSeq *seq, u32 *state, void *data);
static void ConfigPanelSeq_Save(ConfigPanelSeq *seq, u32 *state, void *data);
static BOOL ConfigPanel_IsInRect(const ConfigPanelRect *rect, const ConfigPanelPoint *point);

const GameProcFunctions CONFIG_PANEL_PROC_FUNCTIONS = { ConfigPanel_Init, ConfigPanel_Main, ConfigPanel_Exit };

static const BGSysVRAMConfig sConfigPanelVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const BGSysLCDConfig sConfigPanelLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ConfigPanelBgSetup sConfigPanelBgSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 1, 0, 0 },
      BGMODE_TEXT },
    { 1,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 2, 0, 0 },
      BGMODE_TEXT },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 3, 0, 0 },
      BGMODE_TEXT },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2800), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 0, 0, 0 },
      BGMODE_TEXT },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, 0, 0 },
      BGMODE_TEXT },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 0, 0, 0 },
      BGMODE_TEXT },
};

// The values' texts in the order of sConfigPanelItems, then the options' names
static ConfigPanelLabel sConfigPanelLabels[LABEL_COUNT] = {
    { Title2_Text_Slow, 14, 5, 6, 2 },        { Title2_Text_Mid, 20, 5, 6, 2 },
    { Title2_Text_Fast, 26, 5, 6, 2 },        { Title2_Text_On, 15, 8, 4, 2 },
    { Title2_Text_Off, 24, 8, 6, 2 },         { Title2_Text_Shift, 15, 11, 8, 2 },
    { Title2_Text_Set, 24, 11, 8, 2 },        { Title2_Text_Stereo, 15, 14, 6, 2 },
    { Title2_Text_Mono, 24, 14, 6, 2 },       { Title2_Text_Save, 15, 17, 6, 2 },
    { Title2_Text_NoSave, 24, 17, 7, 2 },     { Title2_Text_TextSpeed, 1, 5, 12, 2 },
    { Title2_Text_BattleScene, 1, 8, 10, 2 }, { Title2_Text_BattleStyle, 1, 11, 12, 2 },
    { Title2_Text_Sound, 1, 14, 6, 2 },       { Title2_Text_SaveBeforeIr, 1, 17, 10, 2 },
};

static const ConfigPanelItem sConfigPanelItems[ITEM_COUNT] = {
    { OPTION_TEXT_SPEED, 0, 120, 0, 112, 0, 48, 24 },       { OPTION_TEXT_SPEED, 1, 165, 0, 160, 0, 48, 24 },
    { OPTION_TEXT_SPEED, 2, 211, 0, 208, 0, 48, 24 },       { OPTION_BATTLE_SCENE, 0, 120, 24, 112, 24, 72, 24 },
    { OPTION_BATTLE_SCENE, 1, 192, 24, 184, 24, 72, 24 },   { OPTION_BATTLE_STYLE, 0, 120, 48, 112, 48, 72, 24 },
    { OPTION_BATTLE_STYLE, 1, 192, 48, 184, 48, 72, 24 },   { OPTION_SOUND, 0, 120, 72, 112, 72, 72, 24 },
    { OPTION_SOUND, 1, 192, 72, 184, 72, 72, 24 },          { OPTION_SAVE_BEFORE_IR, 1, 120, 96, 112, 96, 72, 24 },
    { OPTION_SAVE_BEFORE_IR, 0, 192, 96, 184, 96, 72, 24 },
};

// The buttons' areas of the touch screen
static const ConfigPanelRect sConfigPanelConfirmRect = { 112, 168, 184, 192 };
static const ConfigPanelRect sConfigPanelQuitRect = { 184, 168, 256, 192 };

static const ConfigPanelOption sConfigPanelOptions[OPTION_COUNT] = {
    { Title2_Text_CanChooseFromThree, 3, { 0, 1, 2 } },  { Title2_Text_CanChooseWhetherView, 2, { 3, 4 } },
    { Title2_Text_CanChooseSwitchPokemon, 2, { 5, 6 } }, { Title2_Text_CanChooseWayGame, 2, { 7, 8 } },
    { Title2_Text_CanChooseSaveGame, 2, { 9, 10 } },
};

// The palettes of the rows on the screen, from the top
static const u8 sConfigPanelRowPalettes[OPTION_COUNT] = { 3, 1, 0, 1, 3 };

static BOOL ConfigPanel_Init(GameProc *proc, u32 *state, void *param, void *work) {
    ConfigPanelWork *wk;
    GameData *gameData;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_CONFIG_PANEL, 0x21000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ConfigPanelWork), HEAPID_CONFIG_PANEL);
    sys_memset(wk, 0, sizeof(ConfigPanelWork));
    wk->param = param;
    wk->param->result = FALSE;
    ConfigPanelGraphic_Init(&wk->graphic, HEAPID_CONFIG_PANEL);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_CONFIG_PANEL);
    wk->queue = func_02021998(HEAPID_CONFIG_PANEL);
    wk->buttonRes = AppTaskMenuRes_Create(BG_BAR, 13, wk->font, wk->queue, HEAPID_CONFIG_PANEL);
    wk->confirmRes = AppTaskMenuRes_Create(BG_CONFIRM, 9, wk->font, wk->queue, HEAPID_CONFIG_PANEL);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_2, HEAPID_CONFIG_PANEL);
    wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_CONFIG_PANEL, HEAPID_CONFIG_PANEL, 32, 32);
    ConfigPanelOptions_Init(
        &wk->saved, (Config *)getTrainerDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys))));
    ConfigPanelSeq_Init(&wk->seq, wk, ConfigPanelSeq_FadeIn);
    gameData = GSYS_GetGameData(wk->param->gsys);
    ConfigPanelAppBar_Init(&wk->appBar, ConfigPanelGraphic_GetUnit(&wk->graphic), wk->font, wk->queue, wk->msgData,
                           wk->buttonRes, gameData, HEAPID_CONFIG_PANEL);
    ConfigPanelScroll_Init(&wk->scroll, BG_TEXT, BG_ROWS, &wk->graphic, &wk->saved, HEAPID_CONFIG_PANEL);
    ConfigPanelUi_Init(&wk->ui, HEAPID_CONFIG_PANEL);
    ConfigPanelMsg_Init(&wk->msg, ConfigPanelGraphic_GetWindow(&wk->graphic, WINDOW_MESSAGE), wk->font, wk->msgData,
                        wk->tcbManager, wk->queue, HEAPID_CONFIG_PANEL);
    ConfigPanelConfirm_Init(&wk->confirm, ConfigPanelGraphic_GetWindow(&wk->graphic, WINDOW_CONFIRM), wk->font,
                            wk->msgData, wk->tcbManager, wk->confirmRes, wk->queue, HEAPID_CONFIG_PANEL);
    return TRUE;
}

static BOOL ConfigPanel_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    ConfigPanelWork *wk = work;

    ConfigPanelConfirm_Exit(&wk->confirm);
    ConfigPanelMsg_Exit(&wk->msg);
    ConfigPanelUi_Exit(&wk->ui);
    ConfigPanelScroll_Exit(&wk->scroll);
    ConfigPanelAppBar_Exit(&wk->appBar);
    ConfigPanelSeq_Exit(&wk->seq);
    ConfigPanelOptions_Exit(&wk->saved);
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_MsgDataFree(wk->msgData);
    AppTaskMenuRes_Free(wk->confirmRes);
    AppTaskMenuRes_Free(wk->buttonRes);
    func_02021a18(wk->queue);
    GFL_FontFree(wk->font);
    ConfigPanelGraphic_Exit(&wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_CONFIG_PANEL);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL ConfigPanel_Main(GameProc *proc, u32 *state, void *param, void *work) {
    ConfigPanelWork *wk = work;

    ConfigPanelSeq_Main(&wk->seq);
    ConfigPanelGraphic_Main(&wk->graphic);
    func_02021a3c(wk->queue);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    ConfigPanelAppBar_UpdateButtons(&wk->appBar);
    if (ConfigPanelSeq_IsEnded(&wk->seq)) {
        return TRUE;
    }
    return FALSE;
}

static void ConfigPanelGraphic_Init(ConfigPanelGraphic *graphic, HeapID heapId) {
    sys_memset(graphic, 0, sizeof(ConfigPanelGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(0);
    GFL_BGSysSetVRAMBanks(&sConfigPanelVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    GFL_TextRndUpdateColorIndexLUT(15, 14, 0);
    ConfigPanelBg_Init(&graphic->bg, heapId);
    ConfigPanelObj_Init(&graphic->obj, &sConfigPanelVRAMConfig, heapId);
    graphic->vblankTcb = GFL_VBlankTCBAdd(ConfigPanelGraphic_VBlank, graphic, 0);
}

static void ConfigPanelGraphic_Exit(ConfigPanelGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTcb);
    ConfigPanelObj_Exit(&graphic->obj);
    ConfigPanelBg_Exit(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(ConfigPanelGraphic));
}

void ConfigPanelGraphic_Main(ConfigPanelGraphic *graphic) {
    ConfigPanelObj_Main(&graphic->obj);
    ConfigPanelBg_Main(&graphic->bg);
}

static ClActor *ConfigPanelGraphic_GetActor(ConfigPanelGraphic *graphic, u32 index) {
    return ConfigPanelObj_GetActor(&graphic->obj, index);
}

static ClActUnit *ConfigPanelGraphic_GetUnit(ConfigPanelGraphic *graphic) {
    return ConfigPanelObj_GetUnit(&graphic->obj);
}

static BmpWin *ConfigPanelGraphic_GetWindow(ConfigPanelGraphic *graphic, u32 index) {
    return ConfigPanelBg_GetWindow(&graphic->bg, index);
}

static void ConfigPanelGraphic_ResetPulse(ConfigPanelGraphic *graphic) {
    ConfigPanelBg_ResetPulse(&graphic->bg);
}

static void ConfigPanelGraphic_VBlank(TCB *tcb, void *data) {
    ConfigPanelGraphic *graphic = data;

    ConfigPanelBg_VBlank(&graphic->bg);
    ConfigPanelObj_VBlank(&graphic->obj);
}

static void ConfigPanelBg_Init(ConfigPanelBg *bg, HeapID heapId) {
    int i;
    ArcTool *arc;
    NNSG2dPaletteData *palette;
    void *buf;

    sys_memset(bg, 0, sizeof(ConfigPanelBg));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sConfigPanelLCDConfig);
    for (i = 0; i < BG_COUNT; i++) {
        u32 index = sConfigPanelBgSetups[i].bg;

        GFL_BGSysCreateBG(index, &sConfigPanelBgSetups[i].setup, sConfigPanelBgSetups[i].mode);
        GFL_BGSysClearBG(index);
        GFL_BGSysSetBGEnabled(index, TRUE);
    }
    GFL_BGSysSetBGEnabled(BG_CONFIRM, FALSE);
    GFL_BGSysFillChar(BG_MESSAGE, 0, 1, 0);
    GFL_BGSysFillChar(BG_CONFIRM, 0, 1, 0);

    arc = GFL_ArcSysCreateFileHandle(ARCID_CONFIG_PANEL, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 13, BG_BAR, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 14, BG_BAR, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, BG_ROWS, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 2, BG_ROWS, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 16, BG_CONFIRM, 1, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 8, 0, 4 * 0x20, 15 * 0x20, 0x20, heapId);
    bg->titleCharBuf = GFL_G2DIOReadBGNCGRArc(arc, 15, FALSE, &bg->titleChars, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 8, 4, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 5, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 9, 5, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 16, BG_MESSAGE, 1, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);

    buf = GFL_G2DIOReadNCLR(ARCID_CONFIG_PANEL, 1, &palette, heapId);
    sys_memcpy((u16 *)palette->rawData + 7 * 16, bg->cursorPalette, sizeof(bg->cursorPalette));
    GFL_HeapFree(buf);

    bg->windows[WINDOW_MESSAGE] = BmpWin_CreateDynamic(BG_MESSAGE, 1, 19, 30, 4, 4, TRUE);
    bg->windows[WINDOW_CONFIRM] = BmpWin_CreateDynamic(BG_CONFIRM, 1, 1, 30, 4, 15, TRUE);
    bg->windows[WINDOW_TITLE] = BmpWin_CreateDynamic(BG_BAR, 0, 0, 16, 3, 8, TRUE);
    ConfigPanelBg_PrintLabels(bg);
    GFL_BGSysSetBGEnabled(BG_MESSAGE, FALSE);
}

static void ConfigPanelBg_Exit(ConfigPanelBg *bg) {
    int i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (bg->windows[i] != NULL) {
            BmpWin_Free(bg->windows[i]);
        }
    }
    GFL_HeapFree(bg->titleCharBuf);
    GFL_BGSysFreeFilledChar(BG_MESSAGE, 1, 0);
    GFL_BGSysFreeFilledChar(BG_CONFIRM, 1, 0);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sConfigPanelBgSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(ConfigPanelBg));
}

// Pulses the cursor's color
static void ConfigPanelBg_Main(ConfigPanelBg *bg) {
    ConfigPanel_PulseColor(&bg->pulseColor, &bg->pulseAngle, 0x400, 7, 1, bg->cursorPalette[11], bg->cursorPalette[12]);
}

static BmpWin *ConfigPanelBg_GetWindow(ConfigPanelBg *bg, u32 index) {
    return bg->windows[index];
}

static void ConfigPanelBg_PrintLabels(ConfigPanelBg *bg) {
    int i;
    Font *font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_CONFIG_PANEL);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_2, HEAPID_CONFIG_PANEL);
    StrBuf *str;
    int x;
    const void *chars;

    for (i = 0; i < LABEL_COUNT; i++) {
        ConfigPanelLabel *label;
        u8 width;

        x = 0;
        switch (i) {
        case 0:
            x = 7;
            break;
        case 1:
            x = 4;
            break;
        case 2:
            x = 2;
            break;
        }
        label = &sConfigPanelLabels[i];
        str = GFL_MsgDataLoadStrbufNew(msgData, sConfigPanelLabels[i].msgId);
        width = (x + GFL_FontGetBlockWidth(str, font, 0) + 7) / 8;
        label->width = width;
        bg->windows[i] = BmpWin_CreateDynamic(BG_TEXT, label->x, label->y, width, label->height, 0, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(bg->windows[i]), 0);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(bg->windows[i]), x, 0, str, font);
        BmpWin_Transfer(bg->windows[i]);
        GFL_StrBufFree(str);
    }

    // The title's window starts as the bar's graphic of it
    chars = bg->titleChars->rawData;
    sys_memcpy32(chars, GFL_BitmapGetPixelData(BmpWin_GetBitmap(bg->windows[WINDOW_TITLE])), 16 * 3 * 0x20);
    str = GFL_MsgDataLoadStrbufNew(msgData, Title2_Text_Options);
    x = BmpWin_GetSizeX(bg->windows[WINDOW_TITLE]) * 4;
    x -= GFL_FontGetBlockWidth(str, font, 0) / 2;
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(bg->windows[WINDOW_TITLE]), x, 4, str, font,
                                   PRINT_COLOR(15, 14, 1));
    BmpWin_Transfer(bg->windows[WINDOW_TITLE]);
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    GFL_FontFree(font);
}

static void ConfigPanelBg_ResetPulse(ConfigPanelBg *bg) {
    bg->pulseAngle = 0;
}

static void ConfigPanelBg_VBlank(ConfigPanelBg *bg) {
    GFL_BGSysUpdate();
}

// Moves a color of a main engine BG palette between two colors, one way and back in a full turn of the angle
static void ConfigPanel_PulseColor(u16 *dest, u16 *angle, u16 speed, u8 palette, u8 index, u16 from, u16 to) {
    if (*angle + speed >= 0x10000) {
        *angle = *angle + speed - 0x10000;
    } else {
        *angle += speed;
    }
    {
        u8 fromG = (from & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        s16 t = (FX_CosIdx(*angle) + FX32_ONE) / 2;
        u8 fromR = (from & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
        u8 fromB = (from & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 toB = (to & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 b = fromB + ((toB - fromB) * t >> FX32_SHIFT);
        u8 toR = (to & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT;
        u8 r = fromR + ((toR - fromR) * t >> FX32_SHIFT);
        u8 toG = (to & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 g = fromG + ((toG - fromG) * t >> FX32_SHIFT);

        *dest = GX_RGB(r, g, b);
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, palette * 32 + index * 2, dest, sizeof(u16));
    }
}

static void ConfigPanelObj_Init(ConfigPanelObj *obj, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    int i;
    ArcTool *arc;
    ClActorSetup setup;

    sys_memset(obj, 0, sizeof(ConfigPanelObj));
    ClActSys_Create(&data_02093f08, vramConfig, heapId);
    obj->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(obj->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);

    arc = GFL_ArcSysCreateFileHandle(ARCID_CONFIG_PANEL, heapId);
    obj->palette = func_0204bba0(arc, 6, CLACT_VRAM_MAIN, 0, heapId);
    obj->chars = func_0204b81c(arc, 5, FALSE, CLACT_VRAM_MAIN, heapId);
    obj->cellAnims = func_0204bde0(arc, 4, 3, heapId);
    obj->knobChars = func_0204b81c(arc, 12, FALSE, CLACT_VRAM_MAIN, heapId);
    obj->knobCellAnims = func_0204bde0(arc, 11, 10, heapId);
    GFL_ArcToolFree(arc);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.bgPriority = 2;
    setup.priority = 2;
    for (i = ACTOR_ITEM; i < ACTOR_ITEM + ITEM_COUNT; i++) {
        setup.x = sConfigPanelItems[i - ACTOR_ITEM].x;
        setup.y = sConfigPanelItems[i - ACTOR_ITEM].y;
        obj->actors[i] = func_0204c040(obj->unit, obj->chars, obj->palette, obj->cellAnims, &setup, 0, heapId);
        func_0204c520(obj->actors[i], FALSE);
    }

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 128;
    setup.y = 96;
    setup.bgPriority = 2;
    setup.priority = 1;
    obj->actors[1] = func_0204c040(obj->unit, obj->knobChars, obj->palette, obj->knobCellAnims, &setup, 0, heapId);
    func_0204c378(obj->actors[1], 6, 1);

    if (func_02042788()) {
        func_02042ba8(TRUE, heapId);
    }
}

static void ConfigPanelObj_Exit(ConfigPanelObj *obj) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (obj->actors[i] != NULL) {
            func_0204c108(obj->actors[i]);
        }
    }
    func_0204bcd0(obj->palette);
    func_0204b98c(obj->chars);
    func_0204be64(obj->cellAnims);
    func_0204b98c(obj->knobChars);
    func_0204be64(obj->knobCellAnims);
    func_0204bf98(obj->unit);
    func_0204b758();
    sys_memset(obj, 0, sizeof(ConfigPanelObj));
}

// Animates the values' frames together
static void ConfigPanelObj_Main(ConfigPanelObj *obj) {
    int i;

    if (obj->frame++ > 96) {
        obj->frame = 0;
    }
    for (i = ACTOR_ITEM; i < ACTOR_ITEM + ITEM_COUNT; i++) {
        func_0204c504(obj->actors[i], obj->frame / 12);
    }
    func_0204b794();
}

static void ConfigPanelObj_VBlank(ConfigPanelObj *obj) {
    func_0204b7c8();
}

static ClActor *ConfigPanelObj_GetActor(ConfigPanelObj *obj, u32 index) {
    return obj->actors[index];
}

static ClActUnit *ConfigPanelObj_GetUnit(ConfigPanelObj *obj) {
    return obj->unit;
}

static void ConfigPanelUi_Init(ConfigPanelUi *ui, HeapID heapId) {
    sys_memset(ui, 0, sizeof(ConfigPanelUi));
}

static void ConfigPanelUi_Exit(ConfigPanelUi *ui) {
    sys_memset(ui, 0, sizeof(ConfigPanelUi));
}

static void ConfigPanelUi_Main(ConfigPanelUi *ui) {
    u32 x;
    u32 y;
    u32 pressed;
    u32 held;

    ui->input = INPUT_NONE;
    if (func_0203dac8(&x, &y)) {
        ui->input = INPUT_TOUCH;
        ui->start.x = x;
        ui->start.y = y;
        ui->now.x = x;
        ui->now.y = y;
        ui->touching = TRUE;
    } else if (func_0203da84(&x, &y) && ui->touching) {
        if (MATH_ABS((s32)y - ui->start.y) >= 1) {
            ui->input = INPUT_SLIDE;
            ui->slide.x = x - ui->now.x;
            ui->slide.y = y - ui->now.y;
            ui->now.x = x;
            ui->now.y = y;
        }
    } else {
        ui->touching = FALSE;
    }

    pressed = GCTX_HIDGetPressedKeys();
    held = GCTX_HIDGetHeldKeys();
    if (pressed != 0 && func_0203d554() == TRUE && !(pressed & (PAD_BUTTON_B | PAD_BUTTON_X | PAD_BUTTON_Y))) {
        func_0203d564(FALSE);
        ui->input = INPUT_KEY_MODE;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return;
    }
    if (pressed & PAD_BUTTON_A) {
        ui->input = INPUT_DECIDE;
    } else if (pressed & PAD_BUTTON_B) {
        func_0203d564(FALSE);
        ui->input = INPUT_CANCEL;
    } else if (pressed & PAD_BUTTON_Y) {
        func_0203d564(FALSE);
        ui->input = INPUT_Y;
    } else if (pressed & PAD_BUTTON_X) {
        func_0203d564(FALSE);
        ui->input = INPUT_X;
    } else if (pressed & PAD_KEY_UP) {
        ui->input = INPUT_UP;
    } else if (pressed & PAD_KEY_DOWN) {
        ui->input = INPUT_DOWN;
    } else if (pressed & PAD_KEY_LEFT) {
        ui->input = INPUT_LEFT;
    } else if (pressed & PAD_KEY_RIGHT) {
        ui->input = INPUT_RIGHT;
    } else if (held & PAD_KEY_UP) {
        ui->input = INPUT_UP_HELD;
    } else if (held & PAD_KEY_DOWN) {
        ui->input = INPUT_DOWN_HELD;
    }
}

static u32 ConfigPanelUi_GetInput(ConfigPanelUi *ui) {
    return ui->input;
}

static void ConfigPanelUi_GetPoint(ConfigPanelUi *ui, u32 kind, ConfigPanelPoint *point) {
    if (point != NULL) {
        switch (kind) {
        case POINT_START:
            *point = ui->start;
            break;
        case POINT_SLIDE:
            *point = ui->slide;
            break;
        case POINT_NOW:
            *point = ui->now;
            break;
        }
    }
}

static void ConfigPanelMsg_Init(ConfigPanelMsg *msg, BmpWin *window, Font *font, MsgData *msgData,
                                TCBExManager *tcbManager, PrintQueue *queue, u16 heapId) {
    sys_memset(msg, 0, sizeof(ConfigPanelMsg));
    msg->clearColor = 4;
    msg->window = window;
    msg->heapId = heapId;
    msg->font = font;
    msg->msgData = msgData;
    msg->strbuf = GFL_StrBufCreate(255, heapId);
    msg->tcbManager = tcbManager;
    msg->stream = NULL;
    msg->queue = queue;
    msg->mode = MSG_MODE_NONE;
    AppPrintsysCommon_Init(&msg->common,
                           APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH | APP_PRINTSYS_COMMON_NO_SKIP);
    BmpWin_DrawFrame(msg->window, 2, 1, BmpWin_GetPalette(window));
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), msg->clearColor);
    BmpWin_TransferNow(msg->window);
    PrintWindow_Init(&msg->printWindow, msg->window);
}

static void ConfigPanelMsg_Exit(ConfigPanelMsg *msg) {
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    GFL_StrBufFree(msg->strbuf);
    sys_memset(msg, 0, sizeof(ConfigPanelMsg));
}

static void ConfigPanelMsg_Print(ConfigPanelMsg *msg, u32 msgId, s32 wait, BOOL stream) {
    ConfigPanelMsg_PrintFrom(msg, msg->msgData, msgId, wait, stream);
}

// Prints a message at once, or a character at a time with the wait
static void ConfigPanelMsg_PrintFrom(ConfigPanelMsg *msg, MsgData *msgData, u32 msgId, s32 wait, BOOL stream) {
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), msg->clearColor);
    GFL_MsgDataLoadStrbuf(msgData, msgId, msg->strbuf);
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    if (!stream) {
        PrintWindow_PrintNoColor(&msg->printWindow, msg->queue, 0, 0, msg->strbuf, msg->font);
        msg->mode = MSG_MODE_QUEUE;
    } else {
        msg->stream = func_02022268(msg->window, 0, 0, msg->strbuf, msg->font, wait, msg->tcbManager, 0, msg->heapId,
                                    msg->clearColor);
        msg->mode = MSG_MODE_STREAM;
    }
}

static BOOL ConfigPanelMsg_IsDone(ConfigPanelMsg *msg) {
    switch (msg->mode) {
    case MSG_MODE_NONE:
    default:
        return TRUE;
    case MSG_MODE_QUEUE:
        PrintWindow_Flush(&msg->printWindow, msg->queue);
        return PrintWindow_IsPrinted(&msg->printWindow);
    case MSG_MODE_STREAM:
        return AppPrintsysCommon_Update(&msg->common, msg->stream);
    }
}

static void ConfigPanelAppBar_Init(ConfigPanelAppBar *appBar, ClActUnit *unit, Font *font, PrintQueue *queue,
                                   MsgData *msgData, AppTaskMenuRes *res, GameData *gameData, u16 heapId) {
    sys_memset(appBar, 0, sizeof(ConfigPanelAppBar));
    appBar->button = BUTTON_NONE;
    appBar->font = font;
    appBar->res = res;
    appBar->msgData = msgData;
    appBar->queue = queue;
    appBar->gameData = gameData;

    {
        TouchBarItem items[2] = {
            { TOUCHBAR_ICON_CLOSE, { 80, 168 } },
            { TOUCHBAR_ICON_CHECK, { 48, 172 } },
        };
        TouchBarSetup setup;

        sys_memset(&setup, 0, sizeof(TouchBarSetup));
        setup.items = items;
        setup.count = 2;
        setup.unit = unit;
        setup.objPalette = 8;
        setup.vramType = CLACT_VRAM_BOTH;
        setup.unk1C = TRUE;
        appBar->touchBar = TouchBar_Create(&setup, heapId);
    }
    ConfigPanelAppBar_CreateButtons(appBar, heapId);

    if (GameData_IsShortcutRegistered(appBar->gameData, SHORTCUT_CONFIG_PANEL)) {
        TouchBar_SetIconPressed(appBar->touchBar, TOUCHBAR_ICON_CHECK, TRUE);
    } else {
        TouchBar_SetIconPressed(appBar->touchBar, TOUCHBAR_ICON_CHECK, FALSE);
    }
}

static void ConfigPanelAppBar_Exit(ConfigPanelAppBar *appBar) {
    int i;

    TouchBar_Free(appBar->touchBar);
    for (i = 0; i < 2; i++) {
        GFL_StrBufFree(appBar->items[i].str);
        AppTaskMenuWin_Free(appBar->wins[i]);
    }
    sys_memset(appBar, 0, sizeof(ConfigPanelAppBar));
}

static void ConfigPanelAppBar_Main(ConfigPanelAppBar *appBar, ConfigPanelUi *ui, ConfigPanelScroll *scroll) {
    BOOL update = FALSE;
    u32 input;
    s32 cursor;
    ConfigPanelPoint point;

    appBar->touched = FALSE;
    if (appBar->decided) {
        input = INPUT_NONE;
    } else {
        input = ConfigPanelUi_GetInput(ui);
    }
    cursor = ConfigPanelScroll_GetCursor(scroll);

    switch (input) {
    case INPUT_KEY_MODE:
        return;
    case INPUT_TOUCH:
        ConfigPanelUi_GetPoint(ui, POINT_START, &point);
        if (ConfigPanel_IsInRect(&sConfigPanelConfirmRect, &point) && appBar->button != BUTTON_CONFIRM) {
            GFL_BGSysSetBGEnabled(BG_MESSAGE, FALSE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            appBar->button = BUTTON_CONFIRM;
            appBar->decided = TRUE;
            appBar->touched = TRUE;
            if (!func_0203d554()) {
                func_0203d564(TRUE);
            }
        } else if (ConfigPanel_IsInRect(&sConfigPanelQuitRect, &point) && appBar->button != BUTTON_QUIT) {
            GFL_BGSysSetBGEnabled(BG_MESSAGE, FALSE);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            appBar->button = BUTTON_QUIT;
            appBar->decided = TRUE;
            appBar->touched = TRUE;
            if (!func_0203d554()) {
                func_0203d564(TRUE);
            }
        } else if (appBar->button != BUTTON_NONE) {
            appBar->button = BUTTON_NONE;
        }
        update = TRUE;
        break;
    case INPUT_UP:
    case INPUT_DOWN:
    case INPUT_RIGHT:
    case INPUT_LEFT:
    case INPUT_UP_HELD:
    case INPUT_DOWN_HELD:
        switch (cursor) {
        case CURSOR_CONFIRM:
            appBar->button = BUTTON_CONFIRM;
            break;
        case CURSOR_QUIT:
            appBar->button = BUTTON_QUIT;
            break;
        default:
            appBar->button = BUTTON_NONE;
            break;
        }
        update = TRUE;
        break;
    case INPUT_DECIDE:
        if (appBar->button != BUTTON_NONE) {
            if (appBar->button == BUTTON_QUIT) {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
            } else {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            update = TRUE;
            appBar->decided = TRUE;
        } else {
            appBar->button = BUTTON_CONFIRM;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            update = TRUE;
            appBar->decided = TRUE;
        }
        break;
    case INPUT_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        update = TRUE;
        appBar->button = BUTTON_QUIT;
        appBar->decided = TRUE;
        break;
    }

    if (update) {
        if (appBar->button == BUTTON_NONE) {
            AppTaskMenuWin_SetActive(appBar->wins[BUTTON_CONFIRM], FALSE);
            AppTaskMenuWin_SetActive(appBar->wins[BUTTON_QUIT], FALSE);
        } else {
            if (appBar->decided) {
                AppTaskMenuWin_SetActive(appBar->wins[appBar->button], FALSE);
                AppTaskMenuWin_SetFlashing(appBar->wins[appBar->button], TRUE);
                AppTaskMenuWin_SetActive(appBar->wins[appBar->button ^ 1], FALSE);
            } else {
                AppTaskMenuWin_SetActive(appBar->wins[appBar->button], TRUE);
                AppTaskMenuWin_SetActive(appBar->wins[appBar->button ^ 1], FALSE);
            }
        }
    }

    if (!appBar->decided) {
        TouchBar_Main(appBar->touchBar);
        switch (TouchBar_GetTouched(appBar->touchBar)) {
        case TOUCHBAR_ICON_CHECK:
            if (func_0203d554() == TRUE) {
                appBar->touched = TRUE;
            }
            break;
        case TOUCHBAR_ICON_CLOSE:
            if (func_0203d554() == TRUE) {
                appBar->touched = TRUE;
            }
            break;
        }
        switch (TouchBar_GetDecided(appBar->touchBar)) {
        case TOUCHBAR_ICON_CHECK:
            if (TouchBar_IsIconPressed(appBar->touchBar, TOUCHBAR_ICON_CHECK)) {
                GameData_SetKeyItemRegistration(appBar->gameData, SHORTCUT_CONFIG_PANEL, TRUE);
            } else {
                GameData_SetKeyItemRegistration(appBar->gameData, SHORTCUT_CONFIG_PANEL, FALSE);
            }
            appBar->button = BUTTON_NONE;
            break;
        case TOUCHBAR_ICON_CLOSE:
            appBar->button = BUTTON_CLOSE;
            appBar->decided = TRUE;
            break;
        }
    }
}

static void ConfigPanelAppBar_UpdateButtons(ConfigPanelAppBar *appBar) {
    int i;

    for (i = 0; i < 2; i++) {
        AppTaskMenuWin_Update(appBar->wins[i]);
    }
}

// Returns whether a button was picked, and which
static BOOL ConfigPanelAppBar_GetDecided(ConfigPanelAppBar *appBar, s32 *button) {
    if (button != NULL) {
        *button = appBar->button;
    }
    return appBar->decided;
}

static BOOL ConfigPanelAppBar_IsFlashFinished(ConfigPanelAppBar *appBar) {
    BOOL finished = FALSE;

    if (appBar->decided && appBar->button != BUTTON_NONE) {
        finished = AppTaskMenuWin_IsFlashFinished(appBar->wins[appBar->button]);
        if (finished) {
            ConfigPanelAppBar_ResetButtons(appBar);
        }
    }
    return finished;
}

static void ConfigPanelAppBar_ResetButtons(ConfigPanelAppBar *appBar) {
    int i;

    for (i = 0; i < 2; i++) {
        AppTaskMenuWin_ResetFlash(appBar->wins[i]);
        AppTaskMenuWin_SetActive(appBar->wins[i], FALSE);
    }
}

static void ConfigPanelAppBar_CreateButtons(ConfigPanelAppBar *appBar, u16 heapId) {
    int i;

    for (i = 0; i < 2; i++) {
        if (appBar->items[i].str != NULL) {
            GFL_StrBufFree(appBar->items[i].str);
        }
        appBar->items[i].str = GFL_MsgDataLoadStrbufNew(appBar->msgData, Title2_Text_Confirm + i);
        appBar->items[i].color = PRINT_COLOR(14, 15, 3);
        appBar->items[i].type = i;
        if (appBar->wins[i] != NULL) {
            AppTaskMenuWin_Free(appBar->wins[i]);
        }
        appBar->wins[i] = AppTaskMenuWin_Create(appBar->res, &appBar->items[i], 14 + i * 9, 21, 9, heapId);
    }
}

static BOOL ConfigPanelAppBar_IsTouched(ConfigPanelAppBar *appBar) {
    return appBar->touched;
}

static void ConfigPanelScroll_Init(ConfigPanelScroll *scroll, u16 textBg, u16 rowsBg, ConfigPanelGraphic *graphic,
                                   const ConfigPanelOptions *options, HeapID heapId) {
    int i;

    sys_memset(scroll, 0, sizeof(ConfigPanelScroll));
    scroll->textBg = textBg;
    scroll->rowsBg = rowsBg;
    scroll->y = 16;
    scroll->options = *options;
    for (i = 0; i < ITEM_COUNT; i++) {
        scroll->actors[i] = ConfigPanelGraphic_GetActor(graphic, ACTOR_ITEM + i);
    }
    ConfigPanelScroll_Move(scroll, -8, -192, 192);
    if (func_0203d554() == TRUE) {
        scroll->cursor = -1;
        ConfigPanelScroll_Draw(scroll, FALSE);
        scroll->update = FALSE;
    } else {
        scroll->cursor = 0;
        ConfigPanelScroll_Draw(scroll, TRUE);
        scroll->update = TRUE;
    }
}

static void ConfigPanelScroll_Exit(ConfigPanelScroll *scroll) {
    sys_memset(scroll, 0, sizeof(ConfigPanelScroll));
}

static void ConfigPanelScroll_Main(ConfigPanelScroll *scroll, ConfigPanelUi *ui, ConfigPanelMsg *msg,
                                   ConfigPanelGraphic *graphic, ConfigPanelAppBar *appBar) {
    u32 input = ConfigPanelUi_GetInput(ui);
    ConfigPanelPoint point;

    GFL_BGSysGetBGOffsetY(scroll->rowsBg);
    switch (input) {
    case INPUT_KEY_MODE:
        scroll->cursor = MATH_CLAMP(scroll->cursor, 0, CURSOR_MAX);
        ConfigPanelScroll_Draw(scroll, TRUE);
        scroll->update = TRUE;
        break;
    case INPUT_UP_HELD:
        if (scroll->repeat++ < 10) {
            break;
        }
        // fallthrough
    case INPUT_UP:
        scroll->repeat = 0;
        if (scroll->cursor == 0) {
            scroll->cursor = CURSOR_MAX;
        } else {
            scroll->cursor--;
        }
        scroll->update = TRUE;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        ConfigPanelScroll_Draw(scroll, TRUE);
        ConfigPanelGraphic_ResetPulse(graphic);
        break;
    case INPUT_DOWN_HELD:
        if (scroll->repeat++ < 10) {
            break;
        }
        // fallthrough
    case INPUT_DOWN:
        scroll->repeat = 0;
        if (scroll->cursor == CURSOR_MAX) {
            scroll->cursor = 0;
        } else {
            scroll->cursor++;
        }
        scroll->update = TRUE;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        ConfigPanelScroll_Draw(scroll, TRUE);
        ConfigPanelGraphic_ResetPulse(graphic);
        break;
    case INPUT_TOUCH:
        ConfigPanelUi_GetPoint(ui, POINT_START, &point);
        if (point.y >= ROWS_TOP && point.y <= ROWS_BOTTOM) {
            scroll->cursor = ConfigPanelScroll_GetTouchedRow(scroll, &point);
            ConfigPanelScroll_TouchItems(scroll, &point);
            ConfigPanelScroll_Draw(scroll, FALSE);
            ConfigPanelGraphic_ResetPulse(graphic);
            scroll->update = TRUE;
            if (!func_0203d554()) {
                func_0203d564(TRUE);
            }
        }
        break;
    case INPUT_RIGHT:
        if (scroll->cursor < OPTION_COUNT) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            ConfigPanelScroll_Step(scroll, 1);
            ConfigPanelScroll_Draw(scroll, TRUE);
            ConfigPanelGraphic_ResetPulse(graphic);
        } else if (scroll->cursor == CURSOR_CONFIRM) {
            scroll->cursor++;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            scroll->cursor = MATH_CLAMP(scroll->cursor, 0, CURSOR_MAX);
        }
        break;
    case INPUT_LEFT:
        if (scroll->cursor < OPTION_COUNT) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            ConfigPanelScroll_Step(scroll, -1);
            ConfigPanelScroll_Draw(scroll, TRUE);
            ConfigPanelGraphic_ResetPulse(graphic);
        } else if (scroll->cursor == CURSOR_QUIT) {
            scroll->cursor--;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            scroll->cursor = MATH_CLAMP(scroll->cursor, 0, CURSOR_MAX);
        }
        break;
    case INPUT_X:
        scroll->cursor = MATH_CLAMP(scroll->cursor, 0, CURSOR_MAX);
        break;
    case INPUT_Y:
        scroll->cursor = MATH_CLAMP(scroll->cursor, 0, CURSOR_MAX);
        ConfigPanelScroll_Draw(scroll, TRUE);
        break;
    case INPUT_DECIDE:
    case INPUT_CANCEL:
        ConfigPanelScroll_Draw(scroll, FALSE);
        break;
    }

    // A new text speed shows in the explanation
    switch (ConfigPanelOptions_PopChanged(&scroll->options)) {
    case 0:
    case 1:
    case 2:
        scroll->update = TRUE;
        break;
    }

    if (scroll->update) {
        if (scroll->cursor >= 0 && scroll->cursor < OPTION_COUNT) {
            BOOL stream;
            s32 wait;

            if (scroll->cursor == OPTION_TEXT_SPEED) {
                stream = TRUE;
                wait = ConfigPanelOptions_GetTextWait(&scroll->options);
            } else {
                stream = FALSE;
                wait = 0;
            }
            GFL_BGSysSetBGEnabled(BG_MESSAGE, TRUE);
            ConfigPanelMsg_Print(msg, sConfigPanelOptions[scroll->cursor].msgId, wait, stream);
        } else {
            GFL_BGSysSetBGEnabled(BG_MESSAGE, FALSE);
        }
        scroll->update = FALSE;
    }
    ConfigPanelMsg_IsDone(msg);
    ConfigPanelScroll_Tick(scroll);
}

static void ConfigPanelScroll_HideCursor(ConfigPanelScroll *scroll, ConfigPanelGraphic *graphic) {
    ConfigPanelScroll_Draw(scroll, FALSE);
    GFL_BGSysSetBGEnabled(BG_MESSAGE, FALSE);
    ConfigPanelGraphic_ResetPulse(graphic);
}

static s32 ConfigPanelScroll_GetCursor(ConfigPanelScroll *scroll) {
    return scroll->cursor;
}

static void ConfigPanelScroll_GetOptions(ConfigPanelScroll *scroll, ConfigPanelOptions *options) {
    *options = scroll->options;
}

static void ConfigPanelScroll_Redraw(ConfigPanelScroll *scroll, BOOL unused) {
    ConfigPanelScroll_Draw(scroll, scroll->cursorVisible);
}

static void ConfigPanelScroll_SetSaveBeforeIr(ConfigPanelScroll *scroll, u8 value) {
    scroll->options.values[OPTION_SAVE_BEFORE_IR] = value;
    ConfigPanelScroll_Draw(scroll, scroll->cursorVisible);
}

// Sets the palettes of the rows, the texts and the values' frames for the cursor and the values picked
static void ConfigPanelScroll_Draw(ConfigPanelScroll *scroll, BOOL cursorVisible) {
    int row;
    u32 j;
    int i;

    if (scroll->cursor >= OPTION_COUNT) {
        cursorVisible = FALSE;
    }
    scroll->cursorVisible = cursorVisible;

    row = (GFL_BGSysGetBGOffsetY(scroll->rowsBg) + 20) / 24;
    GFL_BGSysSetScrPaletteNo(scroll->rowsBg, 0, 0, 32, 32, 5);
    for (j = 0; j < OPTION_COUNT; j++) {
        GFL_BGSysSetScrPaletteNo(scroll->rowsBg, 0, (row + j) * 3 + 2, 32, 3, sConfigPanelRowPalettes[j]);
    }

    for (i = 0; i < ITEM_COUNT; i++) {
        if (scroll->cursor == sConfigPanelItems[i].option && cursorVisible) {
            if (ConfigPanelOptions_IsSet(&scroll->options, i)) {
                GFL_BGSysSetScrPaletteNo(scroll->textBg, sConfigPanelLabels[i].x, sConfigPanelLabels[i].y,
                                         sConfigPanelLabels[i].width, sConfigPanelLabels[i].height, 7);
            } else {
                GFL_BGSysSetScrPaletteNo(scroll->textBg, sConfigPanelLabels[i].x, sConfigPanelLabels[i].y,
                                         sConfigPanelLabels[i].width, sConfigPanelLabels[i].height, 12);
            }
        } else {
            if (ConfigPanelOptions_IsSet(&scroll->options, i)) {
                GFL_BGSysSetScrPaletteNo(scroll->textBg, sConfigPanelLabels[i].x, sConfigPanelLabels[i].y,
                                         sConfigPanelLabels[i].width, sConfigPanelLabels[i].height, 0);
            } else {
                GFL_BGSysSetScrPaletteNo(scroll->textBg, sConfigPanelLabels[i].x, sConfigPanelLabels[i].y,
                                         sConfigPanelLabels[i].width, sConfigPanelLabels[i].height, 3);
            }
        }
    }
    for (i = WINDOW_OPTION; i < LABEL_COUNT; i++) {
        if (i - WINDOW_OPTION == scroll->cursor && cursorVisible) {
            GFL_BGSysSetScrPaletteNo(scroll->textBg, sConfigPanelLabels[i].x, sConfigPanelLabels[i].y,
                                     sConfigPanelLabels[i].width, sConfigPanelLabels[i].height, 7);
        } else {
            GFL_BGSysSetScrPaletteNo(scroll->textBg, sConfigPanelLabels[i].x, sConfigPanelLabels[i].y,
                                     sConfigPanelLabels[i].width, sConfigPanelLabels[i].height, 0);
        }
    }
    if (cursorVisible) {
        GFL_BGSysSetScrPaletteNo(scroll->rowsBg, 0, scroll->cursor * 3 + 2, 32, 3, 7);
    }
    GFL_BGSysQueueScrLoad(scroll->rowsBg);
    GFL_BGSysQueueScrLoad(scroll->textBg);

    for (i = 0; i < ITEM_COUNT; i++) {
        if (scroll->cursor == sConfigPanelItems[i].option && cursorVisible) {
            if (ConfigPanelOptions_IsSet(&scroll->options, i)) {
                func_0204c4b8(scroll->actors[i], 3);
            } else {
                func_0204c4b8(scroll->actors[i], 2);
            }
        } else {
            if (ConfigPanelOptions_IsSet(&scroll->options, i)) {
                func_0204c4b8(scroll->actors[i], 0);
            } else {
                func_0204c4b8(scroll->actors[i], 1);
            }
        }
    }
}

// Scrolls the rows by dy, within min and max, and moves the values' frames with them
static void ConfigPanelScroll_Move(ConfigPanelScroll *scroll, int dy, int min, int max) {
    int i;
    ClActorPos pos;
    int y = GFL_BGSysGetBGOffsetY(scroll->rowsBg);

    y += dy;
    y = MATH_CLAMP(y, min, max);
    GFL_BGSysMoveBGReq(scroll->textBg, 3, y + 20);
    GFL_BGSysMoveBGReq(scroll->rowsBg, 3, y);

    scroll->y -= dy;
    scroll->y = MATH_CLAMP(scroll->y, 16 - max, 16 - min);
    for (i = 0; i < ITEM_COUNT; i++) {
        pos.x = sConfigPanelItems[i].x;
        pos.y = scroll->y + sConfigPanelItems[i].y;
        func_0204c140(scroll->actors[i], &pos, 0);
    }
}

// Picks the value at a point of the touch screen
static void ConfigPanelScroll_TouchItems(ConfigPanelScroll *scroll, const ConfigPanelPoint *point) {
    int i;
    ConfigPanelRect rect;

    for (i = 0; i < ITEM_COUNT; i++) {
        rect.top = scroll->y + sConfigPanelItems[i].hitY;
        rect.left = sConfigPanelItems[i].hitX;
        rect.bottom = rect.top + sConfigPanelItems[i].hitHeight;
        rect.right = rect.left + sConfigPanelItems[i].hitWidth;
        if (rect.top >= ROWS_TOP && rect.top <= ROWS_BOTTOM && rect.bottom >= ROWS_TOP && rect.bottom <= ROWS_BOTTOM &&
            ConfigPanel_IsInRect(&rect, point)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            ConfigPanelOptions_SetItem(&scroll->options, i);
            ConfigPanelScroll_ApplySound(scroll, scroll->options.values[OPTION_SOUND], FALSE);
            return;
        }
    }
}

// Switches the sound output while the cursor is on the option, so the change is heard, or always when forced
static void ConfigPanelScroll_ApplySound(ConfigPanelScroll *scroll, s8 sound, BOOL force) {
    if (scroll->cursor == OPTION_SOUND || force == TRUE) {
        if (sound == 0) {
            func_02005c80(TRUE);
        } else {
            func_02005c80(FALSE);
        }
    }
}

static void ConfigPanelScroll_Step(ConfigPanelScroll *scroll, int dir) {
    if (scroll->cursor < OPTION_COUNT) {
        ConfigPanelOptions_Step(&scroll->options, scroll->cursor, dir);
        ConfigPanelScroll_ApplySound(scroll, scroll->options.values[scroll->cursor], FALSE);
    }
}

// The row at a point of the touch screen, or the cursor when it is on none
static s32 ConfigPanelScroll_GetTouchedRow(ConfigPanelScroll *scroll, const ConfigPanelPoint *point) {
    int y = GFL_BGSysGetBGOffsetY(scroll->rowsBg);
    u32 row = (y + point->y - 16) / 24;

    if (row >= OPTION_COUNT) {
        row = scroll->cursor;
    }
    return row;
}

static void ConfigPanelScroll_Tick(ConfigPanelScroll *scroll) {
    scroll->unk3C *= 2;
}

static void ConfigPanelConfirm_Init(ConfigPanelConfirm *confirm, BmpWin *window, Font *font, MsgData *msgData,
                                    TCBExManager *tcbManager, AppTaskMenuRes *res, PrintQueue *queue, u16 heapId) {
    int i;

    sys_memset(confirm, 0, sizeof(ConfigPanelConfirm));
    confirm->result = CONFIRM_NONE;
    confirm->res = res;
    ConfigPanelMsg_Init(&confirm->msg, window, font, msgData, tcbManager, queue, heapId);
    for (i = 0; i < 2; i++) {
        confirm->items[i].str = GFL_MsgDataLoadStrbufNew(msgData, Title2_Text_Yes + i);
        confirm->items[i].color = PRINT_COLOR(14, 15, 3);
        confirm->items[i].type = 0;
    }
    confirm->init.heapId = heapId;
    confirm->init.itemCount = 2;
    confirm->init.items = confirm->items;
    confirm->init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    confirm->init.x = 32;
    confirm->init.y = 12;
    confirm->init.width = 13;
    confirm->init.height = 3;
}

static void ConfigPanelConfirm_Exit(ConfigPanelConfirm *confirm) {
    int i;

    for (i = 0; i < 2; i++) {
        GFL_StrBufFree(confirm->items[i].str);
    }
    ConfigPanelMsg_Exit(&confirm->msg);
    sys_memset(confirm, 0, sizeof(ConfigPanelConfirm));
}

static void ConfigPanelConfirm_Main(ConfigPanelConfirm *confirm) {
    switch (confirm->state) {
    case 0:
        if (ConfigPanelMsg_IsDone(&confirm->msg)) {
            confirm->state = 1;
        }
        break;
    case 1:
        confirm->menu = AppTaskMenu_Create(&confirm->init, confirm->res);
        confirm->state = 2;
        break;
    case 2:
        AppTaskMenu_Update(confirm->menu);
        if (AppTaskMenu_IsFlashFinished(confirm->menu)) {
            confirm->result = AppTaskMenu_GetCursorPos(confirm->menu) + 1;
            confirm->state = 3;
            AppTaskMenu_Free(confirm->menu);
        }
        break;
    }
}

// Asks whether to make the changes
static void ConfigPanelConfirm_Start(ConfigPanelConfirm *confirm, s32 wait) {
    gfxRegSetBrightnessBlend(
        REG_BLDCNT_ADDR,
        GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    GFL_BGSysSetBGEnabled(BG_CONFIRM, TRUE);
    ConfigPanelMsg_Print(&confirm->msg, Title2_Text_WouldLikeMakeThese, wait, FALSE);
}

static u32 ConfigPanelConfirm_GetResult(ConfigPanelConfirm *confirm) {
    return confirm->result;
}

// Prints a message of the script's text over the faded screen
static void ConfigPanelConfirm_PrintScriptMsg(ConfigPanelConfirm *confirm, u32 msgId, s32 wait) {
    MsgData *msgData;

    gfxRegSetBrightnessBlend(
        REG_BLDCNT_ADDR,
        GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    GFL_BGSysSetBGEnabled(BG_CONFIRM, TRUE);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520,
                                 HEAPID_CONFIG_PANEL | HEAPID_TAIL_BIT);
    ConfigPanelMsg_PrintFrom(&confirm->msg, msgData, msgId, wait, TRUE);
    GFL_MsgDataFree(msgData);
}

static void ConfigPanelConfirm_PrintMsg(ConfigPanelConfirm *confirm, u32 msgId, s32 wait) {
    gfxRegSetBrightnessBlend(
        REG_BLDCNT_ADDR,
        GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    GFL_BGSysSetBGEnabled(BG_CONFIRM, TRUE);
    ConfigPanelMsg_Print(&confirm->msg, msgId, wait, TRUE);
}

static BOOL ConfigPanelConfirm_IsMsgDone(ConfigPanelConfirm *confirm) {
    if (ConfigPanelMsg_IsDone(&confirm->msg)) {
        G2_BlendNone();
        GFL_BGSysSetBGEnabled(BG_CONFIRM, FALSE);
        return TRUE;
    }
    return FALSE;
}

static void ConfigPanelOptions_Init(ConfigPanelOptions *options, Config *config) {
    sys_memset(options, 0, sizeof(ConfigPanelOptions));
    options->changed = ITEM_COUNT;
    options->config = config;
    options->values[OPTION_TEXT_SPEED] = func_02008a14(config);
    options->values[OPTION_BATTLE_SCENE] = func_02008a4c(config);
    options->values[OPTION_BATTLE_STYLE] = func_02008a68(config);
    options->values[OPTION_SOUND] = getSoundFlags(config);
    options->values[OPTION_SAVE_BEFORE_IR] = func_02008ac8(config);
}

static void ConfigPanelOptions_Exit(ConfigPanelOptions *options) {
    sys_memset(options, 0, sizeof(ConfigPanelOptions));
}

// Writes the options to the save's
static void ConfigPanelOptions_Apply(const ConfigPanelOptions *options) {
    func_02008a1c(options->config, options->values[OPTION_TEXT_SPEED]);
    func_02008a54(options->config, options->values[OPTION_BATTLE_SCENE]);
    func_02008a70(options->config, options->values[OPTION_BATTLE_STYLE]);
    func_02008a38(options->config, options->values[OPTION_SOUND]);
    func_02008ad0(options->config, options->values[OPTION_SAVE_BEFORE_IR]);
}

static void ConfigPanelOptions_SetItem(ConfigPanelOptions *options, u32 item) {
    if (options->values[sConfigPanelItems[item].option] != sConfigPanelItems[item].value) {
        options->values[sConfigPanelItems[item].option] = sConfigPanelItems[item].value;
        options->changed = item;
        ConfigPanelOptions_Apply(options);
    }
}

// Moves an option to the value before or after its own
static void ConfigPanelOptions_Step(ConfigPanelOptions *options, u32 option, int dir) {
    int i;
    int item = -1;
    s8 pos;

    for (i = 0; i < sConfigPanelOptions[option].count; i++) {
        item = sConfigPanelOptions[option].items[i];
        if (sConfigPanelItems[item].value == options->values[option]) {
            break;
        }
    }
    pos = item - sConfigPanelOptions[option].items[0];
    pos = pos + dir;
    pos = MATH_CLAMP(pos, 0, sConfigPanelOptions[option].count - 1);
    item = sConfigPanelOptions[option].items[pos];
    if (options->values[option] != sConfigPanelItems[item].value) {
        options->values[option] = sConfigPanelItems[item].value;
        options->changed = item;
        ConfigPanelOptions_Apply(options);
    }
}

static BOOL ConfigPanelOptions_IsSet(const ConfigPanelOptions *options, u32 item) {
    if (options->values[sConfigPanelItems[item].option] == sConfigPanelItems[item].value) {
        return TRUE;
    }
    return FALSE;
}

// The wait between characters of the text speed, which ConfigPanelOptions_Apply has already written to the save
static s32 ConfigPanelOptions_GetTextWait(const ConfigPanelOptions *options) {
    return func_02017bcc();
}

static u32 ConfigPanelOptions_PopChanged(ConfigPanelOptions *options) {
    u32 changed = options->changed;

    options->changed = ITEM_COUNT;
    return changed;
}

static BOOL ConfigPanelOptions_Equals(const ConfigPanelOptions *a, const ConfigPanelOptions *b) {
    if (GFL_STD_MemCmp(a, b, OPTION_COUNT) == 0) {
        return TRUE;
    }
    return FALSE;
}

static void ConfigPanelSeq_Init(ConfigPanelSeq *seq, void *data, ConfigPanelSeqFunc func) {
    sys_memset(seq, 0, sizeof(ConfigPanelSeq));
    seq->data = data;
    ConfigPanelSeq_SetNext(seq, func);
}

static void ConfigPanelSeq_Exit(ConfigPanelSeq *seq) {
    sys_memset(seq, 0, sizeof(ConfigPanelSeq));
}

static void ConfigPanelSeq_Main(ConfigPanelSeq *seq) {
    if (!seq->ended) {
        seq->func(seq, &seq->state, seq->data);
    }
}

static BOOL ConfigPanelSeq_IsEnded(ConfigPanelSeq *seq) {
    return seq->ended;
}

static void ConfigPanelSeq_SetNext(ConfigPanelSeq *seq, ConfigPanelSeqFunc func) {
    seq->func = func;
    seq->state = 0;
}

static void ConfigPanelSeq_End(ConfigPanelSeq *seq) {
    seq->ended = TRUE;
}

static void ConfigPanelSeq_FadeIn(ConfigPanelSeq *seq, u32 *state, void *data) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Input);
        break;
    }
}

static void ConfigPanelSeq_FadeOut(ConfigPanelSeq *seq, u32 *state, void *data) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        ConfigPanelSeq_End(seq);
        break;
    }
}

static void ConfigPanelSeq_Input(ConfigPanelSeq *seq, u32 *state, void *data) {
    ConfigPanelWork *wk = data;
    s32 button;

    if (!func_02021c0c(wk->queue)) {
        return;
    }

    switch (*state) {
    case 0:
        ConfigPanelUi_Main(&wk->ui);
        if (!ConfigPanelAppBar_GetDecided(&wk->appBar, NULL)) {
            ConfigPanelScroll_Main(&wk->scroll, &wk->ui, &wk->msg, &wk->graphic, &wk->appBar);
        }
        ConfigPanelAppBar_Main(&wk->appBar, &wk->ui, &wk->scroll);
        if (ConfigPanelAppBar_IsTouched(&wk->appBar)) {
            ConfigPanelScroll_HideCursor(&wk->scroll, &wk->graphic);
        }

        if (ConfigPanelAppBar_GetDecided(&wk->appBar, &button)) {
            switch (button) {
            case BUTTON_CONFIRM:
                if (ConfigPanelAppBar_IsFlashFinished(&wk->appBar)) {
                    ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Save);
                }
                break;
            case BUTTON_CLOSE: {
                ConfigPanelOptions options;

                wk->param->result = TRUE;
                ConfigPanelScroll_GetOptions(&wk->scroll, &options);
                if (ConfigPanelOptions_Equals(&wk->saved, &options)) {
                    ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Quit);
                } else {
                    ConfigPanelAppBar_ResetButtons(&wk->appBar);
                    ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Confirm);
                }
                break;
            }
            case BUTTON_QUIT:
                if (ConfigPanelAppBar_IsFlashFinished(&wk->appBar)) {
                    ConfigPanelOptions options;

                    ConfigPanelScroll_GetOptions(&wk->scroll, &options);
                    if (ConfigPanelOptions_Equals(&wk->saved, &options)) {
                        ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Quit);
                    } else {
                        ConfigPanelAppBar_ResetButtons(&wk->appBar);
                        ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Confirm);
                    }
                }
                break;
            }
        }

        // The save of another game can't be overwritten, so there is nothing to save before infrared
        {
            BOOL otherSave = func_0200746c(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys)));
            ConfigPanelOptions options;

            ConfigPanelScroll_GetOptions(&wk->scroll, &options);
            if (options.values[OPTION_SAVE_BEFORE_IR] == 0 && otherSave) {
                *state = 1;
            }
        }
        break;
    case 1: {
        ConfigPanelOptions options;

        ConfigPanelScroll_GetOptions(&wk->scroll, &options);
        ConfigPanelConfirm_PrintMsg(&wk->confirm, Title2_Text_HaveSavedDataAlready,
                                    ConfigPanelOptions_GetTextWait(&options));
        ConfigPanelScroll_SetSaveBeforeIr(&wk->scroll, 1);
        *state = 5;
        break;
    }
    case 2: {
        ConfigPanelOptions options;

        ConfigPanelScroll_GetOptions(&wk->scroll, &options);
        ConfigPanelConfirm_PrintScriptMsg(&wk->confirm, Global10520_Text_WirelessCommunicationsTurnedOff,
                                          ConfigPanelOptions_GetTextWait(&options));
        ConfigPanelScroll_Redraw(&wk->scroll, FALSE);
        *state = 5;
        break;
    }
    case 3: {
        ConfigPanelOptions options;

        ConfigPanelScroll_GetOptions(&wk->scroll, &options);
        ConfigPanelConfirm_PrintScriptMsg(&wk->confirm, Global10520_Text_WirelessCommunicationsTurnedOff,
                                          ConfigPanelOptions_GetTextWait(&options));
        ConfigPanelScroll_Redraw(&wk->scroll, FALSE);
        *state = 5;
        break;
    }
    case 4: {
        ConfigPanelOptions options;

        ConfigPanelScroll_GetOptions(&wk->scroll, &options);
        ConfigPanelConfirm_PrintMsg(&wk->confirm, Title2_Text_Empty_8, ConfigPanelOptions_GetTextWait(&options));
        ConfigPanelScroll_Redraw(&wk->scroll, TRUE);
        *state = 5;
        break;
    }
    case 5:
        if (ConfigPanelConfirm_IsMsgDone(&wk->confirm)) {
            *state = 0;
        }
        break;
    }
}

// Asks whether to make the changes, then saves them or quits
static void ConfigPanelSeq_Confirm(ConfigPanelSeq *seq, u32 *state, void *data) {
    ConfigPanelWork *wk = data;

    switch (*state) {
    case 0: {
        ConfigPanelOptions options;

        ConfigPanelScroll_GetOptions(&wk->scroll, &options);
        ConfigPanelConfirm_Start(&wk->confirm, ConfigPanelOptions_GetTextWait(&options));
        GFL_BGSysSetBGEnabled(BG_MESSAGE, FALSE);
        *state = 1;
        break;
    }
    case 1:
        switch (ConfigPanelConfirm_GetResult(&wk->confirm)) {
        case CONFIRM_YES:
            *state = 2;
            break;
        case CONFIRM_NO:
            *state = 3;
            break;
        }
        break;
    case 2:
        ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Save);
        break;
    case 3:
        ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_Quit);
        break;
    }
    ConfigPanelConfirm_Main(&wk->confirm);
}

// Puts the options back as they were
static void ConfigPanelSeq_Quit(ConfigPanelSeq *seq, u32 *state, void *data) {
    ConfigPanelWork *wk = data;

    ConfigPanelOptions_Apply(&wk->saved);
    ConfigPanelScroll_ApplySound(&wk->scroll, wk->saved.values[OPTION_SOUND], TRUE);
    ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_FadeOut);
}

static void ConfigPanelSeq_Save(ConfigPanelSeq *seq, u32 *state, void *data) {
    ConfigPanelWork *wk = data;
    ConfigPanelOptions options;

    ConfigPanelScroll_GetOptions(&wk->scroll, &options);
    ConfigPanelOptions_Apply(&options);
    GFL_SndSEPlay(SEQ_SE_SAVE);
    ConfigPanelSeq_SetNext(seq, ConfigPanelSeq_FadeOut);
}

static BOOL ConfigPanel_IsInRect(const ConfigPanelRect *rect, const ConfigPanelPoint *point) {
    return ((u32)(point->x - rect->left) <= (u32)(rect->right - rect->left)) &
           ((u32)(point->y - rect->top) <= (u32)(rect->bottom - rect->top));
}
