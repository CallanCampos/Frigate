#ifndef POKEBW2_DWC_DWC_H
#define POKEBW2_DWC_DWC_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/connectinet.h"
#include "dwc/error.h"
#include "dwc/gsMemory.h"
// GameSpy's stats (gstats.c): the game's name, which the server URLs are made from, the secret key and the stats
// server's host, which the start sets
#include "dwc/gstats.h"
#include "dwc/init.h"
#include "gfl/heap.h"
#include "nitro/rtc.h"

// Nintendo's Wi-Fi Connection library (NitroDWC), in main and overlay 11. The ROM names none of it; what these
// functions do is read off their callers. The account and friend data are in dwc/account.h, the error in dwc/error.h
// and the start in dwc/init.h

// Starts connecting to the access point; one of the calls that end the connection after a server error; and the
// connection's state
void func_ov011_0215dec0(void);
// Starts the connection to the Nintendo Wi-Fi Connection with the access point's name (an empty one takes the
// default) and the buffer of the connection, returning FALSE if it could not start
BOOL func_ov011_0215de44(const char *apName, void *buffer);
// The connection's state, 3 once connected, ending it after a state from 3 to 5
int func_ov011_0215dea8(void);
void func_ov011_0215fb78(void);
int func_ov011_0215df40(void);

// The server's date and time
BOOL func_ov011_0215dda8(RTCDate *date, RTCTime *time);

// Asks the server whether count words are bad, and runs the request, returning 2 once it succeeded
BOOL func_ov011_0216bea4(const u16 **words, int count, const char *reserved, int timeout, char *result,
                         int *badWordCount, int region);
int func_ov011_0216bed4(void);

// Asked how well another machine fits a random match, and the function's work
typedef int (*DWCEvalFunc)(int index, void *work);

// Not in dwc_rap.c, past it in the overlay
int func_ov011_02160344(void);

// Overlay 11's: starting, logging in and the friend list
void func_ov011_0215fa0c(void *userData, int productId, const char *gameName, const char *secretKey, int a4,
                         DWCFriendData *friendList, int friendListLen);
void func_ov011_0215fdf0(const u16 *name, const char *reserved, void (*callback)(int error), void *param);
BOOL func_ov011_0215fe94(const char *name, void (*callback)(int error), void *userData,
                         void (*statusCallback)(int index, u8 status, const char *location, void *param),
                         void *statusParam, void (*deleteCallback)(int deleteIndex, int srcIndex, void *param),
                         void *deleteParam);
void func_ov011_0215fc50(void);
void func_ov011_02160170(void);
void func_ov011_0215e4d4(void (*callback)(int index, void *param), void *param);
u8 func_ov011_0215e410(const DWCFriendData *friendData, char *statusData, int *size);
void func_ov011_0215e47c(const char *statusData, int size);

// Matching: with the key and number, anybody, or a friend, and the callbacks that tell how it went
typedef void (*DWCMatchedFunc)(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
typedef void (*DWCNewClientFunc)(int index, void *param);
typedef BOOL (*DWCAttemptFunc)(void *param);
BOOL func_ov011_0215fef8(int type, u8 numEntry, const char *query, DWCMatchedFunc matched, void *matchedParam,
                         DWCNewClientFunc newClient, void *newClientParam, DWCEvalFunc eval, void *evalParam,
                         DWCAttemptFunc attempt, int *attemptParam, void *param);
BOOL func_ov011_0215ff7c(int type, u8 numEntry, DWCMatchedFunc matched, void *matchedParam, DWCNewClientFunc newClient,
                         void *newClientParam, DWCAttemptFunc attempt, int *attemptParam, void *param);
BOOL func_ov011_0215fff4(int type, int friendIndex, DWCMatchedFunc matched, void *matchedParam,
                         DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt, int *attemptParam,
                         void *param);
BOOL func_ov011_02160ed4(int index, const char *key, const char *value);

// The machines: their IDs and which are there
u8 func_ov011_021602c0(void);
BOOL func_ov011_021602a0(void);
void func_ov011_02160284(u8 netId);
BOOL func_ov011_02160370(u8 netId);
BOOL func_ov011_02168998(u8 netId);
u32 func_ov011_02168ad4(u32 netIds, const void *data, int size);
void func_ov011_02168b84(u8 netId, void *buffer, int size);
BOOL func_ov011_02168ca4(u8 netId, int timeout);
// The functions called when data has been sent, when it has been received, when a connection has been closed and
// when a machine has not sent for too long
typedef void (*DWCSendFunc)(int size, u8 netId);
typedef void (*DWCRecvFunc)(u8 netId, u8 *data, int size);
typedef void (*DWCClosedFunc)(int error, BOOL a1, BOOL a2, u8 netId, int index, void *param);
typedef void (*DWCTimeoutFunc)(u8 netId);
void func_ov011_02168bb0(DWCSendFunc func, int a1);
void func_ov011_02168bd8(DWCRecvFunc func, int a1);
void func_ov011_02160150(DWCClosedFunc func, int a1);
void func_ov011_02168c00(DWCTimeoutFunc func, int a1);

// Overlay 11's, used with the matching keys: adds a key with a value that is read when the match runs, and reads
// a candidate's key
u8 func_ov011_02160e10(u8 keyId, const char *key, const int *valueSrc);
int func_ov011_02160f98(int index, const char *key, int defaultValue);

// Overlay 11's, which the library's start in main (dwc_init.c) calls: chooses the servers that the library talks to
// by the kind of start, 0 for development, 1 for retail (it knows a 2, which the start in main never gives). It also
// gives GameSpy its memory functions (malloc, free, realloc and memalign), with dwc/gsMemory.h's gsiMemoryCallbacksSet
void func_ov011_02169df8(int server);

#endif // POKEBW2_DWC_DWC_H
