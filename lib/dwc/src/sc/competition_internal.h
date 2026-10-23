#ifndef POKEBW2_DWC_COMPETITION_INTERNAL_H
#define POKEBW2_DWC_COMPETITION_INTERNAL_H

#include "dwc/competition.h"

// The parts of the competition client that its files share (sciInterface.c, sciMain.c, sciReport.c, sciSerialize.c
// and sciWebServices.c)

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

// The report's numbers are big-endian
static inline gsi_u16 Sc_SwapU16(gsi_u16 value) {
    return ((value >> 8) & 0x00ff) | ((value << 8) & 0xff00);
}

static inline gsi_u32 Sc_SwapU32(gsi_u32 value) {
    return ((value >> 24) & 0x000000ff) | ((value >> 8) & 0x0000ff00) | ((value << 8) & 0x00ff0000) |
           ((value << 24) & 0xff000000);
}

// sciInterface.c
// The URL of the competition service, set by ScClient_SetServers or once it is asked for
extern char scServiceURL[128];
// Makes a client, returning 0 or an error code; sets up its requests and clears its IDs; and ends them
int sciInterfaceCreate(SCInterface **client);
int sciInterfaceInit(SCInterface *client);
void sciInterfaceDestroy(SCInterface *client);
// Copies the ID of the session, or of the connection, into the client, or clears it for NULL
void sciInterfaceSetSessionId(SCInterface *client, const char *csid);
void sciInterfaceSetConnectionId(SCInterface *client, const char *ccid);
// The URL of the competition service, made on the first call, or NULL when the Wi-Fi library is not running
char *ScClient_BuildUrl(void);

// sciReport.c
// Makes a report for a session and frees it
int sciCreateReport(const char *csid, int version, int playerNum, int teamNum, SCIReport **report);
int sciDestroyReport(SCIReport *report);
// Write a player's record in the roster and its result
int sciReportSetPlayerConnectionId(SCIReport *report, int index, const char *id);
int sciReportSetPlayerTeamIndex(SCIReport *report, int index, int key);
int sciReportSetPlayerGameResult(SCIReport *report, int index, int result);
int sciReportSetPlayerAuthInfo(SCIReport *report, int index, const GSLoginCertificate *certificate, void *unused);
// Start a player's values, or the section of the game's, the players' or the teams' values
int sciReportBeginNewPlayer(SCIReport *report);
int sciReportBeginGlobalData(SCIReport *report);
int sciReportBeginPlayerData(SCIReport *report);
int sciReportBeginTeamData(SCIReport *report);
// Finishes the report, turning its header big-endian
int sciReportEnd(SCIReport *report, int flag, int value);
// Add a value with its key to the current section
int sciReportAddIntValue(SCIReport *report, int key, int value);
int sciReportAddByteValue(SCIReport *report, int key, int value);

// sciSerialize.c
// Each writes a number or a GUID big-endian at the cursor and returns the cursor after it
gsi_u8 *sciSerializeInt8(gsi_u8 *dest, int value);
gsi_u8 *sciSerializeInt16(gsi_u8 *dest, gsi_i16 value);
gsi_u8 *sciSerializeInt32(gsi_u8 *dest, gsi_u32 value);
gsi_u8 *sciSerializeGUID(gsi_u8 *dest, const char *text);

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
