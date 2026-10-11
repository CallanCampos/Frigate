#include "types.h"
#include "save/player_info.h"
#include "constants/version.h"
#include "field/unity_tower.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/str_tool.h"

// The player's name, IDs, gender, country and region (the ROM's string for the file, "mystatus.c"). The names of the
// fields are guessed; the accessors' names are swan's where it gives one.

u32 PlayerInfo_GetSize(void) {
    return sizeof(PlayerInfo);
}

PlayerInfo *func_02008b0c(u32 heapId) {
    PlayerInfo *info = GFL_HeapAllocate(heapId, sizeof(PlayerInfo), FALSE, "mystatus.c", 52);
    func_02008b40(info);
    return info;
}

void func_02008b34(const PlayerInfo *src, PlayerInfo *dest) {
    sys_memcpy(src, dest, sizeof(PlayerInfo));
}

void func_02008b40(PlayerInfo *info) {
    sys_memset(info, 0, sizeof(PlayerInfo));
    info->region = 2;
    func_02008c00(info, GAME_VERSION);
}

BOOL func_02008b5c(const PlayerInfo *info) {
    int i;

    for (i = 0; i < 8; i++) {
        if (info->name[i] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

void copyTrainerName(PlayerInfo *info, const u16 *name) {
    wcharsncpy(name, info->name, 8);
}

void copyTrainerNameFromStrbuf(PlayerInfo *info, const StrBuf *name) {
    GFL_StrBufStoreString(name, info->name, 8);
}

u16 *GetPlayerName(PlayerInfo *info) {
    return info->name;
}

void copyName(const u16 *src, u16 *dest, u32 n) {
    wcharsncpy(src, dest, n);
}

void textCopy(const u16 *src, StrBuf *dest) {
    GFL_StrBufLoadString(dest, src);
}

StrBuf *copyTrainerNameToNewStrbuf(const u16 *name, u32 heapId) {
    StrBuf *buf = GFL_StrBufCreate(8, heapId);
    textCopy(name, buf);
    return buf;
}

void setIDAsUInt(PlayerInfo *info, u32 id) {
    info->id = id;
}

u32 getIDAsUInt(PlayerInfo *info) {
    return info->id;
}

u16 getTrainerID(PlayerInfo *info) {
    return info->id;
}

s32 func_02008bdc(PlayerInfo *info) {
    return info->profileId;
}

void func_02008be0(PlayerInfo *info, s32 profileId) {
    if (info->profileId == 0) {
        info->profileId = profileId;
    }
}

void setTrainerGender(PlayerInfo *info, u32 gender) {
    info->gender = gender;
}

u32 getTrainerGender(PlayerInfo *info) {
    return info->gender;
}

u8 func_02008bf4(PlayerInfo *info) {
    return info->unk1C;
}

void func_02008bf8(PlayerInfo *info, u8 value) {
    info->unk1C = value;
}

u8 func_02008bfc(PlayerInfo *info) {
    return info->version;
}

void func_02008c00(PlayerInfo *info, u8 value) {
    info->version = value;
}

u8 TrainerInfo_GetRegion(PlayerInfo *info) {
    return info->region;
}

void func_02008c08(PlayerInfo *info, u8 value) {
    info->region = value;
}

u8 UnityTowerVisitor_GetCountry(PlayerInfo *info) {
    return info->country;
}

u8 UnityTowerVisitor_GetProvince(PlayerInfo *info) {
    return info->province;
}

void func_02008c14(PlayerInfo *info, u8 country, u8 province) {
    info->country = country;
    info->province = province;
}
