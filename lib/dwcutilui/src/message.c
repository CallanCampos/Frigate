#include "message.h"
#include "archive.h"
#include "deque.h"
#include "dwc_util_heap.h"

// The INF1 block, which follows the file's 0x20-byte header, and the DAT1 block after it, whose text starts 8 bytes in
typedef struct DWCUtilMsgInfo {
    u32 magic;
    u32 size;
    u16 count;
    u16 entrySize;
    u32 unk_0c;
    u32 offsets[1];
} DWCUtilMsgInfo;

typedef struct DWCUtilMsgFile {
    u8 header[0x20];
    DWCUtilMsgInfo info;
} DWCUtilMsgFile;

typedef struct DWCUtilMsgWork {
    DWCUtilMsg msgs[8];
    DWCUtilStack *freeMsgs;
} DWCUtilMsgWork;

static DWCUtilMsgWork *sMsgWork;

void DWCi_MSGlInit(void) {
    sMsgWork = DWCi_HEAPlAllocEx(sizeof(DWCUtilMsgWork), 4);
    sMsgWork->freeMsgs = DWCi_QUElFormEx(8, (u8 *)sMsgWork->msgs, sizeof(DWCUtilMsg));
}

void DWCi_MSGlEnd(void) {
    DWCi_HEAPlFree_(&sMsgWork);
}

DWCUtilMsg *DWCi_MSGlRead(const char *path) {
    u32 size;
    DWCUtilMsgFile *file;
    DWCUtilMsgInfo *info;
    DWCUtilMsg *msg = DWCi_QUElPopFront(sMsgWork->freeMsgs);

    file = DWCUtilRomResource_LoadFile(path, &size, 4);
    msg->file = file;
    info = &file->info;
    msg->offsets = info->offsets;
    msg->text = (u8 *)info + info->size + 8;
    return msg;
}

void DWCi_MSGlRelease(DWCUtilMsg *msg) {
    DWCi_ARClRelease(msg->file);
    DWCi_QUElPushBack(sMsgWork->freeMsgs, msg);
}

u16 *DWCi_MSGlGet(DWCUtilMsg *msg, int index) {
    return (u16 *)(msg->text + msg->offsets[(u16)index]);
}

u16 *DWCi_MSGlGetEx(DWCUtilMsg *msg, int index, int slot, int value) {
    u16 *str = (u16 *)(msg->text + msg->offsets[(u16)index]);

    if (slot >= 0) {
        str[slot] = value + '0';
    }
    return str;
}
