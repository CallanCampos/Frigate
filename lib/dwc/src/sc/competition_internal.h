#ifndef POKEBW2_DWC_COMPETITION_INTERNAL_H
#define POKEBW2_DWC_COMPETITION_INTERNAL_H

#include "types.h"
#include "dwc/auth.h"
#include "dwc/competition.h"

// The parts of the competition client that its two files share (dwc_competition.c and dwc_competition_soap.c)

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
// Post the three requests' SOAP messages to the service, each returning 0 or an error code; the client's function is
// called when the answer has been read
int ScClient_PostCreateSession(ScClient *client, int gameId, AuthCertificate *certificate, AuthKeys *keys,
                               ScClientFunc func, int timeout, void *userData);
int ScClient_PostReportIntention(ScClient *client, int gameId, const char *csid, const char *ccid, int authoritative,
                                 AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func, int timeout,
                                 void *userData);
int ScClient_PostReport(ScClient *client, int gameId, const char *csid, const char *ccid, ScReport *report,
                        int authoritative, AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func, int timeout,
                        void *userData);

#endif // POKEBW2_DWC_COMPETITION_INTERNAL_H
