#include "dwc/account.h"
#include "dwc/bm.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "nitro/os.h"

// DWC's account (TWL-DWC 5.3's dwc_account.c, a guessed name; the ROM names none of it, and the file is ARM code in
// ARM9 main like the rest of the library's base): the user data that the save keeps and the 12-byte friend data, whose
// layout is in dwc/account.h. A friend is a login ID, a friend key or a profile ID; a login ID is a 43-bit user ID and
// a random 32-bit player ID, a friend key is the profile ID and a 7-bit checksum made from it and the game code, and
// the login name that the server is given is both IDs and the game code as text.

// The friend data's header: the high bits of the user ID in the low 11, and the flags in the 21 above them
#define USER_ID_HIGH_MASK 0x7ff
#define FLAGS_SHIFT 11
#define FLAGS_MASK 0x1fffff
// The kind of friend data, in the bottom bits of the flags
#define TYPE_MASK 0x3

// How the user data's checksum is made
#define CRC32_POLY 0xedb88320
// The bytes of the user data that the checksum covers
#define USER_DATA_CHECKED_SIZE (sizeof(DWCUserData) - sizeof(u32))

static BOOL func_020578ec(u64 friendKey, u32 gameCode, int useMd5);
static BOOL func_02057dd0(const DWCUserData *userData);
static void func_02057df4(DWCUserData *userData);

// Puts a value in the bits that a mask selects after a shift; FALSE if it has bits outside the mask
static BOOL func_020575d0(u32 *word, u32 value, u32 shift, u32 mask) {
    if (value & ~mask) {
        return FALSE;
    }
    *word = (*word & ~(mask << shift)) | (value << shift);
    return TRUE;
}

// The 43-bit user ID of a login ID
u64 func_020575fc(const DWCFriendData *loginId) {
    return ((u64)(loginId->header & USER_ID_HIGH_MASK) << 32) | loginId->word1;
}

// The player ID of a login ID
static u32 func_0205761c(const DWCFriendData *loginId) {
    return loginId->word2;
}

// The friend key of friend data that holds one
static u64 func_02057624(const DWCFriendData *friendData) {
    return ((u64)friendData->word2 << 32) | friendData->word1;
}

// The profile ID of friend data that holds one
static u32 func_02057638(const DWCFriendData *friendData) {
    return friendData->word1;
}

// Sets the user ID of a login ID; the header keeps what it had if the ID's high bits don't fit in 11
void func_02057640(DWCFriendData *loginId, u64 userId) {
    func_020575d0(&loginId->header, userId >> 32, 0, USER_ID_HIGH_MASK);
    loginId->word1 = (u32)userId;
}

void func_02057668(DWCFriendData *loginId, u32 playerId) {
    loginId->word2 = playerId;
}

static void func_02057670(DWCFriendData *friendData, u64 friendKey) {
    friendData->word1 = (u32)friendKey;
    friendData->word2 = friendKey >> 32;
}

static void func_02057678(DWCFriendData *friendData, s32 profileId) {
    friendData->word1 = profileId;
}

// The 21 bits of flags, kind included
static u32 func_02057680(const DWCFriendData *friendData) {
    return (friendData->header >> FLAGS_SHIFT) & FLAGS_MASK;
}

// The kind of friend data
static int func_02057694(const DWCFriendData *friendData) {
    return func_02057680(friendData) & TYPE_MASK;
}

BOOL func_020576a4(const DWCFriendData *friendData) {
    if (func_020576d4(friendData) && func_0205770c(friendData)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_020576d4(const DWCFriendData *friendData) {
    if (func_02057694(friendData) == DWC_FRIEND_TYPE_PROFILE_ID) {
        return (func_02057680(friendData) & DWC_FRIEND_FLAG_AUTHENTIC) == DWC_FRIEND_FLAG_AUTHENTIC;
    }
    return FALSE;
}

BOOL func_0205770c(const DWCFriendData *friendData) {
    return (func_02057680(friendData) & DWC_FRIEND_FLAG_BUDDY) == DWC_FRIEND_FLAG_BUDDY;
}

BOOL func_02057728(const DWCFriendData *friendData) {
    return (func_02057680(friendData) & DWC_FRIEND_FLAG_5) == DWC_FRIEND_FLAG_5;
}

int func_02057744(const DWCFriendData *friendData) {
    return func_02057694(friendData);
}

// Sets the flags, kind included
static void func_02057750(DWCFriendData *friendData, u32 flags) {
    func_020575d0(&friendData->header, flags, FLAGS_SHIFT, FLAGS_MASK);
}

// Sets the kind of friend data
static void func_02057768(DWCFriendData *friendData, int type) {
    u32 flags = func_02057680(friendData) & ~TYPE_MASK;

    func_02057750(friendData, flags | type);
}

void func_0205778c(DWCFriendData *friendData) {
    u32 flags;

    if (func_02057694(friendData) != DWC_FRIEND_TYPE_PROFILE_ID) {
        return;
    }
    flags = func_02057680(friendData) | DWC_FRIEND_FLAG_AUTHENTIC;
    func_02057750(friendData, flags);
}

void func_020577b8(DWCFriendData *friendData, BOOL on) {
    u32 flags;

    if (on) {
        flags = func_02057680(friendData) | DWC_FRIEND_FLAG_BUDDY;
    } else {
        flags = func_02057680(friendData) & ~DWC_FRIEND_FLAG_BUDDY;
    }
    func_02057750(friendData, flags);
}

void func_020577e8(DWCFriendData *friendData, BOOL on) {
    u32 flags;

    if (on) {
        flags = func_02057680(friendData) | DWC_FRIEND_FLAG_4;
    } else {
        flags = func_02057680(friendData) & ~DWC_FRIEND_FLAG_4;
    }
    func_02057750(friendData, flags);
}

void func_02057818(DWCFriendData *friendData, BOOL on) {
    u32 flags;

    if (on) {
        flags = func_02057680(friendData) | DWC_FRIEND_FLAG_5;
    } else {
        flags = func_02057680(friendData) & ~DWC_FRIEND_FLAG_5;
    }
    func_02057750(friendData, flags);
}

// The friend key of a profile ID: the ID, and above it a 7-bit checksum of the ID and the game code, which is a CRC-8
// or the top bits of an MD5
static u64 func_02057848(u32 profileId, u32 gameCode, BOOL useMd5) {
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
BOOL func_020578cc(const DWCUserData *userData, u64 friendKey) {
    return func_020578ec(friendKey, userData->gameCode, TRUE);
}

// Whether a friend key is made by the checksum for a game; the profile ID in it must be above 0
static BOOL func_020578ec(u64 friendKey, u32 gameCode, int useMd5) {
    BOOL result = FALSE;
    s32 profileId = (s32)(friendKey & 0xffffffff);

    if (profileId <= 0) {
        return result;
    }
    if (func_02057848(profileId, gameCode, useMd5) == friendKey) {
        result = TRUE;
    }
    return result;
}

// The profile ID in a friend key, if the key is right for the game with either checksum; 0 if not
static u32 func_02057930(u64 friendKey, u32 gameCode) {
    int useMd5;

    for (useMd5 = 0; useMd5 < 2; useMd5++) {
        if (func_020578ec(friendKey, gameCode, useMd5)) {
            return friendKey & 0xffffffff;
        }
    }
    return 0;
}

// Writes the low bits of a value as base 32 text, most significant digit first, and a NUL
void func_02057980(u64 value, int bits, char *dest) {
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
void func_020579ec(const DWCFriendData *loginId, u32 gameCode, char *dest) {
    char userIdText[21];
    char playerIdText[21];

    func_02057980(func_020575fc(loginId), 43, userIdText);
    func_02057980(func_0205761c(loginId), 32, playerIdText);
    func_0207a2c0(dest, 21, "%s%c%c%c%c%s", userIdText, (gameCode >> 24) & 0xff, (gameCode >> 16) & 0xff,
                  (gameCode >> 8) & 0xff, gameCode & 0xff, playerIdText);
}

// Clears the user data and makes it for a game, with a login ID of its own and no profile
static void func_02057a80(DWCUserData *userData, u32 gameCode) {
    MATHCRC32Table table;

    sys_memset(userData, 0, sizeof(DWCUserData));
    userData->size = sizeof(DWCUserData);
    userData->gsProfileId = 0;
    userData->gameCode = gameCode;
    func_02057b00(&userData->pseudo);
    func_02057768(&userData->authentic, DWC_FRIEND_TYPE_NONE);
    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    userData->crc32 = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
    userData->flags |= DWC_USER_DATA_DIRTY;
}

// Makes a login ID of the console's Wi-Fi ID (the pending one until the first is confirmed) and a random player ID,
// from a generator that is seeded with the console's entropy
void func_02057b00(DWCFriendData *loginId) {
    DWCBmWifiId wifiId;
    u32 entropy[8];
    MATHRandContext32 rand;
    u32 i;

    OS_GetLowEntropyData(entropy);
    for (i = 1; i < 8; i++) {
        entropy[i] ^= entropy[i - 1];
    }
    MATH_InitRand32(&rand, entropy[7]);
    func_0205752c(&wifiId);
    if (wifiId.isSet) {
        func_02057640(loginId, wifiId.id);
    } else {
        func_02057640(loginId, wifiId.pendingId);
    }
    func_02057668(loginId, MATH_Rand32(&rand, 0));
    func_02057768(loginId, DWC_FRIEND_TYPE_LOGIN_ID);
}

// Whether a login ID is made of the console's Wi-Fi ID (or the pending one)
BOOL func_02057bbc(const DWCFriendData *loginId) {
    DWCBmWifiId wifiId;

    func_0205752c(&wifiId);
    if (wifiId.isSet) {
        return func_020575fc(loginId) == wifiId.id;
    } else {
        return func_020575fc(loginId) == wifiId.pendingId;
    }
}

// Whether friend data is a login ID
BOOL func_02057c28(const DWCFriendData *friendData) {
    return func_02057694(friendData) == DWC_FRIEND_TYPE_LOGIN_ID;
}

// Whether the user data has an authenticated login ID
BOOL func_02057c40(const DWCUserData *userData) {
    return func_02057c28(&userData->authentic);
}

BOOL DWC_IsValidFriendData(const DWCFriendData *friendData) {
    return func_02057694(friendData) != DWC_FRIEND_TYPE_NONE;
}

BOOL func_02057c68(const DWCFriendData *friendData) {
    return DWC_IsValidFriendData(friendData);
}

// Makes the user data for the game that the library was started with
void func_02057c74(DWCUserData *userData) {
    func_02057a80(userData, func_02058684());
}

BOOL func_02057c90(const DWCUserData *userData) {
    MATHCRC32Table table;
    u32 crc;

    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    crc = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
    return crc == userData->crc32;
}

BOOL DWC_CheckValidConsole(const DWCUserData *userData) {
    DWCBmWifiId wifiId;

    if (func_02057694(&userData->authentic) == DWC_FRIEND_TYPE_NONE) {
        return TRUE;
    }
    func_0205752c(&wifiId);
    if (wifiId.isSet == 0) {
        return FALSE;
    }
    return func_020575fc(&userData->authentic) == wifiId.id;
}

BOOL DWC_CheckHasProfile(const DWCUserData *userData) {
    return func_02057c28(&userData->authentic) && userData->gsProfileId > 0;
}

// Authenticates a login ID for a profile, which marks the user data as changed
void func_02057d68(DWCUserData *userData, const DWCFriendData *loginId, s32 profileId) {
    MATHCRC32Table table;

    userData->authentic = *loginId;
    userData->gsProfileId = profileId;
    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    userData->crc32 = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
    userData->flags |= DWC_USER_DATA_DIRTY;
}

BOOL func_02057dc4(const DWCUserData *userData) {
    return func_02057dd0(userData);
}

static BOOL func_02057dd0(const DWCUserData *userData) {
    return (userData->flags & DWC_USER_DATA_DIRTY) == DWC_USER_DATA_DIRTY;
}

void func_02057de8(DWCUserData *userData) {
    func_02057df4(userData);
}

static void func_02057df4(DWCUserData *userData) {
    MATHCRC32Table table;

    userData->flags &= ~DWC_USER_DATA_DIRTY;
    MATHi_CRC32InitTableRev(&table, CRC32_POLY);
    userData->crc32 = MATH_CalcCRC32(&table, userData, USER_DATA_CHECKED_SIZE);
}

// The friend key of friend data, or 0 if it is of another kind
u64 func_02057e3c(const DWCFriendData *friendData) {
    if (func_02057694(friendData) != DWC_FRIEND_TYPE_FRIEND_KEY) {
        return 0;
    }
    return func_02057624(friendData);
}

s32 DWC_GetGsProfileId(const DWCUserData *userData, const DWCFriendData *friendData) {
    switch (func_02057694(friendData)) {
    case DWC_FRIEND_TYPE_FRIEND_KEY:
        return func_02057930(func_02057624(friendData), userData->gameCode);
    case DWC_FRIEND_TYPE_PROFILE_ID:
        return func_02057638(friendData);
    case DWC_FRIEND_TYPE_LOGIN_ID:
        return -1;
    default:
        return 0;
    }
}

// The player's own friend key, if there is a profile
u64 func_02057ec4(const DWCUserData *userData) {
    u64 friendKey = 0;

    if (userData->gsProfileId != 0) {
        friendKey = func_02057848(userData->gsProfileId, userData->gameCode, TRUE);
    }
    return friendKey;
}

void func_02057ef8(DWCFriendData *token, u64 friendKey) {
    sys_memset(token, 0, sizeof(DWCFriendData));
    func_02057670(token, friendKey);
    func_02057768(token, DWC_FRIEND_TYPE_FRIEND_KEY);
}

void DWC_CreateExchangeToken(const DWCUserData *userData, DWCFriendData *token) {
    sys_memset(token, 0, sizeof(DWCFriendData));
    if (!func_02057c40(userData)) {
        *token = userData->pseudo;
        return;
    }
    func_02057678(token, userData->gsProfileId);
    func_02057768(token, DWC_FRIEND_TYPE_PROFILE_ID);
}

void func_02057f88(DWCFriendData *token, s32 profileId) {
    sys_memset(token, 0, sizeof(DWCFriendData));
    func_02057678(token, profileId);
    func_02057768(token, DWC_FRIEND_TYPE_PROFILE_ID);
}

void func_02057fbc(const DWCUserData *userData, const DWCFriendData *loginId, char *dest) {
    func_020579ec(loginId, userData->gameCode, dest);
}

BOOL func_02057fd4(const DWCFriendData *friendData, const DWCFriendData *other) {
    int type = func_02057694(friendData);

    if (type != func_02057694(other)) {
        return FALSE;
    }
    if (type == DWC_FRIEND_TYPE_PROFILE_ID) {
        return func_02057638(friendData) == func_02057638(other);
    }
    if (type == DWC_FRIEND_TYPE_LOGIN_ID) {
        if (func_020575fc(friendData) == func_020575fc(other) && func_0205761c(friendData) == func_0205761c(other)) {
            return TRUE;
        }
        return FALSE;
    }
    if (type == DWC_FRIEND_TYPE_FRIEND_KEY) {
        return func_02057624(friendData) == func_02057624(other);
    }
    return FALSE;
}
