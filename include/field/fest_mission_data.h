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

// A mission at a level, packed
typedef struct {
    u32 index : 7;
    u32 level : 3;
    // The species or item the mission is about
    u32 target : 10;
    u32 unk0_20 : 10;
    u32 unk0_30 : 1;
    u32 unk0_31 : 1;
    u32 unk4_0 : 10;
    u32 unk4_10 : 7;
    // How many of the target the mission wants
    u32 count : 14;
    u32 unk4_31 : 1;
    u32 unk8_0 : 18;
    u32 unk8_18 : 12;
    u32 unk8_30 : 2;
    u32 unkC_0 : 10;
    u32 unkC_10 : 12;
    u32 unkC_22 : 10;
    u16 unk10_0 : 6;
    u16 unk10_6 : 3;
    u16 unk10_9 : 2;
    u16 unk10_11 : 5;
    u8 messageId;
    u8 resultMessageId;
    FestMissionHeader header;
    u32 unk18;
    // The seed of the mission's random choices (FestMissionConfig's seed)
    u32 seed;
    u8 unk20[0xc];
} FestMission;

// What the kinds of target are: 1 for a species and 2 for an item
extern const u8 data_ov027_021711c0[8];

#define FEST_MISSION_COUNT 55

FestivalText *getTextFileForFestMissions(HeapID heapId);
void FestivalText_Free(FestivalText *text);
void *FestMissionData_LoadHeaders(ArcTool *arc, HeapID heapId);
void FestMissionData_Load(ArcTool *arc, u8 index, FestMissionData *dest);
// All the missions
FestMissionData *FestMissionData_LoadAll(ArcTool *arc, HeapID heapId);
FestMissionData *FestivalText_LoadAllMissions(FestivalText *text, HeapID heapId);
void FestMission_Load(ArcTool *arc, u8 index, u32 level, FestMission *mission);
void FestivalText_LoadMission(FestivalText *text, u8 index, u32 level, FestMission *mission);
// The mission's message and result message, with its target and count
void FestMission_GetMessage(FestivalText *text, StrBuf *dest, const FestMission *mission, HeapID heapId);
void FestMission_GetResultMessage(FestivalText *text, StrBuf *dest, const FestMission *mission, HeapID heapId);
void FestMission_SetTargetWord(WordSet *wordSet, StrBuf *unused, u32 index, u32 kind, u32 target);

#endif // POKEBW2_FIELD_FEST_MISSION_DATA_H
