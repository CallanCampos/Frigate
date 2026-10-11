#ifndef POKEBW2_DWC_GAMESTATS_H
#define POKEBW2_DWC_GAMESTATS_H

#include "types.h"

// The client of Nintendo's game statistics server ("http://gamestats2.gs.nintendowifi.net/"), in overlay 189, which the
// Global Trade Station and the Battle Subway's Wi-Fi library (dpw) talk to the server with. The ROM names none of it,
// so the names say what the code does.
//
// A call sends a page of the server with the player's ID and a payload in base64, in two requests: the first one
// answers with a challenge of 32 characters, and the second carries the hash of the game's secret and the challenge.
// The answer is checked against its own hash before it is kept.

// The servers of the game statistics: the live one, and two for development
#define GAMESTATS_SERVER_LIVE 0
#define GAMESTATS_SERVER_DEVELOPMENT 1
#define GAMESTATS_SERVER_TEST 2

// The states that gamestats_Poll gives: a call failed, or ended with its answer kept
#define GAMESTATS_STATE_FAILED 1
#define GAMESTATS_STATE_DONE 8

// Called with the answer before it is kept; FALSE refuses it
typedef BOOL (*GamestatsCheckFunc)(const void *data, int size);

// Sets up the server (one of the GAMESTATS_SERVER_ constants) and the game's key: "key" is the game's secret of 20
// characters, four numbers of 8 hexadecimal digits that give the generator of the payload's scrambling, and the name
// of the game
void Gamestats_Start(int server, const char *key);
// Runs the call and returns its state
int Gamestats_Poll(void);
// Ends the call that is running
void Gamestats_Abort(void);
// Ends the library's use of the HTTP client and frees the call's memory
void Gamestats_Close(void);
// The answer of the last call and its size
void *Gamestats_GetBody(int *size);
// Sends the page with the player's ID and the payload; 0 when it started, 2 when it ran out of memory, 3 when the
// game's key was not set
int Gamestats_Send(const char *page, int pid, const void *payload, int size, GamestatsCheckFunc check);

#endif // POKEBW2_DWC_GAMESTATS_H
