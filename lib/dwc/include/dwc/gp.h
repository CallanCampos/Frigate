#ifndef POKEBW2_DWC_GP_H
#define POKEBW2_DWC_GP_H

#include "types.h"

// GameSpy's GP (Presence), in overlay 11: the API (gp.c) that DWC's friends and login code calls, which logs in to the
// connection manager (gpcm.gs.nintendowifi.net), keeps the buddy list and the statuses of the buddies, sends them
// messages directly (through gt2.c's "Udp Engine") or through the server, and searches the search manager
// (gpsp.gs.nintendowifi.net). The ROM has no names for the files: they and their functions are named after GameSpy's,
// as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gp.s and the other gp*.s), or the later SDK's for the
// functions that the Wii's GP doesn't have, as some of this code's debug texts give them ("gpiSaveKeysToBuffer: buffer
// Out of memory.", "gpiProcessOperation was passed an operation with an invalid type (%d)\n").
//
// The functions return GameSpy's results: 0 (no error), 1 (out of memory), 2 (a bad parameter), 3 (a network error), 4
// (an error the server sent) and 5 (any other). The error codes of the server that the code checks are 0x106 and 0x201
// (the profile of the login is gone, or the nick is in use: the error then gives the profile) and 0x107 and 0x108 (the
// server refused the connection, or could not be authenticated).

// A connection: the pointer to the state of the connection (GPIConnection, dwc/gpi.h) that gpInitialize makes
typedef void *GPConnection;

// A callback of the app, with the argument of its kind (below) and the app's parameter
typedef void (*GPCallback)(GPConnection *connection, void *arg, void *param);

// The kinds of callback that gpSetCallback sets, the indexes of GPIConnection's callbacks: errors, buddy requests, the
// status of a buddy, a buddy message, a buddy UTM (an untyped message), a game invite, file transfers, the
// authorization of a buddy request and the revocation of a buddy
#define GPI_NUM_CALLBACKS 9

// An error, which GPIConnection's errorString and errorCode keep; fatal ones end the connection
typedef struct GPErrorArg {
    int result;
    int errorCode;
    char *errorString;
    BOOL fatal;
} GPErrorArg;

// The answer to a connect, with the profile that logged in
typedef struct GPConnectResponseArg {
    int result;
    int profile;
    char uniquenick[21];
} GPConnectResponseArg;

// The answers to the operations that make or delete a profile, and that register a unique nick or a CD key
typedef struct GPNewProfileResponseArg {
    int result;
    int profile;
} GPNewProfileResponseArg;

typedef struct GPDeleteProfileResponseArg {
    int result;
    int profile;
} GPDeleteProfileResponseArg;

typedef struct GPRegisterUniqueNickResponseArg {
    int result;
} GPRegisterUniqueNickResponseArg;

typedef struct GPRegisterCdKeyResponseArg {
    int result;
} GPRegisterCdKeyResponseArg;

// The info of a profile, which gpGetInfo gives. sex is 0x500 (male), 0x501 (female) or 0x502 (unknown); birthday,
// birthmonth and birthyear are all 0 for no date
typedef struct GPGetInfoResponseArg {
    int result;
    int profile;
    char nick[31];
    char uniquenick[21];
    char email[51];
    char firstname[31];
    char lastname[31];
    char homepage[76];
    int icquin;
    char zipcode[11];
    char countrycode[3];
    float longitude;
    float latitude;
    char place[128];
    int birthday;
    int birthmonth;
    int birthyear;
    int sex;
    int publicmask;
    char aimname[51];
    int pic;
    int occupationid;
    int industryid;
    int incomeid;
    int marriedid;
    int childcount;
    int interests1;
    int ownership1;
    int conntypeid;
} GPGetInfoResponseArg;

// The status of a buddy, which gpGetBuddyStatus gives
typedef struct GPBuddyStatus {
    int profile;
    int status;
    char statusString[256];
    char locationString[256];
    u32 ip;
    int port;
    int quietModeFlags;
} GPBuddyStatus;

// A buddy message or UTM, a buddy request, the authorization of one, the revocation of a buddy, the status of a buddy
// changing (with its index in the buddy list) and a game invite, as the server or a peer sent them; date is in seconds
typedef struct GPRecvBuddyMessageArg {
    int profile;
    u32 date;
    char *message;
} GPRecvBuddyMessageArg;

typedef struct GPRecvBuddyUTMArg {
    int profile;
    u32 date;
    char *message;
} GPRecvBuddyUTMArg;

typedef struct GPRecvBuddyRequestArg {
    int profile;
    u32 date;
    char reason[1025];
} GPRecvBuddyRequestArg;

typedef struct GPRecvBuddyAuthArg {
    int profile;
    u32 date;
} GPRecvBuddyAuthArg;

typedef struct GPRecvBuddyRevokeArg {
    int profile;
    u32 date;
} GPRecvBuddyRevokeArg;

typedef struct GPRecvBuddyStatusArg {
    int profile;
    u32 date;
    int index;
} GPRecvBuddyStatusArg;

typedef struct GPRecvGameInviteArg {
    int profile;
    int productID;
    char location[256];
} GPRecvGameInviteArg;

// The keys and values of a buddy's status, which the buddy sent its peer
typedef struct GPGetBuddyStatusInfoKeysArg {
    int profile;
    char **keys;
    char **values;
    int numKeys;
} GPGetBuddyStatusInfoKeysArg;

// A file transfer's event, whose message the call frees
typedef struct GPTransferCallbackArg {
    int transfer;
    int type;
    int index;
    int num;
    char *message;
} GPTransferCallbackArg;

// The answers of the search manager: profiles found by their info, the nicks of an e-mail, the players of a product,
// the profiles that have this one as a buddy (with their info, or only their unique nicks), unique nicks to suggest,
// and the list of the profiles of a profile; more is GP_MORE when more matches can be asked for and GP_DONE when not.
// The searches that check an e-mail, a nick with an e-mail (and password) and make a new user answer with the result
// and the profile, or whether the e-mail is valid
#define GP_MORE 0x600
#define GP_DONE 0x601

typedef struct GPIsValidEmailResponseArg {
    int result;
    char email[51];
    BOOL isValid;
} GPIsValidEmailResponseArg;

typedef struct GPCheckResponseArg {
    int result;
    int profile;
} GPCheckResponseArg;

typedef struct GPNewUserResponseArg {
    int result;
    int profile;
} GPNewUserResponseArg;

typedef struct GPProfileSearchMatch {
    int profile;
    char nick[31];
    char uniquenick[21];
    int namespaceID;
    char firstname[31];
    char lastname[31];
    char email[51];
} GPProfileSearchMatch;

typedef struct GPProfileSearchResponseArg {
    int result;
    int numMatches;
    int more;
    GPProfileSearchMatch *matches;
} GPProfileSearchResponseArg;

typedef struct GPGetUserNicksResponseArg {
    int result;
    char email[51];
    int numNicks;
    char **nicks;
    char **uniquenicks;
} GPGetUserNicksResponseArg;

typedef struct GPFindPlayerMatch {
    int profile;
    char nick[31];
    int playerStatus;
    char statusString[256];
} GPFindPlayerMatch;

typedef struct GPFindPlayersResponseArg {
    int result;
    int productID;
    int numMatches;
    GPFindPlayerMatch *matches;
} GPFindPlayersResponseArg;

typedef struct GPGetReverseBuddiesResponseArg {
    int result;
    int numProfiles;
    GPProfileSearchMatch *profiles;
} GPGetReverseBuddiesResponseArg;

typedef struct GPUniqueMatch {
    int profile;
    char uniqueNick[21];
} GPUniqueMatch;

typedef struct GPGetReverseBuddiesListResponseArg {
    int result;
    int numOfUniqueMatchs;
    GPUniqueMatch *matches;
} GPGetReverseBuddiesListResponseArg;

typedef struct GPSuggestUniqueNickResponseArg {
    int result;
    int numSuggestedNicks;
    char **suggestedNicks;
} GPSuggestUniqueNickResponseArg;

// The answer of the "\profilelist\" search (not in GameSpy's GP; the name is ours): the profiles of a profile, unless
// it hides them
typedef struct GPGetProfileListResponseArg {
    int result;
    int profile;
    BOOL hidden;
    int numProfiles;
    int *profiles;
} GPGetProfileListResponseArg;

// The texts of a login ticket
typedef struct GPLoginTicket {
    char ticket[25];
} GPLoginTicket;

int gpInitialize(GPConnection *connection, int productID, int namespaceID, int partnerID);
void gpDestroy(GPConnection *connection);
int gpProcess(GPConnection *connection);
int gpSetCallback(GPConnection *connection, int func, GPCallback callback, void *param);
int gpConnectPreAuthenticated(GPConnection *connection, const char *authtoken, const char *partnerchallenge,
                              int firewall, int blocking, GPCallback callback, void *param);
void gpDisconnect(GPConnection *connection);
int gpProfileSearch(GPConnection *connection, const char *nick, const char *uniquenick, const char *email,
                    const char *firstname, const char *lastname, int icquin, int blocking, GPCallback callback,
                    void *param);
int gpGetInfo(GPConnection *connection, int profile, int checkCache, int blocking, GPCallback callback, void *param);
int gpSetInfos(GPConnection *connection, int info, const char *value);
int gpSendBuddyRequest(GPConnection *connection, int profile, const char *reason);
int gpAuthBuddyRequest(GPConnection *connection, int profile);
int gpDenyBuddyRequest(GPConnection *connection, int profile);
int gpGetNumBuddies(GPConnection *connection, int *numBuddies);
int gpGetBuddyStatus(GPConnection *connection, int index, GPBuddyStatus *status);
int gpGetBuddyIndex(GPConnection *connection, int profile, int *index);
BOOL gpIsBuddy(GPConnection *connection, int profile);
int gpDeleteBuddy(GPConnection *connection, int profile);
int gpSetStatus(GPConnection *connection, int status, const char *statusString, const char *locationString);
int gpSendBuddyMessage(GPConnection *connection, int profile, const char *message);
int gpGetReverseBuddiesList(GPConnection *connection, int *targets, int numOfTargets, int blocking, GPCallback callback,
                            void *param);
int gpGetLoginTicket(GPConnection *connection, GPLoginTicket *loginTicket);

#endif // POKEBW2_DWC_GP_H
