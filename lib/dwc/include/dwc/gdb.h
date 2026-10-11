#ifndef POKEBW2_DWC_GDB_H
#define POKEBW2_DWC_GDB_H

#include "types.h"
#include "dwc/dwc.h"
#include "dwc/sake.h"
#include "dwc/sc.h"

// Nintendo's Wi-Fi Connection library's record database client, in overlay 260 (gdb.c), which wifibattlematch_net.c uses
// to keep the Wi-Fi competitions' stats and the players' teams on the storage server (GameSpy's Sake, dwc/sake.h).
// The ROM names it in its log texts ("Gdb[%d]> ", "DWCGdbError[%d]") and in the names of the Sake calls that it
// reports failing. It works one request at a time, in the order of the library: start it, start a request, run it
// each frame until it ends, then ask what the result was. The Sc, which this header used to hold, is dwc/sc.h's

// A field of a record, to write and as read: SakeField's first fields, with the binary data's size as the callers
// write it. Its types are SakeFieldType's
typedef struct GdbField {
    const char *name;
    u32 type;
    union {
        s32 intValue;
        u8 byteValue;
        void *data;
    } value;
    u32 size;
} GdbField;
#define GDB_FIELD_BYTE SAKE_FIELD_BYTE
#define GDB_FIELD_INT SAKE_FIELD_INT
#define GDB_FIELD_BINARY SAKE_FIELD_BINARY_DATA

// Called when the library has the records that a request asked for: with their number, each an array of the fields the
// request was given, and the work. The library passes the first three; a function may declare a fourth that it does not
// get, as the callers' do
typedef void (*GdbRecordsFunc)(int recordNum, GdbField **records, void *work, int fieldNum);

// The states of the library, as Gdb_GetState gives them
typedef enum GdbState {
    GDB_STATE_OFF = 0,
    GDB_STATE_IDLE = 1,
    GDB_STATE_BUSY = 2,
    GDB_STATE_CANCELING = 3,
} GdbState;

// Starts the library for a game with the player's user data and a kind of server (see Sc_Startup), returning 0 or an
// error code. It needs the Wi-Fi library to be running and the player to have a login ticket
int Gdb_Startup(int gameId, const DWCUserData *userData, int server);
// Cancels the request that is running, returning 0 or 0x11 when there is none
int Gdb_Cancel(void);
// Ends the library, returning 0 or an error code
int Gdb_Shutdown(void);
// Runs the library for a frame
void Gdb_Update(void);
// The state of the library, or 4 when the Wi-Fi library has an error
int Gdb_GetState(void);
// The result of the last request: 1 once it worked, 0 while it runs, or an error code
int Gdb_GetResult(void);
// The requests, returning 0 or an error code: create a record in a table, giving back its ID; update a record; get the
// records of the player; and get specific records by their IDs
int Gdb_CreateRecord(const char *table, GdbField *fields, int fieldNum, int *recordId);
int Gdb_UpdateRecord(const char *table, int recordId, GdbField *fields, int fieldNum);
int Gdb_GetMyRecords(const char *table, const char **fieldNames, int fieldNum, GdbRecordsFunc func, void *work);
int Gdb_GetSpecificRecords(const char *table, int *recordIds, int recordNum, const char **fieldNames, int fieldNum,
                           GdbRecordsFunc func, void *work);

#endif // POKEBW2_DWC_GDB_H
