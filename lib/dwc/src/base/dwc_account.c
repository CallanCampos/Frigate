// DWC's account (TWL-DWC 5.3's dwc_account.c, a guessed name; the ROM names none of it, and the file is ARM code in
// ARM9 main like the rest of the library's base): the user data that the save keeps and the 12-byte friend data, whose
// layout is in dwc/account.h. A friend is a login ID, a friend key or a profile ID; a login ID is a 43-bit user ID and
// a random 32-bit player ID, a friend key is the profile ID and a 7-bit checksum made from it and the game code, and
// the login name that the server is given is both IDs and the game code as text.

#include "dwc/account.h"
#include "dwc/bm.h"
#include "dwc/dwc.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The friend data's header: the high bits of the user ID in the low 11, and the flags in the 21 above them
#define DWC_ACC_USERID_HI32_MASK 0x7ff
#define DWC_ACC_FLAGS_SHIFT 11
#define DWC_ACC_FLAGS_MASK 0x1fffff
// The kind of friend data, in the bottom bits of the flags
#define DWC_ACC_FRIENDDATA_MASK 0x3

// How the user data's checksum is made
#define CRC32_POLY 0xedb88320
// The bytes of the user data that the checksum covers
#define USER_DATA_CHECKED_SIZE (sizeof(DWCUserData) - sizeof(u32))

static BOOL DWC_Acc_CheckFriendKey(u64 friendKey, u32 gameCode, int useMd5);
static BOOL DWCi_Acc_IsDirty(const DWCUserData *userData);
static void DWCi_Acc_ClearDirty(DWCUserData *userData);
static BOOL DWCi_Acc_SetMaskBits(u32 *word, u32 value, u32 shift, u32 mask);
static u32 DWCi_Acc_GetPlayerId(const DWCstAccLoginId *loginId);
static u64 DWCi_Acc_GetFriendKey(const DWCstAccFriendKey *friendKey);
static int DWCi_Acc_GetGsProfileId(const DWCstAccGsProfileId *profileId);
static void DWCi_Acc_SetFriendKey(DWCstAccFriendKey *data, u64 friendKey);
static void DWCi_Acc_SetGsProfileId(DWCstAccGsProfileId *data, int profileId);
static u32 DWCi_Acc_GetFlags(const DWCstAccFlag *flags);
static int DWCi_Acc_GetFlag_DataType(const DWCstAccFlag *flags);
static void DWCi_Acc_SetFlags(DWCstAccFlag *data, u32 flags);
static void DWCi_Acc_SetFlag_DataType(DWCstAccFlag *data, int type);
static u64 DWC_Acc_CreateFriendKey(u32 profileId, u32 gameCode, BOOL useMd5);
static u32 DWC_Acc_FriendKeyToGsProfileId(u64 friendKey, u32 gameCode);
static void DWCi_Acc_CreateUserData(DWCUserData *userData, u32 gameCode);

// Puts a value in the bits that a mask selects after a shift; FALSE if it has bits outside the mask
static BOOL DWCi_Acc_SetMaskBits(u32 *word, u32 value, u32 shift, u32 mask) {
    if (value & ~mask) {
        return FALSE;
    }
    *word = (*word & ~(mask << shift)) | (value << shift);
    return TRUE;
}

// The 43-bit user ID of a login ID
u64 DWCi_Acc_GetUserId(const DWCstAccLoginId *loginId) {
    return ((u64)(loginId->id_data & DWC_ACC_USERID_HI32_MASK) << 32) | loginId->userid_lo32;
}

// The player ID of a login ID
static u32 DWCi_Acc_GetPlayerId(const DWCstAccLoginId *loginId) {
    return loginId->playerid;
}

// The friend key of friend data that holds one
static u64 DWCi_Acc_GetFriendKey(const DWCstAccFriendKey *friendKey) {
    return ((u64)friendKey->friendkey_hi32 << 32) | friendKey->friendkey_lo32;
}

// The profile ID of friend data that holds one
static int DWCi_Acc_GetGsProfileId(const DWCstAccGsProfileId *profileId) {
    return profileId->id;
}

// Sets the user ID of a login ID; the header keeps what it had if the ID's high bits don't fit in 11
void DWCi_Acc_SetUserId(DWCstAccLoginId *loginId, u64 userId) {
    DWCi_Acc_SetMaskBits(&loginId->id_data, userId >> 32, 0, DWC_ACC_USERID_HI32_MASK);
    loginId->userid_lo32 = (u32)userId;
}

void DWCi_Acc_SetPlayerId(DWCstAccLoginId *loginId, u32 playerId) {
    loginId->playerid = playerId;
}

static void DWCi_Acc_SetFriendKey(DWCstAccFriendKey *data, u64 friendKey) {
    data->friendkey_lo32 = (u32)friendKey;
    data->friendkey_hi32 = friendKey >> 32;
}

static void DWCi_Acc_SetGsProfileId(DWCstAccGsProfileId *data, int profileId) {
    data->id = profileId;
}

// The 21 bits of flags, kind included
static u32 DWCi_Acc_GetFlags(const DWCstAccFlag *flags) {
    return (flags->flags >> DWC_ACC_FLAGS_SHIFT) & DWC_ACC_FLAGS_MASK;
}

// The kind of friend data
static int DWCi_Acc_GetFlag_DataType(const DWCstAccFlag *flags) {
    return DWCi_Acc_GetFlags(flags) & DWC_ACC_FRIENDDATA_MASK;
}

BOOL DWC_IsBuddyFriendData(const DWCFriendData *friendData) {
    if (DWCi_Acc_IsBuddyData(friendData) && DWCi_Acc_IsBuddyFlag(friendData)) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWCi_Acc_IsBuddyData(const DWCFriendData *friendData) {
    if (DWCi_Acc_GetFlag_DataType(&friendData->flags) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
        return (DWCi_Acc_GetFlags(&friendData->flags) & DWC_ACC_FRIENDDATA_ISBUDDY) == DWC_ACC_FRIENDDATA_ISBUDDY;
    }
    return FALSE;
}

BOOL DWCi_Acc_IsBuddyFlag(const DWCFriendData *friendData) {
    return (DWCi_Acc_GetFlags(&friendData->flags) & DWC_FRIEND_FLAG_BUDDY) == DWC_FRIEND_FLAG_BUDDY;
}

BOOL DWCi_Acc_IsFlag5(const DWCFriendData *friendData) {
    return (DWCi_Acc_GetFlags(&friendData->flags) & DWC_FRIEND_FLAG_5) == DWC_FRIEND_FLAG_5;
}

int DWC_GetFriendDataType(const DWCFriendData *friendData) {
    return DWCi_Acc_GetFlag_DataType(&friendData->flags);
}

// Sets the flags, kind included
static void DWCi_Acc_SetFlags(DWCstAccFlag *data, u32 flags) {
    DWCi_Acc_SetMaskBits(&data->flags, flags, DWC_ACC_FLAGS_SHIFT, DWC_ACC_FLAGS_MASK);
}

// Sets the kind of friend data
static void DWCi_Acc_SetFlag_DataType(DWCstAccFlag *data, int type) {
    u32 flags = DWCi_Acc_GetFlags(data) & ~DWC_ACC_FRIENDDATA_MASK;

    DWCi_Acc_SetFlags(data, flags | type);
}

void DWCi_SetBuddyFriendData(DWCFriendData *friendData) {
    u32 flags;

    if (DWCi_Acc_GetFlag_DataType(&friendData->flags) != DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
        return;
    }
    flags = DWCi_Acc_GetFlags(&friendData->flags) | DWC_ACC_FRIENDDATA_ISBUDDY;
    DWCi_Acc_SetFlags(&friendData->flags, flags);
}

void DWCi_Acc_SetBuddyFlag(DWCFriendData *friendData, BOOL on) {
    u32 flags;

    if (on) {
        flags = DWCi_Acc_GetFlags(&friendData->flags) | DWC_FRIEND_FLAG_BUDDY;
    } else {
        flags = DWCi_Acc_GetFlags(&friendData->flags) & ~DWC_FRIEND_FLAG_BUDDY;
    }
    DWCi_Acc_SetFlags(&friendData->flags, flags);
}

void DWCi_Acc_SetFlag4(DWCFriendData *friendData, BOOL on) {
    u32 flags;

    if (on) {
        flags = DWCi_Acc_GetFlags(&friendData->flags) | DWC_FRIEND_FLAG_4;
    } else {
        flags = DWCi_Acc_GetFlags(&friendData->flags) & ~DWC_FRIEND_FLAG_4;
    }
    DWCi_Acc_SetFlags(&friendData->flags, flags);
}

void DWCi_Acc_SetFlag5(DWCFriendData *friendData, BOOL on) {
    u32 flags;

    if (on) {
        flags = DWCi_Acc_GetFlags(&friendData->flags) | DWC_FRIEND_FLAG_5;
    } else {
        flags = DWCi_Acc_GetFlags(&friendData->flags) & ~DWC_FRIEND_FLAG_5;
    }
    DWCi_Acc_SetFlags(&friendData->flags, flags);
}

// The friend key of a profile ID: the ID, and above it a 7-bit checksum of the ID and the game code, which is a CRC-8
// or the top bits of an MD5
static u64 DWC_Acc_CreateFriendKey(u32 profileId, u32 gameCode, BOOL useMd5) {
    u32 data[2];
    u8 digest[16];
    MATHCRC8Table table;
    MATHMD5Context md5;
    u32 checksum;

    data[0] = profileId;
    data[1] = gameCode;
    if (!useMd5) {
        MATHi_CRC8InitTable(&table, 7);
        checksum = MATH_CalcCRC8(&table, data, sizeof(data));
    } else {
        MATH_MD5Init(&md5);
        MATH_MD5Update(&md5, data, sizeof(data));
        MATH_MD5GetHash(&md5, digest);
        checksum = digest[0] >> 1;
    }
    checksum &= 0x7f;
    return ((u64)checksum << 32) | profileId;
}

// Whether a friend key is the one that the player's game makes for its profile ID
BOOL DWC_CheckFriendKey(const DWCUserData *userData, u64 friendKey) {
    return DWC_Acc_CheckFriendKey(friendKey, userData->gamecode, TRUE);
}

// Whether a friend key is made by the checksum for a game; the profile ID in it must be above 0
static BOOL DWC_Acc_CheckFriendKey(u64 friendKey, u32 gameCode, int useMd5) {
    BOOL result = FALSE;
    s32 profileId = (s32)(friendKey & 0xffffffff);

    if (profileId <= 0) {
        return result;
    }
    if (DWC_Acc_CreateFriendKey(profileId, gameCode, useMd5) == friendKey) {
        result = TRUE;
    }
    return result;
}

// The profile ID in a friend key, if the key is right for the game with either checksum; 0 if not
static u32 DWC_Acc_FriendKeyToGsProfileId(u64 friendKey, u32 gameCode) {
    int useMd5;

    for (useMd5 = 0; useMd5 < 2; useMd5++) {
        if (DWC_Acc_CheckFriendKey(friendKey, gameCode, useMd5)) {
            return friendKey & 0xffffffff;
        }
    }
    return 0;
}

// Writes the low bits of a value as base 32 text, most significant digit first, and a NUL
void DWCi_Acc_U64ToString32(u64 value, int bits, char *dest) {
    int i;
    int count = (bits + 4) / 5;
    const char *digits = "0123456789abcdefghijklmnopqrstuv";

    for (i = 0; i < count; i++) {
        *(dest + count - 1 - i) = digits[value & 0x1f];
        value >>= 5;
    }
    dest[count] = '\0';
}

// The login name for a login ID and a game: the user ID as 9 digits, the game code's 4 characters and the player ID as
// 7 digits
void DWCi_Acc_LoginIdToUserName(const DWCstAccLoginId *loginId, u32 gameCode, char *dest) {
    char userIdText[21];
    char playerIdText[21];

    DWCi_Acc_U64ToString32(DWCi_Acc_GetUserId(loginId), 43, userIdText);
    DWCi_Acc_U64ToString32(DWCi_Acc_GetPlayerId(loginId), 32, playerIdText);
    OS_SNPrintf(dest, 21, "%s%c%c%c%c%s", userIdText, (gameCode >> 24) & 0xff, (gameCode >> 16) & 0xff,
                (gameCode >> 8) & 0xff, gameCode & 0xff, playerIdText);
}

// Clears the user data and makes it for a game, with a login ID of its own and no profile
static void DWCi_Acc_CreateUserData(DWCUserData *userData, u32 gameCode) {
    MATHCRC32Table table;

    MI_CpuFill8(userData, 0, sizeof(DWCUserData));
    userData->size = sizeof(DWCUserData);
    userData->gs_profile_id = 0;
    userData->gamecode = gameCode;
    DWCi_Acc_CreateTempLoginId(&userData->pseudo);
    DWCi_Acc_SetFlag_DataType((DWCstAccFlag *)&userData->authentic, DWC_ACC_FRIENDDATA_NODATA);
    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    userData->crc32 = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
    userData->flag |= DWC_ACC_USERDATA_DIRTY;
}

// Makes a login ID of the console's Wi-Fi ID (the pending one until the first is confirmed) and a random player ID,
// from a generator that is seeded with the console's entropy
void DWCi_Acc_CreateTempLoginId(DWCstAccLoginId *loginId) {
    DWCAuthWiFiId wifiId;
    u32 entropy[8];
    MATHRandContext32 rand;
    u32 i;

    OS_GetLowEntropyData(entropy);
    for (i = 1; i < 8; i++) {
        entropy[i] ^= entropy[i - 1];
    }
    MATH_InitRand32(&rand, entropy[7]);
    DWC_Auth_GetId(&wifiId);
    if (wifiId.flag) {
        DWCi_Acc_SetUserId(loginId, wifiId.uId);
    } else {
        DWCi_Acc_SetUserId(loginId, wifiId.notAttestedId);
    }
    DWCi_Acc_SetPlayerId(loginId, MATH_Rand32(&rand, 0));
    DWCi_Acc_SetFlag_DataType((DWCstAccFlag *)loginId, DWC_ACC_FRIENDDATA_LOGIN_ID);
}

// Whether a login ID is made of the console's Wi-Fi ID (or the pending one)
BOOL DWCi_Acc_CheckConsoleUserId(const DWCstAccLoginId *loginId) {
    DWCAuthWiFiId wifiId;

    DWC_Auth_GetId(&wifiId);
    if (wifiId.flag) {
        return DWCi_Acc_GetUserId(loginId) == wifiId.uId;
    } else {
        return DWCi_Acc_GetUserId(loginId) == wifiId.notAttestedId;
    }
}

// Whether friend data is a login ID
BOOL DWCi_Acc_IsValidLoginId(const DWCstAccLoginId *loginId) {
    return DWCi_Acc_GetFlag_DataType((const DWCstAccFlag *)loginId) == DWC_ACC_FRIENDDATA_LOGIN_ID;
}

// Whether the user data has an authenticated login ID
BOOL DWCi_Acc_IsAuthentic(const DWCUserData *userData) {
    return DWCi_Acc_IsValidLoginId(&userData->authentic);
}

BOOL DWC_IsValidFriendData(const DWCFriendData *friendData) {
    return DWCi_Acc_GetFlag_DataType(&friendData->flags) != DWC_ACC_FRIENDDATA_NODATA;
}

BOOL DWCi_Acc_IsValidFriendData(const DWCFriendData *friendData) {
    return DWC_IsValidFriendData(friendData);
}

// Makes the user data for the game that the library was started with
void DWC_CreateUserData(DWCUserData *userData) {
    DWCi_Acc_CreateUserData(userData, DWCi_GetGameCode());
}

BOOL DWC_CheckUserData(const DWCUserData *userData) {
    MATHCRC32Table table;
    u32 crc;

    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    crc = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
    return crc == userData->crc32;
}

BOOL DWC_CheckValidConsole(const DWCUserData *userData) {
    DWCAuthWiFiId wifiId;

    if (DWCi_Acc_GetFlag_DataType((const DWCstAccFlag *)&userData->authentic) == DWC_ACC_FRIENDDATA_NODATA) {
        return TRUE;
    }
    DWC_Auth_GetId(&wifiId);
    if (wifiId.flag == 0) {
        return FALSE;
    }
    return DWCi_Acc_GetUserId(&userData->authentic) == wifiId.uId;
}

BOOL DWC_CheckHasProfile(const DWCUserData *userData) {
    return DWCi_Acc_IsValidLoginId(&userData->authentic) && userData->gs_profile_id > 0;
}

// Authenticates a login ID for a profile, which marks the user data as changed
void DWCi_Acc_SetLoginIdToUserData(DWCUserData *userData, const DWCstAccLoginId *loginId, int profileId) {
    MATHCRC32Table table;

    userData->authentic = *loginId;
    userData->gs_profile_id = profileId;
    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    userData->crc32 = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
    userData->flag |= DWC_ACC_USERDATA_DIRTY;
}

BOOL DWC_CheckDirtyFlag(const DWCUserData *userData) {
    return DWCi_Acc_IsDirty(userData);
}

static BOOL DWCi_Acc_IsDirty(const DWCUserData *userData) {
    return (userData->flag & DWC_ACC_USERDATA_DIRTY) == DWC_ACC_USERDATA_DIRTY;
}

void DWC_ClearDirtyFlag(DWCUserData *userData) {
    DWCi_Acc_ClearDirty(userData);
}

static void DWCi_Acc_ClearDirty(DWCUserData *userData) {
    MATHCRC32Table table;

    userData->flag &= ~DWC_ACC_USERDATA_DIRTY;
    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    userData->crc32 = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
}

// The friend key of friend data, or 0 if it is of another kind
u64 DWC_GetFriendKey(const DWCFriendData *friendData) {
    if (DWCi_Acc_GetFlag_DataType(&friendData->flags) != DWC_ACC_FRIENDDATA_FRIEND_KEY) {
        return 0;
    }
    return DWCi_Acc_GetFriendKey(&friendData->friend_key);
}

int DWC_GetGsProfileId(const DWCUserData *userData, const DWCFriendData *friendData) {
    switch (DWCi_Acc_GetFlag_DataType(&friendData->flags)) {
    case DWC_ACC_FRIENDDATA_FRIEND_KEY:
        return DWC_Acc_FriendKeyToGsProfileId(DWCi_Acc_GetFriendKey(&friendData->friend_key), userData->gamecode);
    case DWC_ACC_FRIENDDATA_GS_PROFILE_ID:
        return DWCi_Acc_GetGsProfileId(&friendData->gs_profile_id);
    case DWC_ACC_FRIENDDATA_LOGIN_ID:
        return -1;
    default:
        return 0;
    }
}

// The player's own friend key, if there is a profile
u64 DWC_CreateFriendKey(const DWCUserData *userData) {
    u64 friendKey = 0;

    if (userData->gs_profile_id != 0) {
        friendKey = DWC_Acc_CreateFriendKey(userData->gs_profile_id, userData->gamecode, TRUE);
    }
    return friendKey;
}

void DWC_CreateFriendKeyToken(DWCFriendData *token, u64 friendKey) {
    MI_CpuFill8(token, 0, sizeof(DWCFriendData));
    DWCi_Acc_SetFriendKey(&token->friend_key, friendKey);
    DWCi_Acc_SetFlag_DataType(&token->flags, DWC_ACC_FRIENDDATA_FRIEND_KEY);
}

void DWC_CreateExchangeToken(const DWCUserData *userData, DWCFriendData *token) {
    MI_CpuFill8(token, 0, sizeof(DWCFriendData));
    if (!DWCi_Acc_IsAuthentic(userData)) {
        token->login_id = userData->pseudo;
        return;
    }
    DWCi_Acc_SetGsProfileId(&token->gs_profile_id, userData->gs_profile_id);
    DWCi_Acc_SetFlag_DataType(&token->flags, DWC_ACC_FRIENDDATA_GS_PROFILE_ID);
}

void DWC_SetGsProfileId(DWCFriendData *token, int profileId) {
    MI_CpuFill8(token, 0, sizeof(DWCFriendData));
    DWCi_Acc_SetGsProfileId(&token->gs_profile_id, profileId);
    DWCi_Acc_SetFlag_DataType(&token->flags, DWC_ACC_FRIENDDATA_GS_PROFILE_ID);
}

void DWC_LoginIdToUserName(const DWCUserData *userData, const DWCFriendData *loginId, char *dest) {
    DWCi_Acc_LoginIdToUserName(&loginId->login_id, userData->gamecode, dest);
}

BOOL DWC_IsEqualFriendData(const DWCFriendData *friendData, const DWCFriendData *other) {
    int type = DWCi_Acc_GetFlag_DataType(&friendData->flags);

    if (type != DWCi_Acc_GetFlag_DataType(&other->flags)) {
        return FALSE;
    }
    if (type == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
        return DWCi_Acc_GetGsProfileId(&friendData->gs_profile_id) == DWCi_Acc_GetGsProfileId(&other->gs_profile_id);
    }
    if (type == DWC_ACC_FRIENDDATA_LOGIN_ID) {
        if (DWCi_Acc_GetUserId(&friendData->login_id) == DWCi_Acc_GetUserId(&other->login_id) &&
            DWCi_Acc_GetPlayerId(&friendData->login_id) == DWCi_Acc_GetPlayerId(&other->login_id)) {
            return TRUE;
        }
        return FALSE;
    }
    if (type == DWC_ACC_FRIENDDATA_FRIEND_KEY) {
        return DWCi_Acc_GetFriendKey(&friendData->friend_key) == DWCi_Acc_GetFriendKey(&other->friend_key);
    }
    return FALSE;
}
