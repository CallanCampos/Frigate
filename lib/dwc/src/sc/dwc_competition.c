#include "competition_internal.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "dwc/md5.h"
#include "nitro/std.h"

// GameSpy's competition service, as the Wi-Fi library of overlay 260 talks to it: the client, the calls that start its
// three requests, and the report that the game builds. The report's numbers are big-endian, as the service reads them;
// its first 0x44 bytes are a header. The requests' SOAP messages are in sciWebServices.c. The ROM names neither
// file: this one's name is ours, since its range is four of GameSpy's files (sciInterface.c, sciMain.c, sciReport.c
// and sciSerialize.c). The names in it are GameSpy's own, as its sc.h and sci*.c have them, where the code shows them,
// and ours elsewhere.

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

static int sciInterfaceCreate(SCInterface **client);
static int sciInterfaceInit(SCInterface *client);
static void sciInterfaceDestroy(SCInterface *client);
static char *ScClient_BuildUrl(void);
static int sciCreateReport(const char *csid, int version, int playerNum, int teamNum, SCIReport **report);
static int sciDestroyReport(SCIReport *report);
static int sciReportSetPlayerConnectionId(SCIReport *report, int index, const char *id);
static int sciReportSetPlayerTeamIndex(SCIReport *report, int index, int key);
static int sciReportSetPlayerGameResult(SCIReport *report, int index, int result);
static int sciReportSetPlayerAuthInfo(SCIReport *report, int index, const GSLoginCertificate *certificate,
                                      void *unused);
static int sciReportBeginNewPlayer(SCIReport *report);
static int sciReportBeginGlobalData(SCIReport *report);
static int sciReportBeginPlayerData(SCIReport *report);
static int sciReportBeginTeamData(SCIReport *report);
static int sciReportEndEntity(SCIReport *report);
static int sciReportEnd(SCIReport *report, int flag, int value);
static int sciReportAddIntValue(SCIReport *report, int key, int value);
static int sciReportAddByteValue(SCIReport *report, int key, int value);
static gsi_u8 *sciSerializeInt8(gsi_u8 *dest, int value);
static gsi_u8 *sciSerializeInt16(gsi_u8 *dest, gsi_i16 value);
static gsi_u8 *sciSerializeInt32(gsi_u8 *dest, gsi_u32 value);
static gsi_u8 *sciSerializeGUID(gsi_u8 *dest, const char *text);

// The URL of the competition service, set by ScClient_SetServers or once it is asked for
char scServiceURL[128];

void ScClient_SetServers(int server) {
    switch (server) {
    case 1:
        OS_SNPrintf(scServiceURL, 128,
                    "https://%s.secure2.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(scServiceURL);
        OS_SNPrintf(wsAuthServiceURL, 128, "https://%s.secure2.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(wsAuthServiceURL);
        break;
    case 0:
    default:
        OS_SNPrintf(scServiceURL, 128,
                    "https://%s.secure.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(scServiceURL);
        OS_SNPrintf(wsAuthServiceURL, 128, "https://%s.secure.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(wsAuthServiceURL);
        break;
    }
}

static int sciInterfaceCreate(SCInterface **client) {
    if (__GSIACResult == GSIACAvailable) {
        if (scServiceURL[0] == '\0') {
            OS_SNPrintf(scServiceURL, 128,
                        "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                        __GSIACGamename);
        }
    } else {
        return 1;
    }
    *client = gsimalloc(sizeof(SCInterface));
    if (*client == NULL) {
        return 5;
    }
    memset(*client, 0, sizeof(SCInterface));
    return 0;
}

static int sciInterfaceInit(SCInterface *client) {
    int error = sciWsInit(client, client);

    if (error != 0) {
        return error;
    }
    memset(client->mSessionId, 0, sizeof(client->mSessionId));
    memset(client->mConnectionId, 0, sizeof(client->mConnectionId));
    client->valid = 1;
    return 0;
}

static void sciInterfaceDestroy(SCInterface *client) {
    sciWsDestroy(client);
    memset(client->mSessionId, 0, sizeof(client->mSessionId));
    memset(client->mConnectionId, 0, sizeof(client->mConnectionId));
    // Invalid IDs carry the binary marker 0xDEAD.
    ((gsi_u8 *)client->mSessionId)[0] = 0xde;
    ((gsi_u8 *)client->mSessionId)[1] = 0xad;
    ((gsi_u8 *)client->mConnectionId)[0] = 0xde;
    ((gsi_u8 *)client->mConnectionId)[1] = 0xad;
    client->valid = 0;
}

void sciInterfaceSetSessionId(SCInterface *client, const char *csid) {
    if (csid == NULL) {
        client->mSessionId[0] = '\0';
    } else {
        strcpy(client->mSessionId, csid);
    }
}

void sciInterfaceSetConnectionId(SCInterface *client, const char *ccid) {
    if (ccid == NULL) {
        client->mConnectionId[0] = '\0';
    } else {
        strcpy(client->mConnectionId, ccid);
    }
}

static char *ScClient_BuildUrl(void) {
    if (__GSIACResult == GSIACAvailable) {
        if (scServiceURL[0] == '\0') {
            OS_SNPrintf(scServiceURL, 128,
                        "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                        __GSIACGamename);
        }
        return scServiceURL;
    }
    gsDebugFormat(GSIDebugCat_App, GSIDebugType_State, GSIDebugLevel_HotError, "No availability\r\n");
    return NULL;
}

SCResult scInitialize(int gameId, SCInterface **client) {
    SCInterface *created = NULL;
    int error;

    if (client == NULL) {
        return 2;
    }
    *client = NULL;
    error = sciInterfaceCreate(&created);
    if (error != 0) {
        return error;
    }
    error = sciInterfaceInit(created);
    if (error != 0) {
        return error;
    }
    created->mGameId = gameId;
    *client = created;
    return 0;
}

SCResult scShutdown(SCInterface *client) {
    if (client == NULL) {
        return 2;
    }
    if (client->valid != 0) {
        sciInterfaceDestroy(client);
        gsifree(client);
    }
    return 0;
}

SCResult scThink(SCInterface *client) {
    if (client == NULL) {
        return 2;
    }
    if (client->valid == 0) {
        return 3;
    }
    sciWsThink(client);
    return 0;
}

const char *scGetSessionId(SCInterface *client) {
    return client->mSessionId;
}

const char *scGetConnectionId(SCInterface *client) {
    return client->mConnectionId;
}

SCResult scSetSessionId(SCInterface *client, const gsi_u8 *csid) {
    sciInterfaceSetSessionId(client, csid);
    return 0;
}

SCResult scCreateSession(SCInterface *client, GSLoginCertificate *certificate, GSLoginPrivateData *keys,
                         ScClientFunc func, gsi_time timeout, void *userData) {
    if (Auth_CheckCertificate(certificate)) {
        return sciWsCreateSession(client, client->mGameId, certificate, keys, func, timeout, userData);
    }
    return 2;
}

SCResult scSetReportIntention(SCInterface *client, const gsi_u8 *ccid, gsi_bool authoritative,
                              GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                              gsi_time timeout, void *userData) {
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
    sciInterfaceSetConnectionId(client, ccid);
    return sciWsSetReportIntention(client, client->mGameId, client->mSessionId, client->mConnectionId, authoritative,
                                   certificate, keys, func, timeout, userData);
}

SCResult scSubmitReport(SCInterface *client, SCIReport *report, gsi_bool authoritative, GSLoginCertificate *certificate,
                        GSLoginPrivateData *keys, ScClientFunc func, gsi_time timeout, void *userData) {
    MD5_CTX context;
    gsi_u8 *digest;
    gsi_u8 *clear;
    gsi_u32 i;

    if (client == NULL) {
        return 2;
    }
    if (client->valid == 0) {
        return 3;
    }
    digest = report->mBuffer.mData;
    clear = digest + 8;
    for (i = 0x10; i != 0; i--) {
        *clear++ = 0;
    }
    MD5Init(&context);
    MD5Update(&context, report->mBuffer.mData, report->mBuffer.mPos);
    digest += 8;
    MD5Final(digest, &context);
    return sciWsSubmitReport(client, client->mGameId, client->mSessionId, client->mConnectionId, report, authoritative,
                             certificate, keys, func, timeout, userData);
}

SCResult scCreateReport(SCInterface *client, gsi_u32 version, gsi_u32 playerNum, gsi_u32 teamNum, SCIReport **report) {
    SCIReport *created = NULL;
    int error;

    if (client == NULL) {
        return 2;
    }
    if (report == NULL) {
        return 2;
    }
    error = sciCreateReport(client->mSessionId, version, playerNum, teamNum, &created);
    if (error == 0) {
        *report = created;
    }
    return error;
}

SCResult scReportBeginGlobalData(SCIReport *report) {
    return sciReportBeginGlobalData(report);
}

SCResult scReportBeginPlayerData(SCIReport *report) {
    return sciReportBeginPlayerData(report);
}

SCResult scReportBeginTeamData(SCIReport *report) {
    return sciReportBeginTeamData(report);
}

SCResult scReportBeginNewPlayer(SCIReport *report) {
    return sciReportBeginNewPlayer(report);
}

SCResult scReportEnd(SCIReport *report, gsi_bool flag, SCGameStatus value) {
    return sciReportEnd(report, flag, value);
}

SCResult scReportSetPlayerData(SCIReport *report, gsi_u32 index, const gsi_u8 *id, gsi_u32 key, SCGameResult result,
                               gsi_u32 profileId, const GSLoginCertificate *certificate, const gsi_u8 *unused) {
    SCResult error = SCResult_NO_ERROR;
    int add = gsi_true;
    gsi_u32 i;

    for (i = 0; i < report->keyNum; i++) {
        if (key == report->mTeamIds[i]) {
            add = gsi_false;
            break;
        }
    }
    if (add) {
        report->keyNum++;
        if (report->keyNum > 0x40) {
            return 5;
        }
        report->mTeamIds[report->keyNum - 1] = key;
    }
    if (error == 0) {
        error = sciReportSetPlayerConnectionId(report, index, id);
    }
    if (error == 0) {
        error = sciReportSetPlayerTeamIndex(report, index, key);
    }
    if (error == 0) {
        error = sciReportSetPlayerGameResult(report, index, result);
    }
    if (error == 0) {
        error = sciReportSetPlayerAuthInfo(report, index, certificate, unused);
    }
    return error;
}

SCResult scReportAddIntValue(SCIReport *report, gsi_u16 key, gsi_i32 value) {
    return sciReportAddIntValue(report, key, value);
}

SCResult scReportAddByteValue(SCIReport *report, gsi_u16 key, gsi_i8 value) {
    return sciReportAddByteValue(report, key, value);
}

SCResult scDestroyReport(SCIReport *report) {
    return sciDestroyReport(report);
}

char *ScClient_GetUrl(void) {
    return ScClient_BuildUrl();
}

static int sciCreateReport(const char *csid, int version, int playerNum, int teamNum, SCIReport **report) {
    SCIReport *created;
    SCIReportHeader *header;
    gsi_u32 playerSize;
    gsi_u32 teamSize;

    teamSize = playerNum * 0x10;
    playerSize = teamSize + playerNum * 4;

    created = gsimalloc(sizeof(SCIReport));
    if (created == NULL) {
        return 5;
    }
    memset(created, 0, sizeof(SCIReport));
    header = gsimalloc(0x10000);
    if (header == NULL) {
        gsifree(created);
        return 5;
    }
    memset(header, 0, 0x10000);
    created->mBuffer.unk_00 = 0;
    created->mBuffer.mCapacity = 0x10000;
    memset(header, 0, sizeof(SCIReportHeader));
    header->mProtocolVersion = 0x2000000;
    header->mDeveloperVersion = Sc_SwapU32(version);
    header->mRosterSectionLength = playerSize;
    header->mAuthSectionLength = teamSize;
    header->mResultsSectionLength = (playerNum + teamNum) * 4;
    header->mPlayerCount = playerNum;
    header->mTeamCount = teamNum;
    created->mReportState = SCIReportState_ROSTER;
    created->mBuffer.mData = (gsi_u8 *)header;
    created->mCurEntityStartPos = -1;
    created->keyNum = 0;
    *report = created;
    return 0;
}

static int sciDestroyReport(SCIReport *report) {
    report->mReportState = 0;
    report->mCurEntityKeyCount = 0;
    report->mCurEntityStartPos = 0;
    report->keyNum = 0;
    report->unk_0c = 0;
    gsifree(report->mBuffer.mData);
    report->mBuffer.mData = NULL;
    report->mBuffer.mCapacity = 0;
    report->mBuffer.unk_04 = 0;
    report->mBuffer.mPos = 0;
    gsifree(report);
    return 0;
}

// Each player's record after the header is 0x14 bytes: the 0x10-byte GUID of its connection, then its key
static int sciReportSetPlayerConnectionId(SCIReport *report, int index, const char *id) {
    gsi_u32 offset = sizeof(SCIReportHeader);

    offset += index * 0x14;
    sciSerializeGUID(report->mBuffer.mData + offset, id);
    return 0;
}

static int sciReportSetPlayerTeamIndex(SCIReport *report, int index, int key) {
    gsi_u32 offset = sizeof(SCIReportHeader);

    offset += index * 0x14;
    offset += 0x10;
    sciSerializeInt32(report->mBuffer.mData + offset, key);
    return 0;
}

static int sciReportSetPlayerGameResult(SCIReport *report, int index, int result) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    sciSerializeInt32(report->mBuffer.mData + (header->mRosterSectionLength + sizeof(SCIReportHeader) +
                                               header->mAuthSectionLength + index * 4),
                      result);
    return 0;
}

static int sciReportSetPlayerAuthInfo(SCIReport *report, int index, const GSLoginCertificate *certificate,
                                      void *unused) {
    return 0;
}

static int sciReportBeginNewPlayer(SCIReport *report) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    report->mCurEntityKeyCount = 0;
    report->mCurEntityStartPos = report->mBuffer.mPos;
    report->mBuffer.mPos += 2;
    sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    header->mPlayerSectionLength += 2;
    return 0;
}

static int sciReportBeginGlobalData(SCIReport *report) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    report->mReportState = SCIReportState_GLOBALDATA;
    report->mBuffer.mPos = header->mRosterSectionLength + sizeof(SCIReportHeader) + header->mAuthSectionLength +
                           header->mResultsSectionLength;
    return 0;
}

static int sciReportBeginPlayerData(SCIReport *report) {
    report->mReportState = SCIReportState_PLAYERDATA;
    return 0;
}

static int sciReportBeginTeamData(SCIReport *report) {
    report->mReportState = SCIReportState_TEAMDATA;
    return 0;
}

static int sciReportEndEntity(SCIReport *report) {
    return 0;
}

static int sciReportEnd(SCIReport *report, int flag, int value) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    sciReportEndEntity(report);
    if (flag != 0) {
        header->mFlags |= 1;
    }
    header->mGameStatus = Sc_SwapU32(value);
    header->mFlags = Sc_SwapU32(header->mFlags);
    header->mPlayerCount = Sc_SwapU16(header->mPlayerCount);
    header->mTeamCount = Sc_SwapU16(header->mTeamCount);
    header->mReserved = 0;
    header->mGameKeyCount = Sc_SwapU16(header->mGameKeyCount);
    header->mPlayerKeyCount = Sc_SwapU16(header->mPlayerKeyCount);
    header->mTeamKeyCount = Sc_SwapU16(header->mTeamKeyCount);
    header->mRosterSectionLength = Sc_SwapU32(header->mRosterSectionLength);
    header->mAuthSectionLength = Sc_SwapU32(header->mAuthSectionLength);
    header->mResultsSectionLength = Sc_SwapU32(header->mResultsSectionLength);
    header->mGameSectionLength = Sc_SwapU32(header->mGameSectionLength);
    header->mPlayerSectionLength = Sc_SwapU32(header->mPlayerSectionLength);
    header->mTeamSectionLength = Sc_SwapU32(header->mTeamSectionLength);
    return 0;
}

static int sciReportAddIntValue(SCIReport *report, int key, int value) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    if (report->mReportState == SCIReportState_GLOBALDATA) {
        header->mGameKeyCount++;
        header->mGameSectionLength += 8;
    } else if (report->mReportState == SCIReportState_PLAYERDATA) {
        header->mPlayerKeyCount++;
        header->mPlayerSectionLength += 8;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    } else if (report->mReportState == SCIReportState_TEAMDATA) {
        header->mTeamKeyCount++;
        header->mTeamSectionLength += 8;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    }
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, key);
    report->mBuffer.mPos += 2;
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, 0);
    report->mBuffer.mPos += 2;
    sciSerializeInt32(report->mBuffer.mData + report->mBuffer.mPos, value);
    report->mBuffer.mPos += 4;
    return 0;
}

static int sciReportAddByteValue(SCIReport *report, int key, int value) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    if (report->mReportState == SCIReportState_GLOBALDATA) {
        header->mGameKeyCount++;
        header->mGameSectionLength += 5;
    } else if (report->mReportState == SCIReportState_PLAYERDATA) {
        header->mPlayerKeyCount++;
        header->mPlayerSectionLength += 5;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    } else if (report->mReportState == SCIReportState_TEAMDATA) {
        header->mTeamKeyCount++;
        header->mTeamSectionLength += 5;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    }
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, key);
    report->mBuffer.mPos += 2;
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, 2);
    report->mBuffer.mPos += 2;
    sciSerializeInt8(report->mBuffer.mData + report->mBuffer.mPos, value);
    report->mBuffer.mPos += 1;
    return 0;
}

static gsi_u8 *sciSerializeInt8(gsi_u8 *dest, int value) {
    *dest = value;
    return dest + 1;
}

// The report cursor can be unaligned; copy the swapped scalar as bytes.
static gsi_u8 *sciSerializeInt16(gsi_u8 *dest, gsi_i16 value) {
    gsi_u16 swapped = Sc_SwapU16(value);
    const gsi_u8 *bytes = (const gsi_u8 *)&swapped;

    dest[0] = bytes[0];
    dest[1] = bytes[1];
    return dest + sizeof(swapped);
}

static gsi_u8 *sciSerializeInt32(gsi_u8 *dest, gsi_u32 value) {
    gsi_u32 swapped = Sc_SwapU32(value);
    const gsi_u8 *bytes = (const gsi_u8 *)&swapped;

    dest[0] = bytes[0];
    dest[1] = bytes[1];
    dest[2] = bytes[2];
    dest[3] = bytes[3];
    return dest + sizeof(swapped);
}

static gsi_u8 *sciSerializeGUID(gsi_u8 *dest, const char *text) {
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
    dest = sciSerializeInt32(dest, data1);
    dest = sciSerializeInt16(dest, data2);
    dest = sciSerializeInt16(dest, data3);
    *(ScGuidTail *)dest = tail;
    return dest + sizeof(tail);
}
