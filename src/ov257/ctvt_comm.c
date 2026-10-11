#include "app/comm_tvt/ctvt_comm.h"
#include "types.h"
#include "app/comm_tvt.h"
#include "app/comm_tvt/comm_tvt_sys.h"
#include "app/comm_tvt/ctvt_call.h"
#include "app/comm_tvt/ctvt_camera.h"
#include "app/comm_tvt/ctvt_game.h"
#include "app/comm_tvt/ctvt_mic.h"
#include "app/comm_tvt/ctvt_talk.h"
#include "app/comm_tvt/draw_system.h"
#include "app/comm_tvt/ima_adpcm.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_lower_data.h"
#include "gfl/std.h"
#include "gfl/wih.h"
#include "save/player_info.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "twl/ssp.h"

// The Xtransceiver's communication over the wireless connection: the connection itself, the members of the call,
// their pictures, sent as JPEG, the voice of whoever talks, the strokes of the drawing mode and the minigames' data

#define CTVT_COMM_MEMBERS 4
#define CTVT_COMM_PICTURE_BUFFERS 3
#define CTVT_COMM_PICTURE_SIZE 0xc000
#define CTVT_COMM_VOICE_BUFFER_SIZE 0xc800
#define CTVT_COMM_VOICE_CHUNK_SIZE 0x800
#define CTVT_COMM_DRAW_QUEUE_SIZE 64
// The most strokes in one command
#define CTVT_COMM_DRAW_SEND_MAX 8
// How long an invitation stands, in frames
#define CTVT_COMM_INVITE_FRAMES 1200
#define CTVT_COMM_JPEG_QUALITY 70

// The game's commands, from CTVT_COMM_CMD_BASE
#define CTVT_COMM_CMD_BASE 0x2000
enum {
    CTVT_COMM_CMD_PACKET = CTVT_COMM_CMD_BASE,
    CTVT_COMM_CMD_VOICE,
    CTVT_COMM_CMD_DRAW,
    CTVT_COMM_CMD_INFO,
    CTVT_COMM_CMD_GAME_COMMAND,
    CTVT_COMM_CMD_GAME_PACKET,
    CTVT_COMM_CMD_GAME_DATA,
    CTVT_COMM_CMD_END,
};

#define CTVT_COMM_SEND_ALL 0xff
#define CTVT_COMM_NONE 0xff

// The steps of the connection
enum {
    CTVT_NET_IDLE,
    CTVT_NET_INIT,
    CTVT_NET_WAIT_INIT,
    CTVT_NET_WAIT_CONNECT,
    CTVT_NET_WAIT_NEGOTIATION,
    CTVT_NET_ADD_COMMANDS,
    CTVT_NET_WAIT_SYNC,
    CTVT_NET_CONNECTED,
    CTVT_NET_EXIT,
    CTVT_NET_SEND_END_SYNC,
    CTVT_NET_WAIT_END_SYNC,
    CTVT_NET_WAIT_EXIT,
    CTVT_NET_DONE,
};

// The steps of receiving a member's picture
enum {
    CTVT_PICTURE_NONE,
    CTVT_PICTURE_SEND_READY,
    CTVT_PICTURE_WAIT_START,
    CTVT_PICTURE_WAIT_END,
    CTVT_PICTURE_DONE,
};

typedef struct {
    BOOL active;
    BOOL infoReceived;
    BOOL infoApplied;
    BOOL isSelf;
    BOOL wantsToTalk;
    u8 pictureBuffer;
    int pictureState;
    CtvtCommMemberInfo info;
} CtvtCommMember;

typedef struct {
    u32 value;
    u8 type;
} CtvtCommPacket;

// What a scan finds: the rest is not used
typedef struct {
    u8 unk0[10];
    u8 mac[6];
} CtvtCommScanInfo;

struct CtvtComm {
    u32 unk0;
    u8 memberCount;
    int netState;
    CtvtCommBeacon beacon;
    CtvtCommMemberInfo selfInfo;
    CtvtCommMember members[CTVT_COMM_MEMBERS];
    void *pictureBuffers[CTVT_COMM_PICTURE_BUFFERS];
    u8 pictureBufferMask;
    int connectType;
    int nextConnectType;
    u8 parentMac[6];
    // The members to send the own picture to, and those ready for it
    u8 photoTargetMask;
    u8 photoReadyMask;
    BOOL photoSending;
    void *jpegWork;
    void *photoRaw;
    void *photoJpeg;
    BOOL voiceBusy;
    BOOL voiceReady;
    u16 voiceSize;
    u32 voiceSpeed;
    CtvtVoicePacket *voicePacket;
    s16 *voiceBuffer;
    u8 drawWriteIndex;
    u8 drawReadIndex;
    DrawCommand drawQueue[CTVT_COMM_DRAW_QUEUE_SIZE];
    u8 talker;
    u16 gameCommand;
    CtvtGamePacket gamePacket;
    CtvtGameData gameData;
    BOOL sendGameCommand;
    BOOL sendGamePacket;
    BOOL sendGameData;
    u8 unk3dc[4];
    BOOL talkRequestsChanged;
    BOOL sendInfo;
    BOOL sendZoom;
    BOOL zoom;
    BOOL unk3f0;
    BOOL sendUnk3f0;
    BOOL unk3f8;
    BOOL sendUnk3f8;
    u16 inviteTimer;
    BOOL sendTalker;
    u8 nextTalker;
    CommTvtWork *sys;
};

static void CtvtComm_InitNet(CommTvtWork *sys, CtvtComm *comm);
static void *CtvtComm_GetBeaconData(void *work);
static int CtvtComm_GetBeaconSize(void *work);
static BOOL CtvtComm_CheckBeacon(u32 gameId, u32 value);
static BOOL CtvtComm_FilterInvited(const void *data, void *work);
static BOOL CtvtComm_FilterCall(const void *data, void *work);
static BOOL CtvtComm_FilterNone(const void *data, void *work);
static void CtvtComm_UpdateTalk(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_SetScanTime(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_UpdateMembers(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_ClearMember(CommTvtWork *sys, CtvtComm *comm, CtvtCommMember *member);
static void CtvtComm_UpdateMember(CommTvtWork *sys, CtvtComm *comm, CtvtCommMember *member, u8 netId);
static BOOL CtvtComm_IsMemberTalking(CommTvtWork *sys, CtvtComm *comm, CtvtCommMember *member, u8 netId);
static void CtvtComm_RecvPacket(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *CtvtComm_GetVoiceBuffer(int netId, void *work, int size);
static void CtvtComm_RecvVoice(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CtvtComm_SendDraw(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_RecvDraw(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CtvtComm_SendInfo(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_RecvInfo(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *CtvtComm_GetInfoBuffer(int netId, void *work, int size);
static void CtvtComm_UpdateGame(CommTvtWork *sys, CtvtComm *comm);
static BOOL CtvtComm_SendGameCommand(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_RecvGameCommand(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CtvtComm_SendGamePacket(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_RecvGamePacket(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CtvtComm_SendGameData(CommTvtWork *sys, CtvtComm *comm);
static void CtvtComm_RecvGameData(int netId, int size, const void *data, void *work, NetHandle *handle);

static const NetCommand sCtvtCommCommands[] = {
    { CtvtComm_RecvPacket, NULL },      { CtvtComm_RecvVoice, CtvtComm_GetVoiceBuffer },
    { CtvtComm_RecvDraw, NULL },        { CtvtComm_RecvInfo, CtvtComm_GetInfoBuffer },
    { CtvtComm_RecvGameCommand, NULL }, { CtvtComm_RecvGamePacket, NULL },
    { CtvtComm_RecvGameData, NULL },
};

static const GFLNetInitData sCtvtCommNetInitData = {
    sCtvtCommCommands,
    NELEMS(sCtvtCommCommands),
    NULL,
    NULL,
    NULL,
    NULL,
    CtvtComm_GetBeaconData,
    CtvtComm_GetBeaconSize,
    CtvtComm_CheckBeacon,
    NULL,
    NULL,
    NULL,
    NULL,
    { 0 },
    NULL,
    NULL,
    0,
    { 1, 0, 0, 0, 0x80, 0x13, 0, 0 },
    HEAPID_USER,
    0xd,
    0xf,
    0xd,
    0xf0,
    0,
    CTVT_COMM_MEMBERS,
    0x6e,
    0x10,
    1,
    0,
    0,
    1,
    CTVT_COMM_CMD_BASE >> 8,
    { 0x2c, 1, 0, 0 },
    0,
    0,
};

CtvtComm *CtvtComm_Create(CommTvtWork *sys, HeapID heapId) {
    u8 i, j;
    CtvtComm *comm = GFL_HeapAllocate(heapId, sizeof(CtvtComm), TRUE, "ctvt_comm.c", 307);

    comm->sys = sys;
    comm->netState = CTVT_NET_IDLE;
    comm->memberCount = 1;
    comm->photoReadyMask = 0;
    comm->connectType = 0;
    comm->nextConnectType = 0;
    comm->inviteTimer = 0;
    for (i = 0; i < CTVT_COMM_MEMBERS; i++) {
        CtvtComm_ClearMember(sys, comm, &comm->members[i]);
    }
    for (i = 0; i < CTVT_COMM_PICTURE_BUFFERS; i++) {
        if (!func_ov257_021aab3c(sys) || i == 0) {
            comm->pictureBuffers[i] = allocConfigDSSoftwareFeature(heapId, CTVT_COMM_PICTURE_SIZE, "ctvt_comm.c", 326);
        }
    }
    comm->jpegWork = allocConfigDSSoftwareFeature(
        heapId, SSP_GetJpegEncoderBufferSize(128, 192, SSP_JPEG_OUTPUT_YUV422, SSP_JPEG_RGB555), "ctvt_comm.c", 331);
    comm->photoRaw = allocConfigDSSoftwareFeature(heapId, CTVT_COMM_PICTURE_SIZE, "ctvt_comm.c", 333);
    comm->photoJpeg = allocConfigDSSoftwareFeature(heapId, CTVT_COMM_PICTURE_SIZE, "ctvt_comm.c", 334);
    comm->pictureBufferMask = 0;
    comm->photoTargetMask = 0;
    comm->photoSending = FALSE;
    comm->voiceBusy = FALSE;
    comm->voiceReady = FALSE;
    comm->voicePacket = GFL_HeapAllocate(heapId, sizeof(CtvtVoicePacket), TRUE, "ctvt_comm.c", 341);
    comm->voiceBuffer = GFL_HeapAllocate(heapId, CTVT_COMM_VOICE_BUFFER_SIZE, TRUE, "ctvt_comm.c", 342);
    comm->drawWriteIndex = 0;
    comm->drawReadIndex = 0;
    comm->talker = CTVT_COMM_NONE;
    comm->talkRequestsChanged = FALSE;
    comm->sendInfo = FALSE;
    comm->sendZoom = FALSE;
    comm->zoom = FALSE;
    comm->sendTalker = FALSE;
    comm->unk3f0 = TRUE;
    comm->sendUnk3f0 = FALSE;
    comm->unk3f8 = TRUE;
    comm->sendUnk3f8 = FALSE;
    comm->nextTalker = CTVT_COMM_NONE;
    comm->sendGameCommand = FALSE;
    comm->sendGamePacket = FALSE;
    comm->sendGameData = FALSE;

    func_02008b34(GetGameDataPlayerInfo(CommTvt_GetParam(sys)->gameData), &comm->beacon.player);
    comm->beacon.memberCount = 1;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 6; j++) {
            comm->beacon.inviteMacs[i][j] = 0xff;
        }
    }
    comm->beacon.cameraEnabled = CommTvt_IsCameraEnabled();
    comm->beacon.inviteOnly = FALSE;
    func_02008b34(GetGameDataPlayerInfo(CommTvt_GetParam(sys)->gameData), (PlayerInfo *)comm->selfInfo.playerInfo);
    comm->selfInfo.cameraEnabled = CommTvt_IsCameraEnabled();
    comm->selfInfo.canExchangePhotos = canPlayerExchangePhotos();
    comm->unk0 = 1;
    func_02043868(heapId, FALSE);
    return comm;
}

void CtvtComm_Delete(CommTvtWork *sys, CtvtComm *comm) {
    u8 i;

    GFL_HeapFree(comm->voiceBuffer);
    GFL_HeapFree(comm->voicePacket);
    func_02042ed0(comm->photoRaw);
    func_02042ed0(comm->photoJpeg);
    func_02042ed0(comm->jpegWork);
    for (i = 0; i < CTVT_COMM_PICTURE_BUFFERS; i++) {
        if (!func_ov257_021aab3c(sys) || i == 0) {
            func_02042ed0(comm->pictureBuffers[i]);
        }
    }
    GFL_HeapFree(comm);
}

void CtvtComm_Update(CommTvtWork *sys, CtvtComm *comm) {
    u8 i, j;

    CommTvt_GetHeapId(sys);
    switch (comm->netState) {
    case CTVT_NET_IDLE:
        if (comm->nextConnectType != 0) {
            if (comm->nextConnectType == CTVT_CONNECT_EXISTING) {
                func_02042d8c(FALSE);
                comm->netState = CTVT_NET_ADD_COMMANDS;
                comm->connectType = comm->nextConnectType;
            } else {
                comm->netState = CTVT_NET_INIT;
            }
        }
        break;
    case CTVT_NET_INIT:
        CtvtComm_InitNet(sys, comm);
        comm->netState = CTVT_NET_WAIT_INIT;
        break;
    case CTVT_NET_WAIT_INIT:
        if (func_02042788() == TRUE) {
            switch (comm->nextConnectType) {
            case CTVT_CONNECT_PARENT:
                func_02042968();
                comm->netState = CTVT_NET_CONNECTED;
                break;
            case CTVT_CONNECT_SCAN:
                func_02042970();
                WH_SetScanFilter(CtvtComm_FilterInvited);
                comm->netState = CTVT_NET_WAIT_CONNECT;
                break;
            case CTVT_CONNECT_MAC:
                if (comm->connectType == CTVT_CONNECT_PARENT) {
                    CtvtComm_ClearMember(sys, comm, &comm->members[0]);
                }
                func_02042950(comm->parentMac);
                comm->netState = CTVT_NET_WAIT_CONNECT;
                break;
            }
            comm->connectType = comm->nextConnectType;
        }
        break;
    case CTVT_NET_WAIT_CONNECT:
        if (func_02040504() == TRUE) {
            comm->netState = CTVT_NET_WAIT_NEGOTIATION;
        }
        if (comm->connectType != comm->nextConnectType) {
            comm->netState = CTVT_NET_EXIT;
        }
        break;
    case CTVT_NET_WAIT_NEGOTIATION:
        if (func_0204044c(func_02040440()) == TRUE) {
            comm->netState = CTVT_NET_CONNECTED;
        }
        if (comm->connectType != comm->nextConnectType) {
            comm->netState = CTVT_NET_EXIT;
        }
        break;
    case CTVT_NET_ADD_COMMANDS: {
        NetHandle *handle = func_02040440();

        func_02040c20(CTVT_COMM_CMD_BASE, sCtvtCommCommands, NELEMS(sCtvtCommCommands), comm);
        func_02040624(handle, 8, 32);
        comm->netState = CTVT_NET_WAIT_SYNC;
        break;
    }
    case CTVT_NET_WAIT_SYNC:
        if (func_02040664(func_02040440(), 8, 32) == TRUE) {
            comm->netState = CTVT_NET_CONNECTED;
        }
        break;
    case CTVT_NET_CONNECTED:
        if (CommTvt_GetMode(sys) == COMM_TVT_MODE_GAME) {
            CtvtComm_UpdateGame(sys, comm);
        } else {
            CtvtComm_UpdateTalk(sys, comm);
        }
        if (comm->connectType != comm->nextConnectType) {
            comm->netState = CTVT_NET_EXIT;
        }
        if (comm->connectType == CTVT_CONNECT_PARENT) {
            CtvtComm_SetScanTime(sys, comm);
        }
        break;
    case CTVT_NET_EXIT:
        func_02042860(NULL);
        comm->netState = CTVT_NET_WAIT_EXIT;
        break;
    case CTVT_NET_WAIT_EXIT:
        if (func_020427a4() == TRUE) {
            if (comm->nextConnectType == CTVT_CONNECT_DISCONNECT) {
                comm->netState = CTVT_NET_DONE;
            } else {
                comm->netState = CTVT_NET_INIT;
            }
        }
        break;
    case CTVT_NET_SEND_END_SYNC:
        func_02040624(func_02040440(), 9, 32);
        comm->netState = CTVT_NET_WAIT_END_SYNC;
        break;
    case CTVT_NET_WAIT_END_SYNC:
        if (func_02040664(func_02040440(), 9, 32) == TRUE) {
            func_02040c64(CTVT_COMM_CMD_BASE);
            comm->netState = CTVT_NET_DONE;
        }
        if (GFL_NetErrCheck()) {
            func_02040c64(CTVT_COMM_CMD_BASE);
            comm->netState = CTVT_NET_DONE;
        }
        break;
    }

    if (comm->inviteTimer != 0) {
        comm->inviteTimer--;
        if (comm->inviteTimer == 0) {
            for (i = 0; i < 3; i++) {
                for (j = 0; j < 6; j++) {
                    comm->beacon.inviteMacs[i][j] = 0xff;
                }
            }
            comm->beacon.inviteOnly = FALSE;
        }
    }
}

static void CtvtComm_InitNet(CommTvtWork *sys, CtvtComm *comm) {
    GFLNetInitData init = sCtvtCommNetInitData;

    CommTvt_GetHeapId(sys);
    func_020425ec(&init, NULL, comm);
    func_02042e9c(FALSE);
}

void CtvtComm_Disconnect(CommTvtWork *sys, CtvtComm *comm) {
    if (comm->memberCount <= 1) {
        if (comm->connectType == CTVT_CONNECT_EXISTING) {
            comm->netState = CTVT_NET_DONE;
            func_02040c64(CTVT_COMM_CMD_BASE);
        } else {
            func_02042860(NULL);
            comm->netState = CTVT_NET_WAIT_EXIT;
        }
    } else if (comm->connectType == CTVT_CONNECT_EXISTING) {
        comm->netState = CTVT_NET_SEND_END_SYNC;
    } else {
        comm->netState = CTVT_NET_EXIT;
    }
    comm->nextConnectType = CTVT_CONNECT_DISCONNECT;
}

BOOL CtvtComm_IsDone(CommTvtWork *sys, CtvtComm *comm) {
    if (comm->netState == CTVT_NET_DONE) {
        return TRUE;
    }
    return FALSE;
}

void CtvtComm_SetNextConnectType(CommTvtWork *sys, CtvtComm *comm, int type) {
    comm->nextConnectType = type;
}

int CtvtComm_GetConnectType(CommTvtWork *sys, CtvtComm *comm) {
    return comm->connectType;
}

void CtvtComm_SetParentMac(CommTvtWork *sys, CtvtComm *comm, const u8 *mac) {
    u8 i;

    for (i = 0; i < 6; i++) {
        comm->parentMac[i] = mac[i];
    }
}

BOOL CtvtComm_IsConnected(CommTvtWork *sys, CtvtComm *comm) {
    if (comm->netState == CTVT_NET_CONNECTED) {
        return TRUE;
    }
    return FALSE;
}

static void *CtvtComm_GetBeaconData(void *work) {
    CtvtComm *comm = work;

    return &comm->beacon;
}

static int CtvtComm_GetBeaconSize(void *work) {
    return sizeof(CtvtCommBeacon);
}

static BOOL CtvtComm_CheckBeacon(u32 gameId, u32 value) {
    if (value == 0x20 || value == 3) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CtvtComm_FilterInvited(const void *data, void *work) {
    const CtvtCommScanInfo *info = data;
    CtvtComm *comm = work;

    if (comm->beacon.inviteOnly == FALSE) {
        return TRUE;
    }
    return CtvtComm_IsInvited(&comm->beacon, info->mac);
}

static BOOL CtvtComm_FilterCall(const void *data, void *work) {
    const CtvtCommScanInfo *info = data;
    CtvtComm *comm = work;
    BOOL invited;

    if (CtvtCall_IsBlackWhite(comm->sys, CommTvt_GetCall(comm->sys), info->mac)) {
        return FALSE;
    }
    invited = CtvtComm_IsInvited(&comm->beacon, info->mac);
    if (invited == TRUE) {
        CtvtTalk_AddNewMember(CommTvt_GetTalk(comm->sys));
    }
    return invited;
}

static BOOL CtvtComm_FilterNone(const void *data, void *work) {
    return FALSE;
}

static void CtvtComm_UpdateTalk(CommTvtWork *sys, CtvtComm *comm) {
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 i;
    CtvtTalk *talk;
    BOOL alone;
    BOOL wasNotAlone;

    if (comm->connectType == CTVT_CONNECT_PARENT) {
        if (comm->members[0].active == FALSE) {
            CtvtCamera *camera = CommTvt_GetCamera(sys);

            comm->members[0].active = TRUE;
            comm->members[0].infoReceived = TRUE;
            comm->members[0].infoApplied = TRUE;
            comm->members[0].isSelf = TRUE;
            comm->members[0].wantsToTalk = FALSE;
            comm->members[0].pictureBuffer = CTVT_COMM_NONE;
            comm->members[0].pictureState = CTVT_PICTURE_NONE;
            comm->members[0].info = comm->selfInfo;
            comm->beacon.memberCount = 1;
            comm->memberCount = 1;
            CtvtCamera_Redraw(sys, camera, FALSE, FALSE);
        }
    } else {
        u8 count = func_02042a78();

        if (comm->memberCount != count) {
            CtvtComm_UpdateMembers(sys, comm);
            func_ov257_021aad08(sys);
            if (comm->memberCount < count && selfNetId == 0) {
                comm->sendZoom = TRUE;
                comm->zoom = CommTvt_IsZoomed(sys);
            }
            comm->sendInfo = TRUE;
            comm->memberCount = count;
            comm->beacon.memberCount = count;
        }
    }

    for (i = 0; i < CTVT_COMM_MEMBERS; i++) {
        CtvtComm_UpdateMember(sys, comm, &comm->members[i], i);
    }

    talk = CommTvt_GetTalk(sys);
    alone = TRUE;
    wasNotAlone = CtvtTalk_IsNotAlone(talk);
    for (i = 0; i < CTVT_COMM_MEMBERS; i++) {
        if (CtvtComm_IsMemberTalking(sys, comm, &comm->members[i], i)) {
            alone = FALSE;
        }
    }
    if (alone == FALSE) {
        CtvtTalk_SetNotAlone(talk, TRUE);
    } else {
        CtvtTalk_SetNotAlone(talk, FALSE);
    }
    // alone is the opposite of notAlone, so equal values mean the state changed
    if (alone == wasNotAlone && selfNetId == 0) {
        CtvtTalk_SetNotAloneChanged(CommTvt_GetTalk(sys), TRUE);
    }

    if (comm->memberCount > 1) {
        if (comm->photoSending == TRUE) {
            if (func_02043b10() == TRUE) {
                comm->photoSending = FALSE;
            }
        } else if (comm->photoTargetMask == (comm->photoTargetMask & comm->photoReadyMask) &&
                   comm->photoTargetMask != 0 && func_ov257_021aab5c(sys) == FALSE) {
            func_ov257_021aab60(sys, TRUE);
            if (func_ov257_021aab10(sys) == FALSE) {
                CtvtCamera *camera = CommTvt_GetCamera(sys);
                void *picture = CtvtCamera_GetOwnPicture(sys, camera);
                u32 size = CtvtCamera_GetPictureSize(sys, camera);
                u32 jpegSize;
                u16 width;

                sys_memcpy32(picture, comm->photoRaw, size);
                width = CtvtCamera_GetWidth(sys, camera);
                jpegSize = SSP_StartJpegEncoder(comm->photoRaw, comm->photoJpeg, size, comm->jpegWork, width,
                                                CtvtCamera_GetHeight(sys, camera), CTVT_COMM_JPEG_QUALITY,
                                                SSP_JPEG_OUTPUT_YUV422, SSP_JPEG_RGB555);
                if (jpegSize != 0) {
                    func_0204393c(comm->photoJpeg, jpegSize, comm->photoTargetMask, FALSE);
                    comm->photoReadyMask = 0;
                    comm->photoSending = TRUE;
                }
            }
            func_ov257_021aab60(sys, FALSE);
        }

        if (comm->drawWriteIndex != comm->drawReadIndex && comm->unk3f0 == TRUE) {
            CtvtComm_SendDraw(sys, comm);
        }
        if (comm->sendUnk3f0 == TRUE && CtvtComm_SendPacket(sys, comm, CTVT_PACKET_UNK4, 0) == TRUE) {
            comm->sendUnk3f0 = FALSE;
        }
        if (comm->sendUnk3f8 == TRUE && CtvtComm_SendPacket(sys, comm, CTVT_PACKET_UNK5, 0) == TRUE) {
            comm->sendUnk3f8 = FALSE;
        }
        if (comm->sendInfo == TRUE && CtvtComm_SendInfo(sys, comm) == TRUE) {
            comm->sendInfo = FALSE;
        }
    }

    if (selfNetId == 0) {
        if (comm->sendZoom == TRUE && CtvtComm_SendPacket(sys, comm, CTVT_PACKET_ZOOM, comm->zoom) == TRUE) {
            comm->sendZoom = FALSE;
        }
        if (comm->talkRequestsChanged == TRUE) {
            BOOL found = FALSE;
            u8 netId;

            for (netId = 0; netId < CTVT_COMM_MEMBERS; netId++) {
                if (comm->members[netId].wantsToTalk == TRUE) {
                    if (comm->nextTalker == CTVT_COMM_NONE) {
                        comm->nextTalker = netId;
                    }
                    found = TRUE;
                    comm->members[netId].wantsToTalk = FALSE;
                }
            }
            if (!found) {
                comm->nextTalker = CTVT_COMM_NONE;
            }
            if (comm->nextTalker != comm->talker) {
                comm->sendTalker = TRUE;
            }
            comm->talkRequestsChanged = FALSE;
        }
        if (comm->nextTalker != CTVT_COMM_NONE && comm->members[comm->nextTalker].active == FALSE) {
            comm->nextTalker = CTVT_COMM_NONE;
            comm->sendTalker = TRUE;
            comm->talkRequestsChanged = FALSE;
        }
        if (comm->sendTalker == TRUE && CtvtComm_SendPacket(sys, comm, CTVT_PACKET_TALKER, comm->nextTalker) == TRUE) {
            comm->sendTalker = FALSE;
        }
    }
}

static void CtvtComm_SetScanTime(CommTvtWork *sys, CtvtComm *comm) {
    WH_SetScanTime(5);
}

static void CtvtComm_UpdateMembers(CommTvtWork *sys, CtvtComm *comm) {
    u8 count = 0;
    BOOL changed = FALSE;
    u8 selfNetId = func_02042a6c(func_02040440());
    HeapID heapId = CommTvt_GetHeapId(sys);
    u8 netId;
    u8 i;

    for (netId = 0; netId < CTVT_COMM_MEMBERS; netId++) {
        if (func_02042a80(netId) == TRUE) {
            if (comm->members[netId].active == FALSE) {
                CtvtCamera *camera = CommTvt_GetCamera(sys);

                comm->members[netId].active = TRUE;
                if (netId == selfNetId) {
                    comm->members[netId].isSelf = TRUE;
                } else {
                    for (i = 0; i < CTVT_COMM_PICTURE_BUFFERS; i++) {
                        u32 bit = 1 << i;

                        if (!(comm->pictureBufferMask & bit)) {
                            func_020439a4(CTVT_COMM_PICTURE_SIZE, netId, heapId, comm->pictureBuffers[i]);
                            comm->pictureBufferMask |= bit;
                            comm->members[netId].pictureBuffer = i;
                            break;
                        }
                    }
                    comm->photoTargetMask |= 1 << netId;
                    comm->members[netId].pictureState = CTVT_PICTURE_SEND_READY;
                    CtvtTalk_ClearNewMembers(CommTvt_GetTalk(sys));
                }
                CtvtCamera_MarkWindow(sys, camera, netId);
                changed = TRUE;
            }
            count++;
        } else if (comm->members[netId].active == TRUE) {
            comm->members[netId].active = FALSE;
            if (comm->members[netId].isSelf == FALSE) {
                func_02043a1c(netId);
                comm->pictureBufferMask -= 1 << comm->members[netId].pictureBuffer;
            }
            if (comm->talker == netId) {
                comm->voiceBusy = FALSE;
            }
            changed = TRUE;
            comm->photoTargetMask -= changed << netId;
            CtvtComm_ClearMember(sys, comm, &comm->members[netId]);
        }
    }
    if (changed == TRUE) {
        CommTvt_SetMemberCount(sys, count);
        CommTvt_SetSelfIndex(sys, selfNetId);
    }
    CtvtCamera_Redraw(sys, CommTvt_GetCamera(sys), FALSE, TRUE);
}

BOOL CtvtComm_IsInvited(const CtvtCommBeacon *beacon, const u8 *mac) {
    u8 i, j;

    for (i = 0; i < 3; i++) {
        BOOL same = TRUE;

        for (j = 0; j < 6; j++) {
            if (beacon->inviteMacs[i][j] != mac[j]) {
                same = FALSE;
                break;
            }
        }
        if (same == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

static void CtvtComm_ClearMember(CommTvtWork *sys, CtvtComm *comm, CtvtCommMember *member) {
    member->isSelf = FALSE;
    member->active = FALSE;
    member->infoReceived = FALSE;
    member->infoApplied = FALSE;
    member->wantsToTalk = FALSE;
    member->pictureBuffer = CTVT_COMM_NONE;
    member->pictureState = CTVT_PICTURE_NONE;
    sys_memset(&member->info, 0, sizeof(CtvtCommMemberInfo));
}

static void CtvtComm_UpdateMember(CommTvtWork *sys, CtvtComm *comm, CtvtCommMember *member, u8 netId) {
    CtvtCamera *camera = CommTvt_GetCamera(sys);

    if (member->active == FALSE || member->isSelf == TRUE) {
        return;
    }
    if (member->infoApplied == FALSE && member->infoReceived == TRUE) {
        if (!func_ov257_021aab3c(sys)) {
            func_ov257_021aae54(sys, &member->info);
        }
        member->infoApplied = TRUE;
    }
    switch (member->pictureState) {
    case CTVT_PICTURE_NONE:
        break;
    case CTVT_PICTURE_SEND_READY:
        if (CtvtComm_SendPacket(sys, comm, CTVT_PACKET_READY, 0) == TRUE) {
            member->pictureState = CTVT_PICTURE_WAIT_START;
        }
        break;
    case CTVT_PICTURE_WAIT_START:
        if (func_02043b24(netId) == FALSE) {
            CtvtCamera_ClearDirty(sys, camera, netId);
            member->pictureState = CTVT_PICTURE_WAIT_END;
        }
        break;
    case CTVT_PICTURE_WAIT_END:
        if (func_02043b24(netId) == TRUE) {
            void *picture = CtvtCamera_GetPicture(sys, camera, netId);
            s16 size[2];

            size[1] = CtvtCamera_GetWidth(sys, camera);
            size[0] = CtvtCamera_GetHeight(sys, camera);
            SSP_StartJpegDecoder(func_02043a80(netId), func_02043ac8(netId), picture, &size[1], &size[0], 0);
            CtvtCamera_SetDirty(sys, camera, netId);
            member->pictureState = CTVT_PICTURE_SEND_READY;
        }
        break;
    case CTVT_PICTURE_DONE:
        break;
    }
}

// The caller passes the member's net ID too, which this does not use
static BOOL CtvtComm_IsMemberTalking(CommTvtWork *sys, CtvtComm *comm, CtvtCommMember *member, u8 netId) {
    u8 status;

    CommTvt_GetCamera(sys);
    if (member->active == FALSE) {
        return FALSE;
    }
    if (member->infoReceived == FALSE) {
        return FALSE;
    }
    if (member->isSelf == TRUE) {
        return FALSE;
    }
    status = func_02008bfc((PlayerInfo *)member->info.playerInfo);
    if (status == 0x16 || status == 0x17) {
        return FALSE;
    }
    return TRUE;
}

void CtvtComm_SendZoom(CommTvtWork *sys, CtvtComm *comm, BOOL zoomed) {
    if (CtvtComm_SendPacket(sys, comm, CTVT_PACKET_ZOOM, zoomed) == FALSE) {
        comm->sendZoom = TRUE;
        comm->zoom = zoomed;
    }
}

BOOL CtvtComm_SendPacket(CommTvtWork *sys, CtvtComm *comm, u8 type, u32 value) {
    NetHandle *handle = func_02040440();
    CtvtCommPacket packet;

    packet.type = type;
    packet.value = value;
    return func_02042c18(handle, CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_PACKET, sizeof(CtvtCommPacket), &packet, TRUE, FALSE,
                         FALSE);
}

BOOL CtvtComm_SendPacketAll(CommTvtWork *sys, CtvtComm *comm, u8 type, u32 value) {
    NetHandle *handle = func_02040440();
    CtvtCommPacket packet;

    packet.type = type;
    packet.value = value;
    return func_02042c18(handle, CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_PACKET, sizeof(CtvtCommPacket), &packet, TRUE, TRUE,
                         FALSE);
}

BOOL CtvtComm_SendPacketData(CommTvtWork *sys, CtvtComm *comm, u8 type, const void *value) {
    NetHandle *handle = func_02040440();
    CtvtCommPacket packet;

    packet.type = type;
    sys_memcpy(value, &packet.value, sizeof(packet.value));
    return func_02042c18(handle, CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_PACKET, sizeof(CtvtCommPacket), &packet, TRUE, FALSE,
                         FALSE);
}

static void CtvtComm_RecvPacket(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CtvtComm *comm = work;
    const CtvtCommPacket *packet = data;
    u8 selfNetId = func_02042a6c(func_02040440());

    switch (packet->type) {
    case CTVT_PACKET_READY:
        if (netId != selfNetId) {
            comm->photoReadyMask |= 1 << netId;
        }
        break;
    case CTVT_PACKET_TALKER: {
        u32 talker = packet->value;

        comm->talker = talker;
        if (talker != CTVT_COMM_NONE) {
            func_ov257_021aad74(comm->sys, talker);
        } else {
            CtvtMic *mic = CommTvt_GetMic(comm->sys);

            func_ov257_021aae44(comm->sys);
            CtvtMic_StopPlaying(mic);
        }
        break;
    }
    case CTVT_PACKET_DRAW_INDEX:
        DrawSystem_SetWriteIndex(CommTvt_GetDrawSystem(comm->sys), packet->value);
        func_ov257_021aab30(comm->sys, FALSE);
        break;
    case CTVT_PACKET_ZOOM: {
        CtvtCamera *camera = CommTvt_GetCamera(comm->sys);
        BOOL zoomed = packet->value;

        if (zoomed != CommTvt_IsZoomed(comm->sys)) {
            CommTvt_SetZoomed(comm->sys, zoomed);
            CtvtCamera_Redraw(comm->sys, camera, TRUE, FALSE);
        }
        break;
    }
    case CTVT_PACKET_UNK4:
        if (netId != selfNetId) {
            comm->unk3f0 = TRUE;
        }
        break;
    case CTVT_PACKET_UNK5:
        if (netId != selfNetId) {
            comm->unk3f8 = TRUE;
        }
        break;
    case CTVT_PACKET_REQUEST_TALK:
        if (selfNetId == 0) {
            comm->members[netId].wantsToTalk = TRUE;
            comm->talkRequestsChanged = TRUE;
        }
        break;
    case CTVT_PACKET_PLAY_VOICE: {
        CtvtMic *mic = CommTvt_GetMic(comm->sys);

        // The voice was decoded into voiceBuffer, but the packet buffer is what is played
        if (netId == selfNetId) {
            CtvtMic_Play(mic, comm->voicePacket, comm->voiceSize, 0, comm->voiceSpeed);
        } else {
            CtvtMic_Play(mic, comm->voicePacket, comm->voiceSize, 127, comm->voiceSpeed);
        }
        break;
    }
    case CTVT_PACKET_CANCEL_TALK:
        if (selfNetId == 0) {
            comm->members[netId].wantsToTalk = FALSE;
            comm->talkRequestsChanged = TRUE;
        }
        func_ov257_021aae44(comm->sys);
        break;
    case CTVT_PACKET_TALK_DONE:
        if (netId == comm->talker) {
            comm->talkRequestsChanged = TRUE;
        }
        func_ov257_021aae44(comm->sys);
        break;
    case CTVT_PACKET_UNK_B:
        if (selfNetId != 0) {
            CtvtTalk *talk = CommTvt_GetTalk(comm->sys);

            CtvtGame_SetType(CommTvt_GetGame(comm->sys), packet->value);
            CtvtTalk_SetGameInvited(talk, TRUE);
        }
        break;
    case CTVT_PACKET_UNK_C:
        if (selfNetId == 0) {
            CtvtTalk_SetJoined(CommTvt_GetTalk(comm->sys), netId);
        }
        break;
    case CTVT_PACKET_UNK_D:
        if (selfNetId == 0) {
            CtvtTalk_SetGameCancelRequested(CommTvt_GetTalk(comm->sys), TRUE);
        }
        break;
    case CTVT_PACKET_UNK_E:
        if (selfNetId != 0) {
            CtvtTalk_SetGameStarting(CommTvt_GetTalk(comm->sys), TRUE);
        }
        if (netId == 0) {
            sys_memcpy(packet, comm->unk3dc, sizeof(comm->unk3dc));
        }
        break;
    case CTVT_PACKET_UNK_F:
        if (selfNetId != 0) {
            CtvtTalk_SetGameCancelled(CommTvt_GetTalk(comm->sys), TRUE);
        }
        break;
    case CTVT_PACKET_UNK_10:
        if (selfNetId == 0) {
            CtvtTalk_SetReady(CommTvt_GetTalk(comm->sys), netId);
        }
        break;
    case CTVT_PACKET_UNK_11:
        if (selfNetId != 0) {
            CtvtTalk_SetGameStart(CommTvt_GetTalk(comm->sys), TRUE);
        }
        break;
    case CTVT_PACKET_UNK_A:
        func_ov257_021aab38(comm->sys, TRUE);
        break;
    }
}

BOOL CtvtComm_SendVoice(CommTvtWork *sys, CtvtComm *comm, CtvtVoicePacket *packet) {
    BOOL sent = func_02042c18(func_02040440(), CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_VOICE, packet->size + 8, packet, TRUE,
                              FALSE, TRUE);

    if (sent) {
        comm->voiceBusy = TRUE;
    }
    return sent;
}

static void *CtvtComm_GetVoiceBuffer(int netId, void *work, int size) {
    CtvtComm *comm = work;

    comm->voiceBusy = TRUE;
    sys_memset32(0, comm->voicePacket, sizeof(CtvtVoicePacket));
    return comm->voicePacket;
}

static void CtvtComm_RecvVoice(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CtvtComm *comm = work;
    CtvtMic *mic = CommTvt_GetMic(comm->sys);
    CtvtVoicePacket *packet = comm->voicePacket;
    s8 *voice = packet->data;

    func_02042a6c(func_02040440());
    if (packet->chunk == 0) {
        Adpcm_ResetDecoder();
        sys_memset32(0, comm->voiceBuffer, CTVT_COMM_VOICE_BUFFER_SIZE);
    }
    CtvtMic_Decode(mic, voice, (s16 *)((u8 *)comm->voiceBuffer + packet->chunk * CTVT_COMM_VOICE_CHUNK_SIZE),
                   packet->size);
    if (packet->isLast == TRUE) {
        // The playback speeds, 1.0 at 0x8000
        u16 speeds[] = { 0x4000, 0x5000, 0x6000, 0x7000, 0x8000, 0x9000, 0xa000, 0xb000, 0xc000 };

        comm->voiceSpeed = speeds[packet->speed];
        comm->voiceReady = TRUE;
        comm->voiceSize = packet->playSize;
    }
    comm->voiceBusy = FALSE;
}

static BOOL CtvtComm_SendDraw(CommTvtWork *sys, CtvtComm *comm) {
    NetHandle *handle = func_02040440();
    u8 count;
    BOOL sent;

    if (comm->drawReadIndex > comm->drawWriteIndex) {
        count = CTVT_COMM_DRAW_QUEUE_SIZE - comm->drawReadIndex;
    } else {
        count = comm->drawWriteIndex - comm->drawReadIndex;
    }
    if (count > CTVT_COMM_DRAW_SEND_MAX) {
        count = CTVT_COMM_DRAW_SEND_MAX;
    }
    if (count != 0) {
        sent = func_02042c18(handle, CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_DRAW, count * sizeof(DrawCommand),
                             &comm->drawQueue[comm->drawReadIndex], TRUE, TRUE, FALSE);
        if (sent) {
            comm->drawReadIndex += count;
            if (comm->drawReadIndex >= CTVT_COMM_DRAW_QUEUE_SIZE) {
                comm->drawReadIndex -= CTVT_COMM_DRAW_QUEUE_SIZE;
            }
        }
        return sent;
    }
    return TRUE;
}

static void CtvtComm_RecvDraw(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CtvtComm *comm = work;
    DrawSystem *drawSystem = CommTvt_GetDrawSystem(comm->sys);
    u8 count = size / sizeof(DrawCommand);
    u8 i;

    func_02042a6c(func_02040440());
    for (i = 0; i < count; i++) {
        DrawSystem_AddCommand(drawSystem, (const DrawCommand *)data + i);
    }
}

static BOOL CtvtComm_SendInfo(CommTvtWork *sys, CtvtComm *comm) {
    CtvtCommMemberInfo *info = CtvtComm_GetSelfInfo(sys, comm);

    return func_02042c18(func_02040440(), CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_INFO, sizeof(CtvtCommMemberInfo), info,
                         TRUE, TRUE, TRUE);
}

static void CtvtComm_RecvInfo(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CtvtComm *comm = work;

    if (comm->members[netId].infoReceived == FALSE) {
        comm->members[netId].infoReceived = TRUE;
        func_ov257_021aad08(comm->sys);
    }
}

static void *CtvtComm_GetInfoBuffer(int netId, void *work, int size) {
    CtvtComm *comm = work;

    return CtvtComm_GetMemberInfo(comm->sys, comm, netId);
}

static void CtvtComm_UpdateGame(CommTvtWork *sys, CtvtComm *comm) {
    CtvtGame *game = CommTvt_GetGame(sys);
    u8 selfNetId = func_02042a6c(func_02040440());
    BOOL packetSent = FALSE;

    u8 count = func_02042a78();

    if (comm->memberCount != count) {
        CommTvt_SetErrorShown(sys);
        return;
    }
    if (selfNetId == 0 && CtvtGame_IsPlaying(game) == TRUE) {
        comm->gamePacket.frame = CtvtGame_GetHostFrame(game);
    }
    if (comm->memberCount > 1) {
        if (comm->sendGameCommand == TRUE && CtvtComm_SendGameCommand(sys, comm) == TRUE) {
            comm->sendGameCommand = FALSE;
        }
        if (comm->sendGamePacket == TRUE && CtvtComm_SendGamePacket(sys, comm) == TRUE) {
            comm->sendGamePacket = FALSE;
            packetSent = TRUE;
        }
        if (comm->sendGameData == TRUE && CtvtComm_SendGameData(sys, comm) == TRUE) {
            comm->sendGameData = FALSE;
        }
    }
    if (selfNetId == 0 && CtvtGame_IsPlaying(game) == TRUE && packetSent == FALSE) {
        comm->gamePacket.type = 2;
        CtvtComm_SendGamePacket(sys, comm);
    }
}

void CtvtComm_QueueGameCommand(CommTvtWork *sys, CtvtComm *comm, u16 command) {
    comm->sendGameCommand = TRUE;
    comm->gameCommand = command;
}

static BOOL CtvtComm_SendGameCommand(CommTvtWork *sys, CtvtComm *comm) {
    return func_02042c18(func_02040440(), CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_GAME_COMMAND, sizeof(comm->gameCommand),
                         &comm->gameCommand, TRUE, FALSE, FALSE);
}

static void CtvtComm_RecvGameCommand(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CtvtComm *comm = work;
    const u16 *command = data;
    CtvtGame *game = CommTvt_GetGame(comm->sys);
    u8 selfNetId = func_02042a6c(func_02040440());

    switch (*command) {
    case 0:
        if (selfNetId == 0) {
            CtvtGame_SetJoined(game, netId);
        }
        break;
    case 1:
        if (selfNetId == 0) {
            CtvtGame_SetChildQuit(game, TRUE);
        }
        break;
    case 2:
        if (selfNetId != 0) {
            CtvtGame_SetAllJoined(game, TRUE);
        }
        break;
    case 3:
        if (selfNetId != 0) {
            CtvtGame_SetHostQuit(game, TRUE);
        }
        break;
    case 4:
        if (selfNetId == 0) {
            CtvtGame_SetReady(game, netId);
        }
        break;
    case 5:
        if (selfNetId != 0) {
            CtvtGame_SetReplayStarted(game, TRUE);
        }
        break;
    }
}

void CtvtComm_QueueGamePacket(CtvtComm *comm, CtvtGamePacket packet, u8 type) {
    u8 i;

    comm->sendGamePacket = TRUE;
    if (type == 7 && packet.frame == comm->gamePacket.frame) {
        packet.mask |= comm->gamePacket.mask;
    }
    if (type == 4 && packet.frame == comm->gamePacket.frame) {
        for (i = 0; i < 4; i++) {
            if (!(packet.mask & (1 << i))) {
                packet.values[i] = comm->gamePacket.values[i];
            }
        }
        packet.mask |= comm->gamePacket.mask;
    }
    comm->gamePacket = packet;
    comm->gamePacket.type = type;
}

static BOOL CtvtComm_SendGamePacket(CommTvtWork *sys, CtvtComm *comm) {
    BOOL sent = func_02042c18(func_02040440(), CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_GAME_PACKET, sizeof(CtvtGamePacket),
                              &comm->gamePacket, TRUE, TRUE, FALSE);

    if (sent) {
        sys_memset(&comm->gamePacket, 0, sizeof(CtvtGamePacket));
    }
    return sent;
}

static void CtvtComm_RecvGamePacket(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CtvtComm *comm = work;
    const CtvtGamePacket *packet = data;
    CtvtGame *game = CommTvt_GetGame(comm->sys);
    u8 selfNetId = func_02042a6c(func_02040440());

    if (CommTvt_IsErrorShown(comm->sys) == TRUE) {
        return;
    }
    if (netId == 0 && CtvtGame_IsPlaying(game) == TRUE) {
        CtvtGame_SetFrame(game, packet->frame);
    }
    switch (packet->type) {
    case 0:
        CtvtGame_SpawnTarget(game, packet->unk7);
        break;
    case 1:
        if (netId == 0) {
            CtvtGame_SetSeed(game, packet->value);
        }
        break;
    case 3:
        if (selfNetId == 0 && CtvtGame_IsPlaying(game) == TRUE) {
            CtvtGame_CheckTouch(comm, game, packet, netId);
        }
        break;
    case 4:
        CtvtGame_ApplyHits(game, packet, packet->mask);
        break;
    case 5:
        if (selfNetId == 0) {
            CtvtGame_CountPump(comm, game, FALSE, netId);
        }
        break;
    case 6:
        if (selfNetId == 0) {
            CtvtGame_CountPump(comm, game, TRUE, netId);
        }
        break;
    case 7:
        if (netId == 0 && CtvtGame_IsPlaying(game) == TRUE) {
            CtvtGame_PumpBalloons(game, packet->mask);
        }
        break;
    }
}

void CtvtComm_QueueGameData(CtvtComm *comm, CtvtGameData data) {
    comm->sendGameData = TRUE;
    comm->gameData = data;
}

void CtvtComm_ScanInvited(CommTvtWork *sys, CtvtComm *comm) {
    WH_SetScanFilter(CtvtComm_FilterCall);
}

void CtvtComm_ScanAll(CommTvtWork *sys, CtvtComm *comm) {
    WH_SetScanFilter(CtvtComm_FilterInvited);
}

void CtvtComm_ScanNone(CommTvtWork *sys, CtvtComm *comm) {
    WH_SetScanFilter(CtvtComm_FilterNone);
}

void CtvtComm_ResetSession(CommTvtWork *sys, CtvtComm *comm) {
    u8 i;

    for (i = 0; i < CTVT_COMM_MEMBERS; i++) {
        CtvtComm_ClearMember(sys, comm, &comm->members[i]);
    }
    comm->memberCount = 1;
    comm->pictureBufferMask = 0;
    comm->photoSending = FALSE;
    comm->voiceBusy = FALSE;
    comm->voiceReady = FALSE;
    comm->drawWriteIndex = 0;
    comm->drawReadIndex = 0;
    comm->talkRequestsChanged = FALSE;
    comm->sendInfo = FALSE;
    comm->sendZoom = FALSE;
    comm->zoom = FALSE;
    comm->sendTalker = FALSE;
    comm->unk3f0 = TRUE;
    comm->sendUnk3f0 = FALSE;
    comm->unk3f8 = TRUE;
    comm->sendUnk3f8 = FALSE;
    comm->nextTalker = CTVT_COMM_NONE;
    WH_SetScanFilter(CtvtComm_FilterInvited);
    CtvtComm_UpdateMembers(sys, comm);
}

static BOOL CtvtComm_SendGameData(CommTvtWork *sys, CtvtComm *comm) {
    return func_02042c18(func_02040440(), CTVT_COMM_SEND_ALL, CTVT_COMM_CMD_GAME_DATA, sizeof(CtvtGameData),
                         &comm->gameData, TRUE, TRUE, FALSE);
}

static void CtvtComm_RecvGameData(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CtvtComm *comm = work;
    CtvtGame *game = CommTvt_GetGame(comm->sys);

    func_02042a6c(func_02040440());
    if (netId == 0) {
        CtvtGame_SetScores(game, data);
    }
}

void CtvtComm_StartSync(CommTvtWork *sys, CtvtComm *comm, int no) {
    func_02040624(func_02040440(), no, 32);
}

BOOL CtvtComm_IsSynced(CommTvtWork *sys, CtvtComm *comm, int no) {
    return func_02040664(func_02040440(), no, 32);
}

u8 CtvtComm_GetSelfNetId(CommTvtWork *sys, CtvtComm *comm) {
    return func_02042a6c(func_02040440());
}

u8 CtvtComm_GetTalker(CommTvtWork *sys, CtvtComm *comm) {
    return comm->talker;
}

u8 CtvtComm_GetUnk3dc(CommTvtWork *sys, CtvtComm *comm, u8 index) {
    return comm->unk3dc[index];
}

BOOL CtvtComm_IsVoiceBusy(CommTvtWork *sys, CtvtComm *comm) {
    return comm->voiceBusy;
}

CtvtCommBeacon *CtvtComm_GetBeacon(CommTvtWork *sys, CtvtComm *comm) {
    return &comm->beacon;
}

CtvtVoicePacket *CtvtComm_GetVoicePacket(CommTvtWork *sys, CtvtComm *comm) {
    return comm->voicePacket;
}

BOOL CtvtComm_IsMemberInfoReceived(CommTvtWork *sys, CtvtComm *comm, u8 member) {
    return comm->members[member].infoReceived;
}

CtvtCommMemberInfo *CtvtComm_GetMemberInfo(CommTvtWork *sys, CtvtComm *comm, u8 member) {
    return &comm->members[member].info;
}

CtvtCommMemberInfo *CtvtComm_GetSelfInfo(CommTvtWork *sys, CtvtComm *comm) {
    return &comm->selfInfo;
}

BOOL CtvtComm_IsMemberActive(CommTvtWork *sys, CtvtComm *comm, u8 member) {
    return comm->members[member].active;
}

BOOL CtvtComm_HasMemberCamera(CommTvtWork *sys, CtvtComm *comm, u8 member) {
    return comm->members[member].info.cameraEnabled;
}

BOOL CtvtComm_CanMemberExchangePhotos(CommTvtWork *sys, CtvtComm *comm, u8 member) {
    return comm->members[member].info.canExchangePhotos;
}

DrawCommand *CtvtComm_GetDrawSlot(CommTvtWork *sys, CtvtComm *comm, BOOL *full) {
    u8 next = comm->drawWriteIndex + 1;

    if (next >= CTVT_COMM_DRAW_QUEUE_SIZE) {
        next -= CTVT_COMM_DRAW_QUEUE_SIZE;
    }
    if (next == comm->drawReadIndex) {
        *full = TRUE;
        return NULL;
    }
    *full = FALSE;
    return &comm->drawQueue[comm->drawWriteIndex];
}

void CtvtComm_CommitDrawSlot(CommTvtWork *sys, CtvtComm *comm) {
    comm->drawWriteIndex++;
    if (comm->drawWriteIndex >= CTVT_COMM_DRAW_QUEUE_SIZE) {
        comm->drawWriteIndex -= CTVT_COMM_DRAW_QUEUE_SIZE;
    }
}

void CtvtComm_StartInviteTimer(CommTvtWork *sys, CtvtComm *comm) {
    comm->inviteTimer = CTVT_COMM_INVITE_FRAMES;
}

BOOL CtvtComm_IsInviteTimerDone(CommTvtWork *sys, CtvtComm *comm) {
    if (comm->inviteTimer == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL CtvtComm_GetUnk3f8(CommTvtWork *sys, CtvtComm *comm) {
    return comm->unk3f8;
}
