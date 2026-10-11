#ifndef POKEBW2_GFL_DWC_RAP_H
#define POKEBW2_GFL_DWC_RAP_H

#include "types.h"
#include "gfl/heap.h"
#include "dwc/dwc.h"
#include "gfl/net.h"

// dwc_rap.c, in overlay 11: the GFL net's Wi-Fi connection, over Nintendo's DWC library

// Called with a Wi-Fi library error the connection has queued: its code, its type and its kind, and the work
typedef void (*DWCRapErrorFunc)(void *work, int code, int type, int error);
// Asked about a connection event, with its work and three of the event's values
typedef u32 (*DWCRapEventFunc)(void *work, int a1, int event, int a3);

// Asked whether to take a machine in: its friend index, or -1 for one that is nobody's friend, and the game's work
typedef BOOL (*DWCRapRequestFunc)(int index, void *work);
// Called when a connection is made, with the machine's ID and its work
typedef void (*DWCRapConnectFunc)(u16 netId, void *work);
// Called when a machine has gone, with its ID and its work
typedef void (*DWCRapDisconnectFunc)(u8 netId, void *work);

// Whether the Wi-Fi connection has been started
BOOL DWCRap_IsInitialized(void);
// Starts the connection with the player's user data and friend list, returning 0, or 1 or 2 if the user data is
// invalid
int DWCRap_Init(void *userData, DWCFriendData *friendList);
// Sets the functions that machines' data goes to: the one called on the parent, and the one on the children
void DWCRap_SetRecvFuncs(GFLNetRecvFunc parentFunc, GFLNetRecvFunc childFunc);
// Sets the function asked whether to take a machine in
void DWCRap_SetRequestFunc(DWCRapRequestFunc func);
// Starts matching with whoever has the same key, as numEntry machines
BOOL DWCRap_StartMatch(const char *key, int numEntry, BOOL a2, int a3);
// Starts a match of numEntry machines on a query, with a function that rates the candidates
BOOL DWCRap_StartMatchQuery(const char *query, int numEntry, DWCEvalFunc evalFunc, void *evalWork);
// Runs the connection for a frame
int DWCRap_Process(int a0);
// Sends data to the other machines, as a packet of the type
BOOL DWCRap_SendPacket(const void *data, int size, int type);
BOOL DWCRap_Send(const void *data, int size);
// The machine's ID, or -1 when it is not in a match
int DWCRap_GetNetId(void);
void func_ov011_021515a4(void);
// Starts and ends the voice chat
void DWCRap_StartVoiceChat(void);
void DWCRap_StopVoiceChat(void);
// Whether the voice chat is on
int DWCRap_IsVoiceChatActive(void);
void DWCRap_ResetVoiceChat(void);
void DWCRap_SetMic(BOOL a0);
// The kind of a Wi-Fi library error, from its code and type
int DWCRap_GetErrorKind(int code, int type);
BOOL DWCRap_RequestClose(BOOL a0);
BOOL DWCRap_ResetToReady(void);
void DWCRap_Shutdown(void);
// Sets the data that tells friends where this player is, from 0 to 0x20 bytes
void DWCRap_SetOwnStatusData(const char *data, int size);
// A friend's location data, and their state
char *DWCRap_GetFriendStatusData(int index);
u8 DWCRap_GetFriendStatus(int index);
// Connects to a friend, or to anybody if the index is negative; returns 0, or a negative number when it can't yet
int DWCRap_Connect(int friendIndex, int numEntry, BOOL a2);
// The friend index of the connection
int DWCRap_GetFriendIndex(void);
BOOL func_ov011_02151de4(void);
s16 func_ov011_02151dec(void);
int DWCRap_GetLastNetId(void);
s16 func_ov011_02151e24(void);
void func_ov011_02151e40(s16 a0);
BOOL DWCRap_IsUserDataDirty(void);
void DWCRap_ClearUserDataDirty(void);
BOOL DWCRap_IsReady(void);
BOOL DWCRap_IsConnected(void);
int DWCRap_GetStatus(void);
void func_ov011_02151fec(int a0);
BOOL DWCRap_IsEnding(void);
void DWCRap_SetReportError(int a0);
// Sets the function called when the connection is lost
void DWCRap_SetErrorFunc(DWCRapErrorFunc func, void *work);
// Sets a function asked about connection events, with its work
void DWCRap_SetEventFunc(DWCRapEventFunc func, void *work);

#endif // POKEBW2_GFL_DWC_RAP_H
