// The actors of the Union Room's players heard: for every member of a beacon's entry an actor on the field, made when
// the member is new, run by its step (a row of begin, run and end functions) and freed when the entry times out.
// The file's name is a guess: the ROM only shows that this is a file of its own, before union_oneself.c, by the
// assert file names ("" and "0") that its `.data` holds a second time. Names are not swan's
#include "field/union_actor.h"
#include "types.h"
#include "constants/zones.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_internal.h"
#include "field/union_comm.h"
#include "field/union_eff.h"
#include "field/union_msg.h"
#include "field/union_oneself.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/union_view.h"

#define UNION_ACTOR_ID_MEMBER_BASE 0x20

// A step of an actor
typedef struct {
    UnionActorFunc begin;
    UnionActorFunc run;
    UnionActorFunc end;
} UnionActorStep;

typedef struct {
    s16 x;
    s16 z;
} UnionActorOffset;

static BOOL UnionOneself_SetActorNextStep(UnionActorSlot *slot, u8 step);
static FieldActor *UnionOneself_CreateActor(UnionSystem *unisys, GameData *gameData, u16 look, u16 id);
static u16 UnionOneself_GetActorId(UnionBeaconEntry *entry, UnionActorSlot *slot);
static FieldActor *UnionOneself_FindActor(UnionSystem *unisys, UnionBeaconEntry *entry, UnionActorSlot *slot);
static UnionActorSlot *UnionOneself_CreateActorSlot(UnionSystem *unisys, UnionBeaconEntry *entry, UnionMember *member, int memberIndex, BOOL visible, FieldActor **outActor);
static void UnionOneself_FreeActorSlot(UnionSystem *unisys, UnionBeaconEntry *entry, UnionActorSlot *slot);
static void UnionOneself_UpdateEntryActors(UnionSystem *unisys, RivalDataSave *rival, WifiList *wifiList, void *effWork, UnionBeaconEntry *entry);
static BOOL UnionOneself_IsMemberNew(UnionSystem *unisys, UnionBeaconEntry *entry, const u8 *mac);
static BOOL UnionOneself_ShowFocus(UnionSystem *unisys, UnionBeaconEntry *entry);
static BOOL UnionOneself_RunActor(UnionSystem *unisys, UnionActorSlot *slot, Field *field);
static BOOL UnionOneself_UpdateTimeout(UnionSystem *unisys, UnionBeaconEntry *entry);
static BOOL UnionOneself_ActorIdle(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq);
static BOOL UnionOneself_ActorLeave(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq);
static BOOL UnionOneself_ActorTalk(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq);

static const UnionActorOffset sUnionMemberOffsets[UNION_MEMBER_MAX] = {
    { 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, -1 }, { 0, 1 },
};

static const ZoneNPC sUnionActorNpc = {
    0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, { { 0, 0, 0 } },
};

static const UnionActorOffset sUnionEntryPositions[UNION_BEACON_ENTRY_MAX] = {
    { 13, 10 }, { 4, 12 }, { 7, 10 }, { 13, 5 }, { 16, 12 }, { 4, 7 }, { 16, 7 }, { 10, 7 }, { 7, 5 }, { 10, 12 },
};

static const UnionActorStep sUnionActorSteps[4] = {
    { NULL, UnionOneself_ActorIdle, NULL },
    { NULL, UnionActor_StepAppear, NULL },
    { NULL, UnionOneself_ActorLeave, NULL },
    { NULL, UnionOneself_ActorTalk, NULL },
};

void UnionOneself_ClearActors(UnionSystem *unisys) {
    sys_memset(unisys->actors, 0, sizeof(unisys->actors));
}

static BOOL UnionOneself_SetActorNextStep(UnionActorSlot *slot, u8 step) {
    slot->nextStep = step;
    return TRUE;
}

static FieldActor *UnionOneself_CreateActor(UnionSystem *unisys, GameData *gameData, u16 look, u16 id) {
    MMSys *mmsys = GameData_GetMMSys(gameData);
    u16 entryIndex = id >> 8;
    u16 member = (u8)id - UNION_ACTOR_ID_MEMBER_BASE;
    ZoneNPC npc = sUnionActorNpc;

    npc.uid = id;
    npc.modelId = UnionView_GetTrainerType(look);
    if (member != 0) {
        npc.moveCode = 0xf;
    }
    func_ov012_021682c0(&npc, sUnionEntryPositions[entryIndex].x + sUnionMemberOffsets[member].x,
                        sUnionEntryPositions[entryIndex].z + sUnionMemberOffsets[member].z, 0);
    return CreateNewActorByEntityNoWKOBJCODE(mmsys, &npc, ZONE_UNION_ROOM);
}

static u16 UnionOneself_GetActorId(UnionBeaconEntry *entry, UnionActorSlot *slot) {
    return (entry->index << 8) | (slot->member + UNION_ACTOR_ID_MEMBER_BASE);
}

UnionActorKind UnionOneself_IsMemberId(u16 id) {
    u16 member = (id & 0xff) - UNION_ACTOR_ID_MEMBER_BASE;

    if (member == 0) {
        return UNION_ACTOR_PARENT;
    }
    return UNION_ACTOR_MEMBER;
}

u16 UnionOneself_GetEntryIndex(u16 id) {
    return id >> 8;
}

u8 UnionOneself_GetActorGender(UnionSystem *unisys, u16 id) {
    int i;
    UnionActorSlot *slot = unisys->actors;

    for (i = 0; i < UNION_ACTOR_MAX; i++, slot++) {
        if (slot->used == 1 && slot->entry != NULL && id == UnionOneself_GetActorId(slot->entry, slot)) {
            return slot->gender;
        }
    }
    return 0;
}

BOOL UnionActor_IsId(u16 id) {
    u16 low = id & 0xff;

    if (low < UNION_ACTOR_ID_MEMBER_BASE) {
        return FALSE;
    }
    return TRUE;
}

BOOL UnionOneself_CanTalkTo(UnionSystem *unisys, u16 id) {
    UnionActorSlot *slot = unisys->actors;
    int i;

    for (i = 0; i < UNION_ACTOR_MAX; i++, slot++) {
        if (slot->used == 1 && slot->entry != NULL && slot->entry->timeout != 0
            && id == (slot->entry->index << 8 | (slot->member + UNION_ACTOR_ID_MEMBER_BASE))) {
            if (slot->step == 2 || slot->step == 1 || slot->nextStep == 1 || slot->nextStep == 2) {
                return FALSE;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static FieldActor *UnionOneself_FindActor(UnionSystem *unisys, UnionBeaconEntry *entry, UnionActorSlot *slot) {
    u16 id = UnionOneself_GetActorId(entry, slot);
    FieldActor *mmdl = FindFieldActor(GameData_GetMMSys(unisys->param->gameData), id);

    GFL_ASSERT(mmdl != NULL);
    return mmdl;
}

static UnionActorSlot *UnionOneself_CreateActorSlot(UnionSystem *unisys, UnionBeaconEntry *entry, UnionMember *member,
                                                    int memberIndex, BOOL visible, FieldActor **outActor) {
    UnionActorSlot *slot = unisys->actors;
    FieldActor *actor;
    int i;

    for (i = 0; i < UNION_ACTOR_MAX; i++, slot++) {
        if (slot->used == 0) {
            slot->entry = entry;
            slot->look = member->look;
            slot->gender = member->gender;
            slot->member = memberIndex;
            slot->used = 1;
            actor = UnionOneself_CreateActor(unisys, unisys->param->gameData, member->look,
                                             UnionOneself_GetActorId(entry, slot));
            if (!visible) {
                SetActorHidden(actor, TRUE);
            }
            *outActor = actor;
            return slot;
        }
    }
    GFL_ASSERT(0);
    return NULL;
}

static void UnionOneself_FreeActorSlot(UnionSystem *unisys, UnionBeaconEntry *entry, UnionActorSlot *slot) {
    Field *field = GSYS_GetField(unisys->param->gsys);
    FieldActor *actor = UnionOneself_FindActor(unisys, entry, slot);

    if (field != NULL && actor != NULL) {
        UnionEff_RemoveMark(func_ov036_0218051c(field), actor);
    }
    DeleteActor(actor);
    sys_memset(slot, 0, sizeof(UnionActorSlot));
}

static void UnionOneself_UpdateEntryActors(UnionSystem *unisys, RivalDataSave *rival, WifiList *wifiList, void *effWork,
                                           UnionBeaconEntry *entry) {
    FieldActor *actor;
    u8 ownMac[6];
    int i;

    if (entry->timeout == 0) {
        return;
    }
    func_0207c33c(ownMac);
    for (i = 0; i < UNION_MEMBER_MAX; i++) {
        if (entry->actor[i] == NULL && entry->beacon.members[i].used == 1
            && GFL_STD_MemCmp(ownMac, entry->beacon.members[i].mac, 6) != 0
            && UnionOneself_IsMemberNew(unisys, entry, entry->beacon.members[i].mac)) {
            entry->actor[i] = UnionOneself_CreateActorSlot(unisys, entry, &entry->beacon.members[i], i, FALSE, &actor);
            entry->actor[i]->step = 1;
            UnionOneself_SetActorNextStep(entry->actor[i], 0);
            if (i == 0) {
                if (UnionEff_IsFriendBeacon(wifiList, &entry->beacon) == TRUE) {
                    UnionEff_AddMark(effWork, actor, 0);
                } else if (Misc_IsMetTrainer(rival, entry->beacon.trainerId) == TRUE) {
                    UnionEff_AddMark(effWork, actor, 1);
                }
            }
        }
    }
}

static BOOL UnionOneself_IsMemberNew(UnionSystem *unisys, UnionBeaconEntry *entry, const u8 *mac) {
    UnionBeaconEntry *other = unisys->found;
    int i;

    for (i = 0; i < UNION_BEACON_ENTRY_MAX; i++, other++) {
        if (other != entry && other->beacon.valid == 1) {
            int j;

            for (j = 0; j < UNION_MEMBER_MAX; j++) {
                if (other->beacon.members[j].used == 1 && GFL_STD_MemCmp(mac, other->beacon.members[j].mac, 6) == 0) {
                    if (other->timeout != 0) {
                        other->timeout = 1;
                    }
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

static BOOL UnionOneself_ShowFocus(UnionSystem *unisys, UnionBeaconEntry *entry) {
    int i;

    for (i = 0; i < UNION_MEMBER_MAX; i++) {
        UnionActorSlot *slot = entry->actor[i];

        if (slot != NULL && entry->beacon.members[i].used == 1
            && GFL_STD_MemCmp(entry->beacon.members[i].mac, unisys->situ.mac78, 6) == 0) {
            if (GSYS_CheckField(unisys->param->gsys) == TRUE) {
                void *effWork = func_ov036_0218051c(GSYS_GetField(unisys->param->gsys));

                if (effWork != NULL) {
                    u16 id = UnionOneself_GetActorId(entry, slot);
                    FieldActor *actor = FindFieldActor(GameData_GetMMSys(unisys->param->gameData), id);

                    if (actor != NULL) {
                        UnionEff_AddFocus(effWork, actor);
                    }
                }
            }
            sys_memset(unisys->situ.mac78, 0, 6);
            return TRUE;
        }
    }
    return FALSE;
}

void UnionOneself_UpdateActors(UnionSystem *unisys, GameData *gameData, Field *field) {
    u8 zero[6] = { 0, 0, 0, 0, 0, 0 };
    int i;
    BOOL focus = FALSE;
    RivalDataSave *rival;
    WifiList *wifiList;
    void *effWork;
    MMSys *mmsys;
    UnionBeaconEntry *entry;

    rival = getHollow_RivalData(GameData_GetSaveControl(gameData));
    wifiList = GameData_GetWifiList(gameData);
    effWork = func_ov036_0218051c(field);
    mmsys = GameData_GetMMSys(gameData);
    if (GFL_STD_MemCmp(zero, unisys->situ.mac78, 6) != 0) {
        focus = TRUE;
    }
    entry = unisys->found;
    for (i = 0; i < UNION_BEACON_ENTRY_MAX; i++, entry++) {
        if (entry->beacon.valid == 1) {
            int emptyCount;
            int j;

            UnionOneself_UpdateTimeout(unisys, entry);
            UnionOneself_UpdateEntryActors(unisys, rival, wifiList, effWork, entry);
            if (focus == TRUE && UnionOneself_ShowFocus(unisys, entry) == TRUE) {
                focus = FALSE;
            }
            emptyCount = 0;
            for (j = 0; j < UNION_MEMBER_MAX; j++) {
                UnionActorSlot *slot = entry->actor[j];

                if (slot != NULL) {
                    if (UnionOneself_RunActor(unisys, slot, field) == FALSE) {
                        UnionOneself_FreeActorSlot(unisys, entry, slot);
                        entry->actor[j] = NULL;
                        emptyCount++;
                    }
                } else {
                    emptyCount++;
                }
            }
            if (emptyCount >= UNION_MEMBER_MAX && entry->timeout == 0) {
                sys_memset(entry, 0, sizeof(UnionBeaconEntry));
            }
        }
    }
    sys_memset(unisys->situ.mac78, 0, 6);
}

static BOOL UnionOneself_RunActor(UnionSystem *unisys, UnionActorSlot *slot, Field *field) {
    UnionActorFunc func;
    BOOL done = TRUE;

    switch (slot->phase) {
    case 0:
        func = sUnionActorSteps[slot->step].begin;
        break;
    case 1:
        func = sUnionActorSteps[slot->step].run;
        break;
    case 2:
        func = sUnionActorSteps[slot->step].end;
        break;
    }
    if (func != NULL) {
        done = func(unisys, slot, field, &slot->seq);
    }
    if (done == TRUE) {
        slot->seq = 0;
        slot->phase++;
        if (slot->phase > 2) {
            if (slot->step == 2) {
                return FALSE;
            }
            slot->phase = 0;
            slot->step = slot->nextStep;
            slot->nextStep = 0;
        }
    }
    return TRUE;
}

static BOOL UnionOneself_UpdateTimeout(UnionSystem *unisys, UnionBeaconEntry *entry) {
    UnionSelf *situ = &unisys->situ;

    if (situ->mycomm.target == entry || situ->mycomm.unk4 == entry || situ->mycomm.answer_pc == entry
        || situ->mycomm.parent == entry) {
        if (entry->timeout == 0) {
            entry->timeout = 1;
        }
        return TRUE;
    }
    if (entry->timeout == 0) {
        return FALSE;
    }
    entry->timeout--;
    return TRUE;
}

void UnionOneself_UpdateMarks(UnionSystem *unisys, GameData *gameData, Field *field) {
    RivalDataSave *rival = getHollow_RivalData(GameData_GetSaveControl(gameData));
    WifiList *wifiList = GameData_GetWifiList(gameData);
    void *effWork = func_ov036_0218051c(field);
    UnionBeaconEntry *entry = unisys->found;
    int i;

    for (i = 0; i < UNION_BEACON_ENTRY_MAX; i++, entry++) {
        if (entry->beacon.valid == 1 && entry->actor[0] != NULL) {
            FieldActor *actor = UnionOneself_FindActor(unisys, entry->actor[0]->entry, entry->actor[0]);

            if (actor != NULL) {
                if (UnionEff_IsFriendBeacon(wifiList, &entry->beacon) == TRUE) {
                    UnionEff_AddMark(effWork, actor, 0);
                } else if (Misc_IsMetTrainer(rival, entry->beacon.trainerId) == TRUE) {
                    UnionEff_AddMark(effWork, actor, 1);
                }
            }
        }
    }
}

static BOOL UnionOneself_ActorIdle(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq) {
    UnionBeaconEntry *entry = slot->entry;
    UnionSelf *situ = &unisys->situ;
    u32 beaconAct;

    if (entry->timeout == 0) {
        UnionOneself_SetActorNextStep(slot, 2);
        return TRUE;
    }
    if (entry->beacon.members[slot->member].used == 0 || slot->look != entry->beacon.members[slot->member].look) {
        UnionOneself_SetActorNextStep(slot, 2);
        return TRUE;
    }
    beaconAct = UnionEff_GetKindStep(entry->beacon.activity);
    if (situ->mycomm.unk44 == UnionOneself_GetActorId(slot->entry, slot)) {
        UnionOneself_SetActorNextStep(slot, 3);
        return TRUE;
    }
    if (situ->unk02 != 0 && (situ->unk02 == beaconAct || (beaconAct == 0 && situ->unk02 == entry->beacon.unk07_5))
        && slot->member == 0 && UnionMsg_GetTableByte(entry->beacon.activity) > entry->beacon.memberCount) {
        FieldActor *actor = UnionOneself_FindActor(unisys, slot->entry, slot);

        if (IsAllActorAcmdFinished(actor) == TRUE) {
            FldAct_SetAcmd(actor, 0x31);
        }
    } else if (slot->member == 0 && entry->beacon.memberCount > 1) {
        FieldActor *actor = UnionOneself_FindActor(unisys, slot->entry, slot);

        if (GetActorMoveCode(actor) == 2) {
            ChangeActorMoveCodeSeq(actor, 0xf);
        }
    } else if (slot->member == 0 && entry->beacon.memberCount <= 1) {
        FieldActor *actor = UnionOneself_FindActor(unisys, slot->entry, slot);

        if (GetActorMoveCode(actor) == 0xf) {
            ChangeActorMoveCodeSeq(actor, 2);
        }
    }
    return FALSE;
}

BOOL UnionActor_StepAppear(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq) {
    GridPos actorPos;
    GridPos playerPos;
    FieldActor *actor = UnionOneself_FindActor(unisys, slot->entry, slot);

    switch (*seq) {
    case 0:
        if (IsAllActorAcmdFinished(actor) == TRUE) {
            FldAct_GetGPos(FieldPlayer_GetActor(Field_GetPlayer(field)), &playerPos);
            FldAct_GetGPos(actor, &actorPos);
            if (actorPos.x != playerPos.x || actorPos.z != playerPos.z) {
                FldAct_SetAcmd(actor, 0x44);
                SetActorHidden(actor, FALSE);
                (*seq)++;
            }
        }
        break;
    case 1:
        if (func_ov012_02166ecc(actor) == TRUE) {
            func_ov012_02166ef8(actor);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL UnionOneself_ActorLeave(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq) {
    FieldActor *actor = UnionOneself_FindActor(unisys, slot->entry, slot);

    switch (*seq) {
    case 0:
        if (IsAllActorAcmdFinished(actor) == TRUE) {
            FldAct_SetAcmd(actor, 0x43);
            (*seq)++;
        }
        break;
    case 1:
        if (func_ov012_02166ecc(actor) == TRUE) {
            func_ov012_02166ef8(actor);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL UnionOneself_ActorTalk(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq) {
    UnionSelf *situ = &unisys->situ;
    FieldActor *actor = UnionOneself_FindActor(unisys, slot->entry, slot);

    switch (*seq) {
    case 0:
        if (IsAllActorAcmdFinished(actor) == TRUE) {
            u16 acmd;

            switch (GetActorFaceDir(FieldPlayer_GetActor(Field_GetPlayer(field)))) {
            case 0:
                acmd = 1;
                break;
            case 1:
                acmd = 0;
                break;
            case 2:
                acmd = 3;
                break;
            case 3:
            default:
                acmd = 2;
                break;
            }
            FldAct_SetAcmd(actor, acmd);
            (*seq)++;
        }
        break;
    case 1:
        if (func_ov012_02166ef8(actor) == TRUE) {
            SetActorFlag(actor, 8);
            (*seq)++;
        }
        break;
    case 2:
        if (situ->mycomm.unk44 != UnionOneself_GetActorId(slot->entry, slot)) {
            ClearActorFlag(actor, 8);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
