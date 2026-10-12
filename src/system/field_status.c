#include "types.h"
#include "field/field_status.h"
#include "gfl/heap.h"

// The field's status that is not saved as it is: whether the game was just loaded or continued, Flash, the script
// reserved for the next map and what the field is busy with.
// The file's name is the ROM's string, from GFL_HeapAllocate's call. The names of the functions are swan's

FieldStatus *FieldStatus_Create(HeapID heapId) {
    FieldStatus *status = GFL_HeapAllocate(heapId, sizeof(FieldStatus), TRUE, "field_status.c", 51);

    status->reservedScript = 0;
    return status;
}

void FieldStatus_Free(FieldStatus *status) {
    GFL_HeapFree(status);
}

BOOL FieldStatus_GetInLinkedWorld(FieldStatus *status) {
    return status->inLinkedWorld;
}

void FieldStatus_SetInLinkedWorld(FieldStatus *status, BOOL inLinkedWorld) {
    status->inLinkedWorld = inLinkedWorld;
}

void FieldStatus_SetNewLoadFlag(FieldStatus *status, BOOL flag) {
    status->newLoad = flag;
}

u8 FieldStatus_GetNewLoadFlag(FieldStatus *status) {
    return status->newLoad;
}

void FieldStatus_SetContinueFlag(FieldStatus *status, BOOL flag) {
    status->isContinue = flag;
}

BOOL FieldStatus_CheckContinueFlag(FieldStatus *status) {
    return status->isContinue;
}

void FieldStatus_SetFlashUsed(FieldStatus *status, BOOL value) {
    status->flashUsed = value;
}

BOOL FieldStatus_CheckFlashUsed(FieldStatus *status) {
    return status->flashUsed;
}

void FieldStatus_SetFlashPerms(FieldStatus *status, u32 flags) {
    status->flashPerms = flags;
}

u16 FieldStatus_GetFlashPerms(FieldStatus *status) {
    return status->flashPerms;
}

void FieldStatus_ReserveScript(FieldStatus *status, u16 scriptId) {
    status->reservedScript = scriptId;
}

u16 FieldStatus_GetReserveScript(FieldStatus *status) {
    return status->reservedScript;
}

// Returns the flag it replaces
u32 FieldStatus_SetBusyFlag(FieldStatus *status, u32 flag) {
    u8 old = status->busyFlag;

    status->busyFlag = flag;
    return old;
}

u32 FieldStatus_GetBusyFlag(FieldStatus *status) {
    return status->busyFlag;
}
