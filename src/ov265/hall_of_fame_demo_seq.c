#include "types.h"
#include "constants/sound.h"
#include "demo/hall_of_fame_demo.h"
#include "demo/hall_of_fame_demo_main.h"
#include "demo/hall_of_fame_demo_obj.h"
#include "demo/hall_of_fame_demo_seq.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/key.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/mcss.h"
#include "system/wipe.h"

// The Hall of Fame entry demo's sequences: the set-up, the first scene, which shows the team's Pokémon one by one,
// and the second, which shows the trainer with the team until a button is pressed (overlay 265). The file's name is
// a guess, after the Hall of Fame PC's hall_of_fame_pc_seq.c

// The sequences, which return the one to run next
enum {
    SEQ_INIT,
    SEQ_RELEASE,
    SEQ_WIPE,
    SEQ_WAIT,
    SEQ_1ST_INIT,
    SEQ_1ST_MAIN,
    SEQ_1ST_EXIT,
    SEQ_2ND_INIT,
    SEQ_2ND_MAIN,
    SEQ_2ND_EXIT,
    SEQ_END,
};

typedef int (*HallOfFameDemoSeqFunction)(HallOfFameDemoWork *wk);

static int HallOfFameDemoSeq_Init(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Release(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Wipe(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Wait(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Init1st(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Exit1st(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Main1st(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Init2nd(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Exit2nd(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_Main2nd(HallOfFameDemoWork *wk);
static int HallOfFameDemoSeq_SetFadeIn(HallOfFameDemoWork *wk, int next);
static int HallOfFameDemoSeq_SetFadeOut(HallOfFameDemoWork *wk, int next, int division);
static int HallOfFameDemoSeq_SetWait(HallOfFameDemoWork *wk, int wait);
static void HallOfFameDemoSeq_InitShake(HallOfFameDemoWork *wk);
static BOOL HallOfFameDemoSeq_Shake(HallOfFameDemoWork *wk, u32 id);
static void HallOfFameDemoSeq_RotateBg(HallOfFameDemoWork *wk);

static const HallOfFameDemoSeqFunction sHallOfFameDemoSeqFunctions[] = {
    HallOfFameDemoSeq_Init,    HallOfFameDemoSeq_Release, HallOfFameDemoSeq_Wipe,    HallOfFameDemoSeq_Wait,
    HallOfFameDemoSeq_Init1st, HallOfFameDemoSeq_Main1st, HallOfFameDemoSeq_Exit1st, HallOfFameDemoSeq_Init2nd,
    HallOfFameDemoSeq_Main2nd, HallOfFameDemoSeq_Exit2nd,
};

BOOL HallOfFameDemoSeq_Main(HallOfFameDemoWork *wk) {
    wk->mainSeq = sHallOfFameDemoSeqFunctions[wk->mainSeq](wk);
    if (wk->mainSeq == SEQ_END) {
        return FALSE;
    }
    if (wk->scene == HALL_OF_FAME_DEMO_SCENE_1ST) {
        HallOfFameDemoObj_Main1st(wk);
        HallOfFameDemoMain_MainDouble3D(wk);
    } else if (wk->scene == HALL_OF_FAME_DEMO_SCENE_2ND) {
        HallOfFameDemoObj_Main2nd(wk);
        HallOfFameDemoMain_Main3D(wk);
        HallOfFameDemoSeq_RotateBg(wk);
    }
    return TRUE;
}

static int HallOfFameDemoSeq_Init(HallOfFameDemoWork *wk) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    HallOfFameDemoMain_InitBg();
    HallOfFameDemoMain_InitMsg(wk);
    HallOfFameDemoMain_InitSound(wk);
    HallOfFameDemoMain_GetPokeMax(wk);
    return SEQ_1ST_INIT;
}

static int HallOfFameDemoSeq_Release(HallOfFameDemoWork *wk) {
    HallOfFameDemoMain_ExitSound(wk);
    HallOfFameDemoMain_ExitMsg(wk);
    HallOfFameDemoMain_ExitBg();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    return SEQ_END;
}

static int HallOfFameDemoSeq_Wipe(HallOfFameDemoWork *wk) {
    if (GFL_WipeIsFinished() == TRUE) {
        return wk->nextSeq;
    }
    return SEQ_WIPE;
}

static int HallOfFameDemoSeq_Wait(HallOfFameDemoWork *wk) {
    if (wk->count == 0) {
        return wk->nextSeq;
    }
    wk->count--;
    return SEQ_WAIT;
}

static int HallOfFameDemoSeq_Init1st(HallOfFameDemoWork *wk) {
    HallOfFameDemoMain_InitVram(0);
    HallOfFameDemoMain_InitBgMode();
    HallOfFameDemoMain_Init3D(wk);
    HallOfFameDemoMain_InitParticle();
    HallOfFameDemoMain_InitDouble3D();
    HallOfFameDemoObj_Init(wk, 0);
    HallOfFameDemoObj_Init1st(wk);
    HallOfFameDemoMain_SetBlendAlpha();
    HallOfFameDemoMain_InitVBlank1st(wk);
    wk->scene = HALL_OF_FAME_DEMO_SCENE_1ST;
    return SEQ_1ST_MAIN;
}

static int HallOfFameDemoSeq_Exit1st(HallOfFameDemoWork *wk) {
    HallOfFameDemoMain_ExitVBlank(wk);
    G2_BlendNone();
    G2S_BlendNone();
    HallOfFameDemoObj_Exit1st(wk);
    HallOfFameDemoObj_Exit(wk);
    HallOfFameDemoMain_ExitDouble3D();
    HallOfFameDemoMain_ExitParticle();
    HallOfFameDemoMain_Exit3D(wk);
    wk->scene = HALL_OF_FAME_DEMO_SCENE_NONE;
    return SEQ_2ND_INIT;
}

// Shows a Pokémon: its sprites slide in from the left, the title and name plates from both sides, each shaking as
// it stops, then the Pokémon cries and all slide out again
static int HallOfFameDemoSeq_Main1st(HallOfFameDemoWork *wk) {
    switch (wk->subSeq) {
    case 0:
        wk->subSeq++;
        return HallOfFameDemoSeq_SetWait(wk, 32);
    case 1:
        GFL_SndSEPlay(SEQ_SE_DDEMO_02A);
        wk->subSeq++;
        break;
    case 2:
        wk->count++;
        if (wk->count == 48) {
            GFL_SndSEPlay(SEQ_SE_DDEMO_02B);
            wk->count = 0;
            wk->subSeq++;
        }
        break;
    case 3:
        if (GFL_SndIsPlaying(SEQ_SE_DDEMO_02B) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DDEMO_01);
            HallOfFameDemoMain_GetPokeData(wk);
            HallOfFameDemoMain_LoadPokeVoice(wk);
            HallOfFameDemoMain_CreateTypeParticle(wk);
            HallOfFameDemoMain_CreateNameParticle(wk);
            HallOfFameDemoObj_AddPoke(wk);
            HallOfFameDemoObj_PrintPokeInfo(wk);
            HallOfFameDemoObj_MoveFontOam(wk);
            HallOfFameDemoObj_LoadTypePalette(wk);
            wk->subSeq++;
            return HallOfFameDemoSeq_SetFadeIn(wk, SEQ_1ST_MAIN);
        }
        break;
    case 4:
        if (wk->count == 22) {
            wk->subSeq++;
            return HallOfFameDemoSeq_SetWait(wk, 8);
        }
        HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_POKE_BACK, -16, 0);
        wk->count++;
        break;
    case 5:
        HallOfFameDemoMain_SetTypeParticle(wk, 0);
        HallOfFameDemoMain_SetTypeParticle(wk, 1);
        wk->subSeq++;
        return HallOfFameDemoSeq_SetWait(wk, 8);
    case 6:
        HallOfFameDemoMain_SetNameParticle(wk, 1);
        wk->subSeq++;
        // fallthrough
    case 7: {
        s16 x, y;

        HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE, &x, &y);
        if (x != 168) {
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE, 16, 0);
        } else {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            HallOfFameDemoSeq_InitShake(wk);
            wk->shakeBaseX = x;
            wk->shakeBaseY = y;
            wk->subSeq++;
        }
        HallOfFameDemoObj_MoveFontOam(wk);
        break;
    }
    case 8:
        HallOfFameDemoMain_SetNameParticle(wk, 0);
        wk->subSeq++;
        // fallthrough
    case 9: {
        BOOL shaking = HallOfFameDemoSeq_Shake(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE);
        s16 x, y;

        HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_NAME, &x, &y);
        if (x != 88) {
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_NAME, -16, 0);
        } else if (shaking == FALSE) {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            HallOfFameDemoSeq_InitShake(wk);
            wk->shakeBaseX = x;
            wk->shakeBaseY = y;
            wk->subSeq++;
        }
        HallOfFameDemoObj_MoveFontOam(wk);
        break;
    }
    case 10: {
        BOOL shaking = HallOfFameDemoSeq_Shake(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_NAME);
        s16 x, y;

        HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_POKE_FRONT, &x, &y);
        if (x != 48) {
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_POKE_FRONT, 16, 0);
        } else if (shaking == FALSE) {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            HallOfFameDemoSeq_InitShake(wk);
            wk->shakeBaseX = x;
            wk->shakeBaseY = y;
            wk->subSeq++;
        }
        HallOfFameDemoObj_MoveFontOam(wk);
        break;
    }
    case 11:
        if (HallOfFameDemoSeq_Shake(wk, HALL_OF_FAME_DEMO_OBJ_POKE_FRONT) == FALSE) {
            PokeVoice_StartPlayback(wk->voice);
            HallOfFameDemoObj_SetVisible(wk, HALL_OF_FAME_DEMO_OBJ_FLASH, TRUE);
            HallOfFameDemoObj_SetAnime(wk, HALL_OF_FAME_DEMO_OBJ_FLASH, 6);
            wk->subSeq++;
            return HallOfFameDemoSeq_SetWait(wk, 128);
        }
        break;
    case 12:
        if (PokeVoice_IsPlaying(wk->voice) == FALSE &&
            HallOfFameDemoObj_IsAnimating(wk, HALL_OF_FAME_DEMO_OBJ_FLASH) == FALSE) {
            wk->subSeq++;
        }
        break;
    case 13: {
        s16 x, y;

        HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_POKE_FRONT, &x, &y);
        if (x != -48) {
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_POKE_FRONT, -16, 0);
        } else {
            wk->subSeq++;
        }
        break;
    }
    case 14: {
        s16 x, y;

        HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_NAME, &x, &y);
        if (x != 264) {
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_NAME, 16, 0);
        } else {
            wk->subSeq++;
        }
        HallOfFameDemoObj_MoveFontOam(wk);
        break;
    }
    case 15: {
        s16 x, y;

        HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE, &x, &y);
        if (x != -8) {
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE, -16, 0);
            HallOfFameDemoObj_MoveFontOam(wk);
            break;
        }
        wk->subSeq++;
        return HallOfFameDemoSeq_SetFadeOut(wk, SEQ_1ST_MAIN, 6);
    }
    case 16:
        HallOfFameDemoMain_DeleteNameParticle(wk);
        HallOfFameDemoMain_DeleteTypeParticle(wk);
        wk->monIndex++;
        if (wk->monIndex == wk->monCount) {
            wk->monIndex = 0;
            wk->subSeq = 0;
            return SEQ_1ST_EXIT;
        }
        wk->subSeq = 3;
        break;
    }
    return SEQ_1ST_MAIN;
}

static int HallOfFameDemoSeq_Init2nd(HallOfFameDemoWork *wk) {
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_SUB_MAIN);
    HallOfFameDemoMain_InitVram(1);
    HallOfFameDemoMain_InitBgMode();
    HallOfFameDemoMain_InitBg2nd();
    HallOfFameDemoMain_LoadBg();
    HallOfFameDemoMain_Init3D(wk);
    HallOfFameDemoMain_InitMcss(wk);
    HallOfFameDemoObj_Init(wk, 1);
    HallOfFameDemoObj_Init2nd(wk);
    HallOfFameDemoMain_InitVBlank2nd(wk);
    wk->scene = HALL_OF_FAME_DEMO_SCENE_2ND;
    return HallOfFameDemoSeq_SetFadeIn(wk, SEQ_2ND_MAIN);
}

static int HallOfFameDemoSeq_Exit2nd(HallOfFameDemoWork *wk) {
    HallOfFameDemoMain_ExitVBlank(wk);
    HallOfFameDemoObj_Exit2nd(wk);
    HallOfFameDemoObj_Exit(wk);
    HallOfFameDemoMain_ExitBg2nd();
    HallOfFameDemoMain_ExitMcss(wk);
    HallOfFameDemoMain_Exit3D(wk);
    wk->scene = HALL_OF_FAME_DEMO_SCENE_NONE;
    return SEQ_RELEASE;
}

// The trainer drops in on both screens and the plates open, then the team's Pokémon cross the screen in turn, again
// and again, until A, B or a touch ends the demo
static int HallOfFameDemoSeq_Main2nd(HallOfFameDemoWork *wk) {
    if (wk->end == TRUE && GFL_WipeIsFinished() == TRUE && GFL_SndBGMIsFading() == FALSE) {
        GFL_SndBGMStopImmediate();
        return SEQ_2ND_EXIT;
    }
    switch (wk->subSeq) {
    case 0:
        wk->subSeq++;
        return HallOfFameDemoSeq_SetWait(wk, 32);
    case 1:
        GFL_SndSEPlay(SEQ_SE_DDEMO_03);
        wk->subSeq++;
        break;
    case 2:
        if (wk->count != 51) {
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_TRAINER_MAIN, 0, 8);
            HallOfFameDemoObj_Move(wk, HALL_OF_FAME_DEMO_OBJ_TRAINER_SUB, 0, 8);
            wk->count++;
        } else {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            wk->subSeq++;
            return HallOfFameDemoSeq_SetWait(wk, 32);
        }
        break;
    case 3:
        HallOfFameDemoObj_SetVisible(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_UPPER, TRUE);
        HallOfFameDemoObj_SetVisible(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_LOWER, TRUE);
        HallOfFameDemoObj_SetAnime(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_UPPER, 2);
        HallOfFameDemoObj_SetAnime(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_LOWER, 2);
        wk->subSeq++;
        break;
    case 4:
        if (HallOfFameDemoObj_IsAnimating(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_UPPER) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DDEMO_04);
            wk->subSeq++;
            return HallOfFameDemoSeq_SetWait(wk, 16);
        }
        break;
    case 5:
        HallOfFameDemoObj_SetFontOamVisible(wk, HALL_OF_FAME_DEMO_FOAM_PLAYER, TRUE);
        wk->subSeq++;
        return HallOfFameDemoSeq_SetWait(wk, 32);
    case 6:
        HallOfFameDemoObj_SetFontOamVisible(wk, HALL_OF_FAME_DEMO_FOAM_TITLE_2ND, TRUE);
        wk->subSeq++;
        break;
    case 7:
        if (wk->count == 300) {
            wk->count = 0;
            wk->subSeq++;
        } else {
            wk->count++;
        }
        break;
    case 8: {
        BOOL start = FALSE;

        if (wk->monIndex == 0) {
            if (wk->count == 256) {
                start = TRUE;
            }
        } else {
            if (wk->count == 128) {
                start = TRUE;
            }
        }
        if (start == TRUE) {
            HallOfFameDemoMain_AddMcss(wk);
            wk->count = 0;
            wk->subSeq++;
        } else {
            wk->count++;
        }
        break;
    }
    case 9:
        if (wk->count != 20) {
            HallOfFameDemoMain_MoveMcss(wk, -8);
            wk->count++;
        } else {
            MCSS_ResumeAnimation(wk->mcss);
            wk->count = 0;
            wk->subSeq++;
        }
        break;
    case 10:
        if (wk->count != 128) {
            wk->count++;
        } else {
            HallOfFameDemoMain_SetMcssCallBack(wk);
            wk->count = 0;
            wk->subSeq++;
        }
        break;
    case 11:
        if (wk->mcssAnmEnd != FALSE) {
            if (wk->count != 24) {
                HallOfFameDemoMain_MoveMcss(wk, -8);
                wk->count++;
            } else {
                HallOfFameDemoMain_DelMcss(wk);
                wk->count = 0;
                wk->monIndex++;
                if (wk->monIndex == wk->monCount) {
                    wk->monIndex = 0;
                }
                wk->subSeq = 8;
            }
        }
        break;
    }
    if (wk->subSeq >= 6) {
        HallOfFameDemoObj_MainConfetti(wk);
    }
    if (wk->subSeq >= 8 && wk->end == FALSE) {
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed() == TRUE) {
            wk->end = TRUE;
            GFL_SndBGMFadeOut(220);
            HallOfFameDemoSeq_SetFadeOut(wk, SEQ_2ND_EXIT, 188);
        }
    }
    return SEQ_2ND_MAIN;
}

static int HallOfFameDemoSeq_SetFadeIn(HallOfFameDemoWork *wk, int next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_HALL_OF_FAME_DEMO);
    wk->nextSeq = next;
    return SEQ_WIPE;
}

static int HallOfFameDemoSeq_SetFadeOut(HallOfFameDemoWork *wk, int next, int division) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, division, 1,
                HEAPID_HALL_OF_FAME_DEMO);
    wk->nextSeq = next;
    return SEQ_WIPE;
}

// Waits for the given frames, then goes on with the sequence that runs
static int HallOfFameDemoSeq_SetWait(HallOfFameDemoWork *wk, int wait) {
    wk->count = wait;
    wk->nextSeq = wk->mainSeq;
    return SEQ_WAIT;
}

static void HallOfFameDemoSeq_InitShake(HallOfFameDemoWork *wk) {
    u32 i;

    wk->shakeIndex = 0;
    for (i = 0; i < HALL_OF_FAME_DEMO_SHAKE_COUNT; i++) {
        wk->shakeX[i] = GFL_RandomLC(8) - 4;
        wk->shakeY[i] = GFL_RandomLC(8) - 4;
    }
}

// Moves an actor to the next place of the shake, or back in place and returns FALSE once it is over
static BOOL HallOfFameDemoSeq_Shake(HallOfFameDemoWork *wk, u32 id) {
    if (wk->shakeIndex == HALL_OF_FAME_DEMO_SHAKE_COUNT) {
        HallOfFameDemoObj_SetPos(wk, id, wk->shakeBaseX, wk->shakeBaseY);
        return FALSE;
    }
    HallOfFameDemoObj_SetPos(wk, id, wk->shakeBaseX + wk->shakeX[wk->shakeIndex],
                             wk->shakeBaseY + wk->shakeY[wk->shakeIndex]);
    wk->shakeIndex++;
    return TRUE;
}

static void HallOfFameDemoSeq_RotateBg(HallOfFameDemoWork *wk) {
    GFL_BGSysRotateBGReq(3, BG_ROTATE_ADD, 1);
    GFL_BGSysRotateBGReq(BGSYS_BG_SUB + 3, BG_ROTATE_ADD, 1);
}
