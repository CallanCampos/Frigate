#ifndef POKEBW2_DWC_DWC_ENCSESSION_H
#define POKEBW2_DWC_DWC_ENCSESSION_H

#include "types.h"

// DWC's encrypted session (dwc_encsession.c, libdwcenc), the client of Nintendo's game statistics server
// ("http://gamestats2.gs.nintendowifi.net/"), in overlay 189, which the Global Trade Station and the Battle Subway's
// Wi-Fi library (dpw) talk to the server with. The ROM names none of it: the functions have the SDK's names that Black
// 1's decompilation (aletheia) gives the same functions of libdwcenc.TWL.HYB.a's dwc_encsession.o, and the constants
// and the callback's type are ours.
//
// A call sends a page of the server with the player's ID and a payload in base64, in two requests: the first one
// answers with a challenge of 32 characters, and the second carries the hash of the game's secret and the challenge.
// The answer is checked against its own hash before it is kept.

// The servers of the game statistics: the live one, and two for development
#define GAMESTATS_SERVER_LIVE 0
#define GAMESTATS_SERVER_DEVELOPMENT 1
#define GAMESTATS_SERVER_TEST 2

// The states that DWCi_EncSessionProcess gives: a call failed, or ended with its answer kept
#define GAMESTATS_STATE_FAILED 1
#define GAMESTATS_STATE_DONE 8

// Called with the answer before it is kept; FALSE refuses it
typedef BOOL (*GamestatsCheckFunc)(const void *data, int size);

// Sets up the server (one of the GAMESTATS_SERVER_ constants) and the game's key: "key" is the game's secret of 20
// characters, four numbers of 8 hexadecimal digits that give the generator of the payload's scrambling, and the name
// of the game
void DWCi_EncSessionInitialize(int server, const char *key);
// Runs the call and returns its state
int DWCi_EncSessionProcess(void);
// Ends the call that is running
void DWCi_EncSessionCancel(void);
// Ends the library's use of the HTTP client and frees the call's memory
void DWCi_EncSessionShutdown(void);
// The answer of the last call and its size
void *DWCi_EncSessionGetResponse(int *size);
// Sends the page with the player's ID and the payload; 0 when it started, 2 when it ran out of memory, 3 when the
// game's key was not set
int DWCi_EncSessionGetAsync(const char *page, int pid, const void *payload, int size, GamestatsCheckFunc check);

#endif // POKEBW2_DWC_DWC_ENCSESSION_H
