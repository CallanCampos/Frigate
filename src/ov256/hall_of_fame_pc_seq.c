#include "types.h"
#include "app/hall_of_fame_pc_bmp.h"
#include "app/hall_of_fame_pc_main.h"
#include "app/hall_of_fame_pc_obj.h"
#include "app/hall_of_fame_pc_seq.h"
#include "app/hall_of_fame_pc_ui.h"
#include "constants/sound.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
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
    HALL_OF_FAME_PC_SEQ_INIT,
    HALL_OF_FAME_PC_SEQ_RELEASE,
    HALL_OF_FAME_PC_SEQ_WIPE,
    HALL_OF_FAME_PC_SEQ_BUTTON_ANIME,
    HALL_OF_FAME_PC_SEQ_PAGE_MAIN,
    HALL_OF_FAME_PC_SEQ_PAGE_CHANGE,
    HALL_OF_FAME_PC_SEQ_POKE_IN,
    HALL_OF_FAME_PC_SEQ_POKE_MAIN,
    HALL_OF_FAME_PC_SEQ_POKE_OUT,
    HALL_OF_FAME_PC_SEQ_POKE_TURN,
    HALL_OF_FAME_PC_SEQ_END_SET,
    HALL_OF_FAME_PC_SEQ_END,
};

typedef u32 (*HallOfFamePCSeqFunc)(HallOfFamePCWork *wk);

static u32 HallOfFamePCSeq_Init(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_Release(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_Wipe(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_ButtonAnime(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_PageMain(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_PageChange(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_PokeIn(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_PokeMain(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_PokeOut(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_PokeTurn(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_EndSet(HallOfFamePCWork *wk);
static u32 HallOfFamePCSeq_SetFadeIn(HallOfFamePCWork *wk, u32 next);
static u32 HallOfFamePCSeq_SetFadeOut(HallOfFamePCWork *wk, u32 next);
static u32 HallOfFamePCSeq_SetButtonAnime(HallOfFamePCWork *wk, u32 id, u32 sequence, u32 next);
static BOOL HallOfFamePCSeq_MovePage(HallOfFamePCWork *wk, s8 move);
static BOOL HallOfFamePCSeq_MovePoke(HallOfFamePCWork *wk, s8 move);
static BOOL HallOfFamePCSeq_IsPokeOnLeft(HallOfFamePCWork *wk, s32 pos);
static u8 HallOfFamePCSeq_CountTurnSteps(HallOfFamePCWork *wk, s8 from, s8 to, s8 direction);
static void HallOfFamePCSeq_SetTurn(HallOfFamePCWork *wk, s8 move);
static BOOL HallOfFamePCSeq_Turn(HallOfFamePCWork *wk);

// How far apart the ring's Pokémon are, in degrees, and how many degrees the ring turns in a frame, by their number
static const u8 sHallOfFamePCSeqTurnAngle[HALL_OF_FAME_TEAM_SIZE] = { 0, 180, 120, 90, 72, 60 };
static const u8 sHallOfFamePCSeqTurnSpeed[HALL_OF_FAME_TEAM_SIZE] = { 5, 5, 5, 5, 4, 4 };

static const HallOfFamePCSeqFunc sHallOfFamePCSeqFuncs[] = {
    HallOfFamePCSeq_Init,     HallOfFamePCSeq_Release,    HallOfFamePCSeq_Wipe,   HallOfFamePCSeq_ButtonAnime,
    HallOfFamePCSeq_PageMain, HallOfFamePCSeq_PageChange, HallOfFamePCSeq_PokeIn, HallOfFamePCSeq_PokeMain,
    HallOfFamePCSeq_PokeOut,  HallOfFamePCSeq_PokeTurn,   HallOfFamePCSeq_EndSet,
};

BOOL HallOfFamePCSeq_Main(HallOfFamePCWork *wk) {
    wk->seq = sHallOfFamePCSeqFuncs[wk->seq](wk);
    if (wk->seq == HALL_OF_FAME_PC_SEQ_END) {
        return FALSE;
    }
    HallOfFamePCObj_Animate(wk);
    HallOfFamePCBmp_PrintUpdate(wk);
    return TRUE;
}

static u32 HallOfFamePCSeq_Init(HallOfFamePCWork *wk) {
    GFL_OvlLoad(OVERLAY_ID(139));

    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    // The main engine drives the touch screen
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_SUB_MAIN);
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);

    HallOfFamePCMain_LoadTeams(wk);
    HallOfFamePCMain_InitVram();
    HallOfFamePCMain_InitBg();
    HallOfFamePCMain_LoadBgGraphic();
    HallOfFamePCMain_InitPaletteFade(wk);
    HallOfFamePCMain_InitMsg(wk);
    HallOfFamePCBmp_Init(wk);
    HallOfFamePCObj_Init(wk);
    HallOfFamePCSeq_PageChange(wk);
    HallOfFamePCMain_SetBlend();
    GFL_NetSetIconScreen(TRUE, HEAPID_HALL_OF_FAME_PC);
    HallOfFamePCMain_InitVBlank(wk);

    return HallOfFamePCSeq_SetFadeIn(wk, HALL_OF_FAME_PC_SEQ_PAGE_MAIN);
}

static u32 HallOfFamePCSeq_Release(HallOfFamePCWork *wk) {
    HallOfFamePCMain_ExitVBlank(wk);
    HallOfFamePCObj_Exit(wk);
    HallOfFamePCBmp_Exit(wk);
    HallOfFamePCMain_ExitMsg(wk);
    HallOfFamePCMain_ExitPaletteFade(wk);
    HallOfFamePCMain_ExitBg();
    HallOfFamePCMain_FreeTeams(wk);

    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);

    GFL_OvlUnload(OVERLAY_ID(139));
    return HALL_OF_FAME_PC_SEQ_END;
}

static u32 HallOfFamePCSeq_Wipe(HallOfFamePCWork *wk) {
    if (PrintQueue_IsEmpty(wk->printQueue) == TRUE && GFL_WipeIsFinished() == TRUE) {
        return wk->wipeNext;
    }
    return HALL_OF_FAME_PC_SEQ_WIPE;
}

static u32 HallOfFamePCSeq_ButtonAnime(HallOfFamePCWork *wk) {
    if (HallOfFamePCObj_IsAnimating(wk, wk->animeObj) == FALSE) {
        return wk->animeNext;
    }
    return HALL_OF_FAME_PC_SEQ_BUTTON_ANIME;
}

static u32 HallOfFamePCSeq_PageMain(HallOfFamePCWork *wk) {
    u32 ret;

    if (PrintQueue_IsEmpty(wk->printQueue) == FALSE) {
        return HALL_OF_FAME_PC_SEQ_PAGE_MAIN;
    }

    ret = HallOfFamePCUi_PageMain(wk);
    switch (ret) {
    case HALL_OF_FAME_PC_UI_LEFT:
        if (HallOfFamePCSeq_MovePage(wk, -1) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SELECT4);
            return HallOfFamePCSeq_SetButtonAnime(wk, HALL_OF_FAME_PC_OBJ_ARROW_LEFT, 12,
                                                  HALL_OF_FAME_PC_SEQ_PAGE_CHANGE);
        }
        break;
    case HALL_OF_FAME_PC_UI_RIGHT:
        if (HallOfFamePCSeq_MovePage(wk, 1) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SELECT4);
            return HallOfFamePCSeq_SetButtonAnime(wk, HALL_OF_FAME_PC_OBJ_ARROW_RIGHT, 13,
                                                  HALL_OF_FAME_PC_SEQ_PAGE_CHANGE);
        }
        break;
    case HALL_OF_FAME_PC_UI_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        wk->param->result = HALL_OF_FAME_PC_RESULT_CLOSE;
        return HallOfFamePCSeq_SetButtonAnime(wk, HALL_OF_FAME_PC_OBJ_CLOSE, 8, HALL_OF_FAME_PC_SEQ_END_SET);
    case HALL_OF_FAME_PC_UI_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->param->result = HALL_OF_FAME_PC_RESULT_RETURN;
        return HallOfFamePCSeq_SetButtonAnime(wk, HALL_OF_FAME_PC_OBJ_RETURN, 9, HALL_OF_FAME_PC_SEQ_END_SET);
    case HALL_OF_FAME_PC_UI_DECIDE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        wk->pokeNext = wk->pokePos;
        return HALL_OF_FAME_PC_SEQ_POKE_IN;
    case HALL_OF_FAME_PC_UI_POKE:
    case HALL_OF_FAME_PC_UI_POKE + 1:
    case HALL_OF_FAME_PC_UI_POKE + 2:
    case HALL_OF_FAME_PC_UI_POKE + 3:
    case HALL_OF_FAME_PC_UI_POKE + 4:
    case HALL_OF_FAME_PC_UI_POKE + 5:
        wk->pokeNext = ret - HALL_OF_FAME_PC_UI_POKE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return HALL_OF_FAME_PC_SEQ_POKE_IN;
    }
    return HALL_OF_FAME_PC_SEQ_PAGE_MAIN;
}

static u32 HallOfFamePCSeq_PageChange(HallOfFamePCWork *wk) {
    wk->pokePos = 0;
    HallOfFamePCObj_LoadPokemon(wk);
    HallOfFamePCBmp_PutTitle(wk);
    HallOfFamePCBmp_PutPage(wk);
    return HALL_OF_FAME_PC_SEQ_PAGE_MAIN;
}

static u32 HallOfFamePCSeq_PokeIn(HallOfFamePCWork *wk) {
    s8 pos;

    switch (wk->subSeq) {
    case 0:
        HallOfFamePCObj_SetAnime(wk, HALL_OF_FAME_PC_OBJ_ARROW_LEFT, 18);
        HallOfFamePCObj_SetAnime(wk, HALL_OF_FAME_PC_OBJ_ARROW_RIGHT, 19);
        HallOfFamePCObj_SetAnime(wk, HALL_OF_FAME_PC_OBJ_CLOSE, 14);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
        HallOfFamePCObj_SetPokeFade(wk, TRUE);
        HallOfFamePCMain_BlendPokePalettes(wk);
        wk->subSeq++;
        break;
    case 1:
        if (HallOfFamePCMain_IsPaletteFading(wk) == FALSE) {
            wk->subSeq = 0;
            pos = wk->pokePos;
            if (wk->pokeNext == pos) {
                wk->pokePos = wk->pokeNext;
                HallOfFamePCBmp_PutPokeInfo(wk);
                return HALL_OF_FAME_PC_SEQ_POKE_MAIN;
            }
            wk->pokePos = wk->pokeNext;
            if (HallOfFamePCSeq_IsPokeOnLeft(wk, wk->pokeNext) == TRUE) {
                wk->speed = -sHallOfFamePCSeqTurnSpeed[wk->teams[wk->page].count - 1];
                HallOfFamePCSeq_SetTurn(wk, -HallOfFamePCSeq_CountTurnSteps(wk, pos, wk->pokeNext, 1));
            } else {
                wk->speed = sHallOfFamePCSeqTurnSpeed[wk->teams[wk->page].count - 1];
                HallOfFamePCSeq_SetTurn(wk, HallOfFamePCSeq_CountTurnSteps(wk, pos, wk->pokeNext, -1));
            }
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return HALL_OF_FAME_PC_SEQ_POKE_TURN;
        }
        break;
    }
    return HALL_OF_FAME_PC_SEQ_POKE_IN;
}

static u32 HallOfFamePCSeq_PokeMain(HallOfFamePCWork *wk) {
    u32 ret;
    s8 pos;

    if (PrintQueue_IsEmpty(wk->printQueue) == FALSE) {
        return HALL_OF_FAME_PC_SEQ_POKE_MAIN;
    }

    ret = HallOfFamePCUi_PokeMain(wk);
    switch (ret) {
    case HALL_OF_FAME_PC_UI_LEFT:
        if (HallOfFamePCSeq_MovePoke(wk, 1) == TRUE) {
            wk->speed = -sHallOfFamePCSeqTurnSpeed[wk->teams[wk->page].count - 1];
            HallOfFamePCSeq_SetTurn(wk, -1);
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return HALL_OF_FAME_PC_SEQ_POKE_TURN;
        }
        break;
    case HALL_OF_FAME_PC_UI_RIGHT:
        if (HallOfFamePCSeq_MovePoke(wk, -1) == TRUE) {
            wk->speed = sHallOfFamePCSeqTurnSpeed[wk->teams[wk->page].count - 1];
            HallOfFamePCSeq_SetTurn(wk, 1);
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return HALL_OF_FAME_PC_SEQ_POKE_TURN;
        }
        break;
    case HALL_OF_FAME_PC_UI_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return HallOfFamePCSeq_SetButtonAnime(wk, HALL_OF_FAME_PC_OBJ_RETURN, 9, HALL_OF_FAME_PC_SEQ_POKE_OUT);
    case HALL_OF_FAME_PC_UI_POKE:
    case HALL_OF_FAME_PC_UI_POKE + 1:
    case HALL_OF_FAME_PC_UI_POKE + 2:
    case HALL_OF_FAME_PC_UI_POKE + 3:
    case HALL_OF_FAME_PC_UI_POKE + 4:
    case HALL_OF_FAME_PC_UI_POKE + 5:
        pos = wk->pokePos;
        if (ret - HALL_OF_FAME_PC_UI_POKE != pos) {
            wk->pokePos = ret - HALL_OF_FAME_PC_UI_POKE;
            if (HallOfFamePCSeq_IsPokeOnLeft(wk, ret - HALL_OF_FAME_PC_UI_POKE) == TRUE) {
                wk->speed = -sHallOfFamePCSeqTurnSpeed[wk->teams[wk->page].count - 1];
                HallOfFamePCSeq_SetTurn(wk, -HallOfFamePCSeq_CountTurnSteps(wk, pos, ret - HALL_OF_FAME_PC_UI_POKE, 1));
            } else {
                wk->speed = sHallOfFamePCSeqTurnSpeed[wk->teams[wk->page].count - 1];
                HallOfFamePCSeq_SetTurn(wk, HallOfFamePCSeq_CountTurnSteps(wk, pos, ret - HALL_OF_FAME_PC_UI_POKE, -1));
            }
            GFL_SndSEPlay(SEQ_SE_TRIPLE_S);
            return HALL_OF_FAME_PC_SEQ_POKE_TURN;
        }
        break;
    case HALL_OF_FAME_PC_UI_CLOSE:
    case HALL_OF_FAME_PC_UI_DECIDE:
        break;
    }
    return HALL_OF_FAME_PC_SEQ_POKE_MAIN;
}

static u32 HallOfFamePCSeq_PokeOut(HallOfFamePCWork *wk) {
    switch (wk->subSeq) {
    case 0:
        HallOfFamePCObj_SetPokeFade(wk, FALSE);
        HallOfFamePCMain_BlendPokePalettes(wk);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
        wk->subSeq++;
        break;
    case 1:
        if (HallOfFamePCMain_IsPaletteFading(wk) == FALSE) {
            HallOfFamePCObj_SetAnime(wk, HALL_OF_FAME_PC_OBJ_ARROW_LEFT, 4);
            HallOfFamePCObj_SetAnime(wk, HALL_OF_FAME_PC_OBJ_ARROW_RIGHT, 5);
            HallOfFamePCObj_SetAnime(wk, HALL_OF_FAME_PC_OBJ_CLOSE, 0);
            HallOfFamePCBmp_ClearPokeInfo(wk);
            wk->subSeq = 0;
            return HALL_OF_FAME_PC_SEQ_PAGE_MAIN;
        }
        break;
    }
    return HALL_OF_FAME_PC_SEQ_POKE_OUT;
}

static u32 HallOfFamePCSeq_PokeTurn(HallOfFamePCWork *wk) {
    u32 i;

    if (HallOfFamePCSeq_Turn(wk) == FALSE) {
        for (i = 0; i < wk->teams[wk->page].count; i++) {
            wk->fade[i] = wk->fadeNext[i];
        }
        HallOfFamePCMain_IsPaletteFading(wk);
        HallOfFamePCBmp_PutPokeInfo(wk);
        return HALL_OF_FAME_PC_SEQ_POKE_MAIN;
    }

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        if (wk->fadeNow[i] > wk->fadeNext[i]) {
            wk->fadeNow[i]--;
        } else if (wk->fadeNow[i] < wk->fadeNext[i]) {
            wk->fadeNow[i]++;
        }
    }
    HallOfFamePCMain_BlendPokePalettes(wk);
    return HALL_OF_FAME_PC_SEQ_POKE_TURN;
}

static u32 HallOfFamePCSeq_EndSet(HallOfFamePCWork *wk) {
    return HallOfFamePCSeq_SetFadeOut(wk, HALL_OF_FAME_PC_SEQ_RELEASE);
}

static u32 HallOfFamePCSeq_SetFadeIn(HallOfFamePCWork *wk, u32 next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_HALL_OF_FAME_PC);
    wk->wipeNext = next;
    return HALL_OF_FAME_PC_SEQ_WIPE;
}

static u32 HallOfFamePCSeq_SetFadeOut(HallOfFamePCWork *wk, u32 next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_HALL_OF_FAME_PC);
    wk->wipeNext = next;
    return HALL_OF_FAME_PC_SEQ_WIPE;
}

static u32 HallOfFamePCSeq_SetButtonAnime(HallOfFamePCWork *wk, u32 id, u32 sequence, u32 next) {
    HallOfFamePCObj_SetAnime(wk, id, sequence);
    wk->animeObj = id;
    wk->animeNext = next;
    return HALL_OF_FAME_PC_SEQ_BUTTON_ANIME;
}

// Moves to another page, around the ends, and returns whether the page changed
static BOOL HallOfFamePCSeq_MovePage(HallOfFamePCWork *wk, s8 move) {
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

static BOOL HallOfFamePCSeq_MovePoke(HallOfFamePCWork *wk, s8 move) {
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

static BOOL HallOfFamePCSeq_IsPokeOnLeft(HallOfFamePCWork *wk, s32 pos) {
    s16 x, y;

    HallOfFamePCObj_GetPos(wk, pos + HallOfFamePCObj_GetPokeBase(wk), &x, &y);
    if (x < 128) {
        return TRUE;
    }
    return FALSE;
}

// How many places apart two Pokémon are, going around the ring in a direction
static u8 HallOfFamePCSeq_CountTurnSteps(HallOfFamePCWork *wk, s8 from, s8 to, s8 direction) {
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
static void HallOfFamePCSeq_SetTurn(HallOfFamePCWork *wk, s8 move) {
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
    frames = sHallOfFamePCSeqTurnAngle[wk->teams[wk->page].count - 1] / speed;
    wk->stepCount = frames * places;
}

// Turns the ring a step, and returns whether it is still turning
static BOOL HallOfFamePCSeq_Turn(HallOfFamePCWork *wk) {
    u32 i;
    u32 base = HallOfFamePCObj_GetPokeBase(wk);

    if (wk->stepCount == 0) {
        for (i = 0; i < wk->teams[wk->page].count; i++) {
            wk->angle[i] = wk->angleNext[i];
            HallOfFamePCObj_SetPokePos(wk, base + i, wk->angle[i]);
        }
        HallOfFamePCObj_SortPriority(wk);
        return FALSE;
    }

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        wk->angle[i] += wk->speed;
        if (wk->angle[i] < 0) {
            wk->angle[i] += 360;
        } else if (wk->angle[i] >= 360) {
            wk->angle[i] -= 360;
        }
        HallOfFamePCObj_SetPokePos(wk, base + i, wk->angle[i]);
    }
    HallOfFamePCObj_SortPriority(wk);
    wk->stepCount--;
    return TRUE;
}
