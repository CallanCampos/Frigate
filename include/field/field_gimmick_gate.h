#ifndef POKEBW2_FIELD_FIELD_GIMMICK_GATE_H
#define POKEBW2_FIELD_FIELD_GIMMICK_GATE_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the gates between areas (overlay 104), with the electric news boards of gimmick_obj_elboard.c

// A gate's entry in archive 0xac, 0x7c bytes
struct GimmickGateZoneData {
    u32 zoneId;
    u32 version;
    u32 x;
    u32 y;
    u32 z;
    u32 direction;
    u32 messageIds[19];
    u32 fallbackZones[4];
    u32 anmIndex;
    u32 enabled;
};

// A news entry in archive 0xa4, shown when its flag is set and the gate's zone is one of zones
struct GimmickGateBoardEntry {
    u32 unk00;
    u32 flagId;
    u32 unk08;
    u32 unk0c;
    u32 type;
    u32 zones[4];
};

// The gimmick's entry points, from the gimmick table in overlay 36
void GimmickGate_Setup(Field *field);
void GimmickGate_Delete(Field *field);
void GimmickGate_Update(Field *field);
u32 GimmickGate_GetDirection(Field *field);

// Script command 0x17a: plays a sound and runs the news board's event for as long as the script says
BOOL Scrcmd_FieldGimmickGate_GimmickGateViewBoard(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_FIELD_GIMMICK_GATE_H
