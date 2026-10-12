#include "types.h"
#include "app/unova_link.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/player_info.h"

// Reads what the Memory Link needs from a Black or White save: its player, and the event flags, event work and other
// data that unlock the memories. Named after the ROM's assertion string

// The blocks of a Black or White save that are read
#define WB_SAVE_BLOCK_PLAYER 27
#define WB_SAVE_BLOCK_UNK42 42
#define WB_SAVE_BLOCK_EVENT_WORK 45
#define WB_SAVE_BLOCK_UNK52 52
#define WB_SAVE_BLOCK_UNK66 66

// Where the event flags are in the event work's block, and the first variable
#define WB_EVENT_WORK_FLAGS 0x27c
#define WB_EVENT_WORK_VARS_START 0x4000

// An event flag that sets a bit of the memories' flags when it has the value
typedef struct {
    u32 flag;
    u32 bit;
    u32 value;
} WBEventFlagBit;

// A bit of a word in block 52 that sets a bit of the memories' flags
typedef struct {
    u32 src;
    u32 bit;
} WBUnk52Bit;

// An event work variable and where it goes
typedef struct {
    u32 var;
    u32 index;
} WBEventVar;

static void WBSaveConvert_ReadPlayer(void *save, WBSavePlayer *player);
static void WBSaveConvert_ReadMemories(void *save, WBSaveMemories *memories);

static const WBEventVar sEventVars[3] = {
    { 0x4030, 0 },
    { 0x408e, 1 },
    { 0x40bc, 2 },
};

static const WBUnk52Bit sUnk52Bits[8] = {
    { 0, 8 },
    { 1, 9 },
    { 2, 10 },
    { 3, 11 },
    { 4, 12 },
    { 5, 13 },
    { 6, 14 },
    { 7, 15 },
};

static const WBEventFlagBit sEventFlagBits[8] = {
    { 2400, 0, TRUE },
    { 2401, 1, TRUE },
    { 2427, 2, TRUE },
    { 681, 3, FALSE },
    { 682, 4, FALSE },
    { 683, 5, FALSE },
    { 684, 6, FALSE },
    { 685, 7, FALSE },
};

WBSaveData *WBSaveConvert_Create(void *save, HeapID heapId) {
    WBSaveData *data = GFL_HeapAllocate(heapId, sizeof(WBSaveData), TRUE, "wb_save_convert.c", 90);

    WBSaveConvert_ReadPlayer(save, &data->player);
    WBSaveConvert_ReadMemories(save, &data->memories);
    return data;
}

static void WBSaveConvert_ReadPlayer(void *save, WBSavePlayer *player) {
    PlayerInfo *info;

    sys_memset(player, 0, sizeof(WBSavePlayer));
    info = (PlayerInfo *)((u8 *)WBSaveBlock_Get(save, WB_SAVE_BLOCK_PLAYER) + 4);
    player->id = info->id;
    sys_memcpy(info->name, player->name, sizeof(player->name));
    player->gender = info->gender;
    player->unk15 = info->version;
}

static void WBSaveConvert_ReadMemories(void *save, WBSaveMemories *memories) {
    u8 *eventWork;
    u32 *unk52;
    u8 *unk42;
    u32 i;

    sys_memset(memories, 0, sizeof(WBSaveMemories));
    eventWork = WBSaveBlock_Get(save, WB_SAVE_BLOCK_EVENT_WORK);
    for (i = 0; i < NELEMS(sEventFlagBits); i++) {
        const WBEventFlagBit *entry = &sEventFlagBits[i];
        BOOL set = eventWork[WB_EVENT_WORK_FLAGS + sEventFlagBits[i].flag / 8] & (1 << (sEventFlagBits[i].flag % 8))
                       ? TRUE
                       : FALSE;

        if (entry->value == set) {
            memories->flags[entry->bit / 8] |= 1 << (entry->bit % 8);
        } else {
            memories->flags[entry->bit / 8] &= ~(1 << (entry->bit % 8));
        }
    }
    unk52 = WBSaveBlock_Get(save, WB_SAVE_BLOCK_UNK52);
    for (i = 0; i < NELEMS(sUnk52Bits); i++) {
        if (unk52[1] & (1 << sUnk52Bits[i].src)) {
            memories->flags[sUnk52Bits[i].bit / 8] |= 1 << (sUnk52Bits[i].bit % 8);
        } else {
            memories->flags[sUnk52Bits[i].bit / 8] &= ~(1 << (sUnk52Bits[i].bit % 8));
        }
    }
    for (i = 0; i < NELEMS(sEventVars); i++) {
        memories->vars[sEventVars[i].index] = ((u16 *)eventWork)[sEventVars[i].var - WB_EVENT_WORK_VARS_START];
    }
    unk42 = WBSaveBlock_Get(save, WB_SAVE_BLOCK_UNK42);
    sys_memcpy(unk42 + 0x258, memories->props, sizeof(memories->props));
    memories->unk28 = *(u16 *)(unk42 + 0x256);
    memories->unk2A = *(u16 *)(unk42 + 0x252);
    sys_memcpy(WBSaveBlock_Get(save, WB_SAVE_BLOCK_UNK66), memories->unk2C, sizeof(memories->unk2C));
}
