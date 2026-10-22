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
    } u;
} GHIPostData;

// How far an item of the post of a connection has got
typedef struct GHIPostState {
    GHIPostData *item;
    int position;
    void *file;
    long fileSize;
} GHIPostState;

// The header of a DIME record, with its numbers in network order
typedef struct GHIDimeHeader {
    gsi_u8 flags;
    gsi_u8 type;
    gsi_u16 optionsLength;
    gsi_u16 idLength;
    gsi_u16 typeLength;
    gsi_u32 dataLength;
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
static void ghiFreePostItem(void *element) {
    GHIPostData *item = element;

    if (item->type != GHIXmlData) {
        gsifree(item->name);
    }
    if (item->type == GHIString) {
        gsifree(item->u.string.value);
    } else if (item->type == GHIFileDisk) {
        gsifree(item->u.disk.path);
        gsifree(item->u.disk.fileName);
        gsifree(item->u.disk.contentType);
    } else if (item->type == GHIFileMemory) {
        gsifree(item->u.memory.fileName);
        gsifree(item->u.memory.contentType);
    } else if (item->type == GHIXmlData) {
        gsXmlFreeWriter(item->u.xml.message);
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
    post->items = func_ov011_02171514(sizeof(GHIPostData), 0, ghiFreePostItem);
    if (post->items == NULL) {
        gsifree(post);
        return NULL;
    }
    return post;
}

void ghiPostSetAutoFree(GHTTPPost post, GHTTPBool autoFree) {
    post->autoFree = autoFree;
}

void ghiFreePost(GHTTPPost post) {
    if (post->items != NULL) {
        func_ov011_02171558(post->items);
    }
    post->items = NULL;
    gsifree(post);
}

// Frees a post of a request, and what the request keeps of it
void ghiFreePostOfRequest(GHTTPRequest request, GHTTPPost post) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        ghiPostCleanupState(connection);
    }
    ghiFreePost(post);
}

// Adds a file that is in memory: the post copies the texts but not the bytes. Returns gsi_false if it can't
GHTTPBool ghttpPostAddFileFromMemory(GHTTPPost post, const gsi_char *field, const char *bytes, int size,
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
        item.u.memory.data = bytes;
        item.u.memory.length = size;
        item.u.memory.fileName = fileNameCopy;
        item.u.memory.contentType = contentTypeCopy;
        func_ov011_021715a4(post->items, &item);
        post->hasFiles = gsi_true;
        if (post->hasXml == gsi_true) {
            post->mixed = gsi_true;
        }
        return gsi_true;
    }
}

// Adds an XML message, which the post takes over
GHTTPBool ghiPostAddXml(GHTTPPost post, GSXmlStreamWriter message) {
    GHIPostData item;

    item.type = GHIXmlData;
    item.u.xml.message = message;
    func_ov011_021715a4(post->items, &item);
    post->hasXml = gsi_true;
    if (post->hasFiles == gsi_true) {
        post->mixed = gsi_true;
    }
    return gsi_true;
}

// The content type that the request says its post is
const char *ghiPostGetContentType(GHIConnection *connection) {
    GHTTPPost post = connection->post;

    if (post == NULL) {
        return "";
    }
    if (post->mixed) {
        return "application/dime";
    }
    if (post->hasFiles) {
        return "multipart/form-data; boundary=" GHI_BOUNDARY;
    }
    if (post->hasXml) {
        return "text/xml";
    }
    return "application/x-www-form-urlencoded";
}

// The length of a post without files: the names, the values (as they will be urlencoded) and the = and & between them
static int ghiPostGetUrlEncodedLength(GHIConnection *connection) {
    GHTTPPost post = connection->post;
    int length = 0;
    int count = func_ov011_02171588(post->items);
    int i;

    if (count == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        GHIPostData *item = func_ov011_0217158c(post->items, i);

        if (item->type == GHIString) {
            length += strlen(item->name);
            length += item->u.string.length;
            length += item->u.string.escapeCount * 2;
            length += 1;
        } else if (item->type == GHIXmlData) {
            length += gsXmlWriterGetDataLength(item->u.xml.message);
        }
    }
    return length + (count - 1);
}

// The length of a post with files: what the multipart format or the DIME one add to each item, and the item
static int ghiPostGetMultipartLength(GHIConnection *connection) {
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

    if (post->mixed) {
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

    count = func_ov011_02171588(post->items);
    for (i = 0; i < count; i++) {
        item = func_ov011_0217158c(post->items, i);

        if (item->type == GHIString) {
            length += stringExtra;
            length += strlen(item->name);
            length += item->u.string.length;
        } else if (item->type == GHIFileDisk) {
            GHIPostState *state;
            gsi_bool mixed;
            int fileSize;

            length += fileExtra;
            length += strlen(item->name);
            length += strlen(item->u.disk.contentType);
            state = func_ov011_0217158c(connection->postingState.states, i);
            fileSize = state->fileSize;
            length += fileSize;
            mixed = post->mixed;
            if (!mixed) {
                length += strlen(item->u.disk.fileName);
            }
            if (mixed) {
                padding = GHI_DIME_PAD((int)strlen(item->name));
                if (padding != 4) {
                    length += padding;
                }
                padding = GHI_DIME_PAD((int)strlen(item->u.disk.contentType));
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
            contentType = item->u.memory.contentType;
            length += strlen(contentType);
            dataLength = item->u.memory.length;
            length += dataLength;
            mixed = post->mixed;
            if (!mixed) {
                length += strlen(item->u.memory.fileName);
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
            length += gsXmlWriterGetDataLength(item->u.xml.message);
            padding = GHI_DIME_PAD(gsXmlWriterGetDataLength(item->u.xml.message));
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

int ghiPostGetLength(GHIConnection *connection) {
    if (connection->post == NULL) {
        return 0;
    }
    if (connection->post->hasFiles) {
        return ghiPostGetMultipartLength(connection);
    }
    return ghiPostGetUrlEncodedLength(connection);
}

// Starts an item's state: a file on disk is measured
static gsi_bool ghiPostItemStateInit(GHIPostState *state) {
    int type = state->item->type;

    state->position = -1;
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

static void ghiPostItemStateCleanup(GHIPostState *state) {
    int type = state->item->type;

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
    connection->postingState.finished = 0;
    connection->postingState.callback = post->callback;
    connection->postingState.param = post->param;
    count = func_ov011_02171588(post->items);
    connection->postingState.states = func_ov011_02171514(sizeof(GHIPostState), count, NULL);
    if (connection->postingState.states == NULL) {
        return gsi_false;
    }
    for (i = 0; i < count; i++) {
        GHIPostData *item = func_ov011_0217158c(connection->post->items, i);
        GHIPostState state = { 0 };

        state.item = item;
        if (!ghiPostItemStateInit(&state)) {
            for (i--; i >= 0; i--) {
                ghiPostItemStateCleanup(func_ov011_0217158c(connection->postingState.states, i));
            }
            func_ov011_02171558(connection->postingState.states);
            connection->postingState.states = NULL;
            return gsi_false;
        }
        func_ov011_021715a4(connection->postingState.states, &state);
    }
    connection->postingState.totalBytes = ghiPostGetLength(connection);
    if (connection->post->hasXml == gsi_true) {
        connection->postingState.isXml = gsi_true;
    } else {
        connection->postingState.isXml = gsi_false;
    }
    return gsi_true;
}

void ghiPostCleanupState(GHIConnection *connection) {
    int count;
    int i;

    if (connection->postingState.states != NULL) {
        count = func_ov011_02171588(connection->postingState.states);
        for (i = 0; i < count; i++) {
            ghiPostItemStateCleanup(func_ov011_0217158c(connection->postingState.states, i));
        }
        func_ov011_02171558(connection->postingState.states);
        connection->postingState.states = NULL;
    }
    if (connection->post != NULL && connection->post->autoFree) {
        ghiFreePost(connection->post);
        connection->post = NULL;
    }
}

// Sends a string item's value, urlencoded if the post is. Returns 0 for an error, 1 once it is all sent
static GHIPostingResult ghiPostStringStateDoPosting(GHIPostState *state, GHIConnection *connection) {
    GHIPostData *item = state->item;
    GHTTPPost post;

    if (item->u.string.length == 0) {
        return gsi_true;
    }
    post = connection->post;
    if (!post->hasFiles && !post->hasXml && item->u.string.unk_10 != 0) {
        char *value = item->u.string.value;
        char escape[4] = "%00";
        GHIBuffer *buffer;
        const char *hexDigits;
        int i;
        char c;

        if (connection->encryptor.engine == 0) {
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
        switch (ghiTrySendThenBuffer(connection, item->u.string.value, item->u.string.length)) {
        case 0:
            return gsi_false;
        default:
            return gsi_true;
        }
    }
    if (connection->encryptor.engine == 0) {
        if (!ghiSendBufferedData(connection)) {
            return gsi_false;
        }
        if (connection->sendBuffer.pos == connection->sendBuffer.length) {
            ghiResetBuffer(&connection->sendBuffer);
        }
        return gsi_true;
    } else {
        return gsi_true;
    }
}

// Sends an XML message, with the padding that a DIME record has
static GHIPostingResult ghiPostXmlStateDoPosting(GHIPostState *state, GHIConnection *connection) {
    GSXmlStreamWriter message = state->item->u.xml.message;
    char padding[3];
    int padLength = 0;

    padding[0] = 0;
    padding[1] = 0;
    padding[2] = 0;
    if (connection->post->mixed) {
        padLength = 4 - gsXmlWriterGetDataLength(message) % 4;
        if (padLength == 4) {
            padLength = 0;
        }
    }
    if (connection->encryptor.engine != 0 && connection->encryptor.mode == 1) {
        if (!ghiAppendDataToBuffer(&connection->encodeBuffer, gsXmlWriterGetData(message),
                                   gsXmlWriterGetDataLength(message)) ||
            !ghiAppendDataToBuffer(&connection->encodeBuffer, padding, padLength) ||
            !ghiEncryptDataToBuffer(&connection->sendBuffer, connection->encodeBuffer.data,
                                    connection->encodeBuffer.length)) {
            return gsi_false;
        }
        ghiResetBuffer(&connection->encodeBuffer);
        if (!ghiSendBufferedData(connection)) {
            return gsi_false;
        }
        if (connection->sendBuffer.pos == connection->sendBuffer.length) {
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
        state->position += count;
        if (state->position > state->fileSize) {
            connection->completed = gsi_true;
            connection->result = 14;
            return 0;
        }
        result = ghiTrySendThenBuffer(connection, buffer, count);
        if (!result) {
            return 0;
        }
        if (state->position == state->fileSize) {
            if (connection->post->mixed) {
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
    GHIPostData *item = state->item;
    int length = item->u.memory.length;
    int result;

    if (length == 0) {
        return 1;
    }
    if (connection->encryptor.engine == 0) {
        do {
            result = ghiDoSend(connection, item->u.memory.data + state->position, length - state->position);
            if (result == -1) {
                return 0;
            }
            state->position += result;
            item = state->item;
            length = item->u.memory.length;
            if (length == state->position) {
                if (connection->post->mixed) {
                    char padding[3] = { 0, 0, 0 };
                    int padLength;

                    padLength = 4 - item->u.memory.length % 4;
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
        int chunk = length - state->position;

        if (chunk >= GHI_ENCRYPT_CHUNK_SIZE) {
            chunk = GHI_ENCRYPT_CHUNK_SIZE;
        }
        result = ghiTrySendThenBuffer(connection, item->u.memory.data + state->position, chunk);
        if (!result) {
            return 0;
        }
        state->position += chunk;
        item = state->item;
        length = item->u.memory.length;
        if (length == state->position) {
            if (connection->post->mixed) {
                char padding[3] = { 0, 0, 0 };
                int padLength;

                padLength = 4 - item->u.memory.length % 4;
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

    if (state->position == -1) {
        state->position = 0;
        if (!connection->post->hasFiles && !connection->post->hasXml) {
            if (first) {
                OS_SPrintf(buffer, "%s=", state->item->name);
            } else {
                OS_SPrintf(buffer, "&%s=", state->item->name);
            }
        } else {
            GHIPostData *item = state->item;

            if (item->type == GHIString) {
                OS_SPrintf(buffer, "%sContent-Disposition: form-data; name=\"%s\"\r\n\r\n",
                           first ? GHI_BOUNDARY_LINE "\r\n" : "\r\n" GHI_BOUNDARY_LINE "\r\n", item->name);
            } else if (item->type == GHIXmlData) {
                if (connection->post->mixed) {
                    int count;
                    int padding;

                    headers[1].flags = GHI_DIME_VERSION;
                    if (first) {
                        headers[1].flags |= GHI_DIME_FIRST;
                    }
                    if (last) {
                        headers[1].flags |= GHI_DIME_LAST;
                    }
                    headers[1].type = GHI_DIME_TYPE_URI;
                    headers[1].optionsLength = 0;
                    headers[1].idLength = GHI_DIME_LENGTH16(strlen(GHI_MESSAGE_ID));
                    headers[1].typeLength = GHI_DIME_LENGTH16(strlen(GHI_MESSAGE_TYPE));
                    headers[1].dataLength = GHI_SWAP32(gsXmlWriterGetDataLength(state->item->u.xml.message));
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
                    fileName = item->u.disk.fileName;
                    contentType = item->u.disk.contentType;
                } else {
                    dataLength = item->u.memory.length;
                    fileName = item->u.memory.fileName;
                    contentType = item->u.memory.contentType;
                }
                if (connection->post->mixed) {
                    int count;
                    int padding;
                    char *name;

                    headers[0].flags = GHI_DIME_VERSION;
                    if (first) {
                        headers[0].flags |= GHI_DIME_FIRST;
                    }
                    if (last) {
                        headers[0].flags |= GHI_DIME_LAST;
                    }
                    headers[0].type = GHI_DIME_TYPE_MEDIA;
                    headers[0].optionsLength = 0;
                    name = state->item->name;
                    headers[0].idLength = GHI_DIME_LENGTH16(strlen(name));
                    headers[0].typeLength = GHI_DIME_LENGTH16(strlen(contentType));
                    headers[0].dataLength = GHI_SWAP32(dataLength);
                    ghiCopy(buffer, &headers[0], sizeof(GHIDimeHeader));
                    length += sizeof(GHIDimeHeader);
                    count = gsiSafeStrcpyA(buffer + length, state->item->name, sizeof(buffer) - length);
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

        if (connection->encryptor.engine != 0 && connection->encryptor.mode == 1) {
            if (length == 0) {
                length = strlen(buffer);
            }
            if (!ghiEncryptDataToBuffer(&connection->sendBuffer, buffer, length)) {
                return 0;
            }
            if (!ghiSendBufferedData(connection)) {
                return 0;
            }
            if (connection->sendBuffer.pos < connection->sendBuffer.length) {
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

    if (state->item->type == GHIString) {
        return ghiPostStringStateDoPosting(state, connection);
    }
    if (state->item->type == GHIXmlData) {
        return ghiPostXmlStateDoPosting(state, connection);
    }
    if (state->item->type == GHIFileDisk) {
        return ghiPostFileDiskStateDoPosting(state, connection);
    }
    return ghiPostFileMemoryStateDoPosting(state, connection);
}

// Sends the post of a connection, an item at a time. Returns 0 for an error, 1 once it is all sent, 2 to go on later
// and 3 once the head of an XML message is sent
GHIPostingResult ghiPostDoPosting(GHIConnection *connection) {
    GHIPostingState *posting = &connection->postingState;
    int count = func_ov011_02171588(posting->states);

    if (connection->sendBuffer.pos < connection->sendBuffer.length) {
        if (!ghiSendBufferedData(connection)) {
            return 0;
        }
        if (connection->sendBuffer.pos < connection->sendBuffer.length) {
            return 2;
        }
        ghiResetBuffer(&connection->sendBuffer);
        if (connection->postingState.isXml) {
            return 3;
        }
        if (connection->postingState.index == count) {
            return 1;
        }
    }

    if (connection->postingState.isXml) {
        if (connection->post->hasFiles || connection->post->hasXml) {
            const char *lineEnd = "\r\n";
            int result = ghiTrySendThenBuffer(connection, lineEnd, strlen(lineEnd));

            if (!result) {
                return 0;
            }
            if (result == 2) {
                return 2;
            }
            if (connection->postingState.isXml == 1) {
                return 3;
            }
        } else {
            connection->postingState.isXml = gsi_false;
        }
    }

    while (posting->index < count) {
        GHIPostState *state = func_ov011_0217158c(posting->states, posting->index);
        int index = posting->index;
        gsi_bool last = index == func_ov011_02171588(posting->states) - 1;
        int result = ghiPostStateDoPosting(state, connection, index == 0, last);

        if (!result) {
            return 0;
        }
        if (result == 2) {
            return 2;
        }
        posting->index++;
    }

    if (connection->encryptor.engine != 0) {
        if (connection->encodeBuffer.length > 0) {
            if (!ghiEncryptDataToBuffer(&connection->sendBuffer, connection->encodeBuffer.data,
                                        connection->encodeBuffer.length)) {
                return 0;
            }
            ghiResetBuffer(&connection->encodeBuffer);
        }
    }
    if (connection->post->hasFiles && !connection->post->mixed) {
        const char *lastBoundary = "\r\n" GHI_BOUNDARY_LINE "--\r\n";

        if (!ghiTrySendThenBuffer(connection, lastBoundary, strlen(lastBoundary))) {
            return 0;
        }
    }
    if (connection->sendBuffer.pos < connection->sendBuffer.length) {
        return 2;
    }
    return 1;
}
