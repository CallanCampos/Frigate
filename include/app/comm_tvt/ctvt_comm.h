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
    CTVT_PACKET_ALLOW_DRAW,
    CTVT_PACKET_ALLOW_VOICE,
    CTVT_PACKET_REQUEST_TALK,
    CTVT_PACKET_PLAY_VOICE,
    CTVT_PACKET_CANCEL_TALK,
    CTVT_PACKET_TALK_DONE,
    CTVT_PACKET_EXIT,
    CTVT_PACKET_GAME_INVITE,
    CTVT_PACKET_GAME_JOIN,
    CTVT_PACKET_GAME_CANCEL_REQUEST,
    CTVT_PACKET_GAME_STARTING,
    CTVT_PACKET_GAME_CANCEL,
    CTVT_PACKET_GAME_READY,
    CTVT_PACKET_GAME_START,
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
    // Where a member touched, for CTVT_GAME_PACKET_TOUCH
    u8 touchX;
    u8 touchY;
    // The target slot to launch, for CTVT_GAME_PACKET_SPAWN_TARGET
    u8 target;
    u8 mask;
    u16 frame;
} CtvtGamePacket;

// The types of CtvtGamePacket
enum {
    // The host launches a target
    CTVT_GAME_PACKET_SPAWN_TARGET,
    // The host's seed for the game's random numbers
    CTVT_GAME_PACKET_SEED,
    // A member's touch, for the host to check against the targets
    CTVT_GAME_PACKET_TOUCH = 3,
    // The targets the members in mask hit, from the host
    CTVT_GAME_PACKET_HITS,
    // A member pumps, a small or a big pump
    CTVT_GAME_PACKET_PUMP,
    CTVT_GAME_PACKET_PUMP_BIG,
    // The host grows the balloons of the members in mask
    CTVT_GAME_PACKET_GROW_BALLOONS,
};

// The minigames' commands, which CtvtComm_QueueGameCommand sends
enum {
    // A member joins the host's next round
    CTVT_GAME_COMMAND_JOIN,
    CTVT_GAME_COMMAND_CHILD_QUIT,
    // The host tells the members that all have joined
    CTVT_GAME_COMMAND_ALL_JOINED,
    CTVT_GAME_COMMAND_HOST_QUIT,
    CTVT_GAME_COMMAND_READY,
    // The host starts the next round
    CTVT_GAME_COMMAND_REPLAY,
};

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
u8 CtvtComm_GetGameCharacter(CommTvtWork *sys, CtvtComm *comm, u8 index);
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
BOOL CtvtComm_CanSendVoice(CommTvtWork *sys, CtvtComm *comm);

#endif // POKEBW2_APP_COMM_TVT_CTVT_COMM_H
