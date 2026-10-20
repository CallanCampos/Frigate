#include "competition_internal.h"
#include "dwc/ext.h"
#include "dwc/soap.h"

// The competition client's SOAP requests: setting up and ending the client's request state, posting the CreateSession,
// SetReportIntention and SubmitReport messages to the service, and reading their answers. The ROM doesn't name the
// file; the name is ours. Its .data starts with the namespace table, which is why it is a file of its own
// (docs/matching.md). The first three functions have no data of their own, so that they start the file is a guess.

static void ScClient_OnCreateSession(int status, int unused, SoapReader *reader, void *userData);
static void ScClient_OnReportIntention(int status, int unused, SoapReader *reader, void *userData);
static void ScClient_PutReportForm(void *form, void *userData);
static void ScClient_OnSubmitReport(int status, int unused, SoapReader *reader, void *userData);

// The declaration of the messages' namespace
static const char *sCompetitionNamespaces[] = { "gsc=\"http://gamespy.net/competition/\"" };

int ScClient_Begin(ScClient *client, ScClient *self) {
    if (gsCoreIsShutdown() != 0) {
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

void ScClient_End(ScClient *client) {
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

void ScClient_RunLibrary(ScClient *client) {
    gsCoreThink(0);
}

int ScClient_PostCreateSession(ScClient *client, int gameId, AuthCertificate *certificate, AuthKeys *keys,
                               ScClientFunc func, int timeout, void *userData) {
    SoapWriter *writer;

    if (client->createSessionBusy != 0) {
        return 6;
    }
    writer = gsXmlCreateStreamWriter(sCompetitionNamespaces, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!gsXmlWriteOpenTag(writer, "gsc", "CreateSession") || !gsXmlWriteOpenTag(writer, "gsc", "certificate") ||
        !Auth_WriteCertificate(certificate, "gsc", writer) || !gsXmlWriteCloseTag(writer, "gsc", "certificate") ||
        !gsXmlWriteHexBinaryElement(writer, "gsc", "proof", keys->proof, sizeof(keys->proof)) ||
        !gsXmlWriteIntElement(writer, "gsc", "gameid", gameId) || !gsXmlWriteCloseTag(writer, "gsc", "CreateSession") ||
        !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return 7;
    }
    client->createSessionFunc = func;
    client->createSessionData = userData;
    client->createSessionBusy = 1;
    gsiExecuteSoap(gCompetitionUrl, "SOAPAction: \"http://gamespy.net/competition/CreateSession\"", writer,
                   ScClient_OnCreateSession, client);
    return 0;
}

static void ScClient_OnCreateSession(int status, int unused, SoapReader *reader, void *userData) {
    ScClient *client = userData;
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
                            ScClient_CopyCsid(client->self, csid);
                            ScClient_CopyCcid(client->self, ccid);
                            error = 0;
                        }
                    } else {
                        error = 9;
                    }
                }
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

int ScClient_PostReportIntention(ScClient *client, int gameId, const char *csid, const char *ccid, int authoritative,
                                 AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func, int timeout,
                                 void *userData) {
    SoapWriter *writer;

    if (client->reportIntentionBusy != 0) {
        return 6;
    }
    writer = gsXmlCreateStreamWriter(sCompetitionNamespaces, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!gsXmlWriteOpenTag(writer, "gsc", "SetReportIntention") || !gsXmlWriteOpenTag(writer, "gsc", "certificate") ||
        !Auth_WriteCertificate(certificate, "gsc", writer) || !gsXmlWriteCloseTag(writer, "gsc", "certificate") ||
        !gsXmlWriteHexBinaryElement(writer, "gsc", "proof", keys->proof, sizeof(keys->proof)) ||
        !gsXmlWriteStringElement(writer, "gsc", "csid", csid) ||
        !gsXmlWriteStringElement(writer, "gsc", "ccid", ccid) ||
        !gsXmlWriteIntElement(writer, "gsc", "gameid", gameId) ||
        !gsXmlWriteIntElement(writer, "gsc", "authoritative", authoritative ? 1 : 0) ||
        !gsXmlWriteCloseTag(writer, "gsc", "SetReportIntention") || !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return 7;
    }
    client->reportIntentionFunc = func;
    client->reportIntentionData = userData;
    client->reportIntentionBusy = 1;
    gsiExecuteSoap(gCompetitionUrl, "SOAPAction: \"http://gamespy.net/competition/SetReportIntention\"", writer,
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
            if (!gsXmlMoveToStart(reader) || !gsXmlMoveToNext(reader, "SetReportIntentionResponse") ||
                !gsXmlMoveToNext(reader, "SetReportIntentionResult") ||
                !gsXmlReadChildAsInt(reader, "result", &result) ||
                !gsXmlReadChildAsStringNT(reader, "ccid", ccid, sizeof(ccid))) {
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

int ScClient_PostReport(ScClient *client, int gameId, const char *csid, const char *ccid, ScReport *report,
                        int authoritative, AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func, int timeout,
                        void *userData) {
    SoapWriter *writer;

    if (client->submitReportBusy != 0) {
        return 6;
    }
    if (report->used < sizeof(ScReportHeader)) {
        return 12;
    }
    writer = gsXmlCreateStreamWriter(sCompetitionNamespaces, 1);
    if (writer == NULL) {
        return 5;
    }
    if (!gsXmlWriteOpenTag(writer, "gsc", "SubmitReport") || !gsXmlWriteOpenTag(writer, "gsc", "certificate") ||
        !Auth_WriteCertificate(certificate, "gsc", writer) || !gsXmlWriteCloseTag(writer, "gsc", "certificate") ||
        !gsXmlWriteHexBinaryElement(writer, "gsc", "proof", keys->proof, sizeof(keys->proof)) ||
        !gsXmlWriteStringElement(writer, "gsc", "csid", csid) ||
        !gsXmlWriteStringElement(writer, "gsc", "ccid", ccid) ||
        !gsXmlWriteIntElement(writer, "gsc", "gameid", gameId) ||
        !gsXmlWriteIntElement(writer, "gsc", "authoritative", authoritative ? 1 : 0) ||
        !gsXmlWriteCloseTag(writer, "gsc", "SubmitReport") || !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return 5;
    }
    client->report = report->buffer;
    client->reportSize = report->used;
    client->submitReportFunc = func;
    client->submitReportData = userData;
    client->submitReportBusy = 1;
    gsiExecuteSoapCustom(gCompetitionUrl, "SOAPAction: \"http://gamespy.net/competition/SubmitReport\"", writer,
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
        func = client->submitReportFunc;
        client->submitReportBusy = 0;
        if (func != NULL) {
            func(client->self, status, error, client->submitReportData);
            client->submitReportData = NULL;
            client->submitReportFunc = NULL;
        }
    }
}
