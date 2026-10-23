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
static void PutCmdArgs(BtlServerCmdQueue *que, BtlServerCmd event, int format, const u32 *args);
static void ReadCmdArgs(BtlServerCmdQueue *que, s32 format, u32 *args);
static void ReadMsgArgs(BtlServerCmdQueue *que, u8 event, u32 *args);

// The formats of the commands' arguments: the low nibble is how many there are, and the high one tells formats of as
// many apart. The names give the widths of the arguments in their order, in bits where several share bytes, which are
// packed high bits first
enum {
    SC_ARGS_NONE = 0x00,
    SC_ARGS_MSG_SE = 0x10,
    SC_ARGS_U8 = 0x01,
    SC_ARGS_U16 = 0x11,
    SC_ARGS_U8_U8 = 0x02,
    SC_ARGS_U8_U16 = 0x12,
    SC_ARGS_U8_U32 = 0x22,
    SC_ARGS_4_4 = 0x32,
    SC_ARGS_5_3 = 0x42,
    SC_ARGS_5_3_U8 = 0x03,
    SC_ARGS_5_3_U16 = 0x13,
    SC_ARGS_5_5_5 = 0x23,
    SC_ARGS_5_5_14 = 0x33,
    SC_ARGS_U8_U8_U16 = 0x43,
    SC_ARGS_U8_U8_U32 = 0x53,
    SC_ARGS_5_3_U8_U16 = 0x04,
    SC_ARGS_5_3_5_3 = 0x14,
    SC_ARGS_5_5_14_U8 = 0x24,
    SC_ARGS_5_5_14_U16 = 0x34,
    SC_ARGS_5_5_6_U16 = 0x44,
    SC_ARGS_U8_U8_U16_U16 = 0x54,
    SC_ARGS_5_5_5_U16_U16 = 0x05,
    SC_ARGS_5_3_7_1_U16 = 0x15,
    SC_ARGS_3_3_1_1_U16 = 0x25,
    SC_ARGS_5_5_5_5_5_5 = 0x06,
    SC_ARGS_3_2_1_1_1_U16 = 0x16,
    SC_ARGS_5_5_5_5_U16_U16 = 0x26,
    SC_ARGS_U8X7 = 0x07,
    SC_ARGS_U8X8 = 0x08,
};

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
static void PutCmdArgs(BtlServerCmdQueue *que, BtlServerCmd event, int format, const u32 *args) {
    s32 i;

    PutU16(que, event);
    switch (format) {
    case SC_ARGS_NONE:
        break;
    case SC_ARGS_U8:
        PutU8(que, args[0]);
        break;
    case SC_ARGS_U16:
        PutU16(que, args[0]);
        break;
    case SC_ARGS_U8_U8:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        break;
    case SC_ARGS_U8_U16:
        PutU8(que, args[0]);
        PutU16(que, args[1]);
        break;
    case SC_ARGS_U8_U32:
        PutU8(que, args[0]);
        PutU32(que, args[1]);
        break;
    case SC_ARGS_4_4:
        PutU8(que, ((args[0] & 0xf) << 4) | (args[1] & 0xf));
        break;
    case SC_ARGS_5_3:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        break;
    case SC_ARGS_5_3_U8:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU8(que, args[2]);
        break;
    case SC_ARGS_5_3_U16:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU16(que, args[2]);
        break;
    case SC_ARGS_5_5_5:
        PutU16(que, ((args[1] & 0x1f) << 5) | ((args[0] & 0x1f) << 10) | (args[2] & 0x1f));
        break;
    case SC_ARGS_5_5_14:
        PutU24(que, ((args[1] & 0x1f) << 14) | ((args[0] & 0x1f) << 19) | (args[2] & 0x3fff));
        break;
    case SC_ARGS_U8_U8_U16:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        PutU16(que, args[2]);
        break;
    case SC_ARGS_U8_U8_U32:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        PutU32(que, args[2]);
        break;
    case SC_ARGS_5_3_5_3:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU8(que, ((args[2] & 0x1f) << 3) | (args[3] & 7));
        break;
    case SC_ARGS_5_3_U8_U16:
        PutU8(que, ((args[0] & 0x1f) << 3) | (args[1] & 7));
        PutU8(que, args[2]);
        PutU16(que, args[3]);
        break;
    case SC_ARGS_5_5_14_U8:
        PutU24(que, ((args[1] & 0x1f) << 14) | ((args[0] & 0x1f) << 19) | (args[2] & 0x3fff));
        PutU8(que, args[3]);
        break;
    case SC_ARGS_5_5_14_U16:
        PutU24(que, ((args[1] & 0x1f) << 14) | ((args[0] & 0x1f) << 19) | (args[2] & 0x3fff));
        PutU16(que, args[3]);
        break;
    case SC_ARGS_5_5_6_U16:
        PutU16(que, ((args[1] & 0x1f) << 6) | ((args[0] & 0x1f) << 11) | (args[2] & 0x3f));
        PutU16(que, args[3]);
        break;
    case SC_ARGS_U8_U8_U16_U16:
        PutU8(que, args[0]);
        PutU8(que, args[1]);
        PutU16(que, args[2]);
        PutU16(que, args[3]);
        break;
    case SC_ARGS_5_5_5_U16_U16:
        PutU16(que, ((args[1] & 0x1f) << 5) | ((args[0] & 0x1f) << 10) | (args[2] & 0x1f));
        PutU16(que, args[3]);
        PutU16(que, args[4]);
        break;
    case SC_ARGS_5_3_7_1_U16:
        PutHalfwords(que, (u8)(((args[0] & 0x1f) << 3) | (args[1] & 7)), (u8)(((args[2] & 0x7f) << 1) | (args[3] & 1)));
        PutU16(que, args[4]);
        break;
    case SC_ARGS_3_3_1_1_U16:
        PutU8(que, ((args[0] & 7) << 5) | ((args[1] & 7) << 2) | ((args[2] & 1) << 1) | (args[3] & 1));
        PutU16(que, args[4]);
        break;
    case SC_ARGS_5_5_5_5_5_5:
        PutHalfwords(que, ((args[1] & 0x1f) << 5) | ((args[0] & 0x1f) << 10) | (args[2] & 0x1f),
                     ((args[4] & 0x1f) << 5) | ((args[3] & 0x1f) << 10) | (args[5] & 0x1f));
        break;
    case SC_ARGS_3_2_1_1_1_U16:
        PutU8(que, ((args[0] & 7) << 5) | ((args[1] & 3) << 3) | ((args[2] & 1) << 2) | ((args[3] & 1) << 1) |
                       (args[4] & 1));
        PutU16(que, args[5]);
        break;
    case SC_ARGS_5_5_5_5_U16_U16:
        PutU24(que, ((args[0] & 0x1f) << 15) | ((args[1] & 0x1f) << 10) | ((args[2] & 0x1f) << 5) | (args[3] & 0x1f));
        PutU16(que, args[4]);
        PutU16(que, args[5]);
        break;
    case SC_ARGS_U8X7:
        for (i = 0; i < 7; i++) {
            PutU8(que, args[i]);
        }
        break;
    case SC_ARGS_U8X8:
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
    case SC_ARGS_NONE:
        break;
    case SC_ARGS_U8:
        args[0] = ReadU8(que);
        break;
    case SC_ARGS_U16:
        args[0] = ReadU16(que);
        break;
    case SC_ARGS_U8_U8:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        break;
    case SC_ARGS_U8_U16:
        args[0] = ReadU8(que);
        args[1] = ReadU16(que);
        break;
    case SC_ARGS_U8_U32:
        args[0] = ReadU8(que);
        args[1] = ReadU32(que);
        break;
    case SC_ARGS_4_4:
        byte = ReadU8(que);
        args[0] = (byte >> 4) & 0xf;
        args[1] = byte & 0xf;
        break;
    case SC_ARGS_5_3:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        break;
    case SC_ARGS_5_3_U8:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        args[2] = ReadU8(que);
        break;
    case SC_ARGS_5_3_U16:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        args[2] = ReadU16(que);
        break;
    case SC_ARGS_5_5_5:
        value = ReadU16(que);
        args[0] = (value >> 10) & 0x1f;
        args[1] = (value >> 5) & 0x1f;
        args[2] = value & 0x1f;
        break;
    case SC_ARGS_5_5_14:
        value = ReadU24(que);
        args[0] = (value >> 19) & 0x1f;
        args[1] = (value >> 14) & 0x1f;
        args[2] = value & 0x3fff;
        break;
    case SC_ARGS_U8_U8_U16:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        args[2] = ReadU16(que);
        break;
    case SC_ARGS_U8_U8_U32:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        args[2] = ReadU32(que);
        break;
    case SC_ARGS_5_3_5_3:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        byte = ReadU8(que);
        args[2] = (byte >> 3) & 0x1f;
        args[3] = byte & 7;
        break;
    case SC_ARGS_5_3_U8_U16:
        byte = ReadU8(que);
        args[0] = (byte >> 3) & 0x1f;
        args[1] = byte & 7;
        args[2] = ReadU8(que);
        args[3] = ReadU16(que);
        break;
    case SC_ARGS_5_5_14_U8:
        value = ReadU24(que);
        args[0] = (value >> 19) & 0x1f;
        args[1] = (value >> 14) & 0x1f;
        args[2] = value & 0x3fff;
        args[3] = ReadU8(que);
        break;
    case SC_ARGS_5_5_14_U16:
        value = ReadU24(que);
        args[0] = (value >> 19) & 0x1f;
        args[1] = (value >> 14) & 0x1f;
        args[2] = value & 0x3fff;
        args[3] = ReadU16(que);
        break;
    case SC_ARGS_5_5_6_U16:
        value = ReadU16(que);
        args[0] = (value >> 11) & 0x1f;
        args[1] = (value >> 6) & 0x1f;
        args[2] = value & 0x3f;
        args[3] = ReadU16(que);
        break;
    case SC_ARGS_U8_U8_U16_U16:
        args[0] = ReadU8(que);
        args[1] = ReadU8(que);
        args[2] = ReadU16(que);
        args[3] = ReadU16(que);
        break;
    case SC_ARGS_5_5_5_U16_U16:
        value = ReadU16(que);
        args[0] = (value >> 10) & 0x1f;
        args[1] = (value >> 5) & 0x1f;
        args[2] = value & 0x1f;
        args[3] = ReadU16(que);
        args[4] = ReadU16(que);
        break;
    case SC_ARGS_5_3_7_1_U16:
        value = ReadU16(que);
        value2 = ReadU16(que);
        args[0] = ((u8)value >> 3) & 0x1f;
        args[1] = (u8)value & 7;
        args[2] = ((u8)value2 >> 1) & 0x7f;
        args[3] = (u8)value2 & 1;
        args[4] = ReadU16(que);
        break;
    case SC_ARGS_3_3_1_1_U16:
        value = ReadU8(que);
        args[0] = (value >> 5) & 7;
        args[1] = (value >> 2) & 7;
        args[2] = (value >> 1) & 1;
        args[3] = value & 1;
        args[4] = ReadU16(que);
        break;
    case SC_ARGS_5_5_5_5_5_5:
        value = ReadU16(que);
        value2 = ReadU16(que);
        args[0] = (value >> 10) & 0x1f;
        args[1] = (value >> 5) & 0x1f;
        args[2] = value & 0x1f;
        args[3] = (value2 >> 10) & 0x1f;
        args[4] = (value2 >> 5) & 0x1f;
        args[5] = value2 & 0x1f;
        break;
    case SC_ARGS_3_2_1_1_1_U16:
        value = ReadU8(que);
        args[0] = (value >> 5) & 7;
        args[1] = (value >> 3) & 3;
        args[2] = (value >> 2) & 1;
        args[3] = (value >> 1) & 1;
        args[4] = value & 1;
        args[5] = ReadU16(que);
        break;
    case SC_ARGS_5_5_5_5_U16_U16:
        value = ReadU24(que);
        args[0] = (value >> 15) & 0x1f;
        args[1] = (value >> 10) & 0x1f;
        args[2] = (value >> 5) & 0x1f;
        args[3] = value & 0x1f;
        args[4] = ReadU16(que);
        args[5] = ReadU16(que);
        break;
    case SC_ARGS_U8X7:
        for (i = 0; i < 7; i++) {
            args[i] = ReadU8(que);
        }
        break;
    case SC_ARGS_U8X8:
        for (i = 0; i < 8; i++) {
            args[i] = ReadU8(que);
        }
        break;
    }
}

// Each command's format
static const u8 data_ov167_021d6e50[SC_MAX] = {
    [SC_NULL] = SC_ARGS_NONE,
    [SC_OP_SUBTRACT_HP] = SC_ARGS_U8_U16,
    [SC_OP_ADD_HP] = SC_ARGS_U8_U16,
    [SC_OP_ZERO_HP] = SC_ARGS_U8,
    [SC_OP_SUBTRACT_PP] = SC_ARGS_5_3_U8,
    [SC_OP_SUBTRACT_ORIGINAL_PP] = SC_ARGS_5_3_U8,
    [SC_OP_SET_MOVE_USED] = SC_ARGS_5_3,
    [SC_OP_ADD_PP] = SC_ARGS_5_3_U8,
    [SC_OP_ADD_ORIGINAL_PP] = SC_ARGS_5_3_U8,
    [SC_OP_RAISE_STAT_STAGE] = SC_ARGS_5_3_U8,
    [SC_OP_LOWER_STAT_STAGE] = SC_ARGS_5_3_U8,
    [SC_OP_SET_STAT_STAGES] = SC_ARGS_U8X8,
    [SC_OP_RECOVER_STAT_STAGES] = SC_ARGS_U8,
    [SC_OP_RESET_STAT_STAGES] = SC_ARGS_U8,
    [SC_OP_ADD_CRIT_STAGE] = SC_ARGS_U8_U32,
    [SC_OP_SET_CONDITION] = SC_ARGS_U8_U8_U32,
    [SC_OP_CURE_CONDITION] = SC_ARGS_U8,
    [SC_OP_CURE_MOVE_CONDITION] = SC_ARGS_U8_U16,
    [SC_OP_SEND_IN] = SC_ARGS_5_3_U8_U16,
    [SC_OP_SET_BASE_STAT] = SC_ARGS_5_5_14,
    [SC_OP_SET_WEIGHT] = SC_ARGS_U8_U16,
    [SC_OP_CONDITION_TURN_CHECK] = SC_ARGS_U8_U8_U32,
    [SC_OP_CHANGE_TYPE] = SC_ARGS_U8_U16,
    [SC_OP_CONSUME_ITEM] = SC_ARGS_U8_U16,
    [SC_OP_UPDATE_USED_MOVE] = SC_ARGS_5_5_5_5_U16_U16,
    [SC_OP_SET_CONDITION_FLAG] = SC_ARGS_U8_U8,
    [SC_OP_CLEAR_CONDITION_FLAG] = SC_ARGS_U8_U8,
    [SC_OP_SET_TURN_FLAG] = SC_ARGS_U8_U8,
    [SC_OP_CLEAR_TURN_FLAG] = SC_ARGS_U8_U8,
    [SC_OP_CHANGE_ABILITY] = SC_ARGS_U8_U16,
    [SC_OP_SET_ITEM] = SC_ARGS_U8_U16,
    [SC_OP_UPDATE_MOVE] = SC_ARGS_5_3_7_1_U16,
    [SC_OP_CLEAR_FOR_SWITCH] = SC_ARGS_U8,
    [SC_OP_ADD_FIELD_EFFECT] = SC_ARGS_U8_U32,
    [SC_OP_ADD_FIELD_EFFECT_DEPEND_MON] = SC_ARGS_U8_U8,
    [SC_OP_REMOVE_FIELD_EFFECT_DEPEND_MON] = SC_ARGS_U8,
    [SC_OP_REMOVE_FIELD_EFFECT] = SC_ARGS_U8,
    [SC_OP_SET_COUNTER] = SC_ARGS_5_5_14,
    [SC_OP_BATON_PASS] = SC_ARGS_U8_U8,
    [SC_OP_CREATE_SUBSTITUTE] = SC_ARGS_U8_U16,
    [SC_OP_DELETE_SUBSTITUTE] = SC_ARGS_U8,
    [SC_OP_ADD_SHOOTER_ENERGY] = SC_ARGS_4_4,
    [SC_OP_SETUP_ILLUSION] = SC_ARGS_U8,
    [SC_OP_CLEAR_CONSUMED_ITEM] = SC_ARGS_U8,
    [SC_OP_CURE_MON_DEPENDENT_CONDITIONS] = SC_ARGS_U8_U8,
    [SC_OP_ADD_DAMAGE_RECORD] = SC_ARGS_5_5_5_5_U16_U16,
    [SC_OP_TURN_CHECK] = SC_ARGS_U8,
    [SC_OP_FIELD_TURN_CHECK] = SC_ARGS_U8,
    [SC_MOVE_ANIM] = SC_ARGS_5_5_14_U8,
    [SC_SET_MON_VISIBLE] = SC_ARGS_5_3,
    [SC_DAMAGE] = SC_ARGS_U8_U8_U16,
    [SC_DAMAGE_PLURAL] = SC_ARGS_U8_U8_U16,
    [SC_ONE_HIT_KO] = SC_ARGS_U8,
    [SC_SET_GAUGE_STATUS] = SC_ARGS_U8_U8,
    [SC_CONFUSION_DAMAGE] = SC_ARGS_U8,
    [SC_STAT_UP_EFFECT] = SC_ARGS_5_5_5,
    [SC_STAT_DOWN_EFFECT] = SC_ARGS_5_5_5,
    [SC_FAINT] = SC_ARGS_U8,
    [SC_SHOW_MON] = SC_ARGS_U8,
    [SC_RECALL_MSG] = SC_ARGS_U8_U8,
    [SC_RECALL] = SC_ARGS_U8_U16,
    [SC_SEND_OUT] = SC_ARGS_5_3_5_3,
    [SC_ADD_EVS] = SC_ARGS_U8X7,
    [SC_WEATHER_START] = SC_ARGS_U8_U8,
    [SC_WEATHER_END] = SC_ARGS_U8,
    [SC_UPDATE_HP_GAUGE] = SC_ARGS_U8,
    [SC_USE_HELD_ITEM] = SC_ARGS_U8,
    [SC_FAINT_HP_GAUGE] = SC_ARGS_5_3,
    [SC_SWAP_POS] = SC_ARGS_5_5_5,
    [SC_EXP_GAIN] = SC_ARGS_U8_U32,
    [SC_BALL_THROW] = SC_ARGS_3_2_1_1_1_U16,
    [SC_BALL_THROW_TRAINER] = SC_ARGS_U8_U16,
    [SC_ROTATION] = SC_ARGS_4_4,
    [SC_CHANGE_ABILITY] = SC_ARGS_U8_U16,
    [SC_SWAP_ABILITY] = SC_ARGS_U8_U8_U16_U16,
    [SC_ILLUSION_BREAK] = SC_ARGS_U8,
    [SC_EFFECT] = SC_ARGS_U16,
    [SC_EFFECT_BY_POS] = SC_ARGS_U8_U16,
    [SC_EFFECT_BETWEEN_POS] = SC_ARGS_U8_U8_U16,
    [SC_CHANGE_FORM] = SC_ARGS_U8_U8,
    [SC_MOVE_TO_CENTER] = SC_ARGS_5_3_5_3,
    [SC_SUBSTITUTE_CREATE] = SC_ARGS_U8,
    [SC_SUBSTITUTE_DELETE] = SC_ARGS_U8,
    [SC_TRANSFORM] = SC_ARGS_U8_U8,
    [SC_DAMAGE_EFFECT] = SC_ARGS_5_3_U16,
    [SC_PLAY_BGM] = SC_ARGS_U16,
    [SC_HIDE_MSG_WINDOW] = SC_ARGS_U8,
    [SC_ABILITY_POPUP_IN] = SC_ARGS_U8,
    [SC_ABILITY_POPUP_OUT] = SC_ARGS_U8,
    [SC_MOVE_MSG] = SC_ARGS_U8_U16,
    [SC_MSG_STD] = SC_ARGS_NONE,
    [SC_MSG_SET] = SC_ARGS_NONE,
    [SC_MSG_STD_SE] = SC_ARGS_MSG_SE,
    [SC_MSG_SET_SE] = SC_ARGS_MSG_SE,
    [SC_NONE] = SC_ARGS_NONE,
    [SC_RESERVE] = SC_ARGS_NONE,
};

// Writes a server command with its arguments, in the widths its format gives them
void BtlServerCmdQueue_Put(BtlServerCmdQueue *que, BtlServerCmd event, ...) {
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
u16 SCQUE_RESERVE_Pos(BtlServerCmdQueue *que, BtlServerCmd event) {
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
    PutU16(que, SC_RESERVE);
    PutU8(que, size - 3);
    que->writePtr = pos + size;
    return pos;
}

// Fills in a command that SCQUE_RESERVE_Pos reserved room for
void BtlServerCmdQueue_PutReserved(BtlServerCmdQueue *que, u32 reserve, BtlServerCmd event, ...) {
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
    if (event != SC_RESERVE) {
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
    while (event == SC_RESERVE) {
        que->readPtr += ReadU8(que);
        if (que->readPtr >= que->writePtr) {
            return SC_NONE;
        }
        event = ReadU16(que);
    }
    format = data_ov167_021d6e50[event];
    if (format != SC_ARGS_NONE && format != SC_ARGS_MSG_SE) {
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

// Writes a message command: the message, then its arguments up to SC_MSG_ARG_END
void BtlServerCmdQueue_PutMsg(BtlServerCmdQueue *que, u8 event, ...) {
    va_list list;
    u16 message;
    u32 arg;

    va_start(list, event);
    message = va_arg(list, u32);
    PutU16(que, event);
    PutU16(que, message);
    if (event == SC_MSG_STD_SE) {
        PutU16(que, va_arg(list, u32));
    }
    do {
        arg = va_arg(list, u32);
        PutU32(que, arg);
    } while (arg != SC_MSG_ARG_END);
    va_end(list);
}

// Reads a message command's arguments
static void ReadMsgArgs(BtlServerCmdQueue *que, u8 event, u32 *args) {
    s32 i = 1;

    args[0] = ReadU16(que);
    if (event == SC_MSG_STD_SE) {
        args[1] = ReadU16(que);
        i++;
    }
    for (; i < 16; i++) {
        args[i] = ReadU32(que);
        if (args[i] == SC_MSG_ARG_END) {
            break;
        }
    }
}

void func_ov167_021b1670(void) {
}
