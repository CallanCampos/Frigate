// The multiboot beacon: the parent's description of each program on offer, cut into pieces that go out one by one in
// the game info of the wireless beacon, and the child's reception of them (libmb). The names are NitroSDK's, as
// ntrtwl's reconstruction of NitroSDK has them, except MBGameInfo_SetBeacon, which is ours.
//
// Overlay 181 is a parent, so the link dropped the child's side and the functions nothing calls: MB_DeleteGameInfo,
// everything from MB_InitRecvGameInfoStatus to MB_GetGameInfoRecvList but MBi_CheckMBParent, and the callback and
// user data functions at the end. They are rebuilt here from ntrtwl's NitroSDK (libraries/mb/src/mb_gameinfo.c) with
// no asm to check them against, and dropped from the object again through config/deadstrip.toml (docs/configs.md,
// Dead-stripped code), for the receive side's statics they keep: under -ipa file the statics under 64 bytes share one
// .bss section, in MWCC's size sort, so the four words that only the child uses come first in the ROM's .bss, ahead of
// the callback's timing, the callback and the send state, while the two big ones, each in a section of its own, go
// with the dropped code. The pooled section ends at 0x2c, and the beacon follows at 0x48, 32-aligned as the SDK
// declares it, which the two beacon functions need to match (docs/matching.md, Loops: "An explicit alignment")

#include "mb_private.h"

// How long a parent that is no longer heard is kept, and how long the scan stays locked on one, in seconds
#define MB_GAME_INFO_LIFETIME_SEC 60
#define MB_SCAN_LOCK_SEC 8
// How many times a parent may send the same beacon before the child takes it for gone
#define MB_SAME_BEACON_RECV_MAX_COUNT 3
// The players one beacon carries
#define MB_SEND_MEMBER_MAX_NUM 4

// The lifetimes in scans
#define MB_LIFETIME_MAX_COUNT (MB_GAME_INFO_LIFETIME_SEC * 1000 / MB_SCAN_TIME_NORMAL + 1)
#define MB_LOCKTIME_MAX_COUNT (MB_SCAN_LOCK_SEC * 1000 / MB_SCAN_TIME_NORMAL + 1)
// How much of a lifetime one scan takes, normally and while the scan is locked on a parent
#define MB_SCAN_COUNT_UNIT_NORMAL 1
#define MB_SCAN_COUNT_UNIT_LOCKING (MB_SCAN_TIME_LOCKING / MB_SCAN_TIME_NORMAL)

static MbBeaconSendStatus mbss;
static MbBeaconRecvStatus mbrs;
static MbBeaconRecvStatus *mbrsp = NULL;
static MbScanLockFunc sLockFunc = NULL;
static MbScanUnlockFunc sUnlockFunc = NULL;
static MbBeacon bsendBuff ATTRIBUTE_ALIGN(32);
static MbBeacon *brecvBuffp;
static WMBssDesc bssDescbuf ATTRIBUTE_ALIGN(32);
static MBSendVolatCallbackFunc sSendVolatCallback = NULL;
static u32 sSendVolatCallbackTimming;

static BOOL MBi_ReadIconInfo(const char *path, MBGameInfo *record, BOOL isChar);
static int mystrlen(const u16 *str);
static void MBi_ClearSendStatus(void);
static BOOL MBi_ReadyBeaconSendStatus(void);
static void MBi_InitSendFixedBeacon(void);
static void MBi_SendFixedBeacon(u32 ggid, u16 tgid, u8 attr);
static void MBi_InitSendVolatBeacon(void);
static void MBi_SendVolatBeacon(u32 ggid, u16 tgid, u8 attr);

// The functions the link dropped, which only this file declares (config/deadstrip.toml)
const MbBeaconRecvStatus *MB_GetBeaconRecvStatus(void);
void MBi_SetBeaconRecvStatusBufferDefault(void);
void MBi_SetBeaconRecvStatusBuffer(MbBeaconRecvStatus *buf);
void MBi_SetScanLockFunc(MbScanLockFunc lock, MbScanUnlockFunc unlock);
BOOL MB_DeleteGameInfo(MBGameInfo *record);
void MB_InitRecvGameInfoStatus(void);
BOOL MB_RecvGameInfoBeacon(MBBeaconMsgCallback callback, u16 linkLevel, WMBssDesc *bssDesc);
void MB_CountGameInfoLifetime(MBBeaconMsgCallback callback, BOOL foundParent);
void MB_DeleteRecvGameInfo(int index);
void MB_DeleteRecvGameInfoWithoutBssdesc(int index);
MBGameInfoRecvList *MB_GetGameInfoRecvList(int index);
void MB_SetSendVolatCallback(MBSendVolatCallbackFunc callback, u32 timing);
void MB_SetUserVolatData(u32 ggid, const u8 *userData, u32 size);
void *MB_GetUserVolatData(const WMGameInfo *gameInfo);
static void MBi_SetSSIDToBssDesc(WMBssDesc *bssDesc, u32 ggid);
static int MBi_GetStoreElement(WMBssDesc *bssDesc, MBBeaconMsgCallback callback);
static void MBi_CheckCompleteGameInfoFlagments(int index, MBBeaconMsgCallback callback);
static void MBi_AnalyzeBeacon(WMBssDesc *bssDesc, int index, u16 linkLevel);
static void MBi_CheckTGID(WMBssDesc *bssDesc, int index);
static void MBi_CheckSeqNoFixed(int index);
static void MBi_CheckSeqNoVolat(int index);
static void MBi_InvalidateGameInfoBssID(const u8 *bssid);
static void MBi_RecvFixedBeacon(int index);
static void MBi_RecvVolatBeacon(int index);
static void MBi_LockScanTarget(int index);
static void MBi_UnlockScanTarget(void);

// Rebuilt from ntrtwl's NitroSDK: the child's list of the parents it heard
const MbBeaconRecvStatus *MB_GetBeaconRecvStatus(void) {
    return mbrsp;
}

// Rebuilt from ntrtwl's NitroSDK
void MBi_SetBeaconRecvStatusBufferDefault(void) {
    mbrsp = &mbrs;
}

// Rebuilt from ntrtwl's NitroSDK
void MBi_SetBeaconRecvStatusBuffer(MbBeaconRecvStatus *buf) {
    mbrsp = buf;
}

// Rebuilt from ntrtwl's NitroSDK: what locks the child's scan on one parent, and unlocks it
void MBi_SetScanLockFunc(MbScanLockFunc lock, MbScanUnlockFunc unlock) {
    sLockFunc = lock;
    sUnlockFunc = unlock;
}

// Puts a program on offer: its picture, name, description and player limit, from its registration
void MBi_MakeGameInfo(MBGameInfo *record, const MBGameRegistry *reg, const MBUserInfo *parentUser) {
    BOOL charFailed;
    BOOL paletteFailed;
    u32 nameSize;

    MI_CpuClear16(record, sizeof(MBGameInfo));
    record->dataAttr = MB_BEACON_DATA_ATTR_FIXED_NORMAL;
    charFailed = !MBi_ReadIconInfo(reg->iconCharPathp, record, TRUE);
    paletteFailed = !MBi_ReadIconInfo(reg->iconPalettePathp, record, FALSE);
    if ((charFailed | paletteFailed) != 0) {
        record->dataAttr = MB_BEACON_DATA_ATTR_FIXED_NO_ICON;
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
    if (size != FS_GetFileLength(&file)) {
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
    for (i = 0; i < MB_MAX_CHILD; i++) {
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
    MBGameInfo *last = mbss.gameInfoListTop;

    if (last == NULL) {
        mbss.gameInfoListTop = record;
    } else {
        while (last->nextp != NULL) {
            last = last->nextp;
        }
        last->nextp = record;
    }
    record->nextp = NULL;
}

// Rebuilt from ntrtwl's NitroSDK: takes a program off the list, and goes on to the next if it was being sent
BOOL MB_DeleteGameInfo(MBGameInfo *record) {
    MBGameInfo *current = mbss.gameInfoListTop;
    MBGameInfo *before;

    while (current != NULL) {
        if (current != record) {
            before = current;
            current = current->nextp;
            continue;
        }
        if (current == mbss.gameInfoListTop) {
            mbss.gameInfoListTop = mbss.gameInfoListTop->nextp;
        } else {
            before->nextp = current->nextp;
        }
        if (record == mbss.nowGameInfop) {
            mbss.nowGameInfop = NULL;
            if (!MBi_ReadyBeaconSendStatus()) {
                mbss.state = MB_BEACON_STATE_READY;
            }
        }
        return TRUE;
    }
    return FALSE;
}

void MB_InitSendGameInfoStatus(void) {
    mbss.gameInfoListTop = NULL;
    mbss.nowGameInfop = NULL;
    mbss.state = MB_BEACON_STATE_READY;
    sSendVolatCallback = NULL;
    MBi_ClearSendStatus();
}

static void MBi_ClearSendStatus(void) {
    mbss.seqNoFixed = 0;
    mbss.seqNoVolat = 0;
    mbss.flagmentNo = 0;
    mbss.flagmentMaxNum = 0;
    mbss.beaconNo = 0;
}

// Does the next step of the beacon's cycle: each call sets the game info of one beacon
void MB_SendGameInfoBeacon(u32 ggid, u16 tgid, u8 attr) {
    for (;;) {
        switch (mbss.state) {
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
        case MB_BEACON_STATE_NEXT_GAME:
            break;
        }
    }
}

// Goes on to the next program of the list, or tells the children that there is none
static BOOL MBi_ReadyBeaconSendStatus(void) {
    MBGameInfo *record;

    if (!mbss.gameInfoListTop) {
        WM_SetGameInfo(NULL, (u16 *)&bsendBuff, sizeof(MbBeacon), MBi_GetGgid(), MBi_GetTgid(), WM_ATTR_FLAG_CS);
        return FALSE;
    }
    if (!mbss.nowGameInfop || !mbss.nowGameInfop->nextp) {
        record = mbss.gameInfoListTop;
    } else {
        record = mbss.nowGameInfop->nextp;
    }
    mbss.nowGameInfop = record;
    MBi_ClearSendStatus();
    mbss.seqNoVolat = mbss.nowGameInfop->seqNoVolat;
    mbss.state = MB_BEACON_STATE_FIXED_START;
    return TRUE;
}

// Starts to cut the program's record into pieces, leaving out its picture if that could not be read
static void MBi_InitSendFixedBeacon(void) {
    if (mbss.state == MB_BEACON_STATE_FIXED_START) {
        MBGameInfo *record = mbss.nowGameInfop;

        if (record->dataAttr == MB_BEACON_DATA_ATTR_FIXED_NORMAL) {
            mbss.flagmentMaxNum = 9;
            mbss.srcp = (u8 *)record;
        } else {
            mbss.flagmentMaxNum = 4;
            mbss.srcp = (u8 *)&record->parentUser;
        }
        mbss.state = MB_BEACON_STATE_FIXED;
    }
}

// Puts the beacon out. 1 is the wireless manager's entry attribute: children may connect. Through a u32 parameter the
// original ORs the caller's flag and this one apart, where `attr | WM_ATTR_FLAG_MB | 1` folds them
static inline void MBGameInfo_SetBeacon(u32 ggid, u16 tgid, u32 attr) {
    WM_SetGameInfo(NULL, (u16 *)&bsendBuff, sizeof(MbBeacon), ggid, tgid, attr | 1);
}

// Sends the next piece of the record
static void MBi_SendFixedBeacon(u32 ggid, u16 tgid, u8 attr) {
    const u8 *recordEnd = (const u8 *)&mbss.nowGameInfop->playerCount;

    if (mbss.srcp + MB_BEACON_FIXED_DATA_SIZE <= recordEnd) {
        bsendBuff.data.fragment.length = MB_BEACON_FIXED_DATA_SIZE;
    } else {
        bsendBuff.data.fragment.length = recordEnd - mbss.srcp;
        MI_CpuFill16(&bsendBuff.data.fragment.data[bsendBuff.data.fragment.length], 0,
                     MB_BEACON_FIXED_DATA_SIZE - bsendBuff.data.fragment.length);
    }
    MI_CpuCopy16(mbss.srcp, bsendBuff.data.fragment.data, bsendBuff.data.fragment.length);
    bsendBuff.data.fragment.index = mbss.flagmentNo;
    bsendBuff.data.fragment.count = mbss.flagmentMaxNum;
    bsendBuff.dataAttr = mbss.nowGameInfop->dataAttr;
    bsendBuff.seqNoFixed = mbss.nowGameInfop->seqNoFixed;
    bsendBuff.seqNoVolat = mbss.seqNoVolat;
    bsendBuff.ggid = mbss.nowGameInfop->ggid;
    bsendBuff.fileNo = mbss.nowGameInfop->fileNo;
    bsendBuff.beaconNo = mbss.beaconNo++;
    bsendBuff.checksum = 0;
    bsendBuff.checksum = MBi_calc_cksum(&bsendBuff.checksum, MB_BEACON_DATA_SIZE);
    mbss.flagmentNo++;
    if (mbss.flagmentNo < mbss.flagmentMaxNum) {
        mbss.srcp += MB_BEACON_FIXED_DATA_SIZE;
    } else {
        mbss.state = MB_BEACON_STATE_VOLAT_START;
    }
    MBGameInfo_SetBeacon(ggid, tgid, attr | WM_ATTR_FLAG_MB);
}

// Starts to send who is in the program's game
static void MBi_InitSendVolatBeacon(void) {
    mbss.nowGameInfop->broadcastedPlayerFlag = 1;
    mbss.seqNoVolat = mbss.nowGameInfop->seqNoVolat;
    mbss.state = MB_BEACON_STATE_VOLAT;
}

// Sends up to four of the players that the children have not seen yet, with the program's own data, and calls the
// program's callback before or after. The callback's test is of the pointer itself: with `!= NULL` MWCC loads a 0 to
// compare it with
static void MBi_SendVolatBeacon(u32 ggid, u16 tgid, u8 attr) {
    int i;
    u32 sent;
    u16 missing;

    if (mbss.seqNoVolat != mbss.nowGameInfop->seqNoVolat) {
        MBi_InitSendVolatBeacon();
    }
    bsendBuff.dataAttr = MB_BEACON_DATA_ATTR_VOLAT;
    bsendBuff.seqNoFixed = mbss.nowGameInfop->seqNoFixed;
    bsendBuff.seqNoVolat = mbss.seqNoVolat;
    bsendBuff.ggid = mbss.nowGameInfop->ggid;
    bsendBuff.fileNo = mbss.nowGameInfop->fileNo;
    bsendBuff.beaconNo = mbss.beaconNo++;
    bsendBuff.data.members.playerCount = mbss.nowGameInfop->playerCount;
    bsendBuff.data.members.memberMask = mbss.nowGameInfop->memberMask;
    bsendBuff.data.members.changedMask = mbss.nowGameInfop->changedMask;
    if (sSendVolatCallbackTimming == MB_SEND_VOLAT_CALLBACK_TIMMING_BEFORE && sSendVolatCallback) {
        sSendVolatCallback(mbss.nowGameInfop->ggid);
    }
    for (i = 0; i < MB_USER_VOLAT_DATA_SIZE; i++) {
        bsendBuff.data.members.params[i] = mbss.nowGameInfop->params[i];
    }
    MI_CpuClear16(bsendBuff.data.members.users, sizeof(bsendBuff.data.members.users));
    sent = 0;
    missing = mbss.nowGameInfop->broadcastedPlayerFlag ^ mbss.nowGameInfop->memberMask;
    for (i = 0; i < MB_MAX_CHILD; i++) {
        if ((missing & (2 << i)) == 0) {
            continue;
        }
        MI_CpuCopy16(&mbss.nowGameInfop->members[i], &bsendBuff.data.members.users[sent], sizeof(MBUserInfo));
        mbss.nowGameInfop->broadcastedPlayerFlag |= 2 << i;
        if (++sent == MB_SEND_MEMBER_MAX_NUM) {
            break;
        }
    }
    if (sent < MB_SEND_MEMBER_MAX_NUM) {
        bsendBuff.data.members.users[sent].playerNo = 0;
    }
    bsendBuff.checksum = 0;
    bsendBuff.checksum = MBi_calc_cksum(&bsendBuff.checksum, MB_BEACON_DATA_SIZE);
    if (mbss.nowGameInfop->broadcastedPlayerFlag == mbss.nowGameInfop->memberMask) {
        mbss.state = MB_BEACON_STATE_READY;
    }
    MBGameInfo_SetBeacon(ggid, tgid, attr | WM_ATTR_FLAG_MB);
    if (sSendVolatCallbackTimming == MB_SEND_VOLAT_CALLBACK_TIMMING_AFTER && sSendVolatCallback) {
        sSendVolatCallback(mbss.nowGameInfop->ggid);
    }
}

// Rebuilt from ntrtwl's NitroSDK: forgets every parent the child heard
void MB_InitRecvGameInfoStatus(void) {
    MI_CpuClearFast(mbrsp, sizeof(MbBeaconRecvStatus));
    mbrsp->scanCountUnit = MB_SCAN_COUNT_UNIT_NORMAL;
}

// Rebuilt from ntrtwl's NitroSDK: takes in a beacon that the child's scan found, if it is a multiboot parent's
BOOL MB_RecvGameInfoBeacon(MBBeaconMsgCallback callback, u16 linkLevel, WMBssDesc *bssDesc) {
    int index;

    if (!MBi_CheckMBParent(bssDesc)) {
        return FALSE;
    }
    MI_CpuCopy16(bssDesc, &bssDescbuf, sizeof(WMBssDesc));
    bssDesc = &bssDescbuf;
    brecvBuffp = (MbBeacon *)bssDesc->gameInfo.userGameInfo;
    if (MBi_calc_cksum(&brecvBuffp->checksum, MB_BEACON_DATA_SIZE)) {
        return FALSE;
    }
    index = MBi_GetStoreElement(bssDesc, callback);
    if (index < 0) {
        return FALSE;
    }
    MBi_AnalyzeBeacon(bssDesc, index, linkLevel);
    MBi_CheckCompleteGameInfoFlagments(index, callback);
    return TRUE;
}

// Whether a beacon is one of a program that offers a multiboot download
BOOL MBi_CheckMBParent(const WMBssDesc *bssDesc) {
    if (bssDesc->gameInfo.magicNumber != WM_GAMEINFO_MAGIC_NUMBER ||
        !(bssDesc->gameInfo.gameNameCount_attribute & WM_ATTR_FLAG_MB)) {
        return FALSE;
    }
    return TRUE;
}

// Rebuilt from ntrtwl's NitroSDK: the parent's SSID, which the child connects with, is its game's ID and tgid
static void MBi_SetSSIDToBssDesc(WMBssDesc *bssDesc, u32 ggid) {
    bssDesc->ssidLength = 32;
    ((u16 *)bssDesc->ssid)[0] = ggid & 0xffff;
    ((u16 *)bssDesc->ssid)[1] = (ggid & 0xffff0000) >> 16;
    ((u16 *)bssDesc->ssid)[2] = bssDesc->gameInfo.tgid;
}

// Rebuilt from ntrtwl's NitroSDK: the entry of the list that the beacon's parent has, or a free one for it
static int MBi_GetStoreElement(WMBssDesc *bssDesc, MBBeaconMsgCallback callback) {
    int i;

    for (i = 0; i < MB_GAME_INFO_RECV_LIST_NUM; i++) {
        MBGameInfoRecvList *info = &mbrsp->list[i];

        if ((mbrsp->usingGameInfoFlag & (1 << i)) == 0) {
            continue;
        }
        if (info->gameInfo.ggid != brecvBuffp->ggid) {
            continue;
        }
        if (!WM_IsBssidEqual(info->bssDesc.bssid, bssDesc->bssid)) {
            continue;
        }
        if (mbrsp->list[i].gameInfo.fileNo != brecvBuffp->fileNo) {
            continue;
        }
        if (!(mbrsp->validGameInfoFlag & (1 << i))) {
            MBi_LockScanTarget(i);
        }
        return i;
    }
    for (i = 0; i < MB_GAME_INFO_RECV_LIST_NUM; i++) {
        if (mbrsp->usingGameInfoFlag & (1 << i)) {
            continue;
        }
        MI_CpuCopy16(bssDesc, &mbrsp->list[i].bssDesc, sizeof(WMBssDesc));
        mbrsp->list[i].gameInfo.seqNoFixed = brecvBuffp->seqNoFixed;
        mbrsp->usingGameInfoFlag |= 1 << i;
        MBi_LockScanTarget(i);
        return i;
    }
    if (callback != NULL) {
        callback(MB_BC_MSG_GINFO_LIST_FULL, NULL, 0);
    }
    return -1;
}

// Rebuilt from ntrtwl's NitroSDK: tells the callback when a parent's record is whole, or no longer is
static void MBi_CheckCompleteGameInfoFlagments(int index, MBBeaconMsgCallback callback) {
    MBGameInfoRecvList *info = &mbrsp->list[index];

    if (callback != NULL) {
        callback(MB_BC_MSG_GINFO_BEACON, info, index);
    }
    if (info->getFlagmentFlag == info->allFlagmentFlag && info->getFlagmentFlag && info->gameInfo.memberMask &&
        info->getPlayerFlag == info->gameInfo.memberMask) {
        if (mbrsp->validGameInfoFlag & (1 << index)) {
            return;
        }
        mbrsp->validGameInfoFlag |= 1 << index;
        mbrsp->usefulGameInfoFlag |= 1 << index;
        MBi_UnlockScanTarget();
        if (callback != NULL) {
            callback(MB_BC_MSG_GINFO_VALIDATED, info, index);
        }
    } else {
        if ((mbrsp->validGameInfoFlag & (1 << index)) == 0) {
            return;
        }
        mbrsp->validGameInfoFlag ^= 1 << index;
        if (callback != NULL) {
            callback(MB_BC_MSG_GINFO_INVALIDATED, info, index);
        }
    }
}

// Rebuilt from ntrtwl's NitroSDK: ages the parents by one scan, forgetting those not heard for too long, and unlocks
// the scan when its parent is lost or its time is up
void MB_CountGameInfoLifetime(MBBeaconMsgCallback callback, BOOL foundParent) {
    int i;
    BOOL unlock = FALSE;

    for (i = 0; i < MB_GAME_INFO_RECV_LIST_NUM; i++) {
        MBGameInfoRecvList *info = &mbrsp->list[i];
        u16 mask = 1 << i;

        if ((mbrsp->usingGameInfoFlag & mask) == 0) {
            continue;
        }
        info->lifetimeCount -= mbrsp->scanCountUnit;
        if (info->lifetimeCount >= 0) {
            continue;
        }
        info->lifetimeCount = 0;
        if (mbrsp->validGameInfoFlag & mask) {
            if (callback != NULL) {
                callback(MB_BC_MSG_GINFO_LOST, info, i);
            }
        }
        if (mbrsp->nowScanTargetFlag & mask) {
            unlock = TRUE;
        }
        mbrsp->usingGameInfoFlag &= ~mask;
        MB_DeleteRecvGameInfo(i);
    }
    if (mbrsp->nowScanTargetFlag && mbrsp->nowLockTimeCount > 0) {
        mbrsp->nowLockTimeCount -= mbrsp->scanCountUnit;
        if (mbrsp->nowLockTimeCount < 0) {
            unlock = TRUE;
        } else if (!foundParent) {
            if (++mbrsp->notFoundLockTargetCount > 4) {
                unlock = TRUE;
            }
        } else {
            mbrsp->notFoundLockTargetCount = 0;
        }
    }
    if (unlock) {
        mbrsp->nowLockTimeCount = 0;
        MBi_UnlockScanTarget();
    }
}

// Rebuilt from ntrtwl's NitroSDK: takes a beacon's contents into its parent's entry
static void MBi_AnalyzeBeacon(WMBssDesc *bssDesc, int index, u16 linkLevel) {
    MBGameInfoRecvList *info;

    MBi_CheckTGID(bssDesc, index);
    MBi_CheckSeqNoFixed(index);
    MBi_CheckSeqNoVolat(index);
    info = &mbrsp->list[index];
    if (info->beaconNo == brecvBuffp->beaconNo) {
        if (++info->sameBeaconRecvCount > MB_SAME_BEACON_RECV_MAX_COUNT) {
            info->lifetimeCount = 0;
            MBi_InvalidateGameInfoBssID(info->bssDesc.bssid);
            return;
        }
    } else {
        info->sameBeaconRecvCount = 0;
    }
    info->beaconNo = brecvBuffp->beaconNo;
    info->lifetimeCount = MB_LIFETIME_MAX_COUNT;
    info->gameInfo.ggid = brecvBuffp->ggid;
    info->gameInfo.fileNo = brecvBuffp->fileNo;
    info->linkLevel = linkLevel;
    MBi_SetSSIDToBssDesc(&info->bssDesc, info->gameInfo.ggid);
    if (brecvBuffp->dataAttr == MB_BEACON_DATA_ATTR_VOLAT) {
        MBi_RecvVolatBeacon(index);
    } else {
        MBi_RecvFixedBeacon(index);
    }
}

// Rebuilt from ntrtwl's NitroSDK: a parent that started over, with a new tgid, is taken in afresh
static void MBi_CheckTGID(WMBssDesc *bssDesc, int index) {
    if (mbrsp->list[index].bssDesc.gameInfo.tgid == bssDesc->gameInfo.tgid) {
        return;
    }
    MB_DeleteRecvGameInfoWithoutBssdesc(index);
    MI_CpuCopy16(bssDesc, &mbrsp->list[index].bssDesc, sizeof(WMBssDesc));
    MBi_LockScanTarget(index);
}

// Rebuilt from ntrtwl's NitroSDK: a record that changed is received again
static void MBi_CheckSeqNoFixed(int index) {
    if (mbrsp->list[index].gameInfo.seqNoFixed == brecvBuffp->seqNoFixed) {
        return;
    }
    MB_DeleteRecvGameInfoWithoutBssdesc(index);
    MBi_LockScanTarget(index);
    mbrsp->list[index].gameInfo.seqNoFixed = brecvBuffp->seqNoFixed;
}

// Rebuilt from ntrtwl's NitroSDK: players that changed lock the scan on the parent
static void MBi_CheckSeqNoVolat(int index) {
    if (mbrsp->list[index].gameInfo.seqNoVolat != brecvBuffp->seqNoVolat) {
        MBi_LockScanTarget(index);
    }
}

// Rebuilt from ntrtwl's NitroSDK: ends the lifetime of every entry of a parent
static void MBi_InvalidateGameInfoBssID(const u8 *bssid) {
    int i;

    for (i = 0; i < MB_GAME_INFO_RECV_LIST_NUM; i++) {
        if ((mbrsp->usingGameInfoFlag & (1 << i)) == 0) {
            continue;
        }
        if (!WM_IsBssidEqual(bssid, mbrsp->list[i].bssDesc.bssid)) {
            continue;
        }
        mbrsp->list[i].lifetimeCount = 0;
    }
}

// Rebuilt from ntrtwl's NitroSDK: puts a piece of a record in its place
static void MBi_RecvFixedBeacon(int index) {
    MBGameInfoRecvList *info = &mbrsp->list[index];
    const u8 *recordEnd = (const u8 *)&info->gameInfo.playerCount;
    u8 *dst;

    if (info->gameInfo.seqNoFixed == brecvBuffp->seqNoFixed) {
        if (info->getFlagmentFlag & (1 << brecvBuffp->data.fragment.index)) {
            return;
        }
    }
    if (brecvBuffp->dataAttr == MB_BEACON_DATA_ATTR_FIXED_NORMAL) {
        dst = (u8 *)&info->gameInfo;
    } else {
        dst = (u8 *)&info->gameInfo.parentUser;
    }
    dst += MB_BEACON_FIXED_DATA_SIZE * brecvBuffp->data.fragment.index;
    if (dst + brecvBuffp->data.fragment.length > recordEnd) {
        return;
    }
    MI_CpuCopy16(brecvBuffp->data.fragment.data, dst, brecvBuffp->data.fragment.length);
    info->gameInfo.dataAttr = brecvBuffp->dataAttr;
    info->getFlagmentFlag |= 1 << brecvBuffp->data.fragment.index;
    info->allFlagmentFlag = (1 << brecvBuffp->data.fragment.count) - 1;
}

// Rebuilt from ntrtwl's NitroSDK: takes in the players that a beacon carries, starting over when some were missed
static void MBi_RecvVolatBeacon(int index) {
    int i;
    MBGameInfoRecvList *info = &mbrsp->list[index];

    for (i = 0; i < MB_USER_VOLAT_DATA_SIZE; i++) {
        info->gameInfo.params[i] = brecvBuffp->data.members.params[i];
    }
    MI_CpuCopy16(brecvBuffp, info->bssDesc.gameInfo.userGameInfo, WM_SIZE_USER_GAMEINFO);
    if (info->gameInfo.seqNoVolat != brecvBuffp->seqNoVolat) {
        if ((u8)(info->gameInfo.seqNoVolat + 1) == brecvBuffp->seqNoVolat) {
            for (i = 0; i < MB_MAX_CHILD; i++) {
                if (brecvBuffp->data.members.changedMask & (2 << i)) {
                    MI_CpuClear16(&info->gameInfo.members[i], sizeof(MBUserInfo));
                }
            }
            info->getPlayerFlag &= ~brecvBuffp->data.members.changedMask;
            mbrsp->validGameInfoFlag &= ~(1 << index);
        } else {
            MI_CpuClear16(info->gameInfo.members, sizeof(info->gameInfo.members));
            info->getPlayerFlag = 0;
            mbrsp->validGameInfoFlag &= ~(1 << index);
        }
        info->gameInfo.seqNoVolat = brecvBuffp->seqNoVolat;
    } else if (info->getPlayerFlag == brecvBuffp->data.members.memberMask) {
        return;
    }
    info->gameInfo.playerCount = brecvBuffp->data.members.playerCount;
    info->gameInfo.memberMask = brecvBuffp->data.members.memberMask;
    info->gameInfo.changedMask = brecvBuffp->data.members.changedMask;
    info->getPlayerFlag |= 1;
    for (i = 0; i < MB_SEND_MEMBER_MAX_NUM; i++) {
        int playerNo = brecvBuffp->data.members.users[i].playerNo;

        if (playerNo == 0) {
            continue;
        }
        MI_CpuCopy16(&brecvBuffp->data.members.users[i], &info->gameInfo.members[playerNo - 1], sizeof(MBUserInfo));
        info->getPlayerFlag |= 1 << playerNo;
    }
}

// Rebuilt from ntrtwl's NitroSDK: keeps the scan on one parent while its record comes in
static void MBi_LockScanTarget(int index) {
    if (mbrsp->nowScanTargetFlag) {
        return;
    }
    if (sLockFunc != NULL) {
        sLockFunc(mbrsp->list[index].bssDesc.bssid);
    }
    mbrsp->scanCountUnit = MB_SCAN_COUNT_UNIT_LOCKING;
    mbrsp->nowScanTargetFlag = 1 << index;
    mbrsp->nowLockTimeCount = MB_LOCKTIME_MAX_COUNT;
}

// Rebuilt from ntrtwl's NitroSDK
static void MBi_UnlockScanTarget(void) {
    if (mbrsp->nowScanTargetFlag == 0) {
        return;
    }
    if (sUnlockFunc != NULL) {
        sUnlockFunc();
    }
    mbrsp->scanCountUnit = MB_SCAN_COUNT_UNIT_NORMAL;
    mbrsp->nowScanTargetFlag = 0;
    mbrsp->notFoundLockTargetCount = 0;
}

// Rebuilt from ntrtwl's NitroSDK: forgets a parent
void MB_DeleteRecvGameInfo(int index) {
    mbrsp->usefulGameInfoFlag &= ~(1 << index);
    mbrsp->validGameInfoFlag &= ~(1 << index);
    MI_CpuClear16(&mbrsp->list[index], sizeof(MBGameInfoRecvList));
}

// Rebuilt from ntrtwl's NitroSDK: forgets a parent's record but keeps the beacon it was heard with
void MB_DeleteRecvGameInfoWithoutBssdesc(int index) {
    mbrsp->usefulGameInfoFlag &= ~(1 << index);
    mbrsp->validGameInfoFlag &= ~(1 << index);
    mbrsp->list[index].getFlagmentFlag = 0;
    mbrsp->list[index].allFlagmentFlag = 0;
    mbrsp->list[index].getPlayerFlag = 0;
    mbrsp->list[index].linkLevel = 0;
    MI_CpuClear16(&mbrsp->list[index].gameInfo, sizeof(MBGameInfo));
}

// Rebuilt from ntrtwl's NitroSDK: a parent's entry, once its record came in whole
MBGameInfoRecvList *MB_GetGameInfoRecvList(int index) {
    if ((mbrsp->usefulGameInfoFlag & (1 << index)) == 0) {
        return NULL;
    }
    return &mbrsp->list[index];
}

// Rebuilt from ntrtwl's NitroSDK: what MBi_SendVolatBeacon calls, and whether before or after the beacon
void MB_SetSendVolatCallback(MBSendVolatCallbackFunc callback, u32 timing) {
    OSIntrMode irq = OS_DisableInterrupts();

    sSendVolatCallback = callback;
    sSendVolatCallbackTimming = timing;
    OS_RestoreInterrupts(irq);
}

// Rebuilt from ntrtwl's NitroSDK: sets the data that a program's beacons carry to the children
void MB_SetUserVolatData(u32 ggid, const u8 *userData, u32 size) {
    MBGameInfo *record = mbss.gameInfoListTop;
    OSIntrMode irq;
    u32 i;

    if (record == NULL) {
        return;
    }
#ifdef BUGFIX
    while (record != NULL && record->ggid != ggid) {
        record = record->nextp;
    }
    if (record == NULL) {
        return;
    }
#else
    // BUG: the end of the list is tested after the record's game ID is read, so a game ID that no program has reads
    // through NULL before the loop ends
    while (record->ggid != ggid) {
        if (record == NULL) {
            return;
        }
        record = record->nextp;
    }
#endif
    irq = OS_DisableInterrupts();
    for (i = 0; i < size; i++) {
        record->params[i] = userData[i];
    }
    OS_RestoreInterrupts(irq);
}

// Rebuilt from ntrtwl's NitroSDK: the data that a multiboot parent's beacon carries, if it carries a program's
// players
void *MB_GetUserVolatData(const WMGameInfo *gameInfo) {
    MbBeacon *beacon;

    if (!(gameInfo->gameNameCount_attribute & WM_ATTR_FLAG_MB)) {
        return NULL;
    }
    beacon = (MbBeacon *)gameInfo->userGameInfo;
    if (beacon->dataAttr != MB_BEACON_DATA_ATTR_VOLAT) {
        return NULL;
    }
    return beacon->data.members.params;
}
