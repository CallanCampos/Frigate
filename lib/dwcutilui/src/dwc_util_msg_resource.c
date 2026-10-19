#include "dwc_util_msg_resource.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_rom_resource.h"

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

void func_ov182_021ae084(void) {
    sMsgWork = func_ov182_021bc4d8(sizeof(DWCUtilMsgWork), 4);
    sMsgWork->freeMsgs = func_ov182_021bb9c8(8, (u8 *)sMsgWork->msgs, sizeof(DWCUtilMsg));
}

void func_ov182_021ae0a8(void) {
    func_ov182_021bc4f0(&sMsgWork);
}

DWCUtilMsg *func_ov182_021ae0b8(const char *path) {
    u32 size;
    DWCUtilMsgFile *file;
    DWCUtilMsgInfo *info;
    DWCUtilMsg *msg = func_ov182_021bba38(sMsgWork->freeMsgs);

    file = func_ov182_021bb37c(path, &size, 4);
    msg->file = file;
    info = &file->info;
    msg->offsets = info->offsets;
    msg->text = (u8 *)info + info->size + 8;
    return msg;
}

void func_ov182_021ae0ec(DWCUtilMsg *msg) {
    func_ov182_021bb420(msg->file);
    func_ov182_021bba00(sMsgWork->freeMsgs, msg);
}

u16 *func_ov182_021ae108(DWCUtilMsg *msg, int index) {
    return (u16 *)(msg->text + msg->offsets[(u16)index]);
}

u16 *func_ov182_021ae118(DWCUtilMsg *msg, int index, int slot, int value) {
    u16 *str = (u16 *)(msg->text + msg->offsets[(u16)index]);

    if (slot >= 0) {
        str[slot] = value + '0';
    }
    return str;
}
