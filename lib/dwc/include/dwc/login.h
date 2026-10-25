#ifndef POKEBW2_DWC_LOGIN_H
#define POKEBW2_DWC_LOGIN_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/error.h"
#include "dwc/gp.h"
#include "nitro/os.h"

// DWC's login to GameSpy's GP (overlay 11's dwc_login.c), with the token of the authentication server. The functions
// are named after the SDK's (see dwc_login.c), and so is DWCLoginState; the other names are ours, from their use.
// dwc/ext.h declares the login's token and ticket.

// The callback of the login, with its error and the profile ID that logged in
typedef void (*DWCLoginCallback)(DWCError error, int profileId, void *param);

// The login's states: the SDK's DWCLoginState
typedef enum DWCLoginState {
    DWC_LOGIN_STATE_INIT = 0,
    DWC_LOGIN_STATE_REMOTE_AUTH,
    DWC_LOGIN_STATE_CONNECTING,
    DWC_LOGIN_STATE_GPGETINFO,
    DWC_LOGIN_STATE_GPSETINFO,
    DWC_LOGIN_STATE_CONNECTED,
    DWC_LOGIN_STATE_NUM,
} DWCLoginState;

// The login's state, which dwc_match.c gives the memory for
typedef struct DWCLoginControl {
    GPConnection *pGpObj;
    // 0 before the login, 1 while the authentication server is asked, 2 to 4 while GP logs in (with the user data's
    // profile, or looking for it with the login ID) and 5 once logged in
    DWCLoginState state;
    int productID;
    u32 gamecode;
    const u16 *playerName;
    DWCLoginCallback callback;
    void *param;
    DWCUserData *userdata;
    // The SDK's work of the backup manager, HTTP client and start of the login, which this version leaves unused
    void *bmwork;
    void *http;
    OSTick startTick;
    // Whether GP is being connected to, and since when
    u32 connectFlag;
    OSTick connectTick;
    // The login ID that the login uses until the user data has an authenticated one
    DWCstAccLoginId tempLoginId;
    // What the authentication server gave for GP's login
    char authToken[0x100];
    char partnerChallenge[0x100];
    // The login name: the user ID, the game code and the player ID as text
    char username[21];
} DWCLoginControl;

// The login: starting it with its memory, starting the login to the authentication server, running it for a frame, the
// user data, ending it with an error, and ending it
void DWCi_LoginInit(DWCLoginControl *control, DWCUserData *userData, GPConnection *connection, int productId,
                    u32 gameCode, const u16 *playerName, DWCLoginCallback callback, void *param);
BOOL DWCi_LoginAsync(void);
void DWCi_LoginProcess(void);
DWCUserData *DWCi_GetUserData(void);
void DWCi_StopLogin(DWCError error, int code);
void DWCi_ShutdownLogin(void);
// Whether the player is logged in to GP
BOOL DWCi_CheckLogin(void);

#endif // POKEBW2_DWC_LOGIN_H
