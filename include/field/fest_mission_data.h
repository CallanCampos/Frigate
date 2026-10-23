#ifndef POKEBW2_FIELD_FEST_MISSION_DATA_H
#define POKEBW2_FIELD_FEST_MISSION_DATA_H

#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The Funfest missions' data and text, overlay 27's fest_mission_data.c

struct FestivalText {
    ArcTool *archive;
    MsgData *message;
};

// A mission's entry in file 0 of the missions' archive
typedef struct {
    // The kind of the mission's target, an index into data_ov027_021711c0
    u8 kind;
    u8 unk01;
    u16 unk02;
} FestMissionHeader;

// The kinds of mission that run on the field (fes_gimmick.c), FestMissionHeader's kind
enum {
    MISSION_FLAGS = 2,
    MISSION_HOLLOW,
    MISSION_SPARKLE,
    MISSION_PEOPLE,
};

// A mission's entry in file 1
typedef struct {
    u8 unk00;
    u8 unk01;
    u8 unk02;
    u8 unk03;
    u8 unk04;
    u8 unk05;
    u8 messageId;
    u8 resultMessageId;
    u16 unk08;
    u16 target;
    u16 unk0C;
    u16 unk0E;
    u16 unk10;
    u16 unk12;
    u32 unk14;
    // By the mission's level
    struct {
        u16 unk0;
        u8 count;
        u8 unk3;
    } levels[4];
} FestMissionParams;

// A mission, from files 0 and 1 of the missions' archive
typedef struct {
    FestMissionHeader header;
    FestMissionParams params;
} FestMissionData;

// What the kinds of target are: 1 for a species and 2 for an item
extern const u8 data_ov027_021711c0[8];

#define FEST_MISSION_COUNT 55

FestivalText *getTextFileForFestMissions(HeapID heapId);
void FestivalText_Free(FestivalText *text);
FestMissionHeader *FestMissionData_LoadHeaders(ArcTool *arc, HeapID heapId);
void FestMissionData_Load(ArcTool *arc, u8 index, FestMissionData *dest);
// All the missions
FestMissionData *FestMissionData_LoadAll(ArcTool *arc, HeapID heapId);
FestMissionData *FestivalText_LoadAllMissions(FestivalText *text, HeapID heapId);
// A mission at a level, as the festival keeps it (FestMissionConfig, system/fest_mission.h)
void FestMission_Load(ArcTool *arc, u8 index, u32 level, FestMissionConfig *mission);
void FestivalText_LoadMission(FestivalText *text, u8 index, u32 level, FestMissionConfig *mission);
// The mission's message and result message, with its target and count
void FestMission_GetMessage(FestivalText *text, StrBuf *dest, const FestMissionConfig *mission, HeapID heapId);
void FestMission_GetResultMessage(FestivalText *text, StrBuf *dest, const FestMissionConfig *mission, HeapID heapId);
void FestMission_SetTargetWord(WordSet *wordSet, StrBuf *unused, u32 index, u32 kind, u32 target);

#endif // POKEBW2_FIELD_FEST_MISSION_DATA_H
