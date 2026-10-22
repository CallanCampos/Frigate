#include "competition_internal.h"
#include "dwc/ext.h"
#include "dwc/soap.h"

// The competition client's SOAP requests: setting up and ending the client's request state, posting the CreateSession,
// SetReportIntention and SubmitReport messages to the service, and reading their answers. The ROM doesn't name the
// file; it and its names are GameSpy's own, as its sciWebServices.c has them. Its .data starts with the namespace
// table, which is why it is a file of its own (docs/matching.md). The first three functions have no data of their own,
// so that they start the file is a guess.

static void sciWsCreateSessionCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader,
                                       void *userData);
static void sciWsSetReportIntentionCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader,
                                            void *userData);
static void sciWsSubmitReportCustom(GHTTPPost form, void *userData);
static void sciWsSubmitReportCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader,
                                      void *userData);

// The declaration of the messages' namespace
static const char *SC_SERVICE_NAMESPACES[] = { "gsc=\"http://gamespy.net/competition/\"" };

int sciWsInit(SCInterface *client, SCInterface *self) {
    if (gsCoreIsShutdown() != GSCore_IN_USE) {
        return 4;
    }
    client->mInterface = self;
    client->mCreateSessionCallback = NULL;
    client->mSetReportIntentionCallback = NULL;
    client->mSubmitReportDataCallback = NULL;
    client->mCreateSessionUserData = NULL;
    client->mSetReportIntentionUserData = NULL;
    client->mSubmitReportUserData = NULL;
    client->mCreateSessionPending = 0;
    client->mSetReportIntentionPending = 0;
    client->mSubmitReportPending = 0;
    client->ready = 1;
    return 0;
}

void sciWsDestroy(SCInterface *client) {
    client->ready = 0;
    client->mCreateSessionCallback = NULL;
    client->mSetReportIntentionCallback = NULL;
    client->mSubmitReportDataCallback = NULL;
    client->mCreateSessionUserData = NULL;
    client->mSetReportIntentionUserData = NULL;
    client->mSubmitReportUserData = NULL;
    client->mCreateSessionPending = 0;
    client->mSetReportIntentionPending = 0;
    client->mSubmitReportPending = 0;
    client->mInterface = NULL;
}

void sciWsThink(SCInterface *client) {
    gsCoreThink(0);
}

int sciWsCreateSession(SCInterface *client, int gameId, GSLoginCertificate *certificate, GSLoginPrivateData *keys,
                       ScClientFunc func, int timeout, void *userData) {
    GSXmlStreamWriter writer;

    if (client->mCreateSessionPending != 0) {
        return 6;
    }
    writer = gsXmlCreateStreamWriter(SC_SERVICE_NAMESPACES, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!gsXmlWriteOpenTag(writer, "gsc", "CreateSession") || !gsXmlWriteOpenTag(writer, "gsc", "certificate") ||
        !Auth_WriteCertificate(certificate, "gsc", writer) || !gsXmlWriteCloseTag(writer, "gsc", "certificate") ||
        !gsXmlWriteHexBinaryElement(writer, "gsc", "proof", keys->mKeyHash, sizeof(keys->mKeyHash)) ||
        !gsXmlWriteIntElement(writer, "gsc", "gameid", gameId) || !gsXmlWriteCloseTag(writer, "gsc", "CreateSession") ||
        !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return 7;
    }
    client->mCreateSessionCallback = func;
    client->mCreateSessionUserData = userData;
    client->mCreateSessionPending = 1;
    gsiExecuteSoap(scServiceURL, "SOAPAction: \"http://gamespy.net/competition/CreateSession\"", writer,
                   sciWsCreateSessionCallback, client);
    return 0;
}

static void sciWsCreateSessionCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader,
                                       void *userData) {
    SCInterface *client = userData;
    int result;
    char csid[0xff];
    char ccid[0xff];
    ScClientFunc func;
    int error = 7;

    if (client->ready) {
        if (status == 0) {
            result = 0;
            if (!gsXmlMoveToStart(reader)) {
                error = 9;
            } else if (!gsXmlMoveToNext(reader, "CreateSessionResponse") &&
                       !gsXmlMoveToNext(reader, "CreateMatchlessSessionResponse")) {
                error = 9;
            }
            if (!gsXmlMoveToNext(reader, "CreateSessionResult") &&
                !gsXmlMoveToNext(reader, "CreateMatchlessSessionResult")) {
                error = 9;
            }
            if (!gsXmlReadChildAsInt(reader, "result", &result)) {
                error = 9;
            } else {
                if (error != 9) {
                    if (result == 0) {
                        if (!gsXmlReadChildAsStringNT(reader, "csid", csid, sizeof(csid)) ||
                            !gsXmlReadChildAsStringNT(reader, "ccid", ccid, sizeof(ccid))) {
                            error = 9;
                        } else {
                            sciInterfaceSetSessionId(client->mInterface, csid);
                            sciInterfaceSetConnectionId(client->mInterface, ccid);
                            error = 0;
                        }
                    } else {
                        error = 9;
                    }
                }
            }
        }
        func = client->mCreateSessionCallback;
        client->mCreateSessionPending = 0;
        if (func != NULL) {
            func(client->mInterface, status, error, client->mCreateSessionUserData);
            client->mCreateSessionUserData = NULL;
            client->mCreateSessionCallback = NULL;
        }
    }
}

int sciWsSetReportIntention(SCInterface *client, int gameId, const char *csid, const char *ccid, int authoritative,
                            GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func, int timeout,
                            void *userData) {
    GSXmlStreamWriter writer;

    if (client->mSetReportIntentionPending != 0) {
        return 6;
    }
    writer = gsXmlCreateStreamWriter(SC_SERVICE_NAMESPACES, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!gsXmlWriteOpenTag(writer, "gsc", "SetReportIntention") || !gsXmlWriteOpenTag(writer, "gsc", "certificate") ||
        !Auth_WriteCertificate(certificate, "gsc", writer) || !gsXmlWriteCloseTag(writer, "gsc", "certificate") ||
        !gsXmlWriteHexBinaryElement(writer, "gsc", "proof", keys->mKeyHash, sizeof(keys->mKeyHash)) ||
        !gsXmlWriteStringElement(writer, "gsc", "csid", csid) ||
        !gsXmlWriteStringElement(writer, "gsc", "ccid", ccid) ||
        !gsXmlWriteIntElement(writer, "gsc", "gameid", gameId) ||
        !gsXmlWriteIntElement(writer, "gsc", "authoritative", authoritative ? 1 : 0) ||
        !gsXmlWriteCloseTag(writer, "gsc", "SetReportIntention") || !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return 7;
    }
    client->mSetReportIntentionCallback = func;
    client->mSetReportIntentionUserData = userData;
    client->mSetReportIntentionPending = 1;
    gsiExecuteSoap(scServiceURL, "SOAPAction: \"http://gamespy.net/competition/SetReportIntention\"", writer,
                   sciWsSetReportIntentionCallback, client);
    return 0;
}

static void sciWsSetReportIntentionCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader,
                                            void *userData) {
    SCInterface *client = userData;
    int result;
    char ccid[0xff];
    ScClientFunc func;
    int error;

    if (client->ready) {
        if (status == 0) {
            error = 0;
            result = 0;
            if (!gsXmlMoveToStart(reader) || !gsXmlMoveToNext(reader, "SetReportIntentionResponse") ||
                !gsXmlMoveToNext(reader, "SetReportIntentionResult") ||
                !gsXmlReadChildAsInt(reader, "result", &result) ||
                !gsXmlReadChildAsStringNT(reader, "ccid", ccid, sizeof(ccid))) {
                error = 9;
            } else if (result == 0) {
                sciInterfaceSetConnectionId(client->mInterface, ccid);
            } else {
                error = 8;
            }
        } else {
            error = 7;
        }
        func = client->mSetReportIntentionCallback;
        client->mSetReportIntentionPending = 0;
        if (func != NULL) {
            func(client->mInterface, status, error, client->mSetReportIntentionUserData);
            client->mSetReportIntentionUserData = NULL;
            client->mSetReportIntentionCallback = NULL;
        }
    }
}

static void sciWsSubmitReportCustom(GHTTPPost form, void *userData) {
    SCInterface *client = userData;

    ghiPostAddFileFromMemory(form, "report", client->mSubmitReportData, client->mSubmitReportLength, "report",
                             "application/bin");
}

int sciWsSubmitReport(SCInterface *client, int gameId, const char *csid, const char *ccid, SCIReport *report,
                      int authoritative, GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                      int timeout, void *userData) {
    GSXmlStreamWriter writer;

    if (client->mSubmitReportPending != 0) {
        return 6;
    }
    if (report->mBuffer.mPos < sizeof(SCIReportHeader)) {
        return 12;
    }
    writer = gsXmlCreateStreamWriter(SC_SERVICE_NAMESPACES, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!gsXmlWriteOpenTag(writer, "gsc", "SubmitReport") || !gsXmlWriteOpenTag(writer, "gsc", "certificate") ||
        !Auth_WriteCertificate(certificate, "gsc", writer) || !gsXmlWriteCloseTag(writer, "gsc", "certificate") ||
        !gsXmlWriteHexBinaryElement(writer, "gsc", "proof", keys->mKeyHash, sizeof(keys->mKeyHash)) ||
        !gsXmlWriteStringElement(writer, "gsc", "csid", csid) ||
        !gsXmlWriteStringElement(writer, "gsc", "ccid", ccid) ||
        !gsXmlWriteIntElement(writer, "gsc", "gameid", gameId) ||
        !gsXmlWriteIntElement(writer, "gsc", "authoritative", authoritative ? 1 : 0) ||
        !gsXmlWriteCloseTag(writer, "gsc", "SubmitReport") || !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return 5;
    }
    client->mSubmitReportData = report->mBuffer.mData;
    client->mSubmitReportLength = report->mBuffer.mPos;
    client->mSubmitReportDataCallback = func;
    client->mSubmitReportUserData = userData;
    client->mSubmitReportPending = 1;
    gsiExecuteSoapCustom(scServiceURL, "SOAPAction: \"http://gamespy.net/competition/SubmitReport\"", writer,
                         sciWsSubmitReportCallback, sciWsSubmitReportCustom, client);
    return 0;
}

static void sciWsSubmitReportCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader,
                                      void *userData) {
    SCInterface *client = userData;
    int result;
    ScClientFunc func;
    int error;

    if (client->ready) {
        if (status == 0) {
            error = 0;
            result = 0;
            if (!gsXmlMoveToStart(reader) || !gsXmlMoveToNext(reader, "SubmitReportResponse") ||
                !gsXmlMoveToNext(reader, "SubmitReportResult") || !gsXmlReadChildAsInt(reader, "result", &result)) {
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
        func = client->mSubmitReportDataCallback;
        client->mSubmitReportPending = 0;
        if (func != NULL) {
            func(client->mInterface, status, error, client->mSubmitReportUserData);
            client->mSubmitReportUserData = NULL;
            client->mSubmitReportDataCallback = NULL;
        }
    }
}
