#ifndef POKEBW2_DWC_GSPLATFORMUTIL_H
#define POKEBW2_DWC_GSPLATFORMUTIL_H

#include "dwc/nonport.h"

// GameSpy's helpers (gsPlatformUtil.c), in overlay 11: 64-bit numbers as texts, the lookup of a host, texts, the clock,
// dates, the generator of random numbers and base 64. A base 64 text has one of three alphabets, which differ in
// their last two digits and their padding: encodingType 0 (+, / and =), 1 ([, ] and _) or 2 (-, _ and =)

// A date as the C library's struct tm has it: the year from 1900, the month from 0 and the day of the year from 0
typedef struct GSTime {
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    int isdst;
} GSTime;

// The state of a base 64 encoding that gives four digits at a time: what is left of the input, and the alphabet
typedef struct B64StreamData {
    const char *input;
    int length;
    int encodingType;
} B64StreamData;

gsi_i64 gsiStringToInt64(const char *text);
int gsiInt64ToString(char *text, gsi_i64 value);
// Looks up a host, at once: 0 with a lookup that gsiGetResolvedIP reads (and frees), or -1; and frees a lookup
int gsiStartResolvingHostname(const char *host, void **lookup);
void gsiCancelResolvingHostname(void *lookup);
int gsiGetResolvedIP(void *lookup);
// A copy of a text that it allocates, or NULL; and a text made lower case in place, which it returns
char *goastrdup(const char *text);
char *_strlwr(char *text);
// Start and end the sockets, which needs nothing on the DS
void SocketStartUp(void);
void SocketShutDown(void);
// The clock in milliseconds and in microseconds, and a sleep
gsi_u32 current_time(void);
gsi_u32 current_time_hires(void);
void msleep(gsi_u32 milliseconds);
// The date of some seconds from 1970 (in a buffer that the next call overwrites, NULL before 1970), and the seconds
// of a date, which it also corrects (-1 for a year before 1969 or after 2039)
GSTime *gsiSecondsToDate(const int *seconds);
int gsiDateToSeconds(GSTime *date);
// Seeds the generator, and gives a number from low up to (not with) high
void Util_RandSeed(gsi_u32 seed);
int Util_RandInt(int low, int high);
// Decodes a base 64 text of a length, writing the length of the bytes unless that is NULL; encodes bytes as a text
// that it ends; the length of the bytes of a text; and the encoding as a stream, four digits at a time, which gives
// gsi_false once the input has ended
void B64Decode(const char *input, char *output, int length, int *outputLength, int encodingType);
void B64Encode(const char *input, char *output, int length, int encodingType);
int B64DecodeLen(const char *input, int encodingType);
void B64InitEncodeStream(B64StreamData *data, const char *input, int length, int encodingType);
gsi_bool B64EncodeStream(B64StreamData *data, char output[4]);

#endif // POKEBW2_DWC_GSPLATFORMUTIL_H
