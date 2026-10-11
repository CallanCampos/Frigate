#ifndef POKEBW2_DWC_GDB_H
#define POKEBW2_DWC_GDB_H

#include "types.h"

// The Wi-Fi Connection library's record database client, in overlay 260 beside wifibattlematch_net.c, which uses it to
// keep the Wi-Fi competitions' stats and the players' teams on Nintendo's servers (GameSpy's Sake storage). The ROM
// names none of it except in its assertion and log texts ("Gdb[%d]> ", "DWCGdbError[%d]", "sakeGetMyRecords failed"),
// and its functions have no names yet; they are named by what wifibattlematch_net.c does with them

// The type of a field of a record, as the code writes it. The texts of the requests name more
typedef enum {
    GDB_FIELD_BYTE = 0,
    GDB_FIELD_INT = 2,
    GDB_FIELD_BINARY = 8,
} GdbFieldType;

// A field of a record, to write and as read: its name, its type, its value and, for binary data, the size
typedef struct {
    const char *name;
    u32 type;
    union {
        s32 intValue;
        u8 byteValue;
        void *data;
    } value;
    u32 size;
} GdbField;

// Called with an error code when a request has ended
typedef void (*GdbResultFunc)(int error, void *work);
// Called with the records that a request got, each an array of fieldNum fields, and its work
typedef void (*GdbRecordsFunc)(int recordNum, GdbField **records, void *work, int fieldNum);

// Starts the library for a game's ID, returning 0 or an error code
int func_ov260_021bd43c(int gameId);
// Ends the library's work, and runs it for a frame
void func_ov260_021bd4f4(void);
void func_ov260_021bd554(void);
// Requests that call a function when they end, with the time they may take
int func_ov260_021bd57c(GdbResultFunc func, int timeout, void *work);
int func_ov260_021bd5d4(int a0, GdbResultFunc func, int timeout, void *work);
int func_ov260_021bd634(void *connection, int a1, GdbResultFunc func, int timeout, void *work);
// Checks the data that the other player sent, returning 0 or an error code
int func_ov260_021bd698(const void *data);
// The two blocks of 0x28 bytes that the two players' data is made of
void *func_ov260_021bd6c0(void);
void *func_ov260_021bd6d0(void);
// Opens a connection
int func_ov260_021bd6e0(int a0, int a1, int a2, void **connection);
// Steps of the exchange of the two players' data, each returning 0 or an error code
int func_ov260_021bd718(void *connection);
int func_ov260_021bd738(void *connection);
int func_ov260_021bd758(void *connection);
int func_ov260_021bd778(void *connection);
int func_ov260_021bd798(void *connection, int a1, void *a2, int a3, int a4, u32 a5, void *a6);
int func_ov260_021bd7cc(void *connection, int a1, int a2);
// Sets a field of the exchange to a value, or (the second) a flag
int func_ov260_021bd7f0(void *connection, int field, int value);
int func_ov260_021bd814(void *connection, int field, int value);
// Ends the exchange
void func_ov260_021bd838(void *connection);
int func_ov260_021bd85c(void *info);
// Starts the library's session for a game ID with the player's data and a flag, returning 0 or an error code
int func_ov260_021beaa0(int gameId, void *data, int a2);
// The state of a request that the library runs, and a call that goes with it
int func_ov260_021beb5c(void);
void func_ov260_021becd4(void);
int func_ov260_021becec(void);
int func_ov260_021bed04(void);
// Requests on a table: create a record, update a record, get the player's records, get specific records
int func_ov260_021bed10(const char *table, GdbField *record, int fieldNum, void *data);
int func_ov260_021beda0(const char *table, int recordId, GdbField *record, int fieldNum);
int func_ov260_021bee30(const char *table, const char **fields, int fieldNum, GdbRecordsFunc func, void *work);
int func_ov260_021beec0(const char *table, int *recordIds, int recordNum, const char **fields, int fieldNum,
                        GdbRecordsFunc func, void *work);
// Ends the session, which a Wi-Fi library error of the right range is passed to
void func_ov260_021bec44(void);

// Overlay 11's, used with the matching keys: adds a key with a value that is read when the match runs, and reads
// a candidate's key. They belong with dwc.h's other matching functions
u8 func_ov011_02160e10(u8 keyId, const char *key, const int *valueSrc);
int func_ov011_02160f98(int index, const char *key, int defaultValue);

#endif // POKEBW2_DWC_GDB_H
