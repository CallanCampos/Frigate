#ifndef POKEBW2_DWC_ACCOUNT_H
#define POKEBW2_DWC_ACCOUNT_H

#include "types.h"

// The player's account in DWC (TWL-DWC 5.3's dwc_account.c, in ARM9 main): the user data that the save keeps, and the
// 12-byte friend data that names a friend, or the player, in one of three ways. The ROM names none of it. The names are
// NitroDWC's own, as its libraries' symbols and ntrtwl's reconstruction of NitroDWC have them, where the code shows
// them, and ours, in the SDK's style, elsewhere. DWC_FRIEND_FLAG_4 and _5, and the DWCi_Acc_ functions of flags 4 and
// 5, are placeholders until the overlay 11 code that uses them (0x0215e572, 0x0215ead6) shows what they mean.

// The SDK's DWCstAccFriendData and its four views, as NitroDWC's base/dwc_account.h has them. Its first word holds the
// high 11 bits of a 43-bit user ID in its low bits and 21 bits of flags above them; the bottom two flag bits are the
// kind of ID that the other two words hold
typedef struct DWCstAccFlag {
    u32 flags;
    u32 reserved;
    u32 reserved1;
} DWCstAccFlag;

typedef struct DWCstAccLoginId {
    u32 id_data;
    u32 userid_lo32;
    u32 playerid;
} DWCstAccLoginId;

typedef struct DWCstAccFriendKey {
    u32 id_data;
    u32 friendkey_lo32;
    u32 friendkey_hi32;
} DWCstAccFriendKey;

typedef struct DWCstAccGsProfileId {
    u32 id_data;
    int id;
    u32 reserved;
} DWCstAccGsProfileId;

typedef union DWCstAccFriendData {
    DWCstAccFlag flags;
    DWCstAccLoginId login_id;
    DWCstAccFriendKey friend_key;
    DWCstAccGsProfileId gs_profile_id;
} DWCstAccFriendData;

typedef DWCstAccFriendData DWCFriendData;

// The kinds of friend data
#define DWC_ACC_FRIENDDATA_NODATA 0
// A login ID: the user ID, and in word 2 a 32-bit player ID
#define DWC_ACC_FRIENDDATA_LOGIN_ID 1
// A friend key, the 64-bit number that a player gives to friends, in words 1 and 2
#define DWC_ACC_FRIENDDATA_FRIEND_KEY 2
// A GameSpy profile ID, in word 1
#define DWC_ACC_FRIENDDATA_GS_PROFILE_ID 3

// The flags above the kind. Only a profile ID gets the first, once the server has confirmed it. The second marks a
// buddy, a friend whose status DWCRap reads (DWCRap_UpdateFriendStatus does it for confirmed profile IDs with it); the
// code sets and clears the other two one by one
#define DWC_ACC_FRIENDDATA_ISBUDDY 0x4
#define DWC_FRIEND_FLAG_BUDDY 0x8
#define DWC_FRIEND_FLAG_4 0x10
#define DWC_FRIEND_FLAG_5 0x20

// A DWCstAccUserData of the SDK, which the save keeps for the player
typedef struct DWCstAccUserData {
    int size;
    // The login ID that the library made for this save: the console's Wi-Fi ID and a random player ID
    DWCstAccLoginId pseudo;
    // The login ID that the server has authenticated for the profile, once there is one
    DWCstAccLoginId authentic;
    int gs_profile_id;
    // Bit 0 is set while the data has changed and is still to be saved
    int flag;
    u32 gamecode;
    int reserved[5];
    // Of the 0x3c bytes before it
    u32 crc32;
} DWCstAccUserData;

typedef DWCstAccUserData DWCUserData;

#define DWC_ACC_USERDATA_DIRTY 0x1

// The user ID of a login ID: 43 bits
u64 DWCi_Acc_GetUserId(const DWCstAccLoginId *loginId);
// Sets the user ID of a login ID and its player ID
void DWCi_Acc_SetUserId(DWCstAccLoginId *loginId, u64 userId);
void DWCi_Acc_SetPlayerId(DWCstAccLoginId *loginId, u32 playerId);

// Whether friend data is a buddy: a confirmed profile ID that has the buddy flag; whether it is a confirmed profile ID
// (our name, after DWC_ACC_FRIENDDATA_ISBUDDY: the SDK inlines this test in DWC_IsBuddyFriendData and has no function
// of it); whether it has the buddy flag and whether it has the fifth
BOOL DWC_IsBuddyFriendData(const DWCFriendData *friendData);
BOOL DWCi_Acc_IsBuddyData(const DWCFriendData *friendData);
BOOL DWCi_Acc_IsBuddyFlag(const DWCFriendData *friendData);
BOOL DWCi_Acc_IsFlag5(const DWCFriendData *friendData);
// The kind of friend data (DWC_ACC_FRIENDDATA_*)
int DWC_GetFriendDataType(const DWCFriendData *friendData);
// Marks a profile ID as confirmed; sets or clears the buddy, fourth and fifth flags
void DWCi_SetBuddyFriendData(DWCFriendData *friendData);
void DWCi_Acc_SetBuddyFlag(DWCFriendData *friendData, BOOL on);
void DWCi_Acc_SetFlag4(DWCFriendData *friendData, BOOL on);
void DWCi_Acc_SetFlag5(DWCFriendData *friendData, BOOL on);

// Whether a friend key is the one that the player's game makes for its profile ID
BOOL DWC_CheckFriendKey(const DWCUserData *userData, u64 friendKey);
// Writes the low bits of a value as base 32 text, the most significant digit first, and a NUL
void DWCi_Acc_U64ToString32(u64 value, int bits, char *dest);
// The login name of a login ID in a game: 20 characters and a NUL
void DWCi_Acc_LoginIdToUserName(const DWCstAccLoginId *loginId, u32 gameCode, char *dest);
// Makes a login ID from the console's Wi-Fi ID (or the pending one, while none is confirmed) and a random player ID
void DWCi_Acc_CreateTempLoginId(DWCstAccLoginId *loginId);
// Whether a login ID is made from the console's Wi-Fi ID (or the pending one)
BOOL DWCi_Acc_CheckConsoleUserId(const DWCstAccLoginId *loginId);
// Whether friend data is a login ID; whether the user data has an authenticated one
BOOL DWCi_Acc_IsValidLoginId(const DWCstAccLoginId *loginId);
BOOL DWCi_Acc_IsAuthentic(const DWCUserData *userData);
BOOL DWC_IsValidFriendData(const DWCFriendData *friendData);
BOOL DWCi_Acc_IsValidFriendData(const DWCFriendData *friendData);
// Clears the user data and makes it for the game that the library was started with
void DWC_CreateUserData(DWCUserData *userData);
// Whether the checksum of the user data is right
BOOL DWC_CheckUserData(const DWCUserData *userData);
BOOL DWC_CheckValidConsole(const DWCUserData *userData);
BOOL DWC_CheckHasProfile(const DWCUserData *userData);
// Authenticates a login ID for a profile ID
void DWCi_Acc_SetLoginIdToUserData(DWCUserData *userData, const DWCstAccLoginId *loginId, int profileId);
// Whether the user data has changed since it was saved; and clears that
BOOL DWC_CheckDirtyFlag(const DWCUserData *userData);
void DWC_ClearDirtyFlag(DWCUserData *userData);
// The friend key of friend data, or 0 when it is not one
u64 DWC_GetFriendKey(const DWCFriendData *friendData);
int DWC_GetGsProfileId(const DWCUserData *userData, const DWCFriendData *friendData);
// The player's own friend key, or 0 while the player has no profile ID
u64 DWC_CreateFriendKey(const DWCUserData *userData);
// Makes friend data from a friend key
void DWC_CreateFriendKeyToken(DWCFriendData *token, u64 friendKey);
void DWC_CreateExchangeToken(const DWCUserData *userData, DWCFriendData *token);
// Makes friend data from a profile ID
void DWC_SetGsProfileId(DWCFriendData *token, int profileId);
// The login name of a login ID in the user data's game
void DWC_LoginIdToUserName(const DWCUserData *userData, const DWCFriendData *loginId, char *dest);
// Whether two friends' data name the same friend
BOOL DWC_IsEqualFriendData(const DWCFriendData *friendData, const DWCFriendData *other);

#endif // POKEBW2_DWC_ACCOUNT_H
