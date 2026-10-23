#include "constants/sound.h"
#include "field/fest_mission_data.h"
#include "field/fest_mission_gimmick.h"
#include "field/fest_mission_people.h"
#include "field/festival.h"
#include "field/field_sound.h"
#include "field/player_state.h"
#include "field/rival_select.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The Funfest's work: the current mission, the players in it and the missions seen. The file's name is the ROM's.

static void SetFesGimmickResidentEnable(FestMissionConfig *config, BOOL enable);
static void LinkFestival_TickMissionBeacon(LinkFestival *festival);
static void LinkFestivalBest_InitMine(LinkFestival *festival, LinkFestivalBest *best);
static void ClearLinkFestivalData(LinkFestival *festival);
static void LinkFestival_SetCurrentRecord(LinkFestival *festival, FestMissionConfig *config);
static BOOL LinkFestival_HasRecord(LinkFestival *festival, FestMissionConfig *config);
static void FestMissionRecord_Set(FestMissionRecord *record, const FestMissionConfig *config);
static BOOL FestMissionRecord_Matches(const FestMissionRecord *record, const FestMissionConfig *config);
static void FestMissionRecord_Clear(FestMissionRecord *record);

LinkFestival *CreateLinkFestival(HeapID heapId, GameSystem *gsys) {
    LinkFestival *festival = GFL_HeapAllocate(heapId, sizeof(LinkFestival), TRUE, "link_festival.c", 56);

    festival->gsys = gsys;
    festival->gameData = GSYS_GetGameData(gsys);
    festival->archive = GFL_ArcSysCreateFileHandle(0x121, heapId);
    festival->headers = FestMissionData_LoadHeaders(festival->archive, heapId);
    ClearFesMissionCfg(&festival->config);
    festival->info = &festival->config.info;
    festival->missionHeader = &festival->config.header;
    festival->best = LinkFestivalBest_Create(heapId);
    ClearLinkFestivalData(festival);
    festival->beaconInterval = 600;
    return festival;
}

void FreeLinkFestival(LinkFestival *festival) {
    LinkFestivalBest_Free(festival->best);
    GFL_HeapFree(festival->headers);
    GFL_ArcToolFree(festival->archive);
    MI_CpuFill8(festival, 0, sizeof(LinkFestival));
    GFL_HeapFree(festival);
}

void LinkFestival_Update(LinkFestival *festival) {
    FestMissionOffers_DropExpired(festival);
    if (isFesMissionAvailable(&festival->config)) {
        if (festival->state == 2) {
            int total = LinkFestival_GetTotalScore(festival);
            int gain = total - festival->lastTotal;

            if (gain != 0) {
                u32 added = FestMissionConfig_AddSeconds(&festival->config, gain);

                festival->lastTotal = total;
                festival->secondsGained += (u16)added;
            }
        }
        LinkFestival_TickMissionBeacon(festival);
    }
}

u32 getStatusOfFesMission(LinkFestival *festival) {
    return isFesMissionAvailable(&festival->config);
}

FestMissionConfig *GetFestMissionCfg(LinkFestival *festival) {
    return &festival->config;
}

ArcTool *LinkFestival_GetArchive(LinkFestival *festival) {
    return festival->archive;
}

void LinkFestival_FillMissionInfo(LinkFestival *festival, void *mission, u16 value) {
    FestMissionInfo *info = mission;

    *info = *festival->info;
    info->unkC_0 = value;
    info->seconds = LinkFestival_GetSecondsLeft(festival);
}

u16 LinkFestival_GetScore(LinkFestival *festival) {
    return festival->config.missionId;
}

u16 LinkFestival_GetSecondsLeft(LinkFestival *festival) {
    return FestMissionConfig_GetSecondsLeft(&festival->config);
}

void LinkFestival_GetRanking(LinkFestival *festival, LinkFestivalRanking *ranking) {
    int i = 0;
    int j;
    int gap;
    int count;
    u8 order[LINK_FESTIVAL_RECORD_COUNT];

    MI_CpuFill8(ranking, 0, sizeof(LinkFestivalRanking));
    ranking->count = festival->recordCount + 1;
    ranking->score = festival->config.missionId;
    ranking->total = LinkFestival_GetTotalScore(festival);
    count = festival->recordCount;
    if (count == 0) {
        ranking->rank = 1;
        return;
    }
    for (; i < count; i++) {
        order[i] = i;
    }
    // A Shell sort of the members, the highest score first
    for (gap = 1; gap < count / 9; gap = gap * 3 + 1) {
    }
    for (; gap > 0; gap /= 3) {
        for (i = gap; i < count; i++) {
            for (j = i;
                 j >= gap && festival->records[order[j - gap]].member.score < festival->records[order[j]].member.score;
                 j -= gap) {
                u8 swap = order[j];

                order[j] = order[j - gap];
                order[j - gap] = swap;
            }
        }
    }
    if (ranking->score >= festival->records[order[0]].member.score) {
        ranking->rank = 1;
    } else if (ranking->score <= festival->records[order[count - 1]].member.score) {
        ranking->rank = ranking->count;
    }
    for (i = 0; i < festival->recordCount; i++) {
        if (ranking->score >= festival->records[order[i]].member.score) {
            ranking->rank = i + 1;
            return;
        }
    }
}

FestMissionLog *LinkFestival_GetLog(LinkFestival *festival) {
    return &festival->log;
}

FestMissionLog *LinkFestival_GetViewLog(LinkFestival *festival) {
    return &festival->viewLog;
}

FestMissionPassPowers *LinkFestival_GetPassPowers(LinkFestival *festival) {
    return &festival->passPowers;
}

void FetchFestMissionCfgGimmickID(LinkFestival *festival, u8 index, FestMissionHeader *dest) {
    FestMissionHeader *headers = festival->headers;
    FestMissionHeader *header = dest;

    *header = headers[index];
}

void LinkFestival_StartMission(LinkFestival *festival, const FestMissionConfig *mission, u8 joined) {
    ClearLinkFestivalData(festival);
    FestMissionConfig_Start(&festival->config, mission, joined);
    LinkFestival_SetState(festival, 1);
    GameCommSys_SetInFunfestMission(GSYS_GetGameCommSystem(festival->gsys), TRUE);
    LinkFestival_SetCurrentRecord(festival, &festival->config);
    SetFesGimmickResidentEnable(mission, TRUE);
    GameData_SetFunfestMission(festival->gameData, TRUE);
}

void LinkFestival_StartTimer(LinkFestival *festival, u32 arg1) {
    if (isFesMissionAvailable(&festival->config)) {
        if (festival->config.startSeconds == -1) {
            FestMissionConfig_StartTimer(&festival->config);
        }
        LinkFestival_SetState(festival, 2);
    }
}

void LinkFestival_SetState(LinkFestival *festival, u32 state) {
    festival->state = state;
}

int LinkFestival_GetState(LinkFestival *festival) {
    return festival->state;
}

// Ends the mission. Returns the zone's music when the Funfest's is playing, else -1
u32 LinkFestival_GetNormalChangeBGMID(LinkFestival *festival) {
    u16 zoneId;

    GameData_SetFunfestMission(festival->gameData, FALSE);
    SetFesGimmickResidentEnable(&festival->config, FALSE);
    FestMissionConfig_Clear(&festival->config);
    GameCommSys_SetInFunfestMission(GSYS_GetGameCommSystem(festival->gsys), FALSE);
    ClearLinkFestivalData(festival);
    if (GameData_GetNowBGM(festival->gameData) != SEQ_BGM_FES) {
        return 0xffffffff;
    }
    zoneId = PlayerState_GetZoneID(GameData_GetPlayerState(festival->gameData));
    return GetMapBGMIDByPlayerState2(festival->gameData, zoneId, GameData_GetSeason(festival->gameData));
}

u16 LinkFestival_GetShownTotal(LinkFestival *festival) {
    return festival->shownTotal;
}

u16 LinkFestival_GetShownScore(LinkFestival *festival) {
    return festival->shownScore;
}

void LinkFestival_SetShownTotal(LinkFestival *festival, u16 value) {
    festival->shownTotal = value;
    festival->shownScore = festival->config.missionId;
}

u16 LinkFestival_GetSecondsGained(LinkFestival *festival) {
    return festival->secondsGained;
}

void LinkFestival_ClearSecondsGained(LinkFestival *festival) {
    festival->secondsGained = 0;
}

// Whether the player came to a new zone from outside the Entralink
BOOL LinkFestival_IsNewZoneFromOutside(LinkFestival *festival, u16 zoneId) {
    u16 previous = festival->zoneId;

    if (previous != zoneId) {
        festival->zoneId = zoneId;
        if (!IsZoneEntralinkHub(previous)) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

FestMissionWork *LinkFestival_GetMissionWork(LinkFestival *festival) {
    return &festival->missionWork;
}

int LinkFestival_GetTotalScore(LinkFestival *festival) {
    int i;
    u32 total = festival->config.missionId;

    if (!isFesMissionAvailable(&festival->config)) {
        return 0;
    }
    for (i = 0; i < festival->recordCount; i++) {
        total += festival->records[i].member.score;
    }
    if (total > 9999) {
        return 9999;
    }
    return total;
}

BOOL LinkFestival_IsTargetReached(LinkFestival *festival, u32 *total) {
    BOOL reached = FALSE;

    *total = 0;
    *total = LinkFestival_GetTotalScore(festival);
    if (*total >= festival->info->count) {
        reached = TRUE;
    }
    return reached;
}

BOOL LinkFestival_IsMissionOver(LinkFestival *festival) {
    u32 total;
    u16 seconds;
    BOOL over;

    if (!isFesMissionAvailable(&festival->config)) {
        return FALSE;
    }
    seconds = LinkFestival_GetSecondsLeft(festival);
    if (LinkFestival_IsTargetReached(festival, &total) && festival->info->unk4_31 == TRUE) {
        over = TRUE;
    } else {
        over = TRUE;
        if (seconds != 0) {
            over = FALSE;
        }
    }
    if (over) {
        LinkFestival_SetState(festival, 3);
        festival->config.unk20 = seconds;
    }
    return over;
}

u32 LinkFestival_AddMissionScore(GameSystem *gsys, u32 a1, u32 a2) {
    LinkFestival *festival = GSYS_GetLinkFestival(gsys);
    BOOL result = FestMissionConfig_AddScore(&festival->config, a1, a2, 1);

    if (result) {
        LinkFestival_UpdateBest(festival, 0, festival->config.missionId, NULL);
    }
    return result;
}

int LinkFestival_GetMemberCount(LinkFestival *festival) {
    if (isFesMissionAvailable(&festival->config) == FALSE) {
        return 0;
    }
    return festival->recordCount + 1;
}

void LinkFestival_AddRecord(LinkFestival *festival, void *mission) {
    if (isFesMissionAvailable(&festival->config)) {
        return;
    }
    if (LinkFestival_HasRecord(festival, mission)) {
        return;
    }
    FestMissionRecord_Set(&festival->records[festival->recordIndex], mission);
    festival->recordIndex = (festival->recordIndex + 1) % LINK_FESTIVAL_RECORD_COUNT;
    if (festival->recordCount < LINK_FESTIVAL_RECORD_COUNT) {
        festival->recordCount++;
    }
    FestMissionOffers_Remove(festival, mission);
}

BOOL LinkFestival_ShouldSkipOffer(LinkFestival *festival, FestMissionConfig *config) {
    FestMissionInfo *info = &config->info;

    if (info->index == 0) {
        return TRUE;
    }
    if (festival->current.header.index != FEST_MISSION_NONE && FestMissionRecord_Matches(&festival->current, config)) {
        return TRUE;
    }
    if (LinkFestival_HasRecord(festival, config)) {
        return TRUE;
    }
    if (info->seconds < 30) {
        return TRUE;
    }
    return FALSE;
}

void LinkFestival_UpdateBest(LinkFestival *festival, u16 trainerId, u16 value, StrBuf *name) {
    LinkFestivalBest *best = festival->best;

    if (name == NULL) {
        PlayerInfo *info = GetGameDataPlayerInfo(festival->gameData);

        if (best->score >= festival->config.missionId) {
            return;
        }
        best->score = festival->config.missionId;
        best->trainerId = getTrainerID(info);
        textCopy(info->name, best->name);
    } else {
        if (best->score >= value) {
            return;
        }
        best->score = value;
        best->trainerId = trainerId;
        GFL_StrBufCopy(best->name, name);
    }
    festival->bestChanged = TRUE;
}

BOOL LinkFestival_IsBestChanged(LinkFestival *festival) {
    return festival->bestChanged;
}

// Whether the player has the highest score
BOOL LinkFestival_IsBestMine(LinkFestival *festival) {
    u16 trainerId = getTrainerID(GetGameDataPlayerInfo(festival->gameData));

    if (festival->best->trainerId == trainerId) {
        return TRUE;
    }
    return FALSE;
}

void LinkFestival_GetBest(LinkFestival *festival, LinkFestivalBest *dest) {
    dest->trainerId = festival->best->trainerId;
    dest->score = festival->best->score;
    GFL_StrBufCopy(dest->name, festival->best->name);
    festival->bestChanged = FALSE;
}

LinkFestivalBest *LinkFestivalBest_Create(HeapID heapId) {
    LinkFestivalBest *best = GFL_HeapAllocate(heapId, sizeof(LinkFestivalBest), TRUE, "link_festival.c", 760);

    best->name = GFL_StrBufCreate(8, heapId);
    return best;
}

void LinkFestivalBest_Free(LinkFestivalBest *best) {
    GFL_StrBufFree(best->name);
    MI_CpuFill8(best, 0, sizeof(LinkFestivalBest));
    GFL_HeapFree(best);
}

// Loads or unloads the overlay of the mission's kind
static void SetFesGimmickResidentEnable(FestMissionConfig *config, BOOL enable) {
    u32 overlayId;

    switch (config->header.kind) {
    case 4:
        overlayId = OVERLAY_ID(24);
        break;
    case 5:
        overlayId = OVERLAY_ID(25);
        break;
    case 3:
        overlayId = OVERLAY_ID(26);
        break;
    default:
        return;
    }
    if (enable) {
        GFL_OvlLoad(overlayId);
    } else {
        GFL_OvlUnload(overlayId);
    }
}

static void LinkFestival_TickMissionBeacon(LinkFestival *festival) {
    if (festival->beaconTimer != 0) {
        festival->beaconTimer--;
        return;
    }
    GameBeaconSys_SendMission();
    festival->beaconTimer = festival->beaconInterval;
}

static void LinkFestivalBest_InitMine(LinkFestival *festival, LinkFestivalBest *best) {
    PlayerInfo *info = GetGameDataPlayerInfo(festival->gameData);

    textCopy(info->name, best->name);
    best->trainerId = getTrainerID(info);
    best->score = 0;
}

static void ClearLinkFestivalData(LinkFestival *festival) {
    int i;

    LinkFestivalBest_InitMine(festival, festival->best);
    for (i = 0; i < LINK_FESTIVAL_RECORD_COUNT; i++) {
        FestMissionRecord_Clear(&festival->records[i]);
    }
    festival->recordCount = 0;
    festival->recordIndex = 0;
    festival->beaconTimer = 0;
    festival->shownTotal = 0;
    festival->shownScore = 0;
    festival->secondsGained = 0;
    festival->lastTotal = 0;
    festival->zoneId = 0xffff;
    MI_CpuFill8(festival->missionWork.hollows, 0, sizeof(festival->missionWork.hollows));
    festival->bestChanged = FALSE;
    FestMissionLog_Clear(&festival->log);
    FestMissionLog_Clear(&festival->viewLog);
    FestMissionPassPowers_Clear(&festival->passPowers);
    LinkFestival_SetState(festival, 0);
}

static void LinkFestival_SetCurrentRecord(LinkFestival *festival, FestMissionConfig *config) {
    FestMissionRecord_Clear(&festival->current);
    FestMissionRecord_Set(&festival->current, config);
}

// Whether the mission is among those seen, from the newest
static BOOL LinkFestival_HasRecord(LinkFestival *festival, FestMissionConfig *config) {
    int i;
    int index = (festival->recordIndex + LINK_FESTIVAL_RECORD_COUNT - 1) % LINK_FESTIVAL_RECORD_COUNT;

    for (i = 0; i < festival->recordCount; i++) {
        if (FestMissionRecord_Matches(&festival->records[index], config)) {
            return TRUE;
        }
        index = (index + LINK_FESTIVAL_RECORD_COUNT - 1) % LINK_FESTIVAL_RECORD_COUNT;
    }
    return FALSE;
}

static void FestMissionRecord_Set(FestMissionRecord *record, const FestMissionConfig *config) {
    record->mission.unk4 = config->seed;
    record->mission.unk0 = config->unk00[0];
}

static BOOL FestMissionRecord_Matches(const FestMissionRecord *record, const FestMissionConfig *config) {
    if (record->mission.unk4 != config->seed || record->mission.unk0 != config->unk00[0]) {
        return FALSE;
    }
    return TRUE;
}

static void FestMissionRecord_Clear(FestMissionRecord *record) {
    MI_CpuFill8(record, 0, sizeof(FestMissionRecord));
    record->header.index = FEST_MISSION_NONE;
}

void FestMissionEntry_Clear(FestMissionEntry *entry) {
    MI_CpuFill8(entry, 0, sizeof(FestMissionEntry));
    ClearFesMissionCfg(&entry->config);
    FestMissionRecord_Clear(&entry->record);
}

// Takes a beacon's mission as a member's: 0 when it isn't the current mission or there is no room, 1 for a new member
// and 2 or 4 for a known one, 2 when the member's zone changed, plus 1 when the score rose. out gets the score
int LinkFestival_UpdateMember(LinkFestival *festival, FestMissionEntry *entry, u32 *out) {
    int i = 0;
    FestMissionRecord *record;

    *out = 0;
    if (!FestMissionConfig_IsSame(&festival->config, &entry->config)) {
        return i;
    }
    for (; i < festival->recordCount; i++) {
        record = &festival->records[i];
        if (record->member.trainerId == entry->record.member.trainerId) {
            int result = 0;

            if (entry->record.member.score > record->member.score) {
                record->member.score = entry->record.member.score;
                result = 1;
            }
            *out = record->member.score;
            if (record->member.zoneId != entry->record.member.zoneId) {
                record->member.zoneId = entry->record.member.zoneId;
                return result + 2;
            }
            return result + 4;
        }
    }
    if (festival->recordCount >= LINK_FESTIVAL_RECORD_COUNT) {
        return 0;
    }
    record = &festival->records[festival->recordCount];
    *record = entry->record;
    *out = record->member.score;
    festival->recordCount++;
    return 1;
}
