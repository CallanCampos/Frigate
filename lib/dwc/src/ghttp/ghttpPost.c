#include "ghttp_internal.h"

// ghttpPost: the posts of a request (the form or the XML message that it sends), made of items, and how they are sent.
// The posts are urlencoded if they hold strings alone, multipart (form-data) if they hold files, and DIME messages if
// they hold files and an XML message

// What an item holds
typedef enum GHIPostDataType { GHIString, GHIFileDisk, GHIFileMemory, GHIXmlData } GHIPostDataType;

typedef struct GHIPostData {
    GHIPostDataType type;
    char *name;
    union {
        struct {
            char *value;
            int length;
            GHTTPBool unk_10;
            int escapeCount;
        } string;
        struct {
            char *path;
            char *fileName;
            char *contentType;
        } disk;
        struct {
            const char *data;
            int length;
            char *fileName;
            char *contentType;
        } memory;
        struct {
            GSXmlStreamWriter message;
        } xml;
    } data;
} GHIPostData;

// How far an item of the post of a connection has got
typedef struct GHIPostState {
    GHIPostData *data;
    int pos;
    void *file;
    long fileSize;
} GHIPostState;

// The header of a DIME record, with its numbers in network order
typedef struct GSIDimeHeader {
    gsi_u8 mVersionAndFlags;
    gsi_u8 mTypeT;
    gsi_u16 mOptionsLength;
    gsi_u16 mIdLength;
    gsi_u16 mTypeLength;
    gsi_u32 mDataLength;
} GHIDimeHeader;

#define GHI_DIME_VERSION 0x08
#define GHI_DIME_FIRST 0x04
#define GHI_DIME_LAST 0x02
#define GHI_DIME_TYPE_MEDIA 0x10
#define GHI_DIME_TYPE_URI 0x20

// The length of a text as the 16 bits of a DIME header, in network order
#define GHI_DIME_LENGTH16(x) GHI_SWAP16((gsi_u16)(gsi_i16)(x))
#define GHI_SWAP32(x)                                                                                                  \
    (((((gsi_u32)(x)) << 24) & 0xff000000) |                                                                           \
     (((((gsi_u32)(x)) << 8) & 0x00ff0000) |                                                                           \
      (((((gsi_u32)(x)) >> 24) & 0xff) | ((((gsi_u32)(x)) >> 8) & 0x0000ff00))))

// How many bytes a text of this length is padded with to the next multiple of 4 (4 for none)
#define GHI_DIME_PAD(length) (4 - ((length) % 4))

#define GHI_BOUNDARY "Qr4G823s23d---<<><><<<>--7d118e0536"
#define GHI_BOUNDARY_LINE "--" GHI_BOUNDARY
#define GHI_MESSAGE_ID "cid:id0"
#define GHI_MESSAGE_TYPE "http://schemas.xmlsoap.org/soap/envelope/"

// The characters that urlencoding leaves as they are
static char sUnreservedChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_@-.*";

// Frees what an item holds
static void ghiPostDataFree(void *element) {
    GHIPostData *item = element;

    if (item->type != GHIXmlData) {
        gsifree(item->name);
    }
    if (item->type == GHIString) {
        gsifree(item->data.string.value);
    } else if (item->type == GHIFileDisk) {
        gsifree(item->data.disk.path);
        gsifree(item->data.disk.fileName);
        gsifree(item->data.disk.contentType);
    } else if (item->type == GHIFileMemory) {
        gsifree(item->data.memory.fileName);
        gsifree(item->data.memory.contentType);
    } else if (item->type == GHIXmlData) {
        gsXmlFreeWriter(item->data.xml.message);
    }
}

GHTTPPost ghiNewPost(void) {
    GHTTPPost post = gsimalloc(sizeof(GHIPost));
    char *p;
    gsi_u32 size;

    if (post == NULL) {
        return NULL;
    }
    p = (char *)post;
    size = sizeof(GHIPost);
    while (size--) {
        *p++ = 0;
    }
    post->autoFree = gsi_true;
    post->data = ArrayNew(sizeof(GHIPostData), 0, ghiPostDataFree);
    if (post->data == NULL) {
        gsifree(post);
        return NULL;
    }
    return post;
}

void ghiPostSetAutoFree(GHTTPPost post, GHTTPBool autoFree) {
    post->autoFree = autoFree;
}

void ghiFreePost(GHTTPPost post) {
    if (post->data != NULL) {
        ArrayFree(post->data);
    }
    post->data = NULL;
    gsifree(post);
}

// Frees a post of a request, and what the request keeps of it
void ghiFreePostAndUpdateConnection(GHTTPRequest request, GHTTPPost post) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        ghiPostCleanupState(connection);
    }
    ghiFreePost(post);
}

// Adds a file that is in memory: the post copies the texts but not the bytes. Returns gsi_false if it can't
GHTTPBool ghiPostAddFileFromMemory(GHTTPPost post, const gsi_char *field, const char *bytes, int size,
                                   const gsi_char *fileName, const gsi_char *contentType) {
    char *fieldCopy = goastrdup(field);
    char *fileNameCopy = goastrdup(fileName);
    char *contentTypeCopy = goastrdup(contentType);

    if (fieldCopy == NULL || fileNameCopy == NULL || contentTypeCopy == NULL) {
        gsifree(fieldCopy);
        gsifree(fileNameCopy);
        gsifree(contentTypeCopy);
        return gsi_false;
    } else {
        GHIPostData item = { 0 };

        item.type = GHIFileMemory;
        item.name = fieldCopy;
        item.data.memory.data = bytes;
        item.data.memory.length = size;
        item.data.memory.fileName = fileNameCopy;
        item.data.memory.contentType = contentTypeCopy;
        ArrayAppend(post->data, &item);
        post->hasFiles = gsi_true;
        if (post->hasSoap == gsi_true) {
            post->useDIME = gsi_true;
        }
        return gsi_true;
    }
}

// Adds an XML message, which the post takes over
GHTTPBool ghiPostAddXml(GHTTPPost post, GSXmlStreamWriter message) {
    GHIPostData item;

    item.type = GHIXmlData;
    item.data.xml.message = message;
    ArrayAppend(post->data, &item);
    post->hasSoap = gsi_true;
    if (post->hasFiles == gsi_true) {
        post->useDIME = gsi_true;
    }
    return gsi_true;
}

// The content type that the request says its post is
const char *ghiPostGetContentType(GHIConnection *connection) {
    GHTTPPost post = connection->post;

    if (post == NULL) {
        return "";
    }
    if (post->useDIME) {
        return "application/dime";
    }
    if (post->hasFiles) {
        return "multipart/form-data; boundary=" GHI_BOUNDARY;
    }
    if (post->hasSoap) {
        return "text/xml";
    }
    return "application/x-www-form-urlencoded";
}

// The length of a post without files: the names, the values (as they will be urlencoded) and the = and & between them
static int ghiPostGetNoFilesContentLength(GHIConnection *connection) {
    GHTTPPost post = connection->post;
    int length = 0;
    int count = ArrayLength(post->data);
    int i;

    if (count == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        GHIPostData *item = ArrayNth(post->data, i);

        if (item->type == GHIString) {
            length += strlen(item->name);
            length += item->data.string.length;
            length += item->data.string.escapeCount * 2;
            length += 1;
        } else if (item->type == GHIXmlData) {
            length += gsXmlWriterGetDataLength(item->data.xml.message);
        }
    }
    return length + (count - 1);
}

// The length of a post with files: what the multipart format or the DIME one add to each item, and the item
static int ghiPostGetHasFilesContentLength(GHIConnection *connection) {
    GHTTPPost post = connection->post;
    GHIPostData *item;
    int length = 0;
    int count;
    int stringExtra;
    int fileExtra;
    int lastExtra;
    int xmlExtra;
    int i;
    int padding;

    if (post->useDIME) {
        stringExtra = 12;
        fileExtra = 12;
        xmlExtra = 12;
        lastExtra = 0;
    } else {
        int boundaryLength = strlen(GHI_BOUNDARY_LINE);

        stringExtra = boundaryLength + 0x2f;
        fileExtra = boundaryLength + 0x4c;
        xmlExtra = 0;
        lastExtra = boundaryLength + 4;
    }

    count = ArrayLength(post->data);
    for (i = 0; i < count; i++) {
        item = ArrayNth(post->data, i);

        if (item->type == GHIString) {
            length += stringExtra;
            length += strlen(item->name);
            length += item->data.string.length;
        } else if (item->type == GHIFileDisk) {
            GHIPostState *state;
            gsi_bool mixed;
            int fileSize;

            length += fileExtra;
            length += strlen(item->name);
            length += strlen(item->data.disk.contentType);
            state = ArrayNth(connection->postingState.states, i);
            fileSize = state->fileSize;
            length += fileSize;
            mixed = post->useDIME;
            if (!mixed) {
                length += strlen(item->data.disk.fileName);
            }
            if (mixed) {
                padding = GHI_DIME_PAD((int)strlen(item->name));
                if (padding != 4) {
                    length += padding;
                }
                padding = GHI_DIME_PAD((int)strlen(item->data.disk.contentType));
                if (padding != 4) {
                    length += padding;
                }
                padding = GHI_DIME_PAD(fileSize);
                if (padding != 4) {
                    length += padding;
                }
            }
        } else if (item->type == GHIFileMemory) {
            gsi_bool mixed;
            char *name;
            char *contentType;
            int dataLength;

            length += fileExtra;
            name = item->name;
            length += strlen(name);
            contentType = item->data.memory.contentType;
            length += strlen(contentType);
            dataLength = item->data.memory.length;
            length += dataLength;
            mixed = post->useDIME;
            if (!mixed) {
                length += strlen(item->data.memory.fileName);
            }
            if (mixed) {
                padding = GHI_DIME_PAD((int)strlen(name));
                if (padding != 4) {
                    length += padding;
                }
                padding = GHI_DIME_PAD((int)strlen(contentType));
                if (padding != 4) {
                    length += padding;
                }
                padding = GHI_DIME_PAD(dataLength);
                if (padding != 4) {
                    length += padding;
                }
            }
        } else if (item->type == GHIXmlData) {
            length += xmlExtra;
            length += gsXmlWriterGetDataLength(item->data.xml.message);
            padding = GHI_DIME_PAD(gsXmlWriterGetDataLength(item->data.xml.message));
            if (padding != 4) {
                length += padding;
            }
            length += strlen(GHI_MESSAGE_ID);
            padding = GHI_DIME_PAD((int)strlen(GHI_MESSAGE_ID));
            if (padding != 4) {
                length += padding;
            }
            length += strlen(GHI_MESSAGE_TYPE);
            padding = GHI_DIME_PAD((int)strlen(GHI_MESSAGE_TYPE));
            if (padding != 4) {
                length += padding;
            }
        } else {
            return 0;
        }
    }
    return length + lastExtra;
}

int ghiPostGetContentLength(GHIConnection *connection) {
    if (connection->post == NULL) {
        return 0;
    }
    if (connection->post->hasFiles) {
        return ghiPostGetHasFilesContentLength(connection);
    }
    return ghiPostGetNoFilesContentLength(connection);
}

// Starts an item's state: a file on disk is measured
static gsi_bool ghiPostStateInit(GHIPostState *state) {
    int type = state->data->type;

    state->pos = -1;
    if (type == GHIString) {
    } else if (type == GHIFileDisk) {
        if (state->file == NULL) {
            return gsi_false;
        }
        if (fseek(state->file, 0, 2) != 0) {
            return gsi_false;
        }
        state->fileSize = ftell(state->file);
        if (state->fileSize == -1) {
            return gsi_false;
        }
        rewind(state->file);
    } else if (type == GHIFileMemory || type == GHIXmlData) {
    } else {
        return gsi_false;
    }
    return gsi_true;
}

static void ghiPostStateCleanup(GHIPostState *state) {
    int type = state->data->type;

    if (type == GHIString) {
    } else if (type == GHIFileDisk) {
        if (state->file != NULL) {
            fclose(state->file);
        }
        state->file = NULL;
    }
}

// Makes the states of the items of the post of a connection and works out how long the post is
GHTTPBool ghiPostInitState(GHIConnection *connection) {
    GHTTPPost post = connection->post;
    int count;
    int i;

    if (post == NULL) {
        return gsi_false;
    }
    connection->postingState.index = 0;
    connection->postingState.bytesPosted = 0;
    connection->postingState.totalBytes = 0;
    connection->postingState.completed = 0;
    connection->postingState.callback = post->callback;
    connection->postingState.param = post->param;
    count = ArrayLength(post->data);
    connection->postingState.states = ArrayNew(sizeof(GHIPostState), count, NULL);
    if (connection->postingState.states == NULL) {
        return gsi_false;
    }
    for (i = 0; i < count; i++) {
        GHIPostData *item = ArrayNth(connection->post->data, i);
        GHIPostState state = { 0 };

        state.data = item;
        if (!ghiPostStateInit(&state)) {
            for (i--; i >= 0; i--) {
                ghiPostStateCleanup(ArrayNth(connection->postingState.states, i));
            }
            ArrayFree(connection->postingState.states);
            connection->postingState.states = NULL;
            return gsi_false;
        }
        ArrayAppend(connection->postingState.states, &state);
    }
    connection->postingState.totalBytes = ghiPostGetContentLength(connection);
    if (connection->post->hasSoap == gsi_true) {
        connection->postingState.waitPostContinue = gsi_true;
    } else {
        connection->postingState.waitPostContinue = gsi_false;
    }
    return gsi_true;
}

void ghiPostCleanupState(GHIConnection *connection) {
    int count;
    int i;

    if (connection->postingState.states != NULL) {
        count = ArrayLength(connection->postingState.states);
        for (i = 0; i < count; i++) {
            ghiPostStateCleanup(ArrayNth(connection->postingState.states, i));
        }
        ArrayFree(connection->postingState.states);
        connection->postingState.states = NULL;
    }
    if (connection->post != NULL && connection->post->autoFree) {
        ghiFreePost(connection->post);
        connection->post = NULL;
    }
}

// Sends a string item's value, urlencoded if the post is. Returns 0 for an error, 1 once it is all sent
static GHIPostingResult ghiPostStringStateDoPosting(GHIPostState *state, GHIConnection *connection) {
    GHIPostData *item = state->data;
    GHTTPPost post;

    if (item->data.string.length == 0) {
        return gsi_true;
    }
    post = connection->post;
    if (!post->hasFiles && !post->hasSoap && item->data.string.unk_10 != 0) {
        char *value = item->data.string.value;
        char escape[4] = "%00";
        GHIBuffer *buffer;
        const char *hexDigits;
        int i;
        char c;

        if (connection->encryptor.mEngine == 0) {
            buffer = &connection->sendBuffer;
        } else {
            buffer = &connection->encodeBuffer;
        }
        i = 0;
        c = value[0];
        while (c != '\0') {
            if (strchr(sUnreservedChars, c) != NULL) {
                ghiAppendCharToBuffer(buffer, c);
            } else if (c == ' ') {
                ghiAppendCharToBuffer(buffer, '+');
            } else {
                hexDigits = "0123456789ABCDEF";
                escape[1] = hexDigits[c / 16];
                escape[2] = hexDigits[c % 16];
                ghiAppendDataToBuffer(buffer, escape, 3);
            }
            i++;
            c = value[i];
        }
    } else {
        switch (ghiTrySendThenBuffer(connection, item->data.string.value, item->data.string.length)) {
        case 0:
            return gsi_false;
        default:
            return gsi_true;
        }
    }
    if (connection->encryptor.mEngine == 0) {
        if (!ghiSendBufferedData(connection)) {
            return gsi_false;
        }
        if (connection->sendBuffer.pos == connection->sendBuffer.len) {
            ghiResetBuffer(&connection->sendBuffer);
        }
        return gsi_true;
    } else {
        return gsi_true;
    }
}

// Sends an XML message, with the padding that a DIME record has
static GHIPostingResult ghiPostXmlStateDoPosting(GHIPostState *state, GHIConnection *connection) {
    GSXmlStreamWriter message = state->data->data.xml.message;
    char padding[3];
    int padLength = 0;

    padding[0] = 0;
    padding[1] = 0;
    padding[2] = 0;
    if (connection->post->useDIME) {
        padLength = 4 - gsXmlWriterGetDataLength(message) % 4;
        if (padLength == 4) {
            padLength = 0;
        }
    }
    if (connection->encryptor.mEngine != 0 && connection->encryptor.mEncryptOnBuffer == 1) {
        if (!ghiAppendDataToBuffer(&connection->encodeBuffer, gsXmlWriterGetData(message),
                                   gsXmlWriterGetDataLength(message)) ||
            !ghiAppendDataToBuffer(&connection->encodeBuffer, padding, padLength) ||
            !ghiEncryptDataToBuffer(&connection->sendBuffer, connection->encodeBuffer.data,
                                    connection->encodeBuffer.len)) {
            return gsi_false;
        }
        ghiResetBuffer(&connection->encodeBuffer);
        if (!ghiSendBufferedData(connection)) {
            return gsi_false;
        }
        if (connection->sendBuffer.pos == connection->sendBuffer.len) {
            ghiResetBuffer(&connection->sendBuffer);
        }
        return gsi_true;
    }
    if (!ghiTrySendThenBuffer(connection, gsXmlWriterGetData(message), gsXmlWriterGetDataLength(message))) {
        return gsi_false;
    }
    switch (ghiTrySendThenBuffer(connection, padding, padLength)) {
    case 0:
        return gsi_false;
    default:
        return gsi_true;
    }
}

// Sends a file of the disk, a piece at a time. Returns 0 for an error, 1 once it is all sent and 2 to go on later
static GHIPostingResult ghiPostFileDiskStateDoPosting(GHIPostState *state, GHIConnection *connection) {
    char buffer[0x1000];
    int count;
    int result;

    do {
        count = fread(buffer, 1, sizeof(buffer), state->file);
        if (count <= 0) {
            connection->completed = gsi_true;
            connection->result = 14;
            return 0;
        }
        state->pos += count;
        if (state->pos > state->fileSize) {
            connection->completed = gsi_true;
            connection->result = 14;
            return 0;
        }
        result = ghiTrySendThenBuffer(connection, buffer, count);
        if (!result) {
            return 0;
        }
        if (state->pos == state->fileSize) {
            if (connection->post->useDIME) {
                char padding[3] = { 0, 0, 0 };
                int padLength;

                padLength = 4 - state->fileSize % 4;
                if (padLength != 4 && padLength > 0) {
                    if (!ghiTrySendThenBuffer(connection, padding, padLength)) {
                        return 0;
                    }
                }
            }
            return 1;
        }
    } while (result == 1);
    return 2;
}

// Sends a file that is in memory: the same
static GHIPostingResult ghiPostFileMemoryStateDoPosting(GHIPostState *state, GHIConnection *connection) {
    GHIPostData *item = state->data;
    int length = item->data.memory.length;
    int result;

    if (length == 0) {
        return 1;
    }
    if (connection->encryptor.mEngine == 0) {
        do {
            result = ghiDoSend(connection, item->data.memory.data + state->pos, length - state->pos);
            if (result == -1) {
                return 0;
            }
            state->pos += result;
            item = state->data;
            length = item->data.memory.length;
            if (length == state->pos) {
                if (connection->post->useDIME) {
                    char padding[3] = { 0, 0, 0 };
                    int padLength;

                    padLength = 4 - item->data.memory.length % 4;
                    if (padLength != 4 && padLength > 0) {
                        if (!ghiTrySendThenBuffer(connection, padding, padLength)) {
                            return 0;
                        }
                    }
                }
                return 1;
            }
        } while (result != 0);
        return 2;
    }
    do {
        int chunk = length - state->pos;

        if (chunk >= GHI_ENCRYPT_CHUNK_SIZE) {
            chunk = GHI_ENCRYPT_CHUNK_SIZE;
        }
        result = ghiTrySendThenBuffer(connection, item->data.memory.data + state->pos, chunk);
        if (!result) {
            return 0;
        }
        state->pos += chunk;
        item = state->data;
        length = item->data.memory.length;
        if (length == state->pos) {
            if (connection->post->useDIME) {
                char padding[3] = { 0, 0, 0 };
                int padLength;

                padLength = 4 - item->data.memory.length % 4;
                if (padLength != 4 && padLength > 0) {
                    if (!ghiTrySendThenBuffer(connection, padding, padLength)) {
                        return 0;
                    }
                }
            }
            return 1;
        }
    } while (result == 1);
    return 2;
}

// Sends an item: its head (once) and then what it holds. Returns 0 for an error, 1 once it is all sent and 2 to go on
// later
static GHIPostingResult ghiPostStateDoPosting(GHIPostState *state, GHIConnection *connection, GHTTPBool first,
                                              GHTTPBool last) {
    char buffer[0x800];
    GHIDimeHeader headers[2];
    int length = 0;
    int result;

    if (state->pos == -1) {
        state->pos = 0;
        if (!connection->post->hasFiles && !connection->post->hasSoap) {
            if (first) {
                OS_SPrintf(buffer, "%s=", state->data->name);
            } else {
                OS_SPrintf(buffer, "&%s=", state->data->name);
            }
        } else {
            GHIPostData *item = state->data;

            if (item->type == GHIString) {
                OS_SPrintf(buffer, "%sContent-Disposition: form-data; name=\"%s\"\r\n\r\n",
                           first ? GHI_BOUNDARY_LINE "\r\n" : "\r\n" GHI_BOUNDARY_LINE "\r\n", item->name);
            } else if (item->type == GHIXmlData) {
                if (connection->post->useDIME) {
                    int count;
                    int padding;

                    headers[1].mVersionAndFlags = GHI_DIME_VERSION;
                    if (first) {
                        headers[1].mVersionAndFlags |= GHI_DIME_FIRST;
                    }
                    if (last) {
                        headers[1].mVersionAndFlags |= GHI_DIME_LAST;
                    }
                    headers[1].mTypeT = GHI_DIME_TYPE_URI;
                    headers[1].mOptionsLength = 0;
                    headers[1].mIdLength = GHI_DIME_LENGTH16(strlen(GHI_MESSAGE_ID));
                    headers[1].mTypeLength = GHI_DIME_LENGTH16(strlen(GHI_MESSAGE_TYPE));
                    headers[1].mDataLength = GHI_SWAP32(gsXmlWriterGetDataLength(state->data->data.xml.message));
                    ghiCopy(buffer, &headers[1], sizeof(GHIDimeHeader));
                    length += sizeof(GHIDimeHeader);
                    count = gsiSafeStrcpyA(buffer + length, GHI_MESSAGE_ID, sizeof(buffer) - length);
                    length += count;
                    padding = 4 - (count & 3);
                    if (padding != 4) {
                        while (padding-- > 0) {
                            buffer[length++] = 0;
                        }
                    }
                    count = gsiSafeStrcpyA(buffer + length, GHI_MESSAGE_TYPE, sizeof(buffer) - length);
                    length += count;
                    padding = 4 - (count & 3);
                    if (padding != 4) {
                        while (padding-- > 0) {
                            buffer[length++] = 0;
                        }
                    }
                } else {
                    buffer[0] = '\0';
                }
            } else if (item->type == GHIFileDisk || item->type == GHIFileMemory) {
                int dataLength;
                char *fileName;
                char *contentType;

                if (item->type == GHIFileDisk) {
                    dataLength = state->fileSize;
                    fileName = item->data.disk.fileName;
                    contentType = item->data.disk.contentType;
                } else {
                    dataLength = item->data.memory.length;
                    fileName = item->data.memory.fileName;
                    contentType = item->data.memory.contentType;
                }
                if (connection->post->useDIME) {
                    int count;
                    int padding;
                    char *name;

                    headers[0].mVersionAndFlags = GHI_DIME_VERSION;
                    if (first) {
                        headers[0].mVersionAndFlags |= GHI_DIME_FIRST;
                    }
                    if (last) {
                        headers[0].mVersionAndFlags |= GHI_DIME_LAST;
                    }
                    headers[0].mTypeT = GHI_DIME_TYPE_MEDIA;
                    headers[0].mOptionsLength = 0;
                    name = state->data->name;
                    headers[0].mIdLength = GHI_DIME_LENGTH16(strlen(name));
                    headers[0].mTypeLength = GHI_DIME_LENGTH16(strlen(contentType));
                    headers[0].mDataLength = GHI_SWAP32(dataLength);
                    ghiCopy(buffer, &headers[0], sizeof(GHIDimeHeader));
                    length += sizeof(GHIDimeHeader);
                    count = gsiSafeStrcpyA(buffer + length, state->data->name, sizeof(buffer) - length);
                    length += count;
                    padding = 4 - (count & 3);
                    if (padding != 4) {
                        while (padding-- > 0) {
                            buffer[length++] = 0;
                        }
                    }
                    count = gsiSafeStrcpyA(buffer + length, contentType, sizeof(buffer) - length);
                    length += count;
                    padding = 4 - (count & 3);
                    if (padding != 4) {
                        while (padding-- > 0) {
                            buffer[length++] = 0;
                        }
                    }
                } else {
                    OS_SPrintf(buffer,
                               "%sContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: "
                               "%s\r\n\r\n",
                               first ? GHI_BOUNDARY_LINE "\r\n" : "\r\n" GHI_BOUNDARY_LINE "\r\n", item->name, fileName,
                               contentType);
                }
            }
        }

        if (connection->encryptor.mEngine != 0 && connection->encryptor.mEncryptOnBuffer == 1) {
            if (length == 0) {
                length = strlen(buffer);
            }
            if (!ghiEncryptDataToBuffer(&connection->sendBuffer, buffer, length)) {
                return 0;
            }
            if (!ghiSendBufferedData(connection)) {
                return 0;
            }
            if (connection->sendBuffer.pos < connection->sendBuffer.len) {
                return 2;
            }
        } else {
            if (length == 0) {
                length = strlen(buffer);
            }
            result = ghiTrySendThenBuffer(connection, buffer, length);
            if (!result) {
                return 0;
            }
            if (result == 2) {
                return 2;
            }
        }
        ghiResetBuffer(&connection->sendBuffer);
    }

    if (state->data->type == GHIString) {
        return ghiPostStringStateDoPosting(state, connection);
    }
    if (state->data->type == GHIXmlData) {
        return ghiPostXmlStateDoPosting(state, connection);
    }
    if (state->data->type == GHIFileDisk) {
        return ghiPostFileDiskStateDoPosting(state, connection);
    }
    return ghiPostFileMemoryStateDoPosting(state, connection);
}

// Sends the post of a connection, an item at a time. Returns 0 for an error, 1 once it is all sent, 2 to go on later
// and 3 once the head of an XML message is sent
GHIPostingResult ghiPostDoPosting(GHIConnection *connection) {
    GHIPostingState *posting = &connection->postingState;
    int count = ArrayLength(posting->states);

    if (connection->sendBuffer.pos < connection->sendBuffer.len) {
        if (!ghiSendBufferedData(connection)) {
            return 0;
        }
        if (connection->sendBuffer.pos < connection->sendBuffer.len) {
            return 2;
        }
        ghiResetBuffer(&connection->sendBuffer);
        if (connection->postingState.waitPostContinue) {
            return 3;
        }
        if (connection->postingState.index == count) {
            return 1;
        }
    }

    if (connection->postingState.waitPostContinue) {
        if (connection->post->hasFiles || connection->post->hasSoap) {
            const char *lineEnd = "\r\n";
            int result = ghiTrySendThenBuffer(connection, lineEnd, strlen(lineEnd));

            if (!result) {
                return 0;
            }
            if (result == 2) {
                return 2;
            }
            if (connection->postingState.waitPostContinue == 1) {
                return 3;
            }
        } else {
            connection->postingState.waitPostContinue = gsi_false;
        }
    }

    while (posting->index < count) {
        GHIPostState *state = ArrayNth(posting->states, posting->index);
        int index = posting->index;
        gsi_bool last = index == ArrayLength(posting->states) - 1;
        int result = ghiPostStateDoPosting(state, connection, index == 0, last);

        if (!result) {
            return 0;
        }
        if (result == 2) {
            return 2;
        }
        posting->index++;
    }

    if (connection->encryptor.mEngine != 0) {
        if (connection->encodeBuffer.len > 0) {
            if (!ghiEncryptDataToBuffer(&connection->sendBuffer, connection->encodeBuffer.data,
                                        connection->encodeBuffer.len)) {
                return 0;
            }
            ghiResetBuffer(&connection->encodeBuffer);
        }
    }
    if (connection->post->hasFiles && !connection->post->useDIME) {
        const char *lastBoundary = "\r\n" GHI_BOUNDARY_LINE "--\r\n";

        if (!ghiTrySendThenBuffer(connection, lastBoundary, strlen(lastBoundary))) {
            return 0;
        }
    }
    if (connection->sendBuffer.pos < connection->sendBuffer.len) {
        return 2;
    }
    return 1;
}
