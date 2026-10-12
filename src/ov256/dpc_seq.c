#include "types.h"
#include "app/dpc_seq.h"
#include "app/dpc_bmp.h"
#include "app/dpc_main.h"
#include "app/dpc_obj.h"
#include "app/dpc_ui.h"
#include "constants/sound.h"
#include "gfl/gx_layers.h"
#include "gfl/graphics.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The Hall of Fame PC's sequences: the pages of teams, and a page's Pokémon turning on their ring to bring the
// chosen one to the front. The ROM doesn't name this file; the name is a guess, after box2_seq.c

enum {
    DPCSEQ_INIT,
    DPCSEQ_RELEASE,
    DPCSEQ_WIPE,
    DPCSEQ_BUTTON_ANIME,
    DPCSEQ_PAGE_MAIN,
    DPCSEQ_PAGE_CHANGE,
    DPCSEQ_POKE_IN,
    DPCSEQ_POKE_MAIN,
    DPCSEQ_POKE_OUT,
    DPCSEQ_POKE_TURN,
    DPCSEQ_END_SET,
    DPCSEQ_END,
};

typedef u32 (*DpcSeqFunc)(DendouPCWork *wk);

static u32 DpcSeq_Init(DendouPCWork *wk);
static u32 DpcSeq_Release(DendouPCWork *wk);
static u32 DpcSeq_Wipe(DendouPCWork *wk);
static u32 DpcSeq_ButtonAnime(DendouPCWork *wk);
static u32 DpcSeq_PageMain(DendouPCWork *wk);
static u32 DpcSeq_PageChange(DendouPCWork *wk);
static u32 DpcSeq_PokeIn(DendouPCWork *wk);
static u32 DpcSeq_PokeMain(DendouPCWork *wk);
static u32 DpcSeq_PokeOut(DendouPCWork *wk);
static u32 DpcSeq_PokeTurn(DendouPCWork *wk);
static u32 DpcSeq_EndSet(DendouPCWork *wk);
static u32 DpcSeq_SetFadeIn(DendouPCWork *wk, u32 next);
static u32 DpcSeq_SetFadeOut(DendouPCWork *wk, u32 next);
static u32 DpcSeq_SetButtonAnime(DendouPCWork *wk, u32 id, u32 sequence, u32 next);
static BOOL DpcSeq_MovePage(DendouPCWork *wk, s8 move);
static BOOL DpcSeq_MovePoke(DendouPCWork *wk, s8 move);
static BOOL DpcSeq_IsPokeOnLeft(DendouPCWork *wk, s32 pos);
static u8 DpcSeq_CountTurnSteps(DendouPCWork *wk, s8 from, s8 to, s8 direction);
static void DpcSeq_SetTurn(DendouPCWork *wk, s8 move);
static BOOL DpcSeq_Turn(DendouPCWork *wk);

// How far apart the ring's Pokémon are, in degrees, and how many degrees the ring turns in a frame, by their number
static const u8 sDpcSeqTurnAngle[HALL_OF_FAME_TEAM_SIZE] = { 0, 180, 120, 90, 72, 60 };
static const u8 sDpcSeqTurnSpeed[HALL_OF_FAME_TEAM_SIZE] = { 5, 5, 5, 5, 4, 4 };

static const DpcSeqFunc sDpcSeqFuncs[] = {
    DpcSeq_Init,   DpcSeq_Release,  DpcSeq_Wipe,    DpcSeq_ButtonAnime, DpcSeq_PageMain, DpcSeq_PageChange,
    DpcSeq_PokeIn, DpcSeq_PokeMain, DpcSeq_PokeOut, DpcSeq_PokeTurn,    DpcSeq_EndSet,
};

BOOL DpcSeq_Main(DendouPCWork *wk) {
    wk->seq = sDpcSeqFuncs[wk->seq](wk);
    if (wk->seq == DPCSEQ_END) {
        return FALSE;
    }
    DpcObj_Animate(wk);
    DpcBmp_PrintUpdate(wk);
    return TRUE;
}

static u32 DpcSeq_Init(DendouPCWork *wk) {
    GFL_OvlLoad(OVERLAY_ID(139));

    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    // The main engine drives the touch screen
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_SUB_MAIN);
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);

    DpcMain_LoadTeams(wk);
    DpcMain_InitVram();
    DpcMain_InitBg();
    DpcMain_LoadBgGraphic();
    DpcMain_InitPaletteFade(wk);
    DpcMain_InitMsg(wk);
    DpcBmp_Init(wk);
    DpcObj_Init(wk);
    DpcSeq_PageChange(wk);
    DpcMain_SetBlend();
    func_02042ba8(TRUE, HEAPID_DENDOU_PC);
    DpcMain_InitVBlank(wk);

    return DpcSeq_SetFadeIn(wk, DPCSEQ_PAGE_MAIN);
}

static u32 DpcSeq_Release(DendouPCWork *wk) {
    DpcMain_ExitVBlank(wk);
    DpcObj_Exit(wk);
    DpcBmp_Exit(wk);
    DpcMain_ExitMsg(wk);
    DpcMain_ExitPaletteFade(wk);
    DpcMain_ExitBg();
    DpcMain_FreeTeams(wk);

    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);

    GFL_OvlUnload(OVERLAY_ID(139));
    return DPCSEQ_END;
}

static u32 DpcSeq_Wipe(DendouPCWork *wk) {
    if (func_02021c0c(wk->printQueue) == TRUE && GFL_WipeIsFinished() == TRUE) {
        return wk->wipeNext;
    }
    return DPCSEQ_WIPE;
}

static u32 DpcSeq_ButtonAnime(DendouPCWork *wk) {
    if (DpcObj_IsAnimating(wk, wk->animeObj) == FALSE) {
        return wk->animeNext;
    }
    return DPCSEQ_BUTTON_ANIME;
}

static u32 DpcSeq_PageMain(DendouPCWork *wk) {
    u32 ret;

    if (func_02021c0c(wk->printQueue) == FALSE) {
        return DPCSEQ_PAGE_MAIN;
    }

    ret = DpcUi_PageMain(wk);
    switch (ret) {
    case DPCUI_LEFT:
        if (DpcSeq_MovePage(wk, -1) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SELECT4);
            return DpcSeq_SetButtonAnime(wk, DPCOBJ_ARROW_LEFT, 12, DPCSEQ_PAGE_CHANGE);
        }
        break;
    case DPCUI_RIGHT:
        if (DpcSeq_MovePage(wk, 1) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SELECT4);
            return DpcSeq_SetButtonAnime(wk, DPCOBJ_ARROW_RIGHT, 13, DPCSEQ_PAGE_CHANGE);
        }
        break;
    case DPCUI_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        wk->param->result = DENDOU_PC_RESULT_CLOSE;
        return DpcSeq_SetButtonAnime(wk, DPCOBJ_CLOSE, 8, DPCSEQ_END_SET);
    case DPCUI_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->param->result = DENDOU_PC_RESULT_RETURN;
        return DpcSeq_SetButtonAnime(wk, DPCOBJ_RETURN, 9, DPCSEQ_END_SET);
    case DPCUI_DECIDE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        wk->pokeNext = wk->pokePos;
        return DPCSEQ_POKE_IN;
    case DPCUI_POKE:
    case DPCUI_POKE + 1:
    case DPCUI_POKE + 2:
    case DPCUI_POKE + 3:
    case DPCUI_POKE + 4:
    case DPCUI_POKE + 5:
        wk->pokeNext = ret - DPCUI_POKE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return DPCSEQ_POKE_IN;
    }
    return DPCSEQ_PAGE_MAIN;
}

static u32 DpcSeq_PageChange(DendouPCWork *wk) {
    wk->pokePos = 0;
    DpcObj_LoadPokemon(wk);
    DpcBmp_PutTitle(wk);
    DpcBmp_PutPage(wk);
    return DPCSEQ_PAGE_MAIN;
}

static u32 DpcSeq_PokeIn(DendouPCWork *wk) {
    s8 pos;

    switch (wk->subSeq) {
    case 0:
        DpcObj_SetAnime(wk, DPCOBJ_ARROW_LEFT, 18);
        DpcObj_SetAnime(wk, DPCOBJ_ARROW_RIGHT, 19);
        DpcObj_SetAnime(wk, DPCOBJ_CLOSE, 14);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
        DpcObj_SetPokeFade(wk, TRUE);
        DpcMain_BlendPokePalettes(wk);
        wk->subSeq++;
        break;
    case 1:
        if (DpcMain_IsPaletteFading(wk) == FALSE) {
            wk->subSeq = 0;
            pos = wk->pokePos;
            if (wk->pokeNext == pos) {
                wk->pokePos = wk->pokeNext;
                DpcBmp_PutPokeInfo(wk);
                return DPCSEQ_POKE_MAIN;
            }
            wk->pokePos = wk->pokeNext;
            if (DpcSeq_IsPokeOnLeft(wk, wk->pokeNext) == TRUE) {
                wk->speed = -sDpcSeqTurnSpeed[wk->teams[wk->page].count - 1];
                DpcSeq_SetTurn(wk, -DpcSeq_CountTurnSteps(wk, pos, wk->pokeNext, 1));
            } else {
                wk->speed = sDpcSeqTurnSpeed[wk->teams[wk->page].count - 1];
                DpcSeq_SetTurn(wk, DpcSeq_CountTurnSteps(wk, pos, wk->pokeNext, -1));
            }
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return DPCSEQ_POKE_TURN;
        }
        break;
    }
    return DPCSEQ_POKE_IN;
}

static u32 DpcSeq_PokeMain(DendouPCWork *wk) {
    u32 ret;
    s8 pos;

    if (func_02021c0c(wk->printQueue) == FALSE) {
        return DPCSEQ_POKE_MAIN;
    }

    ret = DpcUi_PokeMain(wk);
    switch (ret) {
    case DPCUI_LEFT:
        if (DpcSeq_MovePoke(wk, 1) == TRUE) {
            wk->speed = -sDpcSeqTurnSpeed[wk->teams[wk->page].count - 1];
            DpcSeq_SetTurn(wk, -1);
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return DPCSEQ_POKE_TURN;
        }
        break;
    case DPCUI_RIGHT:
        if (DpcSeq_MovePoke(wk, -1) == TRUE) {
            wk->speed = sDpcSeqTurnSpeed[wk->teams[wk->page].count - 1];
            DpcSeq_SetTurn(wk, 1);
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return DPCSEQ_POKE_TURN;
        }
        break;
    case DPCUI_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return DpcSeq_SetButtonAnime(wk, DPCOBJ_RETURN, 9, DPCSEQ_POKE_OUT);
    case DPCUI_POKE:
    case DPCUI_POKE + 1:
    case DPCUI_POKE + 2:
    case DPCUI_POKE + 3:
    case DPCUI_POKE + 4:
    case DPCUI_POKE + 5:
        pos = wk->pokePos;
        if (ret - DPCUI_POKE != pos) {
            wk->pokePos = ret - DPCUI_POKE;
            if (DpcSeq_IsPokeOnLeft(wk, ret - DPCUI_POKE) == TRUE) {
                wk->speed = -sDpcSeqTurnSpeed[wk->teams[wk->page].count - 1];
                DpcSeq_SetTurn(wk, -DpcSeq_CountTurnSteps(wk, pos, ret - DPCUI_POKE, 1));
            } else {
                wk->speed = sDpcSeqTurnSpeed[wk->teams[wk->page].count - 1];
                DpcSeq_SetTurn(wk, DpcSeq_CountTurnSteps(wk, pos, ret - DPCUI_POKE, -1));
            }
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return DPCSEQ_POKE_TURN;
        }
        break;
    case DPCUI_CLOSE:
    case DPCUI_DECIDE:
        break;
    }
    return DPCSEQ_POKE_MAIN;
}

static u32 DpcSeq_PokeOut(DendouPCWork *wk) {
    switch (wk->subSeq) {
    case 0:
        DpcObj_SetPokeFade(wk, FALSE);
        DpcMain_BlendPokePalettes(wk);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
        wk->subSeq++;
        break;
    case 1:
        if (DpcMain_IsPaletteFading(wk) == FALSE) {
            DpcObj_SetAnime(wk, DPCOBJ_ARROW_LEFT, 4);
            DpcObj_SetAnime(wk, DPCOBJ_ARROW_RIGHT, 5);
            DpcObj_SetAnime(wk, DPCOBJ_CLOSE, 0);
            DpcBmp_ClearPokeInfo(wk);
            wk->subSeq = 0;
            return DPCSEQ_PAGE_MAIN;
        }
        break;
    }
    return DPCSEQ_POKE_OUT;
}

static u32 DpcSeq_PokeTurn(DendouPCWork *wk) {
    u32 i;

    if (DpcSeq_Turn(wk) == FALSE) {
        for (i = 0; i < wk->teams[wk->page].count; i++) {
            wk->fade[i] = wk->fadeNext[i];
        }
        DpcMain_IsPaletteFading(wk);
        DpcBmp_PutPokeInfo(wk);
        return DPCSEQ_POKE_MAIN;
    }

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        if (wk->fadeNow[i] > wk->fadeNext[i]) {
            wk->fadeNow[i]--;
        } else if (wk->fadeNow[i] < wk->fadeNext[i]) {
            wk->fadeNow[i]++;
        }
    }
    DpcMain_BlendPokePalettes(wk);
    return DPCSEQ_POKE_TURN;
}

static u32 DpcSeq_EndSet(DendouPCWork *wk) {
    return DpcSeq_SetFadeOut(wk, DPCSEQ_RELEASE);
}

static u32 DpcSeq_SetFadeIn(DendouPCWork *wk, u32 next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_DENDOU_PC);
    wk->wipeNext = next;
    return DPCSEQ_WIPE;
}

static u32 DpcSeq_SetFadeOut(DendouPCWork *wk, u32 next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_DENDOU_PC);
    wk->wipeNext = next;
    return DPCSEQ_WIPE;
}

static u32 DpcSeq_SetButtonAnime(DendouPCWork *wk, u32 id, u32 sequence, u32 next) {
    DpcObj_SetAnime(wk, id, sequence);
    wk->animeObj = id;
    wk->animeNext = next;
    return DPCSEQ_BUTTON_ANIME;
}

// Moves to another page, around the ends, and returns whether the page changed
static BOOL DpcSeq_MovePage(DendouPCWork *wk, s8 move) {
    s8 old = wk->page;

    wk->page += move;
    if (wk->page < 0) {
        wk->page = wk->pageMax - 1;
    } else if (wk->page >= wk->pageMax) {
        wk->page = 0;
    }
    if (wk->page != old) {
        return TRUE;
    }
    return FALSE;
}

static BOOL DpcSeq_MovePoke(DendouPCWork *wk, s8 move) {
    s8 old = wk->pokePos;

    wk->pokePos += move;
    if (wk->pokePos < 0) {
        wk->pokePos = wk->teams[wk->page].count - 1;
    } else if (wk->pokePos >= wk->teams[wk->page].count) {
        wk->pokePos = 0;
    }
    if (wk->pokePos != old) {
        return TRUE;
    }
    return FALSE;
}

static BOOL DpcSeq_IsPokeOnLeft(DendouPCWork *wk, s32 pos) {
    s16 x, y;

    DpcObj_GetPos(wk, pos + DpcObj_GetPokeBase(wk), &x, &y);
    if (x < 128) {
        return TRUE;
    }
    return FALSE;
}

// How many places apart two Pokémon are, going around the ring in a direction
static u8 DpcSeq_CountTurnSteps(DendouPCWork *wk, s8 from, s8 to, s8 direction) {
    u8 i;

    for (i = 1; i < HALL_OF_FAME_TEAM_SIZE; i++) {
        from += direction;
        if (from < 0) {
            from = wk->teams[wk->page].count - 1;
        } else if (from >= wk->teams[wk->page].count) {
            from = 0;
        }
        if (from == to) {
            break;
        }
    }
    return i;
}

// Starts turning the ring by a number of places: each Pokémon goes to where the one that many places on is
static void DpcSeq_SetTurn(DendouPCWork *wk, s8 move) {
    u32 i;
    s16 pos = wk->pokePos;
    s16 next = pos + move;
    s32 speed, places, frames;

    if (next < 0) {
        next += wk->teams[wk->page].count;
    } else if (next >= wk->teams[wk->page].count) {
        next -= wk->teams[wk->page].count;
    }

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        wk->angleNext[pos] = wk->angle[next];
        wk->fadeNext[pos] = wk->fade[next];

        pos += move / MATH_ABS(move);
        if (pos < 0) {
            pos = wk->teams[wk->page].count - 1;
        } else if (pos >= wk->teams[wk->page].count) {
            pos = 0;
        }
        next += move / MATH_ABS(move);
        if (next < 0) {
            next = wk->teams[wk->page].count - 1;
        } else if (next >= wk->teams[wk->page].count) {
            next = 0;
        }
    }

    // The frames a place takes, for each place
    speed = MATH_ABS(wk->speed);
    places = MATH_ABS(move);
    frames = sDpcSeqTurnAngle[wk->teams[wk->page].count - 1] / speed;
    wk->stepCount = frames * places;
}

// Turns the ring a step, and returns whether it is still turning
static BOOL DpcSeq_Turn(DendouPCWork *wk) {
    u32 i;
    u32 base = DpcObj_GetPokeBase(wk);

    if (wk->stepCount == 0) {
        for (i = 0; i < wk->teams[wk->page].count; i++) {
            wk->angle[i] = wk->angleNext[i];
            DpcObj_SetPokePos(wk, base + i, wk->angle[i]);
        }
        DpcObj_SortPriority(wk);
        return FALSE;
    }

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        wk->angle[i] += wk->speed;
        if (wk->angle[i] < 0) {
            wk->angle[i] += 360;
        } else if (wk->angle[i] >= 360) {
            wk->angle[i] -= 360;
        }
        DpcObj_SetPokePos(wk, base + i, wk->angle[i]);
    }
    DpcObj_SortPriority(wk);
    wk->stepCount--;
    return TRUE;
}
