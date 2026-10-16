#ifndef POKEBW2_TWL_NWM_H
#define POKEBW2_TWL_NWM_H

#include "types.h"
#include "twl/pm.h"

// TwlSDK's NWM, the DSi's wireless manager, ARM9 side, in the LTD autoload (nwm.c). It does for the DSi's own wireless
// chip what NitroSDK's WM does for the DS's: the ARM9 sends the ARM7 commands through the PXI tag 0x16, in buffers of a
// system area that the game hands to the library (NWM_Init), and the ARM7 answers with events that run the command's
// callback. The functions have the names of the SDK functions they implement, from what the code does, where those are
// known. The game calls the functions through thumb veneers that the linker made.

typedef enum {
    NWM_RETCODE_SUCCESS = 0,
    NWM_RETCODE_OPERATING = 2,     // The command was sent: its callback gets the result
    NWM_RETCODE_ILLEGAL_STATE = 3, // Not initialized, or not in a state that takes the command, or already busy with it
    NWM_RETCODE_UNAVAILABLE = 4,   // The wireless is not available, or the ARM7 is not ready for the tag
    NWM_RETCODE_INVALID_PARAM = 5,
    NWM_RETCODE_SEND_ERROR = 6, // No free command buffer, or the FIFO refused the word
    NWM_RETCODE_DISABLED = 7    // The wireless is turned off in the settings
} NWMRetCode;

// What a command does when the ARM7 is done; the argument is the event that it sent back
typedef void (*NWMCallback)(void *event);

// A scan result: its length in bytes, and from NWM_BSS_DESC_ELEMENTS_OFFSET the 802.11 information elements of
// the beacon, each an ID, a length and its bytes
#define NWM_BSS_DESC_ELEMENTS_OFFSET 0x40

typedef struct NWMBssDesc {
    u16 length;
    u8 unk_02[0x3e]; // The BSSID, the SSID and the other fields that the library does not read
    u8 elements[];
} NWMBssDesc;

// What a scan is asked for. The command takes its fields from here, and the library zeroes the buffer
typedef struct NWMScanParam {
    u8 *buffer;
    u16 bufferSize;
    u16 channelList;
    u16 dwellTime;
    u16 scanType;
    u8 bssid[6];
    u16 ssidLength; // At most 32
    u8 ssid[32];
    u16 ssidMatchLength; // At most 32
} NWMScanParam;

// What the command 8 takes: a block of at most 0x200 bytes, whose first halfword is its length in bytes, with a
// halfword at 0xa that must not be zero. The ARM7 gets a copy of it
typedef struct NWMBlockParam {
    u16 length;
    u8 unk_02[8];
    u16 unk_0a;
    u8 unk_0c[];
} NWMBlockParam;

// What the command 0x12 takes: a type from 4 to 7 and 32 bytes, which fit a pairwise master key
typedef struct NWMPmkParam {
    u16 type;
    u8 pmk[32];
} NWMPmkParam;

// NWM_Init: sets the library up with a 32-byte-aligned system area of the given size (whose beginning holds the work,
// its buffers and its callbacks) and a DMA channel to clear it with, and takes the PXI tag
NWMRetCode NWM_Init(void *systemBuffer, u32 size, u32 dmaNo);
// SUCCESS if the library is initialized, else ILLEGAL_STATE
NWMRetCode func_02702698(void);
// The library's state, 0 when it is not initialized
u16 func_027028b0(void);
// A halfword (at 0x9e) of the ARM7's status block
u16 func_02702980(void);
// A number from the console's MAC address and the VBlank count: 30 to 39 when the argument is 1, else 105 to 114
u16 func_02702998(int type);
NWMRetCode NWM_End(void);
// Also puts the callbacks that exit the application when the console sleeps and before it exits
NWMRetCode NWM_LoadDevice(NWMCallback callback);
NWMRetCode NWM_UnloadDevice(NWMCallback callback);
// The command 5, for a console with the wireless on (the name is a guess from the states)
NWMRetCode NWM_Open(NWMCallback callback);
// The command 6 (the name is a guess from the states)
NWMRetCode NWM_Close(NWMCallback callback);
NWMRetCode NWM_StartScan(NWMCallback callback, const NWMScanParam *param);
// The first information element of a scan result with the ID, or NULL
u8 *func_02703208(NWMBssDesc *bssDesc, u32 id);
// The first vendor-specific element (ID 0xdd) of a scan result that begins with the four bytes (the OUI and the
// type), or NULL; any other ID finds none
u8 *func_02703250(NWMBssDesc *bssDesc, u32 id, const u8 *oui);
// The command 8, with a block for the ARM7 and an index below 3
NWMRetCode func_027032e0(NWMCallback callback, u32 index, const NWMBlockParam *block);
// The command 9, in the connected state (the name is a guess from the state)
NWMRetCode NWM_Disconnect(NWMCallback callback);
// The command 0xd (the name is a guess from the state): the mode is 0 to 3, and the 0x50 bytes of data go along
NWMRetCode NWM_Connect(NWMCallback callback, u32 mode, u32 arg, const void *data, u16 arg2);
// The command 0xe
NWMRetCode func_027035f4(NWMCallback callback, u32 arg);
// NWM_SendFrame: sends the frame, whose first 12 bytes the library fills with the two MAC addresses. The callback runs
// when the ARM7 is done with it
NWMRetCode NWM_SendFrame(NWMCallback callback, const u8 *destAddr, u8 *frame, u32 size);
// NWM_SetReceivingFrameBuffer: gives the ARM7 a buffer (zeroed) to receive frames in, and the callback that the
// events of the given index run
NWMRetCode NWM_SetReceivingFrameBuffer(NWMCallback receiveCallback, void *buffer, u32 size, u32 index);
// The command 0xc, which takes the index of the callback of the received frames
NWMRetCode func_02703854(NWMCallback callback, u32 index);
// The command 0x12 (a pairwise master key, by the parameter)
NWMRetCode func_02703960(NWMCallback callback, NWMPmkParam *param);

#endif // POKEBW2_TWL_NWM_H
