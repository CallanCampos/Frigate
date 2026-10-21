#include "competition_internal.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "dwc/md5.h"
#include "nitro/std.h"

// GameSpy's competition service, as the Wi-Fi library of overlay 260 talks to it: the client, the calls that start its
// three requests, and the report that the game builds. The report's numbers are big-endian, as the service reads them;
// its first 0x44 bytes are a header. The requests' SOAP messages are in dwc_competition_soap.c. The ROM names neither
// file; both names are ours.

// The last eight bytes of a GUID
typedef struct ScGuidTail {
    gsi_u8 data[8];
} ScGuidTail;

static inline gsi_u16 Sc_SwapU16(gsi_u16 value) {
    return ((value >> 8) & 0x00ff) | ((value << 8) & 0xff00);
}

static inline gsi_u32 Sc_SwapU32(gsi_u32 value) {
    return ((value >> 24) & 0x000000ff) | ((value >> 8) & 0x0000ff00) | ((value << 8) & 0x00ff0000) |
           ((value << 24) & 0xff000000);
}

static int ScClient_Alloc(ScClient **client);
static int ScClient_Init(ScClient *client);
static void ScClient_Reset(ScClient *client);
static char *ScClient_BuildUrl(void);
static int ScReport_Create(const char *csid, int version, int playerNum, int teamNum, ScReport **report);
static int ScReport_Destroy(ScReport *report);
static int ScReport_SetPlayerId(ScReport *report, int index, const char *id);
static int ScReport_SetPlayerKey(ScReport *report, int index, int key);
static int ScReport_SetPlayerResult(ScReport *report, int index, int result);
static int ScReport_SetPlayerUnused(ScReport *report, int index, const AuthCertificate *certificate, void *unused);
static int ScReport_StartGroup(ScReport *report);
static int ScReport_StartGame(ScReport *report);
static int ScReport_StartPlayers(ScReport *report);
static int ScReport_StartTeams(ScReport *report);
static int ScReport_Nothing(ScReport *report);
static int ScReport_End(ScReport *report, int flag, int value);
static int ScReport_PutInt(ScReport *report, int key, int value);
static int ScReport_PutByte(ScReport *report, int key, int value);
static gsi_u8 *Sc_PutByte(gsi_u8 *dest, int value);
static gsi_u8 *Sc_PutU16(gsi_u8 *dest, gsi_i16 value);
static gsi_u8 *Sc_PutU32(gsi_u8 *dest, gsi_u32 value);
static gsi_u8 *Sc_PutGuid(gsi_u8 *dest, const char *text);

// The URL of the competition service, set by ScClient_SetServers or once it is asked for
char gCompetitionUrl[128];

void ScClient_SetServers(int server) {
    switch (server) {
    case 1:
        OS_SNPrintf(gCompetitionUrl, 128,
                    "https://%s.secure2.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(gCompetitionUrl);
        OS_SNPrintf(gAuthUrl, 128, "https://%s.secure2.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(gAuthUrl);
        break;
    case 0:
    default:
        OS_SNPrintf(gCompetitionUrl, 128,
                    "https://%s.secure.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(gCompetitionUrl);
        OS_SNPrintf(gAuthUrl, 128, "https://%s.secure.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(gAuthUrl);
        break;
    }
}

static int ScClient_Alloc(ScClient **client) {
    if (__GSIACResult == 1) {
        if (gCompetitionUrl[0] == '\0') {
            OS_SNPrintf(gCompetitionUrl, 128,
                        "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                        __GSIACGamename);
        }
    } else {
        return 1;
    }
    *client = gsimalloc(sizeof(ScClient));
    if (*client == NULL) {
        return 5;
    }
    memset(*client, 0, sizeof(ScClient));
    return 0;
}

static int ScClient_Init(ScClient *client) {
    int error = ScClient_Begin(client, client);

    if (error != 0) {
        return error;
    }
    memset(client->csid, 0, sizeof(client->csid));
    memset(client->ccid, 0, sizeof(client->ccid));
    client->valid = 1;
    return 0;
}

static void ScClient_Reset(ScClient *client) {
    ScClient_End(client);
    memset(client->csid, 0, sizeof(client->csid));
    memset(client->ccid, 0, sizeof(client->ccid));
    // Invalid IDs carry the binary marker 0xDEAD.
    ((gsi_u8 *)client->csid)[0] = 0xde;
    ((gsi_u8 *)client->csid)[1] = 0xad;
    ((gsi_u8 *)client->ccid)[0] = 0xde;
    ((gsi_u8 *)client->ccid)[1] = 0xad;
    client->valid = 0;
}

void ScClient_CopyCsid(ScClient *client, const char *csid) {
    if (csid == NULL) {
        client->csid[0] = '\0';
    } else {
        strcpy(client->csid, csid);
    }
}

void ScClient_CopyCcid(ScClient *client, const char *ccid) {
    if (ccid == NULL) {
        client->ccid[0] = '\0';
    } else {
        strcpy(client->ccid, ccid);
    }
}

static char *ScClient_BuildUrl(void) {
    if (__GSIACResult == 1) {
        if (gCompetitionUrl[0] == '\0') {
            OS_SNPrintf(gCompetitionUrl, 128,
                        "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                        __GSIACGamename);
        }
        return gCompetitionUrl;
    }
    gsDebugFormat(0, 3, 1, "No availability\r\n");
    return NULL;
}

int ScClient_Create(int gameId, ScClient **client) {
    ScClient *created = NULL;
    int error;

    if (client == NULL) {
        return 2;
    }
    *client = NULL;
    error = ScClient_Alloc(&created);
    if (error != 0) {
        return error;
    }
    error = ScClient_Init(created);
    if (error != 0) {
        return error;
    }
    created->gameId = gameId;
    *client = created;
    return 0;
}

int ScClient_Destroy(ScClient *client) {
    if (client == NULL) {
        return 2;
    }
    if (client->valid != 0) {
        ScClient_Reset(client);
        gsifree(client);
    }
    return 0;
}

int ScClient_Update(ScClient *client) {
    if (client == NULL) {
        return 2;
    }
    if (client->valid == 0) {
        return 3;
    }
    ScClient_RunLibrary(client);
    return 0;
}

char *ScClient_GetCsid(ScClient *client) {
    return client->csid;
}

char *ScClient_GetCcid(ScClient *client) {
    return client->ccid;
}

int ScClient_SetCsid(ScClient *client, const char *csid) {
    ScClient_CopyCsid(client, csid);
    return 0;
}

int ScClient_CreateSession(ScClient *client, AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func,
                           int timeout, void *userData) {
    if (Auth_CheckCertificate(certificate)) {
        return ScClient_PostCreateSession(client, client->gameId, certificate, keys, func, timeout, userData);
    }
    return 2;
}

int ScClient_SetReportIntention(ScClient *client, const char *ccid, int authoritative, AuthCertificate *certificate,
                                AuthKeys *keys, ScClientFunc func, int timeout, void *userData) {
    if (client == NULL) {
        return 2;
    }
    if (certificate == NULL) {
        return 2;
    }
    if (keys == NULL) {
        return 2;
    }
    if (client->valid == 0) {
        return 3;
    }
    ScClient_CopyCcid(client, ccid);
    return ScClient_PostReportIntention(client, client->gameId, client->csid, client->ccid, authoritative, certificate,
                                        keys, func, timeout, userData);
}

int ScClient_SubmitReport(ScClient *client, ScReport *report, int authoritative, AuthCertificate *certificate,
                          AuthKeys *keys, ScClientFunc func, int timeout, void *userData) {
    HashContext context;
    gsi_u8 *digest;
    gsi_u8 *clear;
    gsi_u32 i;

    if (client == NULL) {
        return 2;
    }
    if (client->valid == 0) {
        return 3;
    }
    digest = report->buffer;
    clear = digest + 8;
    for (i = 0x10; i != 0; i--) {
        *clear++ = 0;
    }
    func_ov011_021719fc(&context);
    func_ov011_02171a24(&context, report->buffer, report->used);
    digest += 8;
    func_ov011_02171a9c(digest, &context);
    return ScClient_PostReport(client, client->gameId, client->csid, client->ccid, report, authoritative, certificate,
                               keys, func, timeout, userData);
}

int ScClient_CreateReport(ScClient *client, int version, int playerNum, int teamNum, ScReport **report) {
    ScReport *created = NULL;
    int error;

    if (client == NULL) {
        return 2;
    }
    if (report == NULL) {
        return 2;
    }
    error = ScReport_Create(client->csid, version, playerNum, teamNum, &created);
    if (error == 0) {
        *report = created;
    }
    return error;
}

int ScReport_BeginGame(ScReport *report) {
    return ScReport_StartGame(report);
}

int ScReport_BeginPlayers(ScReport *report) {
    return ScReport_StartPlayers(report);
}

int ScReport_BeginTeams(ScReport *report) {
    return ScReport_StartTeams(report);
}

int ScReport_BeginGroup(ScReport *report) {
    return ScReport_StartGroup(report);
}

int ScReport_Finish(ScReport *report, int flag, int value) {
    return ScReport_End(report, flag, value);
}

int ScReport_SetPlayer(ScReport *report, int index, const char *id, int key, int result, int profileId,
                       const AuthCertificate *certificate, void *unused) {
    SCResult error = SCResult_NO_ERROR;
    int add = gsi_true;
    gsi_u32 i;

    for (i = 0; i < report->keyNum; i++) {
        if (key == report->keys[i]) {
            add = gsi_false;
            break;
        }
    }
    if (add) {
        report->keyNum++;
        if (report->keyNum > 0x40) {
            return 5;
        }
        report->keys[report->keyNum - 1] = key;
    }
    if (error == 0) {
        error = ScReport_SetPlayerId(report, index, id);
    }
    if (error == 0) {
        error = ScReport_SetPlayerKey(report, index, key);
    }
    if (error == 0) {
        error = ScReport_SetPlayerResult(report, index, result);
    }
    if (error == 0) {
        error = ScReport_SetPlayerUnused(report, index, certificate, unused);
    }
    return error;
}

int ScReport_AddInt(ScReport *report, int key, int value) {
    return ScReport_PutInt(report, key, value);
}

int ScReport_AddByte(ScReport *report, int key, int value) {
    return ScReport_PutByte(report, key, value);
}

int ScReport_Free(ScReport *report) {
    return ScReport_Destroy(report);
}

char *ScClient_GetUrl(void) {
    return ScClient_BuildUrl();
}

static int ScReport_Create(const char *csid, int version, int playerNum, int teamNum, ScReport **report) {
    ScReport *created;
    ScReportHeader *header;
    gsi_u32 playerSize;
    gsi_u32 teamSize;

    teamSize = playerNum * 0x10;
    playerSize = teamSize + playerNum * 4;

    created = gsimalloc(sizeof(ScReport));
    if (created == NULL) {
        return 5;
    }
    memset(created, 0, sizeof(ScReport));
    header = gsimalloc(0x10000);
    if (header == NULL) {
        gsifree(created);
        return 5;
    }
    memset(header, 0, 0x10000);
    created->unk_114 = 0;
    created->bufferSize = 0x10000;
    memset(header, 0, sizeof(ScReportHeader));
    header->version = 0x2000000;
    header->id = Sc_SwapU32(version);
    header->playerSize = playerSize;
    header->teamSize = teamSize;
    header->resultSize = (playerNum + teamNum) * 4;
    header->playerNum = playerNum;
    header->teamNum = teamNum;
    created->phase = SC_REPORT_PHASE_NEW;
    created->buffer = (gsi_u8 *)header;
    created->groupOffset = -1;
    created->keyNum = 0;
    *report = created;
    return 0;
}

static int ScReport_Destroy(ScReport *report) {
    report->phase = 0;
    report->groupCount = 0;
    report->groupOffset = 0;
    report->keyNum = 0;
    report->unk_0c = 0;
    gsifree(report->buffer);
    report->buffer = NULL;
    report->bufferSize = 0;
    report->unk_118 = 0;
    report->used = 0;
    gsifree(report);
    return 0;
}

// Each player's record after the header is 0x14 bytes: the 0x10-byte GUID of its connection, then its key
static int ScReport_SetPlayerId(ScReport *report, int index, const char *id) {
    gsi_u32 offset = sizeof(ScReportHeader);

    offset += index * 0x14;
    Sc_PutGuid(report->buffer + offset, id);
    return 0;
}

static int ScReport_SetPlayerKey(ScReport *report, int index, int key) {
    gsi_u32 offset = sizeof(ScReportHeader);

    offset += index * 0x14;
    offset += 0x10;
    Sc_PutU32(report->buffer + offset, key);
    return 0;
}

static int ScReport_SetPlayerResult(ScReport *report, int index, int result) {
    ScReportHeader *header = (ScReportHeader *)report->buffer;

    Sc_PutU32(report->buffer + (header->playerSize + sizeof(ScReportHeader) + header->teamSize + index * 4), result);
    return 0;
}

static int ScReport_SetPlayerUnused(ScReport *report, int index, const AuthCertificate *certificate, void *unused) {
    return 0;
}

static int ScReport_StartGroup(ScReport *report) {
    ScReportHeader *header = (ScReportHeader *)report->buffer;

    report->groupCount = 0;
    report->groupOffset = report->used;
    report->used += 2;
    Sc_PutU16(report->buffer + report->groupOffset, report->groupCount);
    header->playerRecordSize += 2;
    return 0;
}

static int ScReport_StartGame(ScReport *report) {
    ScReportHeader *header = (ScReportHeader *)report->buffer;

    report->phase = SC_REPORT_PHASE_GAME;
    report->used = header->playerSize + sizeof(ScReportHeader) + header->teamSize + header->resultSize;
    return 0;
}

static int ScReport_StartPlayers(ScReport *report) {
    report->phase = SC_REPORT_PHASE_PLAYER;
    return 0;
}

static int ScReport_StartTeams(ScReport *report) {
    report->phase = SC_REPORT_PHASE_TEAM;
    return 0;
}

static int ScReport_Nothing(ScReport *report) {
    return 0;
}

static int ScReport_End(ScReport *report, int flag, int value) {
    ScReportHeader *header = (ScReportHeader *)report->buffer;

    ScReport_Nothing(report);
    if (flag != 0) {
        header->flags |= 1;
    }
    header->value = Sc_SwapU32(value);
    header->flags = Sc_SwapU32(header->flags);
    header->playerNum = Sc_SwapU16(header->playerNum);
    header->teamNum = Sc_SwapU16(header->teamNum);
    header->unk_2a = 0;
    header->gameRecordNum = Sc_SwapU16(header->gameRecordNum);
    header->playerRecordNum = Sc_SwapU16(header->playerRecordNum);
    header->teamRecordNum = Sc_SwapU16(header->teamRecordNum);
    header->playerSize = Sc_SwapU32(header->playerSize);
    header->teamSize = Sc_SwapU32(header->teamSize);
    header->resultSize = Sc_SwapU32(header->resultSize);
    header->gameSize = Sc_SwapU32(header->gameSize);
    header->playerRecordSize = Sc_SwapU32(header->playerRecordSize);
    header->teamRecordSize = Sc_SwapU32(header->teamRecordSize);
    return 0;
}

static int ScReport_PutInt(ScReport *report, int key, int value) {
    ScReportHeader *header = (ScReportHeader *)report->buffer;

    if (report->phase == SC_REPORT_PHASE_GAME) {
        header->gameRecordNum++;
        header->gameSize += 8;
    } else if (report->phase == SC_REPORT_PHASE_PLAYER) {
        header->playerRecordNum++;
        header->playerRecordSize += 8;
        report->groupCount++;
        Sc_PutU16(report->buffer + report->groupOffset, report->groupCount);
    } else if (report->phase == SC_REPORT_PHASE_TEAM) {
        header->teamRecordNum++;
        header->teamRecordSize += 8;
        report->groupCount++;
        Sc_PutU16(report->buffer + report->groupOffset, report->groupCount);
    }
    Sc_PutU16(report->buffer + report->used, key);
    report->used += 2;
    Sc_PutU16(report->buffer + report->used, 0);
    report->used += 2;
    Sc_PutU32(report->buffer + report->used, value);
    report->used += 4;
    return 0;
}

static int ScReport_PutByte(ScReport *report, int key, int value) {
    ScReportHeader *header = (ScReportHeader *)report->buffer;

    if (report->phase == SC_REPORT_PHASE_GAME) {
        header->gameRecordNum++;
        header->gameSize += 5;
    } else if (report->phase == SC_REPORT_PHASE_PLAYER) {
        header->playerRecordNum++;
        header->playerRecordSize += 5;
        report->groupCount++;
        Sc_PutU16(report->buffer + report->groupOffset, report->groupCount);
    } else if (report->phase == SC_REPORT_PHASE_TEAM) {
        header->teamRecordNum++;
        header->teamRecordSize += 5;
        report->groupCount++;
        Sc_PutU16(report->buffer + report->groupOffset, report->groupCount);
    }
    Sc_PutU16(report->buffer + report->used, key);
    report->used += 2;
    Sc_PutU16(report->buffer + report->used, 2);
    report->used += 2;
    Sc_PutByte(report->buffer + report->used, value);
    report->used += 1;
    return 0;
}

static gsi_u8 *Sc_PutByte(gsi_u8 *dest, int value) {
    *dest = value;
    return dest + 1;
}

// The report cursor can be unaligned; copy the swapped scalar as bytes.
static gsi_u8 *Sc_PutU16(gsi_u8 *dest, gsi_i16 value) {
    gsi_u16 swapped = Sc_SwapU16(value);
    const gsi_u8 *bytes = (const gsi_u8 *)&swapped;

    dest[0] = bytes[0];
    dest[1] = bytes[1];
    return dest + sizeof(swapped);
}

static gsi_u8 *Sc_PutU32(gsi_u8 *dest, gsi_u32 value) {
    gsi_u32 swapped = Sc_SwapU32(value);
    const gsi_u8 *bytes = (const gsi_u8 *)&swapped;

    dest[0] = bytes[0];
    dest[1] = bytes[1];
    dest[2] = bytes[2];
    dest[3] = bytes[3];
    return dest + sizeof(swapped);
}

static gsi_u8 *Sc_PutGuid(gsi_u8 *dest, const char *text) {
    gsi_u32 data1 = 0;
    int data2 = 0;
    int data3 = 0;
    int data4[8];
    ScGuidTail tail;
    int i;

    STD_TSScanf(text, "%8x", &data1);
    STD_TSScanf(text + 9, "%4x", &data2);
    STD_TSScanf(text + 14, "%4x", &data3);
    STD_TSScanf(text + 19, "%2x", &data4[0]);
    STD_TSScanf(text + 21, "%2x", &data4[1]);
    STD_TSScanf(text + 24, "%2x%2x%2x%2x%2x%2x", &data4[2], &data4[3], &data4[4], &data4[5], &data4[6], &data4[7]);
    for (i = 0; i < 8; i++) {
        tail.data[i] = data4[i];
    }
    dest = Sc_PutU32(dest, data1);
    dest = Sc_PutU16(dest, data2);
    dest = Sc_PutU16(dest, data3);
    *(ScGuidTail *)dest = tail;
    return dest + sizeof(tail);
}
