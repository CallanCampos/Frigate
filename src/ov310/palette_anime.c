#include "types.h"
#include "app/research_radar/palette_anime.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/palanm.h"

// How the colors move toward the animation's color, as a blend of 0 to 16
enum {
    // Pulses, with the period and the strongest blend
    MODE_PULSE_60_10,
    MODE_PULSE_30_3,
    MODE_PULSE_90_3,
    // Flashes every 4 frames, then stops and puts the colors back
    MODE_FLASH_20_6,
    MODE_FLASH_10_6,
    MODE_FLASH_30_6,
    MODE_FLASH_20_9,
    // Fades to 7 and stays
    MODE_FADE_IN,
    // Fades from 7, then stops and puts the colors back
    MODE_FADE_OUT,
    MODE_HOLD,
};

struct PaletteAnime {
    HeapID heapId;
    BOOL setUp;
    u16 *dst;
    u16 *backup;
    u8 count;
    u16 color;
    BOOL active;
    u32 mode;
    u32 frames;
};

static void PaletteAnime_StartCore(PaletteAnime *anime, u32 mode, u16 color);
static void PaletteAnime_StopCore(PaletteAnime *anime);
static void PaletteAnime_UpdateCore(PaletteAnime *anime);
static void PaletteAnime_Pulse60(PaletteAnime *anime);
static void PaletteAnime_Pulse30(PaletteAnime *anime);
static void PaletteAnime_Pulse90(PaletteAnime *anime);
static void PaletteAnime_Flash20(PaletteAnime *anime);
static void PaletteAnime_Flash10(PaletteAnime *anime);
static void PaletteAnime_Flash30(PaletteAnime *anime);
static void PaletteAnime_FlashStrong20(PaletteAnime *anime);
static void PaletteAnime_FadeIn(PaletteAnime *anime);
static void PaletteAnime_FadeOut(PaletteAnime *anime);
static void PaletteAnime_Hold(PaletteAnime *anime);
static void PaletteAnime_RestoreCore(PaletteAnime *anime);
static BOOL PaletteAnime_IsActiveCore(PaletteAnime *anime);
static PaletteAnime *PaletteAnime_Alloc(HeapID heapId);
static void PaletteAnime_Free(PaletteAnime *anime);
static void PaletteAnime_InitWork(PaletteAnime *anime);
static void PaletteAnime_SetupCore(PaletteAnime *anime, u16 *dst, const u16 *src, u8 count);
static void PaletteAnime_Release(PaletteAnime *anime);

PaletteAnime *PaletteAnime_Create(HeapID heapId) {
    PaletteAnime *anime = PaletteAnime_Alloc(heapId);

    PaletteAnime_InitWork(anime);
    return anime;
}

void PaletteAnime_Delete(PaletteAnime *anime) {
    PaletteAnime_Release(anime);
    PaletteAnime_Free(anime);
}

void PaletteAnime_Setup(PaletteAnime *anime, u16 *dst, const u16 *src, u8 count) {
    PaletteAnime_SetupCore(anime, dst, src, count);
}

void PaletteAnime_Update(PaletteAnime *anime) {
    PaletteAnime_UpdateCore(anime);
}

void PaletteAnime_Start(PaletteAnime *anime, u32 mode, u16 color) {
    PaletteAnime_StartCore(anime, mode, color);
}

void PaletteAnime_Stop(PaletteAnime *anime) {
    PaletteAnime_StopCore(anime);
}

void PaletteAnime_Restore(PaletteAnime *anime) {
    PaletteAnime_RestoreCore(anime);
}

BOOL PaletteAnime_IsActive(PaletteAnime *anime) {
    return PaletteAnime_IsActiveCore(anime);
}

static void PaletteAnime_StartCore(PaletteAnime *anime, u32 mode, u16 color) {
    anime->active = TRUE;
    anime->mode = mode;
    anime->frames = 0;
    anime->color = color;
}

static void PaletteAnime_StopCore(PaletteAnime *anime) {
    anime->active = FALSE;
}

static void PaletteAnime_UpdateCore(PaletteAnime *anime) {
    if (!anime->setUp || !anime->active) {
        return;
    }

    switch (anime->mode) {
    case MODE_PULSE_60_10:
        PaletteAnime_Pulse60(anime);
        break;
    case MODE_PULSE_30_3:
        PaletteAnime_Pulse30(anime);
        break;
    case MODE_PULSE_90_3:
        PaletteAnime_Pulse90(anime);
        break;
    case MODE_FLASH_20_6:
        PaletteAnime_Flash20(anime);
        break;
    case MODE_FLASH_10_6:
        PaletteAnime_Flash10(anime);
        break;
    case MODE_FLASH_30_6:
        PaletteAnime_Flash30(anime);
        break;
    case MODE_FLASH_20_9:
        PaletteAnime_FlashStrong20(anime);
        break;
    case MODE_FADE_IN:
        PaletteAnime_FadeIn(anime);
        break;
    case MODE_FADE_OUT:
        PaletteAnime_FadeOut(anime);
        break;
    case MODE_HOLD:
        PaletteAnime_Hold(anime);
        break;
    }
}

static void PaletteAnime_Pulse60(PaletteAnime *anime) {
    u8 evy = (int)(16.0f + 16.0f * FX_FX32_TO_F32(FX_SinIdx((u16)((anime->frames << 16) / 60))));

    evy = evy * 10 / 32;
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
}

static void PaletteAnime_Pulse30(PaletteAnime *anime) {
    u8 evy = (int)(16.0f + 16.0f * FX_FX32_TO_F32(FX_SinIdx((u16)((anime->frames << 16) / 30))));

    evy = evy * 3 / 32;
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
}

static void PaletteAnime_Pulse90(PaletteAnime *anime) {
    u8 evy = (int)(16.0f + 16.0f * FX_FX32_TO_F32(FX_SinIdx((u16)((anime->frames << 16) / 90))));

    evy = evy * 3 / 32;
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
}

static void PaletteAnime_Flash20(PaletteAnime *anime) {
    u8 evy;

    if (((anime->frames >> 2) & 1) == 0) {
        evy = 0;
    } else {
        evy = 6;
    }
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
    if (anime->frames > 20) {
        PaletteAnime_StopCore(anime);
        PaletteAnime_RestoreCore(anime);
    }
}

static void PaletteAnime_Flash10(PaletteAnime *anime) {
    u8 evy;

    if (((anime->frames >> 2) & 1) == 0) {
        evy = 0;
    } else {
        evy = 6;
    }
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
    if (anime->frames > 10) {
        PaletteAnime_StopCore(anime);
        PaletteAnime_RestoreCore(anime);
    }
}

static void PaletteAnime_Flash30(PaletteAnime *anime) {
    u8 evy;

    if (((anime->frames >> 2) & 1) == 0) {
        evy = 0;
    } else {
        evy = 6;
    }
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
    if (anime->frames > 30) {
        PaletteAnime_StopCore(anime);
        PaletteAnime_RestoreCore(anime);
    }
}

static void PaletteAnime_FlashStrong20(PaletteAnime *anime) {
    u8 evy;

    if (((anime->frames >> 2) & 1) == 0) {
        evy = 0;
    } else {
        evy = 9;
    }
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
    if (anime->frames > 20) {
        PaletteAnime_StopCore(anime);
        PaletteAnime_RestoreCore(anime);
    }
}

static void PaletteAnime_FadeIn(PaletteAnime *anime) {
    u8 evy;

    if (anime->frames < 20) {
        evy = anime->frames * 7 / 20;
    } else {
        evy = 7;
    }
    BlendColors(anime->backup, anime->dst, anime->count, evy, anime->color);
    anime->frames++;
}

static void PaletteAnime_FadeOut(PaletteAnime *anime) {
    BlendColors(anime->backup, anime->dst, anime->count, 7 - anime->frames * 7 / 30, anime->color);
    anime->frames++;
    if (anime->frames > 30) {
        PaletteAnime_StopCore(anime);
        PaletteAnime_RestoreCore(anime);
    }
}

static void PaletteAnime_Hold(PaletteAnime *anime) {
    BlendColors(anime->backup, anime->dst, anime->count, 7, anime->color);
    anime->frames++;
}

static void PaletteAnime_RestoreCore(PaletteAnime *anime) {
    sys_memcpy(anime->backup, anime->dst, anime->count * sizeof(u16));
}

static BOOL PaletteAnime_IsActiveCore(PaletteAnime *anime) {
    return anime->active;
}

static PaletteAnime *PaletteAnime_Alloc(HeapID heapId) {
    PaletteAnime *anime = GFL_HeapAllocate(heapId, sizeof(PaletteAnime), FALSE, "palette_anime.c", 667);

    anime->heapId = heapId;
    return anime;
}

static void PaletteAnime_Free(PaletteAnime *anime) {
    GFL_HeapFree(anime);
}

static void PaletteAnime_InitWork(PaletteAnime *anime) {
    HeapID heapId = anime->heapId;

    sys_memset(anime, 0, sizeof(PaletteAnime));
    anime->heapId = heapId;
}

static void PaletteAnime_SetupCore(PaletteAnime *anime, u16 *dst, const u16 *src, u8 count) {
    int i;

    anime->setUp = TRUE;
    anime->dst = dst;
    anime->count = count;
    anime->backup = GFL_HeapAllocate(anime->heapId, count * sizeof(u16), FALSE, "palette_anime.c", 728);
    for (i = 0; i < count; i++) {
        anime->backup[i] = src[i];
    }
}

static void PaletteAnime_Release(PaletteAnime *anime) {
    GFL_HeapFree(anime->backup);
    anime->setUp = FALSE;
}
