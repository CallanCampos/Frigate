#ifndef POKEBW2_DWC_DWC_H
#define POKEBW2_DWC_DWC_H

#include "types.h"
#include "nitro/rtc.h"

// Nintendo's Wi-Fi Connection library (NitroDWC), in main and overlay 11. The ROM names none of it; what these
// functions do is read off their callers. The account functions carry the SDK's names, chosen from what they do

// The player's account and a friend's, as the save keeps them: DWCAccUserData and DWCAccFriendData of the SDK
typedef struct DWCUserData {
    u8 data[0x40];
} DWCUserData;

typedef struct DWCFriendData {
    u8 data[0xc];
} DWCFriendData;

BOOL DWC_IsValidFriendData(const DWCFriendData *friendData);
BOOL DWC_CheckHasProfile(const DWCUserData *userData);
BOOL DWC_CheckValidConsole(const DWCUserData *userData);
s32 DWC_GetGsProfileId(const DWCUserData *userData, const DWCFriendData *friendData);
void DWC_CreateExchangeToken(const DWCUserData *userData, DWCFriendData *token);
// The friend key of the player's user data
u64 func_02057ec4(const DWCUserData *userData);
// Clears the error, unless it is 9
void func_02058490(void);

// Starts connecting to the access point; one of the calls that end the connection after a server error; and the
// connection's state
void func_ov011_0215dec0(void);
void func_ov011_0215fb78(void);
int func_ov011_0215df40(void);

// The server's date and time
BOOL func_ov011_0215dda8(RTCDate *date, RTCTime *time);

// Asks the server whether count words are bad, and runs the request, returning 2 once it succeeded
BOOL func_ov011_0216bea4(const u16 **words, int count, const char *reserved, int timeout, char *result,
                         int *badWordCount, int region);
int func_ov011_0216bed4(void);

#endif // POKEBW2_DWC_DWC_H
