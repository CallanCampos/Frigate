#ifndef POKEBW2_APP_MUSICAL_STA_ACT_POKE_H
#define POKEBW2_APP_MUSICAL_STA_ACT_POKE_H

// Overlay 209's sta_act_poke.c: the Pokémon on the stage, each a sprite of mus_poke_draw.c with its props of
// mus_item_draw.c and a shadow, and the props' effects when they are used

#include "types.h"
#include "app/musical/mus_item_draw.h"
#include "app/musical/mus_poke_draw.h"
#include "app/musical/sta_act_effect.h"
#include "field/musical.h"
#include "gfl/billboard_actor.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

typedef void (*StaActPokeItemFunc)(StaActPokeSys *sys, StaActPoke *poke);

// The effect of the prop being used
typedef struct {
    u16 timer;
    // The equip position of the prop
    u32 pos;
    StaActEffect *effect;
    fx32 speed;
    u32 unk10;
} StaActPokeItemWork;

struct StaActPoke {
    BOOL active;
    // Whether the sprite needs its position, rotation and scale again
    BOOL update;
    BOOL isFront;
    // Whether its props are shown
    BOOL showItems;
    // The personal data's flags that keep it from bouncing and from turning around
    BOOL noBounce;
    BOOL noFlip;
    VecFx32 pos;
    VecFx32 scale;
    VecFx32 offset;
    // How far rotating moves the sprite
    VecFx32 rotOffset;
    u16 rotation;
    BOOL animating;
    // Frames until the sprite's second frame shows, and while it does
    u16 blinkTimer;
    // Whether it faces the other way
    BOOL flip;
    MusPokeDraw *draw;
    u32 shadowActor;
    G3DResource *itemRes[9];
    MusItemDraw *items[9];
    MusicalPokeEquip *equips[9];
    BOOL showItem[9];
    StaActPokeItemWork itemWork;
    StaActPokeItemFunc itemFunc;
};

struct StaActPokeSys {
    HeapID heapId;
    // NULL in the photo
    StaActing *stage;
    BillboardActorScene *billboardActor;
    MusPokeDrawSys *pokeDraw;
    MusItemDrawSys *itemDraw;
    StaActPoke pokes[4];
    // How far the stage has scrolled, in pixels
    u16 scroll;
    u32 shadowMaterial;
    // How many props' effects are running, which play a sound until the last ends
    u8 effectCount;
};

StaActPokeSys *StaActPoke_InitSystem(HeapID heapId, StaActing *stage, MusPokeDrawSys *pokeDraw,
                                     MusItemDrawSys *itemDraw, BillboardActorScene *billboardActor);
void StaActPoke_TermSystem(StaActPokeSys *sys);
void StaActPoke_UpdateSystem(StaActPokeSys *sys);
// Places the props, after the sprites are drawn and their marker cells known, and runs their effects
void StaActPoke_UpdateSystem_Item(StaActPokeSys *sys);
void StaActPoke_DrawSystem(StaActPokeSys *sys);
void StaActPoke_SetScrollOffset(StaActPokeSys *sys, u16 scroll);
StaActPoke *StaActPoke_CreatePoke(StaActPokeSys *sys, MusicalPoke *musPoke);
void StaActPoke_DeletePoke(StaActPokeSys *sys, StaActPoke *poke);
// Starts the effect of the prop at an equip position
void StaActPoke_StartItemEffect(StaActPokeSys *sys, StaActPoke *poke, u32 pos);
BOOL StaActPoke_IsItemEffect(StaActPokeSys *sys, StaActPoke *poke);
void StaActPoke_GetPosition(StaActPokeSys *sys, StaActPoke *poke, VecFx32 *pos);
void StaActPoke_SetPosition(StaActPokeSys *sys, StaActPoke *poke, VecFx32 *pos);
void StaActPoke_SetRotate(StaActPokeSys *sys, StaActPoke *poke, u16 rotation);
void StaActPoke_SetPositionOffset(StaActPokeSys *sys, StaActPoke *poke, VecFx32 *offset);
void StaActPoke_StartAnime(StaActPokeSys *sys, StaActPoke *poke);
void StaActPoke_StopAnime(StaActPokeSys *sys, StaActPoke *poke);
void StaActPoke_ChangeAnime(StaActPokeSys *sys, StaActPoke *poke, u8 anime);
void StaActPoke_SetShowFlg(StaActPokeSys *sys, StaActPoke *poke, BOOL show);
BOOL StaActPoke_GetShowFlg(StaActPokeSys *sys, StaActPoke *poke);
void StaActPoke_SetFlip(StaActPokeSys *sys, StaActPoke *poke, BOOL flip);
void StaActPoke_SetFront(StaActPokeSys *sys, StaActPoke *poke, BOOL front);
void StaActPoke_SetShowItem(StaActPokeSys *sys, StaActPoke *poke, BOOL show);

#endif // POKEBW2_APP_MUSICAL_STA_ACT_POKE_H
