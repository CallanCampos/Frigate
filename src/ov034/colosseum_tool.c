// The colosseum's checks of who is here and ready, and the spots that its players stand on. The file's name is a
// guess: the ROM only shows that this is a file of its own, after union_eff.c, by the order of its tables in
// `.rodata`, which one file with union_eff.c's cannot give. Names are not swan's
#include "field/colosseum_tool.h"
#include "types.h"
#include "field/colosseum.h"
#include "field/comm_player.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "nitro/fx.h"
#include "save/player_info.h"

static u16 UnionEff_GetMemberLook(PlayerInfo *info);

BOOL UnionEff_AllColosseumMembersHere(UnionColosseum *colosseum) {
    int count = 0;
    int i;

    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        if (colosseum->members[i].unk26 == 1) {
            count++;
        }
    }
    if (count >= func_02042a78()) {
        return TRUE;
    }
    return FALSE;
}

BOOL UnionEff_AllColosseumCardsIn(UnionColosseum *colosseum) {
    int count = 0;
    int i;

    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        if (colosseum->cardReady[i] == 1) {
            count++;
        }
    }
    if (count >= func_02042a78()) {
        return TRUE;
    }
    return FALSE;
}

BOOL UnionEff_IsColosseumUnk170Set(UnionColosseum *colosseum) {
    if (colosseum->unk170 == 1) {
        return TRUE;
    }
    return FALSE;
}

void UnionEff_ClearColosseumUnk170(UnionColosseum *colosseum) {
    colosseum->unk170 = 0;
}

BOOL UnionEff_GetEntrySpot(Field *field, u32 kind, u32 *index) {
    static const s16 spotsTwo[2][2] = { { 3, 5 }, { 9, 5 } };
    static const s16 spotsFour[4][2] = { { 3, 4 }, { 9, 6 }, { 3, 6 }, { 9, 4 } };
    FieldPlayer *player = Field_GetPlayer(field);
    FieldActor *actor;
    s16 x;
    s16 z;
    u32 i;

    *index = 0;
    actor = FieldPlayer_GetActor(player);
    x = GetGPosX(actor);
    z = GetGPosZ(actor);
    if (kind == 2) {
        for (i = 0; i < 2; i++) {
            if (x == spotsTwo[i][0] && z == spotsTwo[i][1]) {
                *index = i;
                return TRUE;
            }
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (x == spotsFour[i][0] && z == spotsFour[i][1]) {
                *index = i;
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL UnionEff_AllColosseumReady(UnionColosseum *colosseum) {
    int count = 0;
    int i;
    u8 mask = colosseum->unk17C[2];

    for (i = 0; i < 4; i++) {
        if (colosseum->unk172[i] == 1) {
            count++;
        }
    }
    if (count >= func_02042a78() || count >= MATH_CountPopulation(mask)) {
        return TRUE;
    }
    return FALSE;
}

BOOL UnionEff_AnyColosseumUnk172(UnionColosseum *colosseum) {
    int i;

    for (i = 0; i < 4; i++) {
        if (colosseum->unk172[i] == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

static inline BOOL UnionEff_IsOnSpot(const s16 (*spots)[2], int count, int x, int z) {
    int i;

    for (i = 0; i < count; i++) {
        if (x == spots[i][0] && z == spots[i][1]) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL UnionEff_IsOnSeatSpot(Field *field) {
    static const s16 spotsA[5][2] = { { 0x48, 0xa8 }, { 0x58, 0xa8 }, { 0x68, 0xa8 }, { 0x78, 0xa8 }, { 0x88, 0xa8 } };
    static const s16 spotsB[5][2] = { { 0x48, 0xa8 }, { 0x58, 0xa8 }, { 0x68, 0xa8 }, { 0x78, 0xa8 }, { 0x88, 0xa8 } };
    const s16 (*spots)[2];
    VecFx32 pos;
    int i;
    int count;

    FieldPlayer_GetWPos(Field_GetPlayer(field), &pos);
    pos.x >>= 12;
    pos.z >>= 12;
    if (Field_GetPlayerStateZoneID(field) == 0x96) {
        spots = spotsA;
        count = 5;
    } else {
        spots = spotsB;
        count = 5;
    }
    for (i = 0; i < count; i++) {
        if (pos.x == spots[i][0] && pos.z == spots[i][1]) {
            return TRUE;
        }
    }
    return FALSE;
}

void UnionEff_UpdateColosseumCommPlayers(UnionColosseum *colosseum) {
    ColosseumChoice choice;
    u8 self = func_02042a6c(func_02040440());
    int i;

    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        if (self == i) {
            continue;
        }
        if (func_ov028_02172ae8(colosseum, i, &choice) != 1) {
            continue;
        }
        if (func_ov012_0216165c(colosseum->commPlayer, i) == FALSE) {
            func_ov012_0216148c(colosseum->commPlayer, i, UnionEff_GetMemberLook(&colosseum->members[i].info),
                                (CommPlayerStatus *)&choice);
        } else {
            func_ov012_0216168c(colosseum->commPlayer, i, (CommPlayerStatus *)&choice);
        }
    }
}

static u16 UnionEff_GetMemberLook(PlayerInfo *info) {
    u32 look = func_02008bfc(info);
    u32 gender = getTrainerGender(info);

    if ((u8)(look + 0xea) <= 1) {
        if (gender == GENDER_MALE) {
            return 0xe7;
        }
        return 0xf0;
    }
    if (gender == GENDER_MALE) {
        return 1;
    }
    return 4;
}
