#include "ssp_exifenc.h"
#include "types.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nitro/rtc.h"

// The EXIF writer of TwlSDK's JPEG encoder: the SOI and APP1 segment that start a photo, with its 0th IFD, Exif IFD,
// MakerNote IFD (which holds what the DSi signs), Interoperability IFD and the 1st IFD of its thumbnail. The file name
// is a guess after TwlSDK's ssp/exifenc.h; the date, software and maker note setters were dead-stripped from the game

// EXIF types
enum {
    EXIF_TYPE_BYTE = 1,
    EXIF_TYPE_ASCII,
    EXIF_TYPE_SHORT,
    EXIF_TYPE_LONG,
    EXIF_TYPE_RATIONAL,
    EXIF_TYPE_UNDEFINED = 7,
    EXIF_TYPE_SLONG = 9,
    EXIF_TYPE_SRATIONAL,
};

// EXIF tags
#define EXIF_TAG_INTEROP_INDEX 0x1
#define EXIF_TAG_INTEROP_VERSION 0x2
#define EXIF_TAG_COMPRESSION 0x103
#define EXIF_TAG_MAKE 0x10f
#define EXIF_TAG_MODEL 0x110
#define EXIF_TAG_X_RESOLUTION 0x11a
#define EXIF_TAG_Y_RESOLUTION 0x11b
#define EXIF_TAG_RESOLUTION_UNIT 0x128
#define EXIF_TAG_SOFTWARE 0x131
#define EXIF_TAG_DATE_TIME 0x132
#define EXIF_TAG_JPEG_INTERCHANGE_FORMAT 0x201
#define EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH 0x202
#define EXIF_TAG_YCBCR_POSITIONING 0x213
#define EXIF_TAG_RELATED_IMAGE_FILE_FORMAT 0x1000
#define EXIF_TAG_EXIF_IFD 0x8769
#define EXIF_TAG_EXIF_VERSION 0x9000
#define EXIF_TAG_DATE_TIME_ORIGINAL 0x9003
#define EXIF_TAG_DATE_TIME_DIGITIZED 0x9004
#define EXIF_TAG_COMPONENTS_CONFIGURATION 0x9101
#define EXIF_TAG_MAKER_NOTE 0x927c
#define EXIF_TAG_FLASHPIX_VERSION 0xa000
#define EXIF_TAG_COLOR_SPACE 0xa001
#define EXIF_TAG_PIXEL_X_DIMENSION 0xa002
#define EXIF_TAG_PIXEL_Y_DIMENSION 0xa003
#define EXIF_TAG_INTEROP_IFD 0xa005

// The tags of the MakerNote IFD: the data the DSi signs, and two blocks of the application's
#define MAKER_NOTE_TAG_SIGNATURE 0x1000
#define MAKER_NOTE_TAG_DATA_1 0x1001
#define MAKER_NOTE_TAG_DATA_2 0x1002

// The size of the signature, and of the longest value an entry holds
#define SIGNATURE_SIZE 28
#define EXIF_VALUE_MAX 30

// The IFDs, in the order they are laid out
enum {
    EXIF_IFD_0TH,
    EXIF_IFD_EXIF,
    EXIF_IFD_MAKER_NOTE,
    EXIF_IFD_INTEROP,
    EXIF_IFD_1ST,
    EXIF_IFD_COUNT,
};

// The index of the MakerNote entry in the Exif IFD
#define EXIF_MAKER_NOTE_INDEX 4

typedef struct {
    u16 tag;
    u16 type;
    u32 count;
    s8 byteValue;
    s16 shortValue;
    s32 longValue;
    s32 numerator;
    s32 denominator;
    char undefinedValue[EXIF_VALUE_MAX];
    char asciiValue[EXIF_VALUE_MAX];
    // The value's offset from the TIFF header, when it is longer than 4 bytes
    u32 offset;
} ExifEntry;

typedef struct {
    ExifEntry *entries;
    s16 count;
    // The offset of the IFD's end from the TIFF header, where the next one starts
    u32 offset;
} ExifIfd;

typedef struct {
    ExifIfd ifd[EXIF_IFD_COUNT];
    // The size of the APP1 segment
    u32 size;
    u32 unk40;
    u32 thumbnailSize;
} ExifIfdSet;

static void ExifEnc_SetDateTime(const char *dateTime);
static BOOL ExifEnc_GetDateTime(RTCDate *date, RTCTime *time);
static BOOL ExifEnc_SetCurrentDateTime(void);
static void ExifEnc_SetSoftware(const char *software);
static u32 ExifEnc_GetValueSize(const ExifEntry *entry);
static void ExifEnc_WriteValue(ExifEntry *entry);
static void ExifEnc_WriteMakerNoteIfd(ExifIfdSet *set, ExifIfd *ifd, u8 last);
static void ExifEnc_WriteIfd(ExifIfdSet *set, ExifIfd *ifd, u8 last);
static BOOL ExifEnc_WriteApp1(ExifIfdSet *set, u8 noThumbnail, const u8 *thumbnail, u32 thumbnailSize);
static BOOL ExifEnc_BuildIfds(s16 width, s16 height, BOOL thumbnail, const u8 *thumbnailData, u32 thumbnailSize);

// The file's variables, declared in an order that lays them out as the game has them: MWCC sorts them by size
static u32 sExifEncMakerNoteData1Size;
static u8 *sExifEncMakerNoteData2;
static u32 sExifEncWritePos;
static u32 sExifEncMakerNoteData2Size;
static BOOL sExifEncDateTimeSet;
static u8 *sExifEncMakerNoteData1;
static u32 sExifEncSoftwareLength;
u32 ExifEnc_SignMode;
static u8 *sExifEncWriteBuffer;
static char sExifEncDateTime[20];
static char sExifEncSoftware[32];
static ExifIfdSet sExifEncIfdSet;
static ExifEntry sExifEncSignatureEntry;
static ExifEntry sExifEncInteropIfdEntries[12];
static ExifEntry sExifEncMakerNoteIfdEntries[12];
static ExifEntry sExifEncIfd0Entries[13];
static ExifEntry sExifEncExifIfdEntries[14];
static ExifEntry sExifEncIfd1Entries[14];

// The setter of the sign flag, which the game doesn't call. Its reference keeps the flag among the file's other
// variables, as the dead-stripped setters did
static inline void SetSignMode(BOOL sign) {
    ExifEnc_SignMode = sign;
}

#define WRITE_BYTE(value) (sExifEncWriteBuffer[sExifEncWritePos++] = (value))

static void ExifEnc_SetDateTime(const char *dateTime) {
    MI_CpuCopy(dateTime, sExifEncDateTime, 19);
    sExifEncDateTime[19] = '\0';
    sExifEncDateTimeSet = TRUE;
}

static BOOL ExifEnc_GetDateTime(RTCDate *date, RTCTime *time) {
    if (RTC_GetDateTime(date, time) == 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ExifEnc_SetCurrentDateTime(void) {
    RTCDate date;
    RTCTime time;
    char dateTime[20];

    if (ExifEnc_GetDateTime(&date, &time) == TRUE) {
        STD_TSPrintf(dateTime, "%04d:%02d:%02d %02d:%02d:%02d", date.year + 2000, date.month, date.day, time.hour,
                     time.minute, time.second);
        ExifEnc_SetDateTime(dateTime);
        return TRUE;
    }
    return FALSE;
}

static void ExifEnc_SetSoftware(const char *software) {
    sExifEncSoftwareLength = STD_CopyLString(sExifEncSoftware, software, EXIF_VALUE_MAX) + 1;
}

static u32 ExifEnc_GetValueSize(const ExifEntry *entry) {
    s8 typeSizes[] = { 0, 1, 1, 2, 4, 8, 0, 1, 0, 4, 8 };

    return typeSizes[entry->type] * entry->count;
}

static void ExifEnc_WriteValue(ExifEntry *entry) {
    u32 size;
    u32 i;

    switch (entry->type) {
    case EXIF_TYPE_BYTE:
        WRITE_BYTE(entry->byteValue);
        WRITE_BYTE(0);
        WRITE_BYTE(0);
        WRITE_BYTE(0);
        break;
    case EXIF_TYPE_SHORT:
        WRITE_BYTE((entry->shortValue & 0xff00) >> 8);
        WRITE_BYTE(entry->shortValue);
        WRITE_BYTE(0);
        WRITE_BYTE(0);
        break;
    case EXIF_TYPE_LONG:
    case EXIF_TYPE_SLONG:
        WRITE_BYTE((entry->longValue & 0xff000000) >> 24);
        WRITE_BYTE((entry->longValue & 0xff0000) >> 16);
        WRITE_BYTE((entry->longValue & 0xff00) >> 8);
        WRITE_BYTE(entry->longValue);
        break;
    case EXIF_TYPE_RATIONAL:
    case EXIF_TYPE_SRATIONAL:
        WRITE_BYTE((entry->numerator & 0xff000000) >> 24);
        WRITE_BYTE((entry->numerator & 0xff0000) >> 16);
        WRITE_BYTE((entry->numerator & 0xff00) >> 8);
        WRITE_BYTE(entry->numerator);
        WRITE_BYTE((entry->denominator & 0xff000000) >> 24);
        WRITE_BYTE((entry->denominator & 0xff0000) >> 16);
        WRITE_BYTE((entry->denominator & 0xff00) >> 8);
        WRITE_BYTE(entry->denominator);
        break;
    case EXIF_TYPE_UNDEFINED:
        size = ExifEnc_GetValueSize(entry);
        for (i = 0; i < size; i++) {
            WRITE_BYTE(entry->undefinedValue[i]);
        }
        for (; i < 4; i++) {
            WRITE_BYTE(0);
        }
        break;
    case EXIF_TYPE_ASCII:
        size = ExifEnc_GetValueSize(entry);
        for (i = 0; i < size; i++) {
            WRITE_BYTE(entry->asciiValue[i]);
        }
        for (; i < 4; i++) {
            WRITE_BYTE(0);
        }
        break;
    }
}

static void ExifEnc_WriteMakerNoteIfd(ExifIfdSet *set, ExifIfd *ifd, u8 last) {
    ExifEntry *entry;
    s8 i;
    u32 size;
    u32 next;
    u32 j;

    WRITE_BYTE((ifd->count & 0xff00) >> 8);
    WRITE_BYTE(ifd->count);
    next = ifd->offset;
    entry = ifd->entries;
    for (i = 0; i < ifd->count; i++) {
        WRITE_BYTE((entry->tag & 0xff00) >> 8);
        WRITE_BYTE(entry->tag);
        WRITE_BYTE((entry->type & 0xff00) >> 8);
        WRITE_BYTE(entry->type);
        WRITE_BYTE((entry->count & 0xff000000) >> 24);
        WRITE_BYTE((entry->count & 0xff0000) >> 16);
        WRITE_BYTE((entry->count & 0xff00) >> 8);
        WRITE_BYTE(entry->count);
        size = ExifEnc_GetValueSize(entry);
        if (size <= 4) {
            if (size == 2) {
                WRITE_BYTE(0);
                WRITE_BYTE(0);
            }
            // BUG: The loops below reuse the IFD's entry index, so a maker note block of up to 4 bytes skips entries
#ifdef BUGFIX
            if (entry->tag == MAKER_NOTE_TAG_DATA_1) {
                for (j = 0; j < sExifEncMakerNoteData1Size; j++) {
                    WRITE_BYTE(sExifEncMakerNoteData1[j]);
                }
            } else if (entry->tag == MAKER_NOTE_TAG_DATA_2) {
                for (j = 0; j < sExifEncMakerNoteData2Size; j++) {
                    WRITE_BYTE(sExifEncMakerNoteData2[j]);
                }
            }
#else
            if (entry->tag == MAKER_NOTE_TAG_DATA_1) {
                for (i = 0; i < sExifEncMakerNoteData1Size; i++) {
                    WRITE_BYTE(sExifEncMakerNoteData1[i]);
                }
            } else if (entry->tag == MAKER_NOTE_TAG_DATA_2) {
                for (i = 0; i < sExifEncMakerNoteData2Size; i++) {
                    WRITE_BYTE(sExifEncMakerNoteData2[i]);
                }
            }
#endif
        } else {
            WRITE_BYTE((entry->offset & 0xff000000) >> 24);
            WRITE_BYTE((entry->offset & 0xff0000) >> 16);
            WRITE_BYTE((entry->offset & 0xff00) >> 8);
            WRITE_BYTE(entry->offset);
        }
        entry++;
    }

    if (!last) {
        WRITE_BYTE((s16)(next & 0xff000000) >> 24);
        WRITE_BYTE((s16)(next & 0xff0000) >> 16);
        WRITE_BYTE((s16)(next & 0xff00) >> 8);
        WRITE_BYTE(next);
    } else {
        WRITE_BYTE(0);
        WRITE_BYTE(0);
        WRITE_BYTE(0);
        WRITE_BYTE(0);
    }

    entry = ifd->entries;
    for (i = 0; i < ifd->count; i++) {
        size = ExifEnc_GetValueSize(entry);
        if (size > 4) {
            if (entry->tag == MAKER_NOTE_TAG_SIGNATURE) {
                ExifEnc_WriteValue(entry);
                if (size & 1) {
                    WRITE_BYTE(0);
                }
            } else if (entry->tag == MAKER_NOTE_TAG_DATA_1) {
                for (j = 0; j < sExifEncMakerNoteData1Size; j++) {
                    WRITE_BYTE(sExifEncMakerNoteData1[j]);
                }
            } else if (entry->tag == MAKER_NOTE_TAG_DATA_2) {
                for (j = 0; j < sExifEncMakerNoteData2Size; j++) {
                    WRITE_BYTE(sExifEncMakerNoteData2[j]);
                }
            }
        }
        entry++;
    }
}

static void ExifEnc_WriteIfd(ExifIfdSet *set, ExifIfd *ifd, u8 last) {
    ExifEntry *entry;
    s8 i;
    s32 next;
    u32 size;

    WRITE_BYTE((ifd->count & 0xff00) >> 8);
    WRITE_BYTE(ifd->count);
    next = ifd->offset;
    entry = ifd->entries;
    for (i = 0; i < ifd->count; i++) {
        switch (entry->tag) {
        case EXIF_TAG_JPEG_INTERCHANGE_FORMAT:
            entry->longValue = set->size;
            entry->longValue -= set->thumbnailSize;
            entry->longValue -= 2;
            entry->longValue -= 6;
            break;
        case EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH:
            entry->longValue = set->thumbnailSize;
            break;
        case EXIF_TAG_EXIF_IFD:
            entry->longValue = set->ifd[EXIF_IFD_0TH].offset;
            next = set->ifd[EXIF_IFD_INTEROP].offset;
            break;
        case EXIF_TAG_MAKER_NOTE:
            entry->offset = set->ifd[EXIF_IFD_EXIF].offset;
            next = set->ifd[EXIF_IFD_INTEROP].offset;
            break;
        case EXIF_TAG_INTEROP_IFD:
            entry->longValue = set->ifd[EXIF_IFD_MAKER_NOTE].offset;
            next = set->ifd[EXIF_IFD_INTEROP].offset;
            break;
        }
        WRITE_BYTE((entry->tag & 0xff00) >> 8);
        WRITE_BYTE(entry->tag);
        WRITE_BYTE((entry->type & 0xff00) >> 8);
        WRITE_BYTE(entry->type);
        WRITE_BYTE((entry->count & 0xff000000) >> 24);
        WRITE_BYTE((entry->count & 0xff0000) >> 16);
        WRITE_BYTE((entry->count & 0xff00) >> 8);
        WRITE_BYTE(entry->count);
        if (ExifEnc_GetValueSize(entry) <= 4) {
            ExifEnc_WriteValue(entry);
        } else {
            WRITE_BYTE((entry->offset & 0xff000000) >> 24);
            WRITE_BYTE((entry->offset & 0xff0000) >> 16);
            WRITE_BYTE((entry->offset & 0xff00) >> 8);
            WRITE_BYTE(entry->offset);
        }
        entry++;
    }

    if (!last) {
        WRITE_BYTE((next & 0xff000000) >> 24);
        WRITE_BYTE((next & 0xff0000) >> 16);
        WRITE_BYTE((next & 0xff00) >> 8);
        WRITE_BYTE(next);
    } else {
        WRITE_BYTE(0);
        WRITE_BYTE(0);
        WRITE_BYTE(0);
        WRITE_BYTE(0);
    }

    entry = ifd->entries;
    for (i = 0; i < ifd->count; i++) {
        size = ExifEnc_GetValueSize(entry);
        if (size > 4 && entry->tag != EXIF_TAG_MAKER_NOTE) {
            ExifEnc_WriteValue(entry);
            if (size & 1) {
                WRITE_BYTE(0);
            }
        }
        entry++;
    }
}

static BOOL ExifEnc_WriteApp1(ExifIfdSet *set, u8 noThumbnail, const u8 *thumbnail, u32 thumbnailSize) {
    ExifIfd *ifd;
    int i;
    ExifEntry *entry;
    int j;
    u32 size;
    char exif[] = "Exif";

    WRITE_BYTE(0xff);
    WRITE_BYTE(0xe1);
    // The segment's length, the Exif code and the TIFF header
    set->size = 2;
    set->size += 6;
    set->size += 8;
    for (i = 0; i < EXIF_IFD_COUNT; i++) {
        ifd = &set->ifd[i];
        if (ifd->count > 0 && ifd->entries != NULL) {
            if (!noThumbnail || i != EXIF_IFD_1ST) {
                set->size += 2;
                set->size += 4;
                set->size += ifd->count * 12;
            }
            entry = ifd->entries;
            for (j = 0; j < ifd->count; j++) {
                entry->offset = 0;
                size = ExifEnc_GetValueSize(entry);
                if (size > 4 && entry->tag != EXIF_TAG_MAKER_NOTE) {
                    entry->offset = set->size - 8;
                    if (size & 1) {
                        size++;
                    }
                    if (!noThumbnail || i != EXIF_IFD_1ST) {
                        set->size += size;
                    }
                }
                entry++;
            }
            ifd->offset = set->size - 8;
        }
    }

    if (!noThumbnail) {
        if (set->thumbnailSize & 1) {
            set->thumbnailSize++;
        }
        set->size += set->thumbnailSize;
        if (set->size >= 0x10000) {
            return FALSE;
        }
    }

    WRITE_BYTE((s16)(set->size & 0xff00) >> 8);
    WRITE_BYTE(set->size);
    for (i = 0; i < STD_GetStringLength(exif); i++) {
        WRITE_BYTE(exif[i]);
    }
    WRITE_BYTE(0);
    WRITE_BYTE(0);
    // The TIFF header: big endian, and the 0th IFD right after it
    WRITE_BYTE('M');
    WRITE_BYTE('M');
    WRITE_BYTE(0);
    WRITE_BYTE(0x2a);
    WRITE_BYTE(0);
    WRITE_BYTE(0);
    WRITE_BYTE(0);
    WRITE_BYTE(8);

    ExifEnc_WriteIfd(set, &set->ifd[EXIF_IFD_0TH], noThumbnail);
    if (set->ifd[EXIF_IFD_EXIF].count > 0) {
        ExifEnc_WriteIfd(set, &set->ifd[EXIF_IFD_EXIF], TRUE);
    }
    if (set->ifd[EXIF_IFD_MAKER_NOTE].count > 0) {
        ExifEnc_WriteMakerNoteIfd(set, &set->ifd[EXIF_IFD_MAKER_NOTE], TRUE);
    }
    if (set->ifd[EXIF_IFD_INTEROP].count > 0) {
        ExifEnc_WriteIfd(set, &set->ifd[EXIF_IFD_INTEROP], TRUE);
    }
    if (!noThumbnail) {
        ExifEnc_WriteIfd(set, &set->ifd[EXIF_IFD_1ST], TRUE);
        MI_CpuCopy(thumbnail, sExifEncWriteBuffer + sExifEncWritePos, thumbnailSize);
        sExifEncWritePos += thumbnailSize;
    }
    return TRUE;
}

static BOOL ExifEnc_BuildIfds(s16 width, s16 height, BOOL thumbnail, const u8 *thumbnailData, u32 thumbnailSize) {
    ExifIfdSet *set = &sExifEncIfdSet;
    s16 hasThumbnail;
    ExifEntry *entry;

    set->ifd[EXIF_IFD_0TH].entries = sExifEncIfd0Entries;
    set->ifd[EXIF_IFD_0TH].count = 0;
    set->ifd[EXIF_IFD_0TH].offset = 0;
    set->ifd[EXIF_IFD_EXIF].entries = sExifEncExifIfdEntries;
    set->ifd[EXIF_IFD_EXIF].count = 0;
    set->ifd[EXIF_IFD_EXIF].offset = 0;
    set->ifd[EXIF_IFD_MAKER_NOTE].entries = sExifEncMakerNoteIfdEntries;
    set->ifd[EXIF_IFD_MAKER_NOTE].count = 0;
    set->ifd[EXIF_IFD_MAKER_NOTE].offset = 0;
    set->ifd[EXIF_IFD_INTEROP].entries = sExifEncInteropIfdEntries;
    set->ifd[EXIF_IFD_INTEROP].count = 0;
    set->ifd[EXIF_IFD_INTEROP].offset = 0;
    set->ifd[EXIF_IFD_1ST].entries = sExifEncIfd1Entries;
    set->ifd[EXIF_IFD_1ST].count = 0;
    set->ifd[EXIF_IFD_1ST].offset = 0;
    hasThumbnail = (thumbnail & 1) ? TRUE : FALSE;

    set->ifd[EXIF_IFD_0TH].count = 0;
    entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
    entry->tag = EXIF_TAG_MAKE;
    entry->type = EXIF_TYPE_ASCII;
    entry->count = 9;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    STD_CopyLString(entry->asciiValue, "Nintendo", entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_0TH].count++;
    entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
    entry->tag = EXIF_TAG_MODEL;
    entry->type = EXIF_TYPE_ASCII;
    entry->count = 11;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    STD_CopyLString(entry->asciiValue, "NintendoDS", entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_0TH].count++;
    entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
    entry->tag = EXIF_TAG_X_RESOLUTION;
    entry->type = EXIF_TYPE_RATIONAL;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 72;
    entry->denominator = 1;
    entry->offset = 0;
    set->ifd[EXIF_IFD_0TH].count++;
    entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
    entry->tag = EXIF_TAG_Y_RESOLUTION;
    entry->type = EXIF_TYPE_RATIONAL;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 72;
    entry->denominator = 1;
    entry->offset = 0;
    set->ifd[EXIF_IFD_0TH].count++;
    entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
    entry->tag = EXIF_TAG_RESOLUTION_UNIT;
    entry->type = EXIF_TYPE_SHORT;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 2;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_0TH].count++;
    if (sExifEncSoftwareLength != 0) {
        entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
        entry->tag = EXIF_TAG_SOFTWARE;
        entry->type = EXIF_TYPE_ASCII;
        entry->count = sExifEncSoftwareLength;
        entry->byteValue = 0;
        entry->shortValue = 0;
        entry->longValue = 0;
        entry->numerator = 0;
        entry->denominator = 0;
        STD_CopyLString(entry->asciiValue, sExifEncSoftware, entry->count);
        entry->offset = 0;
        set->ifd[EXIF_IFD_0TH].count++;
    }
    entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
    entry->tag = EXIF_TAG_DATE_TIME;
    entry->type = EXIF_TYPE_ASCII;
    entry->count = 20;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    STD_CopyLString(entry->asciiValue, sExifEncDateTime, entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_0TH].count++;
    entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
    entry->tag = EXIF_TAG_YCBCR_POSITIONING;
    entry->type = EXIF_TYPE_SHORT;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 2;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_0TH].count++;

    set->ifd[EXIF_IFD_EXIF].count = 0;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_EXIF_VERSION;
    entry->type = EXIF_TYPE_UNDEFINED;
    entry->count = 4;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    MI_CpuCopy("0220", entry->undefinedValue, entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_DATE_TIME_ORIGINAL;
    entry->type = EXIF_TYPE_ASCII;
    entry->count = 20;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    STD_CopyLString(entry->asciiValue, sExifEncDateTime, entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_DATE_TIME_DIGITIZED;
    entry->type = EXIF_TYPE_ASCII;
    entry->count = 20;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    STD_CopyLString(entry->asciiValue, sExifEncDateTime, entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_COMPONENTS_CONFIGURATION;
    entry->type = EXIF_TYPE_UNDEFINED;
    entry->count = 4;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->undefinedValue[0] = 1;
    entry->undefinedValue[1] = 2;
    entry->undefinedValue[2] = 3;
    entry->undefinedValue[3] = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_MAKER_NOTE;
    entry->type = EXIF_TYPE_UNDEFINED;
    entry->count = 0;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->undefinedValue[0] = 1;
    entry->undefinedValue[1] = 2;
    entry->undefinedValue[2] = 3;
    entry->undefinedValue[3] = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_FLASHPIX_VERSION;
    entry->type = EXIF_TYPE_UNDEFINED;
    entry->count = 4;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    MI_CpuCopy("0100", entry->undefinedValue, entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_COLOR_SPACE;
    entry->type = EXIF_TYPE_SHORT;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 1;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_PIXEL_X_DIMENSION;
    entry->type = EXIF_TYPE_LONG;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = width;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;
    entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
    entry->tag = EXIF_TAG_PIXEL_Y_DIMENSION;
    entry->type = EXIF_TYPE_LONG;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = height;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_EXIF].count++;

    set->ifd[EXIF_IFD_MAKER_NOTE].count = 0;
    entry = &set->ifd[EXIF_IFD_MAKER_NOTE].entries[set->ifd[EXIF_IFD_MAKER_NOTE].count];
    entry->tag = sExifEncSignatureEntry.tag;
    entry->type = sExifEncSignatureEntry.type;
    entry->count = sExifEncSignatureEntry.count;
    entry->byteValue = sExifEncSignatureEntry.byteValue;
    entry->shortValue = sExifEncSignatureEntry.shortValue;
    entry->longValue = sExifEncSignatureEntry.longValue;
    entry->numerator = sExifEncSignatureEntry.numerator;
    entry->denominator = sExifEncSignatureEntry.denominator;
    STD_CopyLString(entry->asciiValue, sExifEncSignatureEntry.asciiValue, entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_MAKER_NOTE].count++;
    if (sExifEncMakerNoteData1Size != 0) {
        entry = &set->ifd[EXIF_IFD_MAKER_NOTE].entries[set->ifd[EXIF_IFD_MAKER_NOTE].count];
        entry->tag = MAKER_NOTE_TAG_DATA_1;
        entry->type = EXIF_TYPE_UNDEFINED;
        entry->count = sExifEncMakerNoteData1Size;
        entry->byteValue = 0;
        entry->shortValue = 0;
        entry->longValue = 0;
        entry->numerator = 0;
        entry->denominator = 0;
        entry->offset = 0;
        set->ifd[EXIF_IFD_MAKER_NOTE].count++;
    }
    if (sExifEncMakerNoteData2Size != 0) {
        entry = &set->ifd[EXIF_IFD_MAKER_NOTE].entries[set->ifd[EXIF_IFD_MAKER_NOTE].count];
        entry->tag = MAKER_NOTE_TAG_DATA_2;
        entry->type = EXIF_TYPE_UNDEFINED;
        entry->count = sExifEncMakerNoteData2Size;
        entry->byteValue = 0;
        entry->shortValue = 0;
        entry->longValue = 0;
        entry->numerator = 0;
        entry->denominator = 0;
        entry->offset = 0;
        set->ifd[EXIF_IFD_MAKER_NOTE].count++;
    }

    set->ifd[EXIF_IFD_INTEROP].count = 0;
    entry = &set->ifd[EXIF_IFD_INTEROP].entries[set->ifd[EXIF_IFD_INTEROP].count];
    entry->tag = EXIF_TAG_INTEROP_INDEX;
    entry->type = EXIF_TYPE_ASCII;
    entry->count = 4;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    STD_CopyLString(entry->asciiValue, "R98", entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_INTEROP].count++;
    entry = &set->ifd[EXIF_IFD_INTEROP].entries[set->ifd[EXIF_IFD_INTEROP].count];
    entry->tag = EXIF_TAG_INTEROP_VERSION;
    entry->type = EXIF_TYPE_UNDEFINED;
    entry->count = 4;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    MI_CpuCopy("0100", entry->undefinedValue, entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_INTEROP].count++;
    entry = &set->ifd[EXIF_IFD_INTEROP].entries[set->ifd[EXIF_IFD_INTEROP].count];
    entry->tag = EXIF_TAG_RELATED_IMAGE_FILE_FORMAT;
    entry->type = EXIF_TYPE_ASCII;
    entry->count = 18;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    STD_CopyLString(entry->asciiValue, "JPEG Exif Ver 2.2", entry->count);
    entry->offset = 0;
    set->ifd[EXIF_IFD_INTEROP].count++;

    set->ifd[EXIF_IFD_1ST].count = 0;
    entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
    entry->tag = EXIF_TAG_COMPRESSION;
    entry->type = EXIF_TYPE_SHORT;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 6;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_1ST].count++;
    entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
    entry->tag = EXIF_TAG_X_RESOLUTION;
    entry->type = EXIF_TYPE_RATIONAL;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 72;
    entry->denominator = 1;
    entry->offset = 0;
    set->ifd[EXIF_IFD_1ST].count++;
    entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
    entry->tag = EXIF_TAG_Y_RESOLUTION;
    entry->type = EXIF_TYPE_RATIONAL;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 0;
    entry->longValue = 0;
    entry->numerator = 72;
    entry->denominator = 1;
    entry->offset = 0;
    set->ifd[EXIF_IFD_1ST].count++;
    entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
    entry->tag = EXIF_TAG_RESOLUTION_UNIT;
    entry->type = EXIF_TYPE_SHORT;
    entry->count = 1;
    entry->byteValue = 0;
    entry->shortValue = 2;
    entry->longValue = 0;
    entry->numerator = 0;
    entry->denominator = 0;
    entry->offset = 0;
    set->ifd[EXIF_IFD_1ST].count++;

    // The entries that point to other IFDs and to the thumbnail, whose values are only known once the IFDs are laid
    // out. Both branches add the same entries
    if (hasThumbnail) {
        if (set->ifd[EXIF_IFD_0TH].count == 0) {
            return FALSE;
        }
        entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
        entry->tag = EXIF_TAG_EXIF_IFD;
        entry->type = EXIF_TYPE_LONG;
        entry->count = 1;
        set->ifd[EXIF_IFD_0TH].count++;
        if (set->ifd[EXIF_IFD_MAKER_NOTE].count != 0 &&
            sExifEncSignatureEntry.count + sExifEncMakerNoteData1Size + sExifEncMakerNoteData2Size != 0) {
            entry = &set->ifd[EXIF_IFD_EXIF].entries[EXIF_MAKER_NOTE_INDEX];
            entry->tag = EXIF_TAG_MAKER_NOTE;
            entry->type = EXIF_TYPE_UNDEFINED;
            entry->count = 2 + 4;
            if (sExifEncSignatureEntry.count != 0) {
                entry->count += 12;
            }
            if (sExifEncMakerNoteData1Size != 0) {
                entry->count += 12;
            }
            if (sExifEncMakerNoteData2Size != 0) {
                entry->count += 12;
            }
            entry->count += sExifEncSignatureEntry.count + sExifEncMakerNoteData1Size + sExifEncMakerNoteData2Size;
        }
        if (set->ifd[EXIF_IFD_INTEROP].count != 0) {
            entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
            entry->tag = EXIF_TAG_INTEROP_IFD;
            entry->type = EXIF_TYPE_LONG;
            entry->count = 1;
            set->ifd[EXIF_IFD_EXIF].count++;
        }
        entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
        entry->tag = EXIF_TAG_JPEG_INTERCHANGE_FORMAT;
        entry->type = EXIF_TYPE_LONG;
        entry->count = 1;
        set->ifd[EXIF_IFD_1ST].count++;
        entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
        entry->tag = EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH;
        entry->type = EXIF_TYPE_LONG;
        entry->count = 1;
        set->ifd[EXIF_IFD_1ST].count++;
        set->thumbnailSize = thumbnailSize;
    } else {
        if (set->ifd[EXIF_IFD_0TH].count == 0) {
            return FALSE;
        }
        entry = &set->ifd[EXIF_IFD_0TH].entries[set->ifd[EXIF_IFD_0TH].count];
        entry->tag = EXIF_TAG_EXIF_IFD;
        entry->type = EXIF_TYPE_LONG;
        entry->count = 1;
        set->ifd[EXIF_IFD_0TH].count++;
        if (set->ifd[EXIF_IFD_MAKER_NOTE].count != 0 &&
            sExifEncSignatureEntry.count + sExifEncMakerNoteData1Size + sExifEncMakerNoteData2Size != 0) {
            entry = &set->ifd[EXIF_IFD_EXIF].entries[EXIF_MAKER_NOTE_INDEX];
            entry->tag = EXIF_TAG_MAKER_NOTE;
            entry->type = EXIF_TYPE_UNDEFINED;
            entry->count = 2 + 4;
            if (sExifEncSignatureEntry.count != 0) {
                entry->count += 12;
            }
            if (sExifEncMakerNoteData1Size != 0) {
                entry->count += 12;
            }
            if (sExifEncMakerNoteData2Size != 0) {
                entry->count += 12;
            }
            entry->count += sExifEncSignatureEntry.count + sExifEncMakerNoteData1Size + sExifEncMakerNoteData2Size;
        }
        if (set->ifd[EXIF_IFD_INTEROP].count != 0) {
            entry = &set->ifd[EXIF_IFD_EXIF].entries[set->ifd[EXIF_IFD_EXIF].count];
            entry->tag = EXIF_TAG_INTEROP_IFD;
            entry->type = EXIF_TYPE_LONG;
            entry->count = 1;
            set->ifd[EXIF_IFD_EXIF].count++;
        }
        entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
        entry->tag = EXIF_TAG_JPEG_INTERCHANGE_FORMAT;
        entry->type = EXIF_TYPE_LONG;
        entry->count = 1;
        set->ifd[EXIF_IFD_1ST].count++;
        entry = &set->ifd[EXIF_IFD_1ST].entries[set->ifd[EXIF_IFD_1ST].count];
        entry->tag = EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH;
        entry->type = EXIF_TYPE_LONG;
        entry->count = 1;
        set->ifd[EXIF_IFD_1ST].count++;
        set->thumbnailSize = 0;
    }

    // SOI
    WRITE_BYTE(0xff);
    WRITE_BYTE(0xd8);
    if (hasThumbnail) {
        if (!ExifEnc_WriteApp1(set, FALSE, thumbnailData, thumbnailSize)) {
            return FALSE;
        }
    } else {
        if (!ExifEnc_WriteApp1(set, TRUE, thumbnailData, thumbnailSize)) {
            return FALSE;
        }
    }
    if ((sExifEncWritePos & 1) == 1) {
        WRITE_BYTE(0);
    }
    return TRUE;
}

u32 ExifEnc_WriteHeader(u8 *dst, u32 width, u32 height, const u8 *thumbnail, u32 thumbnailSize) {
    u32 titleId;
    char software[32];

    sExifEncWriteBuffer = dst;
    sExifEncSignatureEntry.tag = MAKER_NOTE_TAG_SIGNATURE;
    sExifEncSignatureEntry.type = EXIF_TYPE_UNDEFINED;
    sExifEncSignatureEntry.count = SIGNATURE_SIZE;
    if (sExifEncSignatureEntry.count > EXIF_VALUE_MAX) {
        sExifEncSignatureEntry.count = EXIF_VALUE_MAX;
    }
    sExifEncSignatureEntry.byteValue = 0;
    sExifEncSignatureEntry.shortValue = 0;
    sExifEncSignatureEntry.longValue = 0;
    sExifEncSignatureEntry.numerator = 0;
    sExifEncSignatureEntry.denominator = 0;
    MI_CpuFill8(sExifEncSignatureEntry.asciiValue, 0, sExifEncSignatureEntry.count);

    // The software is the game code, the low word of the DSi's title ID
    titleId = 0;
    if (OS_IsRunOnTwl() == TRUE) {
        titleId = *(u32 *)HW_TWL_TITLE_ID;
    }
    MI_CpuCopy8(&titleId, software, 4);
    software[4] = '\0';
    ExifEnc_SetSoftware(software);

    if (!sExifEncDateTimeSet) {
        while (!ExifEnc_SetCurrentDateTime()) {
            OS_SpinWaitSysCycles(5);
        }
    }

    sExifEncWritePos = 0;
    if (thumbnailSize == 0) {
        if (ExifEnc_BuildIfds(width, height, FALSE, thumbnail, thumbnailSize) != TRUE) {
            OS_Terminate();
        }
    } else {
        if (ExifEnc_BuildIfds(width, height, TRUE, thumbnail, thumbnailSize) != TRUE) {
            OS_Terminate();
        }
    }

    sExifEncMakerNoteData1 = NULL;
    sExifEncMakerNoteData1Size = 0;
    sExifEncMakerNoteData2 = NULL;
    sExifEncMakerNoteData2Size = 0;
    sExifEncDateTimeSet = FALSE;
    return sExifEncWritePos;
}
