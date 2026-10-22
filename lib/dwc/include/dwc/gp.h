#ifndef POKEBW2_DWC_GP_H
#define POKEBW2_DWC_GP_H

#include "dwc/nonport.h"

// GameSpy's GP (Presence), in overlay 11: the API (gp.c) that DWC's friends and login code calls, which logs in to the
// connection manager (gpcm.gs.nintendowifi.net), keeps the buddy list and the statuses of the buddies, sends them
// messages directly (through gsUdpEngine.c's "Udp Engine") or through the server, and searches the search manager
// (gpsp.gs.nintendowifi.net). The ROM has no names for the files: they and their functions are named after GameSpy's,
// as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gp.s and the other gp*.s), or the later SDK's for the
// functions that the Wii's GP doesn't have, as some of this code's debug texts give them ("gpiSaveKeysToBuffer: buffer
// Out of memory.", "gpiProcessOperation was passed an operation with an invalid type (%d)\n").
//
// The functions return GameSpy's results (GPResult). The error codes of the server that the code checks are
// GP_LOGIN_PROFILE_DELETED and GP_NEWUSER_BAD_NICK (the error then gives the profile) and GP_LOGIN_CONNECTION_FAILED
// and GP_LOGIN_SERVER_AUTH_FAILED.

// GameSpy's constants of GP, results and server error codes, as the later SDK has them
typedef enum _GPEnum {
    GP_ERROR = 0,
    GP_RECV_BUDDY_REQUEST,
    GP_RECV_BUDDY_STATUS,
    GP_RECV_BUDDY_MESSAGE,
    GP_RECV_BUDDY_UTM,
    GP_RECV_GAME_INVITE,
    GP_TRANSFER_CALLBACK,
    GP_RECV_BUDDY_AUTH,
    GP_RECV_BUDDY_REVOKE,
    GP_INFO_CACHING = 0x0100,
    GP_SIMULATION,
    GP_INFO_CACHING_BUDDY_AND_BLOCK_ONLY,
    GP_BLOCKING = 1,
    GP_NON_BLOCKING = 0,
    GP_FIREWALL = 1,
    GP_NO_FIREWALL = 0,
    GP_CHECK_CACHE = 1,
    GP_DONT_CHECK_CACHE = 0,
    GP_VALID = 1,
    GP_INVALID = 0,
    GP_FATAL = 1,
    GP_NON_FATAL = 0,
    GP_MALE = 0x0500,
    GP_FEMALE,
    GP_PAT,
    GP_MORE = 0x0600,
    GP_DONE,
    GP_NICK = 0x0700,
    GP_UNIQUENICK,
    GP_EMAIL,
    GP_PASSWORD,
    GP_FIRSTNAME,
    GP_LASTNAME,
    GP_ICQUIN,
    GP_HOMEPAGE,
    GP_ZIPCODE,
    GP_COUNTRYCODE,
    GP_BIRTHDAY,
    GP_SEX,
    GP_CPUBRANDID,
    GP_CPUSPEED,
    GP_MEMORY,
    GP_VIDEOCARD1STRING,
    GP_VIDEOCARD1RAM,
    GP_VIDEOCARD2STRING,
    GP_VIDEOCARD2RAM,
    GP_CONNECTIONID,
    GP_CONNECTIONSPEED,
    GP_HASNETWORK,
    GP_OSSTRING,
    GP_AIMNAME,
    GP_PIC,
    GP_OCCUPATIONID,
    GP_INDUSTRYID,
    GP_INCOMEID,
    GP_MARRIEDID,
    GP_CHILDCOUNT,
    GP_INTERESTS1,
    GP_REPLACE = 1,
    GP_DONT_REPLACE = 0,
    GP_CONNECTED = 1,
    GP_NOT_CONNECTED = 0,
    GP_MASK_NONE = 0x00000000,
    GP_MASK_HOMEPAGE = 0x00000001,
    GP_MASK_ZIPCODE = 0x00000002,
    GP_MASK_COUNTRYCODE = 0x00000004,
    GP_MASK_BIRTHDAY = 0x00000008,
    GP_MASK_SEX = 0x00000010,
    GP_MASK_EMAIL = 0x00000020,
    GP_MASK_ALL = 0xFFFFFFFF,
    GP_OFFLINE = 0,
    GP_ONLINE = 1,
    GP_PLAYING = 2,
    GP_STAGING = 3,
    GP_CHATTING = 4,
    GP_AWAY = 5,
    GP_SESS_IS_CLOSED = 0x00000001,
    GP_SESS_IS_OPEN = 0x00000002,
    GP_SESS_HAS_PASSWORD = 0x00000004,
    GP_SESS_IS_BEHIND_NAT = 0x00000008,
    GP_SESS_IS_RANKED = 0x000000010,
    GP_INTEL = 1,
    GP_AMD,
    GP_CYRIX,
    GP_MOTOROLA,
    GP_ALPHA,
    GP_MODEM = 1,
    GP_ISDN,
    GP_CABLEMODEM,
    GP_DSL,
    GP_SATELLITE,
    GP_ETHERNET,
    GP_WIRELESS,
    GP_TRANSFER_SEND_REQUEST = 0x800,
    GP_TRANSFER_ACCEPTED,
    GP_TRANSFER_REJECTED,
    GP_TRANSFER_NOT_ACCEPTING,
    GP_TRANSFER_NO_CONNECTION,
    GP_TRANSFER_DONE,
    GP_TRANSFER_CANCELLED,
    GP_TRANSFER_LOST_CONNECTION,
    GP_TRANSFER_ERROR,
    GP_TRANSFER_THROTTLE,
    GP_FILE_BEGIN,
    GP_FILE_PROGRESS,
    GP_FILE_END,
    GP_FILE_DIRECTORY,
    GP_FILE_SKIP,
    GP_FILE_FAILED,
    GP_FILE_READ_ERROR = 0x900,
    GP_FILE_WRITE_ERROR,
    GP_FILE_DATA_ERROR,
    GP_TRANSFER_SENDER = 0xA00,
    GP_TRANSFER_RECEIVER,
    GP_DONT_ROUTE = 0xB00,
    GP_SILENCE_NONE = 0x00000000,
    GP_SILENCE_MESSAGES = 0x00000001,
    GP_SILENCE_UTMS = 0x00000002,
    GP_SILENCE_LIST = 0x00000004,
    GP_SILENCE_ALL = 0xFFFFFFFF,
    GP_NEW_STATUS_INFO_SUPPORTED = 0xC00,
    GP_NEW_STATUS_INFO_NOT_SUPPORTED = 0xC01
} GPEnum;

typedef enum _GPResult {
    GP_NO_ERROR,
    GP_MEMORY_ERROR,
    GP_PARAMETER_ERROR,
    GP_NETWORK_ERROR,
    GP_SERVER_ERROR,
    GP_MISC_ERROR,
    GP_COUNT
} GPResult;

typedef enum _GPErrorCode {
    GP_GENERAL = 0x0000,
    GP_PARSE,
    GP_NOT_LOGGED_IN,
    GP_BAD_SESSKEY,
    GP_DATABASE,
    GP_NETWORK,
    GP_FORCED_DISCONNECT,
    GP_CONNECTION_CLOSED,
    GP_UDP_LAYER,
    GP_LOGIN = 0x0100,
    GP_LOGIN_TIMEOUT,
    GP_LOGIN_BAD_NICK,
    GP_LOGIN_BAD_EMAIL,
    GP_LOGIN_BAD_PASSWORD,
    GP_LOGIN_BAD_PROFILE,
    GP_LOGIN_PROFILE_DELETED,
    GP_LOGIN_CONNECTION_FAILED,
    GP_LOGIN_SERVER_AUTH_FAILED,
    GP_LOGIN_BAD_UNIQUENICK,
    GP_LOGIN_BAD_PREAUTH,
    GP_NEWUSER = 0x0200,
    GP_NEWUSER_BAD_NICK,
    GP_NEWUSER_BAD_PASSWORD,
    GP_NEWUSER_UNIQUENICK_INVALID,
    GP_NEWUSER_UNIQUENICK_INUSE,
    GP_UPDATEUI = 0x0300,
    GP_UPDATEUI_BAD_EMAIL,
    GP_NEWPROFILE = 0x0400,
    GP_NEWPROFILE_BAD_NICK,
    GP_NEWPROFILE_BAD_OLD_NICK,
    GP_UPDATEPRO = 0x0500,
    GP_UPDATEPRO_BAD_NICK,
    GP_ADDBUDDY = 0x0600,
    GP_ADDBUDDY_BAD_FROM,
    GP_ADDBUDDY_BAD_NEW,
    GP_ADDBUDDY_ALREADY_BUDDY,
    GP_AUTHADD = 0x0700,
    GP_AUTHADD_BAD_FROM,
    GP_AUTHADD_BAD_SIG,
    GP_STATUS = 0x0800,
    GP_BM = 0x0900,
    GP_BM_NOT_BUDDY,
    GP_BM_EXT_INFO_NOT_SUPPORTED,
    GP_BM_BUDDY_OFFLINE,
    GP_GETPROFILE = 0x0A00,
    GP_GETPROFILE_BAD_PROFILE,
    GP_DELBUDDY = 0x0B00,
    GP_DELBUDDY_NOT_BUDDY,
    GP_DELPROFILE = 0x0C00,
    GP_DELPROFILE_LAST_PROFILE,
    GP_SEARCH = 0x0D00,
    GP_SEARCH_CONNECTION_FAILED,
    GP_SEARCH_TIMED_OUT,
    GP_CHECK = 0x0E00,
    GP_CHECK_BAD_EMAIL,
    GP_CHECK_BAD_NICK,
    GP_CHECK_BAD_PASSWORD,
    GP_REVOKE = 0x0F00,
    GP_REVOKE_NOT_BUDDY,
    GP_REGISTERUNIQUENICK = 0x1000,
    GP_REGISTERUNIQUENICK_TAKEN,
    GP_REGISTERUNIQUENICK_RESERVED,
    GP_REGISTERUNIQUENICK_BAD_NAMESPACE,
    GP_REGISTERCDKEY = 0x1100,
    GP_REGISTERCDKEY_BAD_KEY,
    GP_REGISTERCDKEY_ALREADY_SET,
    GP_REGISTERCDKEY_ALREADY_TAKEN,
    GP_ADDBLOCK = 0x1200,
    GP_ADDBLOCK_ALREADY_BLOCKED,
    GP_REMOVEBLOCK = 0x1300,
    GP_REMOVEBLOCK_NOT_BLOCKED
} GPErrorCode;

// A profile's ID, and a file transfer's
typedef int GPProfile;
typedef int GPTransfer;

// The lengths of GP's texts, with their ends
#define GP_NICK_LEN 31
#define GP_UNIQUENICK_LEN 21
#define GP_FIRSTNAME_LEN 31
#define GP_LASTNAME_LEN 31
#define GP_EMAIL_LEN 51
#define GP_PASSWORD_LEN 31
#define GP_HOMEPAGE_LEN 76
#define GP_ZIPCODE_LEN 11
#define GP_COUNTRYCODE_LEN 3
#define GP_PLACE_LEN 128
#define GP_AIMNAME_LEN 51
#define GP_REASON_LEN 1025
#define GP_STATUS_STRING_LEN 256
#define GP_LOCATION_STRING_LEN 256
#define GP_ERROR_STRING_LEN 256
#define GP_AUTHTOKEN_LEN 256
#define GP_PARTNERCHALLENGE_LEN 256
#define GP_CDKEY_LEN 65
#define GP_LOGIN_TICKET_LEN 25
#define GP_RICH_STATUS_LEN 256
#define GP_STATUS_BASIC_STR_LEN 33

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
    GPResult result;
    GPErrorCode errorCode;
    gsi_char *errorString;
    GPEnum fatal;
} GPErrorArg;

// The answer to a connect, with the profile that logged in
typedef struct GPConnectResponseArg {
    GPResult result;
    GPProfile profile;
    char uniquenick[21];
} GPConnectResponseArg;

// The answers to the operations that make or delete a profile, and that register a unique nick or a CD key
typedef struct GPNewProfileResponseArg {
    GPResult result;
    GPProfile profile;
} GPNewProfileResponseArg;

typedef struct GPDeleteProfileResponseArg {
    GPResult result;
    GPProfile profile;
} GPDeleteProfileResponseArg;

typedef struct GPRegisterUniqueNickResponseArg {
    GPResult result;
} GPRegisterUniqueNickResponseArg;

typedef struct GPRegisterCdKeyResponseArg {
    GPResult result;
} GPRegisterCdKeyResponseArg;

// The info of a profile, which gpGetInfo gives. sex is 0x500 (male), 0x501 (female) or 0x502 (unknown); birthday,
// birthmonth and birthyear are all 0 for no date
typedef struct GPGetInfoResponseArg {
    GPResult result;
    GPProfile profile;
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
    GPEnum sex;
    GPEnum publicmask;
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
    GPProfile profile;
    GPEnum status;
    char statusString[256];
    char locationString[256];
    unsigned int ip;
    int port;
    GPEnum quietModeFlags;
} GPBuddyStatus;

// A buddy message or UTM, a buddy request, the authorization of one, the revocation of a buddy, the status of a buddy
// changing (with its index in the buddy list) and a game invite, as the server or a peer sent them; date is in seconds
typedef struct GPRecvBuddyMessageArg {
    GPProfile profile;
    unsigned int date;
    char *message;
} GPRecvBuddyMessageArg;

typedef struct GPRecvBuddyUTMArg {
    GPProfile profile;
    unsigned int date;
    char *message;
} GPRecvBuddyUTMArg;

typedef struct GPRecvBuddyRequestArg {
    GPProfile profile;
    unsigned int date;
    char reason[1025];
} GPRecvBuddyRequestArg;

typedef struct GPRecvBuddyAuthArg {
    GPProfile profile;
    unsigned int date;
} GPRecvBuddyAuthArg;

typedef struct GPRecvBuddyRevokeArg {
    GPProfile profile;
    unsigned int date;
} GPRecvBuddyRevokeArg;

typedef struct GPRecvBuddyStatusArg {
    GPProfile profile;
    unsigned int date;
    int index;
} GPRecvBuddyStatusArg;

typedef struct GPRecvGameInviteArg {
    GPProfile profile;
    int productID;
    char location[256];
} GPRecvGameInviteArg;

// The keys and values of a buddy's status, which the buddy sent its peer
typedef struct GPGetBuddyStatusInfoKeysArg {
    GPProfile profile;
    char **keys;
    char **values;
    int numKeys;
} GPGetBuddyStatusInfoKeysArg;

// A file transfer's event, whose message the call frees
typedef struct GPTransferCallbackArg {
    GPTransfer transfer;
    GPEnum type;
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
    GPResult result;
    char email[51];
    GPEnum isValid;
} GPIsValidEmailResponseArg;

typedef struct GPCheckResponseArg {
    GPResult result;
    GPProfile profile;
} GPCheckResponseArg;

typedef struct GPNewUserResponseArg {
    GPResult result;
    GPProfile profile;
} GPNewUserResponseArg;

typedef struct GPProfileSearchMatch {
    GPProfile profile;
    char nick[31];
    char uniquenick[21];
    int namespaceID;
    char firstname[31];
    char lastname[31];
    char email[51];
} GPProfileSearchMatch;

typedef struct GPProfileSearchResponseArg {
    GPResult result;
    int numMatches;
    GPEnum more;
    GPProfileSearchMatch *matches;
} GPProfileSearchResponseArg;

typedef struct GPGetUserNicksResponseArg {
    GPResult result;
    char email[51];
    int numNicks;
    char **nicks;
    char **uniquenicks;
} GPGetUserNicksResponseArg;

typedef struct GPFindPlayerMatch {
    GPProfile profile;
    char nick[31];
    GPEnum status;
    char statusString[256];
} GPFindPlayerMatch;

typedef struct GPFindPlayersResponseArg {
    GPResult result;
    int productID;
    int numMatches;
    GPFindPlayerMatch *matches;
} GPFindPlayersResponseArg;

typedef struct GPGetReverseBuddiesResponseArg {
    GPResult result;
    int numProfiles;
    GPProfileSearchMatch *profiles;
} GPGetReverseBuddiesResponseArg;

typedef struct GPUniqueMatch {
    GPProfile profile;
    char uniqueNick[21];
} GPUniqueMatch;

typedef struct GPGetReverseBuddiesListResponseArg {
    GPResult result;
    int numOfUniqueMatches;
    GPUniqueMatch *matches;
} GPGetReverseBuddiesListResponseArg;

typedef struct GPSuggestUniqueNickResponseArg {
    GPResult result;
    int numSuggestedNicks;
    char **suggestedNicks;
} GPSuggestUniqueNickResponseArg;

// The answer of the "\profilelist\" search (not in GameSpy's GP; the name is ours): the profiles of a profile, unless
// it hides them
typedef struct GPGetProfileListResponseArg {
    GPResult result;
    GPProfile profile;
    gsi_bool hidden;
    int numProfiles;
    int *profiles;
} GPGetProfileListResponseArg;

GPResult gpInitialize(GPConnection *connection, int productID, int namespaceID, int partnerID);
void gpDestroy(GPConnection *connection);
GPResult gpProcess(GPConnection *connection);
GPResult gpSetCallback(GPConnection *connection, GPEnum func, GPCallback callback, void *param);
GPResult gpConnectPreAuthenticatedA(GPConnection *connection, const gsi_char *authtoken,
                                    const gsi_char *partnerchallenge, GPEnum firewall, GPEnum blocking,
                                    GPCallback callback, void *param);
void gpDisconnect(GPConnection *connection);
GPResult gpProfileSearchA(GPConnection *connection, const gsi_char *nick, const gsi_char *uniquenick,
                          const gsi_char *email, const gsi_char *firstname, const gsi_char *lastname, int icquin,
                          GPEnum blocking, GPCallback callback, void *param);
GPResult gpGetInfo(GPConnection *connection, GPProfile profile, GPEnum checkCache, GPEnum blocking, GPCallback callback,
                   void *param);
GPResult gpSetInfosA(GPConnection *connection, GPEnum info, const gsi_char *value);
GPResult gpSendBuddyRequestA(GPConnection *connection, GPProfile profile, const gsi_char *reason);
GPResult gpAuthBuddyRequest(GPConnection *connection, GPProfile profile);
GPResult gpDenyBuddyRequest(GPConnection *connection, GPProfile profile);
GPResult gpGetNumBuddies(GPConnection *connection, int *numBuddies);
GPResult gpGetBuddyStatus(GPConnection *connection, int index, GPBuddyStatus *status);
GPResult gpGetBuddyIndex(GPConnection *connection, GPProfile profile, int *index);
int gpIsBuddy(GPConnection *connection, GPProfile profile);
GPResult gpDeleteBuddy(GPConnection *connection, GPProfile profile);
GPResult gpSetStatusA(GPConnection *connection, GPEnum status, const gsi_char *statusString,
                      const gsi_char *locationString);
GPResult gpSendBuddyMessageA(GPConnection *connection, GPProfile profile, const gsi_char *message);
GPResult gpGetReverseBuddiesList(GPConnection *connection, GPProfile *targets, int numOfTargets, GPEnum blocking,
                                 GPCallback callback, void *param);
GPResult gpGetLoginTicket(GPConnection *connection, char loginTicket[GP_LOGIN_TICKET_LEN]);

#endif // POKEBW2_DWC_GP_H
