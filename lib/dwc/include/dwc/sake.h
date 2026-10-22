#ifndef POKEBW2_DWC_SAKE_H
#define POKEBW2_DWC_SAKE_H

#include "dwc/nonport.h"

// GameSpy's Sake, the online record storage that Nintendo's Wi-Fi Connection games keep their ranking data in, in
// overlay 260 (sake.c). It posts SOAP requests to a game's tables on the storage server and calls a function back with
// the answer. The ROM names five of its calls in the texts of gdb.c ("sakeStartup failed", "sakeCreateRecord failed"
// and the likewise for "sakeUpdateRecord", "sakeGetMyRecords" and "sakeGetSpecificRecords"); the rest are named by
// what they do. The ROM's texts name the types of a field's value (`byteValue`, `asciiStringValue`...) and the
// requests (`CreateRecord`, `SearchForRecords`...)

typedef struct SAKEInternal *SAKE;
typedef struct SAKERequestInternal *SAKERequest;

// What sakeStartup returns
typedef enum {
    SAKEStartupResult_SUCCESS,
    SAKEStartupResult_NOT_AVAILABLE,
    SAKEStartupResult_CORE_SHUTDOWN,
    SAKEStartupResult_OUT_OF_MEMORY
} SAKEStartupResult;

// Why a request could not start
typedef enum {
    SAKEStartRequestResult_SUCCESS,
    SAKEStartRequestResult_NOT_AUTHENTICATED,
    SAKEStartRequestResult_OUT_OF_MEMORY,
    SAKEStartRequestResult_BAD_INPUT,
    SAKEStartRequestResult_BAD_TABLEID,
    SAKEStartRequestResult_BAD_FIELDS,
    SAKEStartRequestResult_BAD_NUM_FIELDS,
    SAKEStartRequestResult_BAD_FIELD_NAME,
    SAKEStartRequestResult_BAD_FIELD_TYPE,
    SAKEStartRequestResult_BAD_FIELD_VALUE,
    SAKEStartRequestResult_BAD_OFFSET,
    SAKEStartRequestResult_BAD_MAX,
    SAKEStartRequestResult_BAD_RECORDIDS,
    SAKEStartRequestResult_BAD_NUM_RECORDIDS,
    SAKEStartRequestResult_UNKNOWN_ERROR
} SAKEStartRequestResult;

// The type of a record field's value, as its element is named in the messages
typedef enum {
    SAKEFieldType_BYTE,
    SAKEFieldType_SHORT,
    SAKEFieldType_INT,
    SAKEFieldType_FLOAT,
    SAKEFieldType_ASCII_STRING,
    SAKEFieldType_UNICODE_STRING,
    SAKEFieldType_BOOLEAN,
    SAKEFieldType_DATE_AND_TIME,
    SAKEFieldType_BINARY_DATA,
    SAKEFieldType_INT64,
    SAKEFieldType_NUM_FIELD_TYPES,
} SAKEFieldType;

// Binary data: its bytes and their count
typedef struct {
    gsi_u8 *data;
    int length;
} SAKEBinaryData;

// The value of a field, of its type
typedef union {
    gsi_u8 byteValue;
    gsi_i16 shortValue;
    gsi_i32 intValue;
    float floatValue;
    char *asciiStringValue;
    gsi_u16 *unicodeStringValue;
    gsi_bool booleanValue;
    int dateAndTimeValue;
    SAKEBinaryData binaryDataValue;
    gsi_i64 int64Value;
} SAKEValue;

// A field of a record: its name, its type and its value
typedef struct {
    char *name;
    SAKEFieldType type;
    SAKEValue value;
} SAKEField;

// What a request is told, one for each request. A table is named by its ID; the fields of a record to write by
// SAKEField, and the fields to read by their names
typedef struct {
    char *tableId;
    SAKEField *fields;
    int fieldNum;
} SAKECreateRecordInput;

typedef struct {
    char *tableId;
    int recordId;
    SAKEField *fields;
    int fieldNum;
} SAKEUpdateRecordInput;

typedef struct {
    char *tableId;
    char **fieldNames;
    int fieldNum;
} SAKEGetMyRecordsInput;

typedef struct {
    char *tableId;
    int *recordIds;
    int recordNum;
    char **fieldNames;
    int fieldNum;
} SAKEGetSpecificRecordsInput;

// The answers: the new record's ID for a created record, and the records that were read, each an array of fieldNum
// fields and all freed with the request
typedef struct {
    int recordId;
} SAKECreateRecordOutput;

typedef struct {
    int recordNum;
    SAKEField **records;
} SAKESearchForRecordsOutput;

typedef struct {
    int recordNum;
    SAKEField **records;
} SAKEGetMyRecordsOutput;

typedef struct {
    int recordNum;
    SAKEField **records;
} SAKEGetSpecificRecordsOutput;

// What the callback is given: sake_error.c turns the server's answer text into SUCCESS to TARGET_FILTER_INVALID or
// UNKNOWN_ERROR, and the HTTP client's result into SUCCESS, OUT_OF_MEMORY, CANCELED or CONNECTION_ERROR. CANCELED is
// BW2's own, after GameSpy's results; its name is ours
typedef enum {
    SAKERequestResult_SUCCESS,
    SAKERequestResult_SECRET_KEY_INVALID,
    SAKERequestResult_SERVICE_DISABLED,
    SAKERequestResult_CONNECTION_TIMEOUT,
    SAKERequestResult_CONNECTION_ERROR,
    SAKERequestResult_MALFORMED_RESPONSE,
    SAKERequestResult_OUT_OF_MEMORY,
    SAKERequestResult_DATABASE_UNAVAILABLE,
    SAKERequestResult_LOGIN_TICKET_INVALID,
    SAKERequestResult_LOGIN_TICKET_EXPIRED,
    SAKERequestResult_TABLE_NOT_FOUND,
    SAKERequestResult_RECORD_NOT_FOUND,
    SAKERequestResult_FIELD_NOT_FOUND,
    SAKERequestResult_FIELD_TYPE_INVALID,
    SAKERequestResult_NO_PERMISSION,
    SAKERequestResult_RECORD_LIMIT_REACHED,
    SAKERequestResult_ALREADY_RATED,
    SAKERequestResult_NOT_RATEABLE,
    SAKERequestResult_NOT_OWNED,
    SAKERequestResult_FILTER_INVALID,
    SAKERequestResult_SORT_INVALID,
    SAKERequestResult_TARGET_FILTER_INVALID,
    SAKERequestResult_UNKNOWN_ERROR,
    SAKERequestResult_CANCELED
} SAKERequestResult;

// Called when a request has ended, with the Sake, the request, the error (0 if none), what the request was given,
// the answer (0 on an error) and the user data
typedef void (*SAKERequestCallback)(SAKE sake, SAKERequest request, SAKERequestResult result, void *input, void *output,
                                    void *userData);

// The login ticket of a player's profile, which the Wi-Fi library gives
typedef struct SakeLoginTicket {
    char data[25];
} SakeLoginTicket;

// The Sake: set up for a game, and with the login ticket of the player's profile. The URLs of the file server are
// the library's, not the Sake's
struct SAKEInternal {
    gsi_bool hasGame;
    char gameName[16];
    int gameId;
    char secretKey[12];
    gsi_bool hasTicket;
    int profileId;
    SakeLoginTicket loginTicket;
    SAKEStartRequestResult lastError;
};

// The URL of the storage server, which the Wi-Fi library built (gdb.c)
extern char gSakeStorageUrl[128];

// Makes a Sake, returning 0 or an error code. It needs the Wi-Fi library to be running and not busy
SAKEStartupResult sakeStartup(SAKE *sake);
// Frees a Sake
void Sake_Free(SAKE sake);
// Sets the game's name, ID and secret key, and the profile and login ticket of the player
void Sake_SetGame(SAKE sake, const char *gameName, int gameId, const char *secretKey);
void Sake_SetLoginTicket(SAKE sake, int profileId, const SakeLoginTicket *loginTicket);
// The error that the last call left
SAKEStartRequestResult Sake_GetLastError(SAKE sake);
// Starts a request, returning it or 0 with the error in the Sake
SAKERequest sakeCreateRecord(SAKE sake, SAKECreateRecordInput *input, SAKERequestCallback callback, void *userData);
SAKERequest sakeUpdateRecord(SAKE sake, SAKEUpdateRecordInput *input, SAKERequestCallback callback, void *userData);
SAKERequest sakeGetMyRecords(SAKE sake, SAKEGetMyRecordsInput *input, SAKERequestCallback callback, void *userData);
SAKERequest sakeGetSpecificRecords(SAKE sake, SAKEGetSpecificRecordsInput *input, SAKERequestCallback callback,
                                   void *userData);
// Sets the URLs of the file server in place of the standard, and gets those of a file to download and to upload
gsi_bool Sake_SetDownloadUrl(SAKE sake, const char *url);
gsi_bool Sake_GetDownloadUrl(SAKE sake, int fileId, char *url);
gsi_bool Sake_SetUploadUrl(SAKE sake, const char *url);
gsi_bool Sake_GetUploadUrl(SAKE sake, char *url);
// Stops a request, returning 0 or an error code
int Sake_CancelRequest(SAKE sake, SAKERequest request);

#endif // POKEBW2_DWC_SAKE_H
