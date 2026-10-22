// The multiboot parent's beacon: the description of each program on offer, cut into pieces that go out one by one in
// the game info of the wireless beacon (libmb). The names are descriptive
#include "mb_private.h"

// What the beacon sends: the list of programs, the one being sent, and how far it is
static MbBeaconSendStatus sSender;
// The beacon that the sender fills, an object of its own after the sender's state
static MbBeacon bsendBuff;

static u16 MBi_calc_cksum(const u16 *data, int length);
static BOOL MBi_ReadIconInfo(const char *path, MBGameInfo *record, BOOL isChar);
static int mystrlen(const u16 *str);
static void MBi_ClearSendStatus(void);
static int MBi_ReadyBeaconSendStatus(void);
static void MBi_InitSendFixedBeacon(void);
static void MBi_SendFixedBeacon(u32 ggid, u16 tgid, u8 attr);
static void MBi_InitSendVolatBeacon(void);
static void MBi_SendVolatBeacon(u32 ggid, u16 tgid, u8 attr);

static u16 MBi_calc_cksum(const u16 *data, int length) {
    u32 sum = 0;
    int count = length >> 1;

    while (count > 0) {
        sum += *data++;
        count--;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    return sum ^ 0xffff;
}

// Puts a program on offer: its picture, name, description and player limit, from its registration
void MBi_MakeGameInfo(MBGameInfo *record, const MBGameRegistry *reg, const MBUserInfo *parentUser) {
    BOOL charFailed;
    BOOL paletteFailed;
    u32 nameSize;

    MI_CpuClear16(record, sizeof(MBGameInfo));
    record->dataAttr = 0;
    charFailed = !MBi_ReadIconInfo(reg->iconCharPathp, record, TRUE);
    paletteFailed = !MBi_ReadIconInfo(reg->iconPalettePathp, record, FALSE);
    if ((charFailed | paletteFailed) != 0) {
        record->dataAttr = 1;
        MI_CpuClearFast(record, 0x220);
    }
    record->ggid = reg->ggid;
    if (parentUser != NULL) {
        MI_CpuCopy16(parentUser, &record->parentUser, sizeof(MBUserInfo));
    }
    record->maxPlayers = reg->maxPlayerNum;
    nameSize = mystrlen(reg->gameNamep) * sizeof(u16);
    MI_CpuCopy16(reg->gameNamep, record->gameName, (u16)nameSize);
    MI_CpuCopy16(reg->gameIntroductionp, record->gameIntro, sizeof(record->gameIntro));
    record->playerCount = 1;
    record->memberMask = 1;
    record->broadcastedPlayerFlag = 1;
}

// Reads one of a program's two picture files, which must be of the right size
static BOOL MBi_ReadIconInfo(const char *path, MBGameInfo *record, BOOL isChar) {
    FSFile file;
    u32 size;
    void *dst = record;

    if (isChar) {
        size = sizeof(record->iconChar);
    } else {
        size = sizeof(record->iconPalette);
    }
    if (isChar) {
        dst = record->iconChar;
    }
    if (path == NULL) {
        return FALSE;
    }
    FS_InitFile(&file);
    if (!FS_OpenFileEx(&file, path, FS_FILEMODE_R)) {
        return FALSE;
    }
    if (size != FS_GetLength(&file)) {
        FS_CloseFile(&file);
        return FALSE;
    }
    FS_ReadFile(&file, dst, size);
    FS_CloseFile(&file);
    return TRUE;
}

// Takes the players of the children in a bitmap into a program's record
void MB_UpdateGameInfoMember(MBGameInfo *record, const MBUserInfo *users, u16 memberMask, u16 changedMask) {
    u8 count = 1;
    int i;

    MI_CpuCopy16(users, record->members, sizeof(record->members));
    for (i = 0; i < 15; i++) {
        if ((2 << i) & memberMask) {
            count++;
        }
    }
    record->playerCount = count;
    record->memberMask = memberMask | 1;
    record->changedMask = changedMask;
    record->seqNoVolat++;
}

static int mystrlen(const u16 *str) {
    int count = 0;

    while (*str++ != 0) {
        count++;
    }
    return count;
}

// Adds a program to the end of the list
void MB_AddGameInfo(MBGameInfo *record) {
    MBGameInfo *last = sSender.gameInfoListTop;

    if (last == NULL) {
        sSender.gameInfoListTop = record;
    } else {
        while (last->nextp != NULL) {
            last = last->nextp;
        }
        last->nextp = record;
    }
    record->nextp = NULL;
}

void MB_InitSendGameInfoStatus(void) {
    sSender.gameInfoListTop = NULL;
    sSender.nowGameInfop = NULL;
    sSender.state = MB_BEACON_STATE_READY;
    sSender.callback = NULL;
    MBi_ClearSendStatus();
}

static void MBi_ClearSendStatus(void) {
    sSender.seqNoFixed = 0;
    sSender.seqNoVolat = 0;
    sSender.flagmentNo = 0;
    sSender.flagmentMaxNum = 0;
    sSender.beaconNo = 0;
}

// Does the next step of the beacon's cycle: each call sets the game info of one beacon
void MB_SendGameInfoBeacon(u32 ggid, u16 tgid, u8 attr) {
    for (;;) {
        switch (sSender.state) {
        case MB_BEACON_STATE_STOP:
        case MB_BEACON_STATE_READY:
            if (!MBi_ReadyBeaconSendStatus()) {
                return;
            }
            break;
        case MB_BEACON_STATE_FIXED_START:
            MBi_InitSendFixedBeacon();
            break;
        case MB_BEACON_STATE_FIXED:
            MBi_SendFixedBeacon(ggid, tgid, attr);
            return;
        case MB_BEACON_STATE_VOLAT_START:
            MBi_InitSendVolatBeacon();
            break;
        case MB_BEACON_STATE_VOLAT:
            MBi_SendVolatBeacon(ggid, tgid, attr);
            return;
        case 6: // A state that nothing in this file sets
            break;
        }
    }
}

// Goes on to the next program of the list, or tells the children that there is none
static int MBi_ReadyBeaconSendStatus(void) {
    MBGameInfo *record;

    if (sSender.gameInfoListTop == NULL) {
        u32 ggid = MBi_GetGgid();
        u16 tgid = MBi_GetTgid();

        WM_SetGameInfo(NULL, (u16 *)&bsendBuff, sizeof(MbBeacon), ggid, tgid, WM_ATTR_FLAG_CS);
        return 0;
    }
    if (sSender.nowGameInfop == NULL || (record = sSender.nowGameInfop->nextp) == NULL) {
        record = sSender.gameInfoListTop;
    }
    sSender.nowGameInfop = record;
    MBi_ClearSendStatus();
    sSender.seqNoVolat = sSender.nowGameInfop->seqNoVolat;
    sSender.state = MB_BEACON_STATE_FIXED_START;
    return 1;
}

// Starts to cut the program's record into pieces, leaving out its picture if that could not be read
static void MBi_InitSendFixedBeacon(void) {
    if (sSender.state == MB_BEACON_STATE_FIXED_START) {
        MBGameInfo *record = sSender.nowGameInfop;

        if (record->dataAttr == 0) {
            sSender.flagmentMaxNum = 9;
            sSender.srcp = (u8 *)record;
        } else {
            sSender.flagmentMaxNum = 4;
            sSender.srcp = (u8 *)&record->parentUser;
        }
        sSender.state = MB_BEACON_STATE_FIXED;
    }
}

// Puts the beacon out. 1 is the wireless manager's entry attribute: children may connect. Through a u32 parameter the
// original ORs the caller's flag and this one apart, where `attr | WM_ATTR_FLAG_MB | 1` folds them
static inline void MBGameInfo_SetBeacon(u32 ggid, u16 tgid, u32 attr) {
    WM_SetGameInfo(NULL, (u16 *)&bsendBuff, sizeof(MbBeacon), ggid, tgid, attr | 1);
}

// Sends the next piece of the record
static void MBi_SendFixedBeacon(u32 ggid, u16 tgid, u8 attr) {
    const u8 *recordEnd = (const u8 *)&sSender.nowGameInfop->playerCount;

    if (sSender.srcp + MB_BEACON_FIXED_DATA_SIZE <= recordEnd) {
        bsendBuff.data.fragment.length = MB_BEACON_FIXED_DATA_SIZE;
    } else {
        bsendBuff.data.fragment.length = recordEnd - sSender.srcp;
        MI_CpuFill16(&bsendBuff.data.fragment.data[bsendBuff.data.fragment.length], 0,
                     MB_BEACON_FIXED_DATA_SIZE - bsendBuff.data.fragment.length);
    }
    MI_CpuCopy16(sSender.srcp, bsendBuff.data.fragment.data, bsendBuff.data.fragment.length);
    bsendBuff.data.fragment.index = sSender.flagmentNo;
    bsendBuff.data.fragment.count = sSender.flagmentMaxNum;
    bsendBuff.dataAttr = sSender.nowGameInfop->dataAttr;
    bsendBuff.seqNoFixed = sSender.nowGameInfop->seqNoFixed;
    bsendBuff.seqNoVolat = sSender.seqNoVolat;
    bsendBuff.ggid = sSender.nowGameInfop->ggid;
    bsendBuff.fileNo = sSender.nowGameInfop->fileNo;
    bsendBuff.beaconNo = sSender.beaconNo++;
    bsendBuff.checksum = 0;
    bsendBuff.checksum = MBi_calc_cksum(&bsendBuff.checksum, MB_BEACON_DATA_SIZE);
    sSender.flagmentNo++;
    if (sSender.flagmentNo < sSender.flagmentMaxNum) {
        sSender.srcp += MB_BEACON_FIXED_DATA_SIZE;
    } else {
        sSender.state = MB_BEACON_STATE_VOLAT_START;
    }
    MBGameInfo_SetBeacon(ggid, tgid, attr | WM_ATTR_FLAG_MB);
}

// Starts to send who is in the program's game
static void MBi_InitSendVolatBeacon(void) {
    sSender.nowGameInfop->broadcastedPlayerFlag = 1;
    sSender.seqNoVolat = sSender.nowGameInfop->seqNoVolat;
    sSender.state = MB_BEACON_STATE_VOLAT;
}

// Sends up to four of the players that the children have not seen yet
static void MBi_SendVolatBeacon(u32 ggid, u16 tgid, u8 attr) {
    u8 *params;
    u16 missing;
    int i;
    int j;
    u32 sent;

    if (sSender.seqNoVolat != sSender.nowGameInfop->seqNoVolat) {
        MBi_InitSendVolatBeacon();
    }
    bsendBuff.dataAttr = MB_BEACON_DATA_ATTR_VOLAT;
    bsendBuff.seqNoFixed = sSender.nowGameInfop->seqNoFixed;
    bsendBuff.seqNoVolat = sSender.seqNoVolat;
    bsendBuff.ggid = sSender.nowGameInfop->ggid;
    bsendBuff.fileNo = sSender.nowGameInfop->fileNo;
    bsendBuff.beaconNo = sSender.beaconNo++;
    bsendBuff.data.members.playerCount = sSender.nowGameInfop->playerCount;
    bsendBuff.data.members.memberMask = sSender.nowGameInfop->memberMask;
    bsendBuff.data.members.changedMask = sSender.nowGameInfop->changedMask;
    if (sSender.mode == 0 && sSender.callback != NULL) {
        sSender.callback(sSender.nowGameInfop->ggid);
    }
    // The loops test a 0/1 value: the original enters both with a jump to the test at their end
    for (i = 0, params = bsendBuff.data.members.params; i < 8 ? TRUE : FALSE; i++) {
        params[i] = sSender.nowGameInfop->params[i];
    }
    MI_CpuClear16(bsendBuff.data.members.users, sizeof(bsendBuff.data.members.users));
    missing = sSender.nowGameInfop->broadcastedPlayerFlag ^ sSender.nowGameInfop->memberMask;
    sent = 0;
    for (j = 0; j < 15 ? TRUE : FALSE; j++) {
        if ((2 << j) & missing) {
            MI_CpuCopy16(&sSender.nowGameInfop->members[j], &bsendBuff.data.members.users[sent], sizeof(MBUserInfo));
            sSender.nowGameInfop->broadcastedPlayerFlag |= (u16)(2U << j);
            sent++;
            if (sent == 4) {
                break;
            }
        }
    }
    if (sent < 4) {
        bsendBuff.data.members.users[sent].playerNo = 0;
    }
    bsendBuff.checksum = 0;
    bsendBuff.checksum = MBi_calc_cksum(&bsendBuff.checksum, MB_BEACON_DATA_SIZE);
    if (sSender.nowGameInfop->broadcastedPlayerFlag == sSender.nowGameInfop->memberMask) {
        sSender.state = MB_BEACON_STATE_READY;
    }
    MBGameInfo_SetBeacon(ggid, tgid, attr | WM_ATTR_FLAG_MB);
    if (sSender.mode == 1 && sSender.callback != NULL) {
        sSender.callback(sSender.nowGameInfop->ggid);
    }
}
