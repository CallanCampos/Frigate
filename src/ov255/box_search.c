#include "types.h"
#include "app/box2_main.h"
#include "app/box_search.h"
#include "app/box_search_graphic.h"
#include "app/ui/ui_scene.h"
#include "app/ui/touchbar.h"
#include "app/ui/frame_list.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/personal.h"
#include "save/pokedex.h"
#include "system/app_menu_common.h"
#include "system/cursor_move.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/btl_pokeparam_box_main_save_init.h"

// The PC box's Pokémon search: lists to pick a species by its first letter, a nature, an ability by its first letter,
// the sex, the held item and the markings that the box's Pokémon are filtered by. Names are ours

// The archive of the Pokédex's orders of species
#define ARCID_ZUKAN_DATA 97

// What the search's lists are of
#define LIST_SPECIES 0
#define LIST_NATURE 1
#define LIST_ABILITY 2
#define LIST_SEX 3
#define LIST_ITEM 4
#define LIST_MARKING 5
#define LIST_SPECIES_LETTER 8
#define LIST_LETTER 9
#define LIST_ABILITY_LETTER 10
#define LIST_LETTER_2 11

// The criteria of Box2SearchParam that func_ov255_021d6a48 sets
#define CRITERION_SPECIES 0
#define CRITERION_ITEM 1
#define CRITERION_NATURE 2
#define CRITERION_ABILITY 3
#define CRITERION_SEX 4
#define CRITERION_MARKS 5
#define CRITERION_ACTIVE 6

// The sequence that ends the search
#define SEQ_END 39
#define SEQ_LIST_WAIT 37
#define SEQ_BUTTON_ANM 38
#define SEQ_TOUCH_BAR_ANM 40

#define WINDOW_INFO 14

typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 palette;
    u16 color;
} BoxSearchWindowData;

typedef struct {
    u8 x;
    u8 y;
    u8 anim;
    u8 bgPriority;
    u8 visible;
    u8 res;
    u8 unk6;
} BoxSearchActorData;

typedef int (*BoxSearchSeqFunc)(BoxSearchWork *wk, int seq);

static BOOL func_ov255_021d3b64(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov255_021d3c90(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov255_021d3d00(GameProc *proc, u32 *state, void *param, void *work);
static void func_ov255_021d3df0(HeapID heapId);
static TouchBar *func_ov255_021d3e9c(BoxSearchWork *wk, ClActUnit *unit, HeapID heapId);
static void func_ov255_021d3eec(BoxSearchWork *wk);
static void func_ov255_021d3ef8(TouchBar *bar);
static void func_ov255_021d3f00(BoxSearchWork *wk);
static void func_ov255_021d3f5c(BoxSearchWork *wk);
static void func_ov255_021d3f74(BoxSearchWork *wk, ClActUnit *unit, HeapID heapId);
static void func_ov255_021d40cc(BoxSearchWork *wk);
static void func_ov255_021d40e8(BoxSearchWork *wk, int mode);
static void func_ov255_021d41b0(BmpWin *window);
static void func_ov255_021d41c4(BoxSearchWork *wk);
static void func_ov255_021d4210(BoxSearchWork *wk, u32 index);
static void func_ov255_021d4248(BoxSearchWork *wk, MsgData *msgData, u32 msgId);
static void func_ov255_021d4288(BoxSearchWork *wk);
static void func_ov255_021d42d4(BoxSearchWork *wk);
static void func_ov255_021d4320(BoxSearchWork *wk, BOOL visible);
static int func_ov255_021d434c(BoxSearchWork *wk, int seq);
static int func_ov255_021d43cc(BoxSearchWork *wk, int seq, u32 pos);
static int func_ov255_021d4484(BoxSearchWork *wk, int seq);
static int func_ov255_021d4570(BoxSearchWork *wk, int seq);
static int func_ov255_021d4574(BoxSearchWork *wk, int seq);
static int func_ov255_021d45d8(BoxSearchWork *wk, int seq);
static int func_ov255_021d481c(BoxSearchWork *wk, int seq);
static int func_ov255_021d4844(BoxSearchWork *wk, int seq);
static int func_ov255_021d48dc(BoxSearchWork *wk, int seq);
static int func_ov255_021d4b5c(BoxSearchWork *wk, int seq);
static int func_ov255_021d4b7c(BoxSearchWork *wk, int seq);
static int func_ov255_021d4be0(BoxSearchWork *wk, int seq);
static int func_ov255_021d4db8(BoxSearchWork *wk, int seq);
static int func_ov255_021d4dc8(BoxSearchWork *wk, int seq);
static int func_ov255_021d4e2c(BoxSearchWork *wk, int seq);
static int func_ov255_021d5068(BoxSearchWork *wk, int seq);
static int func_ov255_021d5090(BoxSearchWork *wk, int seq);
static int func_ov255_021d5128(BoxSearchWork *wk, int seq);
static int func_ov255_021d539c(BoxSearchWork *wk, int seq);
static int func_ov255_021d53bc(BoxSearchWork *wk, int seq);
static int func_ov255_021d541c(BoxSearchWork *wk, int seq);
static int func_ov255_021d555c(BoxSearchWork *wk, int seq);
static int func_ov255_021d556c(BoxSearchWork *wk, int seq);
static int func_ov255_021d55cc(BoxSearchWork *wk, int seq);
static int func_ov255_021d5700(BoxSearchWork *wk, int seq);
static int func_ov255_021d5710(BoxSearchWork *wk, int seq);
static int func_ov255_021d57a0(BoxSearchWork *wk, int seq);
static int func_ov255_021d58dc(BoxSearchWork *wk, int seq);
static int func_ov255_021d58ec(BoxSearchWork *wk, int seq);
static int func_ov255_021d591c(BoxSearchWork *wk, int seq);
static int func_ov255_021d594c(BoxSearchWork *wk, int seq);
static int func_ov255_021d5980(BoxSearchWork *wk, int seq);
static int func_ov255_021d5984(BoxSearchWork *wk, int seq);
static int func_ov255_021d59b0(BoxSearchWork *wk, int seq);
static int func_ov255_021d5a10(BoxSearchWork *wk, int seq);
static int func_ov255_021d5a14(BoxSearchWork *wk, int seq);
static int func_ov255_021d5a54(BoxSearchWork *wk, int seq);
static void func_ov255_021d5a68(BoxSearchWork *wk, int seq);
static int func_ov255_021d5a70(BoxSearchWork *wk, int actor, u32 anim, int next);
static int func_ov255_021d5a98(BoxSearchWork *wk, int actor, u32 anim, int next);
static void func_ov255_021d5abc(void *work, int pos, int prevPos);
static void func_ov255_021d5b00(void *work, int pos, int prevPos);
static void func_ov255_021d5b28(void *work, int pos, int prevPos);
static void func_ov255_021d5b6c(void *work, int pos, int prevPos);
static void func_ov255_021d5b70(BoxSearchWork *wk, u32 type);
static void func_ov255_021d5ba4(BoxSearchWork *wk);
static void func_ov255_021d5bc4(BoxSearchWork *wk);
static void func_ov255_021d5c0c(BoxSearchWork *wk);
static void func_ov255_021d5c2c(BoxSearchWork *wk, u32 win, u32 string, int x, int y);
static void func_ov255_021d5c84(BoxSearchWork *wk, u32 win, u32 msgId, int x, int y);
static void func_ov255_021d5ce4(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d5d38(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d5d8c(void *work, u32 index, BOOL moved);
static void func_ov255_021d5d90(void *work, s16 delta);
static void func_ov255_021d5da8(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d5ddc(void *work, u32 index, BOOL moved);
static void func_ov255_021d5df4(void *work, s16 delta);
static void func_ov255_021d5e24(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d5ec8(void *work, u32 index, BOOL moved);
static void func_ov255_021d5f50(void *work, s16 delta);
static void func_ov255_021d5f68(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d5fa0(void *work, u32 index, BOOL moved);
static void func_ov255_021d5fb8(void *work, s16 delta);
static void func_ov255_021d5fe8(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d5ff4(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d6000(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void func_ov255_021d6038(BoxSearchWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData, u32 msgId,
                                u16 color, int y);
static void func_ov255_021d6098(BoxSearchWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                u32 msgId);
static void func_ov255_021d60b4(BoxSearchWork *wk, u32 index, PrintWindow *window, MsgData *msgData);
static void func_ov255_021d60ec(BoxSearchWork *wk, u32 index, PrintWindow *window, MsgData *msgData);
static int func_ov255_021d612c(int letter, int group);
static void func_ov255_021d6150(BoxSearchWork *wk);
static int func_ov255_021d6214(BoxSearchWork *wk, int letter, int group);
static int func_ov255_021d622c(int letter, int group);
static void func_ov255_021d6250(BoxSearchWork *wk, u32 ability);
static void func_ov255_021d6308(BoxSearchWork *wk, u32 species);
static void func_ov255_021d63cc(BoxSearchWork *wk, u32 mode, int pos);
static void func_ov255_021d6798(BoxSearchWork *wk);
static void func_ov255_021d67b0(BoxSearchWork *wk);
static void func_ov255_021d6804(BoxSearchWork *wk, u32 actor, s16 y);
static void func_ov255_021d6828(BoxSearchWork *wk, u32 actor, int row);
static void func_ov255_021d683c(BoxSearchWork *wk, s8 delta);
static void func_ov255_021d6894(BoxSearchWork *wk, u32 actor, int count, int value);
static u16 *func_ov255_021d68d8(u32 heapId, u32 *count);
static void func_ov255_021d68fc(BoxSearchWork *wk, u32 screen);
static void func_ov255_021d6940(BoxSearchWork *wk, BOOL show);
static void func_ov255_021d6a48(BoxSearchWork *wk, u32 criterion, u32 value);
static void func_ov255_021d6a88(BoxSearchWork *wk, u32 mark, u32 on);
static u32 func_ov255_021d6aac(BoxSearchWork *wk, u32 mark);
static void func_ov255_021d6ac0(BoxSearchWork *wk, u32 actor, u32 anim);
static void func_ov255_021d6ae8(BoxSearchWork *wk, u32 actor);
static void func_ov255_021d6b1c(BoxSearchWork *wk, u32 layout);
static void func_ov255_021d6b5c(BoxSearchWork *wk, u32 actor, BOOL visible);
static void func_ov255_021d6b80(BoxSearchWork *wk);
static void func_ov255_021d6bac(BoxSearchWork *wk);
static void func_ov255_021d6bf0(BoxSearchWork *wk, BOOL keep);
static void func_ov255_021d6c94(BoxSearchWork *wk);
static BOOL func_ov255_021d6cd4(BoxSearchWork *wk);
static int func_ov255_021d6cf8(BoxSearchWork *wk);

// The data is declared in the order that gives the ROM's layout once MWCC sorts it by size. The windows are listed in
// data_ov255_021d967c, and the callbacks of each list are given in func_ov255_021d63cc

// Where the scroll bar starts
static const ClActorPos data_ov255_021d8db0 = { 244, 12 };

// The messages of the held item's criterion
static const u16 data_ov255_021d8dba[] = { 199, 204, 203 };

// The screens of the lower screen's frame
static const u16 data_ov255_021d8db4[] = { 79, 81, 80 };

// The messages of the sex's criterion
static const u16 data_ov255_021d8dc0[] = { 199, 200, 201, 202 };

static const BoxSearchWindowData data_ov255_021d8e58 = { 0, 16, 15, 14, 3, 14, PRINT_COLOR(15, 2, 0) };

static const FrameListCallbacks data_ov255_021d8eac = {
    func_ov255_021d6000,
    func_ov255_021d5d8c,
    func_ov255_021d5d90,
};

static const BoxSearchWindowData data_ov255_021d8e7c = { 0, 16, 6, 14, 3, 1, PRINT_COLOR(15, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8dec = { 0, 3, 12, 9, 3, 14, PRINT_COLOR(15, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8e40 = { 0, 16, 12, 14, 3, 1, PRINT_COLOR(15, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8e10 = { 0, 3, 6, 9, 3, 14, PRINT_COLOR(15, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8e34 = { 0, 1, 11, 12, 2, 14, PRINT_COLOR(1, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8dd4 = { 0, 16, 9, 14, 3, 1, PRINT_COLOR(15, 2, 0) };

static const FrameListCallbacks data_ov255_021d8e70 = {
    func_ov255_021d5ce4,
    func_ov255_021d5d8c,
    func_ov255_021d5d90,
};

static const BoxSearchWindowData data_ov255_021d8ec4 = { 4, 1, 1, 20, 2, 14, PRINT_COLOR(1, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8dc8 = { 0, 3, 9, 9, 3, 14, PRINT_COLOR(15, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8df8 = { 4, 1, 10, 30, 2, 14, PRINT_COLOR(1, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8e88 = { 0, 16, 0, 14, 3, 1, PRINT_COLOR(15, 2, 0) };

static const FrameListCallbacks data_ov255_021d8de0 = {
    func_ov255_021d5ff4,
    func_ov255_021d5d8c,
    func_ov255_021d5d90,
};

static const FrameListCallbacks data_ov255_021d8e04 = {
    func_ov255_021d5fe8,
    func_ov255_021d5d8c,
    func_ov255_021d5d90,
};

static const BoxSearchWindowData data_ov255_021d8e4c = { 0, 1, 8, 12, 2, 14, PRINT_COLOR(1, 2, 0) };

static const FrameListCallbacks data_ov255_021d8ef4 = {
    func_ov255_021d5da8,
    func_ov255_021d5ddc,
    func_ov255_021d5df4,
};

static const FrameListCallbacks data_ov255_021d8e64 = {
    func_ov255_021d5d38,
    func_ov255_021d5d8c,
    func_ov255_021d5d90,
};

// The string of each list's title
static const u8 data_ov255_021d8ee8[] = { 0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 2, 2 };

static const BoxSearchWindowData data_ov255_021d8edc = { 0, 3, 0, 9, 3, 14, PRINT_COLOR(15, 2, 0) };

static const FrameListCallbacks data_ov255_021d8ed0 = {
    func_ov255_021d5e24,
    func_ov255_021d5ec8,
    func_ov255_021d5f50,
};

static const BoxSearchWindowData data_ov255_021d8e28 = { 0, 3, 15, 9, 3, 14, PRINT_COLOR(15, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8eb8 = { 4, 1, 13, 30, 10, 14, PRINT_COLOR(1, 2, 0) };

static const BoxSearchWindowData data_ov255_021d8e1c = { 0, 16, 3, 14, 3, 1, PRINT_COLOR(15, 2, 0) };

static const FrameListCallbacks data_ov255_021d8ea0 = {
    func_ov255_021d5f68,
    func_ov255_021d5fa0,
    func_ov255_021d5fb8,
};

static const BoxSearchWindowData data_ov255_021d8e94 = { 0, 3, 3, 9, 3, 14, PRINT_COLOR(15, 2, 0) };

// The animations of the buttons that turn the search on and off
static const u32 data_ov255_021d8f00[][2] = {
    { 5, 4 },
    { 4, 5 },
};

// The messages of BoxSearchWork's strings
static const u16 data_ov255_021d8f10[BOX_SEARCH_STRING_COUNT - 1] = { 149, 150, 151, 152, 153, 154, 199, 114 };

// The main menu's cursor
static const CursorMoveCallbacks data_ov255_021d8f20 = {
    func_ov255_021d5abc,
    func_ov255_021d5b00,
    func_ov255_021d5b28,
    func_ov255_021d5b6c,
};

// How many groups of species each first letter has
static const u8 data_ov255_021d8f30[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

// The animations of the main menu's buttons, off and on
static const u8 data_ov255_021d8f4a[][3] = {
    { 6, 7, 8 },    { 6, 7, 8 },    { 6, 7, 8 },    { 6, 7, 8 },    { 6, 7, 8 },
    { 6, 7, 8 },    { 9, 10, 11 },  { 12, 13, 14 }, { 15, 16, 17 },
};

// The rows of a list of 7 items or fewer, and of a longer one with its scroll bar and arrows
static const FrameListTouch data_ov255_021d90d0[] = {
    { { 0, 23, 184, 231 }, 8 },    { { 24, 47, 184, 231 }, 8 },   { { 48, 71, 184, 231 }, 8 },
    { { 72, 95, 184, 231 }, 8 },   { { 96, 119, 184, 231 }, 8 },  { { 120, 143, 184, 231 }, 8 },
    { { 144, 167, 184, 231 }, 8 }, { { TOUCH_RECT_END, 0, 0, 0 }, 0 },
};

static const FrameListTouch data_ov255_021d9110[] = {
    { { 0, 23, 184, 231 }, 0 },    { { 24, 47, 184, 231 }, 0 },   { { 48, 71, 184, 231 }, 0 },
    { { 72, 95, 184, 231 }, 0 },   { { 96, 119, 184, 231 }, 0 },  { { 120, 143, 184, 231 }, 0 },
    { { 144, 167, 184, 231 }, 0 }, { { 0, 168, 232, 255 }, 1 },   { { 168, 191, 168, 191 }, 4 },
    { { 168, 191, 200, 223 }, 5 }, { { TOUCH_RECT_END, 0, 0, 0 }, 0 },
};

static const UIObjResSetup data_ov255_021d8f88 = { 0, 0, ARCID_BOX2, 93, 92, 90, 91, 3, 0, 6 };

// The rows of the lists
static const TouchRect data_ov255_021d8f65[] = {
    { 0, 23, 112, 183 },    { 24, 47, 112, 183 },   { 48, 71, 112, 183 },   { 72, 95, 112, 183 },
    { 96, 119, 112, 183 },  { 120, 143, 112, 183 }, { 144, 167, 112, 183 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const FrameListSetup data_ov255_021d8fa8 = {
    { 2, 255, 14, 0, 16, 3, 3, 0, 12, 3, 1, 24, 12, 8, 6, 4, 3, 5, 21, 0 },
    7,
    2,
    0,
    7,
    0,
    data_ov255_021d9110,
    NULL,
    NULL,
};

// Where each group of species starts and ends in the Pokédex's order by name
static const u16 data_ov255_021d8fd0[] = {
    0,   28,  62,  112, 147, 167, 184, 226, 249, 253, 260, 282, 315,
    372, 386, 392, 435, 438, 463, 557, 590, 595, 615, 636, 637, 640,
};

static const u16 data_ov255_021d9004[] = {
    28,  62,  112, 147, 167, 184, 226, 249, 253, 260, 282, 315, 372,
    386, 392, 435, 438, 463, 557, 590, 595, 615, 636, 637, 640, 649,
};

// Where each group of abilities starts and ends in data_ov255_021d9248, from 1
static const u8 data_ov255_021d9038[][2] = {
    { 1, 8 },     { 8, 12 },    { 12, 20 },   { 20, 27 },   { 27, 29 },   { 29, 38 },   { 38, 40 },
    { 40, 49 },   { 49, 60 },   { 60, 61 },   { 61, 63 },   { 63, 69 },   { 69, 82 },   { 82, 85 },
    { 85, 89 },   { 89, 98 },   { 98, 99 },   { 99, 107 },  { 107, 141 }, { 141, 152 }, { 152, 155 },
    { 155, 158 }, { 158, 164 }, { 0, 0 },     { 0, 0 },     { 164, 165 },
};

// The messages of the first letters
static const u16 data_ov255_021d906c[] = {
    199, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
    168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
};

const GameProcFunctions BOX_SEARCH_PROC_FUNCTIONS = {
    func_ov255_021d3b64,
    func_ov255_021d3d00,
    func_ov255_021d3c90,
};

// The first group of each first letter
static const u32 data_ov255_021d9168[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
};

// The main menu
static const CursorMoveData data_ov255_021d91d0[] = {
    { 0, 0, 0, 0, 7, 1, 0, 0, { 0, 23, 120, 255 } },
    { 0, 0, 0, 0, 0, 2, 1, 1, { 24, 47, 120, 255 } },
    { 0, 0, 0, 0, 1, 3, 2, 2, { 48, 71, 120, 255 } },
    { 0, 0, 0, 0, 2, 4, 3, 3, { 72, 95, 120, 255 } },
    { 0, 0, 0, 0, 3, 5, 4, 4, { 96, 119, 120, 255 } },
    { 0, 0, 0, 0, 4, 7, 5, 5, { 120, 143, 120, 255 } },
    { 0, 0, 0, 0, 5, 0, 6, 7, { 142, 165, 18, 77 } },
    { 0, 0, 0, 0, 5, 0, 6, 8, { 142, 165, 92, 163 } },
    { 0, 0, 0, 0, 5, 0, 7, 8, { 142, 165, 172, 243 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

// The abilities in the order of their names
static const u8 data_ov255_021d9248[] = {
    91,  106, 76,  148, 83,  107, 71,  123, 4,   145, 66,  34,  29,  13,  16,  14,  126, 130, 56,  6,   129,
    128, 88,  2,   70,  87,  48,  27,  111, 49,  138, 18,  122, 59,  108, 132, 119, 82,  62,  139, 131, 85,
    134, 118, 37,  55,  93,  52,  115, 35,  149, 17,  150, 151, 39,  15,  22,  160, 89,  154, 51,  103, 102,
    26,  135, 31,  7,   64,  156, 98,  40,  42,  63,  58,  104, 141, 78,  153, 136, 121, 152, 30,  99,  96,
    12,  142, 65,  20,  124, 53,  57,  90,  38,  143, 158, 46,  74,  95,  44,  155, 120, 144, 79,  69,  24,
    50,  159, 146, 45,  8,   157, 113, 32,  23,  61,  125, 75,  19,  86,  92,  112, 97,  81,  117, 94,  116,
    43,  3,   100, 9,   80,  1,   60,  114, 5,   21,  105, 68,  33,  28,  77,  101, 140, 164, 47,  110, 67,
    137, 36,  54,  163, 109, 84,  127, 162, 72,  10,  11,  41,  133, 73,  25,  147, 161,
};

static const BoxSearchSeqFunc data_ov255_021d92ec[] = {
    func_ov255_021d434c, func_ov255_021d4484, func_ov255_021d4570, func_ov255_021d4574, func_ov255_021d45d8,
    func_ov255_021d481c, NULL,                NULL,                NULL,                func_ov255_021d4844,
    func_ov255_021d48dc, func_ov255_021d4b5c, func_ov255_021d4b7c, func_ov255_021d4be0, func_ov255_021d4db8,
    func_ov255_021d4dc8, func_ov255_021d4e2c, func_ov255_021d5068, NULL,                NULL,
    NULL,                func_ov255_021d5090, func_ov255_021d5128, func_ov255_021d539c, func_ov255_021d53bc,
    func_ov255_021d541c, func_ov255_021d555c, func_ov255_021d556c, func_ov255_021d55cc, func_ov255_021d5700,
    func_ov255_021d5710, func_ov255_021d57a0, func_ov255_021d58dc, func_ov255_021d58ec, func_ov255_021d591c,
    func_ov255_021d594c, func_ov255_021d5980, func_ov255_021d5984, func_ov255_021d59b0, func_ov255_021d5a10,
    func_ov255_021d5a14, func_ov255_021d5a54,
};

static const BoxSearchActorData data_ov255_021d9394[BOX_SEARCH_ACTOR_COUNT] = {
    { 184, 12, 6, 1, TRUE, 0, 0 },    { 184, 36, 6, 1, TRUE, 0, 0 },    { 184, 60, 6, 1, TRUE, 0, 0 },
    { 184, 84, 6, 1, TRUE, 0, 0 },    { 184, 108, 6, 1, TRUE, 0, 0 },   { 184, 132, 6, 1, TRUE, 0, 0 },
    { 48, 154, 9, 1, TRUE, 0, 0 },    { 128, 154, 12, 1, TRUE, 0, 0 },  { 208, 154, 15, 1, TRUE, 0, 0 },
    { 108, 154, 4, 0, TRUE, 0, 0 },   { 188, 154, 5, 0, TRUE, 0, 0 },   { 64, 128, 5, 2, TRUE, 0, 0 },
    { 64, 128, 5, 2, TRUE, 0, 0 },    { 64, 128, 5, 2, TRUE, 0, 0 },    { 64, 128, 5, 2, TRUE, 0, 0 },
    { 64, 128, 5, 2, TRUE, 0, 0 },    { 64, 128, 5, 2, TRUE, 0, 0 },    { 64, 128, 5, 2, TRUE, 0, 0 },
    { 64, 128, 5, 2, TRUE, 0, 0 },    { 64, 128, 5, 2, TRUE, 0, 0 },    { 64, 128, 5, 2, TRUE, 0, 0 },
    { 244, 12, 0, 0, TRUE, 0, 0 },    { 136, 128, 0, 0, TRUE, 1, 0 },   { 152, 128, 2, 0, TRUE, 1, 0 },
    { 168, 128, 4, 0, TRUE, 1, 0 },   { 184, 128, 6, 0, TRUE, 1, 0 },   { 200, 128, 8, 0, TRUE, 1, 0 },
    { 216, 128, 10, 0, TRUE, 1, 0 },  { 176, 8, 0, 0, TRUE, 1, 0 },     { 176, 32, 2, 0, TRUE, 1, 0 },
    { 176, 56, 4, 0, TRUE, 1, 0 },    { 176, 80, 6, 0, TRUE, 1, 0 },    { 176, 104, 8, 0, TRUE, 1, 0 },
    { 176, 128, 10, 0, TRUE, 1, 0 },  { 168, 168, 4, 0, TRUE, 2, 0 },   { 200, 168, 5, 0, TRUE, 2, 0 },
};

// The sequence that each button of the main menu starts
static u32 data_ov255_021d9664[] = { 3, 12, 15, 24, 27, 30 };

static const BoxSearchWindowData *data_ov255_021d967c[BOX_SEARCH_WINDOW_COUNT] = {
    &data_ov255_021d8ec4, &data_ov255_021d8edc, &data_ov255_021d8e94, &data_ov255_021d8e10, &data_ov255_021d8dc8,
    &data_ov255_021d8dec, &data_ov255_021d8e28, &data_ov255_021d8e88, &data_ov255_021d8e1c, &data_ov255_021d8e7c,
    &data_ov255_021d8dd4, &data_ov255_021d8e40, &data_ov255_021d8e58, &data_ov255_021d8df8, &data_ov255_021d8eb8,
    &data_ov255_021d8e4c, &data_ov255_021d8e34,
};

static BOOL func_ov255_021d3b64(GameProc *proc, u32 *state, void *param, void *work) {
    BoxSearchParam *searchParam = param;
    BoxSearchWork *wk;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BOX_SEARCH, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BoxSearchWork), HEAPID_BOX_SEARCH);
    sys_memset(wk, 0, sizeof(BoxSearchWork));
    wk->param = searchParam;
    wk->heapId = HEAPID_BOX_SEARCH;
    wk->pokedex = GameData_GetPokedex(searchParam->syswk->param->gameData);
    wk->graphic = func_ov255_021d6d28(0, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_POKEPARAM_BOX_MAIN_SAVE_INIT, wk->heapId);
    wk->natureNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_NATURES, wk->heapId);
    wk->abilityNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_NAMES, wk->heapId);
    wk->abilityInfo = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_DESCRIPTIONS, wk->heapId);
    wk->speciesNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SPECIES_NAMES, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    func_ov255_021d3df0(wk->heapId);
    func_ov255_021d5bc4(wk);
    wk->touchBar = func_ov255_021d3e9c(wk, func_ov255_021d6e38(wk->graphic), wk->heapId);
    func_ov255_021d3f74(wk, func_ov255_021d6e38(wk->graphic), wk->heapId);
    func_ov255_021d3f00(wk);
    func_ov255_021d6150(wk);
    func_ov255_021d41c4(wk);
    func_ov255_021d434c(wk, 0);
    func_02042ba8(TRUE, wk->heapId);
    GFL_WipeSet(0, 1, 1, 0, 6, 1, wk->heapId);
    return TRUE;
}

static BOOL func_ov255_021d3c90(GameProc *proc, u32 *state, void *param, void *work) {
    BoxSearchWork *wk = work;
    HeapID heapId;

    func_ov255_021d5c0c(wk);
    func_ov255_021d40cc(wk);
    func_ov255_021d3f5c(wk);
    func_ov255_021d3eec(wk);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->speciesNames);
    GFL_MsgDataFree(wk->natureNames);
    GFL_MsgDataFree(wk->abilityNames);
    GFL_MsgDataFree(wk->abilityInfo);
    GFL_MsgDataFree(wk->msgData);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    func_ov255_021d6dc8(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL func_ov255_021d3d00(GameProc *proc, u32 *state, void *param, void *work) {
    BoxSearchWork *wk = work;
    u32 i;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished() == TRUE) {
            *state = 1;
            wk->seq = 1;
        }
        break;
    case 1:
        if (func_02021c0c(wk->printQueue)) {
            wk->seq = data_ov255_021d92ec[wk->seq](wk, wk->seq);
            if (wk->seq == SEQ_END) {
                *state = 2;
            }
            func_ov255_021d3ef8(wk->touchBar);
        }
        break;
    case 2:
        GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
        *state = 3;
        break;
    case 3:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    }
    func_02021a3c(wk->printQueue);
    for (i = 0; i < BOX_SEARCH_WINDOW_COUNT; i++) {
        PrintWindow_Flush(&wk->printWindows[i], wk->printQueue);
    }
    func_ov255_021d6e1c(wk->graphic);
    func_ov255_021d6e30(wk->graphic);
    func_ov255_021d6e34(wk->graphic);
    return FALSE;
}

// Loads the BGs' graphics
static void func_ov255_021d3df0(HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BOX2, heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 84, 0, 0, 0x80, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 86, 4, 0, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 85, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 82, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 89, 5, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 83, 3, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 14 * 0x20, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 14 * 0x20, 0x20, heapId);
}

// Creates the bar at the bottom with its return button
static TouchBar *func_ov255_021d3e9c(BoxSearchWork *wk, ClActUnit *unit, HeapID heapId) {
    TouchBarSetup setup;
    TouchBarItem item = { 1, { 232, 168 } };

    sys_memset(&setup, 0, sizeof(TouchBarSetup));
    setup.items = &item;
    setup.count = 1;
    setup.unit = unit;
    setup.bg = 1;
    setup.bgPalette = 13;
    setup.objPalette = 0;
    setup.vramType = 2;
    return TouchBar_Create(&setup, heapId);
}

static void func_ov255_021d3eec(BoxSearchWork *wk) {
    TouchBar_Free(wk->touchBar);
}

static void func_ov255_021d3ef8(TouchBar *bar) {
    TouchBar_Main(bar);
}

static void func_ov255_021d3f00(BoxSearchWork *wk) {
    u32 i;

    for (i = 0; i < BOX_SEARCH_WINDOW_COUNT; i++) {
        const BoxSearchWindowData *data = data_ov255_021d967c[i];

        wk->windows[i] =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
        wk->printWindows[i].window = wk->windows[i];
        wk->printWindows[i].flushPending = FALSE;
    }
}

static void func_ov255_021d3f5c(BoxSearchWork *wk) {
    u32 i;

    for (i = 0; i < BOX_SEARCH_WINDOW_COUNT; i++) {
        BmpWin_Free(wk->windows[i]);
    }
}

// Loads the OBJs' resources and creates the actors
static void func_ov255_021d3f74(BoxSearchWork *wk, ClActUnit *unit, HeapID heapId) {
    UIObjResSetup setup = data_ov255_021d8f88;
    int i;

    UIObjRes_Load(&wk->objRes[0], &setup, unit, heapId);
    setup.vramType = 0;
    setup.flags = 0;
    setup.arcId = getUINarcIdx();
    setup.paletteFile = func_0202d944();
    setup.charFile = func_0202d948(2);
    setup.cellFile = func_0202d94c(2);
    setup.animFile = func_0202d950(2);
    setup.paletteOffset = 9;
    setup.paletteStart = 0;
    setup.paletteCount = 1;
    UIObjRes_Load(&wk->objRes[1], &setup, unit, heapId);
    setup.paletteFile = func_0202d810();
    setup.charFile = func_0202d814();
    setup.cellFile = func_0202d818(2);
    setup.animFile = func_0202d81c(2);
    setup.paletteOffset = 10;
    setup.paletteStart = 0;
    setup.paletteCount = 3;
    UIObjRes_Load(&wk->objRes[2], &setup, unit, heapId);
    for (i = 0; i < BOX_SEARCH_ACTOR_COUNT; i++) {
        const BoxSearchActorData *data = &data_ov255_021d9394[i];

        wk->actors[i] = UIObjRes_CreateActor(&wk->objRes[data->res], unit, data_ov255_021d9394[i].x, data->y,
                                            data->anim, heapId);
        func_0204c468(wk->actors[i], data->bgPriority);
        func_0204c124(wk->actors[i], data->visible);
        func_0204c520(wk->actors[i], TRUE);
        func_0204c124(wk->actors[i], FALSE);
    }
    GFL_BGSysLoadNCLRDefault(ARCID_BOX2, 94, 1, 9 * 0x20, 0x20, heapId);
}

static void func_ov255_021d40cc(BoxSearchWork *wk) {
    int i;

    for (i = 0; i < 3; i++) {
        UIObjRes_Free(&wk->objRes[i]);
    }
}

// Prints the title of a list, or the explanation of the item under the cursor
static void func_ov255_021d40e8(BoxSearchWork *wk, int mode) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_INFO]), 0);
    switch (mode) {
    case 0:
        func_ov255_021d5c84(wk, WINDOW_INFO, 115, 1, 0);
        break;
    case 3:
        func_ov255_021d5c84(wk, WINDOW_INFO, 116, 1, 0);
        break;
    case 9:
        func_ov255_021d6308(wk, FrameList_GetValue(wk->list, 0));
        break;
    case 15:
        func_ov255_021d5c84(wk, WINDOW_INFO, 144, 1, 0);
        break;
    case 21:
        func_ov255_021d6250(wk, FrameList_GetValue(wk->list, 0));
        break;
    case 24:
        func_ov255_021d5c84(wk, WINDOW_INFO, 146, 1, 0);
        break;
    case 27:
        func_ov255_021d5c84(wk, WINDOW_INFO, 147, 1, 0);
        break;
    case 30:
        func_ov255_021d5c84(wk, WINDOW_INFO, 148, 1, 0);
        break;
    }
    func_ov255_021d41b0(wk->windows[WINDOW_INFO]);
}

static void func_ov255_021d41b0(BmpWin *window) {
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

// Prints the main menu
static void func_ov255_021d41c4(BoxSearchWork *wk) {
    u32 i;

    func_ov255_021d5c2c(wk, 0, 8, 0, 0);
    func_ov255_021d41b0(wk->windows[0]);
    func_ov255_021d5c2c(wk, 13, 7, 1, 0);
    func_ov255_021d41b0(wk->windows[13]);
    for (i = 0; i < 6; i++) {
        func_ov255_021d5c2c(wk, i + 1, i, 0, 4);
    }
}

static void func_ov255_021d4210(BoxSearchWork *wk, u32 index) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[15]), 0);
    func_ov255_021d5c2c(wk, 15, data_ov255_021d8ee8[index], 0, 0);
    func_ov255_021d41b0(wk->windows[15]);
}

static void func_ov255_021d4248(BoxSearchWork *wk, MsgData *msgData, u32 msgId) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[16]), 0);
    func_ov255_021d6038(wk, &wk->printWindows[16], wk->printQueue, msgData, msgId, PRINT_COLOR(15, 2, 0), 0);
    func_ov255_021d41b0(wk->windows[16]);
}

// Shows the main menu
static void func_ov255_021d4288(BoxSearchWork *wk) {
    u32 i;

    func_ov255_021d40e8(wk, 0);
    func_ov255_021d68fc(wk, 0);
    for (i = 0; i < 6; i++) {
        func_ov255_021d41b0(wk->windows[i + 1]);
    }
    func_ov255_021d6940(wk, TRUE);
    func_ov255_021d5ba4(wk);
    func_ov255_021d6ae8(wk, 22);
    func_ov255_021d6b1c(wk, 0);
    func_ov255_021d4320(wk, TRUE);
}

// Hides the main menu
static void func_ov255_021d42d4(BoxSearchWork *wk) {
    int i;

    func_ov255_021d4320(wk, FALSE);
    for (i = 6; i <= 10; i++) {
        func_0204c124(wk->actors[i], FALSE);
    }
    for (i = 1; i <= 6; i++) {
        BmpWin *window = wk->windows[i];

        BmpWin_ClearScreen(window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    }
    func_ov255_021d6940(wk, FALSE);
}

static void func_ov255_021d4320(BoxSearchWork *wk, BOOL visible) {
    u32 i;

    for (i = 0; i <= 10; i++) {
        func_0204c124(wk->actors[i], visible);
    }
    func_ov255_021d6b5c(wk, 22, visible);
}

// Starts the main menu
static int func_ov255_021d434c(BoxSearchWork *wk, int seq) {
    if (func_0203d554() == FALSE) {
        wk->cursorMove = CursorMove_Create(data_ov255_021d91d0, &data_ov255_021d8f20, wk, TRUE, wk->menuPos, wk->heapId);
        func_ov255_021d5abc(wk, wk->menuPos, wk->menuPos);
    } else {
        wk->cursorMove = CursorMove_Create(data_ov255_021d91d0, &data_ov255_021d8f20, wk, FALSE, wk->menuPos, wk->heapId);
    }
    CursorMove_SetHideOnTouch(wk->cursorMove);
    func_ov255_021d6798(wk);
    func_ov255_021d4288(wk);
    return 1;
}

// A button of the main menu was chosen
static int func_ov255_021d43cc(BoxSearchWork *wk, int seq, u32 pos) {
    switch (pos) {
    case 7:
        CursorMove_Delete(wk->cursorMove);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        seq = func_ov255_021d5a70(wk, 7, 14, 33);
        break;
    case 8:
        CursorMove_Delete(wk->cursorMove);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        seq = func_ov255_021d5a70(wk, 8, 17, 34);
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        wk->group = 0;
        CursorMove_Delete(wk->cursorMove);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        seq = func_ov255_021d5a70(wk, pos, 8, 41);
        break;
    case 6:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        seq = func_ov255_021d5a70(wk, 6, 11, 35);
        break;
    case 9:
        CursorMove_Delete(wk->cursorMove);
        seq = func_ov255_021d5a70(wk, -1, 0, SEQ_END);
        break;
    }
    wk->menuPos = pos;
    return seq;
}

// The main menu
static int func_ov255_021d4484(BoxSearchWork *wk, int seq) {
    u32 ret;

    if (func_ov255_021d6cd4(wk) == TRUE) {
        CursorMove_Delete(wk->cursorMove);
        return func_ov255_021d5a70(wk, -1, 0, SEQ_END);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        func_0203d564(FALSE);
        CursorMove_SetPos(wk->cursorMove, 6);
        CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021d5a70(wk, 6, 11, 35);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        func_0203d564(FALSE);
        CursorMove_SetPos(wk->cursorMove, 7);
        CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        CursorMove_Delete(wk->cursorMove);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021d5a70(wk, 7, 14, 33);
    }
    ret = CursorMove_Update(wk->cursorMove);
    switch (ret) {
    case -8:
    case -7:
    case -6:
    case -5:
    case -2:
    case -1:
        break;
    case -4:
    case -3:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    default:
        seq = func_ov255_021d43cc(wk, seq, ret);
        break;
    }
    return seq;
}

static int func_ov255_021d4570(BoxSearchWork *wk, int seq) {
    return seq;
}

// Opens the list of first letters of species
static int func_ov255_021d4574(BoxSearchWork *wk, int seq) {
    func_ov255_021d42d4(wk);
    func_ov255_021d63cc(wk, LIST_SPECIES_LETTER, wk->group);
    func_ov255_021d6bf0(wk, FALSE);
    func_ov255_021d40e8(wk, 3);
    func_ov255_021d68fc(wk, 1);
    func_ov255_021d4210(wk, 8);
    if (wk->param->syswk->search.species == 0) {
        func_ov255_021d4248(wk, wk->msgData, 199);
    } else {
        func_ov255_021d4248(wk, wk->speciesNames, wk->param->syswk->search.species);
    }
    func_ov255_021d6b80(wk);
    func_ov255_021d5a68(wk, 4);
    return SEQ_LIST_WAIT;
}

// The list of first letters of species
static int func_ov255_021d45d8(BoxSearchWork *wk, int seq) {
    int pos;

    switch (FrameList_Main(wk->list)) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        wk->group = FrameList_GetSelected(wk->list);
        if (wk->group == 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_ov255_021d6a48(wk, CRITERION_SPECIES, 0);
            func_ov255_021d6828(wk, 11, 0);
            func_ov255_021d4248(wk, wk->msgData, 199);
            return 5;
        }
        func_0204c124(wk->actors[11], FALSE);
        if (func_ov255_021d6214(wk, wk->group - 1, 0) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return 5;
        }
        GFL_SndSEPlay(SEQ_SE_BEEP);
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        func_ov255_021d6bf0(wk, FALSE);
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        if (wk->param->syswk->search.species == 0) {
            func_ov255_021d6894(wk, 11, 8, 0);
        } else {
            func_0204c124(wk->actors[11], FALSE);
        }
        break;
    case -7:
        return func_ov255_021d5a98(wk, 34, 12, 4);
    case -6:
        return func_ov255_021d5a98(wk, 35, 13, 4);
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_ov255_021d6a48(wk, CRITERION_SPECIES, 0);
            if (FrameList_GetScroll(wk->list) == 0) {
                func_ov255_021d6828(wk, 11, 0);
            }
            func_ov255_021d4248(wk, wk->msgData, 199);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            wk->group = 0;
            seq = func_ov255_021d5a70(wk, -1, 0, 5);
        } else {
            pos = func_ov255_021d6cf8(wk);
            if (pos >= 0 && pos <= 6) {
                wk->group = pos + FrameList_GetScroll(wk->list);
                if (wk->group == 0) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    func_ov255_021d6a48(wk, CRITERION_SPECIES, 0);
                    func_ov255_021d6828(wk, 11, pos);
                    func_ov255_021d4248(wk, wk->msgData, 199);
                    FrameList_SetCursor(wk->list, pos);
                    return 5;
                }
                func_0204c124(wk->actors[11], FALSE);
                if (func_ov255_021d6214(wk, wk->group - 1, 0) != 0) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    FrameList_SetCursor(wk->list, pos);
                    return 5;
                }
                FrameList_SetCursor(wk->list, pos);
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
        }
        break;
    }
    func_ov255_021d6bac(wk);
    return seq;
}

static int func_ov255_021d481c(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    if (wk->group != 0) {
        wk->subGroup = 0;
        return 9;
    }
    return 0;
}

// Opens the list of species of a first letter
static int func_ov255_021d4844(BoxSearchWork *wk, int seq) {
    u16 count;
    u16 i;

    func_ov255_021d67b0(wk);
    func_ov255_021d63cc(wk, LIST_SPECIES, 0);
    func_ov255_021d40e8(wk, 9);
    count = func_ov255_021d6214(wk, wk->group - 1, wk->subGroup);
    if (count <= 7) {
        func_ov255_021d68fc(wk, 2);
    } else {
        func_ov255_021d68fc(wk, 1);
        func_ov255_021d6bf0(wk, FALSE);
        func_ov255_021d6b80(wk);
        count = 7;
    }
    for (i = 0; i < count; i++) {
        if (wk->param->syswk->search.species == FrameList_GetValue(wk->list, i)) {
            func_ov255_021d6828(wk, 11, i);
            break;
        }
    }
    func_ov255_021d5a68(wk, 10);
    return SEQ_LIST_WAIT;
}

// The list of species of a first letter
static int func_ov255_021d48dc(BoxSearchWork *wk, int seq) {
    u32 ret;
    int i, count, scroll;

    ret = FrameList_Main(wk->list);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (wk->listCount > 7 || func_0203d554() == FALSE) {
            func_ov255_021d6a48(wk, CRITERION_SPECIES,
                                FrameList_GetValue(wk->list, FrameList_GetSelected(wk->list)));
            func_ov255_021d6828(wk, 11, ret);
            func_ov255_021d4248(wk, wk->speciesNames, wk->param->syswk->search.species);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            wk->chosen = TRUE;
            seq = 11;
        }
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        if (func_ov255_021d6214(wk, wk->group - 1, wk->subGroup) > 7) {
            func_ov255_021d6bf0(wk, FALSE);
        }
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        func_0204c124(wk->actors[11], FALSE);
        scroll = FrameList_GetScroll(wk->list);
        count = func_ov255_021d6214(wk, wk->group - 1, wk->subGroup);
        if (count > 8) {
            count = 8;
        }
        for (i = 0; i < count; i++) {
            if (wk->param->syswk->search.species == FrameList_GetValue(wk->list, scroll + i)) {
                func_ov255_021d6828(wk, 11, i);
                break;
            }
        }
        break;
    case -7:
        if (func_ov255_021d6214(wk, wk->group - 1, wk->subGroup) > 7) {
            return func_ov255_021d5a98(wk, 34, 12, 10);
        }
        break;
    case -6:
        if (func_ov255_021d6214(wk, wk->group - 1, wk->subGroup) > 7) {
            return func_ov255_021d5a98(wk, 35, 13, 10);
        }
        break;
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_ov255_021d6a48(wk, CRITERION_SPECIES, 0);
            func_0204c124(wk->actors[11], FALSE);
            func_ov255_021d4248(wk, wk->msgData, 199);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            wk->chosen = FALSE;
            seq = func_ov255_021d5a70(wk, -1, 0, 11);
        } else {
            i = func_ov255_021d6cf8(wk);
            if (i >= 0 && i <= 6 && i < wk->listRows) {
                func_ov255_021d6a48(wk, CRITERION_SPECIES,
                                    FrameList_GetValue(wk->list, i + FrameList_GetScroll(wk->list)));
                    func_ov255_021d6828(wk, 11, i);
                func_ov255_021d4248(wk, wk->speciesNames, wk->param->syswk->search.species);
                FrameList_SetCursor(wk->list, i);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                wk->chosen = TRUE;
                seq = 11;
            }
        }
        break;
    }
    if (func_ov255_021d6214(wk, wk->group - 1, wk->subGroup) > 7) {
        func_ov255_021d6bac(wk);
    }
    return seq;
}

static int func_ov255_021d4b5c(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    if (wk->chosen == FALSE) {
        return 3;
    }
    return 0;
}

// Opens the list of natures
static int func_ov255_021d4b7c(BoxSearchWork *wk, int seq) {
    func_ov255_021d42d4(wk);
    func_ov255_021d63cc(wk, LIST_NATURE, 0);
    func_ov255_021d6bf0(wk, FALSE);
    func_ov255_021d40e8(wk, 12);
    func_ov255_021d68fc(wk, 1);
    func_ov255_021d4210(wk, 1);
    if (wk->param->syswk->search.nature == 0) {
        func_ov255_021d4248(wk, wk->msgData, 199);
    } else {
        func_ov255_021d4248(wk, wk->natureNames, wk->param->syswk->search.nature - 1);
    }
    func_ov255_021d6b80(wk);
    func_ov255_021d5a68(wk, 13);
    return SEQ_LIST_WAIT;
}

// The list of natures
static int func_ov255_021d4be0(BoxSearchWork *wk, int seq) {
    u32 ret;
    int pos;

    ret = FrameList_Main(wk->list);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        func_ov255_021d6a48(wk, CRITERION_NATURE, FrameList_GetSelected(wk->list));
        func_ov255_021d6828(wk, 11, ret);
        if (wk->param->syswk->search.nature == 0) {
            func_ov255_021d4248(wk, wk->msgData, 199);
        } else {
            func_ov255_021d4248(wk, wk->natureNames, wk->param->syswk->search.nature - 1);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        seq = 14;
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        func_ov255_021d6bf0(wk, FALSE);
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        func_ov255_021d6894(wk, 11, 8, wk->param->syswk->search.nature);
        break;
    case -7:
        return func_ov255_021d5a98(wk, 34, 12, 13);
    case -6:
        return func_ov255_021d5a98(wk, 35, 13, 13);
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_ov255_021d6a48(wk, CRITERION_NATURE, 0);
            if (FrameList_GetScroll(wk->list) == 0) {
                func_ov255_021d6828(wk, 11, 0);
            } else {
                func_0204c124(wk->actors[11], FALSE);
            }
            func_ov255_021d4248(wk, wk->msgData, 199);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            seq = func_ov255_021d5a70(wk, -1, 0, 14);
        } else {
            pos = func_ov255_021d6cf8(wk);
            if (pos >= 0 && pos <= 6) {
                func_ov255_021d6a48(wk, CRITERION_NATURE, pos + FrameList_GetScroll(wk->list));
                func_ov255_021d6828(wk, 11, pos);
                if (wk->param->syswk->search.nature == 0) {
                    func_ov255_021d4248(wk, wk->msgData, 199);
                } else {
                    func_ov255_021d4248(wk, wk->natureNames, wk->param->syswk->search.nature - 1);
                }
                FrameList_SetCursor(wk->list, pos);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                seq = 14;
            }
        }
        break;
    }
    func_ov255_021d6bac(wk);
    return seq;
}

static int func_ov255_021d4db8(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    return 0;
}

// Opens the list of first letters of abilities
static int func_ov255_021d4dc8(BoxSearchWork *wk, int seq) {
    func_ov255_021d42d4(wk);
    func_ov255_021d63cc(wk, LIST_ABILITY_LETTER, wk->group);
    func_ov255_021d6bf0(wk, FALSE);
    func_ov255_021d40e8(wk, 15);
    func_ov255_021d68fc(wk, 1);
    func_ov255_021d4210(wk, 10);
    if (wk->param->syswk->search.ability == 0) {
        func_ov255_021d4248(wk, wk->msgData, 199);
    } else {
        func_ov255_021d4248(wk, wk->abilityNames, wk->param->syswk->search.ability);
    }
    func_ov255_021d6b80(wk);
    func_ov255_021d5a68(wk, 16);
    return SEQ_LIST_WAIT;
}

// The list of first letters of abilities
static int func_ov255_021d4e2c(BoxSearchWork *wk, int seq) {
    int pos;

    switch (FrameList_Main(wk->list)) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        wk->group = FrameList_GetSelected(wk->list);
        if (wk->group == 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_ov255_021d6a48(wk, CRITERION_ABILITY, 0);
            func_ov255_021d6828(wk, 11, 0);
            func_ov255_021d4248(wk, wk->msgData, 199);
            return 17;
        }
        func_0204c124(wk->actors[11], FALSE);
        if (func_ov255_021d622c(wk->group - 1, 0) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return 17;
        }
        GFL_SndSEPlay(SEQ_SE_BEEP);
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        func_ov255_021d6bf0(wk, FALSE);
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        if (wk->param->syswk->search.ability == 0) {
            func_ov255_021d6894(wk, 11, 8, 0);
        } else {
            func_0204c124(wk->actors[11], FALSE);
        }
        break;
    case -7:
        return func_ov255_021d5a98(wk, 34, 12, 16);
    case -6:
        return func_ov255_021d5a98(wk, 35, 13, 16);
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_ov255_021d6a48(wk, CRITERION_ABILITY, 0);
            if (FrameList_GetScroll(wk->list) == 0) {
                func_ov255_021d6828(wk, 11, 0);
            }
            func_ov255_021d4248(wk, wk->msgData, 199);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            wk->group = 0;
            seq = func_ov255_021d5a70(wk, -1, 0, 17);
        } else {
            pos = func_ov255_021d6cf8(wk);
            if (pos >= 0 && pos <= 6) {
                wk->group = pos + FrameList_GetScroll(wk->list);
                if (wk->group == 0) {
                    func_ov255_021d6a48(wk, CRITERION_ABILITY, 0);
                    func_ov255_021d6828(wk, 11, pos);
                    func_ov255_021d4248(wk, wk->msgData, 199);
                    FrameList_SetCursor(wk->list, pos);
                    return 17;
                }
                func_0204c124(wk->actors[11], FALSE);
                if (func_ov255_021d622c(wk->group - 1, 0) != 0) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    FrameList_SetCursor(wk->list, pos);
                    return 17;
                }
                FrameList_SetCursor(wk->list, pos);
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
        }
        break;
    }
    func_ov255_021d6bac(wk);
    return seq;
}

static int func_ov255_021d5068(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    if (wk->group != 0) {
        wk->subGroup = 0;
        return 21;
    }
    return 0;
}

// Opens the list of abilities of a first letter
static int func_ov255_021d5090(BoxSearchWork *wk, int seq) {
    u16 count;
    u16 i;

    func_ov255_021d67b0(wk);
    func_ov255_021d63cc(wk, LIST_ABILITY, 0);
    func_ov255_021d40e8(wk, 21);
    count = func_ov255_021d622c(wk->group - 1, wk->subGroup);
    if (count <= 7) {
        func_ov255_021d68fc(wk, 2);
    } else {
        func_ov255_021d68fc(wk, 1);
        func_ov255_021d6bf0(wk, FALSE);
        func_ov255_021d6b80(wk);
        count = 7;
    }
    for (i = 0; i < count; i++) {
        if (wk->param->syswk->search.ability == FrameList_GetValue(wk->list, i)) {
            func_ov255_021d6828(wk, 11, i);
            break;
        }
    }
    func_ov255_021d5a68(wk, 22);
    return SEQ_LIST_WAIT;
}

// The list of abilities of a first letter
static int func_ov255_021d5128(BoxSearchWork *wk, int seq) {
    u32 ret;
    int i, count, scroll;

    ret = FrameList_Main(wk->list);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (wk->listCount > 7 || func_0203d554() == FALSE) {
            func_ov255_021d6a48(wk, CRITERION_ABILITY,
                                FrameList_GetValue(wk->list, FrameList_GetSelected(wk->list)));
            func_ov255_021d6828(wk, 11, ret);
            func_ov255_021d4248(wk, wk->abilityNames, wk->param->syswk->search.ability);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            wk->chosen = TRUE;
            seq = 23;
        }
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        if (func_ov255_021d622c(wk->group - 1, wk->subGroup) > 7) {
            func_ov255_021d6bf0(wk, FALSE);
        }
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        func_0204c124(wk->actors[11], FALSE);
        scroll = FrameList_GetScroll(wk->list);
        count = func_ov255_021d622c(wk->group - 1, wk->subGroup);
        if (count > 8) {
            count = 8;
        }
        for (i = 0; i < count; i++) {
            if (wk->param->syswk->search.ability == FrameList_GetValue(wk->list, scroll + i)) {
                func_ov255_021d6828(wk, 11, i);
                break;
            }
        }
        break;
    case -7:
        if (func_ov255_021d622c(wk->group - 1, wk->subGroup) > 7) {
            return func_ov255_021d5a98(wk, 34, 12, 22);
        }
        break;
    case -6:
        if (func_ov255_021d622c(wk->group - 1, wk->subGroup) > 7) {
            return func_ov255_021d5a98(wk, 35, 13, 22);
        }
        break;
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_ov255_021d6a48(wk, CRITERION_ABILITY, 0);
            func_0204c124(wk->actors[11], FALSE);
            func_ov255_021d4248(wk, wk->msgData, 199);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            wk->chosen = FALSE;
            seq = func_ov255_021d5a70(wk, -1, 0, 23);
        } else {
            i = func_ov255_021d6cf8(wk);
            if (i >= 0 && i <= 6 && i < wk->listRows) {
                func_ov255_021d6a48(wk, CRITERION_ABILITY,
                                    FrameList_GetValue(wk->list, i + FrameList_GetScroll(wk->list)));
                func_ov255_021d6828(wk, 11, i);
                func_ov255_021d4248(wk, wk->abilityNames, wk->param->syswk->search.ability);
                FrameList_SetCursor(wk->list, i);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                wk->chosen = TRUE;
                seq = 23;
            }
        }
        break;
    }
    if (func_ov255_021d622c(wk->group - 1, wk->subGroup) > 7) {
        func_ov255_021d6bac(wk);
    }
    return seq;
}

static int func_ov255_021d539c(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    if (wk->chosen == FALSE) {
        return 15;
    }
    return 0;
}

// Opens the list of sexes
static int func_ov255_021d53bc(BoxSearchWork *wk, int seq) {
    func_ov255_021d42d4(wk);
    func_ov255_021d63cc(wk, LIST_SEX, 0);
    func_ov255_021d40e8(wk, 24);
    func_ov255_021d68fc(wk, 2);
    func_ov255_021d4210(wk, 3);
    func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dc0[wk->param->syswk->search.sex]);
    func_ov255_021d6828(wk, 11, wk->param->syswk->search.sex);
    func_ov255_021d5a68(wk, 25);
    return SEQ_LIST_WAIT;
}

// The list of sexes
static int func_ov255_021d541c(BoxSearchWork *wk, int seq) {
    u32 ret;
    int pos;

    ret = FrameList_Main(wk->list);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (func_0203d554() == FALSE) {
            func_ov255_021d6a48(wk, CRITERION_SEX, ret);
            func_ov255_021d6828(wk, 11, ret);
            func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dc0[wk->param->syswk->search.sex]);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            seq = 26;
        }
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        break;
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_ov255_021d6a48(wk, CRITERION_SEX, 0);
            func_ov255_021d6828(wk, 11, 0);
            func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dc0[wk->param->syswk->search.sex]);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            seq = func_ov255_021d5a70(wk, -1, 0, 26);
        } else {
            pos = func_ov255_021d6cf8(wk);
            if (pos >= 0 && pos <= 3) {
                func_ov255_021d6a48(wk, CRITERION_SEX, pos);
                func_ov255_021d6828(wk, 11, pos);
                func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dc0[wk->param->syswk->search.sex]);
                FrameList_SetCursor(wk->list, pos);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                seq = 26;
            }
        }
        break;
    }
    return seq;
}

static int func_ov255_021d555c(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    return 0;
}

// Opens the list of held item choices
static int func_ov255_021d556c(BoxSearchWork *wk, int seq) {
    func_ov255_021d42d4(wk);
    func_ov255_021d63cc(wk, LIST_ITEM, 0);
    func_ov255_021d40e8(wk, 27);
    func_ov255_021d68fc(wk, 2);
    func_ov255_021d4210(wk, 4);
    func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dba[wk->param->syswk->search.item]);
    func_ov255_021d6828(wk, 11, wk->param->syswk->search.item);
    func_ov255_021d5a68(wk, 28);
    return SEQ_LIST_WAIT;
}

// The list of held item choices
static int func_ov255_021d55cc(BoxSearchWork *wk, int seq) {
    u32 ret;
    int pos;

    ret = FrameList_Main(wk->list);
    switch (ret) {
    case 0:
    case 1:
    case 2:
        if (func_0203d554() == FALSE) {
            func_ov255_021d6a48(wk, CRITERION_ITEM, ret);
            func_ov255_021d6828(wk, 11, ret);
            func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dba[wk->param->syswk->search.item]);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            seq = 29;
        }
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        break;
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_ov255_021d6a48(wk, CRITERION_ITEM, 0);
            func_ov255_021d6828(wk, 11, 0);
            func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dba[wk->param->syswk->search.item]);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            seq = func_ov255_021d5a70(wk, -1, 0, 29);
        } else {
            pos = func_ov255_021d6cf8(wk);
            if (pos >= 0 && pos <= 2) {
                func_ov255_021d6a48(wk, CRITERION_ITEM, pos);
                func_ov255_021d6828(wk, 11, pos);
                func_ov255_021d4248(wk, wk->msgData, data_ov255_021d8dba[wk->param->syswk->search.item]);
                FrameList_SetCursor(wk->list, pos);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                seq = 29;
            }
        }
        break;
    }
    return seq;
}

static int func_ov255_021d5700(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    return 0;
}

// Opens the list of markings
static int func_ov255_021d5710(BoxSearchWork *wk, int seq) {
    int i;

    func_ov255_021d42d4(wk);
    func_ov255_021d63cc(wk, LIST_MARKING, 0);
    func_ov255_021d40e8(wk, 30);
    func_ov255_021d68fc(wk, 2);
    func_ov255_021d4210(wk, 5);
    for (i = 0; i < 6; i++) {
        func_ov255_021d6828(wk, i + 11, i);
        func_0204c124(wk->actors[i + 11], func_ov255_021d6aac(wk, i));
    }
    func_ov255_021d6ae8(wk, 22);
    func_ov255_021d6ae8(wk, 28);
    func_ov255_021d6b1c(wk, 1);
    func_ov255_021d6b5c(wk, 22, TRUE);
    func_ov255_021d6b5c(wk, 28, TRUE);
    func_ov255_021d5a68(wk, 31);
    return SEQ_LIST_WAIT;
}

// The list of markings, which turns each on and off
static int func_ov255_021d57a0(BoxSearchWork *wk, int seq) {
    u32 ret, i;
    int pos;
    u32 on;

    ret = FrameList_Main(wk->list);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (func_0203d554() != FALSE) {
            break;
        }
        on = func_ov255_021d6aac(wk, ret) ^ 1;
        func_ov255_021d6a88(wk, ret, on);
        func_0204c124(wk->actors[ret + 11], on);
        func_ov255_021d6ae8(wk, 22);
        func_ov255_021d6ae8(wk, 28);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        break;
    case -3:
    case -2:
        FrameList_SetCursor(wk->list, FrameList_GetCursor(wk->list));
        break;
    case -1:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            for (i = 0; i < 6; i++) {
                func_ov255_021d6a88(wk, i, FALSE);
                func_0204c124(wk->actors[i + 11], FALSE);
            }
            func_ov255_021d6ae8(wk, 22);
            func_ov255_021d6ae8(wk, 28);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (func_ov255_021d6cd4(wk) == TRUE) {
            seq = func_ov255_021d5a70(wk, -1, 0, 26);
        } else {
            pos = func_ov255_021d6cf8(wk);
            if (pos >= 0 && pos <= 5) {
                on = func_ov255_021d6aac(wk, pos) ^ 1;
                func_ov255_021d6a88(wk, pos, on);
                func_0204c124(wk->actors[pos + 11], on);
                FrameList_SetCursor(wk->list, pos);
                func_ov255_021d6ae8(wk, 22);
                func_ov255_021d6ae8(wk, 28);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
        }
        break;
    }
    return seq;
}

static int func_ov255_021d58dc(BoxSearchWork *wk, int seq) {
    FrameList_Free(wk->list);
    return 0;
}

// Turns the search on
static int func_ov255_021d58ec(BoxSearchWork *wk, int seq) {
    func_ov255_021d5b70(wk, 7);
    func_ov255_021d6a48(wk, CRITERION_ACTIVE, TRUE);
    if (CursorMove_IsCursorVisible(wk->cursorMove) == TRUE) {
        func_0204c488(wk->actors[7], 13);
    }
    return SEQ_END;
}

// Turns the search off
static int func_ov255_021d591c(BoxSearchWork *wk, int seq) {
    func_ov255_021d5b70(wk, 8);
    func_ov255_021d6a48(wk, CRITERION_ACTIVE, FALSE);
    if (CursorMove_IsCursorVisible(wk->cursorMove) == TRUE) {
        func_0204c488(wk->actors[8], 16);
    }
    return SEQ_END;
}

// Clears the criteria
static int func_ov255_021d594c(BoxSearchWork *wk, int seq) {
    func_ov255_021d6c94(wk);
    func_ov255_021d6940(wk, TRUE);
    func_ov255_021d6ae8(wk, 22);
    if (CursorMove_IsCursorVisible(wk->cursorMove) == TRUE) {
        func_0204c488(wk->actors[6], 10);
    }
    return 1;
}

static int func_ov255_021d5980(BoxSearchWork *wk, int seq) {
    return seq;
}

// Waits for a list to be drawn
static int func_ov255_021d5984(BoxSearchWork *wk, int seq) {
    if (FrameList_Draw(wk->list) == FALSE) {
        if (func_0203d554() == FALSE) {
            FrameList_ShowCursor(wk->list);
        }
        seq = wk->nextSeq;
    }
    return seq;
}

// Waits for a list's arrow to be animated
static int func_ov255_021d59b0(BoxSearchWork *wk, int seq) {
    switch (wk->btnAnmSeq) {
    case 0:
        if (FrameList_Main(wk->list) == FRAMELIST_NONE) {
            wk->btnAnmSeq++;
        }
        func_ov255_021d6bac(wk);
        break;
    case 1:
        if (func_0204c560(wk->actors[wk->btnActor]) == FALSE) {
            wk->btnAnmSeq = 0;
            func_ov255_021d6bf0(wk, FALSE);
            return wk->nextSeq;
        }
        break;
    }
    return SEQ_BUTTON_ANM;
}

static int func_ov255_021d5a10(BoxSearchWork *wk, int seq) {
    return seq;
}

// Waits for a button to be animated
static int func_ov255_021d5a14(BoxSearchWork *wk, int seq) {
    if (wk->btnActor == -1) {
        if (TouchBar_GetDecided(wk->touchBar) == TRUE) {
            return wk->nextSeq;
        }
    } else if (func_0204c560(wk->actors[wk->btnActor]) == FALSE) {
        return wk->nextSeq;
    }
    return seq;
}

static int func_ov255_021d5a54(BoxSearchWork *wk, int seq) {
    return data_ov255_021d9664[wk->menuPos];
}

static void func_ov255_021d5a68(BoxSearchWork *wk, int seq) {
    wk->nextSeq = seq;
}

// Animates a button, then goes on to next
static int func_ov255_021d5a70(BoxSearchWork *wk, int actor, u32 anim, int next) {
    if (actor != -1) {
        func_ov255_021d6ac0(wk, actor, anim);
    }
    wk->btnActor = actor;
    func_ov255_021d5a68(wk, next);
    return SEQ_TOUCH_BAR_ANM;
}

static int func_ov255_021d5a98(BoxSearchWork *wk, int actor, u32 anim, int next) {
    func_ov255_021d6ac0(wk, actor, anim);
    wk->btnActor = actor;
    wk->btnAnmSeq = 0;
    func_ov255_021d5a68(wk, next);
    return SEQ_BUTTON_ANM;
}

// The main menu's cursor shows on a button
static void func_ov255_021d5abc(void *work, int pos, int prevPos) {
    BoxSearchWork *wk = work;
    int i;

    for (i = 0; i <= 8; i++) {
        if (pos == i) {
            func_0204c488(wk->actors[i], data_ov255_021d8f4a[i][1]);
        } else {
            func_0204c488(wk->actors[i], data_ov255_021d8f4a[i][0]);
        }
    }
}

static void func_ov255_021d5b00(void *work, int pos, int prevPos) {
    BoxSearchWork *wk = work;
    int i;

    for (i = 0; i <= 8; i++) {
        func_0204c488(wk->actors[i], data_ov255_021d8f4a[i][0]);
    }
}

static void func_ov255_021d5b28(void *work, int pos, int prevPos) {
    BoxSearchWork *wk = work;
    int i;

    for (i = 0; i <= 8; i++) {
        if (pos == i) {
            func_0204c488(wk->actors[i], data_ov255_021d8f4a[i][1]);
        } else {
            func_0204c488(wk->actors[i], data_ov255_021d8f4a[i][0]);
        }
    }
}

static void func_ov255_021d5b6c(void *work, int pos, int prevPos) {
}

static void func_ov255_021d5b70(BoxSearchWork *wk, u32 type) {
    func_0204c488(wk->actors[9], data_ov255_021d8f00[type - 7][0]);
    func_0204c488(wk->actors[10], data_ov255_021d8f00[type - 7][1]);
}

static void func_ov255_021d5ba4(BoxSearchWork *wk) {
    if (wk->param->syswk->search.active) {
        func_ov255_021d5b70(wk, 7);
    } else {
        func_ov255_021d5b70(wk, 8);
    }
}

static void func_ov255_021d5bc4(BoxSearchWork *wk) {
    int i;

    for (i = 0; i < BOX_SEARCH_STRING_COUNT - 1; i++) {
        wk->strings[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, data_ov255_021d8f10[i]);
    }
    if (wk->param->param->mode == 2) {
        wk->strings[8] = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparamBoxMainSaveInit_Text_OrganizeBoxEs);
    } else {
        wk->strings[8] = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparamBoxMainSaveInit_Text_BattleBox_2);
    }
    wk->strbuf = GFL_StrBufCreate(128, wk->heapId);
}

static void func_ov255_021d5c0c(BoxSearchWork *wk) {
    int i;

    GFL_StrBufFree(wk->strbuf);
    for (i = 0; i < BOX_SEARCH_STRING_COUNT; i++) {
        GFL_StrBufFree(wk->strings[i]);
    }
}

static void func_ov255_021d5c2c(BoxSearchWork *wk, u32 win, u32 string, int x, int y) {
    PrintWindow_Print(&wk->printWindows[win], wk->printQueue, x, y, wk->strings[string], wk->font,
                      data_ov255_021d967c[win]->color);
}

static void func_ov255_021d5c84(BoxSearchWork *wk, u32 win, u32 msgId, int x, int y) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    PrintWindow_Print(&wk->printWindows[win], wk->printQueue, x, y, str, wk->font, data_ov255_021d967c[win]->color);
    GFL_StrBufFree(str);
}

// Prints a first letter of abilities, grayed out when no ability starts with it
static void func_ov255_021d5ce4(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;

    if (index == 0 || func_ov255_021d622c(index - 1, 0) != 0) {
        func_ov255_021d60b4(wk, index, window, wk->msgData);
    } else {
        func_ov255_021d60ec(wk, index, window, wk->msgData);
    }
    if (index == 0 && wk->param->syswk->search.ability == 0) {
        func_ov255_021d6804(wk, 11, y + 12);
    }
}

// Prints a first letter of species, grayed out when no species caught starts with it
static void func_ov255_021d5d38(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;

    if (index == 0 || func_ov255_021d6214(wk, index - 1, 0) != 0) {
        func_ov255_021d60b4(wk, index, window, wk->msgData);
    } else {
        func_ov255_021d60ec(wk, index, window, wk->msgData);
    }
    if (index == 0 && wk->param->syswk->search.species == 0) {
        func_ov255_021d6804(wk, 11, y + 12);
    }
}

static void func_ov255_021d5d8c(void *work, u32 index, BOOL moved) {
}

static void func_ov255_021d5d90(void *work, s16 delta) {
    BoxSearchWork *wk = work;

    func_ov255_021d683c(wk, -delta);
    func_ov255_021d6bf0(wk, TRUE);
}

static void func_ov255_021d5da8(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;

    func_ov255_021d60b4(wk, index, window, wk->speciesNames);
    if (wk->param->syswk->search.species == FrameList_GetValue(wk->list, index)) {
        func_ov255_021d6804(wk, 11, y + 12);
    }
}

static void func_ov255_021d5ddc(void *work, u32 index, BOOL moved) {
    BoxSearchWork *wk = work;

    func_ov255_021d6308(wk, FrameList_GetValue(wk->list, index));
}

static void func_ov255_021d5df4(void *work, s16 delta) {
    BoxSearchWork *wk = work;

    if (func_ov255_021d6214(wk, wk->group - 1, wk->subGroup) > 7) {
        func_ov255_021d683c(wk, -delta);
        func_ov255_021d6bf0(wk, TRUE);
    }
}

static void func_ov255_021d5e24(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;
    int width, windowWidth;
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);
    u32 nature = FrameList_GetValue(wk->list, index);
    StrBuf *str;

    if (nature == 0) {
        str = wk->strings[6];
    } else {
        str = GFL_MsgDataLoadStrbufNew(wk->natureNames, nature - 1);
    }
    width = GFL_FontGetBlockWidth(str, wk->font, 0);
    windowWidth = BmpWin_GetSizeX(window->window) * 8;
    PrintWindow_Print(window, queue, (windowWidth - width) / 2, 4, str, wk->font, PRINT_COLOR(15, 14, 0));
    if (nature != 0) {
        GFL_StrBufFree(str);
    }
    if (wk->param->syswk->search.nature == nature) {
        func_ov255_021d6804(wk, 11, y + 12);
    }
}

static void func_ov255_021d5ec8(void *work, u32 index, BOOL moved) {
    BoxSearchWork *wk = work;
    StrBuf *str;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_INFO]), 0);
    if (index != 0) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, index + 118);
        PrintWindow_Print(&wk->printWindows[WINDOW_INFO], wk->printQueue, 1, 0, str, wk->font,
                          data_ov255_021d967c[WINDOW_INFO]->color);
        GFL_StrBufFree(str);
        func_ov255_021d41b0(wk->windows[WINDOW_INFO]);
    } else {
        BmpWin_Transfer(wk->windows[WINDOW_INFO]);
    }
}

static void func_ov255_021d5f50(void *work, s16 delta) {
    BoxSearchWork *wk = work;

    func_ov255_021d683c(wk, -delta);
    func_ov255_021d6bf0(wk, TRUE);
}

static void func_ov255_021d5f68(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;

    func_ov255_021d60b4(wk, index, window, wk->abilityNames);
    if (wk->param->syswk->search.ability == FrameList_GetValue(wk->list, index)) {
        func_ov255_021d6804(wk, 11, y + 12);
    }
}

static void func_ov255_021d5fa0(void *work, u32 index, BOOL moved) {
    BoxSearchWork *wk = work;

    func_ov255_021d6250(wk, FrameList_GetValue(wk->list, index));
}

static void func_ov255_021d5fb8(void *work, s16 delta) {
    BoxSearchWork *wk = work;

    if (func_ov255_021d622c(wk->group - 1, wk->subGroup) > 7) {
        func_ov255_021d683c(wk, -delta);
        func_ov255_021d6bf0(wk, TRUE);
    }
}

static void func_ov255_021d5fe8(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;

    func_ov255_021d60b4(wk, index, window, wk->msgData);
}

static void func_ov255_021d5ff4(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;

    func_ov255_021d60b4(wk, index, window, wk->msgData);
}

static void func_ov255_021d6000(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    BoxSearchWork *wk = work;
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);

    func_02021c7c(queue, BmpWin_GetBitmap(window->window), 0, 4, wk->strings[6], wk->font, 0);
    window->flushPending = TRUE;
}

// Prints a message centered in a list's row
static void func_ov255_021d6038(BoxSearchWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData, u32 msgId,
                                u16 color, int y) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    int width = GFL_FontGetBlockWidth(str, wk->font, 0);
    int windowWidth = BmpWin_GetSizeX(window->window) * 8;

    PrintWindow_Print(window, queue, (windowWidth - width) / 2, y, str, wk->font, color);
    GFL_StrBufFree(str);
}

static void func_ov255_021d6098(BoxSearchWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                u32 msgId) {
    func_ov255_021d6038(wk, window, queue, msgData, msgId, PRINT_COLOR(15, 14, 0), 4);
}

static void func_ov255_021d60b4(BoxSearchWork *wk, u32 index, PrintWindow *window, MsgData *msgData) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);

    func_ov255_021d6098(wk, window, queue, msgData, FrameList_GetValue(wk->list, index));
}

// Prints an item grayed out
static void func_ov255_021d60ec(BoxSearchWork *wk, u32 index, PrintWindow *window, MsgData *msgData) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);

    func_ov255_021d6038(wk, window, queue, msgData, FrameList_GetValue(wk->list, index), PRINT_COLOR(13, 12, 0), 4);
}

// How many species a group has
static int func_ov255_021d612c(int letter, int group) {
    return data_ov255_021d9004[data_ov255_021d9168[letter] + group] -
           data_ov255_021d8fd0[data_ov255_021d9168[letter] + group];
}

// Counts the species caught of each group
static void func_ov255_021d6150(BoxSearchWork *wk) {
    u32 letter, i;
    u32 len;
    u16 *species;
    u32 group;

    sys_memset(wk->caughtCounts, 0, sizeof(wk->caughtCounts));
    species = func_ov255_021d68d8(wk->heapId, &len);
    for (letter = 0; letter < 26; letter++) {
        for (group = 0; group < data_ov255_021d8f30[letter]; group++) {
            u32 count = func_ov255_021d612c(letter, group);
            u16 start = data_ov255_021d8fd0[data_ov255_021d9168[letter] + group];
            u32 *caught = &wk->caughtCounts[group + data_ov255_021d9168[letter]];

            *caught = 0;
            for (i = 0; i < count; i++) {
                if (PokeDex_IsCaught(wk->pokedex, species[start + i])) {
                    (*caught)++;
                }
            }
        }
    }
    GFL_HeapFree(species);
}

static int func_ov255_021d6214(BoxSearchWork *wk, int letter, int group) {
    return wk->caughtCounts[data_ov255_021d9168[letter] + group];
}

// How many abilities a group has
static int func_ov255_021d622c(int letter, int group) {
    return data_ov255_021d9038[data_ov255_021d9168[letter] + group][1] -
           data_ov255_021d9038[data_ov255_021d9168[letter] + group][0];
}

// Prints an ability's name and explanation
static void func_ov255_021d6250(BoxSearchWork *wk, u32 ability) {
    StrBuf *fmt;
    StrBuf *str;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_INFO]), 0);
    fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparamBoxMainSaveInit_Text_PleaseChooseAbilityAbility);
    loadAbilityNameToStrbuf(wk->wordSet, 0, ability);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, fmt);
    PrintWindow_Print(&wk->printWindows[WINDOW_INFO], wk->printQueue, 1, 0, wk->strbuf, wk->font,
                      data_ov255_021d967c[WINDOW_INFO]->color);
    GFL_StrBufFree(fmt);
    str = GFL_MsgDataLoadStrbufNew(wk->abilityInfo, ability);
    PrintWindow_Print(&wk->printWindows[WINDOW_INFO], wk->printQueue, 1, 48, str, wk->font,
                      data_ov255_021d967c[WINDOW_INFO]->color);
    GFL_StrBufFree(str);
    func_ov255_021d41b0(wk->windows[WINDOW_INFO]);
}

// Prints a species' name and types
static void func_ov255_021d6308(BoxSearchWork *wk, u32 species) {
    void *personal = PML_PersonalLoad(species, 0, wk->heapId);
    u32 type1 = PML_PersonalGetParam(personal, PERSONAL_TYPE_1);
    u32 type2 = PML_PersonalGetParam(personal, PERSONAL_TYPE_2);
    StrBuf *fmt;

    PML_PersonalFree(personal);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_INFO]), 0);
    if (type1 != type2) {
        fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparamBoxMainSaveInit_Text_PleaseChoosePokemonName);
        loadTypeTextToStrbuf(wk->wordSet, 2, type2);
    } else {
        fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparamBoxMainSaveInit_Text_PleaseChoosePokemonName_2);
    }
    WordSet_LoadSpeciesName(wk->wordSet, 0, (u16)species);
    loadTypeTextToStrbuf(wk->wordSet, 1, type1);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, fmt);
    PrintWindow_Print(&wk->printWindows[WINDOW_INFO], wk->printQueue, 1, 0, wk->strbuf, wk->font,
                      data_ov255_021d967c[WINDOW_INFO]->color);
    GFL_StrBufFree(fmt);
    func_ov255_021d41b0(wk->windows[WINDOW_INFO]);
}

// Creates a list, with the cursor on pos
static void func_ov255_021d63cc(BoxSearchWork *wk, u32 mode, int pos) {
    FrameListSetup setup = data_ov255_021d8fa8;
    u32 len;
    ArcTool *arc;
    int i;
    u16 start;
    int first;
    u16 *species;
    int j;

    setup.work = wk;
    arc = GFL_ArcSysCreateFileHandle(ARCID_BOX2, wk->heapId);
    wk->listRows = 7;
    switch (mode) {
    case LIST_SPECIES_LETTER:
        setup.count = 27;
        setup.callbacks = &data_ov255_021d8e64;
        break;
    case LIST_SPECIES:
        setup.count = func_ov255_021d6214(wk, wk->group - 1, wk->subGroup);
        setup.callbacks = &data_ov255_021d8ef4;
        if (setup.count < wk->listRows) {
            wk->listRows = setup.count;
        }
        break;
    case LIST_NATURE:
        setup.callbacks = &data_ov255_021d8ed0;
        setup.count = 26;
        break;
    case LIST_ABILITY_LETTER:
        setup.callbacks = &data_ov255_021d8e70;
        setup.count = 27;
        break;
    case LIST_ABILITY:
        setup.callbacks = &data_ov255_021d8ea0;
        setup.count = func_ov255_021d622c(wk->group - 1, wk->subGroup);
        if (setup.count < wk->listRows) {
            wk->listRows = setup.count;
        }
        break;
    case LIST_SEX:
        setup.callbacks = &data_ov255_021d8e04;
        setup.count = 4;
        wk->listRows = 4;
        break;
    case LIST_ITEM:
        setup.callbacks = &data_ov255_021d8de0;
        setup.count = 3;
        wk->listRows = 3;
        break;
    case LIST_MARKING:
        setup.callbacks = &data_ov255_021d8eac;
        setup.count = 6;
        wk->listRows = 6;
        break;
    case LIST_LETTER:
    case LIST_LETTER_2:
        break;
    }
    if (setup.count <= 7) {
        setup.touch = data_ov255_021d90d0;
    }
    if (pos >= wk->listRows) {
        setup.scroll = pos - (wk->listRows - 1);
        setup.cursorPos = pos - setup.scroll;
    } else {
        setup.scroll = 0;
        setup.cursorPos = pos;
    }
    wk->list = FrameList_Create(&setup, wk->heapId);
    wk->listCount = setup.count;
    FrameList_LoadScreen(wk->list, arc, 88, FALSE, 0);
    FrameList_LoadScreen(wk->list, arc, 87, FALSE, 1);
    FrameList_LoadCursorPalette(wk->list, arc, 84, 1, 5);
    GFL_ArcToolFree(arc);
    switch (mode) {
    case LIST_SPECIES_LETTER:
        FrameList_AddItem(wk->list, 0, 199);
        for (j = 1; j < setup.count; j++) {
            FrameList_AddItem(wk->list, 1, data_ov255_021d906c[j]);
        }
        break;
    case LIST_LETTER:
        for (i = 0; i < setup.count; i++) {
            FrameList_AddItem(wk->list, 1, i + data_ov255_021d906c[wk->group]);
        }
        break;
    case LIST_SPECIES:
        start = data_ov255_021d8fd0[data_ov255_021d9168[wk->group - 1] + wk->subGroup];
        species = func_ov255_021d68d8(wk->heapId, &len);
        for (i = 0; i < func_ov255_021d612c(wk->group - 1, wk->subGroup); i++) {
            if (PokeDex_IsCaught(wk->pokedex, species[start + i])) {
                FrameList_AddItem(wk->list, 0, species[start + i]);
            }
        }
        GFL_HeapFree(species);
        break;
    case LIST_NATURE:
        for (i = 0; i < setup.count; i++) {
            FrameList_AddItem(wk->list, 0, i);
        }
        break;
    case LIST_ABILITY_LETTER:
        FrameList_AddItem(wk->list, 0, 199);
        for (j = 1; j < setup.count; j++) {
            FrameList_AddItem(wk->list, 1, data_ov255_021d906c[j]);
        }
        break;
    case LIST_LETTER_2:
        for (i = 0; i < setup.count; i++) {
            FrameList_AddItem(wk->list, 1, i + data_ov255_021d906c[wk->group]);
        }
        break;
    case LIST_ABILITY:
        first = data_ov255_021d9038[data_ov255_021d9168[wk->group - 1] + wk->subGroup][0] - 1;
        for (i = 0; i < setup.count; i++) {
            FrameList_AddItem(wk->list, 0, data_ov255_021d9248[first + i]);
        }
        break;
    case LIST_SEX:
        for (i = 0; i < setup.count; i++) {
            FrameList_AddItem(wk->list, 0, data_ov255_021d8dc0[i]);
        }
        break;
    case LIST_ITEM:
        for (i = 0; i < setup.count; i++) {
            FrameList_AddItem(wk->list, 0, data_ov255_021d8dba[i]);
        }
        break;
    case LIST_MARKING:
        for (i = 0; i < setup.count; i++) {
            FrameList_AddItem(wk->list, 0, 0);
        }
        break;
    }
}

static void func_ov255_021d6798(BoxSearchWork *wk) {
    GFL_BGSysClearScr(0);
    GFL_BGSysQueueScrLoad(0);
    func_ov255_021d67b0(wk);
}

// Hides the list's cursor, markings and arrows
static void func_ov255_021d67b0(BoxSearchWork *wk) {
    u32 i;

    GFL_BGSysClearScr(2);
    GFL_BGSysQueueScrLoad(2);
    for (i = 11; i <= 21; i++) {
        func_0204c124(wk->actors[i], FALSE);
    }
    func_ov255_021d6b5c(wk, 22, FALSE);
    func_ov255_021d6b5c(wk, 28, FALSE);
    func_0204c124(wk->actors[34], FALSE);
    func_0204c124(wk->actors[35], FALSE);
}

static void func_ov255_021d6804(BoxSearchWork *wk, u32 actor, s16 y) {
    ClActorPos pos;

    pos.x = 127;
    pos.y = y;
    func_0204c210(wk->actors[actor], &pos);
    func_0204c124(wk->actors[actor], TRUE);
}

static void func_ov255_021d6828(BoxSearchWork *wk, u32 actor, int row) {
    func_ov255_021d6804(wk, actor, row * 24 + 12);
}

// Scrolls the list's cursor and markings, hiding them off the list
static void func_ov255_021d683c(BoxSearchWork *wk, s8 delta) {
    ClActorPos pos;
    u32 i;

    for (i = 11; i <= 20; i++) {
        if (func_0204c138(wk->actors[i])) {
            func_0204c21c(wk->actors[i], &pos);
            pos.y += delta;
            func_0204c210(wk->actors[i], &pos);
            if (pos.y <= -12 || pos.y >= 204) {
                func_0204c124(wk->actors[i], FALSE);
            }
        }
    }
}

// Puts the cursor on the row of value, if it shows
static void func_ov255_021d6894(BoxSearchWork *wk, u32 actor, int count, int value) {
    int scroll = FrameList_GetScroll(wk->list);
    int i;

    for (i = 0; i < count; i++) {
        if (value == scroll + i) {
            func_ov255_021d6828(wk, actor, i);
            return;
        }
    }
    func_0204c124(wk->actors[actor], FALSE);
}

// Loads the species in the order of their names
static u16 *func_ov255_021d68d8(u32 heapId, u32 *count) {
    u32 size;
    u16 *species = GFL_ArcSysReadHeapNewLZGetLen(ARCID_ZUKAN_DATA, 5, FALSE, heapId, &size);

    *count = size / 2;
    return species;
}

static void func_ov255_021d68fc(BoxSearchWork *wk, u32 screen) {
    NNSG2dScreenData *scr;
    void *buf = GFL_G2DIOReadNSCR(ARCID_BOX2, data_ov255_021d8db4[screen], FALSE, &scr, wk->heapId);

    GFL_BGSysLoadScrAreaAll(3, scr->rawData, 0, 0, 32, 24);
    GFL_HeapFree(buf);
    GFL_BGSysQueueScrLoad(3);
}

// Prints the criteria of the main menu, or clears them
static void func_ov255_021d6940(BoxSearchWork *wk, BOOL show) {
    Box2SearchParam *search = &wk->param->syswk->search;
    int i;

    if (show == FALSE) {
        for (i = 7; i <= 11; i++) {
            BmpWin_ClearScreen(wk->windows[i]);
        }
    } else {
        for (i = 7; i <= 11; i++) {
            GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
        }
        if (search->species == 0) {
            func_ov255_021d6098(wk, &wk->printWindows[7], wk->printQueue, wk->msgData, 199);
        } else {
            func_ov255_021d6098(wk, &wk->printWindows[7], wk->printQueue, wk->speciesNames, search->species);
        }
        if (search->nature == 0) {
            func_ov255_021d6098(wk, &wk->printWindows[8], wk->printQueue, wk->msgData, 199);
        } else {
            func_ov255_021d6098(wk, &wk->printWindows[8], wk->printQueue, wk->natureNames, search->nature - 1);
        }
        if (search->ability == 0) {
            func_ov255_021d6098(wk, &wk->printWindows[9], wk->printQueue, wk->msgData, 199);
        } else {
            func_ov255_021d6098(wk, &wk->printWindows[9], wk->printQueue, wk->abilityNames, search->ability);
        }
        func_ov255_021d6098(wk, &wk->printWindows[10], wk->printQueue, wk->msgData,
                            data_ov255_021d8dc0[search->sex]);
        func_ov255_021d6098(wk, &wk->printWindows[11], wk->printQueue, wk->msgData,
                            data_ov255_021d8dba[search->item]);
        for (i = 7; i <= 11; i++) {
            BmpWin_FlushMap(wk->windows[i]);
        }
    }
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(wk->windows[7]));
}

static void func_ov255_021d6a48(BoxSearchWork *wk, u32 criterion, u32 value) {
    Box2SearchParam *search = &wk->param->syswk->search;

    switch (criterion) {
    case CRITERION_SPECIES:
        search->species = value;
        break;
    case CRITERION_ITEM:
        search->item = value;
        break;
    case CRITERION_NATURE:
        search->nature = value;
        break;
    case CRITERION_ABILITY:
        search->ability = value;
        break;
    case CRITERION_SEX:
        search->sex = value;
        break;
    case CRITERION_MARKS:
        search->marks = value;
        break;
    case CRITERION_ACTIVE:
        search->active = value;
        break;
    }
}

static void func_ov255_021d6a88(BoxSearchWork *wk, u32 mark, u32 on) {
    Box2SearchParam *search = &wk->param->syswk->search;

    search->marks = (u8)(search->marks & (0xff ^ (1 << mark))) | (on << mark);
}

static u32 func_ov255_021d6aac(BoxSearchWork *wk, u32 mark) {
    return (wk->param->syswk->search.marks >> mark) & 1;
}

static void func_ov255_021d6ac0(BoxSearchWork *wk, u32 actor, u32 anim) {
    func_0204c4d4(wk->actors[actor], 0);
    func_0204c488(wk->actors[actor], anim);
    func_0204c520(wk->actors[actor], TRUE);
}

// Shows each marking on or off
static void func_ov255_021d6ae8(BoxSearchWork *wk, u32 actor) {
    int i;

    for (i = 0; i < 6; i++) {
        u32 on = func_ov255_021d6aac(wk, i);

        func_0204c488(wk->actors[actor + i], on + i * 2);
    }
}

static void func_ov255_021d6b1c(BoxSearchWork *wk, u32 layout) {
    ClActorPos pos;
    u32 i;

    if (layout == 0) {
        pos.x = 136;
        pos.y = 128;
    } else {
        pos.x = 12;
        pos.y = 92;
    }
    for (i = 22; i <= 27; i++) {
        func_0204c140(wk->actors[i], &pos, 0);
        pos.x += 16;
    }
}

static void func_ov255_021d6b5c(BoxSearchWork *wk, u32 actor, BOOL visible) {
    int i;

    for (i = 0; i < 6; i++) {
        func_0204c124(wk->actors[actor + i], visible);
    }
}

// Shows the scroll bar
static void func_ov255_021d6b80(BoxSearchWork *wk) {
    ClActorPos pos = data_ov255_021d8db0;

    func_0204c140(wk->actors[21], &pos, 0);
    func_0204c124(wk->actors[21], TRUE);
}

// Moves the scroll bar
static void func_ov255_021d6bac(BoxSearchWork *wk) {
    ClActorPos pos;

    func_0204c21c(wk->actors[21], &pos);
    pos.y = FrameList_ClampBarPos(wk->list, pos.y);
    if (pos.y < 12) {
        pos.y = 12;
    } else if (pos.y > 156) {
        pos.y = 156;
    }
    func_0204c210(wk->actors[21], &pos);
}

// Shows the list's arrows, grayed out at its ends
static void func_ov255_021d6bf0(BoxSearchWork *wk, BOOL keep) {
    int pos = FrameList_GetCursor(wk->list);

    if (keep == FALSE) {
        func_0204c124(wk->actors[34], TRUE);
        func_0204c124(wk->actors[35], TRUE);
    }
    if (func_0204c4a0(wk->actors[34]) != 12 || keep == FALSE) {
        if (FrameList_GetScroll(wk->list) == 0 && pos == 0) {
            func_ov255_021d6ac0(wk, 34, 18);
        } else {
            func_ov255_021d6ac0(wk, 34, 4);
        }
    }
    if (func_0204c4a0(wk->actors[35]) != 13 || keep == FALSE) {
        if (FrameList_CanScrollDown(wk->list) == FALSE && pos == wk->listRows - 1) {
            func_ov255_021d6ac0(wk, 35, 19);
        } else {
            func_ov255_021d6ac0(wk, 35, 5);
        }
    }
}

static void func_ov255_021d6c94(BoxSearchWork *wk) {
    func_ov255_021d6a48(wk, CRITERION_SPECIES, 0);
    func_ov255_021d6a48(wk, CRITERION_ITEM, 0);
    func_ov255_021d6a48(wk, CRITERION_NATURE, 0);
    func_ov255_021d6a48(wk, CRITERION_ABILITY, 0);
    func_ov255_021d6a48(wk, CRITERION_SEX, 0);
    func_ov255_021d6a48(wk, CRITERION_MARKS, 0);
}

// Whether the return button or B was pressed
static BOOL func_ov255_021d6cd4(BoxSearchWork *wk) {
    if (TouchBar_GetTouched(wk->touchBar) == TRUE) {
        return TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        return TRUE;
    }
    return FALSE;
}

// The row of the list touched, or TOUCH_RECT_NONE
static int func_ov255_021d6cf8(BoxSearchWork *wk) {
    int pos = func_0203da0c(data_ov255_021d8f65);

    if (pos >= 0 && pos < wk->listRows && func_0203d554() == FALSE) {
        func_0203d564(TRUE);
    }
    return pos;
}
