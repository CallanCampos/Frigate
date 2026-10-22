// The parent side of the multiboot library's talk with its children: it keeps each child's state, takes in what the
// children send and chooses the next frame to send (libmb). The names are descriptive, except swan's MB_Comm*
#include "mb_private.h"

// The file whose block was sent last, so that the next frame goes to the next file
static u8 sLastFile = 0xff;
// The files that a child asked a block of in this round of frames
static u32 any_recv_bitmap;

static BOOL IsChildAidValid(u16 aid);
static void MBi_CommChangeParentStateCallbackOnly(u16 aid, u32 state, void *arg);
static void MBi_CommChangeParentState(u16 aid, u32 state, void *arg);
static void MBi_CommParentRecvDataPerChild(const WMMpRecvData *recv, u16 aid);
static void MBi_CommParentRecvData(const WMMpRecvHeader *mp);
static int MBi_CommParentSendMsg(u8 type, u16 aidBitmap);
static int MBi_CommParentSendDLFileInfo(void);
static void MBi_ReloadCache(MBiTaskInfo *task);
static int MBi_CommParentSendBlock(void);
static void MBi_CommParentSendData(void);
static void MBi_calc_sendblock(u32 file);
static u16 MBi_calc_nextsendblock(u32 a, u32 b);
static void MBi_CommCallParentError(u16 aid, u16 errcode);

void MB_CommSetParentStateCallback(MBCommPStateCallback callback) {
    OSIntrMode irq = OS_DisableInterrupts();

    pPwork->parent_callback = callback;
    OS_RestoreInterrupts(irq);
}

const MBUserInfo *MB_CommGetChildUser(u16 childAid) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (mbc != NULL && IsChildAidValid(childAid)) {
        MI_CpuCopy8(&pPwork->childUser[childAid - 1], &pPwork->childUserBuf, sizeof(MBUserInfo));
        OS_RestoreInterrupts(irq);
        return &pPwork->childUserBuf;
    }
    OS_RestoreInterrupts(irq);
    return NULL;
}

BOOL MB_CommIsBootable(u16 childAid) {
    if (mbc != NULL && IsChildAidValid(childAid) &&
        pPwork->p_comm_state[childAid - 1] == MB_COMM_PSTATE_SEND_COMPLETE) {
        return TRUE;
    }
    return FALSE;
}

BOOL MB_CommResponseRequest(u16 childAid, MBCommResponseRequestType ack) {
    OSIntrMode irq = OS_DisableInterrupts();
    u16 response;
    u16 state;

    switch (ack) {
    case MB_COMM_RESPONSE_REQUEST_KICK:
        state = MB_COMM_PSTATE_REQUESTED;
        response = MB_COMM_USER_REQ_KICK;
        break;
    case MB_COMM_RESPONSE_REQUEST_ACCEPT:
        state = MB_COMM_PSTATE_REQUESTED;
        response = MB_COMM_USER_REQ_ACCEPT;
        break;
    case MB_COMM_RESPONSE_REQUEST_DOWNLOAD:
        state = MB_COMM_PSTATE_WAIT_TO_SEND;
        response = MB_COMM_USER_REQ_SEND_START;
        break;
    case MB_COMM_RESPONSE_REQUEST_BOOT:
        state = MB_COMM_PSTATE_SEND_COMPLETE;
        response = MB_COMM_USER_REQ_BOOT;
        break;
    default:
        OS_RestoreInterrupts(irq);
        return FALSE;
    }

    if (mbc != NULL && IsChildAidValid(childAid) && state == pPwork->p_comm_state[childAid - 1]) {
        pPwork->req2child[childAid - 1] = response;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    OS_RestoreInterrupts(irq);
    return FALSE;
}

// Records a child's state and tells the callback
static void MBi_CommChangeParentState(u16 aid, u32 state, void *arg) {
    if (IsChildAidValid(aid)) {
        pPwork->p_comm_state[aid - 1] = state;
    }
    MBi_CommChangeParentStateCallbackOnly(aid, state, arg);
}

static void MBi_CommChangeParentStateCallbackOnly(u16 aid, u32 state, void *arg) {
    MB_CommPWork *work = pPwork;

    if (work->parent_callback != NULL) {
        work->parent_callback(aid, state, arg);
    }
}

// What the library's wireless layer reports to the parent
void MBi_CommParentCallback(u16 event, void *arg) {
    switch (event) {
    case MB_CALLBACK_INIT_COMPLETE:
        MBi_CommChangeParentState(0, MB_COMM_PSTATE_INIT_COMPLETE, arg);
        break;
    case MB_CALLBACK_CHILD_CONNECTED: {
        WMStartParentCallback *cb = arg;

        if (cb->aid != 0 && cb->aid < 16) {
            MBi_CommChangeParentState(cb->aid, MB_COMM_PSTATE_CONNECTED, arg);
        }
        break;
    }
    case MB_CALLBACK_CHILD_DISCONNECTED: {
        WMStartParentCallback *cb = arg;

        if (cb->aid == 0 || cb->aid >= 16) {
            break;
        }
        pPwork->childversion[cb->aid - 1] = 0;
        MI_CpuFill8(&pPwork->childggid[cb->aid - 1], 0, sizeof(u32));
        MI_CpuFill8(&pPwork->childUser[cb->aid - 1], 0, sizeof(MBUserInfo));
        MBi_ClearParentPieceBuffer(cb->aid);
        pPwork->req2child[cb->aid - 1] = 0;
        MBComm_LeaveFile(cb->aid);
        if (pPwork->child_entry_bmp & (1 << cb->aid)) {
            pPwork->child_num--;
            pPwork->child_entry_bmp &= ~(1 << cb->aid);
        }
        if (pPwork->p_comm_state[cb->aid - 1] == MB_COMM_PSTATE_BOOT_REQUEST) {
            MBi_CommChangeParentState(cb->aid, MB_COMM_PSTATE_BOOT_STARTABLE, NULL);
        }
        MBi_CommChangeParentState(cb->aid, MB_COMM_PSTATE_DISCONNECTED, arg);
        pPwork->p_comm_state[cb->aid - 1] = MB_COMM_PSTATE_NONE;
        break;
    }
    case MB_CALLBACK_MP_PARENT_RECV:
        MBi_CommParentRecvData(arg);
        break;
    case MB_CALLBACK_END_COMPLETE:
        break;
    case MB_CALLBACK_MP_SEND_ENABLE:
        MBi_CommParentSendData();
        break;
    case MB_CALLBACK_BEACON_SENT: {
        u8 i;

        for (i = 0; i < 16; i++) {
            if (pPwork->fileinfo[i].valid && pPwork->fileinfo[i].changedMask != 0) {
                MB_UpdateGameInfoMember(&pPwork->fileinfo[i].record, pPwork->childUser, pPwork->fileinfo[i].memberMask,
                                        pPwork->fileinfo[i].changedMask);
                pPwork->fileinfo[i].changedMask = 0;
            }
        }
        MB_SendGameInfoBeacon(MBi_GetGgid(), MBi_GetTgid(), MBi_GetAttribute());
        break;
    }
    case MB_CALLBACK_API_ERROR: {
        WMCallback *cb = arg;

        switch (cb->errcode) {
        case 1:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
            MBi_CommCallParentError(0, MB_ERRCODE_FATAL);
            break;
        // The table of the switch runs to 15, so a case of that value goes with the default
        case 15:
        default:
            MBi_CommCallParentError(0, MB_ERRCODE_WM_FAILURE);
            break;
        }
        break;
    }
    case MB_CALLBACK_ERROR: {
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
            MBi_CommCallParentError(0, MB_ERRCODE_FATAL);
            break;
        // The original's switch is one table from 0 to 29, which takes at least two more cases that go with the
        // default; which ones can't be told
        case WM_APIID_RESET:
        case WM_APIID_END:
        default:
            MBi_CommCallParentError(0, MB_ERRCODE_WM_FAILURE);
            break;
        }
        break;
    }
    }

    if (event == MB_CALLBACK_END_COMPLETE) {
        MB_CommPWork *work = pPwork;
        MBCommPStateCallback callback = work->parent_callback;

        MI_CpuClearFast(work, sizeof(MB_CommPWork));
        mbc = NULL;
        if (callback != NULL) {
            callback(0, MB_COMM_PSTATE_END, NULL);
        }
    }
}

// Takes in a frame that a child sent
static void MBi_CommParentRecvDataPerChild(const WMMpRecvData *recv, u16 aid) {
    MBCommChildBlockHeader frame;
    const MBCommRequestData *request;
    u32 state;

    if (aid == 0 || aid > 15) {
        return;
    }
    request = MBi_SetRecvBufferFromChild((const u8 *)&recv->cdata[0], &frame, aid);
    state = pPwork->p_comm_state[aid - 1];

    switch (frame.type) {
    case MB_COMM_TYPE_CHILD_FILEREQ:
        if (request == NULL) {
            return;
        }
        if (state == MB_COMM_PSTATE_CONNECTED) {
            MBCommRequestData req;

            MI_CpuCopy8(request, &req, MB_COMM_REQ_DATA_SIZE);
            pPwork->childggid[aid - 1] = req.ggid;
            pPwork->childversion[aid - 1] = req.version;
            MI_CpuCopy8(&req.userinfo, &pPwork->childUser[aid - 1], sizeof(MBUserInfo));
            pPwork->childUser[aid - 1].playerNo = aid;
            MBi_CommChangeParentState(aid, MB_COMM_PSTATE_REQUESTED, &req.userinfo);
        }
        if (state == MB_COMM_PSTATE_REQUESTED) {
            u8 i;
            u8 count = 0;
            u8 file = request->fileid;

            if (file >= 16 || !pPwork->fileinfo[file].valid ||
                pPwork->childggid[aid - 1] != pPwork->fileinfo[file].registry->ggid) {
                pPwork->req2child[aid - 1] = MB_COMM_USER_REQ_KICK;
            } else {
                for (i = 0; i < 16; i++) {
                    if (pPwork->fileinfo[file].memberMask & (1 << i)) {
                        count++;
                    }
                }
                if (count >= pPwork->fileinfo[file].registry->maxPlayerNum) {
                    pPwork->req2child[aid - 1] = 0;
                    MBi_CommChangeParentState(aid, MB_COMM_PSTATE_MEMBER_FULL, NULL);
                    return;
                }
            }

            switch (pPwork->req2child[aid - 1]) {
            case MB_COMM_USER_REQ_ACCEPT:
                if (pPwork->child_entry_bmp & (1 << aid)) {
                    break;
                }
                pPwork->child_num++;
                pPwork->child_entry_bmp |= 1 << aid;
                pPwork->fileid_of_child[aid - 1] = file;
                pPwork->fileinfo[file].memberMask |= 1 << aid;
                pPwork->fileinfo[file].changedMask |= 1 << aid;
                pPwork->req2child[aid - 1] = 0;
                MBi_CommChangeParentState(aid, MB_COMM_PSTATE_REQ_ACCEPTED, NULL);
                break;
            case MB_COMM_USER_REQ_KICK:
                pPwork->req2child[aid - 1] = 0;
                MBi_CommChangeParentState(aid, MB_COMM_PSTATE_KICKED, NULL);
                break;
            }
        }
        break;
    case MB_COMM_TYPE_CHILD_ACCEPT_FILEINFO:
        if (state == MB_COMM_PSTATE_REQ_ACCEPTED) {
            MBi_CommChangeParentState(aid, MB_COMM_PSTATE_WAIT_TO_SEND, NULL);
        } else if (state == MB_COMM_PSTATE_WAIT_TO_SEND) {
            if (pPwork->req2child[aid - 1] == MB_COMM_USER_REQ_SEND_START) {
                u8 file = pPwork->fileid_of_child[aid - 1];

                pPwork->fileinfo[file].downloadMask |= 1 << aid;
                pPwork->fileinfo[file].sendCursor = 0;
                pPwork->req2child[aid - 1] = 0;
                MBi_CommChangeParentState(aid, MB_COMM_PSTATE_SEND_PROCEED, NULL);
            }
        }
        break;
    case MB_COMM_TYPE_CHILD_CONTINUE:
        if (state == MB_COMM_PSTATE_SEND_PROCEED) {
            u8 file = pPwork->fileid_of_child[aid - 1];

            if (file != 0xff) {
                pPwork->fileinfo[file].requestedBlock =
                    MBi_calc_nextsendblock(pPwork->fileinfo[file].requestedBlock, frame.value);
                any_recv_bitmap |= 1 << file;
            }
        }
        break;
    case MB_COMM_TYPE_CHILD_STOPREQ:
        if (state == MB_COMM_PSTATE_SEND_PROCEED) {
            u8 file = pPwork->fileid_of_child[aid - 1];

            if (file != 0xff) {
                pPwork->fileinfo[file].downloadMask &= (u16) ~(1 << aid);
                MBi_CommChangeParentState(aid, MB_COMM_PSTATE_SEND_COMPLETE, NULL);
            }
        } else if (state == MB_COMM_PSTATE_SEND_COMPLETE) {
            if (pPwork->req2child[aid - 1] == MB_COMM_USER_REQ_BOOT) {
                pPwork->req2child[aid - 1] = 0;
                MBi_CommChangeParentState(aid, MB_COMM_PSTATE_BOOT_REQUEST, NULL);
            }
        }
        break;
    // A frame type that the parent's side takes and ignores: the switch's table runs to it
    case 11:
        break;
    }
}

// Reads what every child sent in the last round
static void MBi_CommParentRecvData(const WMMpRecvHeader *mp) {
    u16 i;
    u16 aid;

    for (i = 0; i < 16; i++) {
        if (pPwork->fileinfo[i].valid) {
            pPwork->fileinfo[i].requestedBlock = 0;
        }
    }
    any_recv_bitmap = 0;
    for (aid = 1; aid <= 15; aid++) {
        const WMMpRecvData *recv = WM_ReadMPData(mp, aid);

        if (recv != NULL && recv->length != 0xffff && recv->length != 0) {
            MBi_CommParentRecvDataPerChild(recv, aid);
        }
    }
}

// Sends a frame with nothing but its type
static int MBi_CommParentSendMsg(u8 type, u16 aidBitmap) {
    MBCommParentBlockHeader frame;

    frame.type = type;
    MBi_MakeParentSendBuffer(&frame, mbc->sendbuf);
    return MBi_BlockHeaderEnd(6, aidBitmap, mbc->sendbuf);
}

// Sends the description of one file to the children that were accepted and want it, the files in turn
static int MBi_CommParentSendDLFileInfo(void) {
    u8 count[16];
    MBCommParentBlockHeader frame;
    s8 file = -1;
    u16 aidBitmap = 0;
    u8 *out;
    u16 aid;
    u8 i;
    MB_CommPWork *work;

    MI_CpuFill8(count, 0, sizeof(count));
    work = pPwork;
    for (aid = 1; aid <= 15; aid++) {
        if (work->p_comm_state[aid - 1] == MB_COMM_PSTATE_REQ_ACCEPTED) {
            count[work->fileid_of_child[aid - 1]]++;
        }
    }
    {
        u8 next = sLastFile;

        for (i = 0; i < 16; i++) {
            next = (next + 1) % 16;
            if (work->fileinfo[next].valid && count[next] != 0) {
                file = next;
                break;
            }
        }
    }
    if (file == -1) {
        return MB_SENDFUNC_STATE_ERR;
    }
    sLastFile = file;
    for (aid = 1; aid <= 15; aid++) {
        if (work->p_comm_state[aid - 1] == MB_COMM_PSTATE_REQ_ACCEPTED && file == work->fileid_of_child[aid - 1]) {
            aidBitmap |= 1 << aid;
        }
    }
    frame.type = MB_COMM_TYPE_PARENT_DL_FILEINFO;
    frame.fid = file;
    out = MBi_MakeParentSendBuffer(&frame, work->common.sendbuf);
    if (out != NULL) {
        MI_CpuCopy8(&pPwork->fileinfo[file], out, 0xe4);
    }
    return MBi_BlockHeaderEnd(0xea, aidBitmap, mbc->sendbuf);
}

// Reads the part of the program that a child will want soon into the cache
static void MBi_ReloadCache(MBiTaskInfo *task) {
    MBiCacheList *cache = task->arg1;
    MBiCacheInfo *entry = task->arg0;
    u32 size = entry->len;
    FSFile file;
    FSArchive *archive;

    archive = FS_FindArchive(cache->arc_name, cache->arc_name_len);
    if (archive == NULL) {
        archive = cache->arc_pointer;
    }
    if (OS_IsRunOnTwl()) {
        if (archive == FS_FindArchive("rom", 3)) {
            const MBTwlRomHeader *header = CARD_GetOwnRomHeaderTWL();
            u32 firstEnd = header->ntrRegionOffset + header->ntrRegionSize;
            u32 secondEnd = header->twlRegionOffset + header->twlRegionSize;
            u32 offset = entry->src;

            if (offset < firstEnd && offset + size > firstEnd) {
                size = firstEnd - offset;
            } else if (offset < secondEnd && offset + size > secondEnd) {
                size = secondEnd - offset;
            }
        }
    }
    FS_InitFile(&file);
    if (FS_OpenFileDirect(&file, archive, entry->src, entry->src + size, -1)) {
        if (size == FS_ReadFile(&file, entry->ptr, size)) {
            entry->state = MB_CACHE_STATE_READY;
        }
        FS_CloseFile(&file);
    }
    if (entry->state != MB_CACHE_STATE_READY) {
        entry->src = 0;
        entry->state = MB_CACHE_STATE_READY;
    }
}

// Sends the next block of a file to the children that are downloading it
static int MBi_CommParentSendBlock(void) {
    MB_BlockInfo source;
    MBiTaskInfo *task;
    u32 offsetInSegment;
    MBCommParentBlockHeader frame;
    u32 tableOffset;
    MBiCacheList *cache;
    u8 *out;
    u8 i;

    if (pPwork->file_num == 0) {
        return MB_SENDFUNC_STATE_ERR;
    }
    for (i = 0; i < 16; i++) {
        pPwork->cur_fileid = (pPwork->cur_fileid + 1) % 16;
        if (pPwork->fileinfo[pPwork->cur_fileid].valid && pPwork->fileinfo[pPwork->cur_fileid].downloadMask != 0) {
            break;
        }
    }
    if (i == 16) {
        return MB_SENDFUNC_STATE_ERR;
    }
    MBi_calc_sendblock(pPwork->cur_fileid);
    if (!MBi_get_blockinfo(&source, &pPwork->fileinfo[pPwork->cur_fileid].layout,
                           pPwork->fileinfo[pPwork->cur_fileid].sendCursor, &pPwork->fileinfo[pPwork->cur_fileid])) {
        return MB_SENDFUNC_STATE_ERR;
    }
    frame.type = MB_COMM_TYPE_PARENT_DATA;
    frame.fid = pPwork->cur_fileid;
    frame.seqno = pPwork->fileinfo[pPwork->cur_fileid].sendCursor;
    out = MBi_MakeParentSendBuffer(&frame, mbc->sendbuf);
    offsetInSegment = source.offset - pPwork->fileinfo[pPwork->cur_fileid].layout.seg_src_offset[source.segment_no];
    tableOffset = pPwork->fileinfo[pPwork->cur_fileid].segmentRomOffsets[source.segment_no];
    cache = pPwork->fileinfo[pPwork->cur_fileid].cache;
    if (!MBi_ReadFromCache(cache, offsetInSegment + tableOffset, out, source.size)) {
        task = &pPwork->cur_task;
        if (!MBi_IsTaskBusy(task)) {
            if (cache->lifetime != 0) {
                cache->lifetime--;
            } else {
                MBiCacheInfo *entries = cache->list;
                MBiCacheInfo *oldest = NULL;
                int n;

                for (n = 0; n < MB_CACHE_INFO_MAX; n++) {
                    MBiCacheInfo *candidate = &entries[n];

                    if (candidate->state == MB_CACHE_STATE_READY && (oldest == NULL || oldest->src > candidate->src)) {
                        oldest = candidate;
                    }
                }
                if (oldest == NULL) {
                    OS_Terminate();
                }
                cache->lifetime = 2;
                oldest->state = MB_CACHE_STATE_BUSY;
                oldest->src = (offsetInSegment + tableOffset) & ~0x1f;
                task->arg0 = oldest;
                task->arg1 = cache;
                MBi_SetTask(task, MBi_ReloadCache, NULL, 4);
            }
        }
        return MB_SENDFUNC_STATE_ERR;
    }
    return MBi_BlockHeaderEnd(source.size + 6, pPwork->fileinfo[pPwork->cur_fileid].downloadMask, mbc->sendbuf);
}

// Sends what the children's states ask for: the frames that tell them how it went, else a block of a file
static void MBi_CommParentSendData(void) {
    u16 want[5];
    u16 aid;
    int result;

    MIi_CpuClear16(0, want, sizeof(want));
    for (aid = 1; aid <= 15; aid++) {
        switch (pPwork->p_comm_state[aid - 1]) {
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
        result = MBi_CommParentSendMsg(MB_COMM_TYPE_PARENT_BOOTREQ, want[3]);
    } else if (want[0] != 0) {
        result = MBi_CommParentSendMsg(MB_COMM_TYPE_PARENT_SENDSTART, want[0]);
    } else if (want[4] != 0) {
        result = MBi_CommParentSendMsg(MB_COMM_TYPE_PARENT_MEMBER_FULL, want[4]);
    } else if (want[2] != 0) {
        result = MBi_CommParentSendMsg(MB_COMM_TYPE_PARENT_KICKREQ, want[2]);
    } else if (want[1] != 0) {
        result = MBi_CommParentSendDLFileInfo();
    } else {
        result = MBi_CommParentSendBlock();
    }
    if (result == MB_SENDFUNC_STATE_ERR) {
        MBi_CommParentSendMsg(MB_COMM_TYPE_DUMMY, 0xffff);
    }
}

// Moves the block that a file sends next on, or back to the one that a child asked for
static void MBi_calc_sendblock(u32 file) {
    if ((any_recv_bitmap & (1 << file)) == 0) {
        return;
    }
    if (pPwork->fileinfo[file].valid == 0 || pPwork->fileinfo[file].downloadMask == 0) {
        return;
    }
    if (pPwork->fileinfo[file].requestedBlock <= pPwork->fileinfo[file].sendCursor &&
        pPwork->fileinfo[file].sendCursor <= pPwork->fileinfo[file].requestedBlock + 2) {
        pPwork->fileinfo[file].sendCursor++;
    } else {
        pPwork->fileinfo[file].sendCursor = pPwork->fileinfo[file].requestedBlock;
    }
}

static u16 MBi_calc_nextsendblock(u32 a, u32 b) {
    if (b <= a) {
        b = a;
    }
    return b;
}

static BOOL IsChildAidValid(u16 aid) {
    if (aid >= 1 && aid <= 15) {
        return TRUE;
    }
    return FALSE;
}

// Tells the callback about an error
static void MBi_CommCallParentError(u16 aid, u16 errcode) {
    MBErrorStatus status;

    status.errcode = errcode;
    MBi_CommChangeParentStateCallbackOnly(aid, MB_COMM_PSTATE_ERROR, &status);
}
