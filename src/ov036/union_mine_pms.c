// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/union_comm.h"
#include "field/union_eff.h"
#include "field/union_oneself.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/str_tool.h"

// The chat line of a player who said nothing
#define UNION_CHAT_TYPE_NORMAL 0

// What a line of the chat holds when it is for a player who has not given a MAC
static const u8 NO_MAC[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static UnionVisitor *UnionChat_AllocateLine(UnionVisitorList *list);
static BOOL UnionChat_IsNewBeacon(UnionVisitorList *list, const u8 *mac, u16 counter);

// Adds a line to the chat: for a beacon heard, or for this player with a sentence
void UnionChat_AppendLine(UnionSystem *unisys, UnionBeaconEntry *bpc, const u16 *mine_pmsdata, u8 mine_chat_type) {
    UnionVisitorList *list = &unisys->visitorList;
    UnionVisitor *visitor;

    // clang-format off
    GFL_ASSERT(bpc != NULL || mine_pmsdata != NULL);
    // clang-format on
    if (bpc == NULL || UnionChat_IsNewBeacon(list, bpc->mac, bpc->beacon.counter)) {
        visitor = UnionChat_AllocateLine(list);
        if (bpc == NULL) {
            PlayerInfo *info = unisys->param->playerInfo;

            copyName(info->name, visitor->name, 8);
            // clang-format off
            GFL_ASSERT(mine_pmsdata != NULL || (mine_pmsdata==NULL && mine_chat_type != UNION_CHAT_TYPE_NORMAL));
            // clang-format on
            visitor->sentence.type = mine_pmsdata[0];
            visitor->sentence.id = mine_pmsdata[1];
            visitor->sentence.words[0] = mine_pmsdata[2];
            visitor->sentence.words[1] = mine_pmsdata[3];
            visitor->kind = mine_chat_type;
            visitor->counter = 0;
            OS_GetMacAddress(visitor->mac);
            visitor->gender = getTrainerGender(info);
            visitor->subKind = UNION_CHAT_RELATION_NONE;
        } else {
            GameData *gameData;
            WifiList *wifiList;
            RivalDataSave *rival;

            wcharsncpy(bpc->beacon.name, visitor->name, 8);
            visitor->sentence.type = bpc->beacon.look2[0];
            visitor->sentence.id = bpc->beacon.look2[1];
            visitor->sentence.words[0] = bpc->beacon.look2[2];
            visitor->sentence.words[1] = bpc->beacon.look2[3];
            visitor->kind = bpc->beacon.state;
            visitor->counter = bpc->beacon.counter;
            MI_CpuCopy8(bpc->mac, visitor->mac, 6);
            visitor->gender = bpc->beacon.gender;
            gameData = GSYS_GetGameData(unisys->param->gsys);
            wifiList = GameData_GetWifiList(gameData);
            rival = getHollow_RivalData(GameData_GetSaveControl(gameData));
            if (UnionEff_IsFriendBeacon(wifiList, &bpc->beacon) == TRUE) {
                visitor->subKind = UNION_CHAT_RELATION_FRIEND;
            } else if (Misc_IsMetTrainer(rival, bpc->beacon.trainerId) == TRUE) {
                visitor->subKind = UNION_CHAT_RELATION_MET;
            } else {
                visitor->subKind = UNION_CHAT_RELATION_NONE;
            }
        }
        if (list->count == list->viewEnd) {
            list->viewEnd++;
        }
        list->count++;
        if (list->count - list->viewEnd >= UNION_VISITOR_MAX) {
            list->viewEnd = list->count - (UNION_VISITOR_MAX - 1);
        }
    }
}

// The next line of the ring to fill
static UnionVisitor *UnionChat_AllocateLine(UnionVisitorList *list) {
    UnionVisitor *visitor;

    if (list->count > 0 && list->next == list->oldest) {
        list->oldest++;
        if (list->oldest >= UNION_VISITOR_MAX) {
            list->oldest = 0;
        }
    }
    visitor = &list->visitors[list->next];
    list->next++;
    if (list->next >= UNION_VISITOR_MAX) {
        list->next = 0;
    }
    return visitor;
}

UnionVisitor *UnionChat_GetLine(UnionVisitorList *list, u32 index) {
    UnionVisitor *visitor = &list->visitors[index % UNION_VISITOR_MAX];

    if (GFL_STD_MemCmp(visitor->mac, NO_MAC, 6) == 0) {
        visitor = NULL;
    }
    return visitor;
}

// Whether the player whose MAC and counter are given has no line in the newest ones yet, or a line with a different
// counter
static BOOL UnionChat_IsNewBeacon(UnionVisitorList *list, const u8 *mac, u16 counter) {
    s32 i;

    for (i = list->count; i > list->count - UNION_VISITOR_MAX; i--) {
        UnionVisitor *visitor;

        if (i < 0) {
            return TRUE;
        }
        visitor = UnionChat_GetLine(list, i);
        if (visitor != NULL && GFL_STD_MemCmp(visitor->mac, mac, 6) == 0) {
            if (visitor->counter != counter) {
                return TRUE;
            }
            return FALSE;
        }
    }
    return TRUE;
}
