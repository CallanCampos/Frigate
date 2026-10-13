#include "types.h"
#include "gfl/std.h"
#include "ssp_private.h"

// The EXIF reader of TwlSDK's JPEG decoder (ssp_jpegdec.c): it walks the TIFF structure of a JPEG's APP1 segment for
// its date and software, the DSi's maker note, which holds the signature and two blocks of the application's, and the
// thumbnail. The game links only what the decoder calls; the getters were dead-stripped. The file's name is a guess
// after TwlSDK's ssp/exifdec.h, and so are the names of its functions and data.

// The tags of the TIFF structure that the reader looks for
#define EXIF_TAG_SOFTWARE 0x131
#define EXIF_TAG_DATE_TIME 0x132
#define EXIF_TAG_JPEG_INTERCHANGE_FORMAT 0x201
#define EXIF_TAG_EXIF_IFD 0x8769
#define EXIF_TAG_MAKER_NOTE 0x927c
#define EXIF_TAG_INTEROPERABILITY_IFD 0xa005

// The tags of the maker note: its first entry, which holds the signature, then the application's two blocks
#define MAKER_NOTE_TAG_SIGNATURE 0x1000
#define MAKER_NOTE_TAG_DATA_1 0x1001
#define MAKER_NOTE_TAG_DATA_2 0x1002

// The offsets in the TIFF structure are from its header, 12 bytes into the APP1 segment
#define TIFF_HEADER_OFFSET 0xc

// An IFD that is not there, or has been read
#define IFD_NONE 0xffff

#define EXIF_SOFTWARE_LENGTH_MAX 30

enum {
    EXIF_RESULT_THUMBNAIL,
    EXIF_RESULT_NEXT_SEGMENT,
    EXIF_RESULT_ERROR,
};

static u16 sSoftwareLength;
static u16 sMakerNoteData1Size;
static u16 sMakerNoteData2Size;
static u32 sMakerNoteData1;
static u32 sMakerNoteData2;
BOOL data_ov257_021b6240;
static char sDateTime[20];
static char sSoftware[32];

// Reads a 16-bit value of the TIFF structure, in its byte order
static u16 func_ov257_021ac664(u8 *data, u32 pos, BOOL bigEndian) {
    u16 value;
    if (bigEndian == FALSE) {
        value = *(data + pos);
        value |= (*(data + pos + 1) << 8) & 0xff00;
        return value;
    }
    value = (*(data + pos) << 8) & 0xff00;
    value |= *(data + pos + 1);
    return value;
}

// Reads a 32-bit value of the TIFF structure, in its byte order
static u32 func_ov257_021ac694(u8 *data, u32 pos, BOOL bigEndian) {
    u32 value;
    if (bigEndian == FALSE) {
        value = *(data + pos);
        value |= (*(data + pos + 1) << 8) & 0xff00;
        value |= (*(data + pos + 2) << 16) & 0xff0000;
        value |= (*(data + pos + 3) << 24) & 0xff000000;
        return value;
    }
    value = (*(data + pos) << 24) & 0xff000000;
    value |= (*(data + pos + 1) << 16) & 0xff0000;
    value |= (*(data + pos + 2) << 8) & 0xff00;
    value |= *(data + pos + 3);
    return value;
}

// Reads the APP1 segment at pos: the 0th IFD, the Exif IFD, then the maker note, the Interoperability IFD and the 1st
// IFD in the order they are stored
static int func_ov257_021ac6f0(u8 *data, u32 size, u32 pos, s32 *posOut, u32 option) {
    u16 len;
    u16 tag;
    u16 count;
    u32 valueCount;
    u32 value;
    BOOL bigEndian;
    u32 exifIfd = IFD_NONE;
    u32 makerNote = IFD_NONE;
    u32 interopIfd = IFD_NONE;
    u32 ifd1;
    s32 thumbnail;

    if (pos + 0x12 >= size) {
        return EXIF_RESULT_ERROR;
    }
    bigEndian = TRUE;
    len = func_ov257_021ac664(data, pos, TRUE);
    if (*(data + pos + 2) != 'E') {
        return EXIF_RESULT_ERROR;
    }
    if (*(data + pos + 3) != 'x') {
        return EXIF_RESULT_ERROR;
    }
    if (*(data + pos + 4) != 'i') {
        return EXIF_RESULT_ERROR;
    }
    if (*(data + pos + 5) != 'f') {
        return EXIF_RESULT_ERROR;
    }
    if (*(data + pos + 6) != 0) {
        return EXIF_RESULT_ERROR;
    }
    if (*(data + pos + 7) != 0) {
        return EXIF_RESULT_ERROR;
    }

    // The byte order is read into the variable the IFD loops read the tags into
    tag = func_ov257_021ac664(data, pos + 8, bigEndian);
    if (tag == 0x4949) {
        bigEndian = FALSE;
    } else if (tag != 0x4d4d) {
        return EXIF_RESULT_ERROR;
    }

    // The 0th IFD
    count = func_ov257_021ac664(data, pos + 0x10, bigEndian);
    pos += 0x12;
    if (pos + count * 12 + 4 >= size) {
        return EXIF_RESULT_ERROR;
    }
    while (count != 0) {
        tag = func_ov257_021ac664(data, pos, bigEndian);
        func_ov257_021ac664(data, pos + 2, bigEndian);
        valueCount = func_ov257_021ac694(data, pos + 4, bigEndian);
        value = func_ov257_021ac694(data, pos + 8, bigEndian);
        pos += 12;
        if (tag == EXIF_TAG_EXIF_IFD) {
            exifIfd = value;
        }
        if (tag == EXIF_TAG_DATE_TIME) {
            if (value + 0x20 >= size) {
                return EXIF_RESULT_ERROR;
            }
            sys_memcpy_fast(&data[value + TIFF_HEADER_OFFSET], sDateTime, sizeof(sDateTime));
        }
        if (tag == EXIF_TAG_SOFTWARE) {
            if (valueCount > EXIF_SOFTWARE_LENGTH_MAX) {
                valueCount = EXIF_SOFTWARE_LENGTH_MAX;
            }
            value += TIFF_HEADER_OFFSET;
            if (value + valueCount >= size) {
                return EXIF_RESULT_ERROR;
            }
            sys_memcpy_fast(data + value, sSoftware, valueCount);
            sSoftwareLength = valueCount;
        }
        count--;
    }
    ifd1 = func_ov257_021ac694(data, pos, bigEndian);
    if (ifd1 == 0) {
        ifd1 = IFD_NONE;
    }

    // The Exif IFD
    thumbnail = -1;
    pos = exifIfd + TIFF_HEADER_OFFSET;
    if (pos + 2 >= size) {
        return EXIF_RESULT_ERROR;
    }
    count = func_ov257_021ac664(data, pos, bigEndian);
    pos += 2;
    if (pos + count * 12 >= size) {
        return EXIF_RESULT_ERROR;
    }
    while (count != 0) {
        tag = func_ov257_021ac664(data, pos, bigEndian);
        func_ov257_021ac664(data, pos + 2, bigEndian);
        valueCount = func_ov257_021ac694(data, pos + 4, bigEndian);
        value = func_ov257_021ac694(data, pos + 8, bigEndian);
        pos += 12;
        if (tag == EXIF_TAG_MAKER_NOTE && valueCount > 4) {
            makerNote = value;
        }
        if (tag == EXIF_TAG_INTEROPERABILITY_IFD) {
            interopIfd = value;
        }
        count--;
    }

    while (makerNote != IFD_NONE || interopIfd != IFD_NONE || ifd1 != IFD_NONE) {
        if (makerNote < interopIfd && makerNote < ifd1) {
            u16 entries;
            u16 firstTag;
            u16 firstType;
            u32 firstCount;

            pos = makerNote + TIFF_HEADER_OFFSET;
            if (pos + 0xe >= size) {
                return EXIF_RESULT_ERROR;
            }
            entries = func_ov257_021ac664(data, pos, bigEndian);
            firstTag = func_ov257_021ac664(data, pos + 2, bigEndian);
            firstType = func_ov257_021ac664(data, pos + 4, bigEndian);
            firstCount = func_ov257_021ac694(data, pos + 6, bigEndian);
            func_ov257_021ac694(data, pos + 10, bigEndian);
            pos += 0xe;
            if (firstTag == MAKER_NOTE_TAG_SIGNATURE && firstType == 7 && firstCount == SSP_SIGNATURE_SIZE) {
                count = entries - 1;
                if (pos + count * 12 >= size) {
                    return EXIF_RESULT_ERROR;
                }
                while (count != 0) {
                    tag = func_ov257_021ac664(data, pos, bigEndian);
                    func_ov257_021ac664(data, pos + 2, bigEndian);
                    valueCount = func_ov257_021ac694(data, pos + 4, bigEndian);
                    value = func_ov257_021ac694(data, pos + 8, bigEndian);
                    pos += 12;
                    if (tag == MAKER_NOTE_TAG_DATA_1) {
                        // Up to 4 bytes are stored in the entry itself. The offsets are kept to 16 bits, the size of
                        // the APP1 segment
                        if (valueCount > 4) {
                            sMakerNoteData1 = (u16)value;
                        } else {
                            sMakerNoteData1 = (u16)(pos - valueCount);
                        }
                        sMakerNoteData1Size = valueCount;
                        if (sMakerNoteData1 + sMakerNoteData1Size >= size) {
                            sMakerNoteData1 = 0;
                            sMakerNoteData1Size = 0;
                            return EXIF_RESULT_ERROR;
                        }
                    } else if (tag == MAKER_NOTE_TAG_DATA_2) {
                        if (valueCount > 4) {
                            sMakerNoteData2 = (u16)value;
                        } else {
                            sMakerNoteData2 = (u16)(pos - valueCount);
                        }
                        sMakerNoteData2Size = valueCount;
                        if (sMakerNoteData2 + sMakerNoteData2Size >= size) {
                            sMakerNoteData2 = 0;
                            sMakerNoteData2Size = 0;
                            return EXIF_RESULT_ERROR;
                        }
                    }
                    count--;
                }
            }
            makerNote = IFD_NONE;
        } else if (interopIfd < makerNote && interopIfd < ifd1) {
            pos = interopIfd + TIFF_HEADER_OFFSET;
            if (pos + 2 >= size) {
                return EXIF_RESULT_ERROR;
            }
            count = func_ov257_021ac664(data, pos, bigEndian);
            pos += 2;
            if (pos + count * 12 >= size) {
                return EXIF_RESULT_ERROR;
            }
            while (count != 0) {
                func_ov257_021ac664(data, pos, bigEndian);
                func_ov257_021ac664(data, pos + 2, bigEndian);
                func_ov257_021ac694(data, pos + 4, bigEndian);
                func_ov257_021ac694(data, pos + 8, bigEndian);
                pos += 12;
                count--;
            }
            interopIfd = IFD_NONE;
        } else if (ifd1 < makerNote && ifd1 < interopIfd) {
            pos = ifd1 + TIFF_HEADER_OFFSET;
            if (pos + 2 >= size) {
                return EXIF_RESULT_ERROR;
            }
            count = func_ov257_021ac664(data, pos, bigEndian);
            pos += 2;
            if (pos + count * 12 >= size) {
                return EXIF_RESULT_ERROR;
            }
            while (count != 0) {
                tag = func_ov257_021ac664(data, pos, bigEndian);
                func_ov257_021ac664(data, pos + 2, bigEndian);
                func_ov257_021ac694(data, pos + 4, bigEndian);
                value = func_ov257_021ac694(data, pos + 8, bigEndian);
                pos += 12;
                if (tag == EXIF_TAG_JPEG_INTERCHANGE_FORMAT) {
                    thumbnail = value;
                    break;
                }
                count--;
            }
            ifd1 = IFD_NONE;
        } else {
            return EXIF_RESULT_ERROR;
        }
    }

    if (option & 1) {
        if (thumbnail == -1) {
            return EXIF_RESULT_ERROR;
        }
        thumbnail += TIFF_HEADER_OFFSET;
        *posOut = thumbnail;
        return EXIF_RESULT_THUMBNAIL;
    }
    // The APP1 segment follows the SOI
    *posOut = len + 4;
    return EXIF_RESULT_NEXT_SEGMENT;
}

int func_ov257_021acc20(u8 *data, u32 size, s32 *pos, u32 option) {
    int result;
    u32 start = *pos;

    sMakerNoteData1 = 0;
    sMakerNoteData1Size = 0;
    sMakerNoteData2 = 0;
    sMakerNoteData2Size = 0;
    sSoftwareLength = 0;
    result = func_ov257_021ac6f0(data, size, start, pos, option);
    if (sMakerNoteData1 != 0) {
        sMakerNoteData1 += (u32)data + TIFF_HEADER_OFFSET;
    }
    if (sMakerNoteData2 != 0) {
        sMakerNoteData2 += (u32)data + TIFF_HEADER_OFFSET;
    }
    return result;
}
