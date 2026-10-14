// NitroSDK libmath's math_crc.c: CRC-8, CRC-16 and CRC-32 through 256-entry tables.
// The split from math_sha1.c and math.c is certain (a run of table and update functions between them).
#include "nitro/math.h"

static void MATHi_CRC8Update(const MATHCRC8Table *table, u8 *crc, const void *data, u32 size);
static void MATHi_CRC16Update(const MATHCRC16Table *table, u16 *crc, const void *data, u32 size);
static void MATHi_CRC32UpdateRev(const MATHCRC32Table *table, u32 *crc, const void *data, u32 size);

void MATHi_CRC8InitTable(MATHCRC8Table *table, u8 poly) {
    u32 v;
    u32 i;
    u32 j;

    for (i = 0; i < 256; i++) {
        v = i;
        for (j = 0; j < 8; j++) {
            if (v & 0x80) {
                v = (v << 1) ^ poly;
            } else {
                v = v << 1;
            }
        }
        table->table[i] = v;
    }
}

static void MATHi_CRC8Update(const MATHCRC8Table *table, u8 *crc, const void *data, u32 size) {
    const u8 *p = data;
    u32 v = *crc;
    u32 i;

    for (i = 0; i < size; i++) {
        v = table->table[(*p++ ^ v) & 0xff];
    }
    *crc = v;
}

void MATHi_CRC16InitTable(MATHCRC16Table *table, u16 poly) {
    u32 v;
    u32 i;
    u32 j;

    for (i = 0; i < 256; i++) {
        v = i << 8;
        for (j = 0; j < 8; j++) {
            if (v & 0x8000) {
                v = (v << 1) ^ poly;
            } else {
                v = v << 1;
            }
        }
        table->table[i] = v;
    }
}

void MATHi_CRC16InitTableRev(MATHCRC16Table *table, u16 poly) {
    u32 v;
    u32 i;
    u32 j;

    for (i = 0; i < 256; i++) {
        v = i;
        for (j = 0; j < 8; j++) {
            if (v & 1) {
                v = (v >> 1) ^ poly;
            } else {
                v = v >> 1;
            }
        }
        table->table[i] = v;
    }
}

static void MATHi_CRC16Update(const MATHCRC16Table *table, u16 *crc, const void *data, u32 size) {
    const u8 *p = data;
    u32 v = *crc;
    u32 i;

    for (i = 0; i < size; i++) {
        v = (v << 8) ^ table->table[((v >> 8) ^ *p++) & 0xff];
    }
    *crc = v;
}

void MATHi_CRC16UpdateRev(const MATHCRC16Table *table, u16 *crc, const void *data, u32 size) {
    const u8 *p = data;
    u32 v = *crc;
    u32 i;

    for (i = 0; i < size; i++) {
        v = (v >> 8) ^ table->table[(*p++ ^ v) & 0xff];
    }
    *crc = v;
}

void MATHi_CRC32InitTableRev(MATHCRC32Table *table, u32 poly) {
    u32 v;
    u32 i;
    u32 j;

    for (i = 0; i < 256; i++) {
        v = i;
        for (j = 0; j < 8; j++) {
            if (v & 1) {
                v = (v >> 1) ^ poly;
            } else {
                v = v >> 1;
            }
        }
        table->table[i] = v;
    }
}

static void MATHi_CRC32UpdateRev(const MATHCRC32Table *table, u32 *crc, const void *data, u32 size) {
    const u8 *p = data;
    u32 v = *crc;
    u32 i;

    for (i = 0; i < size; i++) {
        v = (v >> 8) ^ table->table[(*p ^ v) & 0xff];
        p++;
    }
    *crc = v;
}

u8 MATH_CalcCRC8(const MATHCRC8Table *table, const void *data, u32 size) {
    u8 crc = 0;

    MATHi_CRC8Update(table, &crc, data, size);
    return crc;
}

u16 MATH_CalcCRC16(const MATHCRC16Table *table, const void *data, u32 size) {
    u16 crc = 0;

    MATHi_CRC16UpdateRev(table, &crc, data, size);
    return crc;
}

u16 MATH_CalcCRC16CCITT(const MATHCRC16Table *table, const void *data, u32 size) {
    u16 crc = 0xffff;

    MATHi_CRC16Update(table, &crc, data, size);
    return crc;
}

u32 MATH_CalcCRC32(const MATHCRC32Table *table, const void *data, u32 size) {
    u32 crc = ~0;

    MATHi_CRC32UpdateRev(table, &crc, data, size);
    return ~crc;
}
