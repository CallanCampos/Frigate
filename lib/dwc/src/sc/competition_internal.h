#ifndef POKEBW2_DWC_COMPETITION_INTERNAL_H
#define POKEBW2_DWC_COMPETITION_INTERNAL_H

#include "dwc/competition.h"

// The parts of the competition client that its two files share (dwc_competition.c and dwc_competition_soap.c)

// The header of the report's buffer. The fields up to the sizes are set when it is made and finished; the digest is
// made when the report is submitted
typedef struct ScReportHeader {
    gsi_u32 version;
    gsi_u32 id;
    gsi_u8 digest[0x10];
    gsi_u32 value;
    gsi_u32 flags;
    gsi_u16 playerNum;
    gsi_u16 teamNum;
    gsi_u16 gameRecordNum;
    gsi_u16 playerRecordNum;
    gsi_u16 teamRecordNum;
    gsi_u16 unk_2a;
    gsi_u32 playerSize;
    gsi_u32 teamSize;
    gsi_u32 resultSize;
    gsi_u32 gameSize;
    gsi_u32 playerRecordSize;
    gsi_u32 teamRecordSize;
} ScReportHeader;

// dwc_competition.c
// The URL of the competition service, set by ScClient_SetServers or once it is asked for
extern char gCompetitionUrl[128];
// Copies the ID of the session, or of the connection, into the client, or clears it for NULL
void ScClient_CopyCsid(ScClient *client, const char *csid);
void ScClient_CopyCcid(ScClient *client, const char *ccid);

// dwc_competition_soap.c
// Sets up the client's requests, returning 0 or an error code when the Wi-Fi library can't run them, and ends them
int ScClient_Begin(ScClient *client, ScClient *self);
void ScClient_End(ScClient *client);
// Runs the Wi-Fi library's asynchronous process for a frame
void ScClient_RunLibrary(ScClient *client);
// Each posts a request's SOAP message to the service, returning 0 or an error code; the client's function is called
// when the answer has been read
int ScClient_PostCreateSession(ScClient *client, int gameId, GSLoginCertificate *certificate, GSLoginPrivateData *keys,
                               ScClientFunc func, int timeout, void *userData);
int ScClient_PostReportIntention(ScClient *client, int gameId, const char *csid, const char *ccid, int authoritative,
                                 GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                                 int timeout, void *userData);
int ScClient_PostReport(ScClient *client, int gameId, const char *csid, const char *ccid, ScReport *report,
                        int authoritative, GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                        int timeout, void *userData);

#endif // POKEBW2_DWC_COMPETITION_INTERNAL_H
