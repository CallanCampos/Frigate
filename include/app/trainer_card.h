#ifndef POKEBW2_APP_TRAINER_CARD_H
#define POKEBW2_APP_TRAINER_CARD_H

#include "types.h"
#include "field/trcard_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nnsys/g2d.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/wordset.h"

// Overlay 186's trainer card screen: trainer_card.c (the proc, its input and the signature), trcard_bmp.c (the
// windows and what is printed in them) and trcard_obj.c (the cell actors). The ROM names none of the three; the names
// are guesses after the Trainer Card's files of the earlier games

// The heap of the trainer card screen and of the badge case
#define HEAPID_TRAINER_CARD_SCREEN 0x27

// a/0/3/9, the screens' graphics, and a/0/4/0, the trainer pictures of the union room appearances
#define ARCID_TRAINER_CARD_GRA 39
#define ARCID_TRAINER_CARD_UNION_GRA 40

// The Y button's shortcuts of the card's front, its back and the badge case
#define SHORTCUT_TRAINER_CARD_FRONT 24
#define SHORTCUT_TRAINER_CARD_BACK 25
#define SHORTCUT_TRAINER_CARD_BADGES 26

// The signature: 192x64 pixels, one bit each in the save and one byte each while it is edited
#define TRCARD_SIGN_WIDTH 192
#define TRCARD_SIGN_HEIGHT 64
#define TRCARD_SIGN_PIXELS (TRCARD_SIGN_WIDTH * TRCARD_SIGN_HEIGHT)
#define TRCARD_SIGN_BYTES (TRCARD_SIGN_PIXELS / 8)

#define TRCARD_WIN_COUNT 8
#define TRCARD_ACTOR_COUNT 14
// The rows of the back's list, of which five show at a time
#define TRCARD_ROW_MAX 24
#define TRCARD_ROW_HEIGHT 16

// The actors
enum {
    TRCARD_ACTOR_RETURN,
    TRCARD_ACTOR_EXIT,
    TRCARD_ACTOR_BADGES,
    TRCARD_ACTOR_3,
    TRCARD_ACTOR_ZOOM,
    TRCARD_ACTOR_BRUSH,
    TRCARD_ACTOR_FLIP,
    TRCARD_ACTOR_SHORTCUT,
    // Five that depend on the side shown, from sTrainerCardSideActors
    TRCARD_ACTOR_SIDE,
    TRCARD_ACTOR_SCROLL_UP = 11,
    TRCARD_ACTOR_SCROLL_DOWN,
    TRCARD_ACTOR_RANK,
};

// The windows
enum {
    TRCARD_WIN_NAME,
    TRCARD_WIN_UNION,
    TRCARD_WIN_PERSONALITY,
    TRCARD_WIN_POKEDEX,
    TRCARD_WIN_MONEY,
    TRCARD_WIN_GREETING,
    TRCARD_WIN_LIST,
    TRCARD_WIN_MESSAGE,
};

// Where the pen is on the signature
typedef struct {
    u8 x;
    u8 y;
    // 0 the pen, 1 the eraser
    u8 brush;
    u8 on;
} TrainerCardPen;

typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} TrainerCardObjRes;

typedef struct {
    u16 heapId;
    ClActUnit *unit;
    u8 unk08[0x90];
    ClActor *actors[TRCARD_ACTOR_COUNT];
    // The card's own, and the touch bar's common ones
    TrainerCardObjRes res[2];
} TrainerCardObjWork;

typedef struct {
    u16 heapId;
    BmpWin *win[TRCARD_WIN_COUNT];
    TrainerCardParam *param;
    // The card shown, and the player's own
    TrainerCardData *data;
    TrainerCardData *ownData;
    u8 signPixels[TRCARD_SIGN_PIXELS];
    void *charRes;
    NNSG2dCharacterData *charData;
    // 0 the front, 1 the back
    int side;
    u8 unk303C[4];
    // The card's affine BGs: the scale while it flips or zooms, and its center
    fx32 scaleX;
    fx32 scaleY;
    u8 unk3048[4];
    s32 flipAngle;
    int centerX;
    int centerY;
    int offsetX;
    int offsetY;
    // Which half of the zoomed signature shows, and whether it is zoomed
    u16 zoomSide;
    u16 zoomed;
    BOOL touchMode;
    u8 unk3068[0xc];
    TrainerCardObjWork obj;
    u8 unk315C[0x84];
    u32 unk31E0;
    u8 unk31E4[0x10];
    void *scrRes;
    NNSG2dScreenData *scrData;
    u8 unk31FC[8];
    void *palRes;
    NNSG2dPaletteData *palData;
    int palFrame;
    TCB *vblankTask;
    TCBExManager *tcbl;
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *numBuf;
    StrBuf *colonBuf;
    StrBuf *nameBuf;
    StrBuf *tmpBuf;
    StrBuf *unk3234[3];
    StrBuf *unk3240[2];
    StrBuf *labels[27];
    StrBuf *formats[5];
    // What the Hall of Fame row shows before the first entry
    StrBuf *noClearBuf;
    u8 unk32CC[8];
    PMSDraw *pmsDraw;
    u8 badges[16];
    u8 unk32E8;
    u8 unk32E9;
    u8 subSeq;
    // The eraser button's blinking while the eraser is chosen
    u8 blinkCount;
    u8 blinkMode;
    u8 unk32ED;
    u8 isCleared : 1;
    u8 isOnline : 1;
    u8 affineDirty : 1;
    PrintQueue *printQueue;
    u8 unk32F4[4];
    u8 frameType;
    u8 msgSpeed;
    u8 frame;
    u32 touchX;
    u32 touchY;
    u8 unk3304[2];
    // The back's list: how far it is scrolled (0 or less), where it was last frame, and where the touch that drags it
    // started
    s16 scroll;
    s16 oldScroll;
    s16 dragY;
    s16 dragScroll;
    TrainerCardPen pen;
    TrainerCardPen penNew;
    TrainerCardPen penOld;
    u8 brush;
    u8 scrolled;
    u8 unk331C;
    u8 seWait;
    u8 rows[TRCARD_ROW_MAX];
    u8 rowCount;
    u8 scrollTouch;
    u8 penTouch;
    // Bit 0: the card has a rank; bit 1: its palette is animated
    u32 rankFlags;
    int rank;
    int rankAngle;
    u16 rankPalette[2][16];
    u16 rankPaletteOut[16];
} TrainerCardWork;

// trainer_card.c
BOOL TrainerCard_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL TrainerCard_Main(GameProc *proc, u32 *state, void *param, void *work);
BOOL TrainerCard_Exit(GameProc *proc, u32 *state, void *param, void *work);

// trcard_bmp.c
void TrCardBmp_Init(TrainerCardWork *wk);
void TrCardBmp_Exit(TrainerCardWork *wk);
void TrCardBmp_DrawFront(TrainerCardWork *wk, BmpWin **win, const TrainerCardData *data);
void TrCardBmp_DrawUnionView(TrainerCardWork *wk, u32 view, BOOL flush);
void TrCardBmp_DrawPersonality(TrainerCardWork *wk, u32 personality, BOOL flush);
void TrCardBmp_Flush(TrainerCardWork *wk, BmpWin **win);
void TrCardBmp_DrawBack(TrainerCardWork *wk, BmpWin **win, TrainerCardData *data);
void TrCardBmp_InitRows(TrainerCardWork *wk, TrainerCardData *shown, TrainerCardData *own);
void TrCardBmp_DrawList(TrainerCardWork *wk, int scroll, BOOL flush, int color);
void TrCardBmp_Stub(TrainerCardWork *wk, BmpWin *win, u32 a2, StrBuf *strbuf);

// trcard_obj.c
void TrCardObj_Init(TrainerCardObjWork *obj, const BGSysVRAMConfig *vram);
void TrCardObj_AddActors(TrainerCardObjWork *obj);
void TrCardObj_Exit(TrainerCardObjWork *obj);
void TrCardObj_SetAnime(TrainerCardObjWork *obj, int actor, u8 sequence, u8 visible);
void TrCardObj_SetAnimePalette(TrainerCardObjWork *obj, int actor, u8 sequence, u8 visible, u8 palette);
void TrCardObj_SetVisible(TrainerCardObjWork *obj, u8 actor, BOOL visible);

#endif // POKEBW2_APP_TRAINER_CARD_H
