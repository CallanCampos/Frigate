#ifndef POKEBW2_DWC_SC_H
#define POKEBW2_DWC_SC_H

#include "types.h"
#include "dwc/auth.h"
#include "dwc/competition.h"

// The Wi-Fi library's client of the competition service in overlay 260 (dwc_sc.c), which wrote its debug texts as
// "Sc[%d]> ". It logs in to the authentication service, keeps the certificate and the keys it gets, and makes the
// competition client's requests with them. Its errors are the Wi-Fi library's, set with a type and a code made from the
// step that failed

// Called with the error (0 if none) and the user data when a request has ended
typedef void (*ScResultFunc)(int error, void *userData);

// Starts the library for a game: sets up the servers of a kind (1 for the second kind of secure servers), logs in
// with the token that the Wi-Fi library has and makes the competition client, returning 0 or 7 on failure
int Sc_Startup(int gameId, int server);
// Ends the client and the library's work, returning 0 or the client's error
int Sc_Shutdown(void);
// Runs the client for a frame
int Sc_Update(void);
// The requests: to make a session, to tell that the player will report on it (authoritatively or not), and to submit
// a report. Each returns 0 or an error code and calls a function with the error when it has ended
int Sc_CreateSession(ScResultFunc func, int timeout, void *userData);
int Sc_SetReportIntention(int authoritative, ScResultFunc func, int timeout, void *userData);
int Sc_SubmitReport(ScReport *report, int authoritative, ScResultFunc func, int timeout, void *userData);
// Takes the ID of the session from the other player, and gives the IDs of the session and of the connection
int Sc_SetSessionId(const char *csid);
char *Sc_GetSessionId(void);
char *Sc_GetConnectionId(void);
// Makes a report of a version for a number of players and of teams
int Sc_CreateReport(int version, int playerNum, int teamNum, ScReport **report);
// The steps of the report's building, each returning 0 or an error code: the sections of the game's records, of the
// players' and of the teams', a group of records, a player, the end of the header and the records
int Sc_BeginGame(ScReport *report);
int Sc_BeginPlayers(ScReport *report);
int Sc_BeginTeams(ScReport *report);
int Sc_BeginGroup(ScReport *report);
int Sc_SetPlayer(ScReport *report, int index, const char *id, int key, int result, const AuthCertificate *certificate,
                 void *unused);
int Sc_FinishReport(ScReport *report, int flag, int value);
int Sc_AddInt(ScReport *report, int key, int value);
int Sc_AddByte(ScReport *report, int key, int value);
int Sc_FreeReport(ScReport *report);
// Gives the player's certificate
int Sc_GetCertificate(const AuthCertificate **certificate);

#endif // POKEBW2_DWC_SC_H
