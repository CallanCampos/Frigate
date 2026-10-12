#include "types.h"
#include "app/name_entry.h"
#include "field/game_beacon_set.h"
#include "gfl/str.h"
#include "save/high_link.h"
#include "save/save_control.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/str_tool.h"

static void func_ov012_0215f9b0(u16 value);
static void func_ov012_0215f9d8(GameBeacon *info, u16 value);
static void func_ov012_0215f9e8(u16 value);
static void func_ov012_0215fa10(GameBeacon *info, u16 value);
static void func_ov012_0215fa20(u16 value);
static void func_ov012_0215fa48(GameBeacon *info, u16 value);
static void func_ov012_0215fa58(u16 value);
static void func_ov012_0215fa80(GameBeacon *info, u16 value);
static void func_ov012_0215fae0(GameBeacon *info, u16 itemId);
static void func_ov012_0215fb40(GameBeacon *info, u16 itemId);
static void func_ov012_0215fba0(GameBeacon *info, u16 itemId);
static void func_ov012_0215fc00(GameBeacon *info, u16 itemId);
static void func_ov012_0215fc60(GameBeacon *info, u16 itemId);
static void func_ov012_0215fcc0(GameBeacon *info, u16 itemId);
static void func_ov012_0215fd30(GameBeacon *info, u32 passPower);
static void func_ov012_0215fdac(GameBeacon *info, u16 value);
static void func_ov012_0215fde0(GameBeacon *info);
static void func_ov012_0215fe2c(GameBeacon *info, u32 value);
static void func_ov012_0215fe60(GameBeacon *info);
static void func_ov012_0215feb0(GameBeacon *info);
static void func_ov012_0215ff04(GameBeacon *info, const StrBuf *name);
static void func_ov012_0215ff68(GameBeacon *info, const StrBuf *name);
static void func_ov012_0215ffcc(GameBeacon *info, const StrBuf *name);
static void func_ov012_02160010(GameBeacon *info, const StrBuf *name);
static void func_ov012_02160070(GameBeacon *info, u16 value);
static void func_ov012_021600c4(GameBeacon *info);
static void func_ov012_02160114(GameBeacon *info, u16 species);
static void func_ov012_02160168(GameBeacon *info);
static void func_ov012_02160198(GameBeacon *info);
static void func_ov012_021601cc(GameBeacon *info, u32 value);
static void func_ov012_02160200(GameBeacon *info);
static void func_ov012_02160234(GameBeacon *info);
static void func_ov012_02160268(GameBeacon *info, u8 value);
static void func_ov012_021602c0(GameBeacon *info);
static void func_ov012_021602f0(GameBeacon *info);
static void func_ov012_02160324(GameBeacon *info, const StrBuf *name);
static void func_ov012_02160380(GameBeacon *info, u32 passPower);
static void func_ov012_021603bc(GameBeacon *info, GameData *gameData, u16 value);
static void func_ov012_02160438(GameBeacon *info, const StrBuf *name, u8 value);
static void func_ov012_021604b4(GameBeacon *info, u8 value, u16 type);
static void func_ov012_0216055c(GameBeacon *info, u8 value, u8 extra, u32 type);
static void func_ov012_02160598(GameBeacon *info);
static void func_ov012_02160604(GameBeacon *info, u8 extra, u16 value, u16 type);

static const u8 data_ov012_0216d718[5] = {10, 30, 50, 100, 200};

void func_ov012_0215f958(u16 value) {
    if (GameBeacon_IsSpecialSpecies(value)) {
        func_ov012_0215fa20(value);
    } else if (func_0202cfac(4, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x3e, value);
    } else {
        func_ov012_0215f9b0(value);
    }
}

void func_ov012_0215f994(u16 value) {
    if (GameBeacon_IsSpecialSpecies(value)) {
        func_ov012_0215fa58(value);
    } else {
        func_ov012_0215f9e8(value);
    }
}

static void func_ov012_0215f9b0(u16 value) {
    if (GameBeaconSys_CanSendType(2)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215f9d8(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_0215f9d8(GameBeacon *info, u16 value) {
    info->type = 2;
    info->arg.value = value;
    func_0202d4c8(info, value);
}

static void func_ov012_0215f9e8(u16 value) {
    if (GameBeaconSys_CanSendType(3)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215fa10(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_0215fa10(GameBeacon *info, u16 value) {
    info->type = 3;
    info->arg.value = value;
    GameBeacon_ClearRecent(info);
}

static void func_ov012_0215fa20(u16 value) {
    if (GameBeaconSys_CanSendType(4)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215fa48(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_0215fa48(GameBeacon *info, u16 value) {
    info->type = 4;
    info->arg.value = value;
    func_0202d4e0(info, value);
}

static void func_ov012_0215fa58(u16 value) {
    if (GameBeaconSys_CanSendType(5)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215fa80(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_0215fa80(GameBeacon *info, u16 value) {
    info->type = 5;
    info->arg.value = value;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fa90(u16 itemId) {
    if (itemId < 0x32e) {
        if (func_0202cfac(8, itemId)) {
            GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x42, itemId);
        } else if (GameBeaconSys_CanSendType(6)) {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
            func_ov012_0215fae0(&GameBeaconSys->mine.beacon, itemId);
        }
    }
}

static void func_ov012_0215fae0(GameBeacon *info, u16 itemId) {
    info->type = 6;
    info->payload.value = itemId;
    func_0202d4fc(info, itemId);
}

void func_ov012_0215faf0(u16 itemId) {
    if (itemId < 0x32e) {
        if (func_0202cfac(9, itemId)) {
            GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x43, itemId);
        } else if (GameBeaconSys_CanSendType(7)) {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
            func_ov012_0215fb40(&GameBeaconSys->mine.beacon, itemId);
        }
    }
}

static void func_ov012_0215fb40(GameBeacon *info, u16 itemId) {
    info->type = 7;
    info->payload.value = itemId;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fb50(u16 itemId) {
    if (itemId < 0x32e) {
        if (func_0202cfac(8, itemId)) {
            GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x42, itemId);
        } else if (GameBeaconSys_CanSendType(8)) {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
            func_ov012_0215fba0(&GameBeaconSys->mine.beacon, itemId);
        }
    }
}

static void func_ov012_0215fba0(GameBeacon *info, u16 itemId) {
    info->type = 8;
    info->payload.value = itemId;
    func_0202d518(info, itemId);
}

void func_ov012_0215fbb0(u16 itemId) {
    if (itemId < 0x32e) {
        if (func_0202cfac(9, itemId)) {
            GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x43, itemId);
        } else if (GameBeaconSys_CanSendType(9)) {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
            func_ov012_0215fc00(&GameBeaconSys->mine.beacon, itemId);
        }
    }
}

static void func_ov012_0215fc00(GameBeacon *info, u16 itemId) {
    info->type = 9;
    info->payload.value = itemId;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fc10(u16 itemId) {
    if (itemId < 0x32e) {
        if (func_0202cfac(8, itemId)) {
            GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x42, itemId);
        } else if (GameBeaconSys_CanSendType(10)) {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
            func_ov012_0215fc60(&GameBeaconSys->mine.beacon, itemId);
        }
    }
}

static void func_ov012_0215fc60(GameBeacon *info, u16 itemId) {
    info->type = 10;
    info->payload.value = itemId;
    func_0202d534(info, itemId);
}

void func_ov012_0215fc70(u16 itemId) {
    if (itemId < 0x32e) {
        if (func_0202cfac(9, itemId)) {
            GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x43, itemId);
        } else if (GameBeaconSys_CanSendType(11)) {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
            func_ov012_0215fcc0(&GameBeaconSys->mine.beacon, itemId);
        }
    }
}

static void func_ov012_0215fcc0(GameBeacon *info, u16 itemId) {
    info->type = 11;
    info->payload.value = itemId;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fcd0(u32 passPower) {
    if (func_0202cfac(15, passPower)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x49, passPower);
    } else if (GameBeaconSys_CanSendType(0x12)) {
        if (PassPower_IsBW1Compatible(passPower)) {
            GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        } else {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        }
        func_ov012_0215fd30(&GameBeaconSys->mine.beacon, passPower);
    }
}

static void func_ov012_0215fd30(GameBeacon *info, u32 passPower) {
    info->type = 0x12;
    info->passPower = passPower;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fd50(u16 value) {
    if (func_0202cfac(12, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x46, value);
    } else if (GameBeaconSys_CanSendType(0x13)) {
        if (PML_ItemIsB2W2Only(value)) {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        } else {
            GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        }
        func_ov012_0215fdac(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_0215fdac(GameBeacon *info, u16 value) {
    info->type = 0x13;
    info->payload.value = value;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fdbc(void) {
    if (GameBeaconSys_CanSendType(0x15)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215fde0(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_0215fde0(GameBeacon *info) {
    info->type = 0x15;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fdec(u32 value) {
    u32 i;

    if (GameBeaconSys_CanSendType(0x16)) {
        for (i = 0; i < NELEMS(data_ov012_0216d718); i++) {
            if (value == data_ov012_0216d718[i]) {
                GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
                func_ov012_0215fe2c(&GameBeaconSys->mine.beacon, value);
                return;
            }
        }
    }
}

static void func_ov012_0215fe2c(GameBeacon *info, u32 value) {
    info->type = 0x16;
    info->payload.value32 = value;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fe3c(void) {
    if (GameBeaconSys_CanSendType(0x17)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215fe60(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_0215fe60(GameBeacon *info) {
    info->type = 0x17;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215fe6c(u16 value) {
    if (func_0202cfac(7, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x41, value);
    } else if (GameBeaconSys_CanSendType(0x1e)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215feb0(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_0215feb0(GameBeacon *info) {
    info->type = 0x1e;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215febc(u16 value, const StrBuf *name) {
    if (func_0202cfac(0x2b, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x65, value);
    } else if (GameBeaconSys_CanSendType(0x1f)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215ff04(&GameBeaconSys->mine.beacon, name);
    }
}

static void func_ov012_0215ff04(GameBeacon *info, const StrBuf *name) {
    info->type = 0x1f;
    GameBeacon_StoreText(name, info->payload.text);
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215ff20(u16 value, const StrBuf *name) {
    if (func_0202cfac(0x2c, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x66, value);
    } else if (GameBeaconSys_CanSendType(0x20)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215ff68(&GameBeaconSys->mine.beacon, name);
    }
}

static void func_ov012_0215ff68(GameBeacon *info, const StrBuf *name) {
    info->type = 0x20;
    GameBeacon_StoreText(name, info->payload.text);
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215ff84(u16 value, const StrBuf *name) {
    if (func_0202cfac(0x2a, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x64, value);
    } else if (GameBeaconSys_CanSendType(0x21)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_0215ffcc(&GameBeaconSys->mine.beacon, name);
    }
}

static void func_ov012_0215ffcc(GameBeacon *info, const StrBuf *name) {
    info->type = 0x21;
    GameBeacon_StoreText(name, info->payload.text);
    GameBeacon_ClearRecent(info);
}

void func_ov012_0215ffe8(const StrBuf *name) {
    if (GameBeaconSys_CanSendType(0x22)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160010(&GameBeaconSys->mine.beacon, name);
    }
}

static void func_ov012_02160010(GameBeacon *info, const StrBuf *name) {
    info->type = 0x22;
    GameBeacon_StoreText(name, info->payload.text);
    GameBeacon_ClearRecent(info);
}

void func_ov012_0216002c(u16 value) {
    if (func_0202cfac(13, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x47, value);
    } else if (GameBeaconSys_CanSendType(0x24)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160070(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_02160070(GameBeacon *info, u16 value) {
    info->type = 0x24;
    info->payload.value = value;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160080(void) {
    if (func_0202cfac(0x12, 0)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x4c, 0);
    } else if (GameBeaconSys_CanSendType(0x25)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_021600c4(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_021600c4(GameBeacon *info) {
    info->type = 0x25;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021600d0(u16 species) {
    if (func_0202cfac(0x1b, species)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x55, species);
    } else if (GameBeaconSys_CanSendType(0x26)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160114(&GameBeaconSys->mine.beacon, species);
    }
}

static void func_ov012_02160114(GameBeacon *info, u16 species) {
    info->type = 0x26;
    info->arg.value = species;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160124(void) {
    if (func_0202cfac(14, 0)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x48, 0);
    } else if (GameBeaconSys_CanSendType(0x27)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160168(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_02160168(GameBeacon *info) {
    info->type = 0x27;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160174(void) {
    if (GameBeaconSys_CanSendType(0x28)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160198(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_02160198(GameBeacon *info) {
    info->type = 0x28;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021601a4(u32 value) {
    if (GameBeaconSys_CanSendType(0x29)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_021601cc(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_021601cc(GameBeacon *info, u32 value) {
    info->type = 0x29;
    info->payload.value32 = value;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021601dc(void) {
    if (GameBeaconSys_CanSendType(0x2a)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160200(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_02160200(GameBeacon *info) {
    info->type = 0x2a;
    info->payload.value32 = 0;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160210(void) {
    if (GameBeaconSys_CanSendType(0x2b)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160234(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_02160234(GameBeacon *info) {
    info->type = 0x2b;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160240(u8 value) {
    if (GameBeaconSys_CanSendType(0x2c)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160268(&GameBeaconSys->mine.beacon, value);
    }
}

static void func_ov012_02160268(GameBeacon *info, u8 value) {
    info->type = 0x2c;
    info->payload.value8 = value;
    GameBeacon_ClearRecent(info);
}

void GameBeacon_BroadcastFerrisWheel(void) {
    if (func_0202cfac(16, 0)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x4a, 0);
    } else if (GameBeaconSys_CanSendType(0x2d)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_021602c0(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_021602c0(GameBeacon *info) {
    info->type = 0x2d;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021602cc(void) {
    if (GameBeaconSys_CanSendType(0x2e)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_021602f0(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_021602f0(GameBeacon *info) {
    info->type = 0x2e;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021602fc(const StrBuf *name) {
    if (GameBeaconSys_CanSendType(0x2f)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160324(&GameBeaconSys->mine.beacon, name);
    }
}

static void func_ov012_02160324(GameBeacon *info, const StrBuf *name) {
    info->type = 0x2f;
    GameBeacon_StoreText(name, info->payload.text);
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160340(u32 passPower) {
    if (GameBeaconSys_CanSendType(0x30)) {
        if (PassPower_IsBW1Compatible(passPower)) {
            GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        } else {
            GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        }
        func_ov012_02160380(&GameBeaconSys->mine.beacon, passPower);
    }
}

static void func_ov012_02160380(GameBeacon *info, u32 passPower) {
    info->type = 0x30;
    info->payload.value = passPower;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160390(GameData *gameData, u16 value) {
    if (GameBeaconSys_CanSendType(0x18)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_021603bc(&GameBeaconSys->mine.beacon, gameData, value);
    }
}

static void func_ov012_021603bc(GameBeacon *info, GameData *gameData, u16 value) {
    wcharsncpy(TrainerGameInfo_GetThanks(getTrainerGameInfoAddress(GameData_GetSaveControl(gameData))), info->payload.named.name, 9);
    info->type = 0x18;
    info->payload.named.value = value;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021603ec(StrBuf *name, u8 value) {
    if (func_0202cfac(0x2d, 0)) {
        GameBeaconSendSlot_SetMissionEx(&GameBeaconSys->mine, 0x67, (u32)name, value);
    } else if (GameBeaconSys_CanSendType(0x31)) {
        GameBeaconSendSlot_Reset(&GameBeaconSys->mine);
        func_ov012_02160438(&GameBeaconSys->mine.beacon, name, value);
    }
}

static void func_ov012_02160438(GameBeacon *info, const StrBuf *name, u8 value) {
    GFL_StrBufStoreString(name, info->payload.namedByte.name, 9);
    info->payload.namedByte.value = value;
    info->type = 0x31;
    GameBeacon_ClearRecent(info);
}

void func_ov012_0216045c(u8 value) {
    if (GameBeaconSys_CanSendType(0x33)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_021604b4(&GameBeaconSys->mine.beacon, value, 0x33);
    }
}

void func_ov012_02160488(u8 value) {
    if (GameBeaconSys_CanSendType(0x34)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_021604b4(&GameBeaconSys->mine.beacon, value, 0x34);
    }
}

static void func_ov012_021604b4(GameBeacon *info, u8 value, u16 type) {
    info->type = type;
    info->payload.value8 = value;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021604c4(u8 value) {
    if (GameBeaconSys_CanSendType(0x35)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_0216055c(&GameBeaconSys->mine.beacon, value, 0, 0x35);
    }
}

void func_ov012_021604f4(u8 value, u8 kind) {
    u32 type = 0x38;

    if (kind != 3) {
        type = 0x36;
    }
    if (GameBeaconSys_CanSendType(type)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_0216055c(&GameBeaconSys->mine.beacon, value, kind, type);
    }
}

void func_ov012_0216052c(u8 value, u8 extra) {
    if (GameBeaconSys_CanSendType(0x37)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_0216055c(&GameBeaconSys->mine.beacon, value, extra, 0x37);
    }
}

static void func_ov012_0216055c(GameBeacon *info, u8 value, u8 extra, u32 type) {
    info->type = type;
    info->payload.pair.a = value;
    info->payload.pair.b = extra;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160574(void) {
    if (GameBeaconSys_CanSendType(0x39)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_02160598(&GameBeaconSys->mine.beacon);
    }
}

static void func_ov012_02160598(GameBeacon *info) {
    info->type = 0x39;
    GameBeacon_ClearRecent(info);
}

void func_ov012_021605a4(u8 extra, u16 value) {
    if (GameBeaconSys_CanSendType(0x3a)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_02160604(&GameBeaconSys->mine.beacon, extra, value, 0x3a);
    }
}

void func_ov012_021605d4(u8 extra, u16 value) {
    if (GameBeaconSys_CanSendType(0x3b)) {
        GameBeaconSendSlot_ResetB2W2Only(&GameBeaconSys->mine);
        func_ov012_02160604(&GameBeaconSys->mine.beacon, extra, value, 0x3b);
    }
}

static void func_ov012_02160604(GameBeacon *info, u8 extra, u16 value, u16 type) {
    info->type = type;
    info->payload.withExtra.value = value;
    info->payload.withExtra.extra = extra;
    GameBeacon_ClearRecent(info);
}

void func_ov012_02160618(void) {
    if (func_0202cfac(10, 0)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, 0x44, 0);
    }
}

void func_ov012_0216063c(u8 type, u16 value) {
    if (type >= 4 && func_0202cfac(type, value)) {
        GameBeaconSendSlot_SetMission(&GameBeaconSys->mine, type + 0x3a, value);
    }
}
