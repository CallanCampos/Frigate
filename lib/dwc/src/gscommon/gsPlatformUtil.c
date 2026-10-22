#include "dwc/gsPlatformUtil.h"
#include "dwc/ext.h"

// GameSpy's helpers (gsPlatformUtil.c). The ROM names neither the file nor its functions: the file is the later SDK's,
// and the functions are GameSpy's as pret's pokerevo Wii build has them (nonport.s), or the later SDK's
// (gsiStringToInt64, gsiInt64ToString, gsiSecondsToDate, gsiDateToSeconds, QuartToTrip, TripToQuart, B64Decode,
// B64DecodeLen, the stream, nextlongrand, longrand and the statics, urlSafeEncodeing in its spelling)

static gsi_u32 nextlongrand(gsi_u32 seed);
static gsi_u32 longrand(void);
static void QuartToTrip(const char *quart, char *trip, int count);
static void TripToQuart(const char *trip, char *quart, int count);

// The last two digits and the padding of base 64's three alphabets, defined in the reverse order of the ROM's, as MWCC
// lays out these statics backwards
static const char alternateEncoding[3] = { '[', ']', '_' };
static const char urlSafeEncodeing[3] = { '-', '_', '=' };
static const char defaultEncoding[3] = { '+', '/', '=' };
// The names of the days and the months, which no code of the ROM reads
const char _dnames[] = "SunMonTueWedThuFriSat";
const char _mnames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

static gsi_u32 randomnum = 1;
// The days before each month, less one, in a leap year and in another year
static int _lpdays[13] = { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
static int _days[13] = { -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364 };
static struct tm tb;

gsi_i64 gsiStringToInt64(const char *text) {
    return atoll(text);
}

void gsiInt64ToString(char text[33], gsi_i64 value) {
    OS_SPrintf(text, "%lld", value);
}

// The lookup of a host: the name, which the DS's lookup doesn't keep, and the address
typedef struct GSIResolveHostnameInfo {
    char *hostname;
    unsigned int ip;
} GSIResolveHostnameInfo;

int gsiStartResolvingHostname(const char *host, GSIResolveHostnameHandle *lookup) {
    HOSTENT *entry;
    GSIResolveHostnameInfo *resolve;

    gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_State, GSIDebugLevel_Comment,
                  "(NON-Asynchrounous) DNS lookup starting\n");
    entry = SOC_GetHostByName(host);
    if (entry == NULL) {
        return -1;
    }
    resolve = gsimalloc(sizeof(GSIResolveHostnameInfo));
    if (resolve == NULL) {
        return -1;
    }
    resolve->ip = *(unsigned int *)entry->addrList[0];
    *lookup = resolve;
    return 0;
}

void gsiCancelResolvingHostname(GSIResolveHostnameHandle lookup) {
    gsifree(lookup);
}

unsigned int gsiGetResolvedIP(GSIResolveHostnameHandle lookup) {
    unsigned int ip = lookup->ip;

    gsifree(lookup);
    return ip;
}

char *goastrdup(const char *text) {
    int length;
    char *copy;

    if (text == NULL) {
        return NULL;
    }
    length = strlen(text);
    copy = gsimalloc(length + 1);
    if (copy != NULL) {
        memcpy(copy, text, length + 1);
    }
    return copy;
}

char *_strlwr(char *text) {
    char *start = text;

    while (*text != '\0') {
        *text = tolower(*text);
        text++;
    }
    return start;
}

void SocketStartUp(void) {
}

void SocketShutDown(void) {
}

gsi_time current_time(void) {
    return OS_TicksToMilliSeconds(OS_GetTick());
}

gsi_time current_time_hires(void) {
    return OS_TicksToMicroSeconds(OS_GetTick());
}

void msleep(gsi_time milliseconds) {
    OS_Sleep(milliseconds);
}

// Whether a sum or a product of two values wrapped around
#define ADD_OVERFLOWS(a, b, sum) (((a) >= 0 && (b) >= 0 && (sum) < 0) || ((a) < 0 && (b) < 0 && (sum) >= 0))
#define MUL_OVERFLOWS(a, b, product) ((a) ? ((product) / (a) != (b)) : 0)

struct tm *gsiSecondsToDate(const time_t *seconds) {
    time_t time = *seconds;
    gsi_bool leap = gsi_false;
    struct tm *date = &tb;
    int year;
    int yday;
    int month;
    int *days;

    if (time < 0) {
        return NULL;
    }
    // Four years from 1970, with one leap year, and the years of them
    year = time / 126230400;
    time -= year * 126230400;
    year = year * 4 + 70;
    if (time >= 31536000) {
        time -= 31536000;
        year++;
        if (time >= 31536000) {
            time -= 31536000;
            year++;
            if (time >= 31622400) {
                year++;
                time -= 31622400;
            } else {
                leap++;
            }
        }
    }
    date->tm_year = year;
    yday = time / 86400;
    date->tm_yday = yday;
    time -= yday * 86400;
    days = leap ? _lpdays : _days;
    month = 1;
    while (days[month] < yday) {
        month++;
    }
    date->tm_mon = month - 1;
    date->tm_mday = date->tm_yday - days[date->tm_mon];
    date->tm_wday = (*seconds / 86400 + 4) % 7;
    date->tm_hour = time / 3600;
    time -= date->tm_hour * 3600;
    date->tm_min = time / 60;
    date->tm_sec = time - date->tm_min * 60;
    date->tm_isdst = 0;
    return date;
}

time_t gsiDateToSeconds(struct tm *date) {
    time_t value = date->tm_year;
    int days;
    int sum;
    int addend;
    struct tm *result;

    if (value < 69 || value > 139) {
        return -1;
    }
    if (date->tm_mon < 0 || date->tm_mon > 11) {
        value += date->tm_mon / 12;
        date->tm_mon = date->tm_mon % 12;
        if (date->tm_mon < 0) {
            date->tm_mon += 12;
            value--;
        }
        if (value < 69 || value > 139) {
            return -1;
        }
    }
    days = _days[date->tm_mon];
    if ((value & 3) == 0 && date->tm_mon > 1) {
        days++;
    }
    sum = (value - 1 >> 2) + 365 * (value - 70) - 17;
    sum += days;
    addend = date->tm_mday;
    value = sum + addend;
    if (ADD_OVERFLOWS(sum, addend, value)) {
        return -1;
    }
    sum = value * 24;
    if (MUL_OVERFLOWS(value, 24, sum)) {
        return -1;
    }
    addend = date->tm_hour;
    value = sum + addend;
    if (ADD_OVERFLOWS(sum, addend, value)) {
        return -1;
    }
    sum = value * 60;
    if (MUL_OVERFLOWS(value, 60, sum)) {
        return -1;
    }
    addend = date->tm_min;
    value = sum + addend;
    if (ADD_OVERFLOWS(sum, addend, value)) {
        return -1;
    }
    sum = value * 60;
    if (MUL_OVERFLOWS(value, 60, sum)) {
        return -1;
    }
    addend = date->tm_sec;
    value = sum + addend;
    if (ADD_OVERFLOWS(sum, addend, value)) {
        return -1;
    }
    result = gsiSecondsToDate(&value);
    if (result == NULL) {
        return -1;
    }
    *date = *result;
    return value;
}

// The next number of the Park-Miller generator (16807 times the seed, modulo 2^31 - 1), without a division
static gsi_u32 nextlongrand(gsi_u32 seed) {
    gsi_u32 lo;
    gsi_u32 hi;

    lo = 16807 * (seed & 0xffff);
    hi = 16807 * (seed >> 16);
    lo += (hi & 0x7fff) << 16;
    if (lo > 0x7fffffff) {
        lo &= 0x7fffffff;
        lo++;
    }
    lo += hi >> 15;
    if (lo > 0x7fffffff) {
        lo &= 0x7fffffff;
        lo++;
    }
    return lo;
}

static gsi_u32 longrand(void) {
    return randomnum = nextlongrand(randomnum);
}

void Util_RandSeed(unsigned long seed) {
    randomnum = seed != 0 ? seed & 0x7fffffff : 1;
}

int Util_RandInt(int low, int high) {
    gsi_u32 range = high - low;
    int value;

    if (range == 0) {
        return low;
    }
    value = longrand() % range;
    return value + low;
}

// Turns up to four base 64 digits (values from 0 to 63) into bytes, one less than the digits
static void QuartToTrip(const char *quart, char *trip, int count) {
    if (count >= 2) {
        trip[0] = (quart[0] << 2) | (quart[1] >> 4);
    }
    if (count >= 3) {
        trip[1] = ((quart[1] & 0xf) << 4) | (quart[2] >> 2);
    }
    if (count >= 4) {
        trip[2] = ((quart[2] & 3) << 6) | quart[3];
    }
}

// Turns up to three bytes into four base 64 digits (values from 0 to 63)
static void TripToQuart(const char *trip, char *quart, int count) {
    gsi_u8 triple[3];
    int i;

    for (i = 0; i < count; i++) {
        triple[i] = trip[i];
    }
    while (i < 3) {
        triple[i] = 0;
        i++;
    }
    quart[0] = triple[0] >> 2;
    quart[1] = ((triple[0] & 3) << 4) | (triple[1] >> 4);
    quart[2] = ((triple[1] & 0xf) << 2) | (triple[2] >> 6);
    quart[3] = triple[2] & 0x3f;
}

void B64Decode(const char *input, char *output, int length, int *outputLength, int encodingType) {
    int i = 0;
    int written = 0;
    const char *encoding;
    char quart[4];

    switch (encodingType) {
    case 1:
        encoding = alternateEncoding;
        break;
    case 2:
        encoding = urlSafeEncodeing;
        break;
    default:
        encoding = defaultEncoding;
        break;
    }
    if (length <= 0) {
        if (outputLength != NULL) {
            *outputLength = 0;
        }
        output[0] = '\0';
        return;
    }
    while (i < length && input[i] != encoding[2]) {
        char c = input[i];

        if (c >= '0' && c <= '9') {
            quart[i % 4] = c - '0' + 52;
        } else if (c >= 'a' && c <= 'z') {
            quart[i % 4] = c - 'a' + 26;
        } else if (c >= 'A' && c <= 'Z') {
            quart[i % 4] = c - 'A';
        } else if (encoding[0] == c) {
            quart[i % 4] = 62;
        } else if (encoding[1] == c) {
            quart[i % 4] = 63;
        } else if (encoding[2] == input[i] || input[i] == '\0') {
            break;
        } else {
            if (outputLength != NULL) {
                *outputLength = 0;
            }
            output[0] = '\0';
            return;
        }
        if (i % 4 == 3) {
            QuartToTrip(quart, output + written, 4);
            written += 3;
        }
        i++;
    }
    if (i != 0) {
        i = i % 4;
        if (i != 0) {
            memset(quart + i, encoding[2], 4 - i);
            QuartToTrip(quart, output + written, i);
            if (i == 3) {
                written += 2;
            } else {
                written += 1;
            }
        }
    }
    if (outputLength != NULL) {
        *outputLength = written;
    }
}

void B64Encode(const char *input, char *output, int length, int encodingType) {
    char *start = output;
    int total = length;
    int left = length;
    char *padding;
    const char *encoding;
    int count;

    switch (encodingType) {
    case 1:
        encoding = alternateEncoding;
        break;
    case 2:
        encoding = urlSafeEncodeing;
        break;
    default:
        encoding = defaultEncoding;
        break;
    }
    while (left > 0) {
        count = left;
        if (left >= 3) {
            count = 3;
        }
        TripToQuart(input, output, count);
        left -= 3;
        output += 4;
        input += 3;
    }
    padding = output;
    if (total % 3 == 1) {
        padding = output - 2;
    } else if (total % 3 == 2) {
        padding = output - 1;
    }
    *output = '\0';
    while (output > start) {
        output--;
        if (output >= padding) {
            *output = encoding[2];
        } else if (*output <= 25) {
            *output += 'A';
        } else if (*output <= 51) {
            *output += 'a' - 26;
        } else if (*output <= 61) {
            *output += '0' - 52;
        } else if (*output == 62) {
            *output = encoding[0];
        } else if (*output == 63) {
            *output = encoding[1];
        }
    }
}

int B64DecodeLen(const char *input, int encodingType) {
    const char *encoding;
    const char *start = input;
    char pad;

    switch (encodingType) {
    case 1:
        encoding = alternateEncoding;
        break;
    case 2:
        encoding = urlSafeEncodeing;
        break;
    default:
        encoding = defaultEncoding;
        break;
    }
    if (*input != '\0') {
        pad = encoding[2];
        do {
            if (*input == pad) {
                int count = input - start;

                return (count - 1) % 4 + count / 4 * 3;
            }
            input++;
        } while (*input != '\0');
    }
    return (input - start) / 4 * 3;
}

void B64InitEncodeStream(B64StreamData *data, const char *input, int length, int encodingType) {
    data->input = input;
    data->len = length;
    data->encodingType = encodingType;
}

gsi_bool B64EncodeStream(B64StreamData *data, char output[4]) {
    const char *encoding;
    int i;

    if (data->len <= 0) {
        return gsi_false;
    }
    switch (data->encodingType) {
    case 1:
        encoding = alternateEncoding;
        break;
    case 2:
        encoding = urlSafeEncodeing;
        break;
    default:
        encoding = defaultEncoding;
        break;
    }
    TripToQuart(data->input, output, data->len >= 3 ? 3 : data->len);
    data->input += 3;
    data->len -= 3;
    for (i = 0; i < 4; i++) {
        if (output[i] <= 25) {
            output[i] += 'A';
        } else if (output[i] <= 51) {
            output[i] += 'a' - 26;
        } else if (output[i] <= 61) {
            output[i] += '0' - 52;
        } else if (output[i] == 62) {
            output[i] = encoding[0];
        } else if (output[i] == 63) {
            output[i] = encoding[1];
        }
    }
    if (data->len < 0) {
        output[3] = encoding[2];
        if (data->len == -2) {
            output[2] = encoding[2];
        }
    }
    return gsi_true;
}
