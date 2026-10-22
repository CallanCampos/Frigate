#include "types.h"
#include "constants/sound.h"
#include "demo/ddemo_main.h"
#include "demo/ddemo_obj.h"
#include "demo/ddemo_seq.h"
#include "demo/dendou_demo.h"
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
// a guess, after the Hall of Fame PC's dpc_seq.c

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

typedef int (*DDemoSeqFunction)(DDemoWork *wk);

static int DDemoSeq_Init(DDemoWork *wk);
static int DDemoSeq_Release(DDemoWork *wk);
static int DDemoSeq_Wipe(DDemoWork *wk);
static int DDemoSeq_Wait(DDemoWork *wk);
static int DDemoSeq_Init1st(DDemoWork *wk);
static int DDemoSeq_Exit1st(DDemoWork *wk);
static int DDemoSeq_Main1st(DDemoWork *wk);
static int DDemoSeq_Init2nd(DDemoWork *wk);
static int DDemoSeq_Exit2nd(DDemoWork *wk);
static int DDemoSeq_Main2nd(DDemoWork *wk);
static int DDemoSeq_SetFadeIn(DDemoWork *wk, int next);
static int DDemoSeq_SetFadeOut(DDemoWork *wk, int next, int division);
static int DDemoSeq_SetWait(DDemoWork *wk, int wait);
static void DDemoSeq_InitShake(DDemoWork *wk);
static BOOL DDemoSeq_Shake(DDemoWork *wk, u32 id);
static void DDemoSeq_RotateBg(DDemoWork *wk);

static const DDemoSeqFunction sDDemoSeqFunctions[] = {
    DDemoSeq_Init,    DDemoSeq_Release, DDemoSeq_Wipe,    DDemoSeq_Wait,    DDemoSeq_Init1st,
    DDemoSeq_Main1st, DDemoSeq_Exit1st, DDemoSeq_Init2nd, DDemoSeq_Main2nd, DDemoSeq_Exit2nd,
};

BOOL DDemoSeq_Main(DDemoWork *wk) {
    wk->mainSeq = sDDemoSeqFunctions[wk->mainSeq](wk);
    if (wk->mainSeq == SEQ_END) {
        return FALSE;
    }
    if (wk->scene == DDEMO_SCENE_1ST) {
        DDemoObj_Main1st(wk);
        DDemoMain_MainDouble3D(wk);
    } else if (wk->scene == DDEMO_SCENE_2ND) {
        DDemoObj_Main2nd(wk);
        DDemoMain_Main3D(wk);
        DDemoSeq_RotateBg(wk);
    }
    return TRUE;
}

static int DDemoSeq_Init(DDemoWork *wk) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    DDemoMain_InitBg();
    DDemoMain_InitMsg(wk);
    DDemoMain_InitSound(wk);
    DDemoMain_GetPokeMax(wk);
    return SEQ_1ST_INIT;
}

static int DDemoSeq_Release(DDemoWork *wk) {
    DDemoMain_ExitSound(wk);
    DDemoMain_ExitMsg(wk);
    DDemoMain_ExitBg();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    return SEQ_END;
}

static int DDemoSeq_Wipe(DDemoWork *wk) {
    if (GFL_WipeIsFinished() == TRUE) {
        return wk->nextSeq;
    }
    return SEQ_WIPE;
}

static int DDemoSeq_Wait(DDemoWork *wk) {
    if (wk->count == 0) {
        return wk->nextSeq;
    }
    wk->count--;
    return SEQ_WAIT;
}

static int DDemoSeq_Init1st(DDemoWork *wk) {
    DDemoMain_InitVram(0);
    DDemoMain_InitBgMode();
    DDemoMain_Init3D(wk);
    DDemoMain_InitParticle();
    DDemoMain_InitDouble3D();
    DDemoObj_Init(wk, 0);
    DDemoObj_Init1st(wk);
    DDemoMain_SetBlendAlpha();
    DDemoMain_InitVBlank1st(wk);
    wk->scene = DDEMO_SCENE_1ST;
    return SEQ_1ST_MAIN;
}

static int DDemoSeq_Exit1st(DDemoWork *wk) {
    DDemoMain_ExitVBlank(wk);
    G2_BlendNone();
    G2S_BlendNone();
    DDemoObj_Exit1st(wk);
    DDemoObj_Exit(wk);
    DDemoMain_ExitDouble3D();
    DDemoMain_ExitParticle();
    DDemoMain_Exit3D(wk);
    wk->scene = DDEMO_SCENE_NONE;
    return SEQ_2ND_INIT;
}

// Shows a Pokémon: its sprites slide in from the left, the title and name plates from both sides, each shaking as
// it stops, then the Pokémon cries and all slide out again
static int DDemoSeq_Main1st(DDemoWork *wk) {
    switch (wk->subSeq) {
    case 0:
        wk->subSeq++;
        return DDemoSeq_SetWait(wk, 32);
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
            DDemoMain_GetPokeData(wk);
            DDemoMain_LoadPokeVoice(wk);
            DDemoMain_CreateTypeParticle(wk);
            DDemoMain_CreateNameParticle(wk);
            DDemoObj_AddPoke(wk);
            DDemoObj_PrintPokeInfo(wk);
            DDemoObj_MoveFontOam(wk);
            DDemoObj_LoadTypePalette(wk);
            wk->subSeq++;
            return DDemoSeq_SetFadeIn(wk, SEQ_1ST_MAIN);
        }
        break;
    case 4:
        if (wk->count == 22) {
            wk->subSeq++;
            return DDemoSeq_SetWait(wk, 8);
        }
        DDemoObj_Move(wk, DDEMOOBJ_POKE_BACK, -16, 0);
        wk->count++;
        break;
    case 5:
        DDemoMain_SetTypeParticle(wk, 0);
        DDemoMain_SetTypeParticle(wk, 1);
        wk->subSeq++;
        return DDemoSeq_SetWait(wk, 8);
    case 6:
        DDemoMain_SetNameParticle(wk, 1);
        wk->subSeq++;
        // fallthrough
    case 7: {
        s16 x, y;

        DDemoObj_GetPos(wk, DDEMOOBJ_PLATE_TITLE, &x, &y);
        if (x != 168) {
            DDemoObj_Move(wk, DDEMOOBJ_PLATE_TITLE, 16, 0);
        } else {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            DDemoSeq_InitShake(wk);
            wk->shakeBaseX = x;
            wk->shakeBaseY = y;
            wk->subSeq++;
        }
        DDemoObj_MoveFontOam(wk);
        break;
    }
    case 8:
        DDemoMain_SetNameParticle(wk, 0);
        wk->subSeq++;
        // fallthrough
    case 9: {
        BOOL shaking = DDemoSeq_Shake(wk, DDEMOOBJ_PLATE_TITLE);
        s16 x, y;

        DDemoObj_GetPos(wk, DDEMOOBJ_PLATE_NAME, &x, &y);
        if (x != 88) {
            DDemoObj_Move(wk, DDEMOOBJ_PLATE_NAME, -16, 0);
        } else if (shaking == FALSE) {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            DDemoSeq_InitShake(wk);
            wk->shakeBaseX = x;
            wk->shakeBaseY = y;
            wk->subSeq++;
        }
        DDemoObj_MoveFontOam(wk);
        break;
    }
    case 10: {
        BOOL shaking = DDemoSeq_Shake(wk, DDEMOOBJ_PLATE_NAME);
        s16 x, y;

        DDemoObj_GetPos(wk, DDEMOOBJ_POKE_FRONT, &x, &y);
        if (x != 48) {
            DDemoObj_Move(wk, DDEMOOBJ_POKE_FRONT, 16, 0);
        } else if (shaking == FALSE) {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            DDemoSeq_InitShake(wk);
            wk->shakeBaseX = x;
            wk->shakeBaseY = y;
            wk->subSeq++;
        }
        DDemoObj_MoveFontOam(wk);
        break;
    }
    case 11:
        if (DDemoSeq_Shake(wk, DDEMOOBJ_POKE_FRONT) == FALSE) {
            PokeVoice_StartPlayback(wk->voice);
            DDemoObj_SetVisible(wk, DDEMOOBJ_FLASH, TRUE);
            DDemoObj_SetAnime(wk, DDEMOOBJ_FLASH, 6);
            wk->subSeq++;
            return DDemoSeq_SetWait(wk, 128);
        }
        break;
    case 12:
        if (PokeVoice_IsPlaying(wk->voice) == FALSE && DDemoObj_IsAnimating(wk, DDEMOOBJ_FLASH) == FALSE) {
            wk->subSeq++;
        }
        break;
    case 13: {
        s16 x, y;

        DDemoObj_GetPos(wk, DDEMOOBJ_POKE_FRONT, &x, &y);
        if (x != -48) {
            DDemoObj_Move(wk, DDEMOOBJ_POKE_FRONT, -16, 0);
        } else {
            wk->subSeq++;
        }
        break;
    }
    case 14: {
        s16 x, y;

        DDemoObj_GetPos(wk, DDEMOOBJ_PLATE_NAME, &x, &y);
        if (x != 264) {
            DDemoObj_Move(wk, DDEMOOBJ_PLATE_NAME, 16, 0);
        } else {
            wk->subSeq++;
        }
        DDemoObj_MoveFontOam(wk);
        break;
    }
    case 15: {
        s16 x, y;

        DDemoObj_GetPos(wk, DDEMOOBJ_PLATE_TITLE, &x, &y);
        if (x != -8) {
            DDemoObj_Move(wk, DDEMOOBJ_PLATE_TITLE, -16, 0);
            DDemoObj_MoveFontOam(wk);
            break;
        }
        wk->subSeq++;
        return DDemoSeq_SetFadeOut(wk, SEQ_1ST_MAIN, 6);
    }
    case 16:
        DDemoMain_DeleteNameParticle(wk);
        DDemoMain_DeleteTypeParticle(wk);
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

static int DDemoSeq_Init2nd(DDemoWork *wk) {
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_SUB_MAIN);
    DDemoMain_InitVram(1);
    DDemoMain_InitBgMode();
    DDemoMain_InitBg2nd();
    DDemoMain_LoadBg();
    DDemoMain_Init3D(wk);
    DDemoMain_InitMcss(wk);
    DDemoObj_Init(wk, 1);
    DDemoObj_Init2nd(wk);
    DDemoMain_InitVBlank2nd(wk);
    wk->scene = DDEMO_SCENE_2ND;
    return DDemoSeq_SetFadeIn(wk, SEQ_2ND_MAIN);
}

static int DDemoSeq_Exit2nd(DDemoWork *wk) {
    DDemoMain_ExitVBlank(wk);
    DDemoObj_Exit2nd(wk);
    DDemoObj_Exit(wk);
    DDemoMain_ExitBg2nd();
    DDemoMain_ExitMcss(wk);
    DDemoMain_Exit3D(wk);
    wk->scene = DDEMO_SCENE_NONE;
    return SEQ_RELEASE;
}

// The trainer drops in on both screens and the plates open, then the team's Pokémon cross the screen in turn, again
// and again, until A, B or a touch ends the demo
static int DDemoSeq_Main2nd(DDemoWork *wk) {
    if (wk->end == TRUE && GFL_WipeIsFinished() == TRUE && GFL_SndBGMIsFading() == FALSE) {
        GFL_SndBGMStopImmediate();
        return SEQ_2ND_EXIT;
    }
    switch (wk->subSeq) {
    case 0:
        wk->subSeq++;
        return DDemoSeq_SetWait(wk, 32);
    case 1:
        GFL_SndSEPlay(SEQ_SE_DDEMO_03);
        wk->subSeq++;
        break;
    case 2:
        if (wk->count != 51) {
            DDemoObj_Move(wk, DDEMOOBJ_TRAINER_MAIN, 0, 8);
            DDemoObj_Move(wk, DDEMOOBJ_TRAINER_SUB, 0, 8);
            wk->count++;
        } else {
            GFL_SndSEPlay(SEQ_SE_ROTATION_B);
            wk->subSeq++;
            return DDemoSeq_SetWait(wk, 32);
        }
        break;
    case 3:
        DDemoObj_SetVisible(wk, DDEMOOBJ_PLATE_UPPER, TRUE);
        DDemoObj_SetVisible(wk, DDEMOOBJ_PLATE_LOWER, TRUE);
        DDemoObj_SetAnime(wk, DDEMOOBJ_PLATE_UPPER, 2);
        DDemoObj_SetAnime(wk, DDEMOOBJ_PLATE_LOWER, 2);
        wk->subSeq++;
        break;
    case 4:
        if (DDemoObj_IsAnimating(wk, DDEMOOBJ_PLATE_UPPER) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DDEMO_04);
            wk->subSeq++;
            return DDemoSeq_SetWait(wk, 16);
        }
        break;
    case 5:
        DDemoObj_SetFontOamVisible(wk, DDEMO_FOAM_PLAYER, TRUE);
        wk->subSeq++;
        return DDemoSeq_SetWait(wk, 32);
    case 6:
        DDemoObj_SetFontOamVisible(wk, DDEMO_FOAM_TITLE_2ND, TRUE);
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
            DDemoMain_AddMcss(wk);
            wk->count = 0;
            wk->subSeq++;
        } else {
            wk->count++;
        }
        break;
    }
    case 9:
        if (wk->count != 20) {
            DDemoMain_MoveMcss(wk, -8);
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
            DDemoMain_SetMcssCallBack(wk);
            wk->count = 0;
            wk->subSeq++;
        }
        break;
    case 11:
        if (wk->mcssAnmEnd != FALSE) {
            if (wk->count != 24) {
                DDemoMain_MoveMcss(wk, -8);
                wk->count++;
            } else {
                DDemoMain_DelMcss(wk);
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
        DDemoObj_MainConfetti(wk);
    }
    if (wk->subSeq >= 8 && wk->end == FALSE) {
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed() == TRUE) {
            wk->end = TRUE;
            GFL_SndBGMFadeOut(220);
            DDemoSeq_SetFadeOut(wk, SEQ_2ND_EXIT, 188);
        }
    }
    return SEQ_2ND_MAIN;
}

static int DDemoSeq_SetFadeIn(DDemoWork *wk, int next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_DENDOU_DEMO);
    wk->nextSeq = next;
    return SEQ_WIPE;
}

static int DDemoSeq_SetFadeOut(DDemoWork *wk, int next, int division) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, division, 1,
                HEAPID_DENDOU_DEMO);
    wk->nextSeq = next;
    return SEQ_WIPE;
}

// Waits for the given frames, then goes on with the sequence that runs
static int DDemoSeq_SetWait(DDemoWork *wk, int wait) {
    wk->count = wait;
    wk->nextSeq = wk->mainSeq;
    return SEQ_WAIT;
}

static void DDemoSeq_InitShake(DDemoWork *wk) {
    u32 i;

    wk->shakeIndex = 0;
    for (i = 0; i < DDEMO_SHAKE_COUNT; i++) {
        wk->shakeX[i] = GFL_RandomLC(8) - 4;
        wk->shakeY[i] = GFL_RandomLC(8) - 4;
    }
}

// Moves an actor to the next place of the shake, or back in place and returns FALSE once it is over
static BOOL DDemoSeq_Shake(DDemoWork *wk, u32 id) {
    if (wk->shakeIndex == DDEMO_SHAKE_COUNT) {
        DDemoObj_SetPos(wk, id, wk->shakeBaseX, wk->shakeBaseY);
        return FALSE;
    }
    DDemoObj_SetPos(wk, id, wk->shakeBaseX + wk->shakeX[wk->shakeIndex], wk->shakeBaseY + wk->shakeY[wk->shakeIndex]);
    wk->shakeIndex++;
    return TRUE;
}

static void DDemoSeq_RotateBg(DDemoWork *wk) {
    GFL_BGSysRotateBGReq(3, BG_ROTATE_ADD, 1);
    GFL_BGSysRotateBGReq(BGSYS_BG_SUB + 3, BG_ROTATE_ADD, 1);
}
