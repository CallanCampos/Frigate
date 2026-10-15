#include "types.h"
#include "app/musical/sta_act_audience.h"
#include "app/musical/sta_act_light.h"
#include "app/musical/sta_act_poke.h"
#include "app/musical/sta_acting.h"
#include "field/musical_program.h"
#include "field/musical_stage_sys.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "nitro/fx.h"

// Overlay 209's sta_act_audience.c: the audience, a 12 by 5 grid of spectators in BGs 4 and 6 of the touch screen.
// Each is a fan of one Pokémon, shared out by the points of the program, and has four looks: right, a little right,
// a little left and left

enum {
    STA_ACT_AUDIENCE_LOOK_LEFT,
    STA_ACT_AUDIENCE_LOOK_LEFT_A_LITTLE,
    STA_ACT_AUDIENCE_LOOK_RIGHT_A_LITTLE,
    STA_ACT_AUDIENCE_LOOK_RIGHT,
};

static void StaActAudience_InitGraphic(StaActAudience *sys);
static void StaActAudience_TermGraphic(StaActAudience *sys);
static void StaActAudience_InitMembers(StaActAudience *sys, MusicalStageParam *param);
static void StaActAudience_TermMembers(StaActAudience *sys);
static void StaActAudience_UpdateMembers(StaActAudience *sys);
static void StaActAudience_UpdateMember(StaActAudience *sys, StaActAudienceMember *member);
static void StaActAudience_DrawMember(StaActAudience *sys, StaActAudienceMember *member, u8 look);
static void StaActAudience_SetCheerTargets(StaActAudience *sys);
static void StaActAudience_ClearTargets(StaActAudience *sys);

// The last column of a spectator's four, which StaActAudience_DrawMember counts back from to mirror one. The
// read is folded, so no code refers to the table, but only a referenced static lies in the section of
// STA_ACT_AUDIENCE_CHARS, as the original's does
static const u16 STA_ACT_AUDIENCE_LAST_COLUMN[] = { 3 };

// The first character of each type's spectator
static const u16 STA_ACT_AUDIENCE_CHARS[] = { 0x10, 0x90, 0x08, 0x88 };

StaActAudience *StaActAudience_InitSystem(HeapID heapId, StaActing *stage, MusicalStageParam *param) {
    u8 i;
    StaActAudience *sys = GFL_HeapAllocate(heapId, sizeof(StaActAudience), FALSE, "sta_act_audience.c", 115);

    sys->heapId = heapId;
    StaActAudience_InitGraphic(sys);
    StaActAudience_InitMembers(sys, param);
    sys->stage = stage;
    sys->refresh = FALSE;
    for (i = 0; i < 4; i++) {
        sys->cheer[i] = FALSE;
    }
    sys->lookTarget = STA_ACT_AUDIENCE_NONE;
    return sys;
}

void StaActAudience_TermSystem(StaActAudience *sys) {
    StaActAudience_TermMembers(sys);
    StaActAudience_TermGraphic(sys);
    GFL_HeapFree(sys);
}

void StaActAudience_UpdateSystem(StaActAudience *sys) {
    if (StaActing_GetCurtainOffset(sys->stage) == 0) {
        if (GFL_RandomLCAlt(10) == 0) {
            u8 i;

            for (i = 0; i < 3; i++) {
                u8 look = GFL_RandomLCAlt(4);
                u8 index = GFL_RandomLCAlt(STA_ACT_AUDIENCE_COUNT);

                StaActAudience_DrawMember(sys, &sys->members[index], look);
            }
        }
    } else {
        if (sys->refresh == TRUE) {
            StaActAudience_SetCheerTargets(sys);
            sys->refresh = FALSE;
        }
        StaActAudience_UpdateMembers(sys);
    }
}

static void StaActAudience_InitGraphic(StaActAudience *sys) {
}

static void StaActAudience_TermGraphic(StaActAudience *sys) {
}

static void StaActAudience_InitMembers(StaActAudience *sys, MusicalStageParam *param) {
    u8 y;
    u8 i;
    u8 j;
    u8 n;
    u8 sum;
    u16 total = 0;
    u8 points[4];
    u8 shares[4] = { 0, 0, 0, 0 };
    u8 types[STA_ACT_AUDIENCE_COUNT];

    sum = 0;
    for (i = 0; i < 4; i++) {
        points[i] = func_ov012_0215250c(param->program, i);
        total += points[i];
    }
    for (i = 0; i < 4; i++) {
        shares[i] = points[i] * STA_ACT_AUDIENCE_COUNT / total;
        sum += shares[i];
    }
    for (; sum < STA_ACT_AUDIENCE_COUNT; sum++) {
        shares[GFL_RandomLCAlt(4)]++;
    }
    n = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < shares[i]; j++) {
            types[n] = i;
            n++;
        }
    }
    for (j = 0; j < 3; j++) {
        for (i = 0; i < STA_ACT_AUDIENCE_COUNT; i++) {
            u8 k = GFL_RandomLCAlt(STA_ACT_AUDIENCE_COUNT);
            u8 type = types[i];

            types[i] = types[k];
            types[k] = type;
        }
    }
    for (y = 0; y < 5; y++) {
        for (i = 0; i < 12; i++) {
            u8 index = i + y * 12;

            sys->members[index].x = i + 2;
            sys->members[index].y = y;
            sys->members[index].type = types[index];
            sys->members[index].timer = GFL_RandomLCAlt(60) + 1;
            sys->members[index].target = STA_ACT_AUDIENCE_NONE;
            if (i < 2) {
                StaActAudience_DrawMember(sys, &sys->members[index], STA_ACT_AUDIENCE_LOOK_RIGHT);
            } else if (i < 4) {
                StaActAudience_DrawMember(sys, &sys->members[index], STA_ACT_AUDIENCE_LOOK_RIGHT_A_LITTLE);
            } else if (i < 6) {
                StaActAudience_DrawMember(sys, &sys->members[index], STA_ACT_AUDIENCE_LOOK_LEFT_A_LITTLE);
            } else {
                StaActAudience_DrawMember(sys, &sys->members[index], STA_ACT_AUDIENCE_LOOK_LEFT);
            }
        }
    }
}

static void StaActAudience_TermMembers(StaActAudience *sys) {
}

static void StaActAudience_UpdateMembers(StaActAudience *sys) {
    u8 i;

    for (i = 0; i < STA_ACT_AUDIENCE_COUNT; i++) {
        StaActAudience_UpdateMember(sys, &sys->members[i]);
    }
}

static void StaActAudience_UpdateMember(StaActAudience *sys, StaActAudienceMember *member) {
    if (member->timer != 0) {
        member->timer--;
    }
    if (member->timer == 0) {
        if (member->target == STA_ACT_AUDIENCE_NONE && sys->lookTarget == STA_ACT_AUDIENCE_NONE) {
            member->unk4 = 0xffff;
            if (member->x < 2) {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_RIGHT);
            } else if (member->x < 4) {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_RIGHT_A_LITTLE);
            } else if (member->x < 6) {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_LEFT_A_LITTLE);
            } else {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_LEFT);
            }
        } else {
            u16 memberX;
            s16 dx;
            VecFx32 pos;

            StaActing_GetScrollOffset(sys->stage);
            memberX = (member->x * 32 - 64) * 4 / 3;
            if (member->target != STA_ACT_AUDIENCE_NONE) {
                StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);

                StaActPoke_GetPosition(pokeSys, StaActing_GetPoke(sys->stage, member->target), &pos);
            } else {
                StaActLightSys *lightSys = StaActing_GetLightSys(sys->stage);

                StaActLight_GetPosition(lightSys, StaActing_GetLight(sys->stage, sys->lookTarget), &pos);
            }
            dx = FX_FX32_TO_F32(pos.x) - memberX;
            if (dx < -64) {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_LEFT);
            } else if (dx > 64) {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_RIGHT);
            } else if (dx < 0) {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_LEFT_A_LITTLE);
            } else {
                StaActAudience_DrawMember(sys, member, STA_ACT_AUDIENCE_LOOK_RIGHT_A_LITTLE);
            }
        }
        member->timer = GFL_RandomLCAlt(60) + 1;
    }
}

static void StaActAudience_DrawMember(StaActAudience *sys, StaActAudienceMember *member, u8 look) {
    u16 base;
    BOOL flip = FALSE;
    u8 x;
    u8 y;
    u8 col;

    switch (look) {
    case STA_ACT_AUDIENCE_LOOK_RIGHT:
        flip = TRUE;
        base = STA_ACT_AUDIENCE_CHARS[member->type] + (flip << 10);
        break;
    case STA_ACT_AUDIENCE_LOOK_RIGHT_A_LITTLE:
        flip = TRUE;
        base = STA_ACT_AUDIENCE_CHARS[member->type] + 0x404;
        break;
    case STA_ACT_AUDIENCE_LOOK_LEFT_A_LITTLE:
        base = STA_ACT_AUDIENCE_CHARS[member->type] + 4;
        break;
    case STA_ACT_AUDIENCE_LOOK_LEFT:
        base = STA_ACT_AUDIENCE_CHARS[member->type];
        break;
    }
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            if (flip == TRUE) {
                col = STA_ACT_AUDIENCE_LAST_COLUMN[0] - x + member->x * 4;
            } else {
                col = x + member->x * 4;
            }
            GFL_BGSysFillScrArea(4, base + x + y * 32, col, y + member->y * 4 + 2, 1, 1, 0);
            GFL_BGSysFillScrArea(6, base + x + y * 32, col, y + member->y * 4 + 5, 1, 1, 0);
        }
    }
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysQueueScrLoad(6);
}

static void StaActAudience_SetCheerTargets(StaActAudience *sys) {
    u8 count = 0;
    u8 i;
    u8 targets[4] = { 0, 0, 0, 0 };

    for (i = 0; i < 4; i++) {
        if (sys->cheer[i] == TRUE) {
            targets[count] = i;
            count++;
        }
    }
    if (count == 0) {
        StaActAudience_ClearTargets(sys);
        return;
    }
    for (i = 0; i < STA_ACT_AUDIENCE_COUNT; i++) {
        sys->members[i].timer = GFL_RandomLCAlt(60) + 1;
        sys->members[i].target = targets[GFL_RandomLCAlt(count)];
    }
}

static void StaActAudience_ClearTargets(StaActAudience *sys) {
    u8 i;

    for (i = 0; i < STA_ACT_AUDIENCE_COUNT; i++) {
        sys->members[i].timer = GFL_RandomLCAlt(60) + 1;
        sys->members[i].target = STA_ACT_AUDIENCE_NONE;
    }
}

void StaActAudience_SetCheerPoke(StaActAudience *sys, u8 pos, BOOL cheer) {
    sys->cheer[pos] = cheer;
    sys->refresh = TRUE;
}

void StaActAudience_SetLookLight(StaActAudience *sys, u8 light) {
    sys->lookTarget = light;
    sys->refresh = TRUE;
}

void StaActAudience_SetScrollOffset(StaActAudience *sys, int scroll) {
    u16 bgX = scroll * 128 / 256 + 64;

    GFL_BGSysMoveBG(4, BG_MOVE_SET_X, bgX);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_X, bgX);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_X, bgX);
}
