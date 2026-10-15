#include "dwc/gamestats.h"

#include "gamestats_internal.h"

// The client of the game statistics server, see gamestats.h. A call is a small state machine that the game runs every
// frame with gamestats_Poll: 3 once the key is set, 4 to send the first request, 5 while it is in flight, 6 to send the
// second one, 7 while it is in flight, 8 when the answer is kept, and 1 when anything failed.

// The game's secret, which is copied as a whole
typedef struct GamestatsSecret {
    char text[0x14];
} GamestatsSecret;

// The state of the client
typedef struct GamestatsClient {
    int state;
    BOOL active;
    int request;
    BOOL configured;
    char gameName[0x20];
    GamestatsSecret secret;
    // The scrambling of the payload: a generator of numbers with a multiplier, an increment and a modulus, and the key that
    // the checksum of the payload is xored with
    u32 multiplier;
    u32 increment;
    u32 modulus;
    u32 hashKey;
    // The text of the URL being sent, where its hash and its end are, and the answer to the first request
    char *url;
    char *hashField;
    char *urlEnd;
    char *answer;
    int answerSize;
    // The answer to the second request, once checked, without the hash that ends it
    char *body;
    int bodySize;
    GamestatsCheckFunc check;
    // The hash that the second request carries
    char hash[0x29];
    u8 unk9D[3];
} GamestatsClient;

static u32 GamestatsEncodedSize(u32 size);
static void SeedRandom(u32 seed);
static u8 NextRandom(void);
static BOOL Configure(const char *gameName, const char *secret, u32 multiplier, u32 increment, u32 modulus, u32 hashKey);
static BOOL CheckAnswer(char *data, int size);
static void OnSent(void);
static void OnAnswered(char *data, int size, int status, void *arg);
static int EncodePayload(char *dest, const u8 *header, int headerSize, const u8 *payload, u32 payloadSize);
static void SetFailed(void);
static int Send(const char *page, int pid, const void *payload, int size, GamestatsCheckFunc check, BOOL useHash);

static int sRandomState;
static const char *sBaseUrl;
static GamestatsClient sClient = {2};

// The size of size bytes in base64
static u32 GamestatsEncodedSize(u32 size) {
    return (size / 3 + (size % 3 != 0 ? 1 : 0)) * 4;
}

static void SeedRandom(u32 seed) {
    sRandomState = (seed << 16) | seed;
}

// The next byte of the generator
static u8 NextRandom(void) {
    sRandomState = (sClient.multiplier * sRandomState + sClient.increment) % sClient.modulus;
    return sRandomState >> 16;
}

// Takes the game's name and secret and the generator's numbers, which have to be the ones of a full cycle: the
// multiplier is 5 more than a multiple of 8, the increment odd and the modulus a power of two
static BOOL Configure(const char *gameName, const char *secret, u32 multiplier, u32 increment, u32 modulus, u32 hashKey) {
    int bits = 0;
    int i;

    sClient.configured = FALSE;
    if (strlen(gameName) >= 0x20) {
        return FALSE;
    }
    if (strlen(secret) != 0x14) {
        return FALSE;
    }
    if (((multiplier - 5) & 7) != 0) {
        return FALSE;
    }
    if ((increment & 1) == 0) {
        return FALSE;
    }
    for (i = 0; i < 32; i++) {
        if (((modulus >> i) & 1) == 1) {
            bits++;
        }
    }
    if (bits != 1) {
        return FALSE;
    }
    strncpy(sClient.gameName, gameName, 0x20);
    sClient.secret = *(const GamestatsSecret *)secret;
    sClient.multiplier = multiplier;
    sClient.increment = increment;
    sClient.modulus = modulus;
    sClient.hashKey = hashKey;
    sClient.configured = TRUE;
    return TRUE;
}

// Checks that the answer ends with the hash of the secret, the answer in base64 and the secret, and keeps it
static inline void CopySecret(char *dest) {
    *(GamestatsSecret *)dest = sClient.secret;
}

static BOOL CheckAnswer(char *data, int size) {
    char hash[0x29];
    u8 digest[0x14];
    char digits[17] = "0123456789abcdef";
    char *buffer;
    int bodySize;
    int i;

    if (size <= 0x28) {
        return FALSE;
    }
    bodySize = size - 0x28;
    buffer = func_020586e4(8, GamestatsEncodedSize(bodySize) + 0x29);
    if (buffer == NULL) {
        return FALSE;
    }
    CopySecret(buffer);
    func_ov011_0216e878(data, buffer + 0x14, bodySize, 2);
    CopySecret(buffer + 0x14 + GamestatsEncodedSize(bodySize));
    MATH_CalcSHA1(digest, buffer, GamestatsEncodedSize(bodySize) + 0x28);
    func_02058728(8, buffer, 0);
    for (i = 0; i < 0x14; i++) {
        hash[i * 2] = digits[digest[i] >> 4];
        hash[i * 2 + 1] = digits[digest[i] & 0xf];
    }
    hash[0x28] = '\0';
    size -= 0x28;
    if (strncmp(data + size, hash, 0x28) != 0) {
        return FALSE;
    }
    sClient.body = data;
    sClient.bodySize = bodySize;
    return TRUE;
}

static void OnSent(void) {
}

// The HTTP client's answer to either request. The first one answers with a challenge of 32 characters, from which the hash
// of the second request is made; the second answers with the data, or with a text that starts with "error:"
static void OnAnswered(char *data, int size, int status, void *arg) {
    sClient.request = -1;
    if (sClient.state == 1) {
        return;
    }
    if (status == 0) {
        switch (sClient.state) {
        case 5:
            if (size == 0x20) {
                u8 *digest = (u8 *)sClient.hashField + 0x14;
                u8 block[0x34];
                char digits[17] = "0123456789abcdef";
                int i;

                *(GamestatsSecret *)block = sClient.secret;
                memcpy(block + 0x14, data, 0x20);
                MATH_CalcSHA1(digest, block, 0x34);
                for (i = 0; i < 0x14; i++) {
                    sClient.hashField[i * 2] = digits[digest[i] >> 4];
                    sClient.hashField[i * 2 + 1] = digits[digest[i] & 0xf];
                }
                *(sClient.hashField - strlen("&hash=")) = '&';
                strncpy(sClient.hash, sClient.hashField, 0x29);
                sClient.state = 6;
            } else {
                SetFailed();
            }
            func_02058814(data);
            break;
        case 7:
            if (strncmp(data, "error:", strlen("error:")) == 0) {
                func_02058814(data);
                SetFailed();
                return;
            }
            if (sClient.answer != NULL) {
                func_02058814(sClient.answer);
                sClient.answer = NULL;
            }
            if (!CheckAnswer(data, size)) {
                func_02058814(data);
                SetFailed();
                return;
            }
            if (sClient.check != NULL && !sClient.check(data, size)) {
                func_02058814(data);
                SetFailed();
                return;
            }
            sClient.answer = data;
            sClient.answerSize = size;
            sClient.state = 8;
            break;
        }
    } else {
        SetFailed();
    }
}

// Scrambles the header and the payload into dest, in base64: a checksum xored with the key, and the bytes xored with the
// generator that the checksum seeds
static int EncodePayload(char *dest, const u8 *header, int headerSize, const u8 *payload, u32 payloadSize) {
    int total = payloadSize + headerSize;
    u8 *buffer;
    u32 sum = 0;
    int i;

    buffer = func_020586e4(8, total + 4);
    if (buffer == NULL) {
        return 2;
    }
    for (i = 0; i < headerSize; i++) {
        buffer[i + 4] = header[i];
    }
    for (i = 0; i < (int)payloadSize; i++) {
        buffer[headerSize + i + 4] = payload[i];
    }
    for (i = 0; i < total; i++) {
        sum += buffer[i + 4];
    }
    SeedRandom(sum);
    for (i = 0; i < total; i++) {
        buffer[i + 4] ^= NextRandom();
    }
    sum ^= sClient.hashKey;
    buffer[0] = sum >> 24;
    buffer[1] = sum >> 16;
    buffer[2] = sum >> 8;
    buffer[3] = sum;
    func_ov011_0216e878(buffer, dest, total + 4, 2);
    func_02058728(8, buffer, 0);
    dest[GamestatsEncodedSize(total + 4)] = '\0';
    return 0;
}

void Gamestats_Start(int server, const char *key) {
    char secret[0x15];
    char digits[9] = {0};
    u32 multiplier;
    u32 increment;
    u32 modulus;
    u32 hashKey;

    sClient.request = -1;
    sClient.url = NULL;
    sClient.hashField = NULL;
    sClient.urlEnd = NULL;
    sClient.answer = NULL;
    sClient.answerSize = 0;
    switch (server) {
    case GAMESTATS_SERVER_LIVE:
        sBaseUrl = "http://gamestats2.gs.nintendowifi.net/";
        break;
    case GAMESTATS_SERVER_DEVELOPMENT:
        sBaseUrl = "http://sdkdev.gamespy.com/games/";
        break;
    case GAMESTATS_SERVER_TEST:
        sBaseUrl = "http://ishikawa.servebeer.com/games/";
        break;
    }
    strncpy(secret, key, 0x14);
    secret[0x14] = '\0';
    multiplier = strtoul(strncpy(digits, key + 0x14, 8), NULL, 16);
    increment = strtoul(strncpy(digits, key + 0x1c, 8), NULL, 16);
    modulus = strtoul(strncpy(digits, key + 0x24, 8), NULL, 16);
    hashKey = strtoul(strncpy(digits, key + 0x2c, 8), NULL, 16);
    Configure(key + 0x34, secret, multiplier, increment, modulus, hashKey);
    DWC_InitGHTTP(0);
    sClient.state = 3;
    sClient.active = TRUE;
}

// Fails the call, and tells the Wi-Fi library unless it has an error
static void SetFailed(void) {
    if (!func_020584b0()) {
        func_020584cc(6, -90040);
    }
    sClient.state = 1;
}

int Gamestats_Poll(void) {
    switch (sClient.state) {
    case 0:
    case 2:
    case 3:
    case 8:
        break;
    case 1:
        SetFailed();
        break;
    case 4:
        sClient.request = DWC_GetGHTTPData(sClient.url, 0, 0, OnSent, OnAnswered, NULL);
        if (sClient.request >= 0) {
            sClient.state = 5;
        } else {
            SetFailed();
        }
        break;
    case 5:
        if (!DWC_ProcessGHTTP()) {
            SetFailed();
        }
        break;
    case 6:
        sClient.request = DWC_GetGHTTPData(sClient.url, 0, 0, OnSent, OnAnswered, NULL);
        if (sClient.request >= 0) {
            sClient.state = 7;
        } else {
            SetFailed();
        }
        break;
    case 7:
        if (!DWC_ProcessGHTTP()) {
            SetFailed();
        }
        break;
    }
    return sClient.state;
}

void Gamestats_Abort(void) {
    if (sClient.request >= 0) {
        DWC_CancelGHTTPRequest(sClient.request);
    }
    sClient.state = 0;
}

void Gamestats_Close(void) {
    if (sClient.active) {
        if (sClient.url != NULL) {
            func_02058728(8, sClient.url, 0);
            sClient.url = NULL;
        }
        if (sClient.answer != NULL) {
            func_02058814(sClient.answer);
            sClient.answerSize = 0;
            sClient.answer = NULL;
        }
        DWC_ShutdownGHTTP();
        sClient.state = 2;
        sClient.active = FALSE;
    }
}

void *Gamestats_GetBody(int *size) {
    *size = sClient.bodySize;
    return sClient.body;
}

// Builds the URL of a request: the server, the game, the page and the player's ID, the hash that is still a placeholder and
// the payload. The first request is cut before its hash, and the second keeps the one made from the first's answer
static int Send(const char *page, int pid, const void *payload, int size, GamestatsCheckFunc check, BOOL useHash) {
    char pidText[0x10];
    int header[2];
    int result;
    char *url;

    if (sClient.configured != TRUE) {
        return 3;
    }
    if (sClient.url != NULL) {
        func_02058728(8, sClient.url, 0);
        sClient.url = NULL;
    }
    sClient.url = func_020586e4(8, strlen(sBaseUrl) + strlen(sClient.gameName) + strlen(page) + strlen("?pid=")
                                       + func_0207a2c0(pidText, 0x10, "%d", pid) + strlen("&hash=") + 0x28
                                       + strlen("&data=") + GamestatsEncodedSize(size + 0xc) + 1);
    if (sClient.url == NULL) {
        return 2;
    }
    func_0207a290(sClient.url, "%s%s%s?pid=%d&hash=%s&data=", sBaseUrl, sClient.gameName, page, pid,
                  "0000000000000000000000000000000000000000", "");
    url = sClient.url;
    sClient.urlEnd = url + strlen(url);
    sClient.hashField = sClient.urlEnd - strlen("&data=") - 0x28;
    header[0] = pid;
    header[1] = size;
    result = EncodePayload(sClient.urlEnd, (u8 *)header, 8, payload, size);
    if (result == 2) {
        func_02058728(8, sClient.url, 0);
        sClient.url = NULL;
        return 2;
    }
    if (useHash) {
        memcpy(sClient.hashField, sClient.hash, 0x28);
        sClient.state = 6;
    } else {
        *(sClient.hashField - strlen("&hash=")) = '\0';
        sClient.state = 4;
    }
    sClient.check = check;
    return 0;
}

int Gamestats_Send(const char *page, int pid, const void *payload, int size, GamestatsCheckFunc check) {
    return Send(page, pid, payload, size, check, FALSE);
}
