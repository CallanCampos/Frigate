#ifndef POKEBW2_DWC_COMPETITION_INTERNAL_H
#define POKEBW2_DWC_COMPETITION_INTERNAL_H

#include "dwc/competition.h"

// The parts of the competition client that its two files share (dwc_competition.c and sciWebServices.c)

// The header of the report's buffer. The fields up to the sizes are set when it is made and finished; the digest is
// made when the report is submitted
typedef struct SCIReportHeader {
    gsi_u32 mProtocolVersion;
    gsi_u32 mDeveloperVersion;
    gsi_u8 mChecksum[0x10];
    gsi_u32 mGameStatus;
    gsi_u32 mFlags;
    gsi_u16 mPlayerCount;
    gsi_u16 mTeamCount;
    gsi_u16 mGameKeyCount;
    gsi_u16 mPlayerKeyCount;
    gsi_u16 mTeamKeyCount;
    gsi_u16 mReserved;
    gsi_u32 mRosterSectionLength;
    gsi_u32 mAuthSectionLength;
    gsi_u32 mResultsSectionLength;
    gsi_u32 mGameSectionLength;
    gsi_u32 mPlayerSectionLength;
    gsi_u32 mTeamSectionLength;
} SCIReportHeader;

// dwc_competition.c
// The URL of the competition service, set by ScClient_SetServers or once it is asked for
extern char scServiceURL[128];
// Copies the ID of the session, or of the connection, into the client, or clears it for NULL
void sciInterfaceSetSessionId(SCInterface *client, const char *csid);
void sciInterfaceSetConnectionId(SCInterface *client, const char *ccid);

// sciWebServices.c
// Sets up the client's requests, returning 0 or an error code when the Wi-Fi library can't run them, and ends them
int sciWsInit(SCInterface *client, SCInterface *self);
void sciWsDestroy(SCInterface *client);
// Runs the Wi-Fi library's asynchronous process for a frame
void sciWsThink(SCInterface *client);
// Each posts a request's SOAP message to the service, returning 0 or an error code; the client's function is called
// when the answer has been read
int sciWsCreateSession(SCInterface *client, int gameId, GSLoginCertificate *certificate, GSLoginPrivateData *keys,
                       ScClientFunc func, int timeout, void *userData);
int sciWsSetReportIntention(SCInterface *client, int gameId, const char *csid, const char *ccid, int authoritative,
                            GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func, int timeout,
                            void *userData);
int sciWsSubmitReport(SCInterface *client, int gameId, const char *csid, const char *ccid, SCIReport *report,
                      int authoritative, GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                      int timeout, void *userData);

#endif // POKEBW2_DWC_COMPETITION_INTERNAL_H
