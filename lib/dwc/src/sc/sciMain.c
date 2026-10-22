#include "competition_internal.h"
#include "dwc/ext.h"
#include "dwc/md5.h"

// The competition client's interface to the game: making and running a client, its session, its two requests and the
// report. The file and the names in it are GameSpy's own, as its sciMain.c (UniSpySDK) has them: scInitialize to
// scDestroyReport are its functions in its order. The ROM doesn't name the file. ScClient_GetUrl, after them, is
// Nintendo's, and its name is ours; it could as well start sciReport.c.

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
