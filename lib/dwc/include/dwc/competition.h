#ifndef POKEBW2_DWC_COMPETITION_H
#define POKEBW2_DWC_COMPETITION_H

#include "dwc/AuthService.h"
#include "dwc/ghttp.h"
#include "dwc/nonport.h"

// GameSpy's competition service in overlay 260 (dwc_competition.c and sciWebServices.c), which keeps the results
// of the Wi-Fi battle competitions. A client makes a session, tells that it will report on it, and submits a report,
// three requests that post a SOAP message to the service ("CreateSession", "SetReportIntention", "SubmitReport"). The
// report is a buffer of big-endian numbers that the game builds record by record. The names are GameSpy's own, as its
// sc.h and sci*.h have them, where the code shows them, and ours elsewhere: the client is one object of an SCInterface
// whose SCWebServices is laid out in front of its own fields

typedef struct SCInterface SCInterface;
typedef struct SCIReport SCIReport;

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

typedef void (*ScClientFunc)(SCInterface *client, GHTTPResult httpResult, SCResult result, void *userData);

// The client of the service: the functions to call back and the flags that tell that a request is running, each for
// the three requests, then the report being submitted, the game, and the IDs of the session and of the connection
struct SCInterface {
    SCInterface *mInterface;
    ScClientFunc mCreateSessionCallback;
    ScClientFunc mSetReportIntentionCallback;
    ScClientFunc mSubmitReportDataCallback;
    int mSetReportIntentionPending;
    int mCreateSessionPending;
    int mSubmitReportPending;
    int unk_1c;
    int unk_20;
    int unk_24;
    void *mSetReportIntentionUserData;
    void *mCreateSessionUserData;
    void *mSubmitReportUserData;
    int unk_34;
    int unk_38;
    int unk_3c;
    void *mSubmitReportData;
    int mSubmitReportLength;
    int ready;
    int mGameId;
    int valid;
    int unk_54;
    int unk_58;
    char mSessionId[0x28];
    char mConnectionId[0x28];
};

// The buffer of a report: its size and where the next byte goes
typedef struct SCIReportBuffer {
    int unk_00;
    int unk_04;
    int mCapacity;
    int mPos;
    gsi_u8 *mData;
} SCIReportBuffer;

// The report: the phase of the data being added, the group being added to, the keys that have a record and the
// buffer, which starts with a header of 0x44 bytes. The data has the header's player and team sections, then the
// records of the game, of the players and of the teams
struct SCIReport {
    int mReportState;
    int mCurEntityStartPos;
    gsi_u16 mCurEntityKeyCount;
    int unk_0c;
    gsi_u32 keyNum;
    int mTeamIds[0x40];
    SCIReportBuffer mBuffer;
};

// The phases of a report, which are those of the records' sections
typedef enum SCIReportState {
    SCIReportState_ROSTER = 1,
    SCIReportState_GLOBALDATA = 2,
    SCIReportState_PLAYERDATA = 3,
    SCIReportState_TEAMDATA = 4,
} SCIReportState;

// Sets up the URLs of the competition and of the authentication services for a kind of server
void ScClient_SetServers(int server);
// The URL of the competition service, or 0 when the Wi-Fi library is not running
char *ScClient_GetUrl(void);
// Makes a client for a game, returning 0 or an error code, and frees it and runs the library for a frame (the
// second returns 2 for no client and 3 for one that is not set up)
SCResult scInitialize(int gameId, SCInterface **client);
SCResult scShutdown(SCInterface *client);
SCResult scThink(SCInterface *client);
// The IDs of the session and of the connection, and storing the session's
const char *scGetSessionId(SCInterface *client);
const char *scGetConnectionId(SCInterface *client);
SCResult scSetSessionId(SCInterface *client, const gsi_u8 *csid);
// The three requests, each returning 0 or an error code and calling a function when it has ended. The time they may
// take is not used
SCResult scCreateSession(SCInterface *client, GSLoginCertificate *certificate, GSLoginPrivateData *keys,
                         ScClientFunc func, gsi_time timeout, void *userData);
SCResult scSetReportIntention(SCInterface *client, const gsi_u8 *ccid, gsi_bool authoritative,
                              GSLoginCertificate *certificate, GSLoginPrivateData *keys, ScClientFunc func,
                              gsi_time timeout, void *userData);
SCResult scSubmitReport(SCInterface *client, SCIReport *report, gsi_bool authoritative, GSLoginCertificate *certificate,
                        GSLoginPrivateData *keys, ScClientFunc func, gsi_time timeout, void *userData);

// Makes a report of a version for players and teams, returning 0 or an error code
SCResult scCreateReport(SCInterface *client, gsi_u32 version, gsi_u32 playerNum, gsi_u32 teamNum, SCIReport **report);
// Starts the records of the game, of the players and of the teams, and a group of them (one for each player or team)
SCResult scReportBeginGlobalData(SCIReport *report);
SCResult scReportBeginPlayerData(SCIReport *report);
SCResult scReportBeginTeamData(SCIReport *report);
SCResult scReportBeginNewPlayer(SCIReport *report);
// Sets the ID of a player, his key and his result; the profile ID of the certificate and the last two are given on but
// not used
SCResult scReportSetPlayerData(SCIReport *report, gsi_u32 index, const gsi_u8 *id, gsi_u32 key, SCGameResult result,
                               gsi_u32 profileId, const GSLoginCertificate *certificate, const gsi_u8 *unused);
// Ends the header, with a flag and a value
SCResult scReportEnd(SCIReport *report, gsi_bool flag, SCGameStatus value);
// Adds a record of an int, or of a byte, to the section that is being added to
SCResult scReportAddIntValue(SCIReport *report, gsi_u16 key, gsi_i32 value);
SCResult scReportAddByteValue(SCIReport *report, gsi_u16 key, gsi_i8 value);
// Frees the report
SCResult scDestroyReport(SCIReport *report);

#endif // POKEBW2_DWC_COMPETITION_H
