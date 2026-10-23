#ifndef POKEBW2_APP_COMM_TVT_CTVT_COMM_H
#define POKEBW2_APP_COMM_TVT_CTVT_COMM_H

#include "types.h"
#include "app/comm_tvt/draw_system.h"
#include "gfl/heap.h"
#include "save/player_info.h"
#include "struct_decls.h"

// The Xtransceiver's communication: the wireless connection, the members of the call and what they send each other

// No member: the talker when nobody talks
#define CTVT_COMM_NONE 0xff

// How to connect, which CtvtComm_SetNextConnectType asks for
enum {
    CTVT_CONNECT_NONE,
    CTVT_CONNECT_DISCONNECT,
    CTVT_CONNECT_PARENT,
    CTVT_CONNECT_SCAN,
    CTVT_CONNECT_MAC,
    CTVT_CONNECT_EXISTING,
};

// What the commands of type 0x2000 tell the other members
enum {
    CTVT_PACKET_READY,
    CTVT_PACKET_TALKER,
    CTVT_PACKET_ZOOM,
    CTVT_PACKET_DRAW_INDEX,
    CTVT_PACKET_UNK4,
    CTVT_PACKET_UNK5,
    CTVT_PACKET_REQUEST_TALK,
    CTVT_PACKET_PLAY_VOICE,
    CTVT_PACKET_CANCEL_TALK,
    CTVT_PACKET_TALK_DONE,
    CTVT_PACKET_UNK_A,
    CTVT_PACKET_UNK_B,
    CTVT_PACKET_UNK_C,
    CTVT_PACKET_UNK_D,
    CTVT_PACKET_UNK_E,
    CTVT_PACKET_UNK_F,
    CTVT_PACKET_UNK_10,
    CTVT_PACKET_UNK_11,
};

// The beacon of this machine, which the machines that scan see
struct CtvtCommBeacon {
    PlayerInfo player;
    u8 memberCount;
    u8 cameraEnabled : 1;
    // Only the machines listed in inviteMacs may join
    u8 inviteOnly : 1;
    u8 inviteMacs[3][6];
};

// What each member tells the others about itself. The player's info is a copy of its bytes, so this is sent
// without padding
struct CtvtCommMemberInfo {
    u8 playerInfo[sizeof(PlayerInfo)];
    u8 cameraEnabled;
    u8 canExchangePhotos;
};

// A chunk of the talker's voice, packed with IMA ADPCM
typedef struct {
    u16 size;
    u16 playSize;
    u8 isLast;
    u8 chunk;
    // An index into the playback speeds
    u8 speed;
    u8 padding;
    s8 data[0x200];
} CtvtVoicePacket;

// What the minigames send each other, merged with what is still waiting to be sent
typedef struct {
    union {
        u32 value;
        u8 values[4];
    };
    u8 type;
    u8 unk5[2];
    u8 unk7;
    u8 mask;
    u16 frame;
} CtvtGamePacket;

typedef struct {
    u32 data[4];
} CtvtGameData;

CtvtComm *CtvtComm_Create(CommTvtWork *sys, HeapID heapId);
void CtvtComm_Delete(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_Update(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_Disconnect(CommTvtWork *sys, CtvtComm *comm);
BOOL CtvtComm_IsDone(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_SetNextConnectType(CommTvtWork *sys, CtvtComm *comm, int type);
int CtvtComm_GetConnectType(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_SetParentMac(CommTvtWork *sys, CtvtComm *comm, const u8 *mac);
BOOL CtvtComm_IsConnected(CommTvtWork *sys, CtvtComm *comm);
// Whether mac is one of the machines the beacon invites
BOOL CtvtComm_IsInvited(const CtvtCommBeacon *beacon, const u8 *mac);
void CtvtComm_SendZoom(CommTvtWork *sys, CtvtComm *comm, BOOL zoomed);
BOOL CtvtComm_SendPacket(CommTvtWork *sys, CtvtComm *comm, u8 type, u32 value);
BOOL CtvtComm_SendPacketAll(CommTvtWork *sys, CtvtComm *comm, u8 type, u32 value);
BOOL CtvtComm_SendPacketData(CommTvtWork *sys, CtvtComm *comm, u8 type, const void *value);
BOOL CtvtComm_SendVoice(CommTvtWork *sys, CtvtComm *comm, CtvtVoicePacket *packet);
void CtvtComm_QueueGameCommand(CommTvtWork *sys, CtvtComm *comm, u16 command);
void CtvtComm_QueueGamePacket(CtvtComm *comm, CtvtGamePacket packet, u8 type);
void CtvtComm_QueueGameData(CtvtComm *comm, CtvtGameData data);
void CtvtComm_ScanInvited(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_ScanAll(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_ScanNone(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_ResetSession(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_StartSync(CommTvtWork *sys, CtvtComm *comm, int no);
BOOL CtvtComm_IsSynced(CommTvtWork *sys, CtvtComm *comm, int no);
u8 CtvtComm_GetSelfNetId(CommTvtWork *sys, CtvtComm *comm);
// The member talking, or CTVT_COMM_NONE
u8 CtvtComm_GetTalker(CommTvtWork *sys, CtvtComm *comm);
u8 CtvtComm_GetUnk3dc(CommTvtWork *sys, CtvtComm *comm, u8 index);
BOOL CtvtComm_IsVoiceBusy(CommTvtWork *sys, CtvtComm *comm);
CtvtCommBeacon *CtvtComm_GetBeacon(CommTvtWork *sys, CtvtComm *comm);
CtvtVoicePacket *CtvtComm_GetVoicePacket(CommTvtWork *sys, CtvtComm *comm);
BOOL CtvtComm_IsMemberInfoReceived(CommTvtWork *sys, CtvtComm *comm, u8 member);
CtvtCommMemberInfo *CtvtComm_GetMemberInfo(CommTvtWork *sys, CtvtComm *comm, u8 member);
CtvtCommMemberInfo *CtvtComm_GetSelfInfo(CommTvtWork *sys, CtvtComm *comm);
BOOL CtvtComm_IsMemberActive(CommTvtWork *sys, CtvtComm *comm, u8 member);
BOOL CtvtComm_HasMemberCamera(CommTvtWork *sys, CtvtComm *comm, u8 member);
BOOL CtvtComm_CanMemberExchangePhotos(CommTvtWork *sys, CtvtComm *comm, u8 member);
DrawCommand *CtvtComm_GetDrawSlot(CommTvtWork *sys, CtvtComm *comm, BOOL *full);
void CtvtComm_CommitDrawSlot(CommTvtWork *sys, CtvtComm *comm);
void CtvtComm_StartInviteTimer(CommTvtWork *sys, CtvtComm *comm);
BOOL CtvtComm_IsInviteTimerDone(CommTvtWork *sys, CtvtComm *comm);
BOOL CtvtComm_GetUnk3f8(CommTvtWork *sys, CtvtComm *comm);

#endif // POKEBW2_APP_COMM_TVT_CTVT_COMM_H
