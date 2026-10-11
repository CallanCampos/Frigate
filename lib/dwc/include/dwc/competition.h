#ifndef POKEBW2_DWC_COMPETITION_H
#define POKEBW2_DWC_COMPETITION_H

#include "types.h"
#include "dwc/auth.h"

// GameSpy's competition service in overlay 260 (dwc_competition.c), which keeps the results of the Wi-Fi battle
// competitions. A client makes a session, tells that it will report on it, and submits a report, three requests
// that post a SOAP message to the service ("CreateSession", "SetReportIntention", "SubmitReport"). The report is a
// buffer of big-endian numbers that the game builds record by record

typedef struct ScClient ScClient;
typedef struct ScReport ScReport;

// Called with the client, the status of the request, the error (0 if none) and the user data
typedef void (*ScClientFunc)(ScClient *client, int status, int error, void *userData);

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
    u16 groupCount;
    int unk_0c;
    u32 keyNum;
    int keys[0x40];
    int unk_114;
    int unk_118;
    int bufferSize;
    int used;
    u8 *buffer;
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
int ScClient_Create(int gameId, ScClient **client);
int ScClient_Destroy(ScClient *client);
int ScClient_Update(ScClient *client);
// The IDs of the session and of the connection, and storing the session's
char *ScClient_GetCsid(ScClient *client);
char *ScClient_GetCcid(ScClient *client);
int ScClient_SetCsid(ScClient *client, const char *csid);
// The three requests, each returning 0 or an error code and calling a function when it has ended. The time they may
// take is not used
int ScClient_CreateSession(ScClient *client, AuthCertificate *certificate, AuthKeys *keys, ScClientFunc func,
                           int timeout, void *userData);
int ScClient_SetReportIntention(ScClient *client, const char *ccid, int authoritative, AuthCertificate *certificate,
                                AuthKeys *keys, ScClientFunc func, int timeout, void *userData);
int ScClient_SubmitReport(ScClient *client, ScReport *report, int authoritative, AuthCertificate *certificate,
                          AuthKeys *keys, ScClientFunc func, int timeout, void *userData);

// Makes a report of a version for players and teams, returning 0 or an error code
int ScClient_CreateReport(ScClient *client, int version, int playerNum, int teamNum, ScReport **report);
// Starts the records of the game, of the players and of the teams, and a group of them (one for each player or team)
int ScReport_BeginGame(ScReport *report);
int ScReport_BeginPlayers(ScReport *report);
int ScReport_BeginTeams(ScReport *report);
int ScReport_BeginGroup(ScReport *report);
// Sets the ID of a player, his key and his result; the profile ID of the certificate and the last two are given on but
// not used
int ScReport_SetPlayer(ScReport *report, int index, const char *id, int key, int result, int profileId,
                       const AuthCertificate *certificate, void *unused);
// Ends the header, with a flag and a value
int ScReport_Finish(ScReport *report, int flag, int value);
// Adds a record of an int, or of a byte, to the section that is being added to
int ScReport_AddInt(ScReport *report, int key, int value);
int ScReport_AddByte(ScReport *report, int key, int value);
// Frees the report
int ScReport_Free(ScReport *report);

#endif // POKEBW2_DWC_COMPETITION_H
