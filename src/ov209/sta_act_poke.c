#include "types.h"
#include "app/musical/mus_item_draw.h"
#include "app/musical/mus_poke_draw.h"
#include "app/musical/sta_act_effect.h"
#include "app/musical/sta_act_poke.h"
#include "app/musical/sta_acting.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "field/musical.h"
#include "gfl/blact.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "pml/personal.h"

// Overlay 209's sta_act_poke.c: the Pokémon on the stage. Positions are in fixed point pixels, y down, which the
// sprites and the props get scaled down by 16 and y up

// The personal data's flag that keeps a Pokémon from turning around (not from swan)
#define PERSONAL_NO_FLIP 34

// The kinds of a prop's effect, as overlay 210's table gives them
enum {
    STA_ACT_POKE_ITEM_SPIN,
    STA_ACT_POKE_ITEM_PARTICLE,
    STA_ACT_POKE_ITEM_FALL,
    STA_ACT_POKE_ITEM_THROW,
    STA_ACT_POKE_ITEM_PULSE,
};

// How many frames a prop's effect lasts
#define STA_ACT_POKE_ITEM_FRAMES 180

static void StaActPoke_UpdatePoke(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_UpdateItems(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_DrawPoke(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_SetScaleBySpecies(StaActPokeSys *sys, StaActPoke *poke, u16 species);
static void StaActPoke_ItemSpinInit(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemSpin(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemParticleInit(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemParticle(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemFallInit(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemFall(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemThrowInit(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemThrow(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemPulseInit(StaActPokeSys *sys, StaActPoke *poke);
static void StaActPoke_ItemPulse(StaActPokeSys *sys, StaActPoke *poke);

static const u32 STA_ACT_POKE_ITEM_SE[] = {
    SEQ_SE_MSCL_16, SEQ_SE_MSCL_17, SEQ_SE_MSCL_18, SEQ_SE_W319_02, SEQ_SE_MSCL_19,
};

StaActPokeSys *StaActPoke_InitSystem(HeapID heapId, StaActing *stage, MusPokeDrawSys *pokeDraw,
                                     MusItemDrawSys *itemDraw, BlActScene *blact) {
    u8 i;
    StaActPokeSys *sys = GFL_HeapAllocate(heapId, sizeof(StaActPokeSys), FALSE, "sta_act_poke.c", 147);

    sys->heapId = heapId;
    sys->stage = stage;
    sys->pokeDraw = pokeDraw;
    sys->itemDraw = itemDraw;
    sys->scroll = 0;
    sys->blact = blact;
    sys->effectCount = 0;
    for (i = 0; i < 4; i++) {
        sys->pokes[i].active = FALSE;
        sys->pokes[i].update = FALSE;
    }
    sys->shadowMaterial = func_0204e614(sys->blact, ARCID_MUSICAL, 29, 3, 0x33, 64, 64);
    return sys;
}

void StaActPoke_TermSystem(StaActPokeSys *sys) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sys->pokes[i].active == TRUE) {
            StaActPoke_DeletePoke(sys, &sys->pokes[i]);
        }
    }
    BlActScene_FreeMaterial(sys->blact, sys->shadowMaterial);
    GFL_HeapFree(sys);
}

void StaActPoke_UpdateSystem(StaActPokeSys *sys) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sys->pokes[i].active == TRUE) {
            StaActPoke_UpdatePoke(sys, &sys->pokes[i]);
        }
    }
}

void StaActPoke_UpdateSystem_Item(StaActPokeSys *sys) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sys->pokes[i].active == TRUE) {
            StaActPoke_UpdateItems(sys, &sys->pokes[i]);
            if (sys->stage != NULL) {
                u32 j;

                for (j = 0; j < StaActing_GetUpdateCount(sys->stage); j++) {
                    if (sys->pokes[i].itemFunc != NULL) {
                        sys->pokes[i].itemFunc(sys, &sys->pokes[i]);
                    }
                }
            }
        }
    }
    MusItemDraw_UpdateSystem(sys->itemDraw);
}

static void StaActPoke_UpdatePoke(StaActPokeSys *sys, StaActPoke *poke) {
    VecFx32 *mark4 = MusPokeDraw_GetMarkPos4(poke->draw);
    VecFx32 *mark5 = MusPokeDraw_GetMarkPos5(poke->draw);
    VecFx32 pos;
    VecFx32 scale;
    VecFx32 center;
    MtxFx33 rotation;
    u16 angle;

    VEC_Add(&poke->pos, &poke->offset, &pos);
    angle = poke->rotation;
    center.x = 0;
    center.y = mark5->y;
    MTX_RotZ33_(&rotation, -FX_SinIdx(angle), FX_CosIdx(angle));
    MTX_MultVec33(&center, &rotation, &poke->rotOffset);
    VEC_Subtract(&center, &poke->rotOffset, &poke->rotOffset);
    pos.x = (pos.x - mark4->x - poke->rotOffset.x) / 16;
    pos.y = (FX32_CONST(192) - (poke->rotOffset.y + (pos.y - mark4->y))) / 16;
    MusPokeDraw_SetPosition(poke->draw, &pos);
    MusPokeDraw_SetRotation(poke->draw, poke->rotation);
    scale.x = poke->scale.x * 16;
    scale.y = poke->scale.y * 16;
    scale.z = poke->scale.z * 16;
    if (poke->flip == TRUE) {
        scale.x *= -1;
    }
    MusPokeDraw_SetScale(poke->draw, &scale);
    poke->update = FALSE;
    if (poke->animating == TRUE) {
        poke->blinkTimer += GFL_RandomLC(2);
        if (poke->blinkTimer > 120) {
            MusPokeDraw_SetFlip(poke->draw, TRUE);
        } else {
            MusPokeDraw_SetFlip(poke->draw, FALSE);
        }
        if (poke->blinkTimer > 125) {
            poke->blinkTimer = GFL_RandomLC(120) / 3;
        }
    }
}

static void StaActPoke_UpdateItems(StaActPokeSys *sys, StaActPoke *poke) {
    int i;
    VecFx32 pos;

    for (i = 0; i < 9; i++) {
        u32 equipPos = i;

        // A Pokémon facing the other way wears its left props on the right
        if (poke->flip == TRUE) {
            if (i == 7) {
                equipPos = 8;
            } else if (i == 8) {
                equipPos = 7;
            } else if (i == 0) {
                equipPos = 1;
            } else if (i == 1) {
                equipPos = 0;
            }
        }
        if (poke->items[i] != NULL && poke->showItem[i] == TRUE) {
            if (poke->showItems == TRUE && poke->isFront == TRUE) {
                MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(poke->draw, equipPos);

                if (equip->valid == TRUE) {
                    BOOL flipped = equip->info.scale.x < 0 ? TRUE : FALSE;
                    u16 rot = 0x10000 - equip->info.rotation;
                    u16 itemRot = (0x10000 + equip->info.cellRotation + poke->equips[i]->unk2) % 0x10000;
                    fx32 scaleX;
                    MtxFx33 rotation;
                    VecFx32 center;
                    VecFx32 offset;

                    MusPokeDraw_GetMarkPos4(poke->draw);
                    scaleX = equip->info.scale.x;
                    MTX_RotZ33_(&rotation, -FX_SinIdx(rot), FX_CosIdx(rot));
                    MTX_MultVec33(&equip->info.offset, &rotation, &offset);
                    if (flipped == TRUE) {
                        itemRot = 0x10000 - itemRot;
                    }
                    if (MusItemDraw_GetItemFlag9(poke->items[i]) == FALSE && flipped == TRUE) {
                        scaleX = -scaleX;
                    }
                    MTX_MultVec33(&equip->info.center, &rotation, &center);
                    VEC_Subtract(&equip->info.center, &center, &center);
                    pos.x = equip->info.pos.x + offset.x + FX32_CONST(128) + FX32_CONST(sys->scroll);
                    pos.y = equip->info.pos.y + offset.y + FX32_CONST(96);
                    if (MusItemDraw_GetItemFlag7(poke->items[i]) == TRUE) {
                        pos.z = poke->pos.z - FX32_CONST(20);
                    } else {
                        if (equipPos == 7 || equipPos == 8) {
                            pos.z = poke->pos.z + FX32_CONST(10) - (poke->equips[i]->slot << 11);
                        } else {
                            pos.z = poke->pos.z + FX32_CONST(5) - (poke->equips[i]->slot << 11);
                        }
                    }
                    MusItemDraw_SetPosition(sys->itemDraw, poke->items[i], &pos);
                    MusItemDraw_SetRotation(sys->itemDraw, poke->items[i], itemRot - rot);
                    MusItemDraw_SetSize(sys->itemDraw, poke->items[i], scaleX / 16 / 4, equip->info.scale.y / 16 / 4);
                    MusItemDraw_SetVisible(sys->itemDraw, poke->items[i], TRUE);
                }
            } else {
                MusItemDraw_SetVisible(sys->itemDraw, poke->items[i], FALSE);
            }
        }
    }
    MusPokeDraw_GetMarkPos4(poke->draw);
    pos.x = poke->pos.x / 16;
    pos.y = (FX32_CONST(192) - poke->pos.y) / 16;
    pos.z = 50;
    BlActScene_SetActorPos(sys->blact, poke->shadowActor, &pos);
}

void StaActPoke_DrawSystem(StaActPokeSys *sys) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sys->pokes[i].active == TRUE && StaActPoke_GetShowFlg(sys, &sys->pokes[i]) == TRUE) {
            StaActPoke_DrawPoke(sys, &sys->pokes[i]);
        }
    }
}

static void StaActPoke_DrawPoke(StaActPokeSys *sys, StaActPoke *poke) {
}

void StaActPoke_SetScrollOffset(StaActPokeSys *sys, u16 scroll) {
    u8 i;

    sys->scroll = scroll;
    // BUG: The loop runs over 9 Pokémon, where the system has 4
#ifdef BUGFIX
    for (i = 0; i < 4; i++) {
#else
    for (i = 0; i < 9; i++) {
#endif
        if (sys->pokes[i].active == TRUE) {
            sys->pokes[i].update = TRUE;
        }
    }
}

StaActPoke *StaActPoke_CreatePoke(StaActPokeSys *sys, MusicalPoke *musPoke) {
    u8 i;
    StaActPoke *poke;
    void *personal;
    BOOL show;

    // BUG: The search runs over 9 Pokémon, where the system has 4
#ifdef BUGFIX
    for (i = 0; i < 4; i++) {
#else
    for (i = 0; i < 9; i++) {
#endif
        if (sys->pokes[i].active == FALSE) {
            break;
        }
    }
    poke = &sys->pokes[i];
    sys->pokes[i].active = TRUE;
    poke->update = TRUE;
    poke->isFront = TRUE;
    poke->showItems = FALSE;
    poke->noBounce = FALSE;
    poke->noFlip = FALSE;
    poke->itemFunc = NULL;
    poke->draw = MusPokeDraw_AddPoke(sys->pokeDraw, musPoke, TRUE);
    personal = PML_PersonalLoad(musPoke->species, musPoke->form, sys->heapId);
    if (PML_PersonalGetParam(personal, PERSONAL_NO_BOUNCE) == TRUE) {
        poke->noBounce = TRUE;
    }
    if (PML_PersonalGetParam(personal, PERSONAL_NO_FLIP) == TRUE) {
        poke->noFlip = TRUE;
    }
    PML_PersonalFree(personal);
    poke->pos.x = 0;
    poke->pos.y = 0;
    poke->pos.z = 0;
    poke->offset.x = 0;
    poke->offset.y = 0;
    poke->offset.z = 0;
    poke->scale.x = FX32_ONE;
    poke->scale.y = FX32_ONE;
    poke->scale.z = FX32_ONE;
    StaActPoke_SetScaleBySpecies(sys, poke, musPoke->species);
    poke->rotation = 0;
    for (i = 0; i < 9; i++) {
        u16 itemId = musPoke->equips[i].itemId;

        poke->equips[i] = &musPoke->equips[i];
        if (itemId != 0xff) {
            poke->showItem[i] = TRUE;
            poke->itemRes[i] = MusItemDraw_LoadTexResource(itemId);
            poke->items[i] = MusItemDraw_AddItem(sys->itemDraw, itemId, poke->itemRes[i], &poke->pos);
            MusItemDraw_SetSize(sys->itemDraw, poke->items[i], FX32_ONE / 4, FX32_ONE / 4);
            MusItemDraw_SetVisible(sys->itemDraw, poke->items[i], FALSE);
        } else {
            poke->showItem[i] = FALSE;
            poke->itemRes[i] = NULL;
            poke->items[i] = NULL;
        }
    }
    poke->shadowActor = BlActScene_AddNewActor(sys->blact, sys->shadowMaterial, 0x400, 0x300, &poke->pos, 30, 0, 0);
    show = TRUE;
    BlActScene_SetActorHidden(sys->blact, poke->shadowActor, &show);
    poke->blinkTimer = GFL_RandomLC(150) / 2;
    poke->animating = TRUE;
    return poke;
}

void StaActPoke_DeletePoke(StaActPokeSys *sys, StaActPoke *poke) {
    u8 i;

    BlActScene_ClearActorMaterial(sys->blact, poke->shadowActor);
    for (i = 0; i < 9; i++) {
        if (poke->items[i] != NULL) {
            MusItemDraw_DelItem(sys->itemDraw, poke->items[i]);
            MusItemDraw_FreeTexResource(poke->itemRes[i]);
            poke->itemRes[i] = NULL;
            poke->items[i] = NULL;
        }
    }
    MusPokeDraw_DelPoke(sys->pokeDraw, poke->draw);
    poke->active = FALSE;
}

// The species whose sprites are drawn a little narrower, wider or shorter
static void StaActPoke_SetScaleBySpecies(StaActPokeSys *sys, StaActPoke *poke, u16 species) {
    if (species == SPECIES_DONPHAN) {
        poke->scale.x = 0xfb3;
    } else if (species == SPECIES_GROTLE || species == SPECIES_STOUTLAND) {
        poke->scale.x = 0xfc0;
    } else if (species == SPECIES_GRAVELER || species == SPECIES_FORRETRESS || species == SPECIES_SUICUNE) {
        poke->scale.x = 0xfcc;
    } else if (species == SPECIES_VILEPLUME || species == SPECIES_NUMEL) {
        poke->scale.x = 0x1040;
    } else if (species == SPECIES_BLASTOISE || species == SPECIES_NIDORINO || species == SPECIES_POLIWHIRL ||
               species == SPECIES_ALAKAZAM || species == SPECIES_GOLEM || species == SPECIES_MR_MIME ||
               species == SPECIES_SCYTHER || species == SPECIES_KABUTOPS || species == SPECIES_HERACROSS ||
               species == SPECIES_SMEARGLE || species == SPECIES_GROVYLE || species == SPECIES_COMBUSKEN ||
               species == SPECIES_LINOONE || species == SPECIES_SWELLOW || species == SPECIES_BRELOOM ||
               species == SPECIES_ROSELIA || species == SPECIES_ANORITH || species == SPECIES_STUNKY ||
               species == SPECIES_GABITE || species == SPECIES_MANDIBUZZ) {
        poke->scale.y = 0xfcc;
    } else if (species == SPECIES_VULPIX || species == SPECIES_SNEASEL || species == SPECIES_MAGCARGO ||
               species == SPECIES_GLALIE || species == SPECIES_FRILLISH || species == SPECIES_HAXORUS) {
        poke->scale.y = 0xfd9;
    } else if (species == SPECIES_PIDGEOT || species == SPECIES_PERSIAN || species == SPECIES_MAGMAR ||
               species == SPECIES_TAUROS || species == SPECIES_QUILAVA || species == SPECIES_TYPHLOSION ||
               species == SPECIES_AMPHAROS || species == SPECIES_STEELIX || species == SPECIES_LARVITAR ||
               species == SPECIES_TYRANITAR || species == SPECIES_MEDICHAM || species == SPECIES_BANETTE ||
               species == SPECIES_REGICE || species == SPECIES_PIPLUP || species == SPECIES_STARAPTOR ||
               species == SPECIES_FLOATZEL || species == SPECIES_ABOMASNOW || species == SPECIES_COTTONEE ||
               species == SPECIES_SIGILYPH || species == SPECIES_GOLURK || species == SPECIES_RESHIRAM) {
        poke->scale.y = 0xfe6;
    } else if (species == SPECIES_NINETALES || species == SPECIES_VIGOROTH) {
        poke->scale.y = 0xff3;
    } else if (species == SPECIES_HIPPOPOTAS) {
        poke->scale.x = 0x1040;
        poke->scale.y = 0xfcc;
    } else if (species == SPECIES_GOLETT) {
        poke->scale.x = 0xfc0;
        poke->scale.y = 0xfcc;
    }
}

void StaActPoke_StartItemEffect(StaActPokeSys *sys, StaActPoke *poke, u32 pos) {
    if (poke->items[pos] != NULL && poke->itemFunc == NULL) {
        u8 type = MusItemData_GetEffect(MusItemDraw_GetItemData(sys->itemDraw), poke->equips[pos]->itemId);

        poke->itemWork.timer = 0;
        poke->itemWork.pos = pos;
        poke->itemWork.effect = NULL;
        GFL_SEPlayKeepVol(STA_ACT_POKE_ITEM_SE[type], 2);
        sys->effectCount++;
        switch (type) {
        case STA_ACT_POKE_ITEM_SPIN:
            StaActPoke_ItemSpinInit(sys, poke);
            poke->itemFunc = StaActPoke_ItemSpin;
            break;
        case STA_ACT_POKE_ITEM_PARTICLE:
            StaActPoke_ItemParticleInit(sys, poke);
            poke->itemFunc = StaActPoke_ItemParticle;
            break;
        case STA_ACT_POKE_ITEM_FALL:
            StaActPoke_ItemFallInit(sys, poke);
            poke->itemFunc = StaActPoke_ItemFall;
            break;
        case STA_ACT_POKE_ITEM_THROW:
            StaActPoke_ItemThrowInit(sys, poke);
            poke->itemFunc = StaActPoke_ItemThrow;
            break;
        case STA_ACT_POKE_ITEM_PULSE:
            StaActPoke_ItemPulseInit(sys, poke);
            poke->itemFunc = StaActPoke_ItemPulse;
            break;
        }
    }
}

BOOL StaActPoke_IsItemEffect(StaActPokeSys *sys, StaActPoke *poke) {
    if (poke->itemFunc != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void StaActPoke_ItemSpinInit(StaActPokeSys *sys, StaActPoke *poke) {
}

static void StaActPoke_ItemSpin(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    poke->equips[work->pos]->unk2 += 0x1000;
    work->timer++;
    if (work->timer >= 192) {
        poke->itemFunc = NULL;
        sys->effectCount--;
        if (sys->effectCount == 0) {
            GFL_SndPlayerStop(2);
        }
    }
}

static void StaActPoke_ItemParticleInit(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    VecFx32 pos;
    StaActEffectSys *effectSys = StaActing_GetEffectSys(sys->stage);
    MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(poke->draw, poke->itemWork.pos);

    work->effect = StaActEffect_AddEffect(effectSys, 53);
    if (poke->isFront == TRUE && equip->valid == TRUE) {
        MusItemDraw_GetPosition(sys->itemDraw, poke->items[work->pos], &pos);
        pos.x = pos.x / 16;
        pos.y = (FX32_CONST(192) - pos.y) / 16;
        pos.z = pos.z + FX32_ONE;
    } else {
        VEC_Add(&poke->pos, MusPokeDraw_GetMarkPos5(poke->draw), &pos);
        pos.x = pos.x / 16;
        pos.y = (FX32_CONST(192) - pos.y) / 16;
        pos.z = pos.z - FX32_CONST(28);
    }
    StaActEffect_CreateEmitter(work->effect, 0, &pos);
}

static void StaActPoke_ItemParticle(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    VecFx32 pos;
    MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(poke->draw, poke->itemWork.pos);

    if (poke->isFront == TRUE && equip->valid == TRUE) {
        MusItemDraw_GetPosition(sys->itemDraw, poke->items[work->pos], &pos);
        pos.x = pos.x / 16;
        pos.y = (FX32_CONST(192) - pos.y) / 16;
        pos.z = pos.z + FX32_ONE;
    } else {
        VEC_Add(&poke->pos, MusPokeDraw_GetMarkPos5(poke->draw), &pos);
        pos.x = pos.x / 16;
        pos.y = (FX32_CONST(192) - pos.y) / 16;
        pos.z = pos.z - FX32_CONST(28);
    }
    StaActEffect_SetPosition(work->effect, 0, &pos);
    work->timer++;
    if (work->timer >= STA_ACT_POKE_ITEM_FRAMES) {
        StaActEffectSys *effectSys = StaActing_GetEffectSys(sys->stage);

        StaActEffect_DeleteEmitter(work->effect, 0);
        StaActEffect_DelEffect(effectSys, work->effect);
        poke->itemFunc = NULL;
        sys->effectCount--;
        if (sys->effectCount == 0) {
            GFL_SndPlayerStop(2);
        }
    }
}

static void StaActPoke_ItemFallInit(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(poke->draw, poke->itemWork.pos);

    poke->showItem[poke->itemWork.pos] = FALSE;
    if (poke->isFront == FALSE || equip->valid == FALSE) {
        VecFx32 pos;

        VEC_Add(&poke->pos, MusPokeDraw_GetMarkPos5(poke->draw), &pos);
        pos.z -= FX32_CONST(28);
        MusItemDraw_SetPosition(sys->itemDraw, poke->items[work->pos], &pos);
    }
    MusItemDraw_SetVisible(sys->itemDraw, poke->items[work->pos], TRUE);
    MusItemDraw_SetRotation(sys->itemDraw, poke->items[work->pos], 0);
    MusItemDraw_SetSize(sys->itemDraw, poke->items[work->pos], FX32_ONE / 4, FX32_ONE / 4);
}

static void StaActPoke_ItemFall(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    u16 timer = work->timer;
    fx16 sin = FX_SinIdx((timer * 0x200) % 0x10000);
    VecFx32 pos;

    MusItemDraw_GetPosition(sys->itemDraw, poke->items[work->pos], &pos);
    pos.x += FX_Mul(timer / 3 * 205, sin);
    pos.y -= FX32_CONST(0.75);
    MusItemDraw_SetPosition(sys->itemDraw, poke->items[work->pos], &pos);
    MusItemDraw_SetRotation(sys->itemDraw, poke->items[work->pos], sin >> 1);
    work->timer++;
    if (work->timer >= STA_ACT_POKE_ITEM_FRAMES) {
        MusItemDraw_SetVisible(sys->itemDraw, poke->items[work->pos], FALSE);
        poke->itemFunc = NULL;
        sys->effectCount--;
        if (sys->effectCount == 0) {
            GFL_SndPlayerStop(2);
        }
    }
}

static void StaActPoke_ItemThrowInit(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(poke->draw, poke->itemWork.pos);

    poke->showItem[poke->itemWork.pos] = FALSE;
    if (poke->isFront == FALSE || equip->valid == FALSE) {
        VecFx32 pos;

        VEC_Add(&poke->pos, MusPokeDraw_GetMarkPos5(poke->draw), &pos);
        MusItemDraw_SetPosition(sys->itemDraw, poke->items[work->pos], &pos);
    }
    work->speed = FX32_CONST(-3);
    MusItemDraw_SetVisible(sys->itemDraw, poke->items[work->pos], TRUE);
    MusItemDraw_SetRotation(sys->itemDraw, poke->items[work->pos], 0);
    MusItemDraw_SetSize(sys->itemDraw, poke->items[work->pos], FX32_ONE / 4, FX32_ONE / 4);
}

static void StaActPoke_ItemThrow(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    VecFx32 pos;
    s16 scaleX;
    s16 scaleY;

    MusItemDraw_GetPosition(sys->itemDraw, poke->items[work->pos], &pos);
    work->speed += 0xa4;
    pos.x -= 0xb33;
    pos.y += work->speed;
    pos.z = FX32_CONST(200);
    MusItemDraw_SetPosition(sys->itemDraw, poke->items[work->pos], &pos);
    MusItemDraw_GetSize(sys->itemDraw, poke->items[work->pos], &scaleX, &scaleY);
    scaleX += 12;
    scaleY += 12;
    MusItemDraw_SetSize(sys->itemDraw, poke->items[work->pos], scaleX, scaleY);
    work->timer++;
    if (work->timer >= STA_ACT_POKE_ITEM_FRAMES) {
        MusItemDraw_SetVisible(sys->itemDraw, poke->items[work->pos], FALSE);
        poke->itemFunc = NULL;
        sys->effectCount--;
        if (sys->effectCount == 0) {
            GFL_SndPlayerStop(2);
        }
    }
}

static void StaActPoke_ItemPulseInit(StaActPokeSys *sys, StaActPoke *poke) {
}

static void StaActPoke_ItemPulse(StaActPokeSys *sys, StaActPoke *poke) {
    StaActPokeItemWork *work = &poke->itemWork;
    u16 idx = (work->timer * 0x800) % 0x10000;

    work->timer++;
    if (work->timer >= STA_ACT_POKE_ITEM_FRAMES) {
        poke->itemFunc = NULL;
        sys->effectCount--;
        if (sys->effectCount == 0) {
            GFL_SndPlayerStop(2);
        }
    } else {
        MusItemDraw_SetSize(sys->itemDraw, poke->items[work->pos], FX_SinIdx(idx) / 8 + FX32_ONE / 4,
                            FX_CosIdx(idx) / 8 + FX32_ONE / 4);
    }
}

void StaActPoke_GetPosition(StaActPokeSys *sys, StaActPoke *poke, VecFx32 *pos) {
    pos->x = poke->pos.x;
    pos->y = poke->pos.y;
    pos->z = poke->pos.z;
}

void StaActPoke_SetPosition(StaActPokeSys *sys, StaActPoke *poke, VecFx32 *pos) {
    poke->pos.x = pos->x;
    poke->pos.y = pos->y;
    poke->pos.z = pos->z;
    poke->update = TRUE;
}

void StaActPoke_SetRotate(StaActPokeSys *sys, StaActPoke *poke, u16 rotation) {
    poke->rotation = rotation;
    if (poke->noBounce == TRUE) {
        poke->rotation = 0;
    }
    poke->update = TRUE;
}

void StaActPoke_SetPositionOffset(StaActPokeSys *sys, StaActPoke *poke, VecFx32 *offset) {
    poke->offset.x = offset->x;
    poke->offset.y = offset->y;
    poke->offset.z = offset->z;
    if (poke->noBounce == TRUE) {
        poke->offset.y = 0;
    }
    poke->update = TRUE;
}

void StaActPoke_StartAnime(StaActPokeSys *sys, StaActPoke *poke) {
    MusPokeDraw_StartAnime(poke->draw);
    poke->animating = TRUE;
}

void StaActPoke_StopAnime(StaActPokeSys *sys, StaActPoke *poke) {
    MusPokeDraw_StopAnime(poke->draw);
    poke->animating = FALSE;
    MusPokeDraw_SetFlip(poke->draw, FALSE);
}

void StaActPoke_ChangeAnime(StaActPokeSys *sys, StaActPoke *poke, u8 anime) {
    MusPokeDraw_ChangeAnime(poke->draw, anime);
}

void StaActPoke_SetShowFlg(StaActPokeSys *sys, StaActPoke *poke, const BOOL show) {
    int i;

    MusPokeDraw_SetVisible(poke->draw, show);
    for (i = 0; i < 9; i++) {
        if (poke->items[i] != NULL && poke->showItem[i] == TRUE) {
            MusItemDraw_SetVisible(sys->itemDraw, poke->items[i], show);
        }
    }
    BlActScene_SetActorHidden(sys->blact, poke->shadowActor, &show);
}

BOOL StaActPoke_GetShowFlg(StaActPokeSys *sys, StaActPoke *poke) {
    return MusPokeDraw_IsVisible(poke->draw);
}

void StaActPoke_SetFlip(StaActPokeSys *sys, StaActPoke *poke, BOOL flip) {
    if (poke->noFlip == TRUE) {
        poke->flip = FALSE;
    } else {
        poke->flip = flip;
    }
    poke->update = TRUE;
}

void StaActPoke_SetFront(StaActPokeSys *sys, StaActPoke *poke, BOOL front) {
    poke->isFront = front;
    MusPokeDraw_SetFront(poke->draw, front);
    poke->update = TRUE;
}

void StaActPoke_SetShowItem(StaActPokeSys *sys, StaActPoke *poke, BOOL show) {
    poke->showItems = show;
    poke->update = TRUE;
}
