#include "types.h"
#include "battle/btl_server_cmd.h"
#include "gfl/std.h"
#include "stdarg.h"

static void PutU8(BtlServerCmdQueue *que, u8 value);
static u8 ReadU8(BtlServerCmdQueue *que);
static void PutU16(BtlServerCmdQueue *que, u16 value);
static u16 ReadU16(BtlServerCmdQueue *que);
static void PutU24(BtlServerCmdQueue *que, u32 value);
static u32 ReadU24(BtlServerCmdQueue *que);
static void PutU32(BtlServerCmdQueue *que, u32 value);
static u32 ReadU32(BtlServerCmdQueue *que);
static void PutCmdArgs(BtlServerCmdQueue *que, u32 event, int format, const u32 *args);
static void ReadCmdArgs(BtlServerCmdQueue *que, s32 format, u32 *args);
static void ReadMsgArgs(BtlServerCmdQueue *que, u8 event, u32 *args);

// The arguments of the server command being written
static u32 sCmdArgs[8];

// The asserts' expressions are spelled as the ROM's strings have them
// clang-format off
static void PutU8(BtlServerCmdQueue *que, u8 value) {
    GFL_ASSERT(que->writePtr < BTL_SERVER_CMD_QUE_SIZE);
    que->buffer[que->writePtr++] = value;
}

static u8 ReadU8(BtlServerCmdQueue *que) {
    return que->buffer[que->readPtr++];
}

static void PutU16(BtlServerCmdQueue *que, u16 value) {
    GFL_ASSERT(que->writePtr < (BTL_SERVER_CMD_QUE_SIZE-1));
    que->buffer[que->writePtr++] = value >> 8;
    que->buffer[que->writePtr++] = value;
}

static u16 ReadU16(BtlServerCmdQueue *que) {
    u16 value = (*(que->buffer + que->readPtr) << 8) | *(que->buffer + que->readPtr + 1);

    que->readPtr += 2;
    return value;
}

static void PutU24(BtlServerCmdQueue *que, u32 value) {
    GFL_ASSERT(que->writePtr < (BTL_SERVER_CMD_QUE_SIZE-2));
    que->buffer[que->writePtr++] = value >> 16;
    que->buffer[que->writePtr++] = value >> 8;
    que->buffer[que->writePtr++] = value;
}

static u32 ReadU24(BtlServerCmdQueue *que) {
    u32 value = (*(que->buffer + que->readPtr) << 16) | (*(que->buffer + que->readPtr + 1) << 8)
        | *(que->buffer + que->readPtr + 2);

    que->readPtr += 3;
    return value;
}

static void PutU32(BtlServerCmdQueue *que, u32 value) {
    GFL_ASSERT(que->writePtr < (BTL_SERVER_CMD_QUE_SIZE-3));
    que->buffer[que->writePtr++] = value >> 24;
    que->buffer[que->writePtr++] = value >> 16;
    que->buffer[que->writePtr++] = value >> 8;
    que->buffer[que->writePtr++] = value;
}

static u32 ReadU32(BtlServerCmdQueue *que) {
    u32 value = (*(que->buffer + que->readPtr) << 24) | (*(que->buffer + que->readPtr + 1) << 16)
        | (*(que->buffer + que->readPtr + 2) << 8) | *(que->buffer + que->readPtr + 3);

    que->readPtr += 4;
    return value;
}
// clang-format on

// Writes two halfwords, both computed before either is written
static inline void PutHalfwords(BtlServerCmdQueue *que, u16 first, u16 second) {
    PutU16(que, first);
    PutU16(que, second);
}

// Writes a command and its arguments, packed as its format says
static void PutCmdArgs(BtlServerCmdQueue *que, u32 event, int format, const u32 *args) {
    s32 i;

    PutU16(que, event);
    switch (format) {
    case 0x00:
        break;
    case 0x01:
        PutU8(que, args[0]);
        break;
    case 0x11:
        PutU16(que, args[0]);
        break;
    case 0x02:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        break;
    case 0x12:
        PutU8(que, args[0]);
        PutU16(que, args[1]);
        break;
    case 0x22:
        PutU8(que, args[0]);
        PutU32(que, args[1]);
        break;
    case 0x32:
        PutU8(que, ((args[0] & 0xf) << 4) | (args[1] & 0xf));
        break;
    case 0x42:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        break;
    case 0x03:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU8(que, args[2]);
        break;
    case 0x13:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU16(que, args[2]);
        break;
    case 0x23:
        PutU16(que, ((args[1] & 0x1f) << 5) | ((args[0] & 0x1f) << 10) | (args[2] & 0x1f));
        break;
    case 0x33:
        PutU24(que, ((args[1] & 0x1f) << 14) | ((args[0] & 0x1f) << 19) | (args[2] & 0x3fff));
        break;
    case 0x43:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        PutU16(que, args[2]);
        break;
    case 0x53:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        PutU32(que, args[2]);
        break;
    case 0x14:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU8(que, ((args[2] & 0x1f) << 3) | (args[3] & 7));
        break;
    case 0x04:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU8(que, args[2]);
        PutU16(que, args[3]);
        break;
    case 0x24:
        PutU24(que, ((args[1] & 0x1f) << 14) | ((args[0] & 0x1f) << 19) | (args[2] & 0x3fff));
        PutU8(que, args[3]);
        break;
    case 0x34:
        PutU24(que, ((args[1] & 0x1f) << 14) | ((args[0] & 0x1f) << 19) | (args[2] & 0x3fff));
        PutU16(que, args[3]);
        break;
    case 0x44:
        PutU16(que, ((args[1] & 0x1f) << 6) | ((args[0] & 0x1f) << 11) | (args[2] & 0x3f));
        PutU16(que, args[3]);
        break;
    case 0x54:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        PutU16(que, args[2]);
        PutU16(que, args[3]);
        break;
    case 0x05:
        PutU16(que, ((args[1] & 0x1f) << 5) | ((args[0] & 0x1f) << 10) | (args[2] & 0x1f));
        PutU16(que, args[3]);
        PutU16(que, args[4]);
        break;
    case 0x15:
        PutHalfwords(que, (u8)(((args[0] & 0x1f) << 3) | (args[1] & 7)), (u8)(((args[2] & 0x7f) << 1) | (args[3] & 1)));
        PutU16(que, args[4]);
        break;
    case 0x25:
        PutU8(que, ((args[0] & 7) << 5) | ((args[1] & 7) << 2) | ((args[2] & 1) << 1) | (args[3] & 1));
        PutU16(que, args[4]);
        break;
    case 0x06:
        PutHalfwords(que, ((args[1] & 0x1f) << 5) | ((args[0] & 0x1f) << 10) | (args[2] & 0x1f),
                     ((args[4] & 0x1f) << 5) | ((args[3] & 0x1f) << 10) | (args[5] & 0x1f));
        break;
    case 0x16:
        PutU8(que, ((args[0] & 7) << 5) | ((args[1] & 3) << 3) | ((args[2] & 1) << 2) | ((args[3] & 1) << 1) |
                       (args[4] & 1));
        PutU16(que, args[5]);
        break;
    case 0x26:
        PutU24(que, ((args[0] & 0x1f) << 15) | ((args[1] & 0x1f) << 10) | ((args[2] & 0x1f) << 5) | (args[3] & 0x1f));
        PutU16(que, args[4]);
        PutU16(que, args[5]);
        break;
    case 0x07:
        for (i = 0; i < 7; i++) {
            PutU8(que, args[i]);
        }
        break;
    case 0x08:
        for (i = 0; i < 8; i++) {
            PutU8(que, args[i]);
        }
        break;
    }
}

// Reads a command's arguments, unpacked as its format says
static void ReadCmdArgs(BtlServerCmdQueue *que, s32 format, u32 *args) {
    s32 i;
    u32 value;
    u32 value2;
    u8 byte;

    switch (format) {
    case 0x00:
        break;
    case 0x01:
        args[0] = ReadU8(que);
        break;
    case 0x11:
        args[0] = ReadU16(que);
        break;
    case 0x02:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        break;
    case 0x12:
        args[0] = ReadU8(que);
        args[1] = ReadU16(que);
        break;
    case 0x22:
        args[0] = ReadU8(que);
        args[1] = ReadU32(que);
        break;
    case 0x32:
        byte = ReadU8(que);
        args[0] = (byte >> 4) & 0xf;
        args[1] = byte & 0xf;
        break;
    case 0x42:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        break;
    case 0x03:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        args[2] = ReadU8(que);
        break;
    case 0x13:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        args[2] = ReadU16(que);
        break;
    case 0x23:
        value = ReadU16(que);
        args[0] = (value >> 10) & 0x1f;
        args[1] = (value >> 5) & 0x1f;
        args[2] = value & 0x1f;
        break;
    case 0x33:
        value = ReadU24(que);
        args[0] = (value >> 19) & 0x1f;
        args[1] = (value >> 14) & 0x1f;
        args[2] = value & 0x3fff;
        break;
    case 0x43:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        args[2] = ReadU16(que);
        break;
    case 0x53:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        args[2] = ReadU32(que);
        break;
    case 0x14:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        byte = ReadU8(que);
        args[2] = (byte >> 3) & 0x1f;
        args[3] = byte & 7;
        break;
    case 0x04:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        args[2] = ReadU8(que);
        args[3] = ReadU16(que);
        break;
    case 0x24:
        value = ReadU24(que);
        args[0] = (value >> 19) & 0x1f;
        args[1] = (value >> 14) & 0x1f;
        args[2] = value & 0x3fff;
        args[3] = ReadU8(que);
        break;
    case 0x34:
        value = ReadU24(que);
        args[0] = (value >> 19) & 0x1f;
        args[1] = (value >> 14) & 0x1f;
        args[2] = value & 0x3fff;
        args[3] = ReadU16(que);
        break;
    case 0x44:
        value = ReadU16(que);
        args[0] = (value >> 11) & 0x1f;
        args[1] = (value >> 6) & 0x1f;
        args[2] = value & 0x3f;
        args[3] = ReadU16(que);
        break;
    case 0x54:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        args[2] = ReadU16(que);
        args[3] = ReadU16(que);
        break;
    case 0x05:
        value = ReadU16(que);
        args[0] = (value >> 10) & 0x1f;
        args[1] = (value >> 5) & 0x1f;
        args[2] = value & 0x1f;
        args[3] = ReadU16(que);
        args[4] = ReadU16(que);
        break;
    case 0x15:
        value = ReadU16(que);
        value2 = ReadU16(que);
        args[0] = ((u8)value >> 3) & 0x1f;
        args[1] = (u8)value & 7;
        args[2] = ((u8)value2 >> 1) & 0x7f;
        args[3] = (u8)value2 & 1;
        args[4] = ReadU16(que);
        break;
    case 0x25:
        value = ReadU8(que);
        args[0] = (value >> 5) & 7;
        args[1] = (value >> 2) & 7;
        args[2] = (value >> 1) & 1;
        args[3] = value & 1;
        args[4] = ReadU16(que);
        break;
    case 0x06:
        value = ReadU16(que);
        value2 = ReadU16(que);
        args[0] = (value >> 10) & 0x1f;
        args[1] = (value >> 5) & 0x1f;
        args[2] = value & 0x1f;
        args[3] = (value2 >> 10) & 0x1f;
        args[4] = (value2 >> 5) & 0x1f;
        args[5] = value2 & 0x1f;
        break;
    case 0x16:
        value = ReadU8(que);
        args[0] = (value >> 5) & 7;
        args[1] = (value >> 3) & 3;
        args[2] = (value >> 2) & 1;
        args[3] = (value >> 1) & 1;
        args[4] = value & 1;
        args[5] = ReadU16(que);
        break;
    case 0x26:
        value = ReadU24(que);
        args[0] = (value >> 15) & 0x1f;
        args[1] = (value >> 10) & 0x1f;
        args[2] = (value >> 5) & 0x1f;
        args[3] = value & 0x1f;
        args[4] = ReadU16(que);
        args[5] = ReadU16(que);
        break;
    case 0x07:
        for (i = 0; i < 7; i++) {
            args[i] = ReadU8(que);
        }
        break;
    case 0x08:
        for (i = 0; i < 8; i++) {
            args[i] = ReadU8(que);
        }
        break;
    }
}

// Each command's format: the number of its arguments in the low nibble, and their widths in the high one
static const u8 data_ov167_021d6e50[0x60] = {
    0x00, 0x12, 0x12, 0x01, 0x03, 0x03, 0x42, 0x03, 0x03, 0x03, 0x03, 0x08, 0x01, 0x01, 0x22, 0x53,
    0x01, 0x12, 0x04, 0x33, 0x12, 0x53, 0x12, 0x12, 0x26, 0x02, 0x02, 0x02, 0x02, 0x12, 0x12, 0x15,
    0x01, 0x22, 0x02, 0x01, 0x01, 0x33, 0x02, 0x12, 0x01, 0x32, 0x01, 0x01, 0x02, 0x26, 0x01, 0x01,
    0x24, 0x42, 0x43, 0x43, 0x01, 0x02, 0x01, 0x23, 0x23, 0x01, 0x01, 0x02, 0x12, 0x14, 0x07, 0x02,
    0x01, 0x01, 0x01, 0x42, 0x23, 0x22, 0x16, 0x12, 0x32, 0x12, 0x54, 0x01, 0x11, 0x12, 0x43, 0x02,
    0x14, 0x01, 0x01, 0x02, 0x13, 0x11, 0x01, 0x01, 0x01, 0x12, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00,
};

// Writes a server command with its arguments, in the widths its format gives them
void BtlServerCmdQueue_Put(BtlServerCmdQueue *que, u32 event, ...) {
    va_list list;
    u8 format;
    u32 i;
    u32 count;

    format = data_ov167_021d6e50[event];
    count = format & 0xf;
    va_start(list, event);
    for (i = 0; i < count; i++) {
        sCmdArgs[i] = va_arg(list, u32);
    }
    va_end(list);
    PutCmdArgs(que, event, format, sCmdArgs);
}

// Function name from swan.
u16 SCQUE_RESERVE_Pos(BtlServerCmdQueue *que, u32 event) {
    u8 format = data_ov167_021d6e50[event];
    u8 count = format & 0xf;
    u8 i;
    u16 pos;
    u8 size;

    for (i = 0; i < count; i++) {
        sCmdArgs[i] = 0;
    }
    pos = que->writePtr;
    PutCmdArgs(que, event, format, sCmdArgs);
    size = que->writePtr - pos;
    que->writePtr = pos;
    PutU16(que, 0x5f);
    PutU8(que, size - 3);
    que->writePtr = pos + size;
    return pos;
}

// Fills in a command that SCQUE_RESERVE_Pos reserved room for
void BtlServerCmdQueue_PutReserved(BtlServerCmdQueue *que, u32 reserve, u32 event, ...) {
    va_list list;
    u8 format;
    u32 i;
    u32 count;
    u16 pos;

    format = data_ov167_021d6e50[event];
    count = format & 0xf;
    va_start(list, event);
    for (i = 0; i < count; i++) {
        sCmdArgs[i] = va_arg(list, u32);
    }
    va_end(list);
    pos = que->readPtr;
    que->readPtr = reserve;
    ReadU16(que);
    ReadU8(que);
    que->readPtr = pos;
    if (event != 0x5f) {
        pos = que->writePtr;
        que->writePtr = reserve;
        PutCmdArgs(que, event, format, sCmdArgs);
        que->writePtr = pos;
    }
}

// Reads the next command and its arguments, skipping reserved room left empty
u16 BtlServerCmdQueue_Read(BtlServerCmdQueue *que, u32 *args) {
    u16 event;
    u8 format;

    event = ReadU16(que);
    while (event == 0x5f) {
        que->readPtr += ReadU8(que);
        if (que->readPtr >= que->writePtr) {
            return 0x5e;
        }
        event = ReadU16(que);
    }
    format = data_ov167_021d6e50[event];
    if (format != 0 && format != 0x10) {
        ReadCmdArgs(que, format, args);
    } else {
        ReadMsgArgs(que, event, args);
    }
    return event;
}

void BtlServerCmdQueue_PutByte(BtlServerCmdQueue *que, u8 value) {
    PutU8(que, value);
}

u8 BtlServerCmdQueue_ReadByte(BtlServerCmdQueue *que) {
    return ReadU8(que);
}

// Writes a message command: the message, then its arguments up to 0xffff0000
void BtlServerCmdQueue_PutMsg(BtlServerCmdQueue *que, u8 event, ...) {
    va_list list;
    u16 message;
    u32 arg;

    va_start(list, event);
    message = va_arg(list, u32);
    PutU16(que, event);
    PutU16(que, message);
    if (event == 0x5c) {
        PutU16(que, va_arg(list, u32));
    }
    do {
        arg = va_arg(list, u32);
        PutU32(que, arg);
    } while (arg != 0xffff0000);
    va_end(list);
}

// Reads a message command's arguments
static void ReadMsgArgs(BtlServerCmdQueue *que, u8 event, u32 *args) {
    s32 i = 1;

    args[0] = ReadU16(que);
    if (event == 0x5c) {
        args[1] = ReadU16(que);
        i++;
    }
    for (; i < 16; i++) {
        args[i] = ReadU32(que);
        if (args[i] == 0xffff0000) {
            break;
        }
    }
}

void func_ov167_021b1670(void) {
}
