#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/fest_mission_data.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/fest_mission.h"
#include "system/wordset.h"

// Set in Black 2 and clear in White 2
#ifdef BLACK2
#define MISSION_VERSION_BIT 1
#else
#define MISSION_VERSION_BIT 0
#endif

const u8 data_ov027_021711c0[8] = { 0, 1, 1, 3, 2, 2, 0, 0 };

FestivalText *getTextFileForFestMissions(HeapID heapId) {
    FestivalText *text = GFL_HeapAllocate(heapId, sizeof(FestivalText), TRUE, "fest_mission_data.c", 0x46);
    text->archive = GFL_ArcSysCreateFileHandle(0x121, heapId);
    text->message = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_GET_TEXT_FILE_FOR_FEST_MISSIONS, heapId);
    return text;
}

void FestivalText_Free(FestivalText *text) {
    GFL_MsgDataFree(text->message);
    GFL_ArcToolFree(text->archive);
    GFL_HeapFree(text);
}

FestMissionHeader *FestMissionData_LoadHeaders(ArcTool *arc, HeapID heapId) {
    return GFL_ArcToolReadHeapNew(arc, 0, heapId);
}

void FestMissionData_Load(ArcTool *arc, u8 index, FestMissionData *dest) {
    GFL_ArcToolReadRange(arc, 0, index * sizeof(dest->header), sizeof(dest->header), &dest->header);
    GFL_ArcToolReadRange(arc, 1, index * sizeof(dest->params), sizeof(dest->params), &dest->params);
}

FestMissionData *FestMissionData_LoadAll(ArcTool *arc, HeapID heapId) {
    s32 i;
    FestMissionData *missions;

    missions =
        GFL_HeapAllocate(heapId, sizeof(FestMissionData) * FEST_MISSION_COUNT, TRUE, "fest_mission_data.c", 0x91);
    for (i = 0; i < FEST_MISSION_COUNT; i++) {
        FestMissionData_Load(arc, i, &missions[i]);
    }
    return missions;
}

FestMissionData *FestivalText_LoadAllMissions(FestivalText *text, HeapID heapId) {
    return FestMissionData_LoadAll(text->archive, heapId);
}

void FestMission_Load(ArcTool *arc, u8 index, u32 level, FestMissionConfig *mission) {
    FestMissionData data;
    FestMissionParams *params;

    MI_CpuFill8(mission, 0, sizeof(FestMissionConfig));
    FestMissionData_Load(arc, index, &data);
    params = &data.params;
    mission->info.index = index;
    mission->info.level = level;
    mission->info.target = params->target;
    mission->info.unk0_20 = params->unk0C;
    mission->info.unk0_30 = 0;
    mission->info.unk0_31 = MISSION_VERSION_BIT;
    mission->info.unk4_0 = params->unk0E;
    mission->info.unk4_10 = params->levels[level].unk3;
    mission->info.count = params->levels[level].count;
    mission->info.unk4_31 = params->unk00;
    mission->info.unk8_0 = params->unk14;
    mission->info.requiredFlag = params->unk12;
    mission->info.unk8_30 = params->unk03;
    mission->info.unkC_0 = 0;
    mission->info.seconds = params->levels[level].unk0;
    mission->info.unkC_22 = params->unk08;
    mission->info.unk10_0 = params->unk04;
    mission->info.targetKind = params->unk05;
    mission->info.extraSecondsIndex = params->unk02;
    mission->info.messageId = params->messageId;
    mission->info.resultMessageId = params->resultMessageId;
    mission->header = data.header;
}

void FestivalText_LoadMission(FestivalText *text, u8 index, u32 level, FestMissionConfig *mission) {
    FestMission_Load(text->archive, index, level, mission);
}

void FestMission_GetMessage(FestivalText *text, StrBuf *dest, const FestMissionConfig *mission, HeapID heapId) {
    WordSet *wordSet = GFL_WordSetSystemCreate(2, 0x49, HEAPID_TAIL(heapId));
    StrBuf *message = GFL_StrBufCreate(0x25, HEAPID_TAIL(heapId));

    FestMission_SetTargetWord(wordSet, message, 0, data_ov027_021711c0[mission->header.kind], mission->info.target);
    WordSetNumber(wordSet, 1, mission->info.count, 3, 0, 1);
    GFL_MsgDataLoadStrbuf(text->message, mission->info.messageId, message);
    GFL_WordSetFormatStrbuf(wordSet, dest, message);
    GFL_StrBufFree(message);
    GFL_WordSetSystemFree(wordSet);
}

void FestMission_GetResultMessage(FestivalText *text, StrBuf *dest, const FestMissionConfig *mission, HeapID heapId) {
    WordSet *wordSet = GFL_WordSetSystemCreate(2, 0x49, HEAPID_TAIL(heapId));
    StrBuf *message = GFL_StrBufCreate(0xa1, HEAPID_TAIL(heapId));

    FestMission_SetTargetWord(wordSet, message, 0, data_ov027_021711c0[mission->header.kind], mission->info.target);
    WordSetNumber(wordSet, 1, mission->info.count, 5, 0, 1);
    GFL_MsgDataLoadStrbuf(text->message, mission->info.resultMessageId, message);
    GFL_WordSetFormatStrbuf(wordSet, dest, message);
    GFL_StrBufFree(message);
    GFL_WordSetSystemFree(wordSet);
}

void FestMission_SetTargetWord(WordSet *wordSet, StrBuf *unused, u32 index, u32 kind, u32 target) {
    switch (kind) {
    case 1:
        WordSet_LoadSpeciesName(wordSet, index, (u16)target);
        break;
    case 2:
        loadItemNameToStrbuf(wordSet, index, target);
        break;
    }
}
