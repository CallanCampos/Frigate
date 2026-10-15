#include "types.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "demo/shinka_demo.h"
#include "demo/shinka_demo_view.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "system/mcss.h"

// The evolving Pokémon. Its sprite turns white, then breaks into pieces that fly up into a spinning helix. While they
// spin, the pieces of the old form give way to those of the new one, which fly back into a sprite that fades in from
// white. The pieces are textured quads drawn with the geometry engine, and the rows of the sprite that have not broken
// away yet are drawn as one quad

// The sprite's 96 pixels are this wide in the pieces' 3D space
#define SPRITE_WIDTH 36.397023f
#define SPRITE_PIXELS 96
#define PIECE_ROWS 18
#define PIECE_COLUMNS 18
// The helix of pieces makes a turn for every three rows of the sprite
#define TURN_PIECES (PIECE_COLUMNS * 3)
// A piece has arrived when its squared distance to where it is going is under this
#define PIECE_ARRIVED_DIST_SQ 0x11ac

// The steps of the view
enum {
    VIEW_WAIT_START,
    VIEW_WAIT_CRY,
    VIEW_WAIT_EVOLVE,
    VIEW_WHITEN,
    VIEW_WAIT_WHITE,
    VIEW_WAIT_PIECES,
    VIEW_SHOW_PIECES,
    VIEW_UNUSED_7,
    VIEW_FADE_PIECES,
    VIEW_WAIT_PIECES_FADE,
    VIEW_MOVE_PIECES,
    VIEW_WAIT_RETURNED,
    VIEW_CANCELLED,
    VIEW_WAIT_REVEAL,
    VIEW_REVEAL,
    VIEW_WAIT_FADE_IN,
    VIEW_FADE_IN,
    VIEW_WAIT_FADE,
    VIEW_WAIT_CRY_START,
    VIEW_CRY,
    VIEW_WAIT_CRY_END,
    VIEW_DONE,
    // Created after the evolution has been shown
    VIEW_PLAYED,
};

// What the pieces of a sprite are doing, as far as the view is concerned
enum {
    PIECES_IDLE,
    PIECES_GATHERED,
    PIECES_WHITENING,
    PIECES_WHITE,
    PIECES_RETURNED,
    PIECES_UNWHITENING,
};

// How the pieces are moving
enum {
    MOVE_START,
    MOVE_GATHER,
    MOVE_SPIN,
    MOVE_RETURN,
    MOVE_DONE,
};

// How far a piece has got
enum {
    PIECE_WAIT,
    PIECE_GATHER,
    PIECE_SPIN,
    PIECE_RETURN,
    PIECE_HOME,
};

// The pieces' manager's steps
enum {
    MANAGER_WAIT,
    MANAGER_RUN,
    MANAGER_UNUSED_2,
    MANAGER_PLAYED,
};

typedef struct {
    s32 unk0;
    // The texture's left, right, top and bottom
    fx32 s0;
    fx32 s1;
    fx32 t0;
    fx32 t1;
    u8 alpha;
    VecFx16 vertices[4];
    u32 frame;
    // Where the piece is in the sprite
    VecFx32 home;
    VecFx32 velocity;
    // The piece's place in the helix: its angle from the helix's, its distance from the helix's axis and its height
    s32 angle;
    fx32 radius;
    fx32 height;
    VecFx32 target;
    s32 state;
    VecFx32 pos;
    VecFx32 prevPos;
} SpritePiece;

typedef struct {
    NNSG2dCharacterData *character;
    NNSG2dPaletteData *palette;
    void *characterFile;
    void *paletteFile;
    GFLBitmap *bitmap;
    u32 texAddr;
    u32 plttAddr;
    VecFx32 pos;
    s32 state;
    GXRgb colors[16];
    // The palette fades toward fadeColor, from fadeStart to fadeEnd out of 31 by fadeStep every fadeWait frames
    GXRgb fadeColor;
    s16 fadeStart;
    s16 fadeEnd;
    s16 fadeWait;
    s16 fadeStep;
    s16 fadeValue;
    s16 fadeCounter;
    // The next pieces to hide while the helix spins, going back and forward from the middle of the sprite
    s16 hideColumnBack;
    s16 hideRowBack;
    s16 hideColumn;
    s16 hideRow;
    s32 moveState;
    s32 angle;
    s32 angleSpeed;
    u32 frame;
    SpritePiece pieces[PIECE_ROWS][PIECE_COLUMNS];
    // The rows that are drawn as pieces
    u8 rowCount;
} SpritePieces;

// The rows of the sprite that have not broken into pieces, as one quad
typedef struct {
    u32 texAddr;
    u32 plttAddr;
    VecFx32 pos;
    VecFx32 scale;
    u32 polygonId;
    fx32 s0;
    fx32 s1;
    fx32 t0;
    fx32 t1;
    u8 alpha;
    VecFx16 vertices[4];
    u8 rowCount;
} SpriteRest;

typedef struct {
    s32 state;
    G3DCamera *camera;
    // The old and new forms
    SpritePieces *pieces[2];
    SpriteRest *rest;
} PiecesManager;

struct ShinkaDemoView {
    HeapID heapId;
    PartyPkm *pkm;
    BOOL played;
    u16 species;
    // A copy of the Pokémon that has evolved
    PartyPkm *evolved;
    s32 state;
    u32 frame;
    BOOL started;
    BOOL cryDone;
    BOOL evolveRequested;
    BOOL piecesReturned;
    BOOL revealRequested;
    BOOL fadeInRequested;
    BOOL done;
    BOOL piecesStarted;
    // Set with piecesReturned
    BOOL unk3C;
    BOOL cancelled;
    f32 scale;
    // Which of the sprites is shown
    u8 current;
    MCSSSystem *mcssSystem;
    // The old and new forms
    MCSS *mcss[2];
    PiecesManager *pieces;
    // FALSE while the pieces are drawn instead of the sprites
    BOOL mcssVisible;
};

static void ShinkaDemoView_InitMcss(ShinkaDemoView *view);
static void ShinkaDemoView_FreeMcss(ShinkaDemoView *view);
static fx32 ShinkaDemoView_GetSpriteY(PartyPkm *pkm);
static void ShinkaDemoView_AddMcss(ShinkaDemoView *view);
static void ShinkaDemoView_RemoveMcss(ShinkaDemoView *view);
static u8 ShinkaDemoView_OtherMcss(u8 index);
static void ShinkaDemoView_SetMcssOffsets(ShinkaDemoView *view);
static void ShinkaDemoView_SetMcssX(ShinkaDemoView *view, fx32 x);
static void ShinkaDemoView_InitPieces(ShinkaDemoView *view);
static void ShinkaDemoView_FreePieces(ShinkaDemoView *view);
static void ShinkaDemoView_UpdatePieces(ShinkaDemoView *view);
static void ShinkaDemoView_DrawPieces(ShinkaDemoView *view);
static void ShinkaDemoView_StartPieces(ShinkaDemoView *view);
static BOOL ShinkaDemoView_ArePiecesMoving(ShinkaDemoView *view);
static void ShinkaDemoView_StopPieces(ShinkaDemoView *view);
static void ShinkaDemoView_SetPiecesPosition(ShinkaDemoView *view, fx32 x, fx32 y, fx32 z);
static void ShinkaDemoPieces_SetPosition(SpritePieces *pieces, HeapID heapId, fx32 x, fx32 y, fx32 z);
static void ShinkaDemoView_UnwhitenPieces(ShinkaDemoView *view);
static BOOL ShinkaDemoView_ArePiecesIdle(ShinkaDemoView *view);
static BOOL ShinkaDemoView_ArePiecesReturned(ShinkaDemoView *view);
static SpritePieces *ShinkaDemoPieces_Create(u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 texAddr,
                                             u32 plttAddr, HeapID heapId);
static void ShinkaDemoPieces_Free(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_Init(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_Layout(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_Draw(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoView_RunPieces(ShinkaDemoView *view);
static void ShinkaDemoPieces_Move(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_SetFade(SpritePieces *pieces, HeapID heapId, GXRgb color, s16 start, s16 end, s16 wait,
                                     s16 step);
static BOOL ShinkaDemoPieces_IsFadeDone(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_UpdateFade(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_ApplyFade(SpritePieces *pieces, HeapID heapId);
static BOOL ShinkaDemoPieces_IsIdle(SpritePieces *pieces, HeapID heapId);
static BOOL ShinkaDemoPieces_IsGathered(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_Whiten(SpritePieces *pieces, HeapID heapId);
static BOOL ShinkaDemoPieces_IsWhite(SpritePieces *pieces, HeapID heapId);
static BOOL ShinkaDemoPieces_IsReturned(SpritePieces *pieces, HeapID heapId);
static void ShinkaDemoPieces_Update(SpritePieces *pieces, HeapID heapId);
static SpriteRest *ShinkaDemoRest_Create(u32 texAddr, u32 plttAddr, HeapID heapId);
static void ShinkaDemoRest_Free(SpriteRest *rest, HeapID heapId);
static void ShinkaDemoRest_Update(SpriteRest *rest, HeapID heapId);
static void ShinkaDemoRest_Draw(SpriteRest *rest, HeapID heapId);
static void ShinkaDemoRest_SetPosition(SpriteRest *rest, HeapID heapId, fx32 x, fx32 y, fx32 z);

// Declared in demo/shinka_demo_view.h. This order lays them out as the original has them
const f32 SHINKA_DEMO_HELIX_HEIGHT = 44.0f;
const f32 SHINKA_DEMO_HELIX_RADIUS_MAX = 20.0f;
const u32 SHINKA_DEMO_VIEW_UNK_774 = 774;
const f32 SHINKA_DEMO_HELIX_TOP = 22.0f;
const f32 SHINKA_DEMO_HELIX_RADIUS_MIN = 10.0f;

// The top of the rest of the sprite's quad and texture for each count of rows broken away
static const fx16 sRestTops[PIECE_ROWS + 1] = {
    0x800, 0x72b,  0x655,  0x555,  0x480,  0x3ab,  0x2ab,  0x1d5,  0x100,  0,
    -0xd5, -0x1ab, -0x2ab, -0x380, -0x455, -0x555, -0x62b, -0x700, -0x800,
};

static const u8 sRestTexTops[PIECE_ROWS + 1] = {
    16, 21, 26, 32, 37, 42, 48, 53, 58, 64, 69, 74, 80, 85, 90, 96, 101, 106, 112,
};

ShinkaDemoView *ShinkaDemoView_Create(HeapID heapId, BOOL played, PartyPkm *pkm, u16 species) {
    ShinkaDemoView *view = GFL_HeapAllocate(heapId, sizeof(ShinkaDemoView), TRUE, "shinka_demo_view.c", 513);

    view->heapId = heapId;
    view->played = played;
    view->pkm = pkm;
    view->species = species;
    view->evolved = NULL;
    if (played == FALSE) {
        view->evolved = GFL_HeapAllocate(view->heapId, PokeParty_GetPkmRawSize(), TRUE, "shinka_demo_view.c", 531);
        copyPartyPkm(view->pkm, view->evolved);
        setChangedPkmSpecies(view->evolved, view->species);
    }
    if (view->played == FALSE) {
        view->state = VIEW_WAIT_START;
    } else {
        view->state = VIEW_PLAYED;
    }
    view->frame = 0;
    view->started = FALSE;
    view->cryDone = FALSE;
    view->evolveRequested = FALSE;
    view->done = FALSE;
    view->piecesStarted = FALSE;
    view->unk3C = FALSE;
    view->cancelled = FALSE;
    view->scale = 16.0f;
    if (view->played == FALSE) {
        view->current = 0;
    } else {
        view->current = 1;
    }
    ShinkaDemoView_InitMcss(view);
    ShinkaDemoView_AddMcss(view);
    ShinkaDemoView_InitPieces(view);
    view->mcssVisible = TRUE;
    return view;
}

void ShinkaDemoView_Free(ShinkaDemoView *view) {
    ShinkaDemoView_FreePieces(view);
    ShinkaDemoView_RemoveMcss(view);
    ShinkaDemoView_FreeMcss(view);
    if (view->evolved != NULL) {
        GFL_HeapFree(view->evolved);
    }
    GFL_HeapFree(view);
}

void ShinkaDemoView_Update(ShinkaDemoView *view) {
    PartyPkm *pkm;

    switch (view->state) {
    case VIEW_WAIT_START:
        if (view->started) {
            view->state = VIEW_WAIT_CRY;
            PokeVoice_Play(PokeParty_GetParam(view->pkm, PKM_PARAM_SPECIES, NULL),
                           PokeParty_GetParam(view->pkm, PKM_PARAM_FORM, NULL), 64, 0, 0, 0, 0, 0);
        }
        break;
    case VIEW_WAIT_CRY:
        if (PokeVoice_IsPlayingAny() == FALSE) {
            view->state = VIEW_WAIT_EVOLVE;
            view->cryDone = TRUE;
        }
        break;
    case VIEW_WAIT_EVOLVE:
        if (view->evolveRequested) {
            view->state = VIEW_WHITEN;
        }
        break;
    case VIEW_WHITEN:
        view->state = VIEW_WAIT_WHITE;
        func_0201ae2c(view->mcss[view->current], 0, 16, 0, GX_RGB(31, 31, 31));
        func_0201ae2c(view->mcss[ShinkaDemoView_OtherMcss(view->current)], 16, 16, 0, GX_RGB(31, 31, 31));
        MCSS_PauseAnimation(view->mcss[view->current]);
        break;
    case VIEW_WAIT_WHITE:
        if (func_0201aee8(view->mcss[view->current]) == FALSE) {
            view->state = VIEW_WAIT_PIECES;
        }
        break;
    case VIEW_WAIT_PIECES:
        if (view->frame >= 8) {
            view->state = VIEW_SHOW_PIECES;
            view->frame = 0;
        } else {
            view->frame++;
        }
        break;
    case VIEW_SHOW_PIECES:
        view->state = VIEW_FADE_PIECES;
        ShinkaDemoView_SetMcssX(view, FX32_CONST(256));
        ShinkaDemoView_SetPiecesPosition(view, 0, FX32_CONST(-0.5), 0);
        view->mcssVisible = FALSE;
        NNS_G2dRestartMCAnimation(func_0201adc4(view->mcss[view->current]));
        break;
    case VIEW_UNUSED_7:
        if (view->frame >= 1) {
            view->state = VIEW_FADE_PIECES;
            view->frame = 0;
        } else {
            view->frame++;
        }
        break;
    case VIEW_FADE_PIECES:
        view->state = VIEW_WAIT_PIECES_FADE;
        ShinkaDemoView_UnwhitenPieces(view);
        break;
    case VIEW_WAIT_PIECES_FADE:
        if (ShinkaDemoView_ArePiecesIdle(view)) {
            view->state = VIEW_MOVE_PIECES;
            view->piecesStarted = TRUE;
            ShinkaDemoView_StartPieces(view);
        }
        break;
    case VIEW_MOVE_PIECES:
        if (ShinkaDemoView_ArePiecesReturned(view)) {
            view->state = VIEW_WAIT_RETURNED;
        }
        break;
    case VIEW_WAIT_RETURNED:
        if (view->frame >= 30) {
            view->state = VIEW_WAIT_REVEAL;
            view->frame = 0;
            view->unk3C = TRUE;
            view->piecesReturned = TRUE;
        } else {
            view->frame++;
        }
        break;
    case VIEW_CANCELLED:
        if (ShinkaDemoView_ArePiecesReturned(view)) {
            view->state = VIEW_WAIT_REVEAL;
            view->unk3C = TRUE;
            view->piecesReturned = TRUE;
        }
        break;
    case VIEW_WAIT_REVEAL:
        if (view->revealRequested) {
            view->state = VIEW_REVEAL;
        }
        break;
    case VIEW_REVEAL:
        view->state = VIEW_WAIT_FADE_IN;
        if (view->cancelled) {
            view->frame = 0;
        } else {
            view->current = ShinkaDemoView_OtherMcss(view->current);
            MCSS_Show(view->mcss[view->current]);
            MCSS_Hide(view->mcss[ShinkaDemoView_OtherMcss(view->current)]);
            view->frame = 30;
        }
        ShinkaDemoView_SetMcssX(view, 0);
        ShinkaDemoView_SetPiecesPosition(view, FX32_CONST(256), FX32_CONST(-0.5), 0);
        view->mcssVisible = TRUE;
        break;
    case VIEW_WAIT_FADE_IN:
        if (view->fadeInRequested) {
            view->state = VIEW_FADE_IN;
        }
        break;
    case VIEW_FADE_IN:
        view->state = VIEW_WAIT_FADE;
        func_0201ae2c(view->mcss[view->current], 16, 0, 1, GX_RGB(31, 31, 31));
        break;
    case VIEW_WAIT_FADE:
        if (func_0201aee8(view->mcss[view->current]) == FALSE) {
            view->state = VIEW_WAIT_CRY_START;
            MCSS_ResumeAnimation(view->mcss[view->current]);
        }
        break;
    case VIEW_WAIT_CRY_START:
        if (view->frame == 0) {
            view->state = VIEW_CRY;
            view->frame = 0;
        } else {
            view->frame--;
        }
        break;
    case VIEW_CRY:
        view->state = VIEW_WAIT_CRY_END;
        if (view->cancelled) {
            pkm = view->pkm;
        } else {
            pkm = view->evolved;
        }
        PokeVoice_Play(PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL), PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL),
                       64, 0, 0, 0, 0, 0);
        break;
    case VIEW_WAIT_CRY_END:
        if (PokeVoice_IsPlayingAny() == FALSE) {
            view->state = VIEW_DONE;
            view->frame = 0;
            view->done = TRUE;
        }
        break;
    case VIEW_DONE:
    case VIEW_PLAYED:
        break;
    }
    if (view->mcssVisible) {
        MCSSSys_Update(view->mcssSystem);
    }
    ShinkaDemoView_UpdatePieces(view);
}

void ShinkaDemoView_Draw(ShinkaDemoView *view) {
    if (view->mcssVisible) {
        MCSSSys_Draw(view->mcssSystem);
    } else {
        ShinkaDemoView_DrawPieces(view);
    }
}

void ShinkaDemoView_Start(ShinkaDemoView *view) {
    view->started = TRUE;
}

BOOL ShinkaDemoView_IsCryDone(ShinkaDemoView *view) {
    return view->cryDone;
}

void ShinkaDemoView_Evolve(ShinkaDemoView *view) {
    view->evolveRequested = TRUE;
}

BOOL ShinkaDemoView_HavePiecesReturned(ShinkaDemoView *view) {
    return view->piecesReturned;
}

void ShinkaDemoView_Reveal(ShinkaDemoView *view) {
    view->revealRequested = TRUE;
}

void ShinkaDemoView_FadeIn(ShinkaDemoView *view) {
    view->fadeInRequested = TRUE;
}

BOOL ShinkaDemoView_IsDone(ShinkaDemoView *view) {
    return view->done;
}

BOOL ShinkaDemoView_HavePiecesStarted(ShinkaDemoView *view) {
    return view->piecesStarted;
}

BOOL ShinkaDemoView_GetUnk3C(ShinkaDemoView *view) {
    return view->unk3C;
}

BOOL ShinkaDemoView_Cancel(ShinkaDemoView *view) {
    if (ShinkaDemoView_ArePiecesMoving(view)) {
        view->cancelled = TRUE;
        view->state = VIEW_CANCELLED;
        ShinkaDemoView_StopPieces(view);
        return TRUE;
    }
    return FALSE;
}

static void ShinkaDemoView_InitMcss(ShinkaDemoView *view) {
    view->mcssSystem = MCSSSys_Create(2, view->heapId);
    func_0201aefc(view->mcssSystem, 0x30000);
    func_0201aacc(view->mcssSystem);
}

static void ShinkaDemoView_FreeMcss(ShinkaDemoView *view) {
    MCSSSys_Free(view->mcssSystem);
}

static fx32 ShinkaDemoView_GetSpriteY(PartyPkm *pkm) {
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    fx32 y = FX32_CONST(-18);

    switch (species) {
    case SPECIES_LAMPENT:
        y = FX32_CONST(-27);
        break;
    case SPECIES_CHANDELURE:
        y = FX32_CONST(-39);
        break;
    case SPECIES_KADABRA:
        y = FX32_CONST(-17.9f);
        break;
    }
    return y;
}

static void ShinkaDemoView_AddMcss(ShinkaDemoView *view) {
    u8 i;
    VecFx32 scale;

    for (i = 0; i < NELEMS(view->mcss); i++) {
        view->mcss[i] = NULL;
    }
    scale.x = FX32_CONST(view->scale);
    scale.y = FX32_CONST(view->scale);
    scale.z = FX32_ONE;
    if (view->played == FALSE) {
        for (i = 0; i < NELEMS(view->mcss); i++) {
            if (i == 0) {
                view->mcss[i] =
                    func_0201c14c(view->mcssSystem, view->pkm, 0, 0, ShinkaDemoView_GetSpriteY(view->pkm), 0);
            } else {
                view->mcss[i] =
                    func_0201c14c(view->mcssSystem, view->evolved, 0, 0, ShinkaDemoView_GetSpriteY(view->evolved), 0);
            }
            func_0201aecc(view->mcss[i], 1);
            func_0201c290(view->mcss[i]);
            MCSS_SetScale(view->mcss[i], &scale);
            if (i != 0) {
                MCSS_Hide(view->mcss[i]);
                MCSS_PauseAnimation(view->mcss[i]);
            }
        }
    } else {
        view->mcss[1] = func_0201c14c(view->mcssSystem, view->pkm, 0, 0, ShinkaDemoView_GetSpriteY(view->pkm), 0);
        func_0201aecc(view->mcss[1], 1);
        func_0201c290(view->mcss[1]);
        MCSS_SetScale(view->mcss[1], &scale);
    }
    ShinkaDemoView_SetMcssOffsets(view);
}

static void ShinkaDemoView_RemoveMcss(ShinkaDemoView *view) {
    u8 i;

    for (i = 0; i < NELEMS(view->mcss); i++) {
        if (view->mcss[i] != NULL) {
            MCSS_Hide(view->mcss[i]);
            MCSSSys_Remove(view->mcssSystem, view->mcss[i]);
        }
    }
}

static u8 ShinkaDemoView_OtherMcss(u8 index) {
    return index == 0 ? 1 : 0;
}

static void ShinkaDemoView_SetMcssOffsets(ShinkaDemoView *view) {
    u8 i;
    f32 height;
    f32 top;
    f32 left;
    VecFx32 offset;

    for (i = 0; i < NELEMS(view->mcss); i++) {
        if (view->mcss[i] != NULL) {
            height = func_0201ade8(view->mcss[i]);
            top = func_0201adf8(view->mcss[i]);
            left = func_0201adf0(view->mcss[i]);
            height *= view->scale / 16.0f;
            if (height > 96.0f) {
                height = 96.0f;
            }
            top *= view->scale / 16.0f;
            left *= view->scale / 16.0f;
            height = ((96.0f - height) / 2.0f + top) * 0.33f;
            left = (0.0f - left) * 0.33f;
            offset.x = FX32_CONST(left);
            offset.y = FX32_CONST(height);
            offset.z = 0;
            func_0201ab54(view->mcss[i], &offset);
        }
    }
}

static void ShinkaDemoView_SetMcssX(ShinkaDemoView *view, fx32 x) {
    u8 i;
    VecFx32 pos;

    for (i = 0; i < NELEMS(view->mcss); i++) {
        if (view->mcss[i] != NULL) {
            MCSS_GetPosition(view->mcss[i], &pos);
            pos.x = x;
            MCSS_SetPosition(view->mcss[i], &pos);
        }
    }
}

static void ShinkaDemoView_InitPieces(ShinkaDemoView *view) {
    PiecesManager *manager = GFL_HeapAllocate(view->heapId, sizeof(PiecesManager), TRUE, "shinka_demo_view.c", 1356);
    SpritePieces *pieces;
    u32 species;
    u32 form;
    u32 sex;
    BOOL rare;
    u8 row;
    u8 col;

    manager->pieces[0] = NULL;
    manager->pieces[1] = NULL;
    manager->rest = NULL;
    if (view->played == FALSE) {
        manager->state = MANAGER_WAIT;
    } else {
        manager->state = MANAGER_PLAYED;
    }
    species = PokeParty_GetParam(view->pkm, PKM_PARAM_SPECIES, NULL);
    form = PokeParty_GetParam(view->pkm, PKM_PARAM_FORM, NULL);
    sex = PokeParty_GetSex(view->pkm);
    rare = PokeParty_IsRare(view->pkm);
    pieces = manager->pieces[0] = ShinkaDemoPieces_Create(species, form, sex, rare, 0, 0, 0x26000, 0x960, view->heapId);
    if (view->played == FALSE) {
        ShinkaDemoPieces_Init(pieces, view->heapId);
        pieces->state = PIECES_WHITE;
        ShinkaDemoPieces_SetFade(pieces, view->heapId, GX_RGB(31, 31, 31), 31, 31, 0, 31);
    } else {
        ShinkaDemoPieces_Layout(pieces, view->heapId);
    }
    if (view->played == FALSE) {
        species = PokeParty_GetParam(view->evolved, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(view->evolved, PKM_PARAM_FORM, NULL);
        sex = PokeParty_GetSex(view->evolved);
        rare = PokeParty_IsRare(view->evolved);
        pieces = manager->pieces[1] =
            ShinkaDemoPieces_Create(species, form, sex, rare, 0, 0, 0x28000, 0x980, view->heapId);
        ShinkaDemoPieces_Init(pieces, view->heapId);
        pieces->state = PIECES_WHITE;
        ShinkaDemoPieces_SetFade(pieces, view->heapId, GX_RGB(31, 31, 31), 31, 31, 0, 31);
        for (row = 0; row < PIECE_ROWS; row++) {
            for (col = 0; col < PIECE_COLUMNS; col++) {
                pieces->pieces[row][col].alpha = 0;
            }
        }
    }
    if (view->played == FALSE) {
        manager->rest = ShinkaDemoRest_Create(0x26000, 0x960, view->heapId);
    }
    view->pieces = manager;
    ShinkaDemoView_SetPiecesPosition(view, FX32_CONST(256), FX32_CONST(-0.5), 0);
    {
        VecFx32 cameraPosition = { 0, 0, FX32_CONST(100) };
        VecFx32 cameraUp = { 0, FX32_ONE, 0 };
        VecFx32 cameraTarget = { 0, 0, 0 };

        manager->camera = GFL_G3DCameraCreate(
            G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0),
            0, FX32_ONE, FX32_CONST(1024), 0, &cameraPosition, &cameraUp, &cameraTarget, view->heapId);
    }
}

static void ShinkaDemoView_FreePieces(ShinkaDemoView *view) {
    PiecesManager *manager = view->pieces;
    u8 i;

    GFL_G3DCameraFree(manager->camera);
    if (manager->rest != NULL) {
        ShinkaDemoRest_Free(manager->rest, view->heapId);
    }
    for (i = 0; i < NELEMS(manager->pieces); i++) {
        if (manager->pieces[i] != NULL) {
            ShinkaDemoPieces_Free(manager->pieces[i], view->heapId);
        }
    }
    GFL_HeapFree(manager);
}

static void ShinkaDemoView_UpdatePieces(ShinkaDemoView *view) {
    PiecesManager *manager = view->pieces;
    HeapID heapId = view->heapId;
    u8 i;
    SpritePieces *pieces;

    switch (manager->state) {
    case MANAGER_WAIT:
        break;
    case MANAGER_RUN:
        ShinkaDemoView_RunPieces(view);
        break;
    case MANAGER_UNUSED_2:
        break;
    case MANAGER_PLAYED:
        break;
    }
    for (i = 0; i < NELEMS(manager->pieces); i++) {
        pieces = manager->pieces[i];
        if (pieces != NULL) {
            if (view->cancelled && ShinkaDemoPieces_IsWhite(pieces, heapId)) {
                pieces->state = PIECES_RETURNED;
            }
            ShinkaDemoPieces_Update(pieces, heapId);
        }
    }
}

static void ShinkaDemoView_DrawPieces(ShinkaDemoView *view) {
    PiecesManager *manager = view->pieces;
    G3DCameraProjection projection;
    FxLookAt lookAt;
    u8 i;

    GFL_G3DSysMtxGetProjection(&projection);
    GFL_G3DSysMtxGetViewLookAt(&lookAt);
    {
        VecFx32 scale = { FX32_ONE, FX32_ONE, FX32_ONE };
        MtxFx33 rot;
        VecFx32 trans = { 0, 0, 0 };

        MTX_Identity33_(&rot);
        NNS_G3dGlbSetBaseTrans(&trans);
        NNS_G3dGlbSetBaseRot(&rot);
        NNS_G3dGlbSetBaseScale(&scale);
    }
    GFL_G3DCameraFlush(manager->camera);
    GFL_G3DSysMtxViewFlush();
    NNS_G3DWaitFIFO();
    G3_MtxMode(GX_MTXMODE_TEXTURE);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_MaterialColorDiffAmb(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    G3_MaterialColorSpecEmi(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    for (i = 0; i < NELEMS(manager->pieces); i++) {
        if (manager->pieces[i] != NULL) {
            ShinkaDemoPieces_Draw(manager->pieces[i], view->heapId);
        }
    }
    if (manager->rest != NULL) {
        ShinkaDemoRest_Draw(manager->rest, view->heapId);
    }
    GFL_G3DSysMtxSetProjection(&projection);
    GFL_G3DSysMtxSetViewLookAt(&lookAt);
    GFL_G3DSysMtxViewFlush();
    NNS_G3DWaitFIFO();
}

static void ShinkaDemoView_StartPieces(ShinkaDemoView *view) {
    PiecesManager *manager = view->pieces;

    if (manager->state == MANAGER_WAIT) {
        manager->state = MANAGER_RUN;
    }
}

static BOOL ShinkaDemoView_ArePiecesMoving(ShinkaDemoView *view) {
    PiecesManager *manager = view->pieces;

    if (manager->state != MANAGER_WAIT && manager->state != 4) {
        return TRUE;
    }
    return FALSE;
}

// Makes the old form's pieces go on to fade as if they had gathered, unless they already have
static void ShinkaDemoView_StopPieces(ShinkaDemoView *view) {
    SpritePieces *pieces = view->pieces->pieces[0];

    if (pieces->state != PIECES_GATHERED && pieces->state != PIECES_WHITENING && pieces->state != PIECES_WHITE) {
        pieces->state = PIECES_GATHERED;
    }
}

static void ShinkaDemoView_SetPiecesPosition(ShinkaDemoView *view, fx32 x, fx32 y, fx32 z) {
    PiecesManager *manager = view->pieces;
    u8 i;

    for (i = 0; i < NELEMS(manager->pieces); i++) {
        if (manager->pieces[i] != NULL) {
            ShinkaDemoPieces_SetPosition(manager->pieces[i], view->heapId, x, y, z);
        }
    }
    if (manager->rest != NULL) {
        ShinkaDemoRest_SetPosition(manager->rest, view->heapId, x, y, z);
    }
}

static void ShinkaDemoPieces_SetPosition(SpritePieces *pieces, HeapID heapId, fx32 x, fx32 y, fx32 z) {
    pieces->pos.x = x;
    pieces->pos.y = y;
    pieces->pos.z = z;
}

// Fades the pieces from white to their colors
static void ShinkaDemoView_UnwhitenPieces(ShinkaDemoView *view) {
    HeapID heapId = view->heapId;
    PiecesManager *manager = view->pieces;
    u8 i;
    SpritePieces *pieces;

    for (i = 0; i < NELEMS(manager->pieces); i++) {
        pieces = manager->pieces[i];
        if (pieces != NULL) {
            pieces->state = PIECES_UNWHITENING;
            ShinkaDemoPieces_SetFade(pieces, heapId, GX_RGB(31, 31, 31), 31, 0, 0, -1);
        }
    }
}

static BOOL ShinkaDemoView_ArePiecesIdle(ShinkaDemoView *view) {
    HeapID heapId = view->heapId;
    PiecesManager *manager = view->pieces;
    BOOL idle = TRUE;
    u8 i;

    for (i = 0; i < NELEMS(manager->pieces); i++) {
        if (manager->pieces[i] != NULL && ShinkaDemoPieces_IsIdle(manager->pieces[i], heapId) == FALSE) {
            idle = FALSE;
        }
    }
    return idle;
}

static BOOL ShinkaDemoView_ArePiecesReturned(ShinkaDemoView *view) {
    HeapID heapId = view->heapId;
    PiecesManager *manager = view->pieces;
    BOOL returned = TRUE;
    u8 i;

    for (i = 0; i < NELEMS(manager->pieces); i++) {
        if (manager->pieces[i] != NULL && ShinkaDemoPieces_IsReturned(manager->pieces[i], heapId) == FALSE) {
            returned = FALSE;
        }
    }
    return returned;
}

// Loads a Pokémon's sprite as a 128 by 128 texture, with the sprite's 96 by 96 pixels 16 pixels from its top left
static SpritePieces *ShinkaDemoPieces_Create(u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 texAddr,
                                             u32 plttAddr, HeapID heapId) {
    SpritePieces *pieces = GFL_HeapAllocate(heapId, sizeof(SpritePieces), TRUE, "shinka_demo_view.c", 1824);
    u32 characterNo;
    u32 paletteNo;
    u8 *src;
    u8 *pixels;
    int character;
    int x;
    int y;

    pieces->bitmap = GFL_BitmapCreate(16, 16, 32, heapId);
    pieces->texAddr = texAddr;
    pieces->plttAddr = plttAddr;
    characterNo = GetPokemonSingleCellCharacterDataNo(GetPokemonGraphicsARCID(), species, form, sex, rare, dir, egg);
    paletteNo = GetPokemonPaletteDataNo(GetPokemonGraphicsARCID(), species, form, sex, rare, dir, egg);
    character = 0;
    pieces->character = NULL;
    pieces->palette = NULL;
    pieces->characterFile =
        GFL_G2DIOReadOBJNCGR(GetPokemonGraphicsARCID(), characterNo, TRUE, &pieces->character, heapId);
    pieces->paletteFile = GFL_G2DIOReadNCLR(GetPokemonGraphicsARCID(), paletteNo, &pieces->palette, heapId);
    src = pieces->character->rawData;
    pixels = GFL_BitmapGetPixelData(pieces->bitmap);
    sys_memset32(0, pixels, 16 * 16 * 32);
    // The sprite's characters come in cells of 64 by 64, 32 by 64, 64 by 32 and 32 by 32 pixels
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            sys_memcpy32(src + character * 32, pixels + (2 * 16 + 2) * 32 + (x + y * 16) * 32, 32);
            character++;
        }
    }
    for (y = 0; y < 8; y++) {
        for (x = 8; x < 12; x++) {
            sys_memcpy32(src + character * 32, pixels + (2 * 16 + 2) * 32 + (x + y * 16) * 32, 32);
            character++;
        }
    }
    for (y = 8; y < 12; y++) {
        for (x = 0; x < 8; x++) {
            sys_memcpy32(src + character * 32, pixels + (2 * 16 + 2) * 32 + (x + y * 16) * 32, 32);
            character++;
        }
    }
    for (y = 8; y < 12; y++) {
        for (x = 8; x < 12; x++) {
            sys_memcpy32(src + character * 32, pixels + (2 * 16 + 2) * 32 + (x + y * 16) * 32, 32);
            character++;
        }
    }
    GFL_BitmapMakeLinear(pieces->bitmap, FALSE, heapId);
    NNS_GfdRegisterNewVramTransferTask(0, texAddr, GFL_BitmapGetPixelData(pieces->bitmap), 16 * 16 * 32);
    NNS_GfdRegisterNewVramTransferTask(1, plttAddr, pieces->palette->rawData, 16 * sizeof(GXRgb));
    return pieces;
}

static void ShinkaDemoPieces_Free(SpritePieces *pieces, HeapID heapId) {
    GFL_BitmapFree(pieces->bitmap);
    GFL_HeapFree(pieces->characterFile);
    GFL_HeapFree(pieces->paletteFile);
    GFL_HeapFree(pieces);
}

static inline fx32 SquaredLengthXZ(fx32 x, fx32 z) {
    return FX_Mul(x, x) + FX_Mul(z, z);
}

// Lays the pieces out as the sprite, and gives each its place in the helix and the way there
static void ShinkaDemoPieces_Init(SpritePieces *pieces, HeapID heapId) {
    u8 i;
    u8 row;
    u8 col;
    u8 turn;
    u8 index;
    SpritePiece *piece;
    f32 y;
    f32 radius;
    fx32 distXZ;
    fx32 distY;
    fx32 dx;
    fx32 dy;
    fx32 dz;

    ShinkaDemoPieces_Layout(pieces, heapId);
    pieces->state = PIECES_IDLE;
    for (i = 0; i < NELEMS(pieces->colors); i++) {
        pieces->colors[i] = GX_RGB(31, 31, 31);
    }
    pieces->fadeColor = GX_RGB(31, 31, 31);
    pieces->fadeValue = 0;
    pieces->fadeWait = 0;
    pieces->fadeStep = 0;
    pieces->fadeCounter = 0;
    pieces->hideColumnBack = PIECE_COLUMNS - 1;
    pieces->hideRowBack = PIECE_ROWS / 2 - 1;
    pieces->hideColumn = 0;
    pieces->hideRow = pieces->hideRowBack + 1;
    pieces->moveState = MOVE_START;
    pieces->angle = 0;
    pieces->angleSpeed = 0;
    pieces->frame = 0;
    pieces->rowCount = 0;
    for (row = 0; row < PIECE_ROWS; row++) {
        for (col = 0; col < PIECE_COLUMNS; col++) {
            turn = row / 3;
            piece = &pieces->pieces[row][col];
            piece->frame = 0;
            piece->home = piece->pos;
            // A turn of the helix takes the left and right halves of its three rows in turn
            if (row % 3 == 0) {
                if (col < PIECE_COLUMNS / 2) {
                    index = col * 6;
                } else {
                    index = (col - PIECE_COLUMNS / 2) * 6 + 1;
                }
            } else if (row % 3 == 1) {
                if (col < PIECE_COLUMNS / 2) {
                    index = col * 6 + 2;
                } else {
                    index = (col - PIECE_COLUMNS / 2) * 6 + 3;
                }
            } else {
                if (col < PIECE_COLUMNS / 2) {
                    index = col * 6 + 4;
                } else {
                    index = (col - PIECE_COLUMNS / 2) * 6 + 5;
                }
            }
            piece->angle = (index << 16) / TURN_PIECES;
            piece->height = FX32_CONST(SHINKA_DEMO_HELIX_TOP - turn * (SHINKA_DEMO_HELIX_HEIGHT / 6) -
                                       index * (SHINKA_DEMO_HELIX_HEIGHT / 6 / TURN_PIECES));
            y = piece->height / 4096.0f;
            radius = SHINKA_DEMO_HELIX_RADIUS_MIN + (SHINKA_DEMO_HELIX_RADIUS_MAX - SHINKA_DEMO_HELIX_RADIUS_MIN) * y *
                                                        y / SHINKA_DEMO_HELIX_HEIGHT / SHINKA_DEMO_HELIX_HEIGHT * 2.0f *
                                                        2.0f;
            piece->radius = FX32_CONST(radius);
            piece->target.x = FX_Mul(piece->radius, FX_CosIdx(piece->angle));
            piece->target.z = FX_Mul(piece->radius, FX_SinIdx(piece->angle));
            piece->target.y = piece->height;
            dx = piece->target.x - piece->pos.x;
            dy = piece->target.y - piece->pos.y;
            dz = piece->target.z - piece->pos.z;
            distXZ = SquaredLengthXZ(dx, dz);
            distY = FX_Mul(dy, dy);
            distXZ = FX_Sqrt(distXZ);
            distY = FX_Sqrt(distY);
            if (distXZ > FX32_ONE) {
                piece->velocity.x = FX_Div(dx, distXZ);
                piece->velocity.z = FX_Div(dz, distXZ);
            } else {
                piece->velocity.x = dx;
                piece->velocity.z = dz;
            }
            if (distY > FX32_ONE) {
                piece->velocity.y = FX_Div(dy, distY);
            } else {
                piece->velocity.y = dy;
            }
            piece->state = PIECE_WAIT;
        }
    }
}

// Places the pieces where they make up the sprite
static void ShinkaDemoPieces_Layout(SpritePieces *pieces, HeapID heapId) {
    int i;
    f32 size;
    f32 pixels;
    f32 margin;
    f32 left;
    f32 top;
    f32 marginX;
    f32 marginY;
    int col;
    int row;
    SpritePiece *piece;

    pieces->pos.x = 0;
    pieces->pos.y = 0;
    pieces->pos.z = 0;
    pieces->rowCount = PIECE_ROWS;
    size = SPRITE_WIDTH / PIECE_COLUMNS;
    pixels = (f32)SPRITE_PIXELS / PIECE_COLUMNS;
    margin = 0.0f;
    left = -SPRITE_WIDTH / 2 + size / 2.0f;
    top = SPRITE_WIDTH / 2 - size / 2.0f;
    marginX = margin * (size / pixels);
    marginY = margin * (size / pixels);
    for (i = 0; i < PIECE_ROWS * PIECE_COLUMNS; i++) {
        col = i % PIECE_COLUMNS;
        row = i / PIECE_COLUMNS;
        piece = &pieces->pieces[row][col];
        piece->unk0 = 60;
        piece->alpha = 31;
        piece->vertices[0].x = FX32_CONST((0.0f - size) / 2.0f - marginX);
        piece->vertices[0].y = FX32_CONST(size / 2.0f + marginY);
        piece->vertices[0].z = 0;
        piece->vertices[1].x = FX32_CONST((0.0f - size) / 2.0f - marginX);
        piece->vertices[1].y = FX32_CONST((0.0f - size) / 2.0f - marginY);
        piece->vertices[1].z = 0;
        piece->vertices[2].x = FX32_CONST(size / 2.0f + marginX);
        piece->vertices[2].y = FX32_CONST((0.0f - size) / 2.0f - marginY);
        piece->vertices[2].z = 0;
        piece->vertices[3].x = FX32_CONST(size / 2.0f + marginX);
        piece->vertices[3].y = FX32_CONST(size / 2.0f + marginY);
        piece->vertices[3].z = 0;
        piece->pos = pieces->pos;
        piece->pos.x += FX32_CONST(left + size * col);
        piece->pos.y += FX32_CONST(top - size * row);
        piece->s0 = FX32_CONST(col * pixels + 16.0f - margin);
        piece->t0 = FX32_CONST(row * pixels + 16.0f - margin);
        piece->s1 = piece->s0 + FX32_CONST(pixels + 2.0f * margin);
        piece->t1 = piece->t0 + FX32_CONST(pixels + 2.0f * margin);
        piece->prevPos = piece->pos;
    }
}

static void ShinkaDemoPieces_Draw(SpritePieces *pieces, HeapID heapId) {
    int row;
    int col;
    SpritePiece *rowPieces;
    SpritePiece *piece;

    G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S128, GX_TEXSIZE_T128, GX_TEXREPEAT_ST,
                     GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, pieces->texAddr);
    G3_TexPlttBase(pieces->plttAddr, GX_TEXFMT_PLTT16);
    G3_PushMtx();
    G3_Translate(pieces->pos.x, pieces->pos.y, pieces->pos.z);
    G3_PolygonAttr(0, GX_POLYGONMODE_MODULATE, GX_CULL_BACK, 0, 31, 0);
    for (row = 0; row < pieces->rowCount; row++) {
        rowPieces = pieces->pieces[row];
        for (col = 0; col < PIECE_COLUMNS; col++) {
            piece = &rowPieces[col];
            if (piece->alpha != 0) {
                G3_PushMtx();
                G3_Translate(piece->pos.x, piece->pos.y, piece->pos.z);
                G3_Begin(GX_BEGIN_QUADS);
                G3_Normal(0, 0, -FX16_ONE);
                G3_TexCoord(piece->s0, piece->t0);
                G3_Vtx(piece->vertices[0].x, piece->vertices[0].y, piece->vertices[0].z);
                G3_TexCoord(piece->s0, piece->t1);
                G3_Vtx(piece->vertices[1].x, piece->vertices[1].y, piece->vertices[1].z);
                G3_TexCoord(piece->s1, piece->t1);
                G3_Vtx(piece->vertices[2].x, piece->vertices[2].y, piece->vertices[2].z);
                G3_TexCoord(piece->s1, piece->t0);
                G3_Vtx(piece->vertices[3].x, piece->vertices[3].y, piece->vertices[3].z);
                G3_End();
                G3_PopMtx(1);
            }
        }
    }
    G3_PopMtx(1);
}

// Moves the old form's pieces, has the new form's follow them, and shows the new form's pieces where the old form's
// have been hidden
static void ShinkaDemoView_RunPieces(ShinkaDemoView *view) {
    PiecesManager *manager = view->pieces;
    HeapID heapId = view->heapId;
    SpritePieces *old;
    SpritePieces *new;
    u8 row;
    u8 col;
    s16 c;
    s16 r;
    BOOL show;

    if (manager->state == MANAGER_RUN) {
        ShinkaDemoPieces_Move(manager->pieces[0], heapId);
        old = manager->pieces[0];
        new = manager->pieces[1];
        new->angle = old->angle;
        for (row = 0; row < PIECE_ROWS; row++) {
            for (col = 0; col < PIECE_COLUMNS; col++) {
                new->pieces[row][col].pos = old->pieces[row][col].pos;
            }
        }
        for (r = new->hideRowBack; r >= old->hideRowBack; r--) {
            if (r == new->hideRowBack) {
                c = new->hideColumnBack;
            } else {
                c = PIECE_COLUMNS - 1;
            }
            for (; c >= 0; c--) {
                if (r == old->hideRowBack) {
                    if (c <= old->hideColumnBack) {
                        break;
                    }
                    show = TRUE;
                } else {
                    show = TRUE;
                }
                if (show) {
                    new->pieces[r][c].alpha = 31;
                }
            }
        }
        new->hideColumnBack = old->hideColumnBack;
        new->hideRowBack = old->hideRowBack;
        for (r = new->hideRow; r <= old->hideRow; r++) {
            if (r == new->hideRow) {
                c = new->hideColumn;
            } else {
                c = 0;
            }
            for (; c < PIECE_COLUMNS; c++) {
                if (r == old->hideRow) {
                    if (c >= old->hideColumn) {
                        break;
                    }
                    show = TRUE;
                } else {
                    show = TRUE;
                }
                if (show) {
                    new->pieces[r][c].alpha = 31;
                }
            }
        }
        new->hideColumn = old->hideColumn;
        new->hideRow = old->hideRow;
        new->rowCount = old->rowCount;
        old = manager->pieces[0];
        new = manager->pieces[1];
        new->state = old->state;
        if (ShinkaDemoPieces_IsGathered(old, heapId)) {
            ShinkaDemoPieces_Whiten(old, heapId);
            ShinkaDemoPieces_Whiten(new, heapId);
        }
        ShinkaDemoPieces_IsReturned(old, heapId);
        if (manager->rest != NULL) {
            manager->rest->rowCount = manager->pieces[0]->rowCount;
            ShinkaDemoRest_Update(manager->rest, heapId);
        }
    }
}

static void ShinkaDemoPieces_Move(SpritePieces *pieces, HeapID heapId) {
    VecFx32 delta;
    s16 row;
    s16 col;
    s32 angle;
    SpritePiece *piece;
    SpritePiece *gatherPiece;
    VecFx32 target;
    MtxFx43 gatherMatrix;
    VecFx32 gatherVector;
    BOOL gatherFirst;
    BOOL gatherSecond;
    BOOL done;
    fx32 invDist;
    fx32 sign;
    BOOL nearXZ;
    BOOL nearY;
    BOOL returnFirst;
    SpritePiece *spinPiece;
    MtxFx43 spinMatrix;
    VecFx32 spinVector;
    int hideRow;
    int hideColumn;
    BOOL returnSecond;
    MtxFx43 returnSpinMatrix;
    VecFx32 returnSpinVector;
    MtxFx43 returnStartMatrix;
    VecFx32 returnStartVector;
    fx32 stepX;
    fx32 stepY;
    fx32 stepZ;
    fx32 returnHomeDx;
    fx32 returnHomeDz;

    pieces->angle += pieces->angleSpeed;
    if (pieces->angle >= 0x10000) {
        pieces->angle -= 0x10000;
    }
    for (row = 0; row < PIECE_ROWS; row++) {
        for (col = 0; col < PIECE_COLUMNS; col++) {
            piece = &pieces->pieces[row][col];
            piece->prevPos = piece->pos;
        }
    }
    switch (pieces->moveState) {
    case MOVE_START:
        pieces->moveState = MOVE_GATHER;
        pieces->frame = 0;
        pieces->angleSpeed = 0x600;
        break;
    case MOVE_GATHER:
        // Two pieces leave for the helix each frame, and wait there for the rest
        done = TRUE;

        gatherSecond = gatherFirst = TRUE;
        for (row = 0; row < PIECE_ROWS; row++) {
            for (col = 0; col < PIECE_COLUMNS; col++) {
                gatherPiece = &pieces->pieces[row][col];
                angle = gatherPiece->angle + pieces->angle;
                if (angle >= 0x10000) {
                    angle -= 0x10000;
                }
                MTX_RotY43_(&gatherMatrix, FX_SinIdx(angle), FX_CosIdx(angle));
                gatherVector.x = gatherPiece->radius;
                gatherVector.y = 0;
                gatherVector.z = 0;
                MTX_MultVec43(&gatherVector, &gatherMatrix, &target);
                target.y = gatherPiece->height;
                if (gatherPiece->state == PIECE_WAIT) {
                    if (gatherFirst || gatherSecond) {
                        gatherPiece->frame = 0;
                        gatherPiece->state = PIECE_GATHER;
                        if (gatherFirst) {
                            gatherFirst = FALSE;
                        } else {
                            gatherSecond = FALSE;
                        }
                    }
                    done = FALSE;
                } else if (gatherPiece->state == PIECE_GATHER) {
                    if (gatherPiece->frame == 0) {
                        gatherPiece->pos.x += gatherPiece->velocity.x;
                        gatherPiece->pos.y += gatherPiece->velocity.y;
                        gatherPiece->pos.z += gatherPiece->velocity.z;
                        gatherPiece->frame++;
                        pieces->rowCount = row + 1;
                    } else {
                        fx32 distSqXZ;
                        fx32 distSqY;

                        delta.y = gatherPiece->target.y - gatherPiece->pos.y;
                        delta.z = gatherPiece->target.z - gatherPiece->pos.z;
                        delta.x = gatherPiece->target.x - gatherPiece->pos.x;
                        distSqXZ = FX_Mul(delta.x, delta.x) + FX_Mul(delta.z, delta.z);
                        distSqY = FX_Mul(delta.y, delta.y);
                        if (distSqXZ < PIECE_ARRIVED_DIST_SQ && distSqY < PIECE_ARRIVED_DIST_SQ) {
                            gatherPiece->pos.x = gatherPiece->target.x;
                            gatherPiece->pos.z = gatherPiece->target.z;
                            gatherPiece->pos.y = gatherPiece->target.y;
                            gatherPiece->pos.x = target.x;
                            gatherPiece->pos.z = target.z;
                            gatherPiece->pos.y = target.y;
                            gatherPiece->frame = 0;
                            gatherPiece->state = PIECE_SPIN;
                        } else {
                            nearXZ = FALSE;
                            nearY = FALSE;
                            if (distSqXZ < PIECE_ARRIVED_DIST_SQ) {
                                nearXZ = TRUE;
                            } else if (distSqY < PIECE_ARRIVED_DIST_SQ) {
                                nearY = TRUE;
                            }
                            if (nearXZ) {
                                gatherPiece->pos.x = gatherPiece->target.x;
                                gatherPiece->pos.z = gatherPiece->target.z;
                            } else {
                                invDist = FX_InvSqrt(distSqXZ);
                                gatherPiece->pos.x =
                                    gatherPiece->pos.x +
                                    FX_Mul(gatherPiece->pos.x - gatherPiece->prevPos.x, FX32_ONE / 10) +
                                    FX_Mul(FX_Mul(delta.x, invDist), FX32_CONST(0.875));
                                gatherPiece->pos.z =
                                    gatherPiece->pos.z +
                                    FX_Mul(gatherPiece->pos.z - gatherPiece->prevPos.z, FX32_ONE / 10) +
                                    FX_Mul(FX_Mul(delta.z, invDist), FX32_CONST(0.875));
                            }
                            if (nearY) {
                                gatherPiece->pos.y = gatherPiece->target.y;
                            } else {
                                if (delta.y >= 0) {
                                    sign = FX32_ONE;
                                } else {
                                    sign = -FX32_ONE;
                                }
                                gatherPiece->pos.y =
                                    gatherPiece->pos.y +
                                    FX_Mul(gatherPiece->pos.y - gatherPiece->prevPos.y, FX32_ONE / 10) +
                                    FX_Mul(sign, FX32_CONST(0.875));
                            }
                            gatherPiece->frame++;
                        }
                    }
                    done = FALSE;
                } else if (gatherPiece->state == PIECE_SPIN) {
                    gatherPiece->pos.x = target.x;
                    gatherPiece->pos.z = target.z;
                    gatherPiece->pos.y = target.y;
                }
            }
        }
        if (done) {
            pieces->moveState = MOVE_SPIN;
            if (pieces->state != PIECES_GATHERED && pieces->state != PIECES_WHITENING) {
                pieces->state = PIECES_GATHERED;
            }
        }
        break;
    case MOVE_SPIN:

        if (pieces->frame < 100) {
            pieces->angleSpeed += 0x20;
        }
        if (pieces->frame >= 200 && pieces->frame < 240) {
            pieces->angleSpeed -= 0x20;
        }
        for (row = 0; row < PIECE_ROWS; row++) {
            for (col = 0; col < PIECE_COLUMNS; col++) {
                spinPiece = &pieces->pieces[row][col];
                angle = spinPiece->angle + pieces->angle;
                if (angle >= 0x10000) {
                    angle -= 0x10000;
                }
                MTX_RotY43_(&spinMatrix, FX_SinIdx(angle), FX_CosIdx(angle));
                spinVector.x = spinPiece->radius;
                spinVector.y = 0;
                spinVector.z = 0;
                MTX_MultVec43(&spinVector, &spinMatrix, &spinPiece->pos);
                spinPiece->pos.y = spinPiece->height;
            }
        }
        if (ShinkaDemoPieces_IsWhite(pieces, heapId) && pieces->frame >= 30) {
            hideColumn = pieces->hideColumnBack;
            hideRow = pieces->hideRowBack;
            if (hideRow >= 0 && hideColumn >= 0) {
                pieces->pieces[hideRow][hideColumn].alpha = 0;
                if (--hideColumn < 0) {
                    pieces->hideColumnBack = PIECE_COLUMNS;
                    pieces->hideRowBack--;
                } else {
                    pieces->hideColumnBack--;
                }
            }
            hideColumn = pieces->hideColumn;
            hideRow = pieces->hideRow;
            if (hideRow < PIECE_ROWS && hideColumn < PIECE_COLUMNS) {
                pieces->pieces[hideRow][hideColumn].alpha = 0;
                if (++hideColumn >= PIECE_COLUMNS) {
                    pieces->hideColumn = 0;
                    pieces->hideRow++;
                } else {
                    pieces->hideColumn++;
                }
            }
        }
        if (pieces->hideRowBack < 0 && pieces->hideRow >= PIECE_ROWS && pieces->frame >= 240) {
            pieces->moveState = MOVE_RETURN;
            pieces->frame = 0;
        } else {
            pieces->frame++;
        }
        break;
    case MOVE_RETURN:
        // Two pieces leave the helix each frame, from the last
        done = TRUE;

        returnSecond = returnFirst = TRUE;
        for (row = PIECE_ROWS - 1; row >= 0; row--) {
            for (col = PIECE_COLUMNS - 1; col >= 0; col--) {
                piece = &pieces->pieces[row][col];
                angle = piece->angle + pieces->angle;
                if (angle >= 0x10000) {
                    angle -= 0x10000;
                }
                if (piece->state == PIECE_SPIN) {
                    if (returnFirst || returnSecond) {
                        piece->frame = 0;
                        piece->state = PIECE_RETURN;
                        if (returnFirst) {
                            returnFirst = FALSE;
                        } else {
                            returnSecond = FALSE;
                        }
                    } else {

                        MTX_RotY43_(&returnSpinMatrix, FX_SinIdx(angle), FX_CosIdx(angle));
                        returnSpinVector.x = piece->radius;
                        returnSpinVector.y = 0;
                        returnSpinVector.z = 0;
                        MTX_MultVec43(&returnSpinVector, &returnSpinMatrix, &piece->pos);
                        piece->pos.y = piece->height;
                    }
                    done = FALSE;
                } else if (piece->state == PIECE_RETURN) {
                    if (piece->frame == 0) {
                        fx32 distSqXZ;
                        fx32 distSqY;

                        MTX_RotY43_(&returnStartMatrix, FX_SinIdx(angle), FX_CosIdx(angle));
                        returnStartVector.x = piece->radius;
                        returnStartVector.y = 0;
                        returnStartVector.z = 0;
                        MTX_MultVec43(&returnStartVector, &returnStartMatrix, &piece->pos);
                        piece->pos.y = piece->height;
                        delta.x = piece->pos.x - piece->prevPos.x;
                        delta.y = piece->pos.y - piece->prevPos.y;
                        delta.z = piece->pos.z - piece->prevPos.z;
                        distSqXZ = FX_Mul(delta.x, delta.x) + FX_Mul(delta.z, delta.z);
                        distSqY = FX_Mul(delta.y, delta.y);
                        if (distSqXZ > FX32_ONE) {
                            invDist = FX_InvSqrt(distSqXZ);
                            stepX = FX_Mul(delta.x, invDist);
                            stepZ = FX_Mul(delta.z, invDist);
                        } else {
                            stepX = delta.x;
                            stepZ = delta.z;
                        }
                        if (distSqY > FX32_ONE) {
                            if (delta.y >= 0) {
                                stepY = FX32_ONE;
                            } else {
                                stepY = -FX32_ONE;
                            }
                        } else {
                            stepY = delta.y;
                        }
                        piece->pos.x += stepX;
                        piece->pos.y += stepY;
                        piece->pos.z += stepZ;
                        piece->frame++;
                    } else {
                        fx32 distSqXZ;
                        fx32 distSqY;

                        returnHomeDx = piece->home.x - piece->pos.x;
                        delta.y = piece->home.y - piece->pos.y;
                        returnHomeDz = piece->home.z - piece->pos.z;
                        distSqXZ = FX_Mul(returnHomeDx, returnHomeDx) + FX_Mul(returnHomeDz, returnHomeDz);
                        distSqY = FX_Mul(delta.y, delta.y);
                        if (distSqXZ < PIECE_ARRIVED_DIST_SQ && distSqY < PIECE_ARRIVED_DIST_SQ) {
                            piece->pos.x = piece->home.x;
                            piece->pos.z = piece->home.z;
                            piece->pos.y = piece->home.y;
                            piece->frame = 0;
                            piece->state = PIECE_HOME;
                        } else {
                            if (distSqXZ > FX32_ONE) {
                                invDist = FX_InvSqrt(distSqXZ);
                                returnHomeDx = FX_Mul(returnHomeDx, invDist);
                                returnHomeDz = FX_Mul(returnHomeDz, invDist);
                            }
                            if (distSqY > FX32_ONE) {
                                if (delta.y >= 0) {
                                    delta.y = FX32_ONE;
                                } else {
                                    delta.y = -FX32_ONE;
                                }
                            }
                            piece->pos.x += returnHomeDx;
                            piece->pos.y += delta.y;
                            piece->pos.z += returnHomeDz;
                        }
                    }
                    done = FALSE;
                }
            }
        }
        if (done) {
            pieces->moveState = MOVE_DONE;
            pieces->state = PIECES_RETURNED;
        }
        break;
    case MOVE_DONE:
        break;
    }
}

static void ShinkaDemoPieces_SetFade(SpritePieces *pieces, HeapID heapId, GXRgb color, s16 start, s16 end, s16 wait,
                                     s16 step) {
    pieces->fadeColor = color;
    pieces->fadeStart = start;
    pieces->fadeEnd = end;
    pieces->fadeWait = wait;
    pieces->fadeStep = step;
    pieces->fadeStart = pieces->fadeStart > 31 ? 31 : (pieces->fadeStart < 0 ? 0 : pieces->fadeStart);
    pieces->fadeEnd = pieces->fadeEnd > 31 ? 31 : (pieces->fadeEnd < 0 ? 0 : pieces->fadeEnd);
    pieces->fadeStep = pieces->fadeStep > 31 ? 31 : (pieces->fadeStep < -31 ? -31 : pieces->fadeStep);
    if (pieces->fadeStart < pieces->fadeEnd) {
        if (pieces->fadeStep < 0) {
            pieces->fadeStep *= -1;
        }
    } else if (pieces->fadeStart > pieces->fadeEnd) {
        if (pieces->fadeStep > 0) {
            pieces->fadeStep *= -1;
        }
    } else {
        pieces->fadeStep = 0;
    }
    pieces->fadeValue = pieces->fadeStart;
    pieces->fadeCounter = pieces->fadeWait;
    ShinkaDemoPieces_ApplyFade(pieces, heapId);
}

static BOOL ShinkaDemoPieces_IsFadeDone(SpritePieces *pieces, HeapID heapId) {
    return pieces->fadeStep == 0;
}

static void ShinkaDemoPieces_UpdateFade(SpritePieces *pieces, HeapID heapId) {
    if (pieces->fadeStep != 0) {
        if (pieces->fadeCounter > 0) {
            pieces->fadeCounter--;
        } else {
            pieces->fadeValue += pieces->fadeStep;
            pieces->fadeValue = pieces->fadeValue > 31 ? 31 : (pieces->fadeValue < 0 ? 0 : pieces->fadeValue);
            if (pieces->fadeValue == pieces->fadeEnd) {
                pieces->fadeStep = 0;
            } else {
                pieces->fadeCounter = pieces->fadeWait;
            }
            ShinkaDemoPieces_ApplyFade(pieces, heapId);
        }
    }
}

// Blends the sprite's palette toward fadeColor by fadeValue out of 31, except its transparent color
static void ShinkaDemoPieces_ApplyFade(SpritePieces *pieces, HeapID heapId) {
    GXRgb *src = pieces->palette->rawData;
    s16 fadeR = pieces->fadeColor & 0x1f;
    s16 fadeG = (pieces->fadeColor & 0x3e0) >> 5;
    s16 fadeB = (pieces->fadeColor & 0x7c00) >> 10;
    u8 i;
    s16 r;
    s16 g;
    s16 b;

    for (i = 1; i < NELEMS(pieces->colors); i++) {
        r = src[i] & 0x1f;
        g = (src[i] & 0x3e0) >> 5;
        b = (src[i] & 0x7c00) >> 10;
        if (pieces->fadeValue == 31) {
            r = fadeR;
            g = fadeG;
            b = fadeB;
        } else {
            r = r + (((fadeR - r) * pieces->fadeValue) >> 5);
            g = g + (((fadeG - g) * pieces->fadeValue) >> 5);
            b = b + (((fadeB - b) * pieces->fadeValue) >> 5);
            r = r > 31 ? 31 : (r < 0 ? 0 : r);
            g = g > 31 ? 31 : (g < 0 ? 0 : g);
            b = b > 31 ? 31 : (b < 0 ? 0 : b);
        }
        pieces->colors[i] = GX_RGB(r, g, b);
    }
    NNS_GfdRegisterNewVramTransferTask(1, pieces->plttAddr, pieces->colors, sizeof(pieces->colors));
}

static BOOL ShinkaDemoPieces_IsIdle(SpritePieces *pieces, HeapID heapId) {
    return pieces->state == PIECES_IDLE;
}

static BOOL ShinkaDemoPieces_IsGathered(SpritePieces *pieces, HeapID heapId) {
    return pieces->state == PIECES_GATHERED;
}

static void ShinkaDemoPieces_Whiten(SpritePieces *pieces, HeapID heapId) {
    pieces->state = PIECES_WHITENING;
    ShinkaDemoPieces_SetFade(pieces, heapId, GX_RGB(31, 31, 31), 0, 31, 0, 1);
}

static BOOL ShinkaDemoPieces_IsWhite(SpritePieces *pieces, HeapID heapId) {
    return pieces->state == PIECES_WHITE;
}

static BOOL ShinkaDemoPieces_IsReturned(SpritePieces *pieces, HeapID heapId) {
    return pieces->state == PIECES_RETURNED;
}

static void ShinkaDemoPieces_Update(SpritePieces *pieces, HeapID heapId) {
    switch (pieces->state) {
    case PIECES_IDLE:
    case PIECES_GATHERED:
        break;
    case PIECES_WHITENING:
        if (ShinkaDemoPieces_IsFadeDone(pieces, heapId)) {
            pieces->state = PIECES_WHITE;
        }
        break;
    case PIECES_WHITE:
    case PIECES_RETURNED:
        break;
    case PIECES_UNWHITENING:
        if (ShinkaDemoPieces_IsFadeDone(pieces, heapId)) {
            pieces->state = PIECES_IDLE;
        }
        break;
    }
    ShinkaDemoPieces_UpdateFade(pieces, heapId);
}

static SpriteRest *ShinkaDemoRest_Create(u32 texAddr, u32 plttAddr, HeapID heapId) {
    SpriteRest *rest = GFL_HeapAllocate(heapId, sizeof(SpriteRest), TRUE, "shinka_demo_view.c", 3377);

    rest->texAddr = texAddr;
    rest->plttAddr = plttAddr;
    rest->pos.z = 0;
    rest->pos.y = 0;
    rest->pos.x = 0;
    rest->scale.x = FX32_CONST(SPRITE_WIDTH);
    rest->scale.y = FX32_CONST(SPRITE_WIDTH);
    rest->scale.z = FX32_ONE;
    rest->polygonId = 59;
    rest->s0 = FX32_CONST(16);
    rest->t0 = FX32_CONST(16);
    rest->s1 = FX32_CONST(112);
    rest->t1 = FX32_CONST(112);
    rest->alpha = 31;
    rest->vertices[0].x = -FX16_ONE / 2;
    rest->vertices[0].y = FX16_ONE / 2;
    rest->vertices[0].z = 0;
    rest->vertices[1].x = -FX16_ONE / 2;
    rest->vertices[1].y = -FX16_ONE / 2;
    rest->vertices[1].z = 0;
    rest->vertices[2].x = FX16_ONE / 2;
    rest->vertices[2].y = -FX16_ONE / 2;
    rest->vertices[2].z = 0;
    rest->vertices[3].x = FX16_ONE / 2;
    rest->vertices[3].y = FX16_ONE / 2;
    rest->vertices[3].z = 0;
    rest->rowCount = 0;
    return rest;
}

static void ShinkaDemoRest_Free(SpriteRest *rest, HeapID heapId) {
    GFL_HeapFree(rest);
}

// Moves the top of the quad down to the first row that has not broken away
static void ShinkaDemoRest_Update(SpriteRest *rest, HeapID heapId) {
    rest->vertices[3].y = sRestTops[rest->rowCount];
    rest->vertices[0].y = rest->vertices[3].y;
    rest->t0 = FX32_CONST(sRestTexTops[rest->rowCount]);
}

static void ShinkaDemoRest_Draw(SpriteRest *rest, HeapID heapId) {
    int cull;

    if (rest->rowCount < PIECE_ROWS) {
        G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S128, GX_TEXSIZE_T128, GX_TEXREPEAT_ST,
                         GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, rest->texAddr);
        G3_TexPlttBase(rest->plttAddr, GX_TEXFMT_PLTT16);
        G3_PushMtx();
        cull = GX_CULL_ALL;
        if (rest->alpha != 0) {
            cull = GX_CULL_BACK;
        }
        G3_PolygonAttr(0, GX_POLYGONMODE_MODULATE, cull, rest->polygonId, rest->alpha, 0);
        G3_Translate(rest->pos.x, rest->pos.y, rest->pos.z);
        G3_Scale(rest->scale.x, rest->scale.y, rest->scale.z);
        G3_Begin(GX_BEGIN_QUADS);
        G3_Normal(0, 0, -FX16_ONE);
        G3_TexCoord(rest->s0, rest->t0);
        G3_Vtx(rest->vertices[0].x, rest->vertices[0].y, rest->vertices[0].z);
        G3_TexCoord(rest->s0, rest->t1);
        G3_Vtx(rest->vertices[1].x, rest->vertices[1].y, rest->vertices[1].z);
        G3_TexCoord(rest->s1, rest->t1);
        G3_Vtx(rest->vertices[2].x, rest->vertices[2].y, rest->vertices[2].z);
        G3_TexCoord(rest->s1, rest->t0);
        G3_Vtx(rest->vertices[3].x, rest->vertices[3].y, rest->vertices[3].z);
        G3_End();
        G3_PopMtx(1);
    }
}

static void ShinkaDemoRest_SetPosition(SpriteRest *rest, HeapID heapId, fx32 x, fx32 y, fx32 z) {
    rest->pos.x = x;
    rest->pos.y = y;
    rest->pos.z = z;
}
