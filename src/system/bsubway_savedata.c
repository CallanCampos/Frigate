#include "types.h"
#include "save/bsubway_save.h"
#include "field/bsubway_scr.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// The Battle Subway's three save blocks: the play saved in the middle of (0x38), the scores with the Battle Points
// (0x39) and the Wi-Fi train's downloaded data (0x3a). The file's name is the ROM's, which BSubwayWifiData_AllocLeaders passes to
// the heap.

static u32 BSubwayScoreData_GetModeIndex(u32 mode);
static void BSubwayWifiData_ClearRoomFlags(BSubwayWifiData *data);
static BOOL BSubwayWifiData_IsDateAfter(const RTCDate *date1, const RTCDate *date2);

u32 BSubwayPlayData_GetSize(void) {
    return sizeof(BSubwayPlayData);
}

void BSubwayPlayData_Init(BSubwayPlayData *data) {
    sys_memset(data, 0, sizeof(BSubwayPlayData));
}

void BSubwayPlayData_SetSaveFlag(BSubwayPlayData *data, u32 value) {
    data->saved = value;
}

u32 BSubwayPlayData_GetSaveFlag(BSubwayPlayData *data) {
    return data->saved;
}

u32 BSubwayPlayData_GetData(BSubwayPlayData *data, u32 id, void *buffer) {
    switch (id) {
    case BSUBWAY_PLAY_MODE:
        return data->playMode;
    case BSUBWAY_PLAY_ROUND:
        return data->round;
    case BSUBWAY_PLAY_WIFI_DOWN:
        return data->wifiDown;
    case BSUBWAY_PLAY_WIFI_TURN:
        return data->wifiTurn;
    case BSUBWAY_PLAY_WIFI_DAMAGE:
        return data->wifiDamage;
    case BSUBWAY_PLAY_UNK8:
        sys_memcpy(data->unk8, buffer, sizeof(data->unk8));
        return 0;
    case BSUBWAY_PLAY_UNK28:
        sys_memcpy(data->unk28, buffer, sizeof(data->unk28));
        return 0;
    case BSUBWAY_PLAY_UNK0_0:
        return data->unk0_0;
    case BSUBWAY_PLAY_UNKC:
        sys_memcpy(data->unkC, buffer, sizeof(data->unkC));
        return 0;
    case BSUBWAY_PLAY_UNK0_6:
        return data->unk0_6;
    case BSUBWAY_PLAY_UNK0_9:
        return data->unk0_9;
    case BSUBWAY_PLAY_UNK0_10:
        return data->unk0_10;
    }
    return 0;
}

void BSubwayPlayData_SetData(BSubwayPlayData *data, u32 id, const void *value) {
    switch (id) {
    case BSUBWAY_PLAY_MODE:
        data->playMode = *(const u8 *)value;
        break;
    case BSUBWAY_PLAY_ROUND:
        data->round = *(const u8 *)value;
        break;
    case BSUBWAY_PLAY_WIFI_DOWN:
        data->wifiDown = *(const u8 *)value;
        break;
    case BSUBWAY_PLAY_WIFI_TURN:
        data->wifiTurn = *(const u16 *)value;
        break;
    case BSUBWAY_PLAY_WIFI_DAMAGE:
        data->wifiDamage = *(const u16 *)value;
        break;
    case BSUBWAY_PLAY_UNK8:
        sys_memcpy(value, data->unk8, sizeof(data->unk8));
        break;
    case BSUBWAY_PLAY_UNK28:
        sys_memcpy(value, data->unk28, sizeof(data->unk28));
        break;
    case BSUBWAY_PLAY_UNK0_0:
        data->unk0_0 = *(const u8 *)value;
        break;
    case BSUBWAY_PLAY_UNKC:
        sys_memcpy(value, data->unkC, sizeof(data->unkC));
        break;
    case BSUBWAY_PLAY_UNK0_6:
        data->unk0_6 = *(const u8 *)value;
        break;
    case BSUBWAY_PLAY_UNK0_9:
        data->unk0_9 = *(const u8 *)value;
        break;
    case BSUBWAY_PLAY_UNK0_10:
        data->unk0_10 = *(const u8 *)value;
        break;
    }
}

void BSubwayPlayData_AddWifiRecord(BSubwayPlayData *data, u8 down, u8 turn, u16 damage) {
    if (data->wifiDown + down < 0xff) {
        data->wifiDown += down;
    }
    if (data->wifiTurn + turn < 0xffff) {
        data->wifiTurn += turn;
    }
    if (data->wifiDamage + damage < 0xffff) {
        data->wifiDamage += damage;
    }
}

void BSubwayPlayData_ResetRound(BSubwayPlayData *data) {
    u8 round = 0;

    BSubwayPlayData_SetData(data, BSUBWAY_PLAY_ROUND, &round);
}

void BSubwayPlayData_IncRound(BSubwayPlayData *data) {
    u8 round = BSubwayPlayData_GetData(data, BSUBWAY_PLAY_ROUND, NULL);

    if (round < 0xff) {
        round++;
    }
    BSubwayPlayData_SetData(data, BSUBWAY_PLAY_ROUND, &round);
}

u16 BSubwayPlayData_GetRound(BSubwayPlayData *data) {
    return BSubwayPlayData_GetData(data, BSUBWAY_PLAY_ROUND, NULL);
}

u32 BSubwayScoreData_GetSize(void) {
    return sizeof(BSubwayScoreData);
}

void BSubwayScoreData_Init(BSubwayScoreData *score) {
    sys_memset(score, 0, sizeof(BSubwayScoreData));
    score->wifiRank = 1;
}

void BSubwayScoreData_AddBattlePoints(BSubwayScoreData *score, u16 amount) {
    if (score->battlePoints + amount > BSUBWAY_BATTLE_POINTS_MAX) {
        score->battlePoints = BSUBWAY_BATTLE_POINTS_MAX;
    } else {
        score->battlePoints += amount;
    }
}

void BSubwayScoreData_SubBattlePoints(BSubwayScoreData *score, u16 amount) {
    s16 rest = (s16)score->battlePoints - (s16)amount;

    if (rest >= 0) {
        score->battlePoints -= amount;
    } else {
        score->battlePoints = 0;
    }
}

u16 BSubwayScoreData_GetBattlePoints(BSubwayScoreData *score) {
    return score->battlePoints;
}

// The index of a play mode's scores
static u32 BSubwayScoreData_GetModeIndex(u32 mode) {
    return mode;
}

u16 BSubwayScoreData_GetStreak(BSubwayScoreData *score, u32 mode) {
    return score->streaks[BSubwayScoreData_GetModeIndex(mode)];
}

u16 BSubwayScoreData_GetMaxStreak(BSubwayScoreData *score, u32 mode) {
    return score->maxStreaks[BSubwayScoreData_GetModeIndex(mode)];
}

void BSubwayScoreData_UpdateMaxStreak(BSubwayScoreData *score, u32 mode, u32 value) {
    u32 index = BSubwayScoreData_GetModeIndex(mode);

    if (score->maxStreaks[index] < value) {
        score->maxStreaks[index] = value;
    }
}

void BSubwayScoreData_SetStreak(BSubwayScoreData *score, u32 mode, u32 value) {
    score->streaks[BSubwayScoreData_GetModeIndex(mode)] = value;
}

void BSubwayScoreData_ResetStage(BSubwayScoreData *score, u32 mode) {
    score->stages[BSubwayScoreData_GetModeIndex(mode)] = 0;
}

void BSubwayScoreData_StartStage(BSubwayScoreData *score, u32 mode) {
    score->stages[BSubwayScoreData_GetModeIndex(mode)] = 1;
}

BOOL BSubwayScoreData_HasStage(BSubwayScoreData *score, u32 mode) {
    if (score->stages[BSubwayScoreData_GetModeIndex(mode)] != 0) {
        return TRUE;
    }
    return FALSE;
}

void BSubwayScoreData_IncStage(BSubwayScoreData *score, u32 mode) {
    u32 index = BSubwayScoreData_GetModeIndex(mode);

    if (score->stages[index] < 0xfffe) {
        score->stages[index]++;
    }
}

u16 BSubwayScoreData_GetStage(BSubwayScoreData *score, u32 mode) {
    u16 stage = score->stages[BSubwayScoreData_GetModeIndex(mode)] - 1;

    if ((s16)stage < 0) {
        stage = 0;
    }
    return stage;
}

BOOL BSubwayScoreData_SetFlag(BSubwayScoreData *score, int id, u32 op) {
    u32 i;
    u32 flag = 1;

    if (id >= 11) {
        return FALSE;
    }
    for (i = 0; i < id; i++) {
        flag <<= 1;
    }
    switch (op) {
    case BSUBWAY_FLAG_RESET:
        score->flags &= flag ^ 0xffffffff;
        break;
    case BSUBWAY_FLAG_SET:
        score->flags |= flag;
        break;
    case BSUBWAY_FLAG_GET:
        return (score->flags >> id) & 1;
    }
    return FALSE;
}

void BSubwayScoreData_IncWifiRank(BSubwayScoreData *score) {
    if (score->wifiRank < BSUBWAY_WIFI_RANK_MAX) {
        score->wifiRank++;
    }
}

void BSubwayScoreData_DecWifiRank(BSubwayScoreData *score) {
    if (score->wifiRank > 1) {
        score->wifiRank--;
    }
}

u32 BSubwayScoreData_GetWifiRank(BSubwayScoreData *score) {
    return score->wifiRank;
}

u32 BSubwayScoreData_SetWifiLoseCount(BSubwayScoreData *score, u32 op) {
    switch (op) {
    case BSUBWAY_FLAG_RESET:
        BSubwayScoreData_SetFlag(score, 0, BSUBWAY_FLAG_RESET);
        score->wifiLoseCount = 0;
        break;
    case BSUBWAY_FLAG_INC:
        if (BSubwayScoreData_SetFlag(score, 0, BSUBWAY_FLAG_GET)) {
            if (score->wifiLoseCount < 0xff) {
                score->wifiLoseCount++;
            }
        } else {
            BSubwayScoreData_SetFlag(score, 0, BSUBWAY_FLAG_SET);
            score->wifiLoseCount = 1;
        }
        break;
    }
    return score->wifiLoseCount;
}

void BSubwayScoreData_SetTeam(BSubwayScoreData *score, u32 flag, const void *team) {
    if (flag == 0) {
        sys_memcpy(team, score->team0, sizeof(score->team0));
    } else {
        sys_memcpy(team, score->team1, sizeof(score->team1));
    }
}

void BSubwayScoreData_GetTeam(BSubwayScoreData *score, u32 flag, void *team) {
    if (flag == 0) {
        sys_memcpy(score->team0, team, sizeof(score->team0));
    } else {
        sys_memcpy(score->team1, team, sizeof(score->team1));
    }
}

void BSubwayScoreData_CalcWifiScore(BSubwayScoreData *score, BSubwayPlayData *play) {
    u16 roundScore, recordScore, damageScore;
    u16 turnScore, downScore;

    roundScore = (play->round - 1) * 1000;
    turnScore = play->wifiTurn * 10;
    downScore = play->wifiDown * 20;
    if (turnScore + downScore > 950) {
        recordScore = 0;
    } else {
        recordScore = 950 - (turnScore + downScore);
    }
    if (play->wifiDamage > 970) {
        damageScore = 0;
    } else {
        damageScore = (1000 - play->wifiDamage) / 30;
    }
    score->wifiScore = roundScore + recordScore + damageScore;
}

u16 BSubwayScoreData_GetWifiScore(BSubwayScoreData *score) {
    return score->wifiScore;
}

u8 BSubwayScoreData_GetWifiScoreRounds(BSubwayScoreData *score) {
    return score->wifiScore / 1000;
}

u32 BSubwayWifiData_GetSize(void) {
    return sizeof(BSubwayWifiData);
}

void BSubwayWifiData_Init(BSubwayWifiData *data) {
    sys_memset(data, 0, sizeof(BSubwayWifiData));
}

void BSubwayWifiData_SetRoomFlag(BSubwayWifiData *data, u8 rank, u8 room, const RTCDate *date) {
    u16 id;
    u8 offset, bit;
    u8 flag = 1;

    if (room == 0 || room > BSUBWAY_WIFI_ROOM_MAX) {
        return;
    }
    if (rank == 0 || rank > BSUBWAY_WIFI_RANK_MAX) {
        return;
    }
    id = (room - 1) + (rank - 1) * BSUBWAY_WIFI_ROOM_MAX;
    offset = id / 8;
    bit = id % 8;
    flag <<= bit;
    data->roomFlags[offset] |= flag;
    data->date = (date->year << 24) | ((date->month & 0xff) << 16) | ((date->day & 0xff) << 8) | date->week;
}

static void BSubwayWifiData_ClearRoomFlags(BSubwayWifiData *data) {
    sys_memset(data->roomFlags, 0, sizeof(data->roomFlags));
    sys_memset(&data->date, 0, sizeof(data->date));
}

// Whether a date is after another, by its year, its month or its day
static BOOL BSubwayWifiData_IsDateAfter(const RTCDate *date1, const RTCDate *date2) {
    if (date1->year > date2->year) {
        return TRUE;
    }
    if (date1->month > date2->month) {
        return TRUE;
    }
    if (date1->day > date2->day) {
        return TRUE;
    }
    return FALSE;
}

// Whether the room was cleared today. Clears every room's flag when the day has changed
BOOL BSubwayWifiData_GetRoomFlag(BSubwayWifiData *data, u8 rank, u8 room, const RTCDate *today) {
    u16 id;
    u8 offset, bit;
    u8 flag = 1;
    RTCDate date;

    if (room > BSUBWAY_WIFI_ROOM_MAX || rank > BSUBWAY_WIFI_RANK_MAX) {
        return FALSE;
    }
    date.year = (data->date >> 24) & 0xff;
    date.month = (data->date >> 16) & 0xff;
    date.day = (data->date >> 8) & 0xff;
    date.week = data->date & 0xff;
    if (BSubwayWifiData_IsDateAfter(today, &date)) {
        BSubwayWifiData_ClearRoomFlags(data);
        return FALSE;
    }
    id = (room - 1) + (rank - 1) * BSUBWAY_WIFI_ROOM_MAX;
    offset = id / 8;
    bit = id % 8;
    flag <<= bit;
    if (data->roomFlags[offset] & flag) {
        return TRUE;
    }
    return FALSE;
}

u16 BSubwayWifiData_HasPlayers(BSubwayWifiData *data) {
    return data->hasPlayers;
}

u16 BSubwayWifiData_HasLeaders(BSubwayWifiData *data) {
    return data->hasLeaders;
}

void BSubwayWifiData_SetPlayers(BSubwayWifiData *data, const void *players, u8 rank, u8 room) {
    sys_memcpy(players, data->players, sizeof(data->players));
    data->playerRank = rank;
    data->playerRoom = room;
    data->hasPlayers = TRUE;
}

u16 BSubwayWifiData_GetPlayerRank(BSubwayWifiData *data) {
    return data->playerRank;
}

u16 BSubwayWifiData_GetPlayerRoom(BSubwayWifiData *data) {
    return data->playerRoom;
}

void BSubwayWifiData_GetTrainer(BSubwayWifiData *data, BSubwayTrainer *trainer, u8 round, u16 id) {
    BSubwayWifiPlayer *player = &data->players[round];

    trainer->unk00 = 0xffff;
    trainer->trainerId = player->trainerClass;
    sys_memcpy(player->name, trainer->name, sizeof(player->name));
    sys_memcpy(player->message, &trainer->message, sizeof(player->message));
    sys_memcpy(player->winWords, trainer->winWords, sizeof(player->winWords));
    sys_memcpy(player->loseWords, trainer->loseWords, sizeof(player->loseWords));
    sys_memcpy(player->pokemon, trainer->pokemon, sizeof(player->pokemon));
}

void BSubwayWifiData_SetLeaders(BSubwayWifiData *data, const void *leaders, u8 rank, u8 room) {
    sys_memcpy(leaders, data->leaders, sizeof(data->leaders));
    data->leaderRank = rank;
    data->leaderRoom = room;
    data->hasLeaders = TRUE;
}

u16 BSubwayWifiData_GetLeaderRank(BSubwayWifiData *data) {
    return data->leaderRank;
}

u16 BSubwayWifiData_GetLeaderRoom(BSubwayWifiData *data) {
    return data->leaderRoom;
}

void *BSubwayWifiData_AllocLeaders(BSubwayWifiData *data, HeapID heapId) {
    BSubwayWifiLeader *leaders = GFL_HeapAllocate(heapId, sizeof(data->leaders), TRUE, "bsubway_savedata.c", 1288);

    sys_memcpy(data->leaders, leaders, sizeof(data->leaders));
    return leaders;
}

// How many leaders a list has
int BSubwayWifiLeader_CountList(void *list) {
    BSubwayWifiLeader *leaders = list;
    int i;
    int count = 0;

    for (i = 0; i < BSUBWAY_WIFI_LEADER_MAX; i++) {
        if (leaders[i].name[0] != 0) {
            count++;
        }
    }
    return count;
}

u32 BSubwayWifiLeader_GetID(void *entry) {
    BSubwayWifiLeader *leader = entry;

    return leader->id[0] | (leader->id[1] << 8) | (leader->id[2] << 16) | (leader->id[3] << 24);
}
