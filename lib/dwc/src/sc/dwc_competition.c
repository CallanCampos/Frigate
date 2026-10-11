#include "dwc/competition.h"
#include "dwc/ext.h"
#include "dwc/soap.h"

// GameSpy's competition service, as the Wi-Fi library of overlay 260 talks to it: a client with the three requests, and
// the report that the game builds. The report's numbers are big-endian, as the service reads them; its first 0x44 bytes
// are a header.

// The header of the report's buffer. The fields up to the sizes are set when it is made and finished; the digest is
// made when the report is submitted
typedef struct ScReportHeader {
    u32 version;
    u32 id;
    u8 digest[0x10];
    u32 value;
    u32 flags;
    u16 playerNum;
    u16 teamNum;
    u16 gameRecordNum;
    u16 playerRecordNum;
    u16 teamRecordNum;
    u16 unk_2a;
    u32 playerSize;
    u32 teamSize;
    u32 resultSize;
    u32 gameSize;
    u32 playerRecordSize;
    u32 teamRecordSize;
} ScReportHeader;

// A number as it is put in the buffer, whose places aren't aligned
typedef struct ScWire16 {
    u8 data[2];
} ScWire16;

typedef struct ScWire32 {
    u8 data[4];
} ScWire32;

// The last eight bytes of a GUID
typedef struct ScGuidTail {
    u8 data[8];
} ScGuidTail;

static inline u16 Sc_SwapU16(u16 value) {
    return ((value >> 8) & 0x00ff) | ((value << 8) & 0xff00);
}

static inline u32 Sc_SwapU32(u32 value) {
    return ((value >> 24) & 0x000000ff) | ((value >> 8) & 0x0000ff00) | ((value << 8) & 0x00ff0000)
           | ((value << 24) & 0xff000000);
}

static int ScClient_Alloc(ScClient **client);
static int ScClient_Init(ScClient *client);
static void ScClient_Reset(ScClient *client);
static void ScClient_CopyCsid(ScClient *client, const char *csid);
static void ScClient_CopyCcid(ScClient *client, const char *ccid);
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
static u8 *Sc_PutByte(u8 *dest, int value);
static u8 *Sc_PutU16(u8 *dest, s16 value);
static u8 *Sc_PutU32(u8 *dest, u32 value);
static u8 *Sc_PutGuid(u8 *dest, const char *text);
static int ScClient_Begin(ScClient *client, ScClient *self);
static void ScClient_End(ScClient *client);
static void ScClient_RunLibrary(ScClient *client);
static int ScClient_PostCreateSession(ScClient *client, int gameId, AuthCertificate *certificate, AuthKeys *keys,
                                      ScClientFunc func, int timeout, void *userData);
static void ScClient_OnCreateSession(int status, int unused, SoapReader *reader, void *userData);
static int ScClient_PostReportIntention(ScClient *client, int gameId, const char *csid, const char *ccid,
                                        int authoritative, AuthCertificate *certificate, AuthKeys *keys,
                                        ScClientFunc func, int timeout, void *userData);
static void ScClient_OnReportIntention(int status, int unused, SoapReader *reader, void *userData);
static void ScClient_PutReportForm(void *form, void *userData);
static int ScClient_PostReport(ScClient *client, int gameId, const char *csid, const char *ccid, ScReport *report,
                               int authoritative, AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func,
                               int timeout, void *userData);
static void ScClient_OnSubmitReport(int status, int unused, SoapReader *reader, void *userData);

// The URL of the competition service, set by ScClient_SetServers or once it is asked for
static char sCompetitionUrl[128];

void ScClient_SetServers(int server) {
    switch (server) {
    case 1:
        func_0207a2c0(sCompetitionUrl, 128,
                      "https://%s.secure2.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                      data_ov011_021917bc);
        DWCi_SetGHTTPRootCA(sCompetitionUrl);
        func_0207a2c0(gAuthUrl, 128, "https://%s.secure2.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                      data_ov011_021917bc);
        DWCi_SetGHTTPRootCA(gAuthUrl);
        break;
    case 0:
    default:
        func_0207a2c0(sCompetitionUrl, 128,
                      "https://%s.secure.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                      data_ov011_021917bc);
        DWCi_SetGHTTPRootCA(sCompetitionUrl);
        func_0207a2c0(gAuthUrl, 128, "https://%s.secure.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                      data_ov011_021917bc);
        DWCi_SetGHTTPRootCA(gAuthUrl);
        break;
    }
}

static int ScClient_Alloc(ScClient **client) {
    if (data_ov011_02190698 == 1) {
        if (sCompetitionUrl[0] == '\0') {
            func_0207a2c0(sCompetitionUrl, 128,
                          "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                          data_ov011_0219069c);
        }
    } else {
        return 1;
    }
    *client = func_ov011_0216dddc(sizeof(ScClient));
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
    client->csid[0] = 0xde;
    client->csid[1] = 0xad;
    client->ccid[0] = 0xde;
    client->ccid[1] = 0xad;
    client->valid = 0;
}

static void ScClient_CopyCsid(ScClient *client, const char *csid) {
    if (csid == NULL) {
        client->csid[0] = '\0';
    } else {
        strcpy(client->csid, csid);
    }
}

static void ScClient_CopyCcid(ScClient *client, const char *ccid) {
    if (ccid == NULL) {
        client->ccid[0] = '\0';
    } else {
        strcpy(client->ccid, ccid);
    }
}

static char *ScClient_BuildUrl(void) {
    if (data_ov011_02190698 == 1) {
        if (sCompetitionUrl[0] == '\0') {
            func_0207a2c0(sCompetitionUrl, 128,
                          "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                          data_ov011_0219069c);
        }
        return sCompetitionUrl;
    }
    func_ov011_0216cca8(0, 3, 1, "No availability\r\n");
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
        func_ov011_0216ddfc(client);
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
    u8 *digest;
    u8 *clear;
    int i;

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
    int error = 0;
    int add = TRUE;
    u32 i;

    for (i = 0; i < report->keyNum; i++) {
        if (key == report->keys[i]) {
            add = FALSE;
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
    u32 playerSize;
    u32 teamSize;

    teamSize = playerNum * 0x10;
    playerSize = teamSize + playerNum * 4;

    created = func_ov011_0216dddc(sizeof(ScReport));
    if (created == NULL) {
        return 5;
    }
    memset(created, 0, sizeof(ScReport));
    header = func_ov011_0216dddc(0x10000);
    if (header == NULL) {
        func_ov011_0216ddfc(created);
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
    created->buffer = (u8 *)header;
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
    func_ov011_0216ddfc(report->buffer);
    report->buffer = NULL;
    report->bufferSize = 0;
    report->unk_118 = 0;
    report->used = 0;
    func_ov011_0216ddfc(report);
    return 0;
}

static int ScReport_SetPlayerId(ScReport *report, int index, const char *id) {
    Sc_PutGuid(report->buffer + (sizeof(ScReportHeader) + index * 0x14), id);
    return 0;
}

static int ScReport_SetPlayerKey(ScReport *report, int index, int key) {
    Sc_PutU32(report->buffer + (sizeof(ScReportHeader) + index * 0x14 + 0x10), key);
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

static u8 *Sc_PutByte(u8 *dest, int value) {
    *dest = value;
    return dest + 1;
}

static u8 *Sc_PutU16(u8 *dest, s16 value) {
    u16 swapped = Sc_SwapU16(value);

    *(ScWire16 *)dest = *(ScWire16 *)&swapped;
    return dest + sizeof(ScWire16);
}

static u8 *Sc_PutU32(u8 *dest, u32 value) {
    u32 swapped = Sc_SwapU32(value);

    *(ScWire32 *)dest = *(ScWire32 *)&swapped;
    return dest + sizeof(ScWire32);
}

static u8 *Sc_PutGuid(u8 *dest, const char *text) {
    u32 data1 = 0;
    int data2 = 0;
    int data3 = 0;
    int data4[8];
    ScGuidTail tail;
    int i;

    func_0207fa4c(text, "%8x", &data1);
    func_0207fa4c(text + 9, "%4x", &data2);
    func_0207fa4c(text + 14, "%4x", &data3);
    func_0207fa4c(text + 19, "%2x", &data4[0]);
    func_0207fa4c(text + 21, "%2x", &data4[1]);
    func_0207fa4c(text + 24, "%2x%2x%2x%2x%2x%2x", &data4[2], &data4[3], &data4[4], &data4[5], &data4[6], &data4[7]);
    for (i = 0; i < 8; i++) {
        tail.data[i] = data4[i];
    }
    dest = Sc_PutU32(dest, data1);
    dest = Sc_PutU16(dest, data2);
    dest = Sc_PutU16(dest, data3);
    *(ScGuidTail *)dest = tail;
    return dest + sizeof(tail);
}

// The declaration of the messages' namespace
static const char *sCompetitionNamespaces[] = {"gsc=\"http://gamespy.net/competition/\""};

static int ScClient_Begin(ScClient *client, ScClient *self) {
    if (func_ov011_0216c9f4() != 0) {
        return 4;
    }
    client->self = self;
    client->createSessionFunc = NULL;
    client->reportIntentionFunc = NULL;
    client->submitReportFunc = NULL;
    client->createSessionData = NULL;
    client->reportIntentionData = NULL;
    client->submitReportData = NULL;
    client->createSessionBusy = 0;
    client->reportIntentionBusy = 0;
    client->submitReportBusy = 0;
    client->ready = 1;
    return 0;
}

static void ScClient_End(ScClient *client) {
    client->ready = 0;
    client->createSessionFunc = NULL;
    client->reportIntentionFunc = NULL;
    client->submitReportFunc = NULL;
    client->createSessionData = NULL;
    client->reportIntentionData = NULL;
    client->submitReportData = NULL;
    client->createSessionBusy = 0;
    client->reportIntentionBusy = 0;
    client->submitReportBusy = 0;
    client->self = NULL;
}

static void ScClient_RunLibrary(ScClient *client) {
    func_ov011_0216c910(0);
}

static int ScClient_PostCreateSession(ScClient *client, int gameId, AuthCertificate *certificate, AuthKeys *keys,
                                      ScClientFunc func, int timeout, void *userData) {
    SoapWriter *writer;

    if (client->createSessionBusy != 0) {
        return 6;
    }
    writer = func_ov011_0216fbf8(sCompetitionNamespaces, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!func_ov011_02170614(writer, "gsc", "CreateSession") || !func_ov011_02170614(writer, "gsc", "certificate")
        || !Auth_WriteCertificate(certificate, "gsc", writer) || !func_ov011_02170660(writer, "gsc", "certificate")
        || !func_ov011_0217090c(writer, "gsc", "proof", keys->proof, sizeof(keys->proof))
        || !func_ov011_02170818(writer, "gsc", "gameid", gameId) || !func_ov011_02170660(writer, "gsc", "CreateSession")
        || !func_ov011_0216fea8(writer)) {
        func_ov011_0216fe64(writer);
        return 7;
    }
    client->createSessionFunc = func;
    client->createSessionData = userData;
    client->createSessionBusy = 1;
    func_ov011_0216ea4c(sCompetitionUrl, "SOAPAction: \"http://gamespy.net/competition/CreateSession\"", writer,
                        ScClient_OnCreateSession, client);
    return 0;
}

static void ScClient_OnCreateSession(int status, int unused, SoapReader *reader, void *userData) {
    ScClient *client = userData;
    int result;
    char ccid[0xff];
    char csid[0xff];
    ScClientFunc func;
    int error = 7;

    if (client->ready) {
        if (status == 0) {
            result = 0;
            if (func_ov011_02170d18(reader)) {
                if (!func_ov011_02170d28(reader, "CreateSessionResponse")
                    && !func_ov011_02170d28(reader, "CreateMatchlessSessionResponse")) {
                    error = 9;
                }
            } else {
                error = 9;
            }
            if (!func_ov011_02170d28(reader, "CreateSessionResult")
                && !func_ov011_02170d28(reader, "CreateMatchlessSessionResult")) {
                error = 9;
            }
            if (func_ov011_02171090(reader, "result", &result)) {
                if (error != 9) {
                    if (result == 0) {
                        if (func_ov011_02170e90(reader, "csid", csid, sizeof(csid))
                            && func_ov011_02170e90(reader, "ccid", ccid, sizeof(ccid))) {
                            ScClient_CopyCsid(client->self, csid);
                            ScClient_CopyCcid(client->self, ccid);
                            error = 0;
                        } else {
                            error = 9;
                        }
                    } else {
                        error = 9;
                    }
                }
            } else {
                error = 9;
            }
        }
        func = client->createSessionFunc;
        client->createSessionBusy = 0;
        if (func != NULL) {
            func(client->self, status, error, client->createSessionData);
            client->createSessionData = NULL;
            client->createSessionFunc = NULL;
        }
    }
}

static int ScClient_PostReportIntention(ScClient *client, int gameId, const char *csid, const char *ccid,
                                        int authoritative, AuthCertificate *certificate, AuthKeys *keys,
                                        ScClientFunc func, int timeout, void *userData) {
    SoapWriter *writer;

    if (client->reportIntentionBusy != 0) {
        return 6;
    }
    writer = func_ov011_0216fbf8(sCompetitionNamespaces, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!func_ov011_02170614(writer, "gsc", "SetReportIntention") || !func_ov011_02170614(writer, "gsc", "certificate")
        || !Auth_WriteCertificate(certificate, "gsc", writer) || !func_ov011_02170660(writer, "gsc", "certificate")
        || !func_ov011_0217090c(writer, "gsc", "proof", keys->proof, sizeof(keys->proof))
        || !func_ov011_021706b8(writer, "gsc", "csid", csid) || !func_ov011_021706b8(writer, "gsc", "ccid", ccid)
        || !func_ov011_02170818(writer, "gsc", "gameid", gameId)
        || !func_ov011_02170818(writer, "gsc", "authoritative", authoritative ? 1 : 0)
        || !func_ov011_02170660(writer, "gsc", "SetReportIntention") || !func_ov011_0216fea8(writer)) {
        func_ov011_0216fe64(writer);
        return 7;
    }
    client->reportIntentionFunc = func;
    client->reportIntentionData = userData;
    client->reportIntentionBusy = 1;
    func_ov011_0216ea4c(sCompetitionUrl, "SOAPAction: \"http://gamespy.net/competition/SetReportIntention\"", writer,
                        ScClient_OnReportIntention, client);
    return 0;
}

static void ScClient_OnReportIntention(int status, int unused, SoapReader *reader, void *userData) {
    ScClient *client = userData;
    int result;
    char ccid[0xff];
    ScClientFunc func;
    int error;

    if (client->ready) {
        if (status == 0) {
            error = 0;
            result = 0;
            if (!func_ov011_02170d18(reader) || !func_ov011_02170d28(reader, "SetReportIntentionResponse")
                || !func_ov011_02170d28(reader, "SetReportIntentionResult")
                || !func_ov011_02171090(reader, "result", &result)
                || !func_ov011_02170e90(reader, "ccid", ccid, sizeof(ccid))) {
                error = 9;
            } else if (result == 0) {
                ScClient_CopyCcid(client->self, ccid);
            } else {
                error = 8;
            }
        } else {
            error = 7;
        }
        func = client->reportIntentionFunc;
        client->reportIntentionBusy = 0;
        if (func != NULL) {
            func(client->self, status, error, client->reportIntentionData);
            client->reportIntentionData = NULL;
            client->reportIntentionFunc = NULL;
        }
    }
}

static void ScClient_PutReportForm(void *form, void *userData) {
    ScClient *client = userData;

    ghttpPostAddFileFromMemory(form, "report", client->report, client->reportSize, "report", "application/bin");
}

static int ScClient_PostReport(ScClient *client, int gameId, const char *csid, const char *ccid, ScReport *report,
                               int authoritative, AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func,
                               int timeout, void *userData) {
    SoapWriter *writer;

    if (client->submitReportBusy != 0) {
        return 6;
    }
    if (report->used < sizeof(ScReportHeader)) {
        return 12;
    }
    writer = func_ov011_0216fbf8(sCompetitionNamespaces, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!func_ov011_02170614(writer, "gsc", "SubmitReport") || !func_ov011_02170614(writer, "gsc", "certificate")
        || !Auth_WriteCertificate(certificate, "gsc", writer) || !func_ov011_02170660(writer, "gsc", "certificate")
        || !func_ov011_0217090c(writer, "gsc", "proof", keys->proof, sizeof(keys->proof))
        || !func_ov011_021706b8(writer, "gsc", "csid", csid) || !func_ov011_021706b8(writer, "gsc", "ccid", ccid)
        || !func_ov011_02170818(writer, "gsc", "gameid", gameId)
        || !func_ov011_02170818(writer, "gsc", "authoritative", authoritative ? 1 : 0)
        || !func_ov011_02170660(writer, "gsc", "SubmitReport") || !func_ov011_0216fea8(writer)) {
        func_ov011_0216fe64(writer);
        return 5;
    }
    client->report = report->buffer;
    client->reportSize = report->used;
    client->submitReportFunc = func;
    client->submitReportData = userData;
    client->submitReportBusy = 1;
    func_ov011_0216eae0(sCompetitionUrl, "SOAPAction: \"http://gamespy.net/competition/SubmitReport\"", writer,
                        ScClient_OnSubmitReport, ScClient_PutReportForm, client);
    return 0;
}

static void ScClient_OnSubmitReport(int status, int unused, SoapReader *reader, void *userData) {
    ScClient *client = userData;
    int result;
    ScClientFunc func;
    int error;

    if (client->ready) {
        if (status == 0) {
            error = 0;
            result = 0;
            if (!func_ov011_02170d18(reader) || !func_ov011_02170d28(reader, "SubmitReportResponse")
                || !func_ov011_02170d28(reader, "SubmitReportResult")
                || !func_ov011_02171090(reader, "result", &result)) {
                error = 9;
            } else if (result != 0) {
                if (result == 7) {
                    error = 12;
                } else {
                    error = 8;
                }
            }
        } else {
            error = 7;
        }
        func = client->submitReportFunc;
        client->submitReportBusy = 0;
        if (func != NULL) {
            func(client->self, status, error, client->submitReportData);
            client->submitReportData = NULL;
            client->submitReportFunc = NULL;
        }
    }
}
