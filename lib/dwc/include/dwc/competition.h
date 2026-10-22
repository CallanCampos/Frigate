#ifndef POKEBW2_DWC_COMPETITION_H
#define POKEBW2_DWC_COMPETITION_H

#include "dwc/auth.h"
#include "dwc/ghttp.h"
#include "dwc/nonport.h"

// GameSpy's competition service in overlay 260 (dwc_competition.c and dwc_competition_soap.c), which keeps the results
// of the Wi-Fi battle competitions. A client makes a session, tells that it will report on it, and submits a report,
// three requests that post a SOAP message to the service ("CreateSession", "SetReportIntention", "SubmitReport"). The
// report is a buffer of big-endian numbers that the game builds record by record

typedef struct ScClient ScClient;
typedef struct ScReport ScReport;

// GameSpy's competition-service result codes.
typedef enum {
    SCResult_NO_ERROR = 0,
    SCResult_NO_AVAILABILITY_CHECK,
    SCResult_INVALID_PARAMETERS,
    SCResult_NOT_INITIALIZED,
    SCResult_CORE_NOT_INITIALIZED,
    SCResult_OUT_OF_MEMORY,
    SCResult_CALLBACK_PENDING,
    SCResult_HTTP_ERROR,
    SCResult_UNKNOWN_RESPONSE,
    SCResult_RESPONSE_INVALID,
    SCResult_REPORT_INCOMPLETE,
    SCResult_REPORT_INVALID,
    SCResult_SUBMISSION_FAILED,
    SCResult_UNKNOWN_ERROR,
    SCResultMax
} SCResult;

// Called with the client, the status of the request, the error (0 if none) and the user data
// How a player's game ended, and how complete a report is
typedef enum {
    SCGameResult_WIN,
    SCGameResult_LOSS,
    SCGameResult_DRAW,
    SCGameResult_DISCONNECT,
    SCGameResult_DESYNC,
    SCGameResult_NONE,
    SCGameResultMax
} SCGameResult;

typedef enum { SCGameStatus_COMPLETE, SCGameStatus_PARTIAL, SCGameStatus_BROKEN, SCGameStatusMax } SCGameStatus;

typedef void (*ScClientFunc)(ScClient *client, GHTTPResult httpResult, SCResult result, void *userData);

// The client of the service: the functions to call back and the flags that tell that a request is running, each for
// the three requests, then the report being submitted, the game, and the IDs of the session and of the connection
struct ScClient {
    ScClient *self;
    ScClientFunc createSessionFunc;
    ScClientFunc reportIntentionFunc;
    ScClientFunc submitReportFunc;
    int reportIntentionBusy;
    int createSessionBusy;
    int submitReportBusy;
    int unk_1c;
    int unk_20;
    int unk_24;
    void *reportIntentionData;
    void *createSessionData;
    void *submitReportData;
    int unk_34;
    int unk_38;
    int unk_3c;
    void *report;
    int reportSize;
    int ready;
    int gameId;
    int valid;
    int unk_54;
    int unk_58;
    char csid[0x28];
    char ccid[0x28];
};

// The report: the phase of the data being added, the group being added to, the keys that have a record and the
// buffer, which starts with a header of 0x44 bytes. The data has the header's player and team sections, then the
// records of the game, of the players and of the teams
struct ScReport {
    int phase;
    int groupOffset;
    gsi_u16 groupCount;
    int unk_0c;
    gsi_u32 keyNum;
    int keys[0x40];
    int unk_114;
    int unk_118;
    int bufferSize;
    int used;
    gsi_u8 *buffer;
};

// The phases of a report, which are those of the records' sections
typedef enum ScReportPhase {
    SC_REPORT_PHASE_NEW = 1,
    SC_REPORT_PHASE_GAME = 2,
    SC_REPORT_PHASE_PLAYER = 3,
    SC_REPORT_PHASE_TEAM = 4,
} ScReportPhase;

// Sets up the URLs of the competition and of the authentication services for a kind of server
void ScClient_SetServers(int server);
// The URL of the competition service, or 0 when the Wi-Fi library is not running
char *ScClient_GetUrl(void);
// Makes a client for a game, returning 0 or an error code, and frees it and runs the library for a frame (the
// second returns 2 for no client and 3 for one that is not set up)
SCResult ScClient_Create(int gameId, ScClient **client);
SCResult ScClient_Destroy(ScClient *client);
SCResult ScClient_Update(ScClient *client);
// The IDs of the session and of the connection, and storing the session's
const char *ScClient_GetCsid(ScClient *client);
const char *ScClient_GetCcid(ScClient *client);
SCResult ScClient_SetCsid(ScClient *client, const gsi_u8 *csid);
// The three requests, each returning 0 or an error code and calling a function when it has ended. The time they may
// take is not used
SCResult ScClient_CreateSession(ScClient *client, GSLoginCertificate *certificate, GSLoginPrivateData *keys,
                                ScClientFunc func, gsi_time timeout, void *userData);
SCResult ScClient_SetReportIntention(ScClient *client, const gsi_u8 *ccid, gsi_bool authoritative,
                                     GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                                     gsi_time timeout, void *userData);
SCResult ScClient_SubmitReport(ScClient *client, ScReport *report, gsi_bool authoritative,
                               GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                               gsi_time timeout, void *userData);

// Makes a report of a version for players and teams, returning 0 or an error code
SCResult ScClient_CreateReport(ScClient *client, gsi_u32 version, gsi_u32 playerNum, gsi_u32 teamNum,
                               ScReport **report);
// Starts the records of the game, of the players and of the teams, and a group of them (one for each player or team)
SCResult ScReport_BeginGame(ScReport *report);
SCResult ScReport_BeginPlayers(ScReport *report);
SCResult ScReport_BeginTeams(ScReport *report);
SCResult ScReport_BeginGroup(ScReport *report);
// Sets the ID of a player, his key and his result; the profile ID of the certificate and the last two are given on but
// not used
SCResult ScReport_SetPlayer(ScReport *report, gsi_u32 index, const gsi_u8 *id, gsi_u32 key, SCGameResult result,
                            gsi_u32 profileId, const GSLoginCertificate *certificate, const gsi_u8 *unused);
// Ends the header, with a flag and a value
SCResult ScReport_Finish(ScReport *report, gsi_bool flag, SCGameStatus value);
// Adds a record of an int, or of a byte, to the section that is being added to
SCResult ScReport_AddInt(ScReport *report, gsi_u16 key, gsi_i32 value);
SCResult ScReport_AddByte(ScReport *report, gsi_u16 key, gsi_i8 value);
// Frees the report
SCResult ScReport_Free(ScReport *report);

#endif // POKEBW2_DWC_COMPETITION_H
