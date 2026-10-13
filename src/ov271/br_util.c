// The Battle Recorder's utilities: message windows, lists of strings, the profile of a video's player, the sequences
// its screens step through, and the ball effect that circles a touch. The name is the ROM's string, from
// GFL_HeapAllocate's asserts

#include "types.h"
#include "app/battle_recorder/br_util.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "pml/poke_party.h"
#include "system/pms_data.h"
#include "system/pms_draw.h"
#include "system/wordset.h"

struct BrMsgWin {
    PrintWindow print;
    PrintQueue *que;
    BmpWin *bmpwin;
    StrBuf *str;
    u16 clear_color;
    u16 frame;
    u32 pos_type;
    BrPoint pos;
};

#define BR_LIST_HITTBL_MAX 10

typedef BOOL (*BrListMoveFunc)(BrList *p_wk, s32 dir);
typedef BOOL (*BrListIsMoveFunc)(BrList *p_wk);
typedef void (*BrListDecideFunc)(BrList *p_wk);

struct BrList {
    BrListParam param;
    // The scroll arrows, then the sidebar's arrows
    ClActor *clwk[4];
    BmpWin *bmpwin;
    u16 unk3C;
    u16 list_max;
    u16 cursor;
    u16 list;
    u16 line_max;
    u32 select;
    s8 slide;
    u8 is_slide;
    BrListMoveFunc move_func;
    BrListIsMoveFunc ismove_func;
    BrListDecideFunc decide_func;
    TouchRect hittbl[BR_LIST_HITTBL_MAX];
    BOOL is_hold;
    u16 hold_y;
    GFLBitmap *bmp[];
};

#define BR_PROFILE_MSGWIN_MAX 7

struct BrProfile {
    BrMsgWin *msgwin[BR_PROFILE_MSGWIN_MAX];
    BrRes *res;
    PMSDraw *pms;
    ClActor *pokeIcon;
    ClActor *trainer;
    u32 pokeIconPlt;
    u32 pokeIconChr;
    u32 pokeIconCel;
    u32 trainerPlt;
    u32 trainerChr;
    u32 trainerCel;
};

// A profile message window and the message it shows
typedef struct {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    u32 msgID;
} BrProfileMsgWinData;

struct BrSeq {
    BrSeqFunc seq_function;
    u32 seq;
    void *p_wk_adrs;
    u32 unkC;
};

// The motions of a ball: around a circle as its radius changes, around a circle, along a line, or after another
// point
typedef struct {
    BrPoint center;
    BrPoint now;
    u16 r_start;
    s16 r_end;
    u16 rot_start;
    u16 rot_end;
    s32 sync_max;
} BrBallCircle;

typedef struct {
    BrPoint center;
    BrPoint now;
    u16 r;
    u16 rot;
    s32 sync_max;
} BrBallRot;

typedef struct {
    BrPoint start;
    BrPoint end;
    BrPoint now;
    s32 sync_start;
    s32 sync_max;
} BrBallLine;

typedef struct {
    const BrPoint *target;
    fx32 x;
    fx32 y;
    fx32 z;
    fx32 speed;
} BrBallFollow;

typedef union {
    BrBallCircle circle;
    BrBallRot rot;
    BrBallLine line;
    BrBallFollow follow;
} BrBallMove;

#define BR_BALL_NUM 12

struct BrBallEffect {
    BrPoint pos;
    const BrPoint *p_follow;
    u32 display;
    BrSeq *seq;
    BrRes *res;
    BrBallMove move[BR_BALL_NUM];
    BrPoint ball[BR_BALL_NUM];
    ClActor *clwk[BR_BALL_NUM];
    s32 cnt;
    BOOL is_end;
    u32 unk230;
    BOOL is_move_end[BR_BALL_NUM];
};

static void BrMsgWin_GetPrintPos(u32 type, const BrPoint *cp_pos, GFLBitmap *bmp, const StrBuf *str, Font *font,
                                 BrPoint *p_pos);
static void BrList_WriteCore(BrList *p_wk, u16 list);
static s32 BrList_GetSlide(BrList *p_wk);
static void BrList_BlitBmp(GFLBitmap *src, GFLBitmap *dst, u16 src_x, u16 src_y, u16 dst_x, u16 dst_y, u16 w, u16 h);
static BOOL BrList_Move_Touch(BrList *p_wk, s32 dir);
static BOOL BrList_Move_Cursor(BrList *p_wk, s32 dir);
static BOOL BrList_IsMove_Touch(BrList *p_wk);
static BOOL BrList_IsMove_Cursor(BrList *p_wk);
static void BrList_Decide_Touch(BrList *p_wk);
static void BrList_Decide_Cursor(BrList *p_wk);
static void BrBallEff_Seq_Wait(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_Spread(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_Line(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_LineRot(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_Rot(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_RotSmall(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_Stop(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_Touch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallEff_Seq_Ring(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBallCircle_Init(BrBallCircle *p_wk, const BrPoint *cp_center, u16 r_start, u16 r_end, u16 rot_start,
                              u16 rot_end, s32 sync_max);
static BOOL BrBallCircle_Main(BrBallCircle *p_wk, BrPoint *p_now, s32 sync);
static void BrBallRot_Init(BrBallRot *p_wk, const BrPoint *cp_center, u16 r, u16 rot, s32 sync_max);
static BOOL BrBallRot_Main(BrBallRot *p_wk, BrPoint *p_now, s32 sync);
static void BrBallLine_Init(BrBallLine *p_wk, const BrPoint *cp_start, const BrPoint *cp_end, s32 sync_start,
                            s32 sync_max);
static BOOL BrBallLine_Main(BrBallLine *p_wk, BrPoint *p_now, s32 sync);
static void BrBallFollow_Init(BrBallFollow *p_wk, const BrPoint *cp_target, const BrPoint *cp_start, fx32 speed);
static BOOL BrBallFollow_Main(BrBallFollow *p_wk, BrPoint *p_now);

// Where the balls of the effects start and end. Four objects, declared in an order that MWCC's size sort lays out
// as the ROM has them: top, center, above, below
static const BrPoint sc_ball_pos_center = { 128, 96 };
static const BrPoint sc_ball_pos_above = { 128, -16 };
static const BrPoint sc_ball_pos_top = { 128, 16 };
static const BrPoint sc_ball_pos_below = { 128, 216 };

// The scroll areas at the sides of a list
static const TouchRect sc_slide_rect[] = {
    { 8, 128, 8, 32 },
    { 8, 128, 224, 248 },
    { TOUCH_RECT_END, TOUCH_RECT_END, TOUCH_RECT_END, TOUCH_RECT_END },
};

// The palette of each trainer appearance of the Union Room
static const u8 sc_union_plt[16] = { 2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0 };

// Where the cursor's balls circle, for each row of a cursor list
static const BrPoint sc_cursor_pos[] = {
    { 24, 76 }, { 24, 100 }, { 24, 124 }, { 24, 148 }, { 24, 172 },
};

static const BrProfileMsgWinData sc_profile_msgwin[BR_PROFILE_MSGWIN_MAX] = {
    { 7, 4, 18, 2, 14 },  { 17, 8, 11, 4, 16 }, { 4, 11, 12, 2, 17 }, { 3, 13, 25, 2, 23 },
    { 3, 15, 25, 2, 24 }, { 4, 17, 11, 2, 18 }, { 3, 19, 26, 4, 0 },
};

// Shows the scroll arrows of the directions the list can still scroll in
static inline void BrList_UpdateArrow(BrList *p_wk) {
    if (p_wk->is_slide) {
        if (p_wk->list == 0) {
            func_0204c124(p_wk->clwk[0], FALSE);
        } else {
            func_0204c124(p_wk->clwk[0], TRUE);
        }
        if (p_wk->list + p_wk->line_max == p_wk->param.list_max) {
            func_0204c124(p_wk->clwk[1], FALSE);
        } else {
            func_0204c124(p_wk->clwk[1], TRUE);
        }
    }
}

BrMsgWin *BrMsgWin_Init(u16 frame, u8 x, u8 y, u8 w, u8 h, u8 plt, PrintQueue *que, HeapID heapId) {
    BrMsgWin *p_wk = GFL_HeapAllocate(heapId, sizeof(BrMsgWin), FALSE, "br_util.c", 95);

    sys_memset(p_wk, 0, sizeof(BrMsgWin));
    p_wk->clear_color = 0;
    p_wk->que = que;
    p_wk->frame = frame;
    p_wk->str = GFL_StrBufCreate(255, heapId);
    p_wk->bmpwin = BmpWin_CreateDynamic(frame, x, y, w, h, plt, TRUE);
    PrintWindow_Init(&p_wk->print, p_wk->bmpwin);
    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->bmpwin), p_wk->clear_color);
    BmpWin_TransferNow(p_wk->bmpwin);
    return p_wk;
}

void BrMsgWin_Exit(BrMsgWin *p_wk) {
    BmpWin_ClearScreen(p_wk->bmpwin);
    BmpWin_Free(p_wk->bmpwin);
    GFL_StrBufFree(p_wk->str);
    GFL_HeapFree(p_wk);
}

void BrMsgWin_Print(BrMsgWin *p_wk, MsgData *msg, u32 strID, Font *font, u16 color) {
    BrPoint pos;

    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->bmpwin), p_wk->clear_color);
    GFL_MsgDataLoadStrbuf(msg, strID, p_wk->str);
    BrMsgWin_GetPrintPos(p_wk->pos_type, &p_wk->pos, BmpWin_GetBitmap(p_wk->bmpwin), p_wk->str, font, &pos);
    PrintWindow_Print(&p_wk->print, p_wk->que, pos.x, pos.y, p_wk->str, font, color);
}

void BrMsgWin_PrintBuf(BrMsgWin *p_wk, const StrBuf *str, Font *font, u16 color) {
    BrPoint pos;

    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->bmpwin), p_wk->clear_color);
    GFL_StrBufCopy(p_wk->str, str);
    BrMsgWin_GetPrintPos(p_wk->pos_type, &p_wk->pos, BmpWin_GetBitmap(p_wk->bmpwin), p_wk->str, font, &pos);
    PrintWindow_Print(&p_wk->print, p_wk->que, pos.x, pos.y, p_wk->str, font, color);
}

BOOL BrMsgWin_Main(BrMsgWin *p_wk) {
    PrintWindow_Flush(&p_wk->print, p_wk->que);
    return p_wk->print.flushPending == FALSE;
}

void BrMsgWin_SetPos(BrMsgWin *p_wk, s32 x, s32 y, u32 type) {
    p_wk->pos_type = type;
    p_wk->pos.x = x;
    p_wk->pos.y = y;
}

static void BrMsgWin_GetPrintPos(u32 type, const BrPoint *cp_pos, GFLBitmap *bmp, const StrBuf *str, Font *font,
                                 BrPoint *p_pos) {
    switch (type) {
    case BR_MSGWIN_POS_ABSOLUTE:
        *p_pos = *cp_pos;
        break;
    case BR_MSGWIN_POS_WH_CENTER: {
        u32 x = GFL_BitmapGetWidth(bmp) / 2;
        u32 y = GFL_BitmapGetHeight(bmp) / 2;

        x -= GFL_FontGetBlockWidth(str, font, 0) / 2;
        y -= GFL_FontGetBlockHeight(str, font) / 2;
        p_pos->x = x + cp_pos->x;
        p_pos->y = y + cp_pos->y;
        break;
    }
    }
}

// NitroSDK's MATH_IMin, which nitro/math.h doesn't have yet
static inline int MATH_IMin(int a, int b) {
    return (a <= b) ? a : b;
}

BrList *BrList_Init(const BrListParam *cp_param, HeapID heapId) {
    BrList *p_wk;
    u32 size = sizeof(BrList) + sizeof(GFLBitmap *) * cp_param->list_max;
    int i;

    p_wk = GFL_HeapAllocate(heapId, size, FALSE, "br_util.c", 417);
    sys_memset(p_wk, 0, size);
    p_wk->param = *cp_param;
    p_wk->unk3C = 0;
    p_wk->list_max = cp_param->list_max;
    p_wk->line_max = cp_param->h / cp_param->str_line;
    p_wk->line_max = MATH_IMin(p_wk->line_max, cp_param->list_max);
    if (cp_param->pos != NULL) {
        p_wk->list = cp_param->pos->list;
        p_wk->cursor = cp_param->pos->cursor;
    }

    if (p_wk->param.type == BR_LIST_TYPE_TOUCH) {
        for (i = 0; i < p_wk->line_max; i++) {
            GFL_ASSERT(i < BR_LIST_HITTBL_MAX);
            p_wk->hittbl[i].left = p_wk->param.x * 8;
            p_wk->hittbl[i].top = (p_wk->param.y + i * p_wk->param.str_line) * 8;
            p_wk->hittbl[i].right = (p_wk->param.x + p_wk->param.w) * 8;
            p_wk->hittbl[i].bottom = (p_wk->param.y + (i + 1) * p_wk->param.str_line) * 8;
        }
        GFL_ASSERT(i < BR_LIST_HITTBL_MAX);
        p_wk->hittbl[i].left = TOUCH_RECT_END;
        p_wk->hittbl[i].top = TOUCH_RECT_END;
        p_wk->hittbl[i].right = TOUCH_RECT_END;
        p_wk->hittbl[i].bottom = TOUCH_RECT_END;
        p_wk->move_func = BrList_Move_Touch;
        p_wk->ismove_func = BrList_IsMove_Touch;
        p_wk->decide_func = BrList_Decide_Touch;
    } else if (p_wk->param.type == BR_LIST_TYPE_CURSOR) {
        p_wk->move_func = BrList_Move_Cursor;
        p_wk->ismove_func = BrList_IsMove_Cursor;
        p_wk->decide_func = BrList_Decide_Cursor;
        if (p_wk->param.ballCursor != NULL) {
            BrBallEff_Start(p_wk->param.ballCursor, BR_BALL_EFFECT_ROT_SMALL, &sc_cursor_pos[p_wk->cursor]);
        }
    }

    p_wk->bmpwin =
        BmpWin_CreateDynamic(cp_param->frame, cp_param->x, cp_param->y, cp_param->w, cp_param->h, cp_param->plt, TRUE);
    BmpWin_FlushMap(p_wk->bmpwin);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(p_wk->bmpwin));

    {
        ClActorSetup setup;
        BrResObjData obj;
        u32 display;
        BOOL ret;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        if (cp_param->frame < BGSYS_BG_SUB) {
            BrRes_LoadOBJ(p_wk->param.res, 7, heapId);
            ret = BrRes_GetObjData(p_wk->param.res, 7, &obj);
            GFL_ASSERT(ret);
            display = CLACT_SURFACE_MAIN;
        } else {
            BrRes_LoadOBJ(p_wk->param.res, 8, heapId);
            ret = BrRes_GetObjData(p_wk->param.res, 8, &obj);
            GFL_ASSERT(ret);
            display = CLACT_SURFACE_SUB;
        }
        setup.x = 128;
        setup.priority = 0;
        for (i = 0; i < 2; i++) {
            if (cp_param->type == BR_LIST_TYPE_TOUCH) {
                setup.y = i == 0 ? 16 : 128;
            } else {
                setup.y = i == 0 ? 64 : 184;
            }
            setup.sequence = i;
            p_wk->clwk[i] = func_0204c040(cp_param->unit, obj.chr, obj.plt, obj.cell, &setup, display, heapId);
            func_0204c520(p_wk->clwk[i], TRUE);
            func_0204c318(p_wk->clwk[i], TRUE);
        }
        func_0204c124(p_wk->clwk[0], FALSE);
    }
    {
        ClActorSetup setup;
        BrResObjData obj;
        BOOL ret;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        setup.y = 72;
        setup.sequence = 5;
        setup.priority = 0;
        ret = BrRes_GetObjData(p_wk->param.res, 6, &obj);
        GFL_ASSERT(ret);
        for (i = 2; i < 4; i++) {
            setup.x = i == 2 ? 24 : 232;
            p_wk->clwk[i] =
                func_0204c040(p_wk->param.unit, obj.chr, obj.plt, obj.cell, &setup, CLACT_SURFACE_SUB, heapId);
            func_0204c318(p_wk->clwk[i], TRUE);
        }
    }
    {
        Font *font = BrRes_GetFont(p_wk->param.res);

        for (i = 0; i < p_wk->param.list_max; i++) {
            const StrBuf *str = p_wk->param.cp_list[i].str;

            if (str != NULL) {
                p_wk->bmp[i] = GFL_BitmapCreate(p_wk->param.w, p_wk->param.str_line, 0x20, heapId);
                GFL_BitmapFill(p_wk->bmp[i], 0);
                GFL_TextRendererDrawToBitmapEx(p_wk->bmp[i], 0, 0, str, font, 0x3da0);
            }
        }
    }
    BrList_WriteCore(p_wk, p_wk->list);

    if (cp_param->list_max < p_wk->line_max) {
        p_wk->is_slide = FALSE;
        func_0204c124(p_wk->clwk[0], FALSE);
        func_0204c124(p_wk->clwk[1], FALSE);
    } else {
        p_wk->is_slide = TRUE;
        BrList_UpdateArrow(p_wk);
    }

    if (BrList_IsMove(p_wk)) {
        for (i = 2; i < 4; i++) {
            func_0204c520(p_wk->clwk[i], TRUE);
        }
    }
    return p_wk;
}

void BrList_Exit(BrList *p_wk) {
    int i;

    if (p_wk->param.ballCursor != NULL) {
        BrBallEff_Start(p_wk->param.ballCursor, BR_BALL_EFFECT_NONE, NULL);
    }
    for (i = 0; i < 4; i++) {
        func_0204c108(p_wk->clwk[i]);
    }
    for (i = 0; i < p_wk->param.list_max; i++) {
        if (p_wk->bmp[i] != NULL && p_wk->param.cp_list[i].str != NULL) {
            GFL_BitmapFree(p_wk->bmp[i]);
        }
    }
    BmpWin_ClearScreen(p_wk->bmpwin);
    BmpWin_Free(p_wk->bmpwin);
    if (p_wk->param.frame < BGSYS_BG_SUB) {
        BrRes_UnloadOBJ(p_wk->param.res, 7);
    } else {
        BrRes_UnloadOBJ(p_wk->param.res, 8);
    }
    if (p_wk->param.pos != NULL) {
        p_wk->param.pos->list = p_wk->list;
        p_wk->param.pos->cursor = p_wk->cursor;
    }
    GFL_HeapFree(p_wk);
}

void BrList_Main(BrList *p_wk) {
    if (BrList_IsMove(p_wk)) {
        p_wk->slide = BrList_GetSlide(p_wk);
        if (p_wk->slide != 0) {
            if (p_wk->move_func(p_wk, p_wk->slide)) {
                if (!GFL_SndIsPlaying(0x6fe)) {
                    GFL_SndSEPlay(0x6fe);
                }
                BrList_WriteCore(p_wk, p_wk->list);
            }
            BrList_UpdateArrow(p_wk);
        }
    }
    p_wk->decide_func(p_wk);
}

BOOL BrList_IsMove(BrList *p_wk) {
    return p_wk->ismove_func(p_wk);
}

u32 BrList_GetSelect(const BrList *cp_wk) {
    return cp_wk->select;
}

void BrList_SetBmp(BrList *p_wk, u32 idx, GFLBitmap *bmp) {
    GFL_ASSERT(idx < p_wk->param.list_max);
    GFL_ASSERT(p_wk->param.cp_list[idx].str == NULL);
    p_wk->bmp[idx] = bmp;
}

GFLBitmap *BrList_GetBmp(const BrList *cp_wk, u32 idx) {
    GFL_ASSERT(idx < cp_wk->param.list_max);
    return cp_wk->bmp[idx];
}

u32 BrList_GetParam(const BrList *cp_wk, u32 paramID) {
    switch (paramID) {
    case BR_LIST_PARAM_CURSOR:
        return cp_wk->cursor;
    case BR_LIST_PARAM_LIST:
        return cp_wk->list;
    case BR_LIST_PARAM_SLIDE:
        return cp_wk->slide;
    case BR_LIST_PARAM_LISTMAX:
        return cp_wk->param.list_max;
    default:
        GFL_ASSERT(0);
        return 0;
    }
}

void BrList_Write(BrList *p_wk) {
    BrList_WriteCore(p_wk, p_wk->list);
}

static void BrList_WriteCore(BrList *p_wk, u16 list) {
    int i;

    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->bmpwin), 0);
    for (i = list; i < list + p_wk->line_max; i++) {
        if (i < p_wk->param.list_max && p_wk->bmp[i] != NULL) {
            BrList_BlitBmp(p_wk->bmp[i], BmpWin_GetBitmap(p_wk->bmpwin), 0, 0, 0, (i - list) * p_wk->param.str_line,
                           p_wk->param.w, p_wk->param.str_line);
        }
    }
    BmpWin_FlushChar(p_wk->bmpwin);
}

// The direction the list is dragged in, a step each time the touch moves 8 pixels
static s32 BrList_GetSlide(BrList *p_wk) {
    s32 ret = 0;

    if (func_0203d9c8(sc_slide_rect) != TOUCH_RECT_NONE) {
        u32 x, y;

        func_0203da84(&x, &y);
        if (!p_wk->is_hold) {
            p_wk->is_hold = TRUE;
            p_wk->hold_y = y;
        } else {
            s32 dist = y - p_wk->hold_y;

            if (MATH_ABS(dist) > 8) {
                if (dist < 0) {
                    ret = -1;
                } else {
                    ret = 1;
                }
                p_wk->hold_y = y;
            }
        }
    } else if (p_wk->is_hold) {
        p_wk->is_hold = FALSE;
    }
    return ret;
}

// Copies whole rows of characters from one bitmap to another of the same width
static void BrList_BlitBmp(GFLBitmap *src, GFLBitmap *dst, u16 src_x, u16 src_y, u16 dst_x, u16 dst_y, u16 w, u16 h) {
    u8 *p_src = GFL_BitmapGetPixelData(src);
    u8 *p_dst = GFL_BitmapGetPixelData(dst);

    sys_memcpy32(p_src + (w * src_y + src_x) * 0x20, p_dst + (w * dst_y + dst_x) * 0x20, w * h * 0x20);
}

static BOOL BrList_Move_Touch(BrList *p_wk, s32 dir) {
    if (p_wk->list != 0 && dir == -1) {
        p_wk->list += dir;
        return TRUE;
    }
    if (p_wk->list + p_wk->line_max < p_wk->param.list_max && dir == 1) {
        p_wk->list += dir;
        return TRUE;
    }
    return FALSE;
}

// The cursor moves to the middle row, then the list scrolls under it
static BOOL BrList_Move_Cursor(BrList *p_wk, s32 dir) {
    BOOL ret = FALSE;
    int line = p_wk->line_max;
    int center = (line - 1) / 2 + (line - 1) % 2;

    if (dir > 0) {
        if (center == p_wk->cursor && p_wk->list + line < p_wk->param.list_max) {
            p_wk->list += dir;
            ret = TRUE;
        } else if (p_wk->list + p_wk->cursor < p_wk->param.list_max - 1) {
            p_wk->cursor += dir;
            ret = TRUE;
        }
    } else if (dir < 0) {
        if (center == p_wk->cursor && p_wk->list != 0) {
            p_wk->list += dir;
            ret = TRUE;
        } else if (p_wk->cursor != 0) {
            p_wk->cursor += dir;
            ret = TRUE;
        }
    }

    if (ret && p_wk->param.ballCursor != NULL) {
        BrBallEff_Start(p_wk->param.ballCursor, BR_BALL_EFFECT_ROT_SMALL, &sc_cursor_pos[p_wk->cursor]);
    }
    return ret;
}

static BOOL BrList_IsMove_Touch(BrList *p_wk) {
    return p_wk->param.list_max >= p_wk->line_max;
}

static BOOL BrList_IsMove_Cursor(BrList *p_wk) {
    return p_wk->param.list_max > 1;
}

static void BrList_Decide_Touch(BrList *p_wk) {
    s32 trg;

    p_wk->select = BR_LIST_SELECT_NONE;
    trg = func_0203da0c(p_wk->hittbl);
    if (trg != TOUCH_RECT_NONE) {
        if (p_wk->param.ballDecide != NULL) {
            u32 x, y;
            BrPoint pos;

            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->param.ballDecide, BR_BALL_EFFECT_SPREAD, &pos);
        }
        GFL_SndSEPlay(0x703);
        p_wk->select = p_wk->param.cp_list[trg + p_wk->list].param;
    }
}

static void BrList_Decide_Cursor(BrList *p_wk) {
    p_wk->select = p_wk->param.cp_list[p_wk->cursor + p_wk->list].param;
}

BrMsgWin *BrText_Init(BrRes *res, PrintQueue *que, HeapID heapId) {
    BrMsgWin *p_wk;

    BrRes_LoadBG(res, 2, heapId);
    p_wk = BrMsgWin_Init(0, 1, 19, 30, 4, 2, que, heapId);
    p_wk->clear_color = 12;
    return p_wk;
}

void BrText_Exit(BrMsgWin *p_wk, BrRes *res) {
    BrMsgWin_Exit(p_wk);
    BrRes_UnloadBG(res, 2);
}

void BrText_Print(BrMsgWin *p_wk, BrRes *res, u32 msgID) {
    Font *font = BrRes_GetFont(res);

    BrMsgWin_Print(p_wk, BrRes_GetMsgData(res), msgID, font, 0x3dac);
}

void BrText_PrintBuf(BrMsgWin *p_wk, BrRes *res, const StrBuf *str) {
    BrMsgWin_PrintBuf(p_wk, str, BrRes_GetFont(res), 0x3dac);
}

BOOL BrText_Main(BrMsgWin *p_wk) {
    return BrMsgWin_Main(p_wk);
}

BrProfile *BrProfile_Init(GdsProfile *cp_profile, BrRes *res, ClActUnit *unit, PrintQueue *que, u32 type,
                          HeapID heapId) {
    BrProfile *p_wk;
    Font *font;
    int i;
    MsgData *msg;
    WordSet *wordset;

    p_wk = GFL_HeapAllocate(heapId, sizeof(BrProfile), FALSE, "br_util.c", 1345);
    sys_memset(p_wk, 0, sizeof(BrProfile));
    p_wk->res = res;
    font = BrRes_GetFont(res);
    p_wk->pms = PMSDraw_Create(unit, CLACT_VRAM_MAIN, que, font, 7, 1, heapId);
    PMSDraw_SetObjMode(p_wk->pms, 0, TRUE);
    BrRes_LoadBG(p_wk->res, 8, heapId);
    msg = BrRes_GetMsgData(res);
    wordset = BrRes_GetWordSet(res);

    for (i = 0; i < BR_PROFILE_MSGWIN_MAX; i++) {
        const BrProfileMsgWinData *cp_data = &sc_profile_msgwin[i];
        BOOL isPrint;
        StrBuf *str;
        StrBuf *src;

        p_wk->msgwin[i] = BrMsgWin_Init(1, cp_data->x, cp_data->y, cp_data->w, cp_data->h, 14, que, heapId);
        BrMsgWin_SetPos(p_wk->msgwin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        isPrint = TRUE;

        switch (i) {
        case 0: {
            StrBuf *name;

            str = GFL_StrBufCreate(128, heapId);
            name = GdsProfile_CreateNameStrBuf(cp_profile, heapId);
            src = GFL_MsgDataLoadStrbufNew(msg, cp_data->msgID);
            func_0202437c(wordset, 0, name, GdsProfile_GetSex(cp_profile), TRUE, 2);
            GFL_WordSetFormatStrbuf(wordset, str, src);
            GFL_StrBufFree(name);
            GFL_StrBufFree(src);
            break;
        }
        case 1: {
            u32 month = GdsProfile_GetBirthMonth(cp_profile);

            str = GFL_StrBufCreate(128, heapId);
            src = GFL_MsgDataLoadStrbufNew(msg, cp_data->msgID);
            loadMonthToStrbuf(wordset, 0, month);
            GFL_WordSetFormatStrbuf(wordset, str, src);
            GFL_StrBufFree(src);
            break;
        }
        case 3: {
            u32 country = GdsProfile_GetCountry(cp_profile);

            str = GFL_StrBufCreate(128, heapId);
            src = GFL_MsgDataLoadStrbufNew(msg, cp_data->msgID);
            loadCountryToStrbuf(wordset, 0, country);
            GFL_WordSetFormatStrbuf(wordset, str, src);
            GFL_StrBufFree(src);
            BrMsgWin_SetPos(p_wk->msgwin[i], 8, 1, BR_MSGWIN_POS_ABSOLUTE);
            break;
        }
        case 4: {
            u32 country = GdsProfile_GetCountry(cp_profile);
            u32 area = GdsProfile_GetRegion(cp_profile);

            str = GFL_StrBufCreate(128, heapId);
            src = GFL_MsgDataLoadStrbufNew(msg, cp_data->msgID);
            loadCountryAreaToStrbuf(wordset, 0, country, area);
            GFL_WordSetFormatStrbuf(wordset, str, src);
            GFL_StrBufFree(src);
            BrMsgWin_SetPos(p_wk->msgwin[i], 8, 1, BR_MSGWIN_POS_ABSOLUTE);
            break;
        }
        case 6: {
            PMSData pms;

            src = GdsProfile_GetIntroduction(cp_profile, &pms, heapId);
            if (src != NULL) {
                str = GFL_StrBufClone(src, heapId);
                GFL_StrBufFree(src);
                BrMsgWin_SetPos(p_wk->msgwin[i], 8, 1, BR_MSGWIN_POS_ABSOLUTE);
            } else {
                PMSDrawPos pos = { 8, 0 };

                PMSDraw_SetBackColor(p_wk->pms, 0);
                PMSDraw_SetColor(p_wk->pms, 0x3da0);
                PMSDraw_PrintEx(p_wk->pms, p_wk->msgwin[i]->bmpwin, &pms, 0, &pos);
                isPrint = FALSE;
            }
            break;
        }
        default:
            str = GFL_MsgDataLoadStrbufNew(msg, cp_data->msgID);
            break;
        }

        if (isPrint) {
            BrMsgWin_PrintBuf(p_wk->msgwin[i], str, font, 0x3da0);
            GFL_StrBufFree(str);
        }
    }

    {
        BOOL egg = GdsProfile_IsEgg(cp_profile);
        u32 species = GdsProfile_GetSpecies(cp_profile);
        u32 form = GdsProfile_GetForm(cp_profile);
        u32 gender = GdsProfile_GetGender(cp_profile);

        if (species != 0) {
            ArcTool *handle = GFL_ArcSysCreateFileHandle(7, HEAPID_TAIL(heapId));
            ClActorSetup setup;

            p_wk->pokeIconPlt = func_0204bc48(handle, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 0x80, HEAPID_TAIL(heapId));
            p_wk->pokeIconCel = func_0204bde0(handle, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), HEAPID_TAIL(heapId));
            p_wk->pokeIconChr = func_0204b81c(handle, PokeParty_GetIconIndex(species, form, gender, egg), FALSE,
                                              CLACT_VRAM_MAIN, HEAPID_TAIL(heapId));
            GFL_ArcToolFree(handle);

            sys_memset(&setup, 0, sizeof(ClActorSetup));
            setup.x = 75;
            setup.y = 64;
            setup.sequence = 1;
            p_wk->pokeIcon = func_0204c040(unit, p_wk->pokeIconChr, p_wk->pokeIconPlt, p_wk->pokeIconCel, &setup,
                                           CLACT_SURFACE_MAIN, heapId);
            func_0204c378(p_wk->pokeIcon, PokeIcon_GetPalette(species, form, gender, egg), 0);
            func_0204c318(p_wk->pokeIcon, TRUE);
        }
    }

    switch (type) {
    case BR_PROFILE_TYPE_TRAINER: {
        u32 cel, chr, plt, anm;
        ArcTool *handle;

        if (GdsProfile_GetSex(cp_profile) == 0) {
            chr = 20;
            plt = 4;
            cel = 18;
            anm = 19;
        } else {
            chr = 23;
            plt = 6;
            cel = 21;
            anm = 22;
        }
        handle = GFL_ArcSysCreateFileHandle(30, HEAPID_TAIL(heapId));
        p_wk->trainerPlt = func_0204bbb8(handle, plt, CLACT_VRAM_MAIN, 0x1a0, 0, 1, heapId);
        p_wk->trainerCel = func_0204bde0(handle, cel, anm, heapId);
        p_wk->trainerChr = func_0204b81c(handle, chr, FALSE, CLACT_VRAM_MAIN, heapId);
        GFL_ArcToolFree(handle);
        break;
    }
    case BR_PROFILE_TYPE_UNION: {
        u32 view = GdsProfile_GetUnionView(cp_profile);
        ArcTool *handle = GFL_ArcSysCreateFileHandle(31, HEAPID_TAIL(heapId));

        p_wk->trainerPlt = func_0204bbb8(handle, 0, CLACT_VRAM_MAIN, 0x1a0, sc_union_plt[view], 1, heapId);
        p_wk->trainerCel = func_0204bde0(handle, 65, 66, heapId);
        p_wk->trainerChr = func_0204b81c(handle, view + 49, FALSE, CLACT_VRAM_MAIN, heapId);
        GFL_ArcToolFree(handle);
        break;
    }
    }

    {
        ClActorSetup setup;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        setup.x = 48;
        setup.y = 64;
        p_wk->trainer = func_0204c040(unit, p_wk->trainerChr, p_wk->trainerPlt, p_wk->trainerCel, &setup,
                                      CLACT_SURFACE_MAIN, heapId);
        func_0204c378(p_wk->trainer, 0, 1);
        func_0204c318(p_wk->trainer, TRUE);
    }
    return p_wk;
}

void BrProfile_Exit(BrProfile *p_wk) {
    int i;

    if (p_wk->trainer != NULL) {
        func_0204c108(p_wk->trainer);
        func_0204bcd0(p_wk->trainerPlt);
        func_0204b98c(p_wk->trainerChr);
        func_0204be64(p_wk->trainerCel);
    }
    if (p_wk->pokeIcon != NULL) {
        func_0204c108(p_wk->pokeIcon);
        func_0204bcd0(p_wk->pokeIconPlt);
        func_0204b98c(p_wk->pokeIconChr);
        func_0204be64(p_wk->pokeIconCel);
    }
    for (i = 0; i < BR_PROFILE_MSGWIN_MAX; i++) {
        if (p_wk->msgwin[i] != NULL) {
            BrMsgWin_Exit(p_wk->msgwin[i]);
            p_wk->msgwin[i] = NULL;
        }
    }
    PMSDraw_Delete(p_wk->pms);
    BrRes_UnloadBG(p_wk->res, 8);
    GFL_HeapFree(p_wk);
    GFL_BGSysLoadScr(1);
}

BOOL BrProfile_Main(BrProfile *p_wk) {
    BOOL isEnd = TRUE;
    int i;

    for (i = 0; i < BR_PROFILE_MSGWIN_MAX; i++) {
        if (p_wk->msgwin[i] != NULL) {
            isEnd &= BrMsgWin_Main(p_wk->msgwin[i]);
        }
    }
    PMSDraw_Main(p_wk->pms);
    return isEnd;
}

BrSeq *BrSeq_Init(void *p_wk_adrs, BrSeqFunc seq_function, HeapID heapId) {
    BrSeq *p_wk = GFL_HeapAllocate(heapId, sizeof(BrSeq), FALSE, "br_util.c", 1698);

    sys_memset(p_wk, 0, sizeof(BrSeq));
    p_wk->p_wk_adrs = p_wk_adrs;
    BrSeq_SetNext(p_wk, seq_function);
    return p_wk;
}

void BrSeq_Exit(BrSeq *p_wk) {
    GFL_HeapFree(p_wk);
}

void BrSeq_Main(BrSeq *p_wk) {
    if (p_wk->seq_function != NULL) {
        p_wk->seq_function(p_wk, &p_wk->seq, p_wk->p_wk_adrs);
    }
}

BOOL BrSeq_IsEnd(const BrSeq *cp_wk) {
    return cp_wk->seq_function == NULL;
}

void BrSeq_SetNext(BrSeq *p_wk, BrSeqFunc seq_function) {
    p_wk->seq_function = seq_function;
    p_wk->seq = 0;
}

void BrSeq_End(BrSeq *p_wk) {
    BrSeq_SetNext(p_wk, NULL);
}

BOOL BrSeq_IsComp(const BrSeq *cp_wk, BrSeqFunc seq_function) {
    return cp_wk->seq_function == seq_function;
}

BrBallEffect *BrBallEff_Init(ClActUnit *unit, BrRes *res, u32 display, HeapID heapId) {
    BrBallEffect *p_wk = GFL_HeapAllocate(heapId, sizeof(BrBallEffect), FALSE, "br_util.c", 1994);
    int i;

    sys_memset(p_wk, 0, sizeof(BrBallEffect));
    p_wk->res = res;
    p_wk->display = display;
    p_wk->seq = BrSeq_Init(p_wk, BrBallEff_Seq_Wait, heapId);
    BrRes_LoadOBJ(p_wk->res, display + 9, heapId);
    {
        ClActorSetup setup;
        BrResObjData obj;
        BOOL ret;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        ret = BrRes_GetObjData(p_wk->res, display + 9, &obj);
        GFL_ASSERT(ret);
        for (i = 0; i < BR_BALL_NUM; i++) {
            p_wk->clwk[i] = func_0204c040(unit, obj.chr, obj.plt, obj.cell, &setup, display, heapId);
            func_0204c124(p_wk->clwk[i], FALSE);
            func_0204c318(p_wk->clwk[i], TRUE);
        }
    }
    return p_wk;
}

void BrBallEff_Exit(BrBallEffect *p_wk) {
    int i;

    for (i = 0; i < BR_BALL_NUM; i++) {
        func_0204c108(p_wk->clwk[i]);
    }
    BrRes_UnloadOBJ(p_wk->res, p_wk->display + 9);
    BrSeq_Exit(p_wk->seq);
    GFL_HeapFree(p_wk);
}

void BrBallEff_Main(BrBallEffect *p_wk) {
    BrSeq_Main(p_wk->seq);
}

void BrBallEff_Start(BrBallEffect *p_wk, u32 type, const BrPoint *pos) {
    if (pos != NULL) {
        p_wk->pos = *pos;
        p_wk->p_follow = pos;
    }
    p_wk->is_end = FALSE;
    if (type == BR_BALL_EFFECT_NONE && BrSeq_IsComp(p_wk->seq, BrBallEff_Seq_Rot)) {
        GFL_SndSEPlay(0x701);
    }
    switch (type) {
    case BR_BALL_EFFECT_NONE:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Wait);
        break;
    case BR_BALL_EFFECT_SPREAD:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Spread);
        break;
    case BR_BALL_EFFECT_LINE:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Line);
        break;
    case BR_BALL_EFFECT_LINE_ROT:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_LineRot);
        break;
    case BR_BALL_EFFECT_ROT:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Rot);
        break;
    case BR_BALL_EFFECT_ROT_SMALL:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_RotSmall);
        break;
    case BR_BALL_EFFECT_STOP:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Stop);
        break;
    case BR_BALL_EFFECT_TOUCH:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Touch);
        break;
    case BR_BALL_EFFECT_RING:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Ring);
        break;
    }
}

BOOL BrBallEff_IsEnd(const BrBallEffect *cp_wk) {
    return cp_wk->is_end;
}

void BrBallEff_SetAnmSeq(BrBallEffect *p_wk, u32 seq) {
    int i;

    for (i = 0; i < BR_BALL_NUM; i++) {
        func_0204c488(p_wk->clwk[i], seq);
    }
}

static void BrBallEff_Seq_Wait(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        for (i = 0; i < BR_BALL_NUM; i++) {
            func_0204c124(p_wk->clwk[i], FALSE);
        }
        p_wk->is_end = TRUE;
        *p_seq = 1;
        break;
    case 1:
        break;
    }
}

static void BrBallEff_Seq_Spread(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        p_wk->cnt = 0;
        for (i = 0; i < 6; i++) {
            BrBallCircle_Init(&p_wk->move[i].circle, &p_wk->pos, 8, 20, 0xffff * i / 6, 0xffff * i / 6 + 0x3333, 15);
            func_0204c124(p_wk->clwk[i], TRUE);
        }
        *p_seq = 1;
        // fallthrough
    case 1:
        for (i = 0; i < 6; i++) {
            BrPoint now;
            ClActorPos pos;

            BrBallCircle_Main(&p_wk->move[i].circle, &now, p_wk->cnt);
            pos.x = now.x;
            pos.y = now.y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        if (p_wk->cnt++ > 15) {
            *p_seq = 2;
        }
        break;
    case 2:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Wait);
        break;
    }
}

static void BrBallEff_Seq_Line(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;
    ClActorPos pos;
    BOOL isEnd;

    switch (*p_seq) {
    case 0:
        p_wk->cnt = 0;
        for (i = 0; i < BR_BALL_NUM; i++) {
            p_wk->ball[i] = sc_ball_pos_top;
            func_0204c124(p_wk->clwk[i], TRUE);
        }
        BrBallLine_Init(&p_wk->move[0].line, &p_wk->ball[0], &sc_ball_pos_below, 0, 50);
        for (i = 1; i < BR_BALL_NUM; i++) {
            BrBallFollow_Init(&p_wk->move[i].follow, &p_wk->ball[i - 1], &p_wk->ball[i], FX32_CONST(4) - 0x666 * i / 2);
        }
        *p_seq = 1;
        // fallthrough
    case 1:
        isEnd = TRUE;
        p_wk->is_end = BrBallLine_Main(&p_wk->move[0].line, &p_wk->ball[0], p_wk->cnt);
        isEnd &= p_wk->is_end;
        for (i = 1; i < BR_BALL_NUM; i++) {
            isEnd &= BrBallFollow_Main(&p_wk->move[i].follow, &p_wk->ball[i]);
        }
        for (i = 0; i < BR_BALL_NUM; i++) {
            pos.x = p_wk->ball[i].x;
            pos.y = p_wk->ball[i].y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        p_wk->cnt++;
        if (isEnd) {
            *p_seq = 2;
        }
        break;
    case 2:
        BrSeq_SetNext(p_wk->seq, BrBallEff_Seq_Wait);
        break;
    }
}

static void BrBallEff_Seq_LineRot(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        p_wk->cnt = 0;
        for (i = 0; i < 6; i++) {
            p_wk->ball[i] = sc_ball_pos_above;
            func_0204c124(p_wk->clwk[i], TRUE);
        }
        BrBallLine_Init(&p_wk->move[0].line, &p_wk->ball[0], &sc_ball_pos_center, 0, 36);
        for (i = 1; i < 6; i++) {
            BrBallFollow_Init(&p_wk->move[i].follow, &p_wk->ball[i - 1], &p_wk->ball[i],
                              FX32_CONST(3.5) - 0xb33 * i / 2);
        }
        *p_seq = 1;
        // fallthrough
    case 1:
        p_wk->is_move_end[0] = BrBallLine_Main(&p_wk->move[0].line, &p_wk->ball[0], p_wk->cnt);
        for (i = 1; i < 6; i++) {
            p_wk->is_move_end[i] = BrBallFollow_Main(&p_wk->move[i].follow, &p_wk->ball[i]);
        }
        for (i = 0; i < 6; i++) {
            ClActorPos pos;

            pos.x = p_wk->ball[i].x;
            pos.y = p_wk->ball[i].y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        p_wk->cnt++;
        if (p_wk->is_move_end[0]) {
            func_0204c124(p_wk->clwk[0], FALSE);
            *p_seq = 2;
        }
        break;
    case 2:
        p_wk->cnt = 0;
        for (i = 6; i < BR_BALL_NUM; i++) {
            BrBallRot_Init(&p_wk->move[i].rot, &sc_ball_pos_center, 15, 0xffff * (i - 6) / 6, 90);
            func_0204c124(p_wk->clwk[i], TRUE);
        }
        *p_seq = 3;
        // fallthrough
    case 3:
        for (i = 6; i < BR_BALL_NUM; i++) {
            BrBallRot_Main(&p_wk->move[i].rot, &p_wk->ball[i], p_wk->cnt);
        }
        for (i = 1; i < 6; i++) {
            p_wk->is_move_end[i] = BrBallFollow_Main(&p_wk->move[i].follow, &p_wk->ball[i]);
            if (p_wk->is_move_end[i]) {
                func_0204c124(p_wk->clwk[i], FALSE);
            }
        }
        for (i = 0; i < BR_BALL_NUM; i++) {
            ClActorPos pos;

            pos.x = p_wk->ball[i].x;
            pos.y = p_wk->ball[i].y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        p_wk->is_end = TRUE;
        p_wk->cnt++;
        break;
    }
}

static void BrBallEff_Seq_Rot(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        p_wk->cnt = 0;
        for (i = 0; i < 6; i++) {
            BrBallRot_Init(&p_wk->move[i].rot, &p_wk->pos, 15, 0xffff * i / 6, 90);
            func_0204c124(p_wk->clwk[i], TRUE);
        }
        GFL_SndSEPlay(0x705);
        *p_seq = 1;
        // fallthrough
    case 1:
        for (i = 0; i < 6; i++) {
            BrPoint now;
            ClActorPos pos;

            BrBallRot_Main(&p_wk->move[i].rot, &now, p_wk->cnt);
            pos.x = now.x;
            pos.y = now.y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        p_wk->cnt++;
        break;
    }
}

static void BrBallEff_Seq_RotSmall(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        p_wk->cnt = 0;
        for (i = 0; i < 6; i++) {
            BrBallRot_Init(&p_wk->move[i].rot, &p_wk->pos, 10, 0xffff * i / 6, 90);
            func_0204c124(p_wk->clwk[i], TRUE);
        }
        *p_seq = 1;
        // fallthrough
    case 1:
        for (i = 0; i < 6; i++) {
            BrPoint now;
            ClActorPos pos;

            BrBallRot_Main(&p_wk->move[i].rot, &now, p_wk->cnt);
            pos.x = now.x;
            pos.y = now.y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        p_wk->cnt++;
        break;
    }
}

static void BrBallEff_Seq_Stop(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
}

static void BrBallEff_Seq_Touch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        p_wk->cnt = 0;
        for (i = 0; i < 6; i++) {
            BrBallCircle_Init(&p_wk->move[i].circle, &p_wk->pos, 8, 20, 0xffff * i / 6, 0xffff * i / 6 + 0x3333, 16);
            func_0204c124(p_wk->clwk[i], TRUE);
        }
        *p_seq = 1;
        // fallthrough
    case 1:
        for (i = 0; i < 6; i++) {
            ClActorPos pos;

            BrBallCircle_Main(&p_wk->move[i].circle, &p_wk->ball[i], p_wk->cnt);
            pos.x = p_wk->ball[i].x;
            pos.y = p_wk->ball[i].y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        if (p_wk->cnt++ > 16) {
            *p_seq = 2;
        }
        break;
    case 2:
        *p_seq = 3;
        break;
    case 3:
        for (i = 0; i < 6; i++) {
            BrBallFollow_Init(&p_wk->move[i].follow, p_wk->p_follow, &p_wk->ball[i], FX32_CONST(4) - 0x99a * i / 2);
        }
        *p_seq = 4;
        // fallthrough
    case 4: {
        BOOL isEnd = TRUE;

        for (i = 0; i < 6; i++) {
            ClActorPos pos;

            isEnd &= BrBallFollow_Main(&p_wk->move[i].follow, &p_wk->ball[i]);
            pos.x = p_wk->ball[i].x;
            pos.y = p_wk->ball[i].y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        if (isEnd) {
            *p_seq = 5;
        }
        break;
    }
    case 5:
        BrSeq_SetNext(p_seqwk, BrBallEff_Seq_Wait);
        break;
    }
}

static void BrBallEff_Seq_Ring(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBallEffect *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        BrBallEff_SetAnmSeq(p_wk, 0);
        for (i = 6; i < BR_BALL_NUM; i++) {
            // BUG: BrBallRot_Main turns by 0xffff * cnt / sync_max, so the ring starts from each ball's first angle
            // instead of where it had turned to, and the balls jump
#ifdef BUGFIX
            u16 rot = p_wk->move[i].rot.rot + 0xffff * p_wk->cnt / p_wk->move[i].rot.sync_max;
#else
            u16 rot = p_wk->move[i].rot.rot + p_wk->cnt / p_wk->move[i].rot.sync_max;
#endif

            BrBallCircle_Init(&p_wk->move[i].circle, &sc_ball_pos_center, 15, 1, rot, rot + 0xf000, 30);
            func_0204c318(p_wk->clwk[i], FALSE);
        }
        p_wk->cnt = 0;
        *p_seq = 1;
        // fallthrough
    case 1:
        for (i = 6; i < BR_BALL_NUM; i++) {
            BrPoint now;
            ClActorPos pos;

            BrBallCircle_Main(&p_wk->move[i].circle, &now, p_wk->cnt);
            pos.x = now.x;
            pos.y = now.y;
            func_0204c140(p_wk->clwk[i], &pos, p_wk->display);
        }
        if (p_wk->cnt++ > 60) {
            *p_seq = 2;
        }
        break;
    case 2:
        BrSeq_SetNext(p_seqwk, BrBallEff_Seq_Wait);
        break;
    }
}

static void BrBallCircle_Init(BrBallCircle *p_wk, const BrPoint *cp_center, u16 r_start, u16 r_end, u16 rot_start,
                              u16 rot_end, s32 sync_max) {
    sys_memset(p_wk, 0, sizeof(BrBallCircle));
    p_wk->r_start = r_start;
    p_wk->r_end = r_end;
    p_wk->rot_start = rot_start;
    p_wk->rot_end = rot_end;
    p_wk->sync_max = sync_max;
    p_wk->center = *cp_center;
    BrBallCircle_Main(p_wk, &p_wk->now, 0);
}

static BOOL BrBallCircle_Main(BrBallCircle *p_wk, BrPoint *p_now, s32 sync) {
    u16 rot_dist = p_wk->rot_end - p_wk->rot_start;
    s16 r_dist = p_wk->r_end - (s16)p_wk->r_start;
    s8 dir = r_dist / MATH_ABS(r_dist);
    u16 rot = p_wk->rot_start + rot_dist * sync / p_wk->sync_max;
    s32 r = p_wk->r_start + dir * (MATH_ABS(r_dist) * sync / p_wk->sync_max);

    if (r < 0) {
        r = 0;
    }
    p_wk->now.x = (FX_CosIdx(rot) * r) >> FX32_SHIFT;
    p_wk->now.y = (FX_SinIdx(rot) * r) >> FX32_SHIFT;
    p_wk->now.x += p_wk->center.x;
    p_wk->now.y += p_wk->center.y;
    if (p_now != NULL) {
        *p_now = p_wk->now;
    }
    return sync == p_wk->sync_max;
}

static void BrBallRot_Init(BrBallRot *p_wk, const BrPoint *cp_center, u16 r, u16 rot, s32 sync_max) {
    sys_memset(p_wk, 0, sizeof(BrBallRot));
    p_wk->center = *cp_center;
    p_wk->sync_max = sync_max;
    p_wk->r = r;
    p_wk->rot = rot;
    BrBallRot_Main(p_wk, NULL, 0);
}

static BOOL BrBallRot_Main(BrBallRot *p_wk, BrPoint *p_now, s32 sync) {
    u16 rot = p_wk->rot + 0xffff * sync / p_wk->sync_max;

    p_wk->now.x = (FX_CosIdx(rot) * p_wk->r) >> FX32_SHIFT;
    p_wk->now.y = (FX_SinIdx(rot) * p_wk->r) >> FX32_SHIFT;
    p_wk->now.x += p_wk->center.x;
    p_wk->now.y += p_wk->center.y;
    if (p_now != NULL) {
        *p_now = p_wk->now;
    }
    return sync == p_wk->sync_max;
}

static void BrBallLine_Init(BrBallLine *p_wk, const BrPoint *cp_start, const BrPoint *cp_end, s32 sync_start,
                            s32 sync_max) {
    sys_memset(p_wk, 0, sizeof(BrBallLine));
    p_wk->start = *cp_start;
    p_wk->end = *cp_end;
    p_wk->sync_max = sync_max;
    p_wk->sync_start = sync_start;
    BrBallLine_Main(p_wk, NULL, 0);
}

static BOOL BrBallLine_Main(BrBallLine *p_wk, BrPoint *p_now, s32 sync) {
    BOOL ret = FALSE;
    s32 now = sync - p_wk->sync_start;

    if (now < 0) {
        now = 0;
    }
    p_wk->now.x = p_wk->start.x + (p_wk->end.x - p_wk->start.x) * now / p_wk->sync_max;
    p_wk->now.y = p_wk->start.y + (p_wk->end.y - p_wk->start.y) * now / p_wk->sync_max;
    if (now >= p_wk->sync_max) {
        p_wk->now = p_wk->end;
        ret = TRUE;
    }
    if (p_now != NULL) {
        *p_now = p_wk->now;
    }
    return ret;
}

static void BrBallFollow_Init(BrBallFollow *p_wk, const BrPoint *cp_target, const BrPoint *cp_start, fx32 speed) {
    sys_memset(p_wk, 0, sizeof(BrBallFollow));
    p_wk->target = cp_target;
    p_wk->x = FX32_CONST(cp_start->x);
    p_wk->y = FX32_CONST(cp_start->y);
    p_wk->z = 0;
    p_wk->speed = speed;
}

// Moves toward the target at the speed, and onto it once it is closer than a step. TRUE once it is on it
static BOOL BrBallFollow_Main(BrBallFollow *p_wk, BrPoint *p_now) {
    VecFx32 dist;
    fx32 len;

    dist.x = FX32_CONST(p_wk->target->x) - p_wk->x;
    dist.y = FX32_CONST(p_wk->target->y) - p_wk->y;
    dist.z = 0;
    len = VEC_Mag(&dist);
    if (len != 0) {
        fx32 dx = FX_Div(FX_Mul(dist.x, p_wk->speed), len);
        fx32 dy = FX_Div(FX_Mul(dist.y, p_wk->speed), len);

        if (MATH_ABS(dist.x) > MATH_ABS(dx)) {
            p_wk->x += dx;
        } else {
            p_wk->x = FX32_CONST(p_wk->target->x);
        }
        if (MATH_ABS(dist.y) > MATH_ABS(dy)) {
            p_wk->y += dy;
        } else {
            p_wk->y = FX32_CONST(p_wk->target->y);
        }
    }
    if (p_now != NULL) {
        p_now->x = FX_Whole(p_wk->x);
        p_now->y = FX_Whole(p_wk->y);
    }
    return len == 0;
}
