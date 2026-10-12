#ifndef POKEBW2_FIELD_FIELD_STATUS_H
#define POKEBW2_FIELD_FIELD_STATUS_H

// Names, layouts and constants from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

#define FLD_STATUS_BUSY_NONE 0x0
#define FLD_STATUS_BUSY_BATTLE 0x1
#define FLD_STATUS_BUSY_LOADING 0x2

#define FLD_FLASH_NONE 0x0
#define FLD_FLASH_ALLOW 0x1
#define FLD_FLASH_ACTIVE 0x2

struct FieldStatus {
    BOOL inLinkedWorld;
    u8 newLoad;
    u8 isContinue;
    u8 flashUsed;
    u16 flashPerms;
    u8 unk0A[7];
    u8 busyFlag;
    // The script to run on the next map, 0 for none
    u16 reservedScript;
}; // 0x14

FieldStatus *FieldStatus_Create(HeapID heapId);
void FieldStatus_Free(FieldStatus *status);
BOOL FieldStatus_CheckFlashUsed(FieldStatus *status);
u16 FieldStatus_GetFlashPerms(FieldStatus *status);
BOOL FieldStatus_CheckContinueFlag(FieldStatus *status);
void FieldStatus_ReserveScript(FieldStatus *status, u16 scriptId);
u16 FieldStatus_GetReserveScript(FieldStatus *status);
// Returns the flag it replaces
u32 FieldStatus_SetBusyFlag(FieldStatus *status, u32 flag);
u32 FieldStatus_GetBusyFlag(FieldStatus *status);
void FieldStatus_SetContinueFlag(FieldStatus *status, BOOL flag);
void FieldStatus_SetFlashPerms(FieldStatus *status, u32 flags);
void FieldStatus_SetFlashUsed(FieldStatus *status, BOOL value);
void FieldStatus_SetInLinkedWorld(FieldStatus *status, BOOL inLinkedWorld);
void FieldStatus_SetNewLoadFlag(FieldStatus *status, BOOL flag);
u8 FieldStatus_GetNewLoadFlag(FieldStatus *status);
BOOL FieldStatus_GetInLinkedWorld(FieldStatus *status);

#endif // POKEBW2_FIELD_FIELD_STATUS_H
