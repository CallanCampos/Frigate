// The parent side of the multiboot library's talk with its children: it keeps each child's state, takes in what the
// children send and chooses the next frame to send (libmb). The names are descriptive, except swan's MB_Comm*
#include "mb_private.h"

// The file whose block was sent last, so that the next frame goes to the next file
static u8 sLastFile = 0xff;
// The files that a child asked a block of in this round of frames
static u32 sBlockRequestMask;

static BOOL MBComm_IsValidAid(u16 aid);
static void MBComm_NotifyState(u16 aid, u32 state, void *arg);
static void MBComm_SetState(u16 aid, u32 state, void *arg);
static void MBComm_ProcessChildFrame(const WMMpRecvData *recv, u16 aid);
static void MBComm_ReadMPData(const WMMpRecvBuf *mp);
static int MBComm_SendFrame(u8 type, u16 aidBitmap);
static int MBComm_SendFileInfo(void);
static void MBComm_LoadCacheBlock(MBTask *task);
static int MBComm_SendBlock(void);
static void MBComm_SendNextFrame(void);
static void MBComm_AdvanceSendCursor(u32 file);
static u16 MBComm_MaxBlock(u32 a, u32 b);
static void MBComm_NotifyError(u16 aid, u16 errcode);

void MB_CommSetParentStateCallback(MBCommPStateCallback callback) {
    u32 irq = CPU_IRQDisable();

    gMBCommWork->callback = callback;
    CPU_SetIRQMask(irq);
}

const MBUserInfo *MB_CommGetChildUser(u16 childAid) {
    u32 irq = CPU_IRQDisable();

    if (gMBCommWork != NULL && MBComm_IsValidAid(childAid)) {
        sys_memcpy(&gMBCommWork->childUser[childAid - 1], &gMBCommWork->childUserCopy, sizeof(MBUserInfo));
        CPU_SetIRQMask(irq);
        return &gMBCommWork->childUserCopy;
    }
    CPU_SetIRQMask(irq);
    return NULL;
}

BOOL MB_CommIsBootable(u16 childAid) {
    if (gMBCommWork != NULL && MBComm_IsValidAid(childAid)
        && gMBCommWork->childState[childAid - 1] == MB_COMM_PSTATE_SEND_COMPLETE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MB_CommResponseRequest(u16 childAid, MBCommResponseRequestType ack) {
    u32 irq = CPU_IRQDisable();
    u16 response;
    u16 state;

    switch (ack) {
    case MB_COMM_RESPONSE_REQUEST_KICK:
        state = MB_COMM_PSTATE_REQUESTED;
        response = MB_COMM_RESPONSE_KICK;
        break;
    case MB_COMM_RESPONSE_REQUEST_ACCEPT:
        state = MB_COMM_PSTATE_REQUESTED;
        response = MB_COMM_RESPONSE_ACCEPT;
        break;
    case MB_COMM_RESPONSE_REQUEST_DOWNLOAD:
        state = MB_COMM_PSTATE_WAIT_TO_SEND;
        response = MB_COMM_RESPONSE_DOWNLOAD;
        break;
    case MB_COMM_RESPONSE_REQUEST_BOOT:
        state = MB_COMM_PSTATE_SEND_COMPLETE;
        response = MB_COMM_RESPONSE_BOOT;
        break;
    default:
        CPU_SetIRQMask(irq);
        return FALSE;
    }

    if (gMBCommWork != NULL && MBComm_IsValidAid(childAid)
        && state == gMBCommWork->childState[childAid - 1]) {
        gMBCommWork->response[childAid - 1] = response;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    CPU_SetIRQMask(irq);
    return FALSE;
}

// Records a child's state and tells the callback
static void MBComm_SetState(u16 aid, u32 state, void *arg) {
    if (MBComm_IsValidAid(aid)) {
        gMBCommWork->childState[aid - 1] = state;
    }
    MBComm_NotifyState(aid, state, arg);
}

static void MBComm_NotifyState(u16 aid, u32 state, void *arg) {
    MBCommWork *work = gMBCommWork;

    if (work->callback != NULL) {
        work->callback(aid, state, arg);
    }
}

// What the library's wireless layer reports to the parent
void MBComm_OnWmEvent(u16 event, void *arg) {
    switch (event) {
    case MB_WM_EVENT_INIT_COMPLETE:
        MBComm_SetState(0, MB_COMM_PSTATE_INIT_COMPLETE, arg);
        break;
    case MB_WM_EVENT_CONNECTED: {
        WMStartParentCallback *cb = arg;

        if (cb->aid != 0 && cb->aid < 16) {
            MBComm_SetState(cb->aid, MB_COMM_PSTATE_CONNECTED, arg);
        }
        break;
    }
    case MB_WM_EVENT_DISCONNECTED: {
        WMStartParentCallback *cb = arg;
        MBCommWork *work;
        u16 aid = cb->aid;

        if (aid == 0 || aid >= 16) {
            break;
        }
        gMBCommWork->childUnk148a[aid - 1] = 0;
        sys_memset(&gMBCommWork->childRequestGgid[aid - 1], 0, sizeof(u32));
        sys_memset(&gMBCommWork->childUser[aid - 1], 0, sizeof(MBUserInfo));
        MBFrame_ClearChild(aid);
        gMBCommWork->response[aid - 1] = 0;
        work = gMBCommWork;
        if (work->childFile[aid - 1] != -1) {
            MBCommFile *file = &work->files[(u8)work->childFile[aid - 1]];
            u16 bit = 1 << aid;

            file->memberMask &= (u16)~bit;
            file->changedMask |= bit;
            work->childFile[aid - 1] = -1;
            file->downloadMask &= (u16)~bit;
        }
        work = gMBCommWork;
        if (work->childBitmap & (1 << aid)) {
            work->childCount--;
            work->childBitmap &= (u16)~(1 << aid);
        }
        if (gMBCommWork->childState[aid - 1] == MB_COMM_PSTATE_BOOT_REQUEST) {
            MBComm_SetState(aid, MB_COMM_PSTATE_BOOT_STARTABLE, NULL);
        }
        MBComm_SetState(aid, MB_COMM_PSTATE_DISCONNECTED, arg);
        gMBCommWork->childState[aid - 1] = MB_COMM_PSTATE_NONE;
        break;
    }
    case MB_WM_EVENT_MP_RECEIVED:
        MBComm_ReadMPData(arg);
        break;
    case MB_WM_EVENT_MP_READY:
        MBComm_SendNextFrame();
        break;
    case MB_WM_EVENT_BEACON_SENT: {
        u8 i;

        for (i = 0; i < 16; i++) {
            MBCommFile *file = &gMBCommWork->files[i];

            if (file->valid && file->changedMask != 0) {
                MBGameInfo_Update(&file->record, gMBCommWork->childUser, file->memberMask, file->changedMask);
                file->changedMask = 0;
            }
        }
        MBGameInfo_Send(MBWm_GetGgid(), MBWm_GetTgid(), MBWm_GetAttribute());
        break;
    }
    case MB_WM_EVENT_ERROR: {
        WMCallback *cb = arg;

        switch (cb->errcode) {
        case 1:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
            MBComm_NotifyError(0, MB_ERRCODE_FATAL);
            break;
        default:
            MBComm_NotifyError(0, MB_ERRCODE_WM_FAILURE);
            break;
        }
        break;
    }
    case MB_WM_EVENT_WM_ERROR: {
        WMCallback *cb = arg;

        switch (cb->apiid) {
        case 0:
        case 7:
        case 8:
        case 13:
        case 14:
        case 15:
        case 17:
        case 18:
        case 21:
        case 25:
        case 29:
            MBComm_NotifyError(0, MB_ERRCODE_FATAL);
            break;
        default:
            MBComm_NotifyError(0, MB_ERRCODE_WM_FAILURE);
            break;
        }
        break;
    }
    }

    if (event == MB_WM_EVENT_END) {
        MBCommWork *work = gMBCommWork;
        MBCommPStateCallback callback = work->callback;

        MI_CpuClearFast(work, sizeof(MBCommWork));
        gMBCommWork = NULL;
        if (callback != NULL) {
            callback(0, MB_COMM_PSTATE_END, NULL);
        }
    }
}

// Takes in a frame that a child sent
static void MBComm_ProcessChildFrame(const WMMpRecvData *recv, u16 aid) {
    MBCommWork *work = gMBCommWork;
    MBChildFrame frame;
    const u8 *request;
    u32 state;

    if (aid == 0 || aid > 15) {
        return;
    }
    request = MBFrame_Decode(recv->data, &frame, aid);
    state = work->childState[aid - 1];

    switch (frame.type) {
    case MB_FRAME_REQUEST:
        if (request == NULL) {
            return;
        }
        if (state == MB_COMM_PSTATE_CONNECTED) {
            u8 buf[0x1d];
            MBUserInfo *user;

            sys_memcpy(request, buf, sizeof(buf));
            gMBCommWork->childRequestGgid[aid - 1] = *(u32 *)buf;
            gMBCommWork->childUnk148a[aid - 1] = *(u16 *)&buf[0x1a];
            user = (MBUserInfo *)&buf[4];
            sys_memcpy(user, &gMBCommWork->childUser[aid - 1], sizeof(MBUserInfo));
            gMBCommWork->childUser[aid - 1].playerNo = aid;
            MBComm_SetState(aid, MB_COMM_PSTATE_REQUESTED, user);
        }
        if (state == MB_COMM_PSTATE_REQUESTED) {
            u8 file = request[0x1c];
            u8 count = 0;
            u8 i;

            if (file < 16 && gMBCommWork->files[file].valid
                && gMBCommWork->childRequestGgid[aid - 1] == gMBCommWork->files[file].registry->ggid) {
                for (i = 0; i < 16; i++) {
                    if (gMBCommWork->files[file].memberMask & (1 << i)) {
                        count++;
                    }
                }
                if (count >= gMBCommWork->files[file].registry->maxPlayerNum) {
                    gMBCommWork->response[aid - 1] = 0;
                    MBComm_SetState(aid, MB_COMM_PSTATE_MEMBER_FULL, NULL);
                    return;
                }
            } else {
                gMBCommWork->response[aid - 1] = MB_COMM_RESPONSE_KICK;
            }

            if (gMBCommWork->response[aid - 1] == MB_COMM_RESPONSE_ACCEPT) {
                u16 bit = 1 << aid;

                if (gMBCommWork->childBitmap & bit) {
                    return;
                }
                gMBCommWork->childCount++;
                gMBCommWork->childBitmap |= bit;
                gMBCommWork->childFile[aid - 1] = file;
                gMBCommWork->files[file].memberMask |= bit;
                gMBCommWork->files[file].changedMask |= bit;
                gMBCommWork->response[aid - 1] = 0;
                MBComm_SetState(aid, MB_COMM_PSTATE_REQ_ACCEPTED, NULL);
            } else if (gMBCommWork->response[aid - 1] == MB_COMM_RESPONSE_KICK) {
                gMBCommWork->response[aid - 1] = 0;
                MBComm_SetState(aid, MB_COMM_PSTATE_KICKED, NULL);
            }
        }
        break;
    case MB_FRAME_DOWNLOAD_REQUEST:
        if (state == MB_COMM_PSTATE_REQ_ACCEPTED) {
            MBComm_SetState(aid, MB_COMM_PSTATE_WAIT_TO_SEND, NULL);
        } else if (state == MB_COMM_PSTATE_WAIT_TO_SEND) {
            if (work->response[aid - 1] == MB_COMM_RESPONSE_DOWNLOAD) {
                MBCommFile *file = &work->files[(u8)work->childFile[aid - 1]];

                file->downloadMask |= 1 << aid;
                file->sendCursor = 0;
                work->response[aid - 1] = 0;
                MBComm_SetState(aid, MB_COMM_PSTATE_SEND_PROCEED, NULL);
            }
        }
        break;
    case MB_FRAME_BLOCK_REQUEST:
        if (state == MB_COMM_PSTATE_SEND_PROCEED) {
            u8 file = work->childFile[aid - 1];

            if (file != 0xff) {
                work->files[file].requestedBlock = MBComm_MaxBlock(work->files[file].requestedBlock, frame.value);
                sBlockRequestMask |= 1 << file;
            }
        }
        break;
    case MB_FRAME_DOWNLOAD_DONE:
        if (state == MB_COMM_PSTATE_SEND_PROCEED) {
            u8 file = work->childFile[aid - 1];

            if (file != 0xff) {
                work->files[file].downloadMask &= (u16)~(1 << aid);
                MBComm_SetState(aid, MB_COMM_PSTATE_SEND_COMPLETE, NULL);
            }
        } else if (state == MB_COMM_PSTATE_SEND_COMPLETE) {
            if (work->response[aid - 1] == MB_COMM_RESPONSE_BOOT) {
                work->response[aid - 1] = 0;
                MBComm_SetState(aid, MB_COMM_PSTATE_BOOT_REQUEST, NULL);
            }
        }
        break;
    }
}

// Reads what every child sent in the last round
static void MBComm_ReadMPData(const WMMpRecvBuf *mp) {
    u16 i;
    u16 aid;

    for (i = 0; i < 16; i++) {
        if (gMBCommWork->files[i].valid) {
            gMBCommWork->files[i].requestedBlock = 0;
        }
    }
    sBlockRequestMask = 0;
    for (aid = 1; aid <= 15; aid++) {
        const WMMpRecvData *recv = func_02081040(mp, aid);

        if (recv != NULL && recv->length != 0xffff && recv->length != 0) {
            MBComm_ProcessChildFrame(recv, aid);
        }
    }
}

// Sends a frame with nothing but its type
static int MBComm_SendFrame(u8 type, u16 aidBitmap) {
    MBFrame frame;

    frame.type = type;
    MBFrame_Encode(&frame, gMBCommWork->sendBuf);
    return MBComm_FlushAndSend(6, aidBitmap, gMBCommWork->sendBuf);
}

// Sends the description of one file to the children that were accepted and want it, the files in turn
static int MBComm_SendFileInfo(void) {
    u8 count[16];
    MBFrame frame;
    s8 file = -1;
    u16 aidBitmap = 0;
    u8 *out;
    u16 aid;
    u8 i;
    MBCommWork *work;

    sys_memset(count, 0, sizeof(count));
    work = gMBCommWork;
    for (aid = 1; aid <= 15; aid++) {
        if (work->childState[aid - 1] == MB_COMM_PSTATE_REQ_ACCEPTED) {
            count[work->childFile[aid - 1]]++;
        }
    }
    {
        u8 next = sLastFile;

        for (i = 0; i < 16; i++) {
            next = (next + 1) % 16;
            if (work->files[next].valid && count[next] != 0) {
                file = next;
                break;
            }
        }
    }
    if (file == -1) {
        return MB_SEND_NOTHING;
    }
    sLastFile = file;
    for (aid = 1; aid <= 15; aid++) {
        if (work->childState[aid - 1] == MB_COMM_PSTATE_REQ_ACCEPTED && file == work->childFile[aid - 1]) {
            aidBitmap |= 1 << aid;
        }
    }
    frame.type = MB_FRAME_FILE_INFO;
    frame.value = file;
    out = MBFrame_Encode(&frame, work->sendBuf);
    if (out != NULL) {
        sys_memcpy(&gMBCommWork->files[file], out, 0xe4);
    }
    return MBComm_FlushAndSend(0xea, aidBitmap, gMBCommWork->sendBuf);
}

// Reads the part of the program that a child will want soon into the cache
static void MBComm_LoadCacheBlock(MBTask *task) {
    MBCache *cache = task->arg1;
    MBCacheEntry *entry = task->arg0;
    u32 size = entry->size;
    FSFile file;
    FSArchive *archive;

    archive = func_0207062c(cache->archiveName, cache->nameLength);
    if (archive == NULL) {
        archive = cache->archive;
    }
    if (hw_isDSi()) {
        if (archive == func_0207062c("rom", 3)) {
            const MBTwlRomHeader *header = func_0206eee8();
            u32 firstEnd = header->ntrRegionOffset + header->ntrRegionSize;
            u32 secondEnd = header->twlRegionOffset + header->twlRegionSize;
            u32 offset = entry->offset;

            if (offset < firstEnd) {
                if (offset + size > firstEnd) {
                    size = firstEnd - offset;
                }
            } else if (offset < secondEnd) {
                if (offset + size > secondEnd) {
                    size = secondEnd - offset;
                }
            }
        }
    }
    finit(&file);
    if (fopen_raw(&file, archive, entry->offset, entry->offset + size, -1)) {
        if (size == romfs_fread(&file, entry->buf, size)) {
            entry->state = MB_CACHE_ENTRY_READY;
        }
        romfs_fclose(&file);
    }
    if (entry->state != MB_CACHE_ENTRY_READY) {
        entry->offset = 0;
        entry->state = MB_CACHE_ENTRY_READY;
    }
}

// Sends the next block of a file to the children that are downloading it
static int MBComm_SendBlock(void) {
    MBBlockSource source;
    MBTask *task;
    u32 offsetInSegment;
    MBFrame frame;
    u32 tableOffset;
    MBCache *cache;
    u8 *out;
    u8 i;

    if (gMBCommWork->fileCount == 0) {
        return MB_SEND_NOTHING;
    }
    for (i = 0; i < 16; i++) {
        gMBCommWork->fileCursor = (gMBCommWork->fileCursor + 1) % 16;
        if (gMBCommWork->files[gMBCommWork->fileCursor].valid
            && gMBCommWork->files[gMBCommWork->fileCursor].downloadMask != 0) {
            break;
        }
    }
    if (i == 16) {
        return MB_SEND_NOTHING;
    }
    MBComm_AdvanceSendCursor(gMBCommWork->fileCursor);
    if (!MBFile_LocateBlock(&source, &gMBCommWork->files[gMBCommWork->fileCursor].layout,
                            gMBCommWork->files[gMBCommWork->fileCursor].sendCursor,
                            &gMBCommWork->files[gMBCommWork->fileCursor])) {
        return MB_SEND_NOTHING;
    }
    frame.type = MB_FRAME_BLOCK;
    frame.value = gMBCommWork->fileCursor;
    frame.value2 = gMBCommWork->files[gMBCommWork->fileCursor].sendCursor;
    out = MBFrame_Encode(&frame, gMBCommWork->sendBuf);
    offsetInSegment = source.bufAddr
                      - gMBCommWork->files[gMBCommWork->fileCursor].layout.segOffset[source.segment];
    tableOffset = gMBCommWork->files[gMBCommWork->fileCursor].segmentRomOffsets[source.segment];
    cache = gMBCommWork->files[gMBCommWork->fileCursor].cache;
    if (!MBCache_Read(cache, offsetInSegment + tableOffset, out, source.size)) {
        task = &gMBCommWork->task;
        if (!MBTask_IsBusy(task)) {
            if (cache->missCounter != 0) {
                cache->missCounter--;
            } else {
                MBCacheEntry *oldest = NULL;
                int n;

                for (n = 0; n < MB_CACHE_ENTRY_COUNT; n++) {
                    MBCacheEntry *candidate = &cache->entries[n];

                    if (candidate->state == MB_CACHE_ENTRY_READY
                        && (oldest == NULL || oldest->offset > candidate->offset)) {
                        oldest = candidate;
                    }
                }
                if (oldest == NULL) {
                    sys_exit();
                }
                cache->missCounter = 2;
                oldest->state = MB_CACHE_ENTRY_LOADING;
                oldest->offset = (offsetInSegment + tableOffset) & ~0x1f;
                task->arg0 = oldest;
                task->arg1 = cache;
                MBTask_Add(task, MBComm_LoadCacheBlock, NULL, 4);
            }
        }
        return MB_SEND_NOTHING;
    }
    return MBComm_FlushAndSend(source.size + 6, gMBCommWork->files[gMBCommWork->fileCursor].downloadMask,
                               gMBCommWork->sendBuf);
}

// Sends what the children's states ask for: the frames that tell them how it went, else a block of a file
static void MBComm_SendNextFrame(void) {
    u16 want[5];
    u16 aid;
    int result;

    sys_memset16(0, want, sizeof(want));
    for (aid = 1; aid <= 15; aid++) {
        switch (gMBCommWork->childState[aid - 1]) {
        case MB_COMM_PSTATE_CONNECTED:
            want[0] |= 1 << aid;
            break;
        case MB_COMM_PSTATE_REQ_ACCEPTED:
            want[1] |= 1 << aid;
            break;
        case MB_COMM_PSTATE_KICKED:
            want[2] |= 1 << aid;
            break;
        case MB_COMM_PSTATE_BOOT_REQUEST:
            want[3] |= 1 << aid;
            break;
        case MB_COMM_PSTATE_MEMBER_FULL:
            want[4] |= 1 << aid;
            break;
        }
    }

    if (want[3] != 0) {
        result = MBComm_SendFrame(MB_FRAME_BOOT, want[3]);
    } else if (want[0] != 0) {
        result = MBComm_SendFrame(MB_FRAME_CONNECTED, want[0]);
    } else if (want[4] != 0) {
        result = MBComm_SendFrame(MB_FRAME_MEMBER_FULL, want[4]);
    } else if (want[2] != 0) {
        result = MBComm_SendFrame(MB_FRAME_KICK, want[2]);
    } else if (want[1] != 0) {
        result = MBComm_SendFileInfo();
    } else {
        result = MBComm_SendBlock();
    }
    if (result == MB_SEND_NOTHING) {
        MBComm_SendFrame(MB_FRAME_NONE, 0xffff);
    }
}

// Moves the block that a file sends next on, or back to the one that a child asked for
static void MBComm_AdvanceSendCursor(u32 file) {
    MBCommFile *entry;

    if ((sBlockRequestMask & (1 << file)) == 0) {
        return;
    }
    entry = &gMBCommWork->files[file];
    if (entry->valid == 0 || entry->downloadMask == 0) {
        return;
    }
    if (entry->requestedBlock <= entry->sendCursor && entry->sendCursor <= entry->requestedBlock + 2) {
        entry->sendCursor++;
    } else {
        entry->sendCursor = entry->requestedBlock;
    }
}

static u16 MBComm_MaxBlock(u32 a, u32 b) {
    if (b <= a) {
        b = a;
    }
    return b;
}

static BOOL MBComm_IsValidAid(u16 aid) {
    if (aid >= 1 && aid <= 15) {
        return TRUE;
    }
    return FALSE;
}

// Tells the callback about an error
static void MBComm_NotifyError(u16 aid, u16 errcode) {
    MBErrorStatus status;

    status.errcode = errcode;
    MBComm_NotifyState(aid, MB_COMM_PSTATE_ERROR, &status);
}
