#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/fest_mission_data.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
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

void func_ov027_02170b00(FestivalText *text) {
    GFL_MsgDataFree(text->message);
    GFL_ArcToolFree(text->archive);
    GFL_HeapFree(text);
}

void *func_ov027_02170b18(ArcTool *arc, HeapID heapId) {
    return GFL_ArcToolReadHeapNew(arc, 0, heapId);
}

void func_ov027_02170b24(ArcTool *arc, u8 index, FestMissionData *dest) {
    GFL_ArcToolReadRange(arc, 0, index * sizeof(dest->header), sizeof(dest->header), &dest->header);
    GFL_ArcToolReadRange(arc, 1, index * sizeof(dest->params), sizeof(dest->params), &dest->params);
}

FestMissionData *func_ov027_02170b50(ArcTool *arc, HeapID heapId) {
    s32 i;
    FestMissionData *missions;

    missions = GFL_HeapAllocate(heapId, sizeof(FestMissionData) * FEST_MISSION_COUNT, TRUE, "fest_mission_data.c", 0x91);
    for (i = 0; i < FEST_MISSION_COUNT; i++) {
        func_ov027_02170b24(arc, i, &missions[i]);
    }
    return missions;
}

FestMissionData *func_ov027_02170b8c(FestivalText *text, HeapID heapId) {
    return func_ov027_02170b50(text->archive, heapId);
}

void func_ov027_02170b98(ArcTool *arc, u8 index, u32 level, FestMission *mission) {
    FestMissionData data;
    FestMissionParams *params;

    sys_memset(mission, 0, sizeof(FestMission));
    func_ov027_02170b24(arc, index, &data);
    params = &data.params;
    mission->index = index;
    mission->level = level;
    mission->target = params->target;
    mission->unk0_20 = params->unk0C;
    mission->unk0_30 = 0;
    mission->unk0_31 = MISSION_VERSION_BIT;
    mission->unk4_0 = params->unk0E;
    mission->unk4_10 = params->levels[level].unk3;
    mission->count = params->levels[level].count;
    mission->unk4_31 = params->unk00;
    mission->unk8_0 = params->unk14;
    mission->unk8_18 = params->unk12;
    mission->unk8_30 = params->unk03;
    mission->unkC_0 = 0;
    mission->unkC_10 = params->levels[level].unk0;
    mission->unkC_22 = params->unk08;
    mission->unk10_0 = params->unk04;
    mission->unk10_6 = params->unk05;
    mission->unk10_9 = params->unk02;
    mission->messageId = params->messageId;
    mission->resultMessageId = params->resultMessageId;
    mission->header = data.header;
}

void func_ov027_02170cf8(FestivalText *text, u8 index, u32 level, FestMission *mission) {
    func_ov027_02170b98(text->archive, index, level, mission);
}

void func_ov027_02170d04(FestivalText *text, StrBuf *dest, const FestMission *mission, HeapID heapId) {
    WordSet *wordSet = GFL_WordSetSystemCreate(2, 0x49, HEAPID_TAIL(heapId));
    StrBuf *message = GFL_StrBufCreate(0x25, HEAPID_TAIL(heapId));

    func_ov027_02170e1c(wordSet, message, 0, data_ov027_021711c0[mission->header.kind], mission->target);
    WordSetNumber(wordSet, 1, mission->count, 3, 0, 1);
    GFL_MsgDataLoadStrbuf(text->message, mission->messageId, message);
    GFL_WordSetFormatStrbuf(wordSet, dest, message);
    GFL_StrBufFree(message);
    GFL_WordSetSystemFree(wordSet);
}

void func_ov027_02170d90(FestivalText *text, StrBuf *dest, const FestMission *mission, HeapID heapId) {
    WordSet *wordSet = GFL_WordSetSystemCreate(2, 0x49, HEAPID_TAIL(heapId));
    StrBuf *message = GFL_StrBufCreate(0xa1, HEAPID_TAIL(heapId));

    func_ov027_02170e1c(wordSet, message, 0, data_ov027_021711c0[mission->header.kind], mission->target);
    WordSetNumber(wordSet, 1, mission->count, 5, 0, 1);
    GFL_MsgDataLoadStrbuf(text->message, mission->resultMessageId, message);
    GFL_WordSetFormatStrbuf(wordSet, dest, message);
    GFL_StrBufFree(message);
    GFL_WordSetSystemFree(wordSet);
}

void func_ov027_02170e1c(WordSet *wordSet, StrBuf *unused, u32 index, u32 kind, u32 target) {
    switch (kind) {
    case 1:
        WordSet_LoadSpeciesName(wordSet, index, (u16)target);
        break;
    case 2:
        loadItemNameToStrbuf(wordSet, index, target);
        break;
    }
}
