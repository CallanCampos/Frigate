// The icon that shows over the player while the game saves, which animates on a background layer. The name is the
// ROM's own, from GFL_HeapAllocate's file argument.
#include "types.h"
#include "constants/arc.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_effect.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/g3d.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/game_system.h"

struct FieldSaveAnime {
    Field *field;
    BmpWin *window;
    u16 offsetX;
    u16 offsetY;
    TCB *tcb;
    GFLBitmap *bitmap;
    int timer;
    u32 frame;
};

// How long each frame of the icon shows, and which frame of the bitmap follows
typedef struct {
    u16 nextBitmapFrame;
    u16 duration;
} SaveAnimeStep;

static void func_ov036_021c6d58(FieldSaveAnime *anime, HeapID heapId);
static void func_ov036_021c6db8(FieldSaveAnime *anime);
static void func_ov036_021c6dc8(FieldSaveAnime *anime);
static void func_ov036_021c6f54(FieldSaveAnime *anime);
static void func_ov036_021c6f64(FieldSaveAnime *anime);
static void func_ov036_021c6f7c(FieldSaveAnime *anime);
static void func_ov036_021c6f94(FieldSaveAnime *anime, int frame);
static void func_ov036_021c6fec(TCB *tcb, void *data);

static const u32 SAVE_ANIME_PALETTE_OFFSETS[2] = { 0, 1 };

static const VecFx32 SAVE_ANIME_UP = { 0, FX32_ONE, 0 };

static const SaveAnimeStep SAVE_ANIME_STEPS[4] = { { 0, 20 }, { 1, 40 }, { 0, 60 }, { 2, 80 } };

FieldSaveAnime *func_ov036_021c6cc8(HeapID heapId, Field *field) {
    FieldSaveAnime *anime = GFL_HeapAllocate(heapId, sizeof(FieldSaveAnime), TRUE, "field_saveanime.c", 143);

    anime->field = field;
    func_ov036_021c6d58(anime, heapId);
    func_ov036_021c6dc8(anime);
    return anime;
}

void func_ov036_021c6cf8(FieldSaveAnime *anime) {
    func_ov036_021c6d3c(anime);
    func_ov036_021c6f54(anime);
    func_ov036_021c6db8(anime);
    GFL_HeapFree(anime);
}

void func_ov036_021c6d14(FieldSaveAnime *anime) {
    GFL_BGSysClearBG(3);
    anime->timer = 0;
    func_ov036_021c6f64(anime);
    anime->tcb = GFL_VBlankTCBAdd(func_ov036_021c6fec, anime, 128);
}

void func_ov036_021c6d3c(FieldSaveAnime *anime) {
    if (anime->tcb != NULL) {
        GFL_TCBRemove(anime->tcb);
        anime->tcb = NULL;
        func_ov036_021c6f7c(anime);
    }
}

static void func_ov036_021c6d58(FieldSaveAnime *anime, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_SAVEANIME, heapId);
    u32 gender = getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(Field_GetGameSystem(anime->field))));

    anime->bitmap = GFL_G2DIOLoadBitmap(ARCID_FIELD_SAVEANIME, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 1, 0, SAVE_ANIME_PALETTE_OFFSETS[gender] << 5, 0x120, 0x20, heapId);
    GFL_ArcToolFree(arc);
}

static void func_ov036_021c6db8(FieldSaveAnime *anime) {
    GFL_BitmapFree(anime->bitmap);
    anime->bitmap = NULL;
}

static void func_ov036_021c6dc8(FieldSaveAnime *anime) {
    VecFx32 pos;
    VecFx32 diff;
    VecFx32 flat;
    VecFx32 right;
    VecFx32 vertical;
    VecFx32 camPos;
    VecFx32 target;
    int x;
    int y;
    FieldPlayer *player = Field_GetPlayer(anime->field);
    G3DCamera *camera = FieldCamera_GetG3DCamera(Field_GetCameraSystem(anime->field));

    GFL_G3DCameraFlush(camera);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    GFL_G3DCameraGetLookatPos(camera, &camPos);
    VEC_Subtract(&target, &camPos, &diff);
    flat = diff;
    flat.y = 0;
    VEC_Normalize(&flat, &flat);
    VEC_CrossProduct(&flat, &SAVE_ANIME_UP, &right);
    VEC_CrossProduct(&diff, &right, &vertical);
    VEC_Normalize(&vertical, &vertical);
    FieldPlayer_GetWPos(player, &pos);
    pos.x += FX_Mul(vertical.x, -FX32_CONST(28));
    pos.y += FX_Mul(vertical.y, -FX32_CONST(28));
    pos.z += FX_Mul(vertical.z, -FX32_CONST(28));
    NNS_G3dWorldPosToScrPos(&pos, &x, &y);
    x -= 16;
    y -= 24;
    anime->offsetX = x % 8;
    anime->offsetY = y % 8;
    x /= 8;
    y /= 8;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x > 31) {
        x = 31;
    }
    if (y > 24) {
        y = 24;
    }
    anime->window = BmpWin_CreateDynamic(3, x, y, 5, 5, 9, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(anime->window), 0);
    BmpWin_FlushChar(anime->window);
    func_ov036_021c6f94(anime, 0);
}

static void func_ov036_021c6f54(FieldSaveAnime *anime) {
    BmpWin_Free(anime->window);
    anime->window = NULL;
}

static void func_ov036_021c6f64(FieldSaveAnime *anime) {
    BmpWin_FlushMap(anime->window);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysSetBGEnabled(3, TRUE);
}

static void func_ov036_021c6f7c(FieldSaveAnime *anime) {
    BmpWin_ClearScreen(anime->window);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysSetBGEnabled(3, FALSE);
}

static void func_ov036_021c6f94(FieldSaveAnime *anime, int frame) {
    u32 charPos = BmpWin_GetCharPos(anime->window);
    GFLBitmap *bitmap = BmpWin_GetBitmap(anime->window);

    GFL_BitmapCopyArea(anime->bitmap, bitmap, 0, frame << 5, anime->offsetX, anime->offsetY, 32, 32, 0xffff);
    GFL_BGSysLoadChar(3, GFL_BitmapGetPixelData(bitmap), GFL_BitmapCalcPixelDataSize(bitmap), charPos);
}

static void func_ov036_021c6fec(TCB *tcb, void *data) {
    FieldSaveAnime *anime = data;

    anime->timer++;
    if (SAVE_ANIME_STEPS[anime->frame].duration <= anime->timer) {
        anime->frame++;
        if (anime->frame >= 4) {
            anime->frame = 0;
            anime->timer = 0;
        }
        func_ov036_021c6f94(anime, SAVE_ANIME_STEPS[anime->frame].nextBitmapFrame);
    }
}
