#include "types.h"
#include "app/musical/mus_poke_draw.h"
#include "app/musical/musical_mcss.h"
#include "constants/arc.h"
#include "field/musical.h"
#include "gfl/heap.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "system/mcss.h"

// Overlay 209's mus_poke_draw.c: the musical's Pokémon, each a sprite of musical_mcss.c with its front and maybe its
// back, and the places of its props, which the sprite's marker cells give as it is drawn

// The marker cells: two of kinds 4 and 5, and one for each prop position from kind 7
#define MUS_POKE_DRAW_CELL_MARK4 4
#define MUS_POKE_DRAW_CELL_MARK5 5
#define MUS_POKE_DRAW_CELL_EQUIP 7

static void MusPokeDraw_CellCallback(u32 kind, MusicalMcssCellInfo *info, void *work);
static void MusPokeDraw_GetLoadInfo(MusicalPoke *musPoke, MCSSLoadInfo *info, BOOL back);

MusPokeDrawSys *MusPokeDraw_InitSystem(HeapID heapId) {
    int i;
    MusPokeDrawSys *sys = GFL_HeapAllocate(heapId, sizeof(MusPokeDrawSys), FALSE, "mus_poke_draw.c", 77);

    sys->heapId = heapId;
    sys->mcssSys = MusicalMcss_InitSystem(8, heapId);
    for (i = 0; i < 8; i++) {
        sys->pokes[i].active = FALSE;
    }
    MusicalMcss_SetOrthoMode(sys->mcssSys);
    return sys;
}

void MusPokeDraw_TermSystem(MusPokeDrawSys *sys) {
    int i;

    for (i = 0; i < 8; i++) {
        if (sys->pokes[i].active == TRUE) {
            MusPokeDraw_DelPoke(sys, &sys->pokes[i]);
        }
    }
    MusicalMcss_TermSystem(sys->mcssSys);
    GFL_HeapFree(sys);
}

void MusPokeDraw_UpdateSystem(MusPokeDrawSys *sys) {
    MusicalMcss_UpdateSystem(sys->mcssSys);
}

void MusPokeDraw_DrawSystem(MusPokeDrawSys *sys) {
    MusicalMcss_DrawSystem(sys->mcssSys, MusPokeDraw_CellCallback);
}

MusPokeDraw *MusPokeDraw_AddPoke(MusPokeDrawSys *sys, MusicalPoke *musPoke, BOOL withBack) {
    int i;
    int j;
    MusPokeDraw *poke;
    VecFx32 scale;
    MCSSLoadInfo info;

    for (i = 0; i < 8; i++) {
        if (sys->pokes[i].active == FALSE) {
            break;
        }
    }
    sys->pokes[i].active = TRUE;
    poke = &sys->pokes[i];
    MusPokeDraw_GetLoadInfo(musPoke, &info, FALSE);
    sys->pokes[i].front = MusicalMcss_Add(sys->mcssSys, 0, 0, 0, &info, poke, FALSE);
    if (withBack == TRUE) {
        MusPokeDraw_GetLoadInfo(musPoke, &info, TRUE);
        sys->pokes[i].back = MusicalMcss_Add(sys->mcssSys, 0, 0, 0, &info, poke, FALSE);
        MusicalMcss_Hide(sys->pokes[i].back);
    } else {
        sys->pokes[i].back = NULL;
    }
    sys->pokes[i].isFront = TRUE;
    sys->pokes[i].mcss = sys->pokes[i].front;
    scale.x = FX32_CONST(16);
    scale.y = FX32_CONST(16);
    scale.z = FX32_ONE;
    for (j = 0; j < 9; j++) {
        sys->pokes[i].equips[j].valid = FALSE;
    }
    sys->pokes[i].markValid4 = FALSE;
    sys->pokes[i].markValid5 = FALSE;
    sys->pokes[i].markPos4.x = 0;
    sys->pokes[i].markPos4.y = 0;
    sys->pokes[i].markPos4.z = 0;
    sys->pokes[i].markPos5.x = 0;
    sys->pokes[i].markPos5.y = 0;
    sys->pokes[i].markPos5.z = 0;
    MusPokeDraw_SetScale(&sys->pokes[i], &scale);
    MusicalMcss_StopAnime(sys->pokes[i].mcss);
    return &sys->pokes[i];
}

void MusPokeDraw_DelPoke(MusPokeDrawSys *sys, MusPokeDraw *poke) {
    if (poke->back != NULL) {
        MusicalMcss_Del(sys->mcssSys, poke->back);
    }
    MusicalMcss_Del(sys->mcssSys, poke->front);
    poke->active = FALSE;
}

void MusPokeDraw_SetPosition(MusPokeDraw *poke, VecFx32 *pos) {
    MusicalMcss_SetPosition(poke->mcss, pos);
}

void MusPokeDraw_SetScale(MusPokeDraw *poke, VecFx32 *scale) {
    MusicalMcss_SetScale(poke->mcss, scale);
}

void MusPokeDraw_SetRotation(MusPokeDraw *poke, u16 rotation) {
    MusicalMcss_SetRotation(poke->mcss, rotation);
}

void MusPokeDraw_SetVisible(MusPokeDraw *poke, BOOL visible) {
    if (visible == TRUE) {
        MusicalMcss_Show(poke->mcss);
    } else {
        MusicalMcss_Hide(poke->mcss);
    }
}

BOOL MusPokeDraw_IsVisible(MusPokeDraw *poke) {
    if (MusicalMcss_IsHidden(poke->mcss) == TRUE) {
        return FALSE;
    }
    return TRUE;
}

void MusPokeDraw_StartAnime(MusPokeDraw *poke) {
    MusicalMcss_StartAnime(poke->front);
    if (poke->back != NULL) {
        MusicalMcss_StartAnime(poke->back);
    }
}

void MusPokeDraw_StopAnime(MusPokeDraw *poke) {
    MusicalMcss_StopAnime(poke->front);
    if (poke->back != NULL) {
        MusicalMcss_StopAnime(poke->back);
    }
}

void MusPokeDraw_ChangeAnime(MusPokeDraw *poke, u16 anime) {
    MusicalMcss_ChangeAnime(poke->mcss, anime);
}

void MusPokeDraw_TurnAround(MusPokeDraw *poke) {
    if (poke->isFront == TRUE) {
        MusicalMcss_CopyState(poke->front, poke->back);
        poke->mcss = poke->back;
        poke->isFront = FALSE;
        MusicalMcss_Hide(poke->front);
    } else {
        MusicalMcss_CopyState(poke->back, poke->front);
        poke->mcss = poke->front;
        poke->isFront = TRUE;
        MusicalMcss_Hide(poke->back);
    }
}

void MusPokeDraw_SetFront(MusPokeDraw *poke, BOOL front) {
    if (front != poke->isFront) {
        MusPokeDraw_TurnAround(poke);
    }
}

void MusPokeDraw_SetFlip(MusPokeDraw *poke, BOOL flip) {
    if (flip == TRUE) {
        MusicalMcss_SetFlip(poke->mcss);
    } else {
        MusicalMcss_ResetFlip(poke->mcss);
    }
}

void MusPokeDraw_SetTexBase(MusPokeDrawSys *sys, u32 base) {
    MusicalMcss_SetTexBase(sys->mcssSys, base);
}

void MusPokeDraw_SetPlttBase(MusPokeDrawSys *sys, u32 base) {
    MusicalMcss_SetPlttBase(sys->mcssSys, base);
}

MusPokeDrawEquipPos *MusPokeDraw_GetEquipPos(MusPokeDraw *poke, u32 pos) {
    return &poke->equips[pos];
}

VecFx32 *MusPokeDraw_GetMarkPos4(MusPokeDraw *poke) {
    return &poke->markPos4;
}

VecFx32 *MusPokeDraw_GetMarkPos5(MusPokeDraw *poke) {
    return &poke->markPos5;
}

static void MusPokeDraw_CellCallback(u32 kind, MusicalMcssCellInfo *info, void *work) {
    MusPokeDraw *poke = work;

    if (kind >= MUS_POKE_DRAW_CELL_EQUIP) {
        kind -= MUS_POKE_DRAW_CELL_EQUIP;
        poke->equips[kind].valid = TRUE;
        poke->equips[kind].info.pos = info->pos;
        poke->equips[kind].info.offset = info->offset;
        poke->equips[kind].info.rotation = info->rotation;
        poke->equips[kind].info.cellRotation = info->cellRotation;
        poke->equips[kind].info.scale = info->scale;
        poke->equips[kind].info.center = info->center;
    } else if (kind == MUS_POKE_DRAW_CELL_MARK4) {
        fx32 x = info->offset.x;
        fx32 y = info->offset.y;

        poke->markPos4.x = x;
        poke->markPos4.y = y;
        poke->markPos4.z = 0;
        poke->markValid4 = TRUE;
    } else if (kind == MUS_POKE_DRAW_CELL_MARK5) {
        fx32 x = info->offset.x;
        fx32 y = info->offset.y;

        poke->markPos5.x = x;
        poke->markPos5.y = y;
        poke->markPos5.z = 0;
        poke->markValid5 = TRUE;
    }
}

static void MusPokeDraw_GetLoadInfo(MusicalPoke *musPoke, MCSSLoadInfo *info, BOOL back) {
    u32 species;
    u32 sex;
    u32 form;
    u8 rare;
    u32 dir;

    func_0201d624(musPoke->pkm);
    species = musPoke->species;
    form = PML_PkmSanitizeForme(species, musPoke->form);
    sex = musPoke->sex;
    rare = musPoke->rare;
    dir = POKEGRA_DIR_FRONT;
    form = PML_PkmGetSpriteForme(musPoke->species, form);
    if (back == TRUE) {
        dir = POKEGRA_DIR_BACK;
    }
    info->arcId = ARCID_MUSICAL_POKEGRA;
    info->character = MusicalMcss_GetCharacterDataNo(info->arcId, species, form, sex, rare, dir, FALSE);
    info->palette = MusicalMcss_GetPaletteDataNo(info->arcId, species, form, sex, rare, dir, FALSE);
    info->cells = MusicalMcss_GetCellsDataNo(info->arcId, species, form, sex, rare, dir, FALSE);
    info->cellAnime = MusicalMcss_GetCellAnimeDataNo(info->arcId, species, form, sex, rare, dir, FALSE);
    info->multiCells = MusicalMcss_GetMultiCellsDataNo(info->arcId, species, form, sex, rare, dir, FALSE);
    info->multiCellAnime = MusicalMcss_GetMultiCellAnimeDataNo(info->arcId, species, form, sex, rare, dir, FALSE);
    info->bin = MusicalMcss_GetBinFileDataNo(info->arcId, species, form, sex, rare, dir, FALSE);
    info->unk20 = musPoke->personality;
}
