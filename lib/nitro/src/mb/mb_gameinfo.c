// The multiboot parent's beacon: the description of each program on offer, cut into pieces that go out one by one in
// the game info of the wireless beacon (libmb). The names are descriptive
#include "mb_private.h"

// What the beacon sends: the list of programs, the one being sent, and how far it is
static MBGameInfoSender sSender;
// The beacon that the sender fills, an object of its own after the sender's state
static MBBeacon sBeacon;

static u16 MBGameInfo_Checksum(const u16 *data, int length);
static BOOL MBGameInfo_LoadIconFile(const char *path, MBGameInfoRecord *record, BOOL isChar);
static int MBGameInfo_CountChars(const u16 *str);
static void MBGameInfo_ResetCounters(void);
static int MBGameInfo_PickRecord(void);
static void MBGameInfo_StartFragments(void);
static void MBGameInfo_SendFragment(u32 ggid, u16 tgid, u8 attr);
static void MBGameInfo_StartMembers(void);
static void MBGameInfo_SendMembers(u32 ggid, u16 tgid, u8 attr);

static u16 MBGameInfo_Checksum(const u16 *data, int length) {
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
void MBGameInfo_Build(MBGameInfoRecord *record, const MBGameRegistry *reg, const MBUserInfo *parentUser) {
    BOOL charFailed;
    BOOL paletteFailed;

    MI_CpuClear16(record, sizeof(MBGameInfoRecord));
    record->iconError = 0;
    charFailed = !MBGameInfo_LoadIconFile(reg->iconCharPathp, record, TRUE);
    paletteFailed = !MBGameInfo_LoadIconFile(reg->iconPalettePathp, record, FALSE);
    if ((charFailed | paletteFailed) != 0) {
        record->iconError = 1;
        MI_CpuClearFast(record, 0x220);
    }
    record->ggid = reg->ggid;
    if (parentUser != NULL) {
        MI_CpuCopy16(parentUser, &record->parentUser, sizeof(MBUserInfo));
    }
    record->maxPlayers = reg->maxPlayerNum;
    MI_CpuCopy16(reg->gameNamep, record->gameName, (u16)(MBGameInfo_CountChars(reg->gameNamep) * 2));
    MI_CpuCopy16(reg->gameIntroductionp, record->gameIntro, sizeof(record->gameIntro));
    record->playerCount = 1;
    record->memberMask = 1;
    record->sentMask = 1;
}

// Reads one of a program's two picture files, which must be of the right size
static BOOL MBGameInfo_LoadIconFile(const char *path, MBGameInfoRecord *record, BOOL isChar) {
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
    finit(&file);
    if (!romfs_fopen_core(&file, path, FS_FILEMODE_R)) {
        return FALSE;
    }
    if (size != GetFileSize(&file)) {
        romfs_fclose(&file);
        return FALSE;
    }
    romfs_fread(&file, dst, size);
    romfs_fclose(&file);
    return TRUE;
}

// Takes the players of the children in a bitmap into a program's record
void MBGameInfo_Update(MBGameInfoRecord *record, const MBUserInfo *users, u16 memberMask, u16 changedMask) {
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
    record->updateCount++;
}

static int MBGameInfo_CountChars(const u16 *str) {
    int count = 0;

    while (*str++ != 0) {
        count++;
    }
    return count;
}

// Adds a program to the end of the list
void MBGameInfo_Link(MBGameInfoRecord *record) {
    MBGameInfoRecord *last = sSender.head;

    if (last == NULL) {
        sSender.head = record;
    } else {
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = record;
    }
    record->next = NULL;
}

void MBGameInfo_Init(void) {
    sSender.head = NULL;
    sSender.current = NULL;
    sSender.state = MB_BEACON_STATE_PICK;
    sSender.callback = NULL;
    MBGameInfo_ResetCounters();
}

static void MBGameInfo_ResetCounters(void) {
    sSender.unk_25 = 0;
    sSender.updateCount = 0;
    sSender.fragmentIndex = 0;
    sSender.fragmentCount = 0;
    sSender.sequence = 0;
}

// Does the next step of the beacon's cycle: each call sets the game info of one beacon
void MBGameInfo_Send(u32 ggid, u16 tgid, u8 attr) {
    for (;;) {
        switch (sSender.state) {
        case MB_BEACON_STATE_PICK_FIRST:
        case MB_BEACON_STATE_PICK:
            if (!MBGameInfo_PickRecord()) {
                return;
            }
            break;
        case MB_BEACON_STATE_START_FRAGMENTS:
            MBGameInfo_StartFragments();
            break;
        case MB_BEACON_STATE_FRAGMENTS:
            MBGameInfo_SendFragment(ggid, tgid, attr);
            return;
        case MB_BEACON_STATE_START_MEMBERS:
            MBGameInfo_StartMembers();
            break;
        case MB_BEACON_STATE_MEMBERS:
            MBGameInfo_SendMembers(ggid, tgid, attr);
            return;
        case 6: // A state that nothing in this file sets
            break;
        }
    }
}

// Goes on to the next program of the list, or tells the children that there is none
static int MBGameInfo_PickRecord(void) {
    MBGameInfoRecord *record;

    if (sSender.head == NULL) {
        u32 ggid = MBWm_GetGgid();
        u16 tgid = MBWm_GetTgid();

        func_020824bc(NULL, &sBeacon, sizeof(MBBeacon), ggid, tgid, MB_BEACON_ATTR_NO_GAME);
        return 0;
    }
    if (sSender.current == NULL || (record = sSender.current->next) == NULL) {
        record = sSender.head;
    }
    sSender.current = record;
    MBGameInfo_ResetCounters();
    sSender.updateCount = sSender.current->updateCount;
    sSender.state = MB_BEACON_STATE_START_FRAGMENTS;
    return 1;
}

// Starts to cut the program's record into pieces, leaving out its picture if that could not be read
static void MBGameInfo_StartFragments(void) {
    if (sSender.state == MB_BEACON_STATE_START_FRAGMENTS) {
        MBGameInfoRecord *record = sSender.current;

        if (record->iconError == 0) {
            sSender.fragmentCount = 9;
            sSender.source = (u8 *)record;
        } else {
            sSender.fragmentCount = 4;
            sSender.source = (u8 *)&record->parentUser;
        }
        sSender.state = MB_BEACON_STATE_FRAGMENTS;
    }
}

// Sends the next piece of the record
static void MBGameInfo_SendFragment(u32 ggid, u16 tgid, u8 attr) {
    const u8 *recordEnd = (const u8 *)&sSender.current->playerCount;

    if (sSender.source + MB_BEACON_FRAGMENT_SIZE <= recordEnd) {
        sBeacon.u.fragment.length = MB_BEACON_FRAGMENT_SIZE;
    } else {
        sBeacon.u.fragment.length = recordEnd - sSender.source;
        MI_CpuFill16(&sBeacon.u.fragment.data[sBeacon.u.fragment.length], 0,
                     MB_BEACON_FRAGMENT_SIZE - sBeacon.u.fragment.length);
    }
    MI_CpuCopy16(sSender.source, sBeacon.u.fragment.data, sBeacon.u.fragment.length);
    sBeacon.u.fragment.index = sSender.fragmentIndex;
    sBeacon.u.fragment.count = sSender.fragmentCount;
    sBeacon.kind = sSender.current->iconError;
    sBeacon.gameNo = sSender.current->gameNo;
    sBeacon.updateCount = sSender.updateCount;
    sBeacon.ggid = sSender.current->ggid;
    sBeacon.slot = sSender.current->slot;
    sBeacon.sequence = sSender.sequence++;
    sBeacon.checksum = 0;
    sBeacon.checksum = MBGameInfo_Checksum(&sBeacon.checksum, MB_BEACON_CHECKED_SIZE);
    sSender.fragmentIndex++;
    if (sSender.fragmentIndex < sSender.fragmentCount) {
        sSender.source += MB_BEACON_FRAGMENT_SIZE;
    } else {
        sSender.state = MB_BEACON_STATE_START_MEMBERS;
    }
    // 1 is the wireless manager's entry attribute: children may connect
    func_020824bc(NULL, &sBeacon, sizeof(MBBeacon), ggid, tgid, attr | MB_BEACON_ATTR_MULTIBOOT | 1);
}

// Starts to send who is in the program's game
static void MBGameInfo_StartMembers(void) {
    sSender.current->sentMask = 1;
    sSender.updateCount = sSender.current->updateCount;
    sSender.state = MB_BEACON_STATE_MEMBERS;
}

// Sends up to four of the players that the children have not seen yet
static void MBGameInfo_SendMembers(u32 ggid, u16 tgid, u8 attr) {
    u16 missing;
    u32 sent;
    int i;

    if (sSender.updateCount != sSender.current->updateCount) {
        MBGameInfo_StartMembers();
    }
    sBeacon.kind = MB_BEACON_KIND_MEMBERS;
    sBeacon.gameNo = sSender.current->gameNo;
    sBeacon.updateCount = sSender.updateCount;
    sBeacon.ggid = sSender.current->ggid;
    sBeacon.slot = sSender.current->slot;
    sBeacon.sequence = sSender.sequence++;
    sBeacon.u.members.playerCount = sSender.current->playerCount;
    sBeacon.u.members.memberMask = sSender.current->memberMask;
    sBeacon.u.members.changedMask = sSender.current->changedMask;
    if (sSender.mode == 0 && sSender.callback != NULL) {
        sSender.callback(sSender.current->ggid);
    }
    {
        u8 *params = sBeacon.u.members.params;

        for (i = 0; i < 8; i++) {
            params[i] = sSender.current->params[i];
        }
    }
    MI_CpuClear16(sBeacon.u.members.users, sizeof(sBeacon.u.members.users));
    missing = sSender.current->sentMask ^ sSender.current->memberMask;
    sent = 0;
    for (i = 0; i < 15; i++) {
        if ((2 << i) & missing) {
            MI_CpuCopy16(&sSender.current->members[i], &sBeacon.u.members.users[sent], sizeof(MBUserInfo));
            sSender.current->sentMask |= (u16)(2 << i);
            sent++;
            if (sent == 4) {
                break;
            }
        }
    }
    if (sent < 4) {
        sBeacon.u.members.users[sent].playerNo = 0;
    }
    sBeacon.checksum = 0;
    sBeacon.checksum = MBGameInfo_Checksum(&sBeacon.checksum, MB_BEACON_CHECKED_SIZE);
    if (sSender.current->sentMask == sSender.current->memberMask) {
        sSender.state = MB_BEACON_STATE_PICK;
    }
    // 1 is the wireless manager's entry attribute: children may connect
    func_020824bc(NULL, &sBeacon, sizeof(MBBeacon), ggid, tgid, attr | MB_BEACON_ATTR_MULTIBOOT | 1);
    if (sSender.mode == 1 && sSender.callback != NULL) {
        sSender.callback(sSender.current->ggid);
    }
}
