#include "types.h"
#include "demo/intro.h"
#include "gfl/graphics.h"
#include "gfl/random.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "system/mcss.h"

// The intro's sprites, drawn by MCSS

#define SPRITE_COUNT 8

typedef struct {
    MCSS *mcss;
    BOOL animationEnded;
    // Whether the animation goes back to its first frame when it ends
    BOOL restartAtEnd;
} IntroSprite;

struct IntroMcss {
    HeapID heapId;
    MCSSSystem *system;
    IntroSprite sprites[SPRITE_COUNT];
    // Frames until the Pokémon plays its animation again
    u32 idleTimer;
};

static void IntroMcss_OnAnimationEnd(uptr param, fx32 frame);

static const VecFx32 sScale = { FX32_CONST(16), FX32_CONST(16), FX32_ONE };
// Pokémon face the other way
static const VecFx32 sPokemonScale = { -FX32_CONST(16), FX32_CONST(16), FX32_ONE };

IntroMcss *IntroMcss_Create(HeapID heapId, u32 mode) {
    IntroMcss *mcss = GFL_HeapAllocate(heapId, sizeof(IntroMcss), TRUE, "intro_mcss.c", 98);

    mcss->system = MCSSSys_Create(SPRITE_COUNT, heapId);
    mcss->heapId = heapId;
    if (mode != INTRO_MODE_PLAYER_NAMED && mode != INTRO_MODE_RIVAL_NAMED) {
        MCSSSys_SetTexBase(mcss->system, 0x70000);
    }
    MCSSSys_SetPlttBase(mcss->system, 0x4000);
    MCSSSys_EnableOrtho(mcss->system);
    return mcss;
}

void IntroMcss_Free(IntroMcss *mcss) {
    int i;

    for (i = 0; i < SPRITE_COUNT; i++) {
        if (mcss->sprites[i].mcss != NULL) {
            MCSSSys_Remove(mcss->system, mcss->sprites[i].mcss);
        }
    }
    MCSSSys_Free(mcss->system);
    GFL_HeapFree(mcss);
}

void IntroMcss_Draw(IntroMcss *mcss) {
    MCSSSys_Update(mcss->system);
    MCSSSys_Draw(mcss->system);
}

void IntroMcss_Add(IntroMcss *mcss, fx32 x, fx32 y, fx32 z, const MCSSLoadInfo *info, u8 index) {
    VecFx32 scale = sScale;

    mcss->sprites[index].mcss = MCSSSys_Add(mcss->system, x, y, z, info);
    MCSS_SetScale(mcss->sprites[index].mcss, &scale);
    mcss->sprites[index].animationEnded = FALSE;
    mcss->sprites[index].restartAtEnd = TRUE;
    MCSS_SetAnimationEndCallback(mcss->sprites[index].mcss, (uptr)&mcss->sprites[index], IntroMcss_OnAnimationEnd, 0);
}

void IntroMcss_AddPokemon(IntroMcss *mcss, fx32 x, fx32 y, fx32 z, u32 species, u8 index) {
    MCSSLoadInfo info;
    VecFx32 scale = sPokemonScale;
    PartyPkm *pkm = PokeParty_NewTempPkm(species, 0, 0, mcss->heapId);

    SetupPokemonLoaderByPartyPkm(pkm, &info, 0);
    GFL_HeapFree(pkm);
    mcss->sprites[index].mcss = MCSSSys_Add(mcss->system, x, y, z, &info);
    MCSS_SetScale(mcss->sprites[index].mcss, &scale);
    MCSS_PauseAnimation(mcss->sprites[index].mcss);
    mcss->sprites[index].animationEnded = FALSE;
    mcss->sprites[index].restartAtEnd = TRUE;
    mcss->idleTimer = GFL_RandomMTRange(5) * 20 + 200;
    MCSS_SetAnimationEndCallback(mcss->sprites[index].mcss, (uptr)&mcss->sprites[index], IntroMcss_OnAnimationEnd, 0);
}

void IntroMcss_SetVisible(IntroMcss *mcss, BOOL visible, u8 index) {
    if (visible == TRUE) {
        MCSS_Show(mcss->sprites[index].mcss);
    } else {
        MCSS_Hide(mcss->sprites[index].mcss);
    }
}

void IntroMcss_SetAnimation(IntroMcss *mcss, u8 index, u32 animation, BOOL restartAtEnd) {
    MCSS_SetAnimation(mcss->sprites[index].mcss, animation);
    MCSS_ResumeAnimation(mcss->sprites[index].mcss);
    mcss->sprites[index].animationEnded = FALSE;
    mcss->sprites[index].restartAtEnd = restartAtEnd;
    MCSS_SetAnimationEndCallback(mcss->sprites[index].mcss, (uptr)&mcss->sprites[index], IntroMcss_OnAnimationEnd, 0);
}

BOOL IntroMcss_IsAnimationEnded(IntroMcss *mcss, u8 index) {
    return mcss->sprites[index].animationEnded;
}

void IntroMcss_ClearAnimationEnded(IntroMcss *mcss, u8 index) {
    mcss->sprites[index].animationEnded = FALSE;
}

void IntroMcss_SetAlpha(IntroMcss *mcss, u8 index, u32 alpha) {
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 1, 9, 0, 0);
    if (alpha > 31) {
        alpha = 31;
    }
    MCSS_SetAlpha(mcss->sprites[index].mcss, alpha);
}

void IntroMcss_SetFlipV(IntroMcss *mcss, u8 index, BOOL a2) {
    if (a2) {
        MCSS_EnableFlipV(mcss->sprites[index].mcss);
    } else {
        MCSS_DisableFlipV(mcss->sprites[index].mcss);
    }
}

static void IntroMcss_OnAnimationEnd(uptr param, fx32 frame) {
    IntroSprite *sprite = (IntroSprite *)param;

    sprite->animationEnded = TRUE;
    if (sprite->restartAtEnd == TRUE) {
        MCSS_RestartAnimation(sprite->mcss);
    }
}

// Moves a sprite along x by step, and returns TRUE once it has reached target
BOOL IntroMcss_MoveX(IntroMcss *mcss, u8 index, fx32 step, fx32 target) {
    VecFx32 position;
    BOOL reached;

    MCSS_GetPosition(mcss->sprites[index].mcss, &position);
    reached = TRUE;
    position.x += step;
    if (position.x != target) {
        if ((step > 0 && position.x > target) || (step < 0 && position.x < target)) {
            position.x = target;
        } else {
            reached = FALSE;
        }
    }
    MCSS_SetPosition(mcss->sprites[index].mcss, &position);
    if (mcss->sprites[index].animationEnded == TRUE) {
        MCSS_PauseAnimation(mcss->sprites[index].mcss);
    }
    return reached;
}

// Moves sprite 1 down y by step, and returns TRUE once it has reached min
BOOL IntroMcss_DecreaseY(IntroMcss *mcss, fx32 step, fx32 min) {
    VecFx32 position;
    BOOL reached;

    MCSS_GetPosition(mcss->sprites[1].mcss, &position);
    position.y -= step;
    if (position.y <= min) {
        position.y = min;
        reached = TRUE;
    } else {
        reached = FALSE;
    }
    MCSS_SetPosition(mcss->sprites[1].mcss, &position);
    return reached;
}

// Plays the animation of sprite 1 again every 200 to 280 frames, keeping it still in between
void IntroMcss_Update(IntroMcss *mcss) {
    if (mcss->sprites[1].animationEnded) {
        if (mcss->idleTimer == 0) {
            mcss->idleTimer = GFL_RandomMTRange(5) * 20 + 200;
            IntroMcss_SetAnimation(mcss, 1, 0, TRUE);
        } else {
            MCSS_PauseAnimation(mcss->sprites[1].mcss);
            mcss->idleTimer--;
        }
    }
}
