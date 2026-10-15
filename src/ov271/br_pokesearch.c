// The Battle Recorder's Pokémon search: nine buttons for the first letter of a Pokémon's name, then the list of the
// Pokémon that the Pokédex has seen with that letter. The name is the ROM's string, which GFL_HeapAllocate is given.

#include "types.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_pokesearch.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "pml/species_names.h"
#include "save/pokedex.h"
#include "system/bmp_menuwork.h"

// The first letters, which the buttons are for
#define BR_POKESEARCH_LETTER_MAX 9

// The archive with the Pokémon sorted by the first letter of their names, and its file
#define BR_POKESEARCH_ARC 0x61
#define BR_POKESEARCH_ARC_FILE 5

// BR_RES_BG_* of the letter buttons' screen and of the list's
#define BR_POKESEARCH_BG_LETTER 0x14
#define BR_POKESEARCH_BG_LIST 0x15

// The steps of BrPokeSearch_Main
enum {
    BR_POKESEARCH_SEQ_START,
    BR_POKESEARCH_SEQ_WAIT,
    BR_POKESEARCH_SEQ_LETTER,
    BR_POKESEARCH_SEQ_LIST,
    BR_POKESEARCH_SEQ_FADE_OUT_WAIT,
    BR_POKESEARCH_SEQ_FADE_OUT,
    BR_POKESEARCH_SEQ_CHANGE,
    BR_POKESEARCH_SEQ_FADE_IN,
    BR_POKESEARCH_SEQ_END_0,
    BR_POKESEARCH_SEQ_END_1,
    BR_POKESEARCH_SEQ_END_2,
};

// What BrPokeSearch_CheckTouch gives when no letter is touched
#define BR_POKESEARCH_LETTER_NONE BR_POKESEARCH_LETTER_MAX

// Which of its two screens it shows
enum {
    BR_POKESEARCH_MODE_LETTER,
    BR_POKESEARCH_MODE_LIST,
};

struct BrPokeSearch {
    u32 seq;
    u32 next_seq;
    // BR_POKESEARCH_SELECT_*
    u32 select;
    u32 letter;
    // The species the p_list selected
    u32 species;
    // Whether the Pokédex has seen a Pokémon of each letter
    u8 is_letter_seen[BR_POKESEARCH_LETTER_MAX];
    BOOL is_end;
    HeapID heap_id;
    BrRes *res;
    BrBtn *btn;
    BrFade *fade;
    ClActUnit *unit;
    BmpOamSys *bmpoam;
    PrintQueue *que;
    BrMsgWin *msgwin[BR_POKESEARCH_LETTER_MAX];
    ListMenuOption *p_list_data;
    BrList *p_list;
    BOOL is_active;
    u32 mode;
    BrBallEffect *ball_main;
    BrBallEffect *ball_sub;
    PokeDexSave *pokedex;
    // The species sorted by letter, and how many
    u16 *monsno_table;
    u32 monsno_table_num;
    // The species of the chosen letter that the Pokédex has seen, and how many
    u16 *p_monsno_buff;
    u32 monsno_num;
};

static const u16 sc_pokesearch_letter_index[BR_POKESEARCH_LETTER_MAX + 1] = {
    0x0000, 0x0070, 0x00b8, 0x00fd, 0x013b, 0x0188, 0x01cf, 0x0253, 0x027d, 0x0289,
};

static const BrListParam sc_pokesearch_list_param = {
    NULL, 0, 8, 3, 16, 12, 14, 4, 2, BR_LIST_TYPE_TOUCH, NULL, NULL, NULL, NULL, NULL,
};

static const TouchRect sc_pokesearch_touch_rects[BR_POKESEARCH_LETTER_MAX + 1] = {
    { 0x18, 0x38, 0x10, 0x30 },           { 0x18, 0x38, 0x40, 0x60 }, { 0x18, 0x38, 0x70, 0x90 },
    { 0x18, 0x38, 0xa0, 0xc0 },           { 0x18, 0x38, 0xd0, 0xf0 }, { 0x48, 0x68, 0x10, 0x30 },
    { 0x48, 0x68, 0x40, 0x60 },           { 0x48, 0x68, 0x70, 0x90 }, { 0x48, 0x68, 0xa0, 0xc0 },
    { TOUCH_RECT_END, 0xff, 0xff, 0xff },
};

static void BrPokeSearch_InitBtn(BrPokeSearch *p_wk);
static void BrPokeSearch_ExitBtn(BrPokeSearch *p_wk);
static void BrPokeSearch_InitLetterWin(BrPokeSearch *p_wk);
static void BrPokeSearch_ExitLetterWin(BrPokeSearch *p_wk);
static void BrPokeSearch_InitList(BrPokeSearch *p_wk);
static void BrPokeSearch_ExitList(BrPokeSearch *p_wk);
static u32 BrPokeSearch_CheckTouch(BrPokeSearch *p_wk);
static BOOL BrPokeSearch_IsLetterSeen(PokeDexSave *pokedex, u32 letter, const u16 *table);
static u16 *BrPokeSearch_CreateMonsList(PokeDexSave *pokedex, u32 letter, const u16 *table, u32 *num, HeapID heapId);
static u16 *BrPokeSearch_LoadMonsTable(HeapID heapId, u32 *num);

BrPokeSearch *BrPokeSearch_Init(PokeDexSave *pokedex, BrRes *res, ClActUnit *unit, BmpOamSys *bmpoam, BrFade *fade,
                                BrBallEffect *ballMain, BrBallEffect *ballSub, HeapID heapId) {
    BrPokeSearch *p_wk;
    int i;

    p_wk = GFL_HeapAllocate(heapId, sizeof(BrPokeSearch), FALSE, "br_pokesearch.c", 298);
    sys_memset(p_wk, 0, sizeof(BrPokeSearch));

    p_wk->heap_id = heapId;
    p_wk->pokedex = pokedex;
    p_wk->res = res;
    p_wk->fade = fade;
    p_wk->unit = unit;
    p_wk->bmpoam = bmpoam;
    p_wk->que = func_02021998(p_wk->heap_id);
    p_wk->monsno_table = BrPokeSearch_LoadMonsTable(HEAPID_TAIL(heapId), &p_wk->monsno_table_num);
    p_wk->ball_main = ballMain;
    p_wk->ball_sub = ballSub;

    for (i = 0; i < BR_POKESEARCH_LETTER_MAX; i++) {
        p_wk->is_letter_seen[i] = BrPokeSearch_IsLetterSeen(p_wk->pokedex, i, p_wk->monsno_table);
    }
    return p_wk;
}

void BrPokeSearch_Exit(BrPokeSearch *p_wk) {
    if (p_wk->p_monsno_buff != NULL) {
        GFL_HeapFree(p_wk->p_monsno_buff);
        p_wk->p_monsno_buff = NULL;
    }
    GFL_HeapFree(p_wk->monsno_table);
    func_02021a18(p_wk->que);
    GFL_HeapFree(p_wk);
}

void BrPokeSearch_Main(BrPokeSearch *p_wk) {
    if (p_wk->is_active) {
        switch (p_wk->seq) {
        case BR_POKESEARCH_SEQ_START:
            p_wk->seq++;
            break;
        case BR_POKESEARCH_SEQ_WAIT:
            p_wk->seq = BR_POKESEARCH_SEQ_LETTER;
            break;
        case BR_POKESEARCH_SEQ_LETTER: {
            u32 x, y;
            BOOL isTouch;

            p_wk->letter = BrPokeSearch_CheckTouch(p_wk);
            isTouch = func_0203dac8(&x, &y);
            if (BrBtn_GetTrg(p_wk->btn, x, y) & isTouch) {
                BrPoint pos;

                pos.x = x;
                pos.y = y;
                BrBallEff_Start(p_wk->ball_sub, BR_BALL_EFFECT_SPREAD, &pos);
                p_wk->seq = BR_POKESEARCH_SEQ_END_0;
                p_wk->select = BR_POKESEARCH_SELECT_CANCEL;
            } else if (p_wk->letter != BR_POKESEARCH_LETTER_NONE) {
                p_wk->seq = BR_POKESEARCH_SEQ_FADE_OUT_WAIT;
                p_wk->next_seq = BR_POKESEARCH_SEQ_LIST;
            }
            break;
        }
        case BR_POKESEARCH_SEQ_LIST: {
            u32 x, y;
            BOOL isTouch;

            BrList_Main(p_wk->p_list);
            p_wk->species = BrList_GetSelect(p_wk->p_list);
            isTouch = func_0203dac8(&x, &y);
            if (BrBtn_GetTrg(p_wk->btn, x, y) & isTouch) {
                BrPoint pos;

                pos.x = x;
                pos.y = y;
                BrBallEff_Start(p_wk->ball_sub, BR_BALL_EFFECT_SPREAD, &pos);
                p_wk->seq = BR_POKESEARCH_SEQ_FADE_OUT_WAIT;
                p_wk->next_seq = BR_POKESEARCH_SEQ_LETTER;
            } else if (p_wk->species != BR_LIST_SELECT_NONE) {
                p_wk->seq = BR_POKESEARCH_SEQ_END_0;
                p_wk->select = BR_POKESEARCH_SELECT_DECIDE;
            }
            break;
        }
        case BR_POKESEARCH_SEQ_FADE_OUT_WAIT:
            if (BrBallEff_IsEnd(p_wk->ball_sub)) {
                BrFade_StartFade(p_wk->fade, 2, 2, 1);
                p_wk->seq++;
            }
            break;
        case BR_POKESEARCH_SEQ_FADE_OUT:
            if (BrFade_IsEnd(p_wk->fade)) {
                p_wk->seq++;
            }
            break;
        case BR_POKESEARCH_SEQ_CHANGE:
            if (p_wk->next_seq == BR_POKESEARCH_SEQ_LIST) {
                BrPokeSearch_ExitLetterWin(p_wk);
                GFL_ASSERT(p_wk->p_monsno_buff == NULL);
                p_wk->p_monsno_buff = BrPokeSearch_CreateMonsList(p_wk->pokedex, p_wk->letter, p_wk->monsno_table,
                                                                  &p_wk->monsno_num, p_wk->heap_id);
                BrPokeSearch_InitList(p_wk);
            } else {
                BrPokeSearch_ExitList(p_wk);
                if (p_wk->p_monsno_buff != NULL) {
                    GFL_HeapFree(p_wk->p_monsno_buff);
                    p_wk->p_monsno_buff = NULL;
                }
                BrPokeSearch_InitLetterWin(p_wk);
            }
            BrFade_StartFade(p_wk->fade, 2, 2, 0);
            p_wk->seq++;
            break;
        case BR_POKESEARCH_SEQ_FADE_IN:
            if (BrFade_IsEnd(p_wk->fade)) {
                p_wk->seq = p_wk->next_seq;
            }
            break;
        case BR_POKESEARCH_SEQ_END_0:
            p_wk->seq++;
            break;
        case BR_POKESEARCH_SEQ_END_1:
            p_wk->seq++;
            break;
        case BR_POKESEARCH_SEQ_END_2:
            p_wk->is_end = TRUE;
            break;
        }
    }
    BrPokeSearch_PrintMain(p_wk);
}

BOOL BrPokeSearch_PrintMain(BrPokeSearch *p_wk) {
    BOOL ret = TRUE;
    int i;

    for (i = 0; i < BR_POKESEARCH_LETTER_MAX; i++) {
        if (p_wk->msgwin[i] != NULL) {
            ret &= BrMsgWin_Main(p_wk->msgwin[i]);
        }
    }
    return ret & func_02021a3c(p_wk->que);
}

void BrPokeSearch_StartUp(BrPokeSearch *p_wk) {
    BrPokeSearch_InitBtn(p_wk);
    BrPokeSearch_InitLetterWin(p_wk);
    p_wk->is_active = TRUE;
}

void BrPokeSearch_CleanUp(BrPokeSearch *p_wk) {
    p_wk->is_active = FALSE;
    if (p_wk->mode == BR_POKESEARCH_MODE_LIST) {
        BrPokeSearch_ExitList(p_wk);
    } else {
        BrPokeSearch_ExitLetterWin(p_wk);
    }
    BrPokeSearch_ExitBtn(p_wk);
}

u32 BrPokeSearch_GetSelect(const BrPokeSearch *cp_wk, u16 *species) {
    if (cp_wk->is_end) {
        if (cp_wk->select == BR_POKESEARCH_SELECT_DECIDE) {
            *species = cp_wk->species;
        }
        return cp_wk->select;
    }
    return BR_POKESEARCH_SELECT_NONE;
}

static void BrPokeSearch_InitBtn(BrPokeSearch *p_wk) {
    Font *font = BrRes_GetFont(p_wk->res);
    MsgData *msg = BrRes_GetMsgData(p_wk->res);
    ClActorSetup setup;
    BrResObjData obj;
    BOOL ret;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 80;
    setup.y = 168;
    setup.sequence = 0;
    setup.priority = 1;

    // The back button
    ret = BrRes_GetObjData(p_wk->res, 6, &obj);
    GFL_ASSERT(ret);
    p_wk->btn = BrBtn_InitEx(&setup, 4, 96, 1, p_wk->unit, p_wk->bmpoam, font, msg, &obj, p_wk->heap_id);
}

static void BrPokeSearch_ExitBtn(BrPokeSearch *p_wk) {
    BrBtn_Exit(p_wk->btn);
}

static void BrPokeSearch_InitLetterWin(BrPokeSearch *p_wk) {
    Font *font;
    MsgData *msg;
    int i;

    p_wk->mode = BR_POKESEARCH_MODE_LETTER;
    BrRes_LoadBG(p_wk->res, BR_POKESEARCH_BG_LETTER, p_wk->heap_id);
    font = BrRes_GetFont(p_wk->res);
    msg = BrRes_GetMsgData(p_wk->res);

    for (i = 0; i < BR_POKESEARCH_LETTER_MAX; i++) {
        u8 x = 2 + (i % 5) * 6;
        u8 y = 4 + (i >= 5 ? 6 : 0);
        u16 color;

        p_wk->msgwin[i] = BrMsgWin_Init(4, x, y, 4, 2, 14, p_wk->que, p_wk->heap_id);
        color = p_wk->is_letter_seen[i] ? 0x3da0 : 0x2da0;
        BrMsgWin_SetPos(p_wk->msgwin[i], 0, 0, BR_MSGWIN_POS_WH_CENTER);
        BrMsgWin_Print(p_wk->msgwin[i], msg, 0x3a + i, font, color);
    }
}

static void BrPokeSearch_ExitLetterWin(BrPokeSearch *p_wk) {
    int i;

    for (i = 0; i < BR_POKESEARCH_LETTER_MAX; i++) {
        BrMsgWin_Exit(p_wk->msgwin[i]);
        p_wk->msgwin[i] = NULL;
    }
    GFL_BGSysLoadScr(4);
    BrRes_UnloadBG(p_wk->res, BR_POKESEARCH_BG_LETTER);
}

static void BrPokeSearch_InitList(BrPokeSearch *p_wk) {
    BrListParam param;
    StrBuf *buf;
    u32 i;

    p_wk->mode = BR_POKESEARCH_MODE_LIST;
    BrRes_LoadBG(p_wk->res, BR_POKESEARCH_BG_LIST, p_wk->heap_id);
    GFL_ASSERT(p_wk->p_list_data == NULL);
    GFL_ASSERT(p_wk->p_list == NULL);

    p_wk->p_list_data = ListMenuCore_CreateOptionList(p_wk->monsno_num, p_wk->heap_id);
    buf = GFL_StrBufCreate(11, p_wk->heap_id);
    for (i = 0; i < p_wk->monsno_num; i++) {
        GFL_MsgDataLoadStrbuf(g_PMLSpeciesNamesResident, p_wk->p_monsno_buff[i], buf);
        ListMenuCore_AppendStrBufOption(&p_wk->p_list_data[i], buf, p_wk->p_monsno_buff[i], p_wk->heap_id);
    }
    GFL_StrBufFree(buf);

    param = sc_pokesearch_list_param;
    param.cp_list = (const BrListItem *)p_wk->p_list_data;
    param.list_max = p_wk->monsno_num;
    param.res = p_wk->res;
    param.unit = p_wk->unit;
    param.ballCursor = p_wk->ball_main;
    param.ballDecide = p_wk->ball_sub;
    p_wk->p_list = BrList_Init(&param, p_wk->heap_id);
}

static void BrPokeSearch_ExitList(BrPokeSearch *p_wk) {
    BrList_Exit(p_wk->p_list);
    p_wk->p_list = NULL;
    ListMenuCore_FreeOptionList(p_wk->p_list_data);
    p_wk->p_list_data = NULL;
    BrRes_UnloadBG(p_wk->res, BR_POKESEARCH_BG_LIST);
}

static u32 BrPokeSearch_CheckTouch(BrPokeSearch *p_wk) {
    s32 letter = func_0203da0c(sc_pokesearch_touch_rects);

    if (letter != TOUCH_RECT_NONE) {
        if (p_wk->is_letter_seen[letter]) {
            u32 x, y;
            BrPoint pos;

            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ball_sub, BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(0x703);
            return letter;
        }
        GFL_SndSEPlay(0x704);
    }
    return BR_POKESEARCH_LETTER_NONE;
}

static BOOL BrPokeSearch_IsLetterSeen(PokeDexSave *pokedex, u32 letter, const u16 *table) {
    u32 i;
    u32 end = sc_pokesearch_letter_index[letter + 1];
    BOOL ret = FALSE;

    for (i = sc_pokesearch_letter_index[letter]; i < end; i++) {
        if (PokeDex_IsSeen(pokedex, table[i])) {
            ret = TRUE;
        }
    }
    return ret;
}

static u16 *BrPokeSearch_CreateMonsList(PokeDexSave *pokedex, u32 letter, const u16 *table, u32 *num, HeapID heapId) {
    u16 *buff;
    u32 i;
    u32 end = sc_pokesearch_letter_index[letter + 1];
    u32 count = 0;
    u32 start = sc_pokesearch_letter_index[letter];

    for (i = start; i < end; i++) {
        if (PokeDex_IsSeen(pokedex, table[i])) {
            count++;
        }
    }

    buff = GFL_HeapAllocate(heapId, count * sizeof(u16), FALSE, "br_pokesearch.c", 984);
    sys_memset(buff, 0, count * sizeof(u16));

    {
        u32 n = 0;
        for (i = start; i < end; i++) {
            if (PokeDex_IsSeen(pokedex, table[i])) {
                buff[n] = table[i];
                n++;
            }
        }

        if (num != NULL) {
            *num = n;
        }
    }
    return buff;
}

static u16 *BrPokeSearch_LoadMonsTable(HeapID heapId, u32 *num) {
    u32 size;
    u16 *table = GFL_ArcSysReadHeapNewLZGetLen(BR_POKESEARCH_ARC, BR_POKESEARCH_ARC_FILE, FALSE, heapId, &size);

    *num = size / sizeof(u16);
    return table;
}
