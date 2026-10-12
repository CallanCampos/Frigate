#include "types.h"
#include "app/name_entry.h"
#include "app/namein_data.h"
#include "app/namein_graphic.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/join_avenue.h"
#include "save/save_control.h"
#include "save/save_control_intr.h"
#include "system/app_menu_common.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/nsbtx_to_clwk.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/prof_word.h"
#include "system/str_tool.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/ability_handlers_btl_main.h"

// The name entry screen: a name field along the top, a keyboard of keys that the keys or the touch screen pick, and the
// icon of whatever is being named. Overlay 280, with the graphics of namein_graphic.c and the tables of namein_data.c.
// The ROM gives this file no name; "namein" follows its siblings. The names are ours

// The archive of the screen's graphics, and two archives of the graphics of some of the icons
#define ARCID_NAMEIN_GRAPHIC 0x77
#define ARCID_NAMEIN_ICON_TYPE2 0x1f
#define ARCID_NAMEIN_ICON_TYPE6 0x53

// How many cell actors the screen has: the name field's, the key highlight's and the selectors'
#define OBJ_COUNT 24
// The cell actors of the name field, one for each possible character
#define NAME_SLOT_COUNT 16

// The characters that a Game Sync ID is made of
#define GAME_SYNC_ID_CHAR_COUNT 32

// What the keyboard reports after a frame
enum {
    KEY_RESULT_NONE,
    KEY_RESULT_CHAR,
    KEY_RESULT_COMPOSE_CHAR,
    KEY_RESULT_DAKUTEN,
    KEY_RESULT_HANDAKUTEN,
    KEY_RESULT_BACKSPACE,
    KEY_RESULT_CLEAR_COMPOSE,
    KEY_RESULT_OK,
    KEY_RESULT_SWITCH_PAGE,
    KEY_RESULT_SPACE,
    KEY_RESULT_CYCLE,
    KEY_RESULT_COMMIT_COMPOSE,
};

// What is under a position of a keyboard layout
enum {
    KEY_KIND_NONE,
    KEY_KIND_CHAR,
    KEY_KIND_TYPE_0,
    KEY_KIND_TYPE_1,
    KEY_KIND_TYPE_2,
    KEY_KIND_TYPE_3,
    KEY_KIND_BACKSPACE,
    KEY_KIND_OK,
    KEY_KIND_SWITCH_PAGE,
    KEY_KIND_SPACE,
    KEY_KIND_DAKUTEN,
    KEY_KIND_HANDAKUTEN,
};

// The keyboard types
enum {
    KEYBOARD_TYPE_0,
    KEYBOARD_TYPE_1,
    KEYBOARD_TYPE_2,
    KEYBOARD_TYPE_KANA,
};

// What an icon shows
enum {
    ICON_TYPE_PLAYER,
    ICON_TYPE_RIVAL,
    ICON_TYPE_FRIEND,
    ICON_TYPE_POKEMON,
    ICON_TYPE_BOX,
    ICON_TYPE_KEY,
    ICON_TYPE_AVENUE,
    ICON_TYPE_POKEMON_NO_LABEL,
};

typedef struct {
    int x;
    int y;
} KeyPos;

// A rectangle in tiles
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} KeyRect;

typedef struct {
    u32 x;
    u32 y;
} TouchPos;

typedef struct NameEntryObjs {
    u32 palette;
    u32 chars;
    u32 cellAnims;
    ClActor *actors[OBJ_COUNT];
} NameEntryObjs;

// The blinking highlight of the key that is selected, whose colour changes between two palettes
typedef struct KeyHighlight {
    u16 baseColor[16];
    u16 altColor[16];
    u16 color[16];
    u16 phase[16];
    u16 timer;
    KeyRect rect;
    u32 mode;
    BOOL active;
    u32 kanaPage;
    u32 type;
    ClActor *actors[2];
    ClActorPos pos[2];
} KeyHighlight;

typedef struct NameBox {
    u16 text[11];
    u16 pending[4];
    u16 length;
    u16 pendingLength;
    u16 maxLength;
    u16 centered;
    BOOL dirty;
    BmpWin *printWindow;
    u8 printing;
    Font *font;
    PrintQueue *queue;
    StrBuf *buf;
    BmpWin *window;
    ClActor *slots[NAME_SLOT_COUNT];
    NameinWordTable *words[3];
    NameinConvTable *conv;
} NameBox;

typedef struct KeyLayout {
    NameinKeyboard *keyboard;
    u8 memory[13];
    BOOL (*getRect)(const KeyPos *pos, KeyRect *rect);
    void (*move)(KeyPos *pos, const KeyPos *delta, u8 *memory);
    int (*getKind)(const KeyPos *pos, KeyPos *cell);
    u16 width;
    u16 height;
} KeyLayout;

// The window that the keys are printed to, and whether they are still being printed
typedef struct KeyPrint {
    BmpWin *window;
    u8 printing;
} KeyPrint;

// What the player did on a frame: a button, or a position that was moved to or touched
typedef struct KeyInput {
    int button;
    BOOL moved;
    KeyPos pos;
} KeyInput;

typedef struct Keyboard {
    KeyLayout layout;
    KeyHighlight highlight;
    KeyPos pos;
    BmpWin *window;
    u32 state;
    u32 result;
    KeyPrint print;
    Font *font;
    PrintQueue *queue;
    u16 lastChar;
    u16 heapId;
    u32 type;
    u32 kanaPage;
    u8 kanaGroup;
    u16 slideFrame;
    u16 slideStage;
    BOOL sliding;
    u32 nextType;
    BOOL hasUpperLayer;
    void *screenData[2];
    NNSG2dScreenData *screens[2];
    NameinKeyboardSet *keyboards;
    BOOL locked;
} Keyboard;

// The message at the bottom of the screen
typedef struct NameEntryMessage {
    MsgData *msgData;
    Font *font;
    BmpWin *printWindow;
    u8 printing;
    PrintQueue *queue;
    BmpWin *window;
    WordSet *wordSet;
    StrBuf *buf;
    u16 fillColor;
} NameEntryMessage;

typedef struct NameEntryTalk NameEntryTalk;
typedef struct NameEntryWork NameEntryWork;
typedef struct NameEntryTask NameEntryTask;
typedef void (*NameEntryTaskFunc)(NameEntryTask *task, u32 *state, NameEntryWork *work);

// The message that is printed a letter at a time before the screen closes
struct NameEntryTalk {
    PrintStream *stream;
    TCBExManager *tcbManager;
    NameEntryTaskFunc next;
    u16 waitFrames;
    u16 frame;
};

// The icon of what is being named, with its label
typedef struct NameEntryIcon {
    ClActor *actor;
    u32 unk4;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    BOOL touched;
    u32 state;
    BOOL bouncing;
    u16 frame;
    u32 labelPalette;
    StrBuf *label;
    BmpOamActor *labelActor;
    GFLBitmap *labelBitmap;
    BOOL labelDirty;
    PrintQueue *labelQueue;
    int baseY;
} NameEntryIcon;

// The screen's task, a state machine that runs a function a frame until it chooses the next
struct NameEntryTask {
    NameEntryTaskFunc func;
    BOOL done;
    u32 state;
    NameEntryWork *work;
};

struct NameEntryWork {
    NameinGraphic *graphic;
    NameEntryObjs objs;
    u32 bgs;
    NameBox box;
    Keyboard keyboard;
    NameinKeyboardSet *keyboards;
    NameEntryMessage message;
    NameEntryTalk talk;
    NameEntryIcon icon;
    NameEntryTask task;
    Font *font;
    PrintQueue *queue;
    MsgData *msgData;
    WordSet *wordSet;
    BmpOamSys *bmpOam;
    NameEntryParam *param;
    BOOL messagePending;
    ProfWordLists *profWord;
    StrBuf *name;
    BOOL usesPresetNames;
    BOOL presetNameFilled;
};

BOOL NameEntry_ProcInit(GameProc *proc, u32 *state, void *arg, void *unused);
BOOL NameEntry_ProcExit(GameProc *proc, u32 *state, void *param, void *arg);
BOOL NameEntry_ProcMain(GameProc *proc, u32 *state, void *param, void *arg);

static void NameEntry_LoadBgGraphics(u32 *bgs, HeapID heapId);
static void NameEntry_FreeBgGraphics(u32 *bgs);
static void NameEntryObjs_Init(NameEntryObjs *objs, ClActUnit *unit, HeapID heapId);
static void NameEntryObjs_Free(NameEntryObjs *objs);
static ClActor *NameEntryObjs_Get(NameEntryObjs *objs, u32 index);
static void KeyHighlight_Init(KeyHighlight *highlight, u32 type, NameEntryObjs *objs, HeapID heapId);
static void KeyHighlight_Clear(KeyHighlight *highlight);
static void KeyHighlight_Set(KeyHighlight *highlight, u32 mode, const KeyRect *rect, u32 kanaPage, u32 type);
static void KeyHighlight_Update(KeyHighlight *highlight, u32 type, const KeyRect *rect);
static BOOL KeyHighlight_BlendColor(u16 *color, u16 *phase, u16 step, u32 palette, u8 index, u16 from, u16 to);
static void NameBox_Init(NameBox *box, StrBuf *name, u8 maxLength, Font *font, PrintQueue *queue,
                         NameEntryObjs *objs, HeapID heapId);
static void NameBox_Free(NameBox *box);
static BOOL NameBox_Update(NameBox *box);
static BOOL NameBox_DeleteChar(NameBox *box);
static BOOL NameBox_AddChar(NameBox *box, u16 c);
static BOOL NameBox_AddComposeChar(NameBox *box, u16 c, int set);
static BOOL NameBox_Compose(NameBox *box, int which);
static BOOL NameBox_CycleLast(NameBox *box);
static BOOL NameBox_IsPrintDone(NameBox *box);
static void NameBox_GetString(NameBox *box, StrBuf *dest);
static void NameBox_ClearCompose(NameBox *box);
static void NameBox_CommitCompose(NameBox *box);
static void NameBox_Clear(NameBox *box);
static void NameBox_SetString(NameBox *box, const u16 *str);
static BOOL NameBox_IsFull(NameBox *box);
static u16 NameBox_GetLength(NameBox *box);
static u16 NameBox_GetComposeLength(NameBox *box);
static BOOL NameBox_ComposeAll(NameBox *box, int set);
static BOOL NameBox_HasVisibleChar(NameBox *box);
static BOOL NameBox_IsValidGameSyncId(NameBox *box);
static BOOL NameEntry_IsGameSyncIdChar(u16 c);
static void KeyLayout_Init(KeyLayout *layout, u32 type, u32 index, const NameinKeyboardSet *keyboards, HeapID heapId);
static void KeyLayout_SetKeyboard(KeyLayout *layout, u32 index, const NameinKeyboardSet *keyboards);
static void KeyLayout_Clear(KeyLayout *layout);
static int KeyLayout_GetChar(KeyLayout *layout, const KeyPos *pos, u16 *c);
static BOOL KeyLayout_FindAtTouch(KeyLayout *layout, const TouchPos *touch, KeyPos *found);
static void KeyLayout_Move(KeyLayout *layout, KeyPos *pos, const KeyPos *delta);
static void KeyLayout_PrintKeys(KeyLayout *layout, KeyPrint *print, PrintQueue *queue, Font *font, u16 heapId);
static BOOL KeyLayout_GetRect(KeyLayout *layout, const KeyPos *pos, KeyRect *rect);
static BOOL KeyLayout_FindKind(KeyLayout *layout, int kind, KeyPos *found);
static void KeyLayout_ConvertPos(KeyPos *pos, u32 from, u32 to);
static BOOL AlphaLayout_GetRect(const KeyPos *pos, KeyRect *rect);
static BOOL KanaLayout_GetRect(const KeyPos *pos, KeyRect *rect);
static void AlphaLayout_Move(KeyPos *pos, const KeyPos *delta, u8 *memory);
static void KanaLayout_Move(KeyPos *pos, const KeyPos *delta, u8 *memory);
static int AlphaLayout_GetKindDefault(const KeyPos *pos, KeyPos *cell);
static int AlphaLayout_GetKind(const KeyPos *pos, KeyPos *cell);
static int KanaLayout_GetKind(const KeyPos *pos, KeyPos *cell);
static void Keyboard_Init(Keyboard *kb, u32 type, Font *font, PrintQueue *queue, NameinKeyboardSet *keyboards,
                          NameEntryObjs *objs, HeapID heapId);
static void Keyboard_Free(Keyboard *kb);
static void Keyboard_Update(Keyboard *kb, NameBox *box);
static int Keyboard_GetResult(Keyboard *kb, u16 *c);
static u32 Keyboard_GetKanaPage(Keyboard *kb);
static BOOL Keyboard_IsPrintDone(Keyboard *kb);
static u32 Keyboard_GetType(Keyboard *kb);
static void Keyboard_SetLocked(Keyboard *kb, BOOL locked);
static u32 NameEntry_GetLayoutIndex(u32 type, u32 page, u8 group);
static BOOL Keyboard_StartSwitch(Keyboard *kb, u32 type);
static BOOL Keyboard_UpdateSlide(Keyboard *kb);
static void Keyboard_ApplyType(Keyboard *kb, u32 type);
static void Keyboard_SetKanaPage(Keyboard *kb, u32 page, u32 group);
static BOOL Keyboard_HandleKeys(Keyboard *kb, KeyInput *input);
static BOOL Keyboard_HandleButtons(Keyboard *kb, KeyInput *input);
static BOOL Keyboard_HandleTouch(Keyboard *kb, KeyInput *input);
static void Keyboard_Select(Keyboard *kb, KeyInput *input);
static BOOL Keyboard_HandleKeysSlide(Keyboard *kb, KeyInput *input);
static BOOL Keyboard_HandleButtonsSlide(Keyboard *kb, KeyInput *input);
static BOOL Keyboard_HandleTouchSlide(Keyboard *kb, KeyInput *input);
static void Keyboard_SelectSlide(Keyboard *kb, KeyInput *input);
static void NameEntryMessage_Init(NameEntryMessage *message, Font *font, MsgData *msgData, PrintQueue *queue,
                                  WordSet *wordSet, HeapID heapId);
static void NameEntryMessage_Free(NameEntryMessage *message);
static void NameEntryMessage_Print(NameEntryMessage *message, u32 messageId);
static void NameEntryMessage_PrintSpecies(NameEntryMessage *message, u32 messageId, u16 species, u32 form,
                                          HeapID heapId);
static void NameEntryMessage_PrintPokemon(NameEntryMessage *message, u32 messageId, PartyPkm *pkm);
static BOOL NameEntryMessage_IsPrintDone(NameEntryMessage *message);
static void NameEntryTalk_Init(NameEntryTalk *talk, HeapID heapId);
static void NameEntryTalk_Free(NameEntryTalk *talk);
static void NameEntryTalk_Start(NameEntryTalk *talk, NameEntryMessage *message, NameEntryParam *param, HeapID heapId);
static void NameEntryIcon_Init(NameEntryIcon *icon, u32 type, u32 species, u32 form, ClActUnit *unit, Font *font,
                               PrintQueue *queue, MsgData *msgData, BmpOamSys *bmpOam, HeapID heapId);
static void NameEntryIcon_Free(NameEntryIcon *icon);
static void NameEntryIcon_Update(NameEntryIcon *icon);
static void NameEntryIcon_UpdateLabel(NameEntryIcon *icon);
static BOOL NameEntryIcon_WasTouched(NameEntryIcon *icon);
static void NameEntryIcon_StartBounce(NameEntryIcon *icon);
static BOOL NameEntryIcon_Bounce(NameEntryIcon *icon);
static u32 NameEntry_GetIconType(u32 mode, u32 species);
static void NameEntryObjs_ShowSelectors(NameEntryObjs *objs, BOOL show);
static int NameEntryObjs_UpdateSelectors(NameEntryObjs *objs);
static StrBuf *NameEntry_LoadPresetName(NameEntryParam *param, u32 which, HeapID heapId);
static BOOL NameEntry_PointInRect(const KeyRect *rect, const TouchPos *touch);
static StrBuf *NameEntry_GetPresetName(NameEntryWork *work, u32 mode, HeapID heapId);
static void NameEntry_CheckChanged(NameEntryWork *work);
static BOOL NameEntry_IsNameAcceptable(NameEntryWork *work);
static BOOL NameEntry_IsGameSyncIdValid(NameEntryWork *work);
static void NameEntry_UpperCaseName(StrBuf *name);
static void NameEntryTask_Init(NameEntryTask *task, NameEntryWork *work, NameEntryTaskFunc func);
static void NameEntryTask_Clear(NameEntryTask *task);
static void NameEntryTask_Run(NameEntryTask *task);
static BOOL NameEntryTask_IsDone(NameEntryTask *task);
static void NameEntryTask_SetFunc(NameEntryTask *task, NameEntryTaskFunc func);
static void NameEntryTask_Finish(NameEntryTask *task);
static void NameEntryTask_WaitDraw(NameEntryTask *task, u32 *state, NameEntryWork *work);
static void NameEntryTask_FadeIn(NameEntryTask *task, u32 *state, NameEntryWork *work);
static void NameEntryTask_FadeOut(NameEntryTask *task, u32 *state, NameEntryWork *work);
static void NameEntryTask_Talk(NameEntryTask *task, u32 *state, NameEntryWork *work);
static void NameEntryTask_Input(NameEntryTask *task, u32 *state, NameEntryWork *work);
static void NameEntryTask_SaveKeyboardType(NameEntryTask *task, u32 *state, NameEntryWork *work);
static void NameEntryTask_PauseSave(NameEntryTask *task, u32 *state, NameEntryWork *work);

// The original's .rodata starts with 8 words that nothing in this file refers to, 1, 0, 0, 3, 2, 7, 0, 0, which are not
// reproduced

// The kana keyboard's rows: where each starts and its top
static const u32 sKanaRowLeft[3] = { 5, 6, 7 };
static const u32 sKanaRowTop[3] = { 8, 12, 16 };

// Where the selection starts on each part of a keyboard layout
static const u8 sDefaultKeyMemory[13] = { 0, 2, 3, 5, 6, 10, 0, 1, 9, 8, 0, 4, 4 };

// The palette of the icon of each type
static const u8 sIconPalettes[16] = { 2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0 };

const u32 NAME_ENTRY_SOUND_COUNT = 6;

const GameProcFunctions NAME_ENTRY_PROC_FUNCTIONS = { NameEntry_ProcInit, NameEntry_ProcMain, NameEntry_ProcExit };

const u32 NAME_ENTRY_SOUNDS[6] = { SEQ_SE_SELECT1, SEQ_SE_CANCEL3, SEQ_SE_DECIDE1,
                                   SEQ_SE_SYS_12, SEQ_SE_DECIDE2, SEQ_SE_BEEP };

// The characters of a Game Sync ID (no I, O, 0 or 1)
static const u16 sGameSyncIdChars[GAME_SYNC_ID_CHAR_COUNT] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7', '8', '9',
};

// How fast the key highlight blinks
static u32 sKeyBlinkStep = 0x51e;

BOOL NameEntry_ProcInit(GameProc *proc, u32 *state, void *arg, void *unused) {
    NameEntryParam *param = arg;
    NameEntryWork *work;
    int keyboardType = 0;
    u32 slot;
    u16 species;
    ClActUnit *unit;

    GFL_OvlLoad(OVERLAY_PROF_WORD);
    if (param->mode != NAME_ENTRY_GAME_SYNC_ID && param->gameInfo != NULL) {
        // The modes that name the same kind of thing share a slot of the keyboard that was used last
        slot = param->mode;
        if (param->mode == NAME_ENTRY_IMPRESSED) {
            slot = NAME_ENTRY_FAVORITE_PHRASE;
        } else if (param->mode == NAME_ENTRY_TRAINER_MESSAGE) {
            slot = NAME_ENTRY_PASSERBY_MESSAGE;
        } else if (param->mode == NAME_ENTRY_HAPPY_MESSAGE) {
            slot = NAME_ENTRY_THANKS;
        } else if (param->mode == NAME_ENTRY_POKEMON_RENAME) {
            slot = NAME_ENTRY_POKEMON;
        }
        keyboardType = func_0200c84c(param->gameInfo, slot);
        if (keyboardType >= 5) {
            keyboardType = 0;
        }
    }

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_NAMEIN, 0x70000);
    work = GFL_ProcInitSubsystem(proc, sizeof(NameEntryWork), HEAPID_NAMEIN);
    sys_memset(work, 0, sizeof(NameEntryWork));
    work->param = param;
    if (param->mode == NAME_ENTRY_PASSERBY_MESSAGE || param->mode == NAME_ENTRY_TRAINER_MESSAGE) {
        work->usesPresetNames = TRUE;
    } else {
        work->usesPresetNames = FALSE;
    }

    work->font = GFL_FontCreate(ARCID_FONT, 0, 1, FALSE, HEAPID_NAMEIN);
    work->queue = func_020219a8(0x1000, HEAPID_NAMEIN);
    work->msgData = GFL_MsgSysLoadData(TRUE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, HEAPID_NAMEIN);
    work->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_NAMEIN);
    work->graphic = NameinGraphic_Create(0, HEAPID_NAMEIN);
    work->bmpOam = BmpOam_Init(HEAPID_NAMEIN, NameinGraphic_GetClActUnit(work->graphic));
    NameEntry_LoadBgGraphics(&work->bgs, HEAPID_NAMEIN);
    unit = NameinGraphic_GetClActUnit(work->graphic);
    NameEntryObjs_Init(&work->objs, unit, HEAPID_NAMEIN);
    species = param->gender;
    NameEntryIcon_Init(&work->icon, NameEntry_GetIconType(param->mode, species), species, param->unk6, unit,
                       work->font, work->queue, work->msgData, work->bmpOam, HEAPID_NAMEIN);
    work->keyboards = NameinData_LoadKeyboards(HEAPID_NAMEIN);
    work->name = GFL_StrBufClone(param->name, HEAPID_NAMEIN);
    if (GFL_StrBufGetCharCount(work->name) != 0) {
        if (GFL_StrBufGetCharCount(work->name) > param->maxLength
            || !NameinData_IsNameValid(GFL_StrBufGetStringPtr(work->name), HEAPID_NAMEIN)) {
            GFL_StrBufClear(work->name);
        }
    }
    NameEntryTask_Init(&work->task, work, NameEntryTask_WaitDraw);
    NameBox_Init(&work->box, work->name, param->maxLength, work->font, work->queue, &work->objs, HEAPID_NAMEIN);
    Keyboard_Init(&work->keyboard, keyboardType, work->font, work->queue, work->keyboards, &work->objs,
                  HEAPID_NAMEIN);
    if (param->mode == NAME_ENTRY_GAME_SYNC_ID) {
        Keyboard_SetLocked(&work->keyboard, TRUE);
    }
    NameEntryMessage_Init(&work->message, work->font, work->msgData, work->queue, work->wordSet, HEAPID_NAMEIN);
    NameEntryTalk_Init(&work->talk, HEAPID_NAMEIN);
    work->profWord = ProfWord_LoadLists(HEAPID_NAMEIN);
    NameEntryObjs_ShowSelectors(&work->objs, work->usesPresetNames);
    if (param->mode == NAME_ENTRY_POKEMON) {
        if (param->pkm == NULL) {
            NameEntryMessage_PrintSpecies(&work->message, param->mode, param->gender, param->unk6, HEAPID_NAMEIN);
        } else {
            NameEntryMessage_PrintPokemon(&work->message, param->mode, param->pkm);
        }
    } else {
        NameEntryMessage_Print(&work->message, param->mode);
    }
    if (func_02042788()) {
        func_02042ba8(TRUE, HEAPID_NAMEIN);
    }
    if (work->param->saveTask != NULL) {
        SaveControlIntr_Resume(work->param->saveTask);
    }
    return TRUE;
}

BOOL NameEntry_ProcExit(GameProc *proc, u32 *state, void *param, void *arg) {
    NameEntryWork *work = arg;

    ProfWord_FreeLists(work->profWord);
    NameEntryTalk_Free(&work->talk);
    NameEntryMessage_Free(&work->message);
    Keyboard_Free(&work->keyboard);
    GFL_StrBufFree(work->name);
    NameinData_FreeKeyboards(work->keyboards);
    NameBox_Free(&work->box);
    NameEntryTask_Clear(&work->task);
    NameEntryIcon_Free(&work->icon);
    NameEntryObjs_Free(&work->objs);
    NameEntry_FreeBgGraphics(&work->bgs);
    BmpOam_Exit(work->bmpOam);
    NameinGraphic_Delete(work->graphic);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_MsgDataFree(work->msgData);
    func_02021c44(work->queue);
    func_02021a18(work->queue);
    GFL_FontFree(work->font);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_NAMEIN);
    GFL_OvlUnload(OVERLAY_PROF_WORD);
    return TRUE;
}

BOOL NameEntry_ProcMain(GameProc *proc, u32 *state, void *param, void *arg) {
    NameEntryWork *work = arg;

    NameEntryTask_Run(&work->task);
    NameinGraphic_Main(work->graphic);
    func_02021a3c(work->queue);
    NameEntryIcon_UpdateLabel(&work->icon);
    if (NameEntryTask_IsDone(&work->task)) {
        return TRUE;
    }
    return FALSE;
}

// Nothing is read from the first argument of the BG graphics' functions
static void NameEntry_LoadBgGraphics(u32 *bgs, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_NAMEIN_GRAPHIC, heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 2, 4, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 5, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, 5, 0, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 2, 0, 0, 0, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 2, 0, 0x40, 0x60, 0x20, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 2, 0, 0x40, 0x80, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 0, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0, 0, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 2, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 4, 0x1e0, 0x20, heapId);
    GFL_ArcToolFree(arc);

    GFL_BGSysFillChar(4, 0, 1, 0);
    LoadSysMsgBox(4, 1, 14, 0, heapId);
}

static void NameEntry_FreeBgGraphics(u32 *bgs) {
    GFL_BGSysFreeFilledChar(4, 1, 0);
}

static void NameEntryObjs_Init(NameEntryObjs *objs, ClActUnit *unit, HeapID heapId) {
    ArcTool *arc;
    ClActorSetup setup;
    int i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_NAMEIN_GRAPHIC, heapId);
    objs->palette = func_0204bba0(arc, 9, 0, 0, heapId);
    objs->cellAnims = func_0204bde0(arc, 7, 6, heapId);
    objs->chars = func_0204b81c(arc, 8, FALSE, 0, heapId);
    GFL_ArcToolFree(arc);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.sequence = 0;
    for (i = 0; i < NAME_SLOT_COUNT; i++) {
        objs->actors[i] = func_0204c040(unit, objs->chars, objs->palette, objs->cellAnims, &setup, 0, heapId);
    }
    setup.sequence = 1;
    objs->actors[17] = func_0204c040(unit, objs->chars, objs->palette, objs->cellAnims, &setup, 0, heapId);
    func_0204c124(objs->actors[17], FALSE);

    for (i = 18; i < 20; i++) {
        setup.x = -32;
        setup.y = -32;
        setup.sequence = i - 15;
        objs->actors[i] = func_0204c040(unit, objs->chars, objs->palette, objs->cellAnims, &setup, 0, heapId);
        func_0204c520(objs->actors[i], TRUE);
        func_0204c124(objs->actors[i], FALSE);
    }

    for (i = 21; i <= 23; i++) {
        setup.x = 200 + (i - 21) * 16;
        setup.x += i - 21;
        setup.y = 20;
        setup.sequence = i - 15;
        objs->actors[i] = func_0204c040(unit, objs->chars, objs->palette, objs->cellAnims, &setup, 0, heapId);
        func_0204c124(objs->actors[i], FALSE);
    }
}

static void NameEntryObjs_Free(NameEntryObjs *objs) {
    int i;

    for (i = 0; i < OBJ_COUNT; i++) {
        if (objs->actors[i] != NULL) {
            func_0204c108(objs->actors[i]);
        }
    }
    func_0204bcd0(objs->palette);
    func_0204b98c(objs->chars);
    func_0204be64(objs->cellAnims);
}

static ClActor *NameEntryObjs_Get(NameEntryObjs *objs, u32 index) {
    return objs->actors[index];
}

static void KeyHighlight_Init(KeyHighlight *highlight, u32 type, NameEntryObjs *objs, HeapID heapId) {
    NNSG2dPaletteData *palette;
    u16 *colors;
    void *file;
    int i;
    int j;

    sys_memset(highlight, 0, sizeof(KeyHighlight));
    highlight->type = type;
    for (i = 0; i < 2; i++) {
        highlight->actors[i] = NameEntryObjs_Get(objs, 18 + i);
    }
    file = GFL_G2DIOReadNCLR(ARCID_NAMEIN_GRAPHIC, 2, &palette, heapId);
    colors = palette->rawData;
    for (i = 0; i < 16; i++) {
        highlight->baseColor[i] = colors[16 + i];
    }
    for (j = 0; j < 16; j++) {
        highlight->altColor[j] = colors[32 + j];
    }
    GFL_HeapFree(file);
}

static void KeyHighlight_Clear(KeyHighlight *highlight) {
    sys_memset(highlight, 0, sizeof(KeyHighlight));
}

static void KeyHighlight_Set(KeyHighlight *highlight, u32 mode, const KeyRect *rect, u32 kanaPage, u32 type) {
    int palette;
    s16 y;

    highlight->active = FALSE;
    highlight->mode = mode;
    highlight->kanaPage = kanaPage;
    highlight->type = type;
    GFL_BGSysSetScrPaletteNo(2, 0, 0, 32, 24, 1);
    GFL_BGSysSetScrPaletteNo(1, 0, 0, 32, 24, 1);
    if (kanaPage && type == KEYBOARD_TYPE_KANA) {
        GFL_BGSysSetScrPaletteNo(1, 3, 16, 4, 3, 4);
        GFL_BGSysSetScrPaletteNo(1, 23, 16, 4, 3, 4);
    }
    palette = (type + 1) * 3;
    GFL_BGSysSetScrPaletteNo(1, palette, 20, 3, 3, 4);
    GFL_BGSysSetScrPaletteNo(2, palette, 20, 3, 3, 4);
    if (rect != NULL && mode != 0) {
        highlight->rect = *rect;
        if (mode == 2) {
            GFL_BGSysSetScrPaletteNo(2, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
                                     mode + 1);
            GFL_BGSysSetScrPaletteNo(1, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
                                     mode + 1);
        }
        if (!func_0203d554()) {
            highlight->pos[0].x = rect->left * 8 + 7;
            y = rect->top * 8 + (rect->bottom - rect->top) * 4 - 3;
            highlight->pos[0].y = y;
            highlight->pos[1].x = rect->right * 8 - 7;
            highlight->pos[1].y = y;
            func_0204c56c(highlight->actors[0]);
            func_0204c56c(highlight->actors[1]);
            func_0204c124(highlight->actors[0], TRUE);
            func_0204c124(highlight->actors[1], TRUE);
        }
    }
    if (mode == 0 || func_0203d554() == TRUE) {
        func_0204c124(highlight->actors[0], FALSE);
        func_0204c124(highlight->actors[1], FALSE);
    }
    GFL_BGSysLoadScr(2);
    GFL_BGSysLoadScr(1);
    if (mode != 0) {
        sys_memset(highlight->phase, 0, sizeof(highlight->phase));
        highlight->timer = 0;
        highlight->active = TRUE;
    }
}

static void KeyHighlight_Update(KeyHighlight *highlight, u32 type, const KeyRect *rect) {
    ClActorPos pos;
    u32 palette;
    u16 step;
    int i;

    if (highlight->active) {
        switch (highlight->mode) {
        case 0:
            break;
        case 1:
            palette = 2;
            step = sKeyBlinkStep;
            break;
        case 2:
            palette = 3;
            step = 0x1999;
            if (highlight->timer++ > 10) {
                if (!func_0203d554()) {
                    KeyHighlight_Set(highlight, 1, rect, highlight->kanaPage, type);
                } else {
                    KeyHighlight_Set(highlight, 0, rect, highlight->kanaPage, type);
                }
            }
            break;
        }
        for (i = 0; i < 16; i++) {
            KeyHighlight_BlendColor(&highlight->color[i], &highlight->phase[i], step, palette, i, highlight->baseColor[i],
                                 highlight->altColor[i]);
        }
    }

    pos = highlight->pos[0];
    pos.y -= GFL_BGSysGetBGOffsetY(2) * 8;
    func_0204c140(highlight->actors[0], &pos, 0);
    pos = highlight->pos[1];
    pos.y -= GFL_BGSysGetBGOffsetY(2) * 8;
    func_0204c140(highlight->actors[1], &pos, 0);
    if (highlight->type != type) {
        if (!func_0203d554()) {
            KeyHighlight_Set(highlight, 1, &highlight->rect, highlight->kanaPage, type);
        } else {
            KeyHighlight_Set(highlight, 0, &highlight->rect, highlight->kanaPage, type);
        }
    }
}

// Moves the colour towards the other of the two along a cosine, uploads it to the palette and returns whether it got there
static BOOL KeyHighlight_BlendColor(u16 *color, u16 *phase, u16 step, u32 palette, u8 index, u16 from, u16 to) {
    int next = *phase + step;
    u8 fromR;
    u8 fromG;
    u8 fromB;
    u8 toR;
    u8 toG;
    u8 toB;
    u8 r;
    u8 g;
    u8 b;
    fx16 t;

    if (next >= 0x10000) {
        *phase = next - 0x10000;
    } else {
        *phase = next;
    }
    fromG = (from & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    t = (FX_CosIdx(*phase) + FX32_ONE) / 2;
    fromR = from & GX_RGB_R_MASK;
    fromB = (from & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    toB = (to & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    b = fromB + ((toB - fromB) * t >> 12);
    toR = to & GX_RGB_R_MASK;
    r = fromR + ((toR - fromR) * t >> 12);
    toG = (to & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    g = fromG + ((toG - fromG) * t >> 12);
    *color = GX_RGB(r, g, b);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, palette * 32 + index * 2, color, 2);
    if (*color == to) {
        return TRUE;
    }
    return FALSE;
}

static void NameBox_Init(NameBox *box, StrBuf *name, u8 maxLength, Font *font, PrintQueue *queue,
                         NameEntryObjs *objs, HeapID heapId) {
    BmpWin *window;
    ClActorPos pos;
    int offset;
    int i;

    sys_memset(box, 0, sizeof(NameBox));
    box->font = font;
    box->maxLength = maxLength;
    box->queue = queue;
    if (maxLength > 8) {
        box->centered = FALSE;
    } else {
        box->centered = TRUE;
    }
    window = BmpWin_CreateDynamic(0, 8, 2, 24, 2, 15, TRUE);
    box->window = window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    box->printWindow = box->window;
    box->printing = FALSE;
    box->buf = GFL_StrBufCreate(14, heapId);
    for (i = 0; i < 3; i++) {
        box->words[i] = NameinData_LoadWordTable(i, heapId);
    }
    box->conv = NameinData_LoadConvTable(heapId);
    for (i = 0; i < NAME_SLOT_COUNT; i++) {
        box->slots[i] = NameEntryObjs_Get(objs, i);
    }

    if (GFL_StrBufGetCharCount(name) == 0) {
        box->text[0] = GFL_StrBufGetTerminator();
    } else {
        GFL_StrBufStoreString(name, box->text, maxLength + 1);
        box->length = GFL_StrBufGetCharCount(name);
    }

    pos.y = 37;
    if (box->centered) {
        offset = 64 - box->maxLength * 14 / 2;
        if (offset < 0) {
            offset = 0;
        }
    } else {
        offset = 0;
    }
    offset += 64;
    for (i = 0; i < NAME_SLOT_COUNT; i++) {
        pos.x = offset + i * 14;
        func_0204c488(box->slots[i], 1);
        func_0204c520(box->slots[i], TRUE);
        func_0204c140(box->slots[i], &pos, 0);
        func_0204c124(box->slots[i], TRUE);
    }
    for (i = box->maxLength; i < NAME_SLOT_COUNT; i++) {
        func_0204c124(box->slots[i], FALSE);
    }
    func_0204c540(box->slots[box->length]);
    box->dirty = TRUE;
    NameBox_Update(box);
}

static void NameBox_Free(NameBox *box) {
    int i;

    NameinData_FreeConvTable(box->conv);
    for (i = 0; i < 3; i++) {
        NameinData_FreeWordTable(box->words[i]);
    }
    GFL_StrBufFree(box->buf);
    BmpWin_Free(box->window);
    sys_memset(box, 0, sizeof(NameBox));
}

static BOOL NameBox_Update(NameBox *box) {
    u16 chars[14];
    u16 ch[2];
    PrintQueue *queue;
    int x;
    int i;

    if (box->dirty) {
        if (box->length != 0) {
            wcharsncpy(box->text, chars, box->length + 1);
        }
        if (box->pendingLength != 0) {
            wcharsncpy(box->pending, &chars[box->length], box->pendingLength + 1);
        }
        if (box->length + box->pendingLength > 0) {
            GFL_BitmapFill(BmpWin_GetBitmap(box->window), 0);
            for (i = 0; i < box->length + box->pendingLength; i++) {
                ch[0] = chars[i];
                ch[1] = GFL_StrBufGetTerminator();
                GFL_StrBufLoadFixedString(box->buf, ch, 2);
                if (box->centered) {
                    x = 64 - box->maxLength * 14 / 2;
                    if (x < 0) {
                        x = 0;
                    }
                    x += i * 14 + 7;
                    x -= GFL_FontGetBlockWidth(box->buf, box->font, 0) / 2;
                } else {
                    x = 0;
                    x += i * 14;
                    x += 7 - GFL_FontGetBlockWidth(box->buf, box->font, 0) / 2;
                }
                if (i >= box->length) {
                    queue = box->queue;
                    func_02021c7c(queue, BmpWin_GetBitmap(box->printWindow), x, 0, box->buf, box->font, 0xc80);
                } else {
                    queue = box->queue;
                    func_02021c54(queue, BmpWin_GetBitmap(box->printWindow), x, 0, box->buf, box->font);
                }
                box->printing = TRUE;
            }
        } else {
            GFL_BitmapFill(BmpWin_GetBitmap(box->window), 0);
            BmpWin_FlushChar(box->window);
        }

        for (i = 0; i < box->maxLength; i++) {
            func_0204c4d4(box->slots[i], 0);
            func_0204c488(box->slots[i], 1);
            func_0204c124(box->slots[i], TRUE);
        }
        for (; i < NAME_SLOT_COUNT; i++) {
            func_0204c4d4(box->slots[i], 0);
            func_0204c488(box->slots[i], 1);
            func_0204c124(box->slots[i], FALSE);
        }
        for (i = 0; i <= box->length && i < box->maxLength; i++) {
            func_0204c124(box->slots[i], TRUE);
        }
        func_0204c488(box->slots[box->length], 0);
        box->dirty = FALSE;
    }
    return NameBox_IsPrintDone(box);
}

static BOOL NameBox_DeleteChar(NameBox *box) {
    if (box->pendingLength != 0) {
        box->pendingLength--;
        box->pending[box->pendingLength] = GFL_StrBufGetTerminator();
        box->dirty = TRUE;
        return TRUE;
    }
    if (box->length != 0) {
        box->length--;
        box->text[box->length] = GFL_StrBufGetTerminator();
        box->dirty = TRUE;
        return TRUE;
    }
    return FALSE;
}

static BOOL NameBox_AddChar(NameBox *box, u16 c) {
    if (box->length < box->maxLength) {
        box->text[box->length] = c;
        box->length++;
        box->text[box->length] = GFL_StrBufGetTerminator();
        box->dirty = TRUE;
        return TRUE;
    }
    return FALSE;
}

// Adds a character to the ones that are being composed, which turn into others
static BOOL NameBox_AddComposeChar(NameBox *box, u16 c, int set) {
    if (box->length < box->maxLength && box->pendingLength < 3) {
        box->pending[box->pendingLength] = c;
        box->pendingLength++;
        box->pending[box->pendingLength] = GFL_StrBufGetTerminator();
        NameBox_ComposeAll(box, set);
        box->dirty = TRUE;
        return TRUE;
    }
    return FALSE;
}

// Turns the last character into its other spelling
static BOOL NameBox_Compose(NameBox *box, int which) {
    const NameinWordTable *table;
    u16 index;
    u16 added[4];
    u16 removed[4];
    u8 addCount;
    u8 removeCount;
    int i;

    switch (which) {
    case 0:
        table = box->words[0];
        break;
    case 1:
        table = box->words[1];
        break;
    default:
        return FALSE;
    }
    if (box->length != 0) {
        index = NameinWordTable_Find(table, &box->text[box->length - 1]);
        if (index != 0xffff) {
            if (NameinWordTable_GetForm1(table, index, added, 4, &addCount)) {
                NameinWordTable_GetForm0(table, index, removed, 4, &removeCount);
                for (i = 0; i < removeCount; i++) {
                    NameBox_DeleteChar(box);
                }
                for (i = 0; i < addCount; i++) {
                    NameBox_AddChar(box, added[i]);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL NameBox_CycleLast(NameBox *box) {
    u16 next;

    if (box->length != 0) {
        if (NameinConvTable_GetNext(box->conv, &box->text[box->length - 1], &next)) {
            NameBox_DeleteChar(box);
            NameBox_AddChar(box, next);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL NameBox_IsPrintDone(NameBox *box) {
    PrintQueue *queue = box->queue;

    if (box->printing) {
        if (!func_02021c1c(queue, BmpWin_GetBitmap(box->printWindow))) {
            BmpWin_FlushChar(box->printWindow);
            box->printing = FALSE;
        }
    }
    if (!box->printing) {
        return TRUE;
    }
    return FALSE;
}

static void NameBox_GetString(NameBox *box, StrBuf *dest) {
    GFL_StrBufLoadFixedString(dest, box->text, box->length + 1);
}

static void NameBox_ClearCompose(NameBox *box) {
    if (box->pendingLength != 0) {
        box->pendingLength = 0;
        box->pending[box->pendingLength] = GFL_StrBufGetTerminator();
        box->dirty = TRUE;
    }
}

static void NameBox_CommitCompose(NameBox *box) {
    int i;

    for (i = 0; i < box->pendingLength; i++) {
        NameBox_AddChar(box, box->pending[i]);
    }
    box->pendingLength = 0;
    box->pending[box->pendingLength] = GFL_StrBufGetTerminator();
    box->dirty = TRUE;
}

static void NameBox_Clear(NameBox *box) {
    if (box->length != 0) {
        box->length = 0;
        box->text[box->length] = GFL_StrBufGetTerminator();
        box->dirty = TRUE;
    }
    NameBox_ClearCompose(box);
}

static void NameBox_SetString(NameBox *box, const u16 *str) {
    u16 c = *str;

    while (c != GFL_StrBufGetTerminator()) {
        if (!NameBox_AddChar(box, c)) {
            break;
        }
        str++;
        c = *str;
    }
}

static BOOL NameBox_IsFull(NameBox *box) {
    return box->length == box->maxLength;
}

static u16 NameBox_GetLength(NameBox *box) {
    return box->length;
}

static u16 NameBox_GetComposeLength(NameBox *box) {
    return box->pendingLength;
}

// Looks the composed characters up in the third word table and, when a character has an entry, commits the ones before
// it and replaces it with the spelling that the entry gives for the set
static BOOL NameBox_ComposeAll(NameBox *box, int set) {
    BOOL (*lookup)(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
    u8 count;
    u16 chars[4];
    u16 index;
    int i;
    int j;

    if (set) {
        lookup = NameinWordTable_GetForm2;
    } else {
        lookup = NameinWordTable_GetForm1;
    }
    for (i = 0; i < box->pendingLength; i++) {
        index = NameinWordTable_Find(box->words[2], &box->pending[i]);
        if (index != 0xffff) {
            for (j = 0; j < i; j++) {
                NameBox_AddChar(box, box->pending[j]);
            }
            for (j = 0; j < i; j++) {
                NameBox_DeleteChar(box);
            }
            if (lookup(box->words[2], index, chars, 4, &count)) {
                for (i = 0; i < count; i++) {
                    NameBox_AddChar(box, chars[i]);
                }
                NameinWordTable_GetForm0(box->words[2], index, chars, 4, &count);
                for (i = 0; i < count; i++) {
                    NameBox_DeleteChar(box);
                }
                NameinWordTable_GetForm3(box->words[2], index, chars, 4, &count);
                for (i = 0; i < count; i++) {
                    box->pending[box->pendingLength] = chars[i];
                    box->pendingLength++;
                    box->pending[box->pendingLength] = GFL_StrBufGetTerminator();
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL NameBox_HasVisibleChar(NameBox *box) {
    int spaces;
    int i;

    if (box->length == 0) {
        return FALSE;
    }
    spaces = 0;
    for (i = 0; i < box->length; i++) {
        if (box->text[i] == ' ') {
            spaces++;
        }
    }
    return i != spaces;
}

static BOOL NameBox_IsValidGameSyncId(NameBox *box) {
    int i;

    if (box->length != 0 && box->length != 10) {
        return FALSE;
    }
    for (i = 0; i < box->length; i++) {
        if (!NameEntry_IsGameSyncIdChar(box->text[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL NameEntry_IsGameSyncIdChar(u16 c) {
    u32 i;

    c = GFL_StrCharToUpperCase(c);
    for (i = 0; i < GAME_SYNC_ID_CHAR_COUNT; i++) {
        if (c == sGameSyncIdChars[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static void KeyLayout_Init(KeyLayout *layout, u32 type, u32 index, const NameinKeyboardSet *keyboards, HeapID heapId) {
    int i;

    sys_memset(layout, 0, sizeof(KeyLayout));
    layout->keyboard = NameinData_GetKeyboard(keyboards, index);
    switch (type) {
    case KEYBOARD_TYPE_KANA:
        layout->width = 11;
        layout->height = 4;
        layout->getRect = KanaLayout_GetRect;
        layout->move = KanaLayout_Move;
        layout->getKind = KanaLayout_GetKind;
        break;
    case KEYBOARD_TYPE_0:
    case KEYBOARD_TYPE_1:
    case KEYBOARD_TYPE_2:
        layout->width = 13;
        layout->height = 6;
        layout->getRect = AlphaLayout_GetRect;
        layout->move = AlphaLayout_Move;
        layout->getKind = AlphaLayout_GetKind;
        break;
    }
    for (i = 0; i < 13; i++) {
        layout->memory[i] = sDefaultKeyMemory[i];
    }
}

static void KeyLayout_SetKeyboard(KeyLayout *layout, u32 index, const NameinKeyboardSet *keyboards) {
    layout->keyboard = NameinData_GetKeyboard(keyboards, index);
}

static void KeyLayout_Clear(KeyLayout *layout) {
    sys_memset(layout, 0, sizeof(KeyLayout));
}

// Returns what is at the position, and the character if it is one
static int KeyLayout_GetChar(KeyLayout *layout, const KeyPos *pos, u16 *c) {
    KeyPos cell;
    int kind = layout->getKind(pos, &cell);

    if (kind == KEY_KIND_CHAR || kind == KEY_KIND_DAKUTEN || kind == KEY_KIND_HANDAKUTEN) {
        if (c != NULL) {
            *c = NameinKeyboard_GetChar(layout->keyboard, cell.x, cell.y);
        }
    } else if (kind == KEY_KIND_SPACE) {
        if (c != NULL) {
            *c = ' ';
        }
    }
    return kind;
}

static BOOL KeyLayout_FindAtTouch(KeyLayout *layout, const TouchPos *touch, KeyPos *found) {
    KeyPos pos;
    KeyRect rect;

    pos.y = 0;
    while (pos.y < layout->height) {
        pos.x = 0;
        while (pos.x < layout->width) {
            if (KeyLayout_GetRect(layout, &pos, &rect)) {
                rect.left *= 8;
                rect.top *= 8;
                rect.right *= 8;
                rect.bottom *= 8;
                if (NameEntry_PointInRect(&rect, touch)) {
                    *found = pos;
                    return TRUE;
                }
            }
            pos.x++;
        }
        pos.y++;
    }
    return FALSE;
}

static void KeyLayout_Move(KeyLayout *layout, KeyPos *pos, const KeyPos *delta) {
    layout->move(pos, delta, layout->memory);
}

// Prints the characters of the keys into the window
static void KeyLayout_PrintKeys(KeyLayout *layout, KeyPrint *print, PrintQueue *queue, Font *font, u16 heapId) {
    StrBuf *buf = GFL_StrBufCreate(2, heapId);
    KeyPos pos;
    KeyRect rect;
    u16 c;
    u16 str[2];
    u16 x;
    u16 y;
    int kind;

    for (pos.y = 0; pos.y < NameinKeyboard_GetHeight(layout->keyboard); pos.y++) {
        for (pos.x = 0; pos.x < NameinKeyboard_GetWidth(layout->keyboard); pos.x++) {
            if (KeyLayout_GetRect(layout, &pos, &rect)) {
                kind = KeyLayout_GetChar(layout, &pos, &c);
                if (kind == KEY_KIND_CHAR || kind == KEY_KIND_DAKUTEN || kind == KEY_KIND_HANDAKUTEN) {
                    str[0] = c;
                    str[1] = GFL_StrBufGetTerminator();
                    GFL_StrBufLoadFixedString(buf, str, 2);
                    rect.left *= 8;
                    rect.top *= 8;
                    rect.right *= 8;
                    rect.bottom *= 8;
                    x = rect.left + (rect.right - rect.left) / 2 - 3 * 8;
                    y = rect.top + (rect.bottom - rect.top) / 2 - 5 * 8;
                    x -= GFL_FontGetBlockWidth(buf, font, 0) / 2;
                    y -= GFL_FontGetBlockHeight(buf, font) / 2;
                    func_02021c54(queue, BmpWin_GetBitmap(print->window), x - 1, y, buf, font);
                    print->printing = TRUE;
                }
            }
        }
    }
    GFL_StrBufFree(buf);
}

static BOOL KeyLayout_GetRect(KeyLayout *layout, const KeyPos *pos, KeyRect *rect) {
    return layout->getRect(pos, rect);
}

// Finds the position of a kind of key
static BOOL KeyLayout_FindKind(KeyLayout *layout, int kind, KeyPos *found) {
    KeyPos pos;

    for (pos.y = 0; pos.y <= layout->height; pos.y++) {
        for (pos.x = 0; pos.x <= layout->width; pos.x++) {
            if (kind == layout->getKind(&pos, NULL)) {
                *found = pos;
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Moves the position to the key of the same kind in the other layout
static void KeyLayout_ConvertPos(KeyPos *pos, u32 from, u32 to) {
    int (*oldKind)(const KeyPos *pos, KeyPos *cell);
    int (*newKind)(const KeyPos *pos, KeyPos *cell);
    int width;
    int height;
    int kind;
    KeyPos next;

    if (from == KEYBOARD_TYPE_KANA) {
        oldKind = KanaLayout_GetKind;
        newKind = AlphaLayout_GetKindDefault;
        width = 13;
        height = 5;
    } else if (to == KEYBOARD_TYPE_KANA) {
        oldKind = AlphaLayout_GetKindDefault;
        newKind = KanaLayout_GetKind;
        width = 11;
        height = 4;
    }
    kind = oldKind(pos, NULL);
    for (next.y = 0; next.y <= height; next.y++) {
        for (next.x = 0; next.x <= width; next.x++) {
            if (kind == newKind(&next, NULL)) {
                *pos = next;
                return;
            }
        }
    }
}

static BOOL AlphaLayout_GetRect(const KeyPos *pos, KeyRect *rect) {
    KeyPos cell;

    switch (AlphaLayout_GetKindDefault(pos, &cell)) {
    case KEY_KIND_CHAR:
    case KEY_KIND_DAKUTEN:
    case KEY_KIND_HANDAKUTEN:
        rect->left = cell.x * 2 + 3;
        rect->top = cell.y * 3 + 5;
        rect->right = rect->left + 2;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_TYPE_0:
    case KEY_KIND_TYPE_1:
    case KEY_KIND_TYPE_2:
    case KEY_KIND_TYPE_3:
        rect->left = (pos->x + 1) * 3;
        rect->top = pos->y * 3 + 5;
        rect->right = rect->left + 3;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_BACKSPACE:
        rect->left = 15;
        rect->top = pos->y * 3 + 5;
        rect->right = rect->left + 7;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_OK:
        rect->left = 22;
        rect->top = pos->y * 3 + 5;
        rect->right = rect->left + 7;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_NONE:
        return FALSE;
    }
    if (pos->y == 5 && pos->x > 6) {
        return FALSE;
    }
    return TRUE;
}

static BOOL KanaLayout_GetRect(const KeyPos *pos, KeyRect *rect) {
    KeyPos cell;
    int left;

    switch (KanaLayout_GetKind(pos, &cell)) {
    case KEY_KIND_CHAR:
        rect->left = sKanaRowLeft[cell.y] + cell.x * 2;
        rect->top = sKanaRowTop[cell.y];
        rect->right = rect->left + 2;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_TYPE_0:
    case KEY_KIND_TYPE_1:
    case KEY_KIND_TYPE_2:
    case KEY_KIND_TYPE_3:
        rect->left = (pos->x + 1) * 3;
        rect->top = 20;
        rect->right = rect->left + 3;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_BACKSPACE:
        rect->left = 25;
        rect->top = 8;
        rect->right = rect->left + 4;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_OK:
        rect->left = 24;
        rect->top = 12;
        rect->right = rect->left + 5;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_SWITCH_PAGE:
        if (pos->x == 0) {
            left = 3;
        } else {
            left = 23;
        }
        rect->left = left;
        rect->top = 16;
        rect->right = rect->left + 4;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_SPACE:
        rect->left = 15;
        rect->top = 20;
        rect->right = rect->left + 12;
        rect->bottom = rect->top + 3;
        break;
    case KEY_KIND_NONE:
        return FALSE;
    }
    switch (pos->y) {
    case 1:
        if (pos->x > 9) {
            return FALSE;
        }
        break;
    case 2:
        if (pos->x > 9) {
            return FALSE;
        }
        break;
    case 3:
        if (pos->x > 4) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static void AlphaLayout_Move(KeyPos *pos, const KeyPos *delta, u8 *memory) {
    BOOL toBottom = FALSE;
    BOOL fromBottom = FALSE;
    int x;
    int y;
    int slot;

    switch (pos->y) {
    case 0:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 12;
        } else if (x > 12) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 5;
        } else if (y > 5) {
            y = 0;
        }
        pos->y = y;
        if (y == 5) {
            toBottom = TRUE;
        }
        break;
    case 4:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 12;
        } else if (x > 12) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 5;
        } else if (y > 5) {
            y = 0;
        }
        pos->y = y;
        if (y == 5) {
            toBottom = TRUE;
        }
        break;
    case 5:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 5;
        } else if (x > 5) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 5;
        } else if (y > 5) {
            y = 0;
        }
        pos->y = y;
        if (y != 5) {
            fromBottom = TRUE;
        }
        break;
    default:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 12;
        } else if (x > 12) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 5;
        } else if (y > 5) {
            y = 0;
        }
        pos->y = y;
        break;
    }

    if (toBottom) {
        x = pos->x;
        if (x >= 6 && x <= 9) {
            memory[4] = x;
            pos->x = 4;
        } else if (x >= 10 && x <= 12) {
            memory[5] = x;
            pos->x = 5;
        } else if (x >= 0 && x <= 5) {
            slot = x * 2 / 3;
            memory[slot] = x;
            pos->x = slot;
        }
    }
    if (fromBottom) {
        if (pos->x == 4) {
            pos->x = memory[4];
        } else if (pos->x == 5) {
            pos->x = memory[5];
        } else if (pos->x >= 0 && pos->x <= 3) {
            pos->x = memory[pos->x];
        }
    }
}

static void KanaLayout_Move(KeyPos *pos, const KeyPos *delta, u8 *memory) {
    int clamped;
    int x;
    int y;

    switch (pos->y) {
    case 0:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 10;
        } else if (x > 10) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 3;
        } else if (y > 3) {
            y = 0;
        }
        pos->y = y;
        if (y == 3) {
            switch (pos->x) {
            case 0:
            case 1:
            case 2:
            case 3:
                break;
            case 8:
            case 9:
                memory[8] = pos->x;
                memory[12] = pos->x;
                memory[11] = 8;
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                memory[12] = pos->x;
                memory[11] = pos->x;
                break;
            case 10:
                memory[12] = 10;
                memory[11] = 9;
                break;
            }
            switch (pos->x) {
            case 0:
                pos->x = memory[6];
                return;
            case 1:
            case 2:
                pos->x += 1;
                return;
            default:
                pos->x = 4;
                return;
            }
        } else if (y == 1) {
            switch (pos->x) {
            case 0:
            case 1:
            case 2:
            case 3:
                break;
            case 8:
            case 9:
                memory[8] = pos->x;
                memory[12] = pos->x;
                clamped = pos->x;
                if (clamped > 9) {
                    clamped = 9;
                } else if (clamped < 4) {
                    clamped = 4;
                }
                memory[11] = clamped;
                break;
            case 4:
            case 5:
            case 6:
            case 7:
            case 10:
                memory[12] = pos->x;
                clamped = pos->x;
                if (clamped > 9) {
                    clamped = 9;
                } else if (clamped < 4) {
                    clamped = 4;
                }
                memory[11] = clamped;
                break;
            }
            switch (pos->x) {
            case 8:
            case 9:
                pos->x = 8;
                return;
            case 10:
                pos->x = 9;
                return;
            }
        }
        break;
    case 1:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 9;
        } else if (x > 9) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 3;
        } else if (y > 3) {
            y = 0;
        }
        pos->y = y;
        if (y == 0) {
            switch (pos->x) {
            case 7:
            case 8:
                memory[9] = pos->x;
                break;
            }
            switch (pos->x) {
            case 8:
                pos->x = memory[8];
                return;
            case 9:
                pos->x = 10;
                return;
            }
        } else if (y == 2) {
            switch (pos->x) {
            case 7:
            case 8:
                memory[9] = pos->x;
                break;
            }
            switch (pos->x) {
            case 0:
                pos->x = memory[7];
                return;
            case 7:
            case 8:
                pos->x = 8;
                return;
            case 9:
                pos->x = 9;
                return;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            default:
                pos->x += 1;
                return;
            }
        }
        break;
    case 2:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 9;
        } else if (x > 9) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 3;
        } else if (y > 3) {
            y = 0;
        }
        pos->y = y;
        if (y == 1) {
            switch (pos->x) {
            case 0:
            case 1:
                memory[7] = pos->x;
                break;
            case 2:
            case 3:
                break;
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                memory[11] = pos->x;
                memory[12] = pos->x;
                break;
            }
            switch (pos->x) {
            case 0:
            case 1:
                pos->x = 0;
                return;
            case 8:
                pos->x = memory[9];
                return;
            case 9:
                pos->x = 9;
                return;
            default:
                pos->x -= 1;
                return;
            }
        } else if (y == 3) {
            switch (pos->x) {
            case 0:
            case 1:
                memory[7] = pos->x;
                break;
            case 2:
            case 3:
                break;
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                memory[11] = pos->x;
                memory[12] = pos->x;
                break;
            case 9:
                memory[11] = 9;
                memory[12] = 10;
                break;
            }
            switch (pos->x) {
            case 0:
                pos->x = memory[10];
                return;
            case 1:
            case 2:
            case 3:
                break;
            default:
                pos->x = 4;
                return;
            }
        }
        break;
    case 3:
        x = pos->x + delta->x;
        pos->x = x;
        if (x < 0) {
            x = 4;
        } else if (x > 4) {
            x = 0;
        }
        pos->x = x;
        y = pos->y + delta->y;
        pos->y = y;
        if (y < 0) {
            y = 3;
        } else if (y > 3) {
            y = 0;
        }
        pos->y = y;
        if (y == 2) {
            switch (pos->x) {
            case 0:
            case 1:
                memory[10] = pos->x;
                memory[6] = pos->x;
                break;
            }
            switch (pos->x) {
            case 0:
            case 1:
            case 2:
            case 3:
                break;
            case 4:
                pos->x = memory[11];
                return;
            }
        } else if (y == 0) {
            switch (pos->x) {
            case 0:
            case 1:
                memory[10] = pos->x;
                memory[6] = pos->x;
                break;
            }
            switch (pos->x) {
            case 0:
            case 1:
                pos->x = 0;
                return;
            case 2:
            case 3:
                pos->x -= 1;
                return;
            case 4:
                pos->x = memory[12];
                return;
            }
        }
        break;
    }
}

static int AlphaLayout_GetKindDefault(const KeyPos *pos, KeyPos *cell) {
    return AlphaLayout_GetKind(pos, cell);
}

static int AlphaLayout_GetKind(const KeyPos *pos, KeyPos *cell) {
    if (pos->y < 5) {
        if (cell != NULL) {
            cell->y = pos->y;
            cell->x = pos->x;
        }
        return KEY_KIND_CHAR;
    }
    if (pos->x <= 5) {
        return pos->x + 2;
    }
    return KEY_KIND_NONE;
}

static int KanaLayout_GetKind(const KeyPos *pos, KeyPos *cell) {
    int kind = KEY_KIND_NONE;
    KeyPos found;

    switch (pos->y) {
    case 0:
        if (pos->x == 10) {
            kind = KEY_KIND_BACKSPACE;
        } else {
            found = *pos;
            kind = KEY_KIND_CHAR;
        }
        break;
    case 1:
        if (pos->x == 9) {
            kind = KEY_KIND_OK;
        } else {
            found = *pos;
            kind = KEY_KIND_CHAR;
        }
        break;
    case 2:
        if (pos->x == 0 || pos->x == 9) {
            kind = KEY_KIND_SWITCH_PAGE;
        } else {
            found.x = pos->x - 1;
            found.y = pos->y;
            kind = KEY_KIND_CHAR;
        }
        break;
    case 3:
        kind = KEY_KIND_SPACE;
        if (pos->x != 4) {
            kind = pos->x + 2;
        }
        break;
    }
    if (kind == KEY_KIND_CHAR && cell != NULL) {
        *cell = found;
    }
    return kind;
}

static void Keyboard_Init(Keyboard *kb, u32 type, Font *font, PrintQueue *queue, NameinKeyboardSet *keyboards,
                          NameEntryObjs *objs, HeapID heapId) {
    BmpWin *window;
    KeyRect rect;

    sys_memset(kb, 0, sizeof(Keyboard));
    kb->pos.x = 0;
    kb->pos.y = 0;
    kb->font = font;
    kb->queue = queue;
    kb->heapId = heapId;
    kb->type = type;
    kb->nextType = type;
    kb->keyboards = keyboards;
    kb->screenData[0] = GFL_G2DIOReadNSCR(ARCID_NAMEIN_GRAPHIC, 10, FALSE, &kb->screens[0], heapId);
    kb->screenData[1] = GFL_G2DIOReadNSCR(ARCID_NAMEIN_GRAPHIC, 5, FALSE, &kb->screens[1], heapId);
    if (type == KEYBOARD_TYPE_KANA) {
        GFL_BGSysBufferScrDefault(1, kb->screens[0]->rawData, 0x800);
        GFL_BGSysSetBGEnabled(2, FALSE);
    } else {
        GFL_BGSysBufferScrDefault(1, kb->screens[1]->rawData, 0x800);
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    GFL_BGSysLoadScr(1);
    KeyHighlight_Init(&kb->highlight, type, objs, heapId);
    KeyLayout_Init(&kb->layout, type, NameEntry_GetLayoutIndex(kb->type, kb->kanaPage, kb->kanaGroup),
                   kb->keyboards, heapId);
    window = BmpWin_CreateDynamic(3, 3, 5, 26, 15, 15, TRUE);
    kb->window = window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    kb->print.window = kb->window;
    kb->print.printing = FALSE;
    KeyLayout_PrintKeys(&kb->layout, &kb->print, queue, font, heapId);
    if (!func_0203d554()) {
        if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
            KeyHighlight_Set(&kb->highlight, 1, &rect, kb->kanaPage, kb->type);
        }
    } else {
        if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
            KeyHighlight_Set(&kb->highlight, 0, &rect, kb->kanaPage, kb->type);
        }
    }
}

static void Keyboard_Free(Keyboard *kb) {
    BmpWin_Free(kb->window);
    KeyLayout_Clear(&kb->layout);
    KeyHighlight_Clear(&kb->highlight);
    GFL_HeapFree(kb->screenData[0]);
    GFL_HeapFree(kb->screenData[1]);
    sys_memset(kb, 0, sizeof(Keyboard));
}

static void Keyboard_Update(Keyboard *kb, NameBox *box) {
    kb->result = KEY_RESULT_NONE;
    switch (kb->state) {
    case 0: {
        KeyInput input;
        BOOL acted;

        sys_memset(&input, 0, sizeof(KeyInput));
        acted = Keyboard_HandleTouch(kb, &input);
        if (!acted) {
            Keyboard_HandleKeys(kb, &input);
            acted = Keyboard_HandleButtons(kb, &input);
        }
        if (acted) {
            Keyboard_Select(kb, &input);
        }
        break;
    }
    case 2: {
        KeyInput input;
        BOOL acted;

        sys_memset(&input, 0, sizeof(KeyInput));
        acted = Keyboard_HandleTouchSlide(kb, &input);
        if (!acted) {
            Keyboard_HandleKeysSlide(kb, &input);
            acted = Keyboard_HandleButtonsSlide(kb, &input);
        }
        if (acted) {
            Keyboard_SelectSlide(kb, &input);
        }
        if (Keyboard_UpdateSlide(kb)) {
            kb->state = 0;
        }
        break;
    }
    case 1:
        if (NameBox_IsFull(box)) {
            KeyLayout_FindKind(&kb->layout, KEY_KIND_OK, &kb->pos);
            kb->result = KEY_RESULT_COMMIT_COMPOSE;
        }
        kb->state = 0;
        break;
    }
    {
        KeyRect rect;

        KeyLayout_GetRect(&kb->layout, &kb->pos, &rect);
        KeyHighlight_Update(&kb->highlight, kb->type, &rect);
    }
    Keyboard_IsPrintDone(kb);
}

static int Keyboard_GetResult(Keyboard *kb, u16 *c) {
    if (kb->result == KEY_RESULT_CHAR || kb->result == KEY_RESULT_COMPOSE_CHAR || kb->result == KEY_RESULT_SPACE) {
        if (c != NULL) {
            *c = kb->lastChar;
        }
    }
    return kb->result;
}

static u32 Keyboard_GetKanaPage(Keyboard *kb) {
    return kb->kanaPage;
}

static BOOL Keyboard_IsPrintDone(Keyboard *kb) {
    PrintQueue *queue = kb->queue;

    if (kb->print.printing) {
        if (!func_02021c1c(queue, BmpWin_GetBitmap(kb->print.window))) {
            BmpWin_FlushChar(kb->print.window);
            kb->print.printing = FALSE;
        }
    }
    if (!kb->print.printing) {
        return TRUE;
    }
    return FALSE;
}

static u32 Keyboard_GetType(Keyboard *kb) {
    return kb->type;
}

static void Keyboard_SetLocked(Keyboard *kb, BOOL locked) {
    kb->locked = locked;
}

static u32 NameEntry_GetLayoutIndex(u32 type, u32 page, u8 group) {
    if (type == KEYBOARD_TYPE_KANA) {
        return type + group * 2 + page;
    }
    return type;
}

// Starts changing to another keyboard; the old one slides off, and the new one in
static BOOL Keyboard_StartSwitch(Keyboard *kb, u32 type) {
    BOOL hasUpperLayer;

    if (kb->nextType != type) {
        kb->nextType = type;
        hasUpperLayer = FALSE;
        kb->slideFrame = 0;
        kb->slideStage = 0;
        kb->sliding = TRUE;
        if (type == KEYBOARD_TYPE_KANA || kb->type == KEYBOARD_TYPE_KANA) {
            hasUpperLayer = TRUE;
        }
        kb->hasUpperLayer = hasUpperLayer;
        if (hasUpperLayer) {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR,
                                GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3,
                                GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2
                                    | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD,
                                16, 16);
        } else {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG3,
                                GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2
                                    | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD,
                                16, 16);
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL Keyboard_UpdateSlide(Keyboard *kb) {
    int frame;
    s16 alpha;
    s16 offset;

    if (kb->sliding) {
        switch (kb->slideStage) {
        case 0:
            GFL_BGSysGetBGOffsetY(1);
            frame = kb->slideFrame;
            alpha = -(frame * 16) / 7 + 16;
            reg_G2_BLDALPHA = alpha | (1 << 12);
            offset = -48 * frame / 7;
            GFL_BGSysMoveBG(1, 3, offset);
            GFL_BGSysMoveBG(3, 3, offset);
            if (kb->hasUpperLayer) {
                GFL_BGSysMoveBG(2, 3, offset);
            }
            if (kb->slideFrame++ > 7) {
                if (kb->hasUpperLayer) {
                    GFL_BGSysSetBGEnabled(1, FALSE);
                    GFL_BGSysSetBGEnabled(2, FALSE);
                } else {
                    GFL_BGSysSetBGEnabled(1, FALSE);
                }
                GFL_BGSysSetBGEnabled(3, FALSE);
                kb->slideFrame = 0;
                kb->slideStage = 1;
            }
            break;
        case 1:
            offset = -48;
            GFL_BGSysMoveBG(1, 3, offset);
            GFL_BGSysMoveBG(3, 3, offset);
            if (kb->hasUpperLayer) {
                GFL_BGSysMoveBG(2, 3, offset);
            }
            Keyboard_ApplyType(kb, kb->nextType);
            kb->slideStage = 2;
            break;
        case 2:
            kb->slideFrame = 7;
            kb->slideStage = 3;
            GFL_BGSysSetBGEnabled(1, TRUE);
            GFL_BGSysSetBGEnabled(3, TRUE);
            if (kb->type != KEYBOARD_TYPE_KANA) {
                GFL_BGSysSetBGEnabled(2, TRUE);
            }
            reg_G2_BLDALPHA = 1 << 12;
            break;
        case 3:
            frame = kb->slideFrame;
            alpha = -(frame * 16) / 7 + 16;
            reg_G2_BLDALPHA = alpha | (1 << 12);
            offset = -48 * frame / 7;
            GFL_BGSysMoveBG(1, 3, offset);
            GFL_BGSysMoveBG(3, 3, offset);
            if (kb->hasUpperLayer) {
                GFL_BGSysMoveBG(2, 3, offset);
            }
            if (kb->slideFrame-- == 0) {
                kb->slideStage = 4;
            }
            break;
        case 4:
            GFL_BGSysMoveBG(1, 3, 0);
            GFL_BGSysMoveBG(3, 3, 0);
            GFL_BGSysMoveBG(2, 3, 0);
            kb->hasUpperLayer = FALSE;
            reg_G2_BLDCNT = 0;
            kb->sliding = FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

static void Keyboard_ApplyType(Keyboard *kb, u32 type) {
    if (kb->type != type) {
        KeyLayout_Clear(&kb->layout);
        KeyLayout_Init(&kb->layout, type, NameEntry_GetLayoutIndex(type, kb->kanaPage, kb->kanaGroup),
                       kb->keyboards, kb->heapId);
        GFL_BitmapFill(BmpWin_GetBitmap(kb->window), 0);
        KeyLayout_PrintKeys(&kb->layout, &kb->print, kb->queue, kb->font, kb->heapId);
        if (type == KEYBOARD_TYPE_KANA) {
            GFL_BGSysBufferScrDefault(1, kb->screens[0]->rawData, 0x800);
            GFL_BGSysSetBGEnabled(2, FALSE);
        } else {
            GFL_BGSysBufferScrDefault(1, kb->screens[1]->rawData, 0x800);
        }
        GFL_BGSysLoadScr(1);
        if (type == KEYBOARD_TYPE_KANA || kb->type == KEYBOARD_TYPE_KANA) {
            KeyLayout_ConvertPos(&kb->pos, kb->type, type);
        }
        kb->type = type;
    }
}

static void Keyboard_SetKanaPage(Keyboard *kb, u32 page, u32 group) {
    if (kb->type == KEYBOARD_TYPE_KANA) {
        if (kb->kanaGroup == group && kb->kanaPage == page) {
            return;
        }
        KeyLayout_SetKeyboard(&kb->layout, NameEntry_GetLayoutIndex(kb->type, page, group), kb->keyboards);
        GFL_BitmapFill(BmpWin_GetBitmap(kb->window), 0);
        KeyLayout_PrintKeys(&kb->layout, &kb->print, kb->queue, kb->font, kb->heapId);
        kb->kanaPage = page;
        kb->kanaGroup = group;
    }
}

static BOOL Keyboard_HandleKeys(Keyboard *kb, KeyInput *input) {
    KeyPos delta;
    KeyRect rect;
    BOOL moved = FALSE;

    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        delta.x = 0;
        delta.y = -1;
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        delta.x = 0;
        delta.y = 1;
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
        delta.x = -1;
        delta.y = 0;
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
        delta.x = 1;
        delta.y = 0;
        moved = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        delta.x = 0;
        delta.y = 0;
        moved = TRUE;
        KeyLayout_FindKind(&kb->layout, KEY_KIND_OK, &kb->pos);
        KeyLayout_Move(&kb->layout, &kb->pos, &delta);
    }
    if (moved) {
        if (func_0203d554() == TRUE) {
            if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
                func_0203d564(FALSE);
                KeyHighlight_Set(&kb->highlight, 1, &rect, kb->kanaPage, kb->type);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        } else {
            KeyLayout_Move(&kb->layout, &kb->pos, &delta);
            if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                KeyHighlight_Set(&kb->highlight, 1, &rect, kb->kanaPage, kb->type);
            }
        }
    }
    return FALSE;
}

static BOOL Keyboard_HandleButtons(Keyboard *kb, KeyInput *input) {
    KeyRect rect;
    BOOL acted = FALSE;

    if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
        input->pos = kb->pos;
        acted = TRUE;
        input->moved = TRUE;
    }
    if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_B) {
        KeyLayout_FindKind(&kb->layout, KEY_KIND_BACKSPACE, &input->pos);
        input->button = KEY_KIND_BACKSPACE;
        acted = TRUE;
    }
    if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_SELECT) {
        input->button = kb->type + 3;
        if (input->button > KEY_KIND_TYPE_3) {
            input->button = KEY_KIND_TYPE_0;
        }
        KeyLayout_FindKind(&kb->layout, input->button, &kb->pos);
        input->pos = kb->pos;
        acted = TRUE;
    }
    if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_R) {
        kb->result = KEY_RESULT_CYCLE;
        acted = FALSE;
    }
    if (acted && func_0203d554() == TRUE) {
        if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0203d564(FALSE);
            KeyHighlight_Set(&kb->highlight, 1, &rect, kb->kanaPage, kb->type);
            return FALSE;
        }
    }
    return acted;
}

static BOOL Keyboard_HandleTouch(Keyboard *kb, KeyInput *input) {
    TouchPos touch;
    u32 touchX;
    u32 touchY;
    int x;
    int y;

    if (func_0203dac8(&touchX, &touchY)) {
        touch.x = touchX;
        touch.y = touchY;
        if (KeyLayout_FindAtTouch(&kb->layout, &touch, &kb->pos)) {
            x = kb->pos.x;
            y = kb->pos.y;
            input->pos.y = y;
            input->moved = TRUE;
            input->pos.x = x;
            func_0203d564(TRUE);
            return TRUE;
        }
    }
    return FALSE;
}

static void Keyboard_Select(Keyboard *kb, KeyInput *input) {
    KeyRect rect;
    int kind;

    if (input->moved) {
        kind = KeyLayout_GetChar(&kb->layout, &kb->pos, &kb->lastChar);
    } else {
        kind = input->button;
    }
    switch (kind) {
    case KEY_KIND_CHAR:
        kb->result = KEY_RESULT_CHAR;
        kb->state = 1;
        break;
    case KEY_KIND_SPACE:
        kb->result = KEY_RESULT_SPACE;
        kb->state = 1;
        break;
    case KEY_KIND_TYPE_0:
    case KEY_KIND_TYPE_1:
    case KEY_KIND_TYPE_2:
    case KEY_KIND_TYPE_3:
        if (kb->locked) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            break;
        }
        if (Keyboard_StartSwitch(kb, kind - 2)) {
            GFL_SndSEPlay(SEQ_SE_SYS_12);
            kb->state = 2;
            kb->result = KEY_RESULT_CLEAR_COMPOSE;
        }
        break;
    case KEY_KIND_BACKSPACE:
        kb->result = KEY_RESULT_BACKSPACE;
        break;
    case KEY_KIND_OK:
        kb->result = KEY_RESULT_OK;
        break;
    case KEY_KIND_SWITCH_PAGE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        kb->result = KEY_RESULT_SWITCH_PAGE;
        Keyboard_SetKanaPage(kb, kb->kanaPage ^ 1, kb->kanaGroup);
        break;
    case KEY_KIND_DAKUTEN:
        kb->result = KEY_RESULT_DAKUTEN;
        break;
    case KEY_KIND_HANDAKUTEN:
        kb->result = KEY_RESULT_HANDAKUTEN;
        break;
    }
    if (KeyLayout_GetRect(&kb->layout, &input->pos, &rect)) {
        KeyHighlight_Set(&kb->highlight, 2, &rect, kb->kanaPage, kb->type);
    }
}

static BOOL Keyboard_HandleKeysSlide(Keyboard *kb, KeyInput *input) {
    KeyRect rect;
    KeyPos delta;
    KeyPos pos;
    int kind;
    BOOL moved = FALSE;

    if (!kb->hasUpperLayer) {
        if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
            delta.x = 0;
            delta.y = -1;
            moved = TRUE;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
            delta.x = 0;
            delta.y = 1;
            moved = TRUE;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            delta.x = -1;
            delta.y = 0;
            moved = TRUE;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            delta.x = 1;
            delta.y = 0;
            moved = TRUE;
        }
    }
    if (moved) {
        pos = kb->pos;
        KeyLayout_Move(&kb->layout, &pos, &delta);
        kind = KeyLayout_GetChar(&kb->layout, &pos, NULL);
        if (kind == KEY_KIND_TYPE_0 || kind == KEY_KIND_TYPE_1 || kind == KEY_KIND_TYPE_2
            || kind == KEY_KIND_TYPE_3) {
            if (func_0203d554() == TRUE) {
                if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    func_0203d564(FALSE);
                    KeyHighlight_Set(&kb->highlight, 1, &rect, kb->kanaPage, kb->type);
                }
            } else {
                KeyLayout_Move(&kb->layout, &kb->pos, &delta);
                if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    KeyHighlight_Set(&kb->highlight, 1, &rect, kb->kanaPage, kb->type);
                }
            }
        }
    }
    return FALSE;
}

static BOOL Keyboard_HandleButtonsSlide(Keyboard *kb, KeyInput *input) {
    KeyRect rect;
    BOOL acted = FALSE;

    if (!kb->hasUpperLayer) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            input->pos = kb->pos;
            acted = TRUE;
            input->moved = TRUE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            input->button = kb->type + 3;
            if (input->button > KEY_KIND_TYPE_3) {
                input->button = KEY_KIND_TYPE_0;
            }
            KeyLayout_FindKind(&kb->layout, input->button, &kb->pos);
            input->pos = kb->pos;
            acted = TRUE;
        }
        if (acted && func_0203d554() == TRUE) {
            if (KeyLayout_GetRect(&kb->layout, &kb->pos, &rect)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                func_0203d564(FALSE);
                KeyHighlight_Set(&kb->highlight, 1, &rect, kb->kanaPage, kb->type);
                return FALSE;
            }
        }
    }
    return acted;
}

static BOOL Keyboard_HandleTouchSlide(Keyboard *kb, KeyInput *input) {
    TouchPos touch;
    u32 touchX;
    u32 touchY;
    int x;
    int y;
    int kind;

    if (!kb->hasUpperLayer) {
        if (func_0203dac8(&touchX, &touchY)) {
            touch.x = touchX;
            touch.y = touchY;
            if (KeyLayout_FindAtTouch(&kb->layout, &touch, &kb->pos)) {
                kind = KeyLayout_GetChar(&kb->layout, &kb->pos, NULL);
                if (kind == KEY_KIND_TYPE_0 || kind == KEY_KIND_TYPE_1 || kind == KEY_KIND_TYPE_2
                    || kind == KEY_KIND_TYPE_3) {
                    x = kb->pos.x;
                    y = kb->pos.y;
                    input->pos.y = y;
                    input->moved = TRUE;
                    input->pos.x = x;
                    func_0203d564(TRUE);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static void Keyboard_SelectSlide(Keyboard *kb, KeyInput *input) {
    KeyRect rect;
    int kind;

    if (input->moved) {
        kind = KeyLayout_GetChar(&kb->layout, &kb->pos, &kb->lastChar);
    } else {
        kind = input->button;
    }
    switch (kind) {
    case KEY_KIND_TYPE_0:
    case KEY_KIND_TYPE_1:
    case KEY_KIND_TYPE_2:
    case KEY_KIND_TYPE_3:
        if (Keyboard_StartSwitch(kb, kind - 2)) {
            GFL_SndSEPlay(SEQ_SE_SYS_12);
            kb->state = 2;
            kb->result = KEY_RESULT_CLEAR_COMPOSE;
        }
        break;
    }
    if (KeyLayout_GetRect(&kb->layout, &input->pos, &rect)) {
        KeyHighlight_Set(&kb->highlight, 2, &rect, kb->kanaPage, kb->type);
    }
}

static void NameEntryMessage_Init(NameEntryMessage *message, Font *font, MsgData *msgData, PrintQueue *queue,
                                  WordSet *wordSet, HeapID heapId) {
    BmpWin *window;

    sys_memset(message, 0, sizeof(NameEntryMessage));
    message->fillColor = 15;
    message->font = font;
    message->wordSet = wordSet;
    message->msgData = msgData;
    message->queue = queue;
    message->buf = GFL_StrBufCreate(255, heapId);
    message->window = BmpWin_CreateDynamic(4, 1, 19, 30, 4, 15, TRUE);
    BmpWin_DrawFrame(message->window, 2, 1, 14);
    message->printWindow = message->window;
    message->printing = FALSE;
    GFL_BitmapFill(BmpWin_GetBitmap(message->window), message->fillColor);
    window = message->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
}

static void NameEntryMessage_Free(NameEntryMessage *message) {
    BmpWin_Free(message->window);
    GFL_StrBufFree(message->buf);
    sys_memset(message, 0, sizeof(NameEntryMessage));
}

static void NameEntryMessage_Print(NameEntryMessage *message, u32 messageId) {
    PrintQueue *queue;

    while (func_02021c1c(message->queue, BmpWin_GetBitmap(message->window))) {
        func_02021a3c(message->queue);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(message->window), message->fillColor);
    GFL_MsgDataLoadStrbuf(message->msgData, messageId, message->buf);
    queue = message->queue;
    func_02021c54(queue, BmpWin_GetBitmap(message->printWindow), 0, 0, message->buf, message->font);
    message->printing = TRUE;
}

static void NameEntryMessage_PrintSpecies(NameEntryMessage *message, u32 messageId, u16 species, u32 form,
                                          HeapID heapId) {
    PartyPkm *pkm = PokeParty_NewTempPkm(species, 1, 0, heapId);

    PokeParty_SetParam(pkm, PKM_PARAM_FORM, form);
    NameEntryMessage_PrintPokemon(message, messageId, pkm);
    GFL_HeapFree(pkm);
}

static void NameEntryMessage_PrintPokemon(NameEntryMessage *message, u32 messageId, PartyPkm *pkm) {
    StrBuf *format;
    PrintQueue *queue;

    setPartyPokemonSpeciesNameToStrbuf(message->wordSet, 0, pkm);
    GFL_BitmapFill(BmpWin_GetBitmap(message->window), message->fillColor);
    format = GFL_MsgDataLoadStrbufNew(message->msgData, messageId);
    GFL_WordSetFormatStrbuf(message->wordSet, message->buf, format);
    queue = message->queue;
    func_02021c54(queue, BmpWin_GetBitmap(message->printWindow), 0, 0, message->buf, message->font);
    message->printing = TRUE;
    GFL_StrBufFree(format);
}

static BOOL NameEntryMessage_IsPrintDone(NameEntryMessage *message) {
    PrintQueue *queue = message->queue;

    if (message->printing) {
        if (!func_02021c1c(queue, BmpWin_GetBitmap(message->printWindow))) {
            BmpWin_FlushChar(message->printWindow);
            message->printing = FALSE;
        }
    }
    if (!message->printing) {
        return TRUE;
    }
    return FALSE;
}

static void NameEntryTalk_Init(NameEntryTalk *talk, HeapID heapId) {
    sys_memset(talk, 0, sizeof(NameEntryTalk));
    talk->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 0);
}

static void NameEntryTalk_Free(NameEntryTalk *talk) {
    if (talk->stream != NULL) {
        func_020223cc(talk->stream);
    }
    GFL_TCBExMgrFree(talk->tcbManager);
    sys_memset(talk, 0, sizeof(NameEntryTalk));
}

// Prints the message that the name was given
static void NameEntryTalk_Start(NameEntryTalk *talk, NameEntryMessage *message, NameEntryParam *param, HeapID heapId) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);
    StrBuf *name;

    if (func_ov012_02165b0c(param)) {
        loadPokemonNicknameToStrbuf(wordSet, 0, param->pkm);
    } else {
        name = GFL_StrBufClone(param->name, heapId);
        func_ov012_02165afc(param, name);
        func_0202437c(wordSet, 0, name, 0, 0, 0);
        GFL_StrBufFree(name);
    }
    loadBoxNameForDisplay(wordSet, 1, (void *)param->unk10, param->unk14);
    if (talk->stream != NULL) {
        func_020223cc(talk->stream);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(message->window), message->fillColor);
    GFL_StrBufClear(message->buf);
    GFL_WordSetFormatStrbuf(wordSet, message->buf, (const StrBuf *)param->unkC);
    talk->stream = func_02022268(message->window, 0, 0, message->buf, message->font, func_02017bcc(),
                                 talk->tcbManager, 2, heapId, message->fillColor);
    talk->frame = 0;
    talk->next = NameEntryTask_FadeOut;
    talk->waitFrames = 60;
    GFL_WordSetSystemFree(wordSet);
}

static void NameEntryIcon_Init(NameEntryIcon *icon, u32 type, u32 species, u32 form, ClActUnit *unit, Font *font,
                               PrintQueue *queue, MsgData *msgData, BmpOamSys *bmpOam, HeapID heapId) {
    BmpOamActorSetup labelSetup;
    ClActorSetup setup;
    BOOL hasLabel = FALSE;
    int arcId;
    u32 paletteFile;
    u32 charsFile;
    u32 cellFile;
    u32 animFile;
    u8 paletteStart;
    ArcTool *arc;
    int x = 34;
    int y = 20;
    int sex;
    BOOL ownPalette = FALSE;
    u32 messageId;
    u16 color;

    sys_memset(icon, 0, sizeof(NameEntryIcon));
    paletteStart = 0;
    switch (type) {
    case ICON_TYPE_PLAYER:
    case ICON_TYPE_RIVAL:
        arcId = getUINarcIdx();
        paletteFile = func_0202d8a0();
        charsFile = func_0202d8a4();
        cellFile = func_0202d8a8(2);
        animFile = func_0202d8ac(2);
        sex = species;
        break;
    case ICON_TYPE_FRIEND:
        arcId = ARCID_NAMEIN_ICON_TYPE2;
        paletteFile = 0;
        paletteStart = sIconPalettes[species];
        charsFile = species + 49;
        cellFile = 65;
        animFile = 66;
        sex = species >= 8 ? 1 : 0;
        break;
    case ICON_TYPE_POKEMON:
    case ICON_TYPE_POKEMON_NO_LABEL:
        hasLabel = TRUE;
        sex = (form & 0xff00) >> 8;
        if (sex == 2) {
            hasLabel = FALSE;
        }
        if (type == ICON_TYPE_POKEMON_NO_LABEL) {
            hasLabel = FALSE;
        }
        arcId = ARCID_POKEICON;
        paletteFile = PokeIcon_GetPaletteFile();
        charsFile = PokeParty_GetIconIndex(species, (u8)form, sex, FALSE);
        cellFile = PokeIcon_GetCellFile_Main();
        animFile = getOBJTileMapping_MainEng();
        x = 30;
        y = 20;
        ownPalette = TRUE;
        break;
    case ICON_TYPE_AVENUE:
        arcId = ARCID_NAMEIN_ICON_TYPE6;
        paletteFile = 14;
        charsFile = 13;
        cellFile = 12;
        animFile = 11;
        y = 28;
        break;
    case ICON_TYPE_BOX:
    case ICON_TYPE_KEY:
        arcId = ARCID_NAMEIN_GRAPHIC;
        paletteFile = 9;
        charsFile = 8;
        cellFile = 7;
        animFile = 6;
        paletteStart = 1;
        break;
    }

    arc = GFL_ArcSysCreateFileHandle(arcId, heapId);
    if (ownPalette) {
        icon->palette = func_0204bc48(arc, paletteFile, 0, 128, heapId);
    } else {
        icon->palette = func_0204bbb8(arc, paletteFile, 0, 128, paletteStart, 1, heapId);
    }
    icon->cellAnims = func_0204bde0(arc, cellFile, animFile, heapId);
    icon->chars = func_0204b81c(arc, charsFile, FALSE, 0, heapId);
    GFL_ArcToolFree(arc);

    icon->baseY = y;
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = x;
    setup.y = y;
    icon->actor = func_0204c040(unit, icon->chars, icon->palette, icon->cellAnims, &setup, 0, heapId);
    func_0204c520(icon->actor, TRUE);
    switch (type) {
    case ICON_TYPE_PLAYER:
        if (species == 0) {
            NsbtxToClwk_LoadTexture(icon->actor, 48, 209, 3, 4, 4, 0, 0, heapId);
        } else {
            NsbtxToClwk_LoadTexture(icon->actor, 48, 218, 3, 4, 4, 0, 0, heapId);
        }
        break;
    case ICON_TYPE_RIVAL:
        NsbtxToClwk_LoadTexture(icon->actor, 48, 48 + 221, 3, 4, 4, 0, 0, heapId);
        break;
    case ICON_TYPE_FRIEND:
        func_0204c378(icon->actor, 0, TRUE);
        break;
    case ICON_TYPE_POKEMON:
    case ICON_TYPE_POKEMON_NO_LABEL:
        func_0204c488(icon->actor, 1);
        func_0204c378(icon->actor, PokeIcon_GetPalette(species, (u8)form, (form & 0xff00) >> 8, FALSE), FALSE);
        break;
    case ICON_TYPE_KEY:
        func_0204c488(icon->actor, 5);
        func_0204c378(icon->actor, 0, TRUE);
        break;
    case ICON_TYPE_BOX:
        func_0204c488(icon->actor, 2);
        func_0204c378(icon->actor, 0, TRUE);
        break;
    case ICON_TYPE_AVENUE:
        func_0204c378(icon->actor, 0, TRUE);
        break;
    }

    if (hasLabel) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, heapId);
        icon->labelBitmap = GFL_BitmapCreate(2, 2, 32, heapId);
        icon->labelPalette = func_0204bc48(arc, 5, 0, 224, heapId);
        GFL_ArcToolFree(arc);
        sys_memset(&labelSetup, 0, sizeof(BmpOamActorSetup));
        labelSetup.bitmap = icon->labelBitmap;
        labelSetup.x = 46;
        labelSetup.y = 20;
        labelSetup.palette = icon->labelPalette;
        labelSetup.priority = 0;
        labelSetup.surface = 0;
        labelSetup.vramType = 0;
        labelSetup.bgPriority = 0;
        labelSetup.paletteOffset = 0;
        icon->labelActor = BmpOam_ActorAdd(bmpOam, &labelSetup);
        if (sex == 1) {
            messageId = AbilityHandlersBtlMain_Text_Empty_6;
            color = 50 << 6;
        } else {
            messageId = AbilityHandlersBtlMain_Text_Empty_5;
            color = 83 << 6;
        }
        icon->label = GFL_MsgDataLoadStrbufNew(msgData, messageId);
        GFL_BitmapFill(icon->labelBitmap, 0);
        func_02021c7c(queue, icon->labelBitmap, 0, 0, icon->label, font, color);
        icon->labelQueue = queue;
        icon->labelDirty = TRUE;
    }
}

static void NameEntryIcon_Free(NameEntryIcon *icon) {
    if (icon->labelActor != NULL) {
        func_0204bcd0(icon->labelPalette);
        GFL_StrBufFree(icon->label);
        BmpOam_ActorDel(icon->labelActor);
        GFL_BitmapFree(icon->labelBitmap);
    }
    func_0204bcd0(icon->palette);
    func_0204b98c(icon->chars);
    func_0204be64(icon->cellAnims);
    func_0204c108(icon->actor);
    sys_memset(icon, 0, sizeof(NameEntryIcon));
}

static void NameEntryIcon_Update(NameEntryIcon *icon) {
    KeyRect rect;
    u32 x;
    u32 y;
    ClActorPos pos;
    TouchPos touch;

    switch (icon->state) {
    case 0:
        func_0204c178(icon->actor, &pos, 0);
        icon->touched = FALSE;
        if (func_0203dac8(&x, &y)) {
            rect.left = pos.x - 16;
            rect.top = pos.y - 16;
            rect.right = pos.x + 16;
            rect.bottom = pos.y + 16;
            touch.x = x;
            touch.y = y;
            if (NameEntry_PointInRect(&rect, &touch)) {
                NameEntryIcon_StartBounce(icon);
                icon->state = 1;
            }
        }
        break;
    case 1:
        if (NameEntryIcon_Bounce(icon)) {
            icon->touched = TRUE;
            icon->state = 0;
        }
        break;
    }
}

static void NameEntryIcon_UpdateLabel(NameEntryIcon *icon) {
    if (icon->labelDirty) {
        if (!func_02021c1c(icon->labelQueue, icon->labelBitmap)) {
            BmpOam_ActorBmpTrans(icon->labelActor);
            icon->labelDirty = FALSE;
        }
    }
}

static BOOL NameEntryIcon_WasTouched(NameEntryIcon *icon) {
    return icon->touched;
}

static void NameEntryIcon_StartBounce(NameEntryIcon *icon) {
    icon->frame = 0;
    icon->bouncing = TRUE;
}

// Makes the icon hop; returns TRUE once it is not hopping
static BOOL NameEntryIcon_Bounce(NameEntryIcon *icon) {
    ClActorPos pos;
    u16 frame;
    u16 angle;

    if (icon->bouncing) {
        frame = icon->frame++;
        func_0204c178(icon->actor, &pos, 0);
        angle = frame * 0x7fff / 8;
        pos.y = icon->baseY - (12 * FX_SinIdx(angle) >> 12);
        func_0204c140(icon->actor, &pos, 0);
        if (icon->frame > 8) {
            icon->bouncing = FALSE;
        }
        return FALSE;
    }
    return TRUE;
}

static u32 NameEntry_GetIconType(u32 mode, u32 species) {
    switch (mode) {
    case NAME_ENTRY_PLAYER:
    case NAME_ENTRY_GREETING:
    case NAME_ENTRY_THANKS:
    case NAME_ENTRY_PASSERBY_MESSAGE:
    case NAME_ENTRY_AVENUE_TITLE:
    case NAME_ENTRY_FAVORITE_PHRASE:
    case NAME_ENTRY_IMPRESSED:
    case NAME_ENTRY_TRAINER_MESSAGE:
    case NAME_ENTRY_HAPPY_MESSAGE:
        return ICON_TYPE_PLAYER;
    case NAME_ENTRY_POKEMON:
        return ICON_TYPE_POKEMON;
    case NAME_ENTRY_POKEMON_RENAME:
        return ICON_TYPE_POKEMON_NO_LABEL;
    case NAME_ENTRY_BOX:
        return ICON_TYPE_BOX;
    case NAME_ENTRY_RIVAL:
        return ICON_TYPE_RIVAL;
    case NAME_ENTRY_FRIEND:
        if (species == 0xffff) {
            return ICON_TYPE_KEY;
        }
        return ICON_TYPE_FRIEND;
    case NAME_ENTRY_AVENUE_NAME:
        return ICON_TYPE_AVENUE;
    case NAME_ENTRY_GAME_SYNC_ID:
        return ICON_TYPE_KEY;
    case NAME_ENTRY_PLASMA_PASSWORD:
        return ICON_TYPE_KEY;
    default:
        return ICON_TYPE_PLAYER;
    }
}

static void NameEntryObjs_ShowSelectors(NameEntryObjs *objs, BOOL show) {
    int i;

    if (show == TRUE) {
        for (i = 21; i <= 23; i++) {
            func_0204c124(NameEntryObjs_Get(objs, i), TRUE);
        }
    }
}

// Returns which of the preset names' selectors is touched, or 3
static int NameEntryObjs_UpdateSelectors(NameEntryObjs *objs) {
    KeyRect rect;
    u32 x;
    u32 y;
    ClActorPos pos;
    TouchPos touch;
    int i;

    for (i = 21; i <= 23; i++) {
        func_0204c178(NameEntryObjs_Get(objs, i), &pos, 0);
        if (func_0203dac8(&x, &y)) {
            rect.left = pos.x - 8;
            rect.top = pos.y - 12;
            rect.right = pos.x + 8;
            rect.bottom = pos.y + 12;
            touch.x = x;
            touch.y = y;
            if (NameEntry_PointInRect(&rect, &touch)) {
                func_0204c520(NameEntryObjs_Get(objs, i), TRUE);
                func_0204c4d4(NameEntryObjs_Get(objs, i), 0);
                func_0204c56c(NameEntryObjs_Get(objs, i));
                return i - 21;
            }
        }
    }
    return 3;
}

// The name that the trainer card, or the avenue, already has
static StrBuf *NameEntry_LoadPresetName(NameEntryParam *param, u32 which, HeapID heapId) {
    StrBuf *buf = GFL_StrBufCreate(9, heapId);

    switch (which) {
    case 0:
        GFL_StrBufLoadString(buf, TrainerGameInfo_GetGreeting(param->gameInfo));
        break;
    case 1:
        GFL_StrBufLoadString(buf, TrainerGameInfo_GetThanks(param->gameInfo));
        break;
    case 2:
        GFL_StrBufLoadString(buf, func_0201150c(param->unk30));
        break;
    default:
        return NULL;
    }
    return buf;
}

static BOOL NameEntry_PointInRect(const KeyRect *rect, const TouchPos *touch) {
    return (touch->x - rect->left <= rect->right - rect->left) & (touch->y - rect->top <= rect->bottom - rect->top);
}

// The name that touching the icon puts in the field
static StrBuf *NameEntry_GetPresetName(NameEntryWork *work, u32 mode, HeapID heapId) {
    switch (mode) {
    case 0:
        return NULL;
    case 1:
        if (GFL_StrBufGetCharCount(work->name) == 0) {
            return GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, work->param->gender);
        }
        return GFL_StrBufClone(work->name, heapId);
    case 2:
        return GFL_StrBufClone(work->name, heapId);
    case 3:
        return GFL_StrBufClone(work->name, heapId);
    case 4:
        return GFL_StrBufClone(work->name, heapId);
    case 5:
        return GFL_StrBufClone(work->name, heapId);
    case 6:
        return GFL_StrBufClone(work->name, heapId);
    case 15:
        return GFL_StrBufClone(work->name, heapId);
    case 7:
    case 14:
        return GFL_StrBufClone(work->name, heapId);
    case 11:
        return GFL_StrBufClone(work->name, heapId);
    default:
        return NULL;
    }
}

// Settles what the name entry reports: whether the name is as it was, which preset name it matches
static void NameEntry_CheckChanged(NameEntryWork *work) {
    StrBuf *previous;
    StrBuf *preset;
    int i;

    work->param->unk1C = FALSE;
    previous = GFL_StrBufClone(work->param->name, HEAPID_NAMEIN);
    NameBox_GetString(&work->box, work->param->name);
    if (work->param->mode == NAME_ENTRY_GAME_SYNC_ID) {
        NameEntry_UpperCaseName(work->param->name);
    }
    if (GFL_StrBufCmp(previous, work->param->name)) {
        work->param->unk1C = TRUE;
    }
    if (!NameBox_HasVisibleChar(&work->box)) {
        GFL_StrBufCopy(work->param->name, previous);
        work->param->unk1C = TRUE;
    }
    GFL_StrBufFree(previous);

    if (work->usesPresetNames == TRUE && work->presetNameFilled) {
        preset = GFL_StrBufCreate(9, HEAPID_NAMEIN);
        for (i = 0; i <= 2; i++) {
            switch (i) {
            case 0:
                GFL_StrBufLoadString(preset, TrainerGameInfo_GetGreeting(work->param->gameInfo));
                break;
            case 1:
                GFL_StrBufLoadString(preset, TrainerGameInfo_GetThanks(work->param->gameInfo));
                break;
            case 2:
                GFL_StrBufLoadString(preset, func_0201150c(work->param->unk30));
                break;
            }
            if (GFL_StrBufCmp(preset, work->param->name)) {
                work->param->unk34 = i + 1;
                break;
            }
        }
        GFL_StrBufFree(preset);
    }
    if (GFL_StrBufGetCharCount(work->param->name) == 0 && work->param->mode != NAME_ENTRY_PLAYER
        && work->param->mode != NAME_ENTRY_RIVAL) {
        work->param->unk1C = TRUE;
    }
    if (work->param->mode == NAME_ENTRY_POKEMON && work->param->unk1C == FALSE) {
        func_02038bc8(12);
    }
}

static BOOL NameEntry_IsNameAcceptable(NameEntryWork *work) {
    u32 mode = work->param->mode;

    if (mode == NAME_ENTRY_PLAYER || mode == NAME_ENTRY_RIVAL || mode == NAME_ENTRY_AVENUE_TITLE
        || mode == NAME_ENTRY_FAVORITE_PHRASE || mode == NAME_ENTRY_IMPRESSED) {
        if (NameBox_GetLength(&work->box) == 0 && NameBox_GetComposeLength(&work->box) == 0) {
            return FALSE;
        }
        if (!NameBox_HasVisibleChar(&work->box)) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL NameEntry_IsGameSyncIdValid(NameEntryWork *work) {
    return NameBox_IsValidGameSyncId(&work->box);
}

static void NameEntry_UpperCaseName(StrBuf *name) {
    u16 chars[12];
    int i;

    GFL_StrBufStoreString(name, chars, 11);
    i = 0;
    while (chars[i] != GFL_StrBufGetTerminator()) {
        chars[i] = GFL_StrCharToUpperCase(chars[i]);
        i++;
    }
    GFL_StrBufLoadString(name, chars);
}

static void NameEntryTask_Init(NameEntryTask *task, NameEntryWork *work, NameEntryTaskFunc func) {
    sys_memset(task, 0, sizeof(NameEntryTask));
    task->work = work;
    NameEntryTask_SetFunc(task, func);
}

static void NameEntryTask_Clear(NameEntryTask *task) {
    sys_memset(task, 0, sizeof(NameEntryTask));
}

static void NameEntryTask_Run(NameEntryTask *task) {
    if (!task->done) {
        task->func(task, &task->state, task->work);
    }
}

static BOOL NameEntryTask_IsDone(NameEntryTask *task) {
    return task->done;
}

static void NameEntryTask_SetFunc(NameEntryTask *task, NameEntryTaskFunc func) {
    task->func = func;
    task->state = 0;
}

static void NameEntryTask_Finish(NameEntryTask *task) {
    task->done = TRUE;
}

static void NameEntryTask_WaitDraw(NameEntryTask *task, u32 *state, NameEntryWork *work) {
#ifdef BUGFIX
    BOOL keyboardDone = Keyboard_IsPrintDone(&work->keyboard);
    BOOL nameDone = NameBox_IsPrintDone(&work->box);
    BOOL messageDone = NameEntryMessage_IsPrintDone(&work->message);

    if (keyboardDone && nameDone && messageDone) {
        NameEntryTask_SetFunc(task, NameEntryTask_FadeIn);
    }
#else
    // BUG: the fade in starts when any one of the three has printed, not when all have
    BOOL ready = Keyboard_IsPrintDone(&work->keyboard);

    ready |= NameBox_IsPrintDone(&work->box);
    if (NameEntryMessage_IsPrintDone(&work->message) | ready) {
        NameEntryTask_SetFunc(task, NameEntryTask_FadeIn);
    }
#endif
}

static void NameEntryTask_FadeIn(NameEntryTask *task, u32 *state, NameEntryWork *work) {
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 16, 0, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        NameEntryTask_SetFunc(task, NameEntryTask_Input);
        break;
    }
}

static void NameEntryTask_FadeOut(NameEntryTask *task, u32 *state, NameEntryWork *work) {
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 0, 16, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        NameEntryTask_SetFunc(task, NameEntryTask_SaveKeyboardType);
        break;
    }
}

static void NameEntryTask_Talk(NameEntryTask *task, u32 *state, NameEntryWork *work) {
    NameEntryTalk *talk = &work->talk;

    switch (*state) {
    case 0:
        GFL_TCBExMgrUpdate(talk->tcbManager);
        switch (func_020223b4(talk->stream)) {
        case PRINT_STREAM_RUNNING:
            if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da2c()) {
                func_020223e0(talk->stream, 0);
                if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                    func_0203d564(FALSE);
                } else {
                    func_0203d564(TRUE);
                }
            }
            break;
        case PRINT_STREAM_DONE:
            *state = 1;
            break;
        case PRINT_STREAM_PAUSED:
            if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
                func_020223bc(talk->stream);
                if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                    func_0203d564(FALSE);
                } else {
                    func_0203d564(TRUE);
                }
            }
            break;
        }
        break;
    case 1:
        if (talk->frame >= talk->waitFrames) {
            *state = 2;
        } else {
            talk->frame++;
        }
        break;
    case 2:
        NameEntryTask_SetFunc(task, talk->next);
        break;
    }
}

static void NameEntryTask_Input(NameEntryTask *task, u32 *state, NameEntryWork *work) {
    u16 c;
    int result;
    u32 page;
    BOOL success;
    StrBuf *name;
    int which;

    result = Keyboard_GetResult(&work->keyboard, &c);
    page = Keyboard_GetKanaPage(&work->keyboard);
    if (work->messagePending == TRUE) {
        if (result != 0 || NameEntryIcon_WasTouched(&work->icon) || (GCTX_HIDGetPressedKeys() & ~PAD_BUTTON_A) != 0) {
            if (work->param->mode == NAME_ENTRY_POKEMON) {
                if (work->param->pkm == NULL) {
                    NameEntryMessage_PrintSpecies(&work->message, work->param->mode, work->param->gender,
                                                  work->param->unk6, HEAPID_NAMEIN);
                } else {
                    NameEntryMessage_PrintPokemon(&work->message, work->param->mode, work->param->pkm);
                }
            } else {
                NameEntryMessage_Print(&work->message, work->param->mode);
            }
            work->messagePending = FALSE;
        }
    }

    switch (result) {
    case KEY_RESULT_CHAR:
        if (NameBox_AddChar(&work->box, c)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case KEY_RESULT_COMPOSE_CHAR:
        if (NameBox_AddComposeChar(&work->box, c, page)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case KEY_RESULT_DAKUTEN:
        success = TRUE;
        if (!NameBox_Compose(&work->box, 0)) {
            success = FALSE;
        }
        if (success) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case KEY_RESULT_HANDAKUTEN:
        success = TRUE;
        if (!NameBox_Compose(&work->box, 1)) {
            success = FALSE;
        }
        if (success) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case KEY_RESULT_CYCLE:
        if (NameBox_CycleLast(&work->box)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case KEY_RESULT_BACKSPACE:
        if (NameBox_DeleteChar(&work->box)) {
            GFL_SndSEPlay(SEQ_SE_CANCEL3);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    case KEY_RESULT_CLEAR_COMPOSE:
        NameBox_ClearCompose(&work->box);
        break;
    case KEY_RESULT_COMMIT_COMPOSE:
        NameBox_CommitCompose(&work->box);
        break;
    case KEY_RESULT_OK:
        if (NameBox_GetComposeLength(&work->box) == 0) {
            if (!NameEntry_IsNameAcceptable(work)) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                if (work->param->mode == NAME_ENTRY_PLAYER || work->param->mode == NAME_ENTRY_RIVAL) {
                    NameEntryMessage_Print(&work->message, AbilityHandlersBtlMain_Text_PleaseEnterName);
                } else {
                    NameEntryMessage_Print(&work->message, AbilityHandlersBtlMain_Text_PleaseEnterSomething);
                }
                work->messagePending = TRUE;
                break;
            }
            if (work->param->mode == NAME_ENTRY_GAME_SYNC_ID && !NameEntry_IsGameSyncIdValid(work)) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                NameEntryMessage_Print(&work->message, AbilityHandlersBtlMain_Text_GameSyncIdWrong);
                work->messagePending = TRUE;
                break;
            }
            if (!(work->param->unk2C & 2) && ProfWord_Check(work->profWord, work->box.text, HEAPID_NAMEIN)) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                break;
            }
            if (!(work->param->unk2C & 1) && ProfWord_HasManyDigits(work->box.text, work->box.length, HEAPID_NAMEIN)) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                NameEntryMessage_Print(&work->message, AbilityHandlersBtlMain_Text_NumbersMayContainOnly);
                work->messagePending = TRUE;
                break;
            }
            GFL_SndSEPlay(SEQ_SE_DECIDE2);
            NameEntry_CheckChanged(work);
            if (work->param->mode == NAME_ENTRY_POKEMON && work->param->unkC != 0) {
                NameEntryTalk_Start(&work->talk, &work->message, work->param, HEAPID_NAMEIN);
                NameEntryTask_SetFunc(task, NameEntryTask_Talk);
            } else {
                NameEntryTask_SetFunc(task, NameEntryTask_FadeOut);
            }
        } else {
            NameBox_CommitCompose(&work->box);
            GFL_SndSEPlay(SEQ_SE_DECIDE2);
        }
        break;
    case KEY_RESULT_NONE:
        if (NameEntryIcon_WasTouched(&work->icon)) {
            name = NameEntry_GetPresetName(work, work->param->mode, HEAPID_NAMEIN);
            NameBox_Clear(&work->box);
            if (name != NULL) {
                NameBox_SetString(&work->box, GFL_StrBufGetStringPtr(name));
                GFL_StrBufFree(name);
            }
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        if (work->usesPresetNames == TRUE) {
            which = NameEntryObjs_UpdateSelectors(&work->objs);
            if (which != 3) {
                name = NameEntry_LoadPresetName(work->param, which, HEAPID_NAMEIN);
                NameBox_Clear(&work->box);
                if (name != NULL) {
                    work->presetNameFilled = TRUE;
                    NameBox_SetString(&work->box, GFL_StrBufGetStringPtr(name));
                    GFL_StrBufFree(name);
                }
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
        }
        break;
    case KEY_RESULT_SPACE:
        NameBox_CommitCompose(&work->box);
        if (NameBox_AddChar(&work->box, c)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        break;
    }

    if (work->usesPresetNames == TRUE && work->presetNameFilled == TRUE && result != KEY_RESULT_NONE
        && result != KEY_RESULT_OK) {
        work->presetNameFilled = FALSE;
    }
    Keyboard_Update(&work->keyboard, &work->box);
    NameBox_Update(&work->box);
    NameEntryIcon_Update(&work->icon);
    NameEntryMessage_IsPrintDone(&work->message);
}

static void NameEntryTask_SaveKeyboardType(NameEntryTask *task, u32 *state, NameEntryWork *work) {
    u32 slot = work->param->mode;

    if (slot != NAME_ENTRY_GAME_SYNC_ID && work->param->gameInfo != NULL) {
        if (slot == NAME_ENTRY_IMPRESSED) {
            slot = NAME_ENTRY_FAVORITE_PHRASE;
        } else if (slot == NAME_ENTRY_TRAINER_MESSAGE) {
            slot = NAME_ENTRY_PASSERBY_MESSAGE;
        } else if (slot == NAME_ENTRY_HAPPY_MESSAGE) {
            slot = NAME_ENTRY_THANKS;
        } else if (slot == NAME_ENTRY_POKEMON_RENAME) {
            slot = NAME_ENTRY_POKEMON;
        }
        func_0200c844(work->param->gameInfo, slot, Keyboard_GetType(&work->keyboard));
    }
    if (work->param->saveTask != NULL) {
        NameEntryTask_SetFunc(task, NameEntryTask_PauseSave);
    } else {
        NameEntryTask_Finish(task);
    }
}

static void NameEntryTask_PauseSave(NameEntryTask *task, u32 *state, NameEntryWork *work) {
    switch (*state) {
    case 0:
        SaveControlIntr_Pause(work->param->saveTask);
        *state = 1;
        break;
    case 1:
        if (SaveControlIntr_IsPausedOrDone(work->param->saveTask) == TRUE) {
            NameEntryTask_Finish(task);
        }
        break;
    }
}
