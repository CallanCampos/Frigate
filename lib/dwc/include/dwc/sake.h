#ifndef POKEBW2_DWC_SAKE_H
#define POKEBW2_DWC_SAKE_H

#include "types.h"

// GameSpy's Sake, the online record storage that Nintendo's Wi-Fi Connection games keep their ranking data in, in
// overlay 260 (sake.c). It posts SOAP requests to a game's tables on the storage server and calls a function back with
// the answer. The ROM names five of its calls in the texts of gdb.c ("sakeStartup failed", "sakeCreateRecord failed"
// and the likewise for "sakeUpdateRecord", "sakeGetMyRecords" and "sakeGetSpecificRecords"); the rest are named by
// what they do. The ROM's texts name the types of a field's value (`byteValue`, `asciiStringValue`...) and the
// requests (`CreateRecord`, `SearchForRecords`...)

typedef struct Sake Sake;
typedef struct SakeRequest SakeRequest;

// The type of a record field's value, as its element is named in the messages
typedef enum SakeFieldType {
    SAKE_FIELD_BYTE,
    SAKE_FIELD_SHORT,
    SAKE_FIELD_INT,
    SAKE_FIELD_FLOAT,
    SAKE_FIELD_ASCII_STRING,
    SAKE_FIELD_UNICODE_STRING,
    SAKE_FIELD_BOOLEAN,
    SAKE_FIELD_DATE_AND_TIME,
    SAKE_FIELD_BINARY_DATA,
    SAKE_FIELD_INT64,
    SAKE_FIELD_TYPE_NUM,
} SakeFieldType;

// A field of a record: its name, its type and its value
typedef struct SakeField {
    const char *name;
    int type;
    union {
        u8 byteValue;
        s16 shortValue;
        int intValue;
        float floatValue;
        const char *asciiStringValue;
        u16 *unicodeStringValue;
        BOOL booleanValue;
        int dateAndTimeValue;
        struct {
            void *data;
            int length;
        } binaryDataValue;
        s64 int64Value;
    } value;
} SakeField;

// What a request is told, one for each request. A table is named by its ID; the fields of a record to write by
// SakeField, and the fields to read by their names
typedef struct SakeCreateRecordInput {
    const char *tableId;
    SakeField *fields;
    int fieldNum;
} SakeCreateRecordInput;

typedef struct SakeUpdateRecordInput {
    const char *tableId;
    int recordId;
    SakeField *fields;
    int fieldNum;
} SakeUpdateRecordInput;

typedef struct SakeGetMyRecordsInput {
    const char *tableId;
    const char **fieldNames;
    int fieldNum;
} SakeGetMyRecordsInput;

typedef struct SakeGetSpecificRecordsInput {
    const char *tableId;
    int *recordIds;
    int recordNum;
    const char **fieldNames;
    int fieldNum;
} SakeGetSpecificRecordsInput;

// The answers: the new record's ID for a created record, and the records that were read, each an array of fieldNum
// fields and all freed with the request
typedef struct SakeGetRecordsOutput {
    int recordNum;
    SakeField **records;
} SakeGetRecordsOutput;

// What the callback is given as the error. The values come from the asm: sake_error.c turns the server's answer text
// into 0 and 1, 2, 7 to 21 (SUCCESS to TARGET_FILTER_INVALID, named as the ROM's texts name them) or UNKNOWN, and the
// HTTP client's status into 0, OUT_OF_MEMORY, CANCELED or CONNECTION_ERROR. The names that are not the server's
// (CONNECTION_ERROR, MALFORMED_RESPONSE, OUT_OF_MEMORY, UNKNOWN, CANCELED) are descriptive, from what returns them
typedef enum SakeResult {
    SAKE_RESULT_SUCCESS = 0,
    SAKE_RESULT_SECRET_KEY_INVALID = 1,
    SAKE_RESULT_SERVICE_DISABLED = 2,
    SAKE_RESULT_CONNECTION_ERROR = 4,
    SAKE_RESULT_MALFORMED_RESPONSE = 5,
    SAKE_RESULT_OUT_OF_MEMORY = 6,
    SAKE_RESULT_DATABASE_UNAVAILABLE = 7,
    SAKE_RESULT_LOGIN_TICKET_INVALID = 8,
    SAKE_RESULT_LOGIN_TICKET_EXPIRED = 9,
    SAKE_RESULT_TABLE_NOT_FOUND = 10,
    SAKE_RESULT_RECORD_NOT_FOUND = 11,
    SAKE_RESULT_FIELD_NOT_FOUND = 12,
    SAKE_RESULT_FIELD_TYPE_INVALID = 13,
    SAKE_RESULT_NO_PERMISSION = 14,
    SAKE_RESULT_RECORD_LIMIT_REACHED = 15,
    SAKE_RESULT_ALREADY_RATED = 16,
    SAKE_RESULT_NOT_RATEABLE = 17,
    SAKE_RESULT_NOT_OWNED = 18,
    SAKE_RESULT_FILTER_INVALID = 19,
    SAKE_RESULT_SORT_INVALID = 20,
    SAKE_RESULT_TARGET_FILTER_INVALID = 21,
    SAKE_RESULT_UNKNOWN = 22,
    SAKE_RESULT_CANCELED = 23,
} SakeResult;

// Called when a request has ended, with the Sake, the request, the error (0 if none), what the request was given,
// the answer (0 on an error) and the user data
typedef void (*SakeCallback)(Sake *sake, SakeRequest *request, int error, void *input, void *output, void *userData);

// The login ticket of a player's profile, which the Wi-Fi library gives
typedef struct SakeLoginTicket {
    char data[25];
} SakeLoginTicket;

// The Sake: set up for a game, and with the login ticket of the player's profile. The URLs of the file server are
// the library's, not the Sake's
struct Sake {
    int hasGame;
    char gameName[16];
    int gameId;
    char secretKey[12];
    int hasTicket;
    int profileId;
    SakeLoginTicket loginTicket;
    int lastError;
};

// The URL of the storage server, which the Wi-Fi library built (gdb.c)
extern char gSakeStorageUrl[128];

// Makes a Sake, returning 0 or an error code. It needs the Wi-Fi library to be running and not busy
int sakeStartup(Sake **sake);
// Frees a Sake
void Sake_Free(Sake *sake);
// Sets the game's name, ID and secret key, and the profile and login ticket of the player
void Sake_SetGame(Sake *sake, const char *gameName, int gameId, const char *secretKey);
void Sake_SetLoginTicket(Sake *sake, int profileId, const SakeLoginTicket *loginTicket);
// The error that the last call left
int Sake_GetLastError(const Sake *sake);
// Starts a request, returning it or 0 with the error in the Sake
SakeRequest *sakeCreateRecord(Sake *sake, SakeCreateRecordInput *input, SakeCallback callback, void *userData);
SakeRequest *sakeUpdateRecord(Sake *sake, SakeUpdateRecordInput *input, SakeCallback callback, void *userData);
SakeRequest *sakeGetMyRecords(Sake *sake, SakeGetMyRecordsInput *input, SakeCallback callback, void *userData);
SakeRequest *sakeGetSpecificRecords(Sake *sake, SakeGetSpecificRecordsInput *input, SakeCallback callback,
                                    void *userData);
// Sets the URLs of the file server in place of the standard, and gets those of a file to download and to upload
BOOL Sake_SetDownloadUrl(Sake *sake, const char *url);
BOOL Sake_GetDownloadUrl(const Sake *sake, int fileId, char *url);
BOOL Sake_SetUploadUrl(Sake *sake, const char *url);
BOOL Sake_GetUploadUrl(const Sake *sake, char *url);
// Stops a request, returning 0 or an error code
int Sake_CancelRequest(Sake *sake, SakeRequest *request);

#endif // POKEBW2_DWC_SAKE_H
