// The Battle Recorder's fades between its screens: by the master brightness, by alpha blending the BGs, or by
// fading the palettes to the color the player chose. The name is the ROM's string, from GFL_HeapAllocate's asserts

#include "types.h"
#include "app/battle_recorder/br_fade.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/palanm.h"

// The main screen's BGs blend over everything but the backdrop
#define BR_FADE_BLEND_ALL                                                                                              \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |               \
     GX_BLEND_PLANEMASK_OBJ)

typedef BOOL (*BrFadeFunc)(BrFade *p_wk, u32 *p_seq);

struct BrFade {
    BOOL is_end;
    u32 display;
    u32 dir;
    u32 seq;
    u32 cnt;
    u32 cnt_max;
    u32 sync;
    BrFadeFunc fade_func;
    PaletteFade *pfd;
    u16 fade_color;
    u8 unk26[0x16];
    u32 unk3C;
};

static BOOL BrFade_Fade_MasterBright_Black(BrFade *p_wk, u32 *p_seq);
static BOOL BrFade_Fade_MasterBright_White(BrFade *p_wk, u32 *p_seq);
static BOOL BrFade_Fade_Alpha_BG012(BrFade *p_wk, u32 *p_seq);
static BOOL BrFade_Fade_Pltt(BrFade *p_wk, u32 *p_seq);
static BOOL BrFade_Fade_MasterBrightAndAlpha(BrFade *p_wk, u32 *p_seq);
static BrFadeFunc BrFade_GetFadeFunc(u32 type);

BrFade *BrFade_Init(HeapID heapId) {
    BrFade *p_wk = GFL_HeapAllocate(heapId, sizeof(BrFade), FALSE, "br_fade.c", 98);

    MI_CpuFill8(p_wk, 0, sizeof(BrFade));
    p_wk->unk3C = 0xffff;
    p_wk->pfd = PaletteFade_Create(heapId);
    PaletteFade_AllocBuffer(p_wk->pfd, PALFADE_BUFFER_MAIN_BG, 0x200, heapId);
    PaletteFade_AllocBuffer(p_wk->pfd, PALFADE_BUFFER_MAIN_OBJ, 0x1c0, heapId);
    PaletteFade_AllocBuffer(p_wk->pfd, PALFADE_BUFFER_SUB_BG, 0x200, heapId);
    PaletteFade_AllocBuffer(p_wk->pfd, PALFADE_BUFFER_SUB_OBJ, 0x1c0, heapId);
    PaletteFade_SetTransferAll(p_wk->pfd, TRUE);
    return p_wk;
}

void BrFade_Exit(BrFade *p_wk) {
    PaletteFade_FreeBuffer(p_wk->pfd, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(p_wk->pfd, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(p_wk->pfd, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_FreeBuffer(p_wk->pfd, PALFADE_BUFFER_SUB_BG);
    PaletteFade_Free(p_wk->pfd);
    GFL_HeapFree(p_wk);
}

void BrFade_Main(BrFade *p_wk) {
    if (p_wk->fade_func != NULL) {
        p_wk->is_end = p_wk->fade_func(p_wk, &p_wk->seq);
        if (p_wk->is_end) {
            p_wk->fade_func = NULL;
        }
    }
}

void BrFade_StartFade(BrFade *p_wk, u32 type, u32 display, u32 dir) {
    BrFade_StartFadeEx(p_wk, type, display, dir, 0);
}

void BrFade_StartFadeEx(BrFade *p_wk, u32 type, u32 display, u32 dir, u32 sync) {
    p_wk->display = display;
    p_wk->sync = sync;
    p_wk->dir = dir;
    p_wk->fade_func = BrFade_GetFadeFunc(type);
    p_wk->is_end = FALSE;
    p_wk->seq = 0;
}

BOOL BrFade_IsEnd(const BrFade *cp_wk) {
    return cp_wk->is_end;
}

void BrFade_LoadPltt(BrFade *p_wk) {
    PaletteFade_LoadFromVRAM(p_wk->pfd, PALFADE_VRAM_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(p_wk->pfd, PALFADE_VRAM_MAIN_OBJ, 0, 0x1c0);
    PaletteFade_LoadFromVRAM(p_wk->pfd, PALFADE_VRAM_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(p_wk->pfd, PALFADE_VRAM_SUB_OBJ, 0, 0x1c0);
}

void BrFade_SetColor(BrFade *p_wk, u16 color) {
    p_wk->fade_color = color;
}

void BrFade_FillColor(BrFade *p_wk, u32 display) {
    if (display & BR_FADE_DISPLAY_MAIN) {
        PaletteFade_BlendPalettes(p_wk->pfd, PALFADE_BUFFER_MAIN_BG, 0xbfff, 16, p_wk->fade_color);
    }
    if (display & BR_FADE_DISPLAY_SUB) {
        PaletteFade_BlendPalettes(p_wk->pfd, PALFADE_BUFFER_SUB_BG, 0xbfff, 16, p_wk->fade_color);
    }
    PaletteFade_SetAllActive(p_wk->pfd, TRUE);
    PaletteFade_Transfer(p_wk->pfd);
}

void BrFade_SetAlpha(BrFade *p_wk, u32 display, u32 ev) {
    if (display & BR_FADE_DISPLAY_MAIN) {
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2,
                           BR_FADE_BLEND_ALL, ev, 16 - ev);
        if (ev == 0) {
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, FALSE);
        } else {
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, TRUE);
        }
    }
    if (display & BR_FADE_DISPLAY_SUB) {
        G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1,
                           GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2, ev, 16 - ev);
        if (ev == 0) {
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
        } else {
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, TRUE);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
        }
    }
}

void BrFade_LoadPlttArc(BrFade *p_wk, ArcTool *handle, u32 fileId, u32 buffer, u32 offset, u32 size, HeapID heapId) {
    PaletteFade_LoadArcNCLREx(p_wk->pfd, handle, fileId, heapId, buffer, size, offset, 0);
}

static BOOL BrFade_Fade_MasterBright_Black(BrFade *p_wk, u32 *p_seq) {
    switch (*p_seq) {
    case 0: {
        u32 mode = 0;
        s32 start, end;

        if (p_wk->display & BR_FADE_DISPLAY_MAIN) {
            mode |= FADE_ENGINE_A_BLACK;
        }
        if (p_wk->display & BR_FADE_DISPLAY_SUB) {
            mode |= FADE_ENGINE_B_BLACK;
        }
        if (p_wk->dir == BR_FADE_DIR_IN) {
            start = 16;
            end = 0;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            start = 0;
            end = 16;
        }
        GFL_FadeSet(mode, start, end, p_wk->sync);
        *p_seq = 1;
        break;
    }
    case 1:
        if (!GFL_FadeIsRunning()) {
            *p_seq = 2;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL BrFade_Fade_MasterBright_White(BrFade *p_wk, u32 *p_seq) {
    switch (*p_seq) {
    case 0: {
        u32 mode = 0;
        s32 start, end;

        if (p_wk->display & BR_FADE_DISPLAY_MAIN) {
            mode |= FADE_ENGINE_A_WHITE;
        }
        if (p_wk->display & BR_FADE_DISPLAY_SUB) {
            mode |= FADE_ENGINE_B_WHITE;
        }
        if (p_wk->dir == BR_FADE_DIR_IN) {
            start = 16;
            end = 0;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            start = 0;
            end = 16;
        }
        GFL_FadeSet(mode, start, end, p_wk->sync);
        *p_seq = 1;
        break;
    }
    case 1:
        if (!GFL_FadeIsRunning()) {
            *p_seq = 2;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

// ev1 and ev2 are the alphas of the first and second targets when the blend is set up, and the other way round when
// it steps
static BOOL BrFade_Fade_Alpha_BG012(BrFade *p_wk, u32 *p_seq) {
    s32 ev1, ev2;

    switch (*p_seq) {
    case 0:
        if (p_wk->dir == BR_FADE_DIR_IN) {
            ev1 = 0;
            ev2 = 16;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            ev1 = 16;
            ev2 = 0;
        }
        if (p_wk->display & BR_FADE_DISPLAY_MAIN) {
            G2x_SetBlendAlpha_(REG_BLDCNT_ADDR,
                               GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2,
                               BR_FADE_BLEND_ALL, ev1, ev2);
        }
        if (p_wk->display & BR_FADE_DISPLAY_SUB) {
            G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1,
                               GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2, ev1, ev2);
        }
        p_wk->cnt = 0;
        p_wk->cnt_max = p_wk->sync != 0 ? p_wk->sync : 16 / GFL_FadeGetUpdateFreq();
        *p_seq = 1;
        break;
    case 1:
        if (p_wk->dir == BR_FADE_DIR_IN) {
            ev2 = p_wk->cnt * 16 / p_wk->cnt_max;
            ev1 = 16 - ev2;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            ev1 = p_wk->cnt * 16 / p_wk->cnt_max;
            ev2 = 16 - ev1;
        }
        if (p_wk->display & BR_FADE_DISPLAY_MAIN) {
            G2_ChangeBlendAlpha(ev2, ev1);
            if (ev2 == 0) {
                GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
                GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
                GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, FALSE);
            } else {
                GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
                GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
                GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, TRUE);
            }
        }
        if (p_wk->display & BR_FADE_DISPLAY_SUB) {
            G2S_ChangeBlendAlpha(ev2, ev1);
            if (ev2 == 0) {
                GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
                GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
            } else {
                GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, TRUE);
                GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
            }
        }
        if (p_wk->cnt++ >= p_wk->cnt_max) {
            *p_seq = 2;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL BrFade_Fade_Pltt(BrFade *p_wk, u32 *p_seq) {
    u8 evy;

    switch (*p_seq) {
    case 0:
        p_wk->cnt = 0;
        p_wk->cnt_max = p_wk->sync != 0 ? p_wk->sync : 16 / GFL_FadeGetUpdateFreq();
        PaletteFade_SetAllActive(p_wk->pfd, TRUE);
        *p_seq = 1;
        break;
    case 1:
        if (p_wk->dir == BR_FADE_DIR_IN) {
            evy = 16 - p_wk->cnt * 16 / p_wk->cnt_max;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            evy = p_wk->cnt * 16 / p_wk->cnt_max;
        }
        // Not one of the BR_FADE_DISPLAY_* masks: sub BG palettes 12 and 13 to white
        if (p_wk->display == 0xf) {
            PaletteFade_BlendPalettes(p_wk->pfd, PALFADE_BUFFER_SUB_BG, 0x3000, evy, 0xffff);
        } else {
            if (p_wk->display & BR_FADE_DISPLAY_MAIN) {
                PaletteFade_BlendPalettes(p_wk->pfd, PALFADE_BUFFER_MAIN_OBJ, 0x3fff, evy, p_wk->fade_color);
                PaletteFade_BlendPalettes(p_wk->pfd, PALFADE_BUFFER_MAIN_BG, 0xbfff, evy, p_wk->fade_color);
            }
            if (p_wk->display & BR_FADE_DISPLAY_SUB) {
                PaletteFade_BlendPalettes(p_wk->pfd, PALFADE_BUFFER_SUB_OBJ, 0x3fff, evy, p_wk->fade_color);
                PaletteFade_BlendPalettes(p_wk->pfd, PALFADE_BUFFER_SUB_BG, 0xbfff, evy, p_wk->fade_color);
            }
        }
        PaletteFade_Transfer(p_wk->pfd);
        if (p_wk->cnt++ >= p_wk->cnt_max) {
            *p_seq = 2;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL BrFade_Fade_MasterBrightAndAlpha(BrFade *p_wk, u32 *p_seq) {
    switch (*p_seq) {
    case 0: {
        s32 start, end;
        s32 eva, evb;

        if (p_wk->dir == BR_FADE_DIR_IN) {
            start = 16;
            end = 0;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            start = 0;
            end = 16;
        }
        if (p_wk->dir == BR_FADE_DIR_IN) {
            eva = 0;
            evb = 16;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            eva = 16;
            evb = 0;
        }
        GFL_FadeSet(FADE_ENGINE_A_WHITE, start, end, p_wk->sync);
        G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1,
                           GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2, eva, evb);
        p_wk->cnt = 0;
        p_wk->cnt_max = p_wk->sync != 0 ? p_wk->sync : 16 / GFL_FadeGetUpdateFreq();
        *p_seq = 1;
        break;
    }
    case 1: {
        BOOL isEnd = TRUE;
        s32 eva, evb;

        if (p_wk->dir == BR_FADE_DIR_IN) {
            eva = p_wk->cnt * 16 / p_wk->cnt_max;
            evb = 16 - eva;
        } else if (p_wk->dir == BR_FADE_DIR_OUT) {
            evb = p_wk->cnt * 16 / p_wk->cnt_max;
            eva = 16 - evb;
        }
        G2S_ChangeBlendAlpha(eva, evb);
        if (eva == 0) {
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, FALSE);
        } else {
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, TRUE);
        }
        // Ends once both the steps and the master brightness fade are done
        isEnd &= p_wk->cnt++ >= p_wk->cnt_max;
        if ((GFL_FadeIsRunning() == FALSE) & isEnd) {
            *p_seq = 2;
        }
        break;
    }
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BrFadeFunc BrFade_GetFadeFunc(u32 type) {
    switch (type) {
    case BR_FADE_TYPE_MASTER_BRIGHT_BLACK:
        return BrFade_Fade_MasterBright_Black;
    case BR_FADE_TYPE_MASTER_BRIGHT_WHITE:
        return BrFade_Fade_MasterBright_White;
    case BR_FADE_TYPE_ALPHA_BG012:
        return BrFade_Fade_Alpha_BG012;
    case BR_FADE_TYPE_PLTT:
        return BrFade_Fade_Pltt;
    case BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA:
        return BrFade_Fade_MasterBrightAndAlpha;
    default:
        return NULL;
    }
}
