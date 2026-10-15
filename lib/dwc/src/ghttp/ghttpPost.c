#include "ghttp_internal.h"


// ghttpPost: the posts of a request (the form or the XML message that it sends), made of items, and how they are sent.
// The posts are urlencoded if they hold strings alone, multipart (form-data) if they hold files, and DIME messages if
// they hold files and an XML message

// What an item holds
typedef enum GHIPostItemType {
    GHI_POST_STRING,
    GHI_POST_DISK_FILE,
    GHI_POST_MEMORY_FILE,
    GHI_POST_XML
} GHIPostItemType;

typedef struct GHIPostItem {
    int type;
    char *name;
    union {
        struct {
            char *value;
            int length;
            int unk_10;
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
            SoapWriter *message;
        } xml;
    } u;
} GHIPostItem;

// How far an item of the post of a connection has got
typedef struct GHIPostItemState {
    GHIPostItem *item;
    int position;
    void *file;
    int fileSize;
} GHIPostItemState;

// The header of a DIME record, with its numbers in network order
typedef struct GHIDimeHeader {
    u8 flags;
    u8 type;
    u16 optionsLength;
    u16 idLength;
    u16 typeLength;
    u32 dataLength;
} GHIDimeHeader;

#define GHI_DIME_VERSION 0x08
#define GHI_DIME_FIRST 0x04
#define GHI_DIME_LAST 0x02
#define GHI_DIME_TYPE_MEDIA 0x10
#define GHI_DIME_TYPE_URI 0x20

// The length of a text as the 16 bits of a DIME header, in network order
#define GHI_DIME_LENGTH16(x) GHI_SWAP16((u16)(s16)(x))
#define GHI_SWAP32(x)                                                                                    \
    (((((u32)(x)) << 24) & 0xff000000) |                                                                 \
     (((((u32)(x)) << 8) & 0x00ff0000) | (((((u32)(x)) >> 24) & 0xff) | ((((u32)(x)) >> 8) & 0x0000ff00))))

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
    GHIPostItem *item = element;

    if (item->type != GHI_POST_XML) {
        func_ov011_0216ddfc(item->name);
    }
    if (item->type == GHI_POST_STRING) {
        func_ov011_0216ddfc(item->u.string.value);
    } else if (item->type == GHI_POST_DISK_FILE) {
        func_ov011_0216ddfc(item->u.disk.path);
        func_ov011_0216ddfc(item->u.disk.fileName);
        func_ov011_0216ddfc(item->u.disk.contentType);
    } else if (item->type == GHI_POST_MEMORY_FILE) {
        func_ov011_0216ddfc(item->u.memory.fileName);
        func_ov011_0216ddfc(item->u.memory.contentType);
    } else if (item->type == GHI_POST_XML) {
        func_ov011_0216fe64(item->u.xml.message);
    }
}

GHIPost *ghiNewPost(void) {
    GHIPost *post = func_ov011_0216dddc(sizeof(GHIPost));
    char *p;
    u32 size;

    if (post == NULL) {
        return NULL;
    }
    p = (char *)post;
    size = sizeof(GHIPost);
    while (size--) {
        *p++ = 0;
    }
    post->autoFree = TRUE;
    post->items = func_ov011_02171514(sizeof(GHIPostItem), 0, ghiFreePostItem);
    if (post->items == NULL) {
        func_ov011_0216ddfc(post);
        return NULL;
    }
    return post;
}

void ghiPostSetAutoFree(GHIPost *post, BOOL autoFree) {
    post->autoFree = autoFree;
}

void ghiFreePost(GHIPost *post) {
    if (post->items != NULL) {
        func_ov011_02171558(post->items);
    }
    post->items = NULL;
    func_ov011_0216ddfc(post);
}

// Frees a post of a request, and what the request keeps of it
void ghiFreePostOfRequest(int request, GHIPost *post) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != NULL) {
        ghiPostCleanupState(connection);
    }
    ghiFreePost(post);
}

// Adds a file that is in memory: the post copies the texts but not the bytes. Returns FALSE if it can't
int ghttpPostAddFileFromMemory(GHIPost *post, const char *field, const void *bytes, int size, const char *fileName,
                               const char *contentType) {
    char *fieldCopy = func_ov011_0216e2c8(field);
    char *fileNameCopy = func_ov011_0216e2c8(fileName);
    char *contentTypeCopy = func_ov011_0216e2c8(contentType);

    if (fieldCopy == NULL || fileNameCopy == NULL || contentTypeCopy == NULL) {
        func_ov011_0216ddfc(fieldCopy);
        func_ov011_0216ddfc(fileNameCopy);
        func_ov011_0216ddfc(contentTypeCopy);
        return FALSE;
    } else {
        GHIPostItem item = {0};

        item.type = GHI_POST_MEMORY_FILE;
        item.name = fieldCopy;
        item.u.memory.data = bytes;
        item.u.memory.length = size;
        item.u.memory.fileName = fileNameCopy;
        item.u.memory.contentType = contentTypeCopy;
        func_ov011_021715a4(post->items, &item);
        post->hasFiles = TRUE;
        if (post->hasXml == TRUE) {
            post->mixed = TRUE;
        }
        return TRUE;
    }
}

// Adds an XML message, which the post takes over
BOOL ghiPostAddXml(GHIPost *post, SoapWriter *message) {
    GHIPostItem item;

    item.type = GHI_POST_XML;
    item.u.xml.message = message;
    func_ov011_021715a4(post->items, &item);
    post->hasXml = TRUE;
    if (post->hasFiles == TRUE) {
        post->mixed = TRUE;
    }
    return TRUE;
}

// The content type that the request says its post is
const char *ghiPostGetContentType(GHIConnection *connection) {
    GHIPost *post = connection->post;

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
    GHIPost *post = connection->post;
    int length = 0;
    int count = func_ov011_02171588(post->items);
    int i;

    if (count == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        GHIPostItem *item = func_ov011_0217158c(post->items, i);

        if (item->type == GHI_POST_STRING) {
            length += strlen(item->name);
            length += item->u.string.length;
            length += item->u.string.escapeCount * 2;
            length += 1;
        } else if (item->type == GHI_POST_XML) {
            length += func_ov011_0216fed0(item->u.xml.message);
        }
    }
    return length + (count - 1);
}

// The length of a post with files: what the multipart format or the DIME one add to each item, and the item
static int ghiPostGetMultipartLength(GHIConnection *connection) {
    GHIPost *post = connection->post;
    GHIPostItem *item;
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

        if (item->type == GHI_POST_STRING) {
            length += stringExtra;
            length += strlen(item->name);
            length += item->u.string.length;
        } else if (item->type == GHI_POST_DISK_FILE) {
            GHIPostItemState *state;
            BOOL mixed;
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
        } else if (item->type == GHI_POST_MEMORY_FILE) {
            BOOL mixed;
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
        } else if (item->type == GHI_POST_XML) {
            length += xmlExtra;
            length += func_ov011_0216fed0(item->u.xml.message);
            padding = GHI_DIME_PAD(func_ov011_0216fed0(item->u.xml.message));
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
static BOOL ghiPostItemStateInit(GHIPostItemState *state) {
    int type = state->item->type;

    state->position = -1;
    if (type == GHI_POST_STRING) {
    } else if (type == GHI_POST_DISK_FILE) {
        if (state->file == NULL) {
            return FALSE;
        }
        if (fseek(state->file, 0, 2) != 0) {
            return FALSE;
        }
        state->fileSize = ftell(state->file);
        if (state->fileSize == -1) {
            return FALSE;
        }
        rewind(state->file);
    } else if (type == GHI_POST_MEMORY_FILE || type == GHI_POST_XML) {
    } else {
        return FALSE;
    }
    return TRUE;
}

static void ghiPostItemStateCleanup(GHIPostItemState *state) {
    int type = state->item->type;

    if (type == GHI_POST_STRING) {
    } else if (type == GHI_POST_DISK_FILE) {
        if (state->file != NULL) {
            fclose(state->file);
        }
        state->file = NULL;
    }
}

// Makes the states of the items of the post of a connection and works out how long the post is
BOOL ghiPostInitState(GHIConnection *connection) {
    GHIPost *post = connection->post;
    int count;
    int i;

    if (post == NULL) {
        return FALSE;
    }
    connection->postingState.index = 0;
    connection->postingState.bytesPosted = 0;
    connection->postingState.totalBytes = 0;
    connection->postingState.finished = 0;
    connection->postingState.callback = post->callback;
    connection->postingState.unk_14 = post->unk_08;
    count = func_ov011_02171588(post->items);
    connection->postingState.states = func_ov011_02171514(sizeof(GHIPostItemState), count, NULL);
    if (connection->postingState.states == NULL) {
        return FALSE;
    }
    for (i = 0; i < count; i++) {
        GHIPostItem *item = func_ov011_0217158c(connection->post->items, i);
        GHIPostItemState state = {0};

        state.item = item;
        if (!ghiPostItemStateInit(&state)) {
            for (i--; i >= 0; i--) {
                ghiPostItemStateCleanup(func_ov011_0217158c(connection->postingState.states, i));
            }
            func_ov011_02171558(connection->postingState.states);
            connection->postingState.states = NULL;
            return FALSE;
        }
        func_ov011_021715a4(connection->postingState.states, &state);
    }
    connection->postingState.totalBytes = ghiPostGetLength(connection);
    if (connection->post->hasXml == TRUE) {
        connection->postingState.isXml = TRUE;
    } else {
        connection->postingState.isXml = FALSE;
    }
    return TRUE;
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
static BOOL ghiPostStringStateDoPosting(GHIPostItemState *state, GHIConnection *connection) {
    GHIPostItem *item = state->item;
    GHIPost *post;

    if (item->u.string.length == 0) {
        return TRUE;
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
            return FALSE;
        default:
            return TRUE;
        }
    }
    if (connection->encryptor.engine == 0) {
        if (!ghiSendBufferedData(connection)) {
            return FALSE;
        }
        if (connection->sendBuffer.pos == connection->sendBuffer.length) {
            ghiResetBuffer(&connection->sendBuffer);
        }
        return TRUE;
    } else {
        return TRUE;
    }
}

// Sends an XML message, with the padding that a DIME record has
static BOOL ghiPostXmlStateDoPosting(GHIPostItemState *state, GHIConnection *connection) {
    SoapWriter *message = state->item->u.xml.message;
    char padding[3];
    int padLength = 0;

    padding[0] = 0;
    padding[1] = 0;
    padding[2] = 0;
    if (connection->post->mixed) {
        padLength = 4 - func_ov011_0216fed0(message) % 4;
        if (padLength == 4) {
            padLength = 0;
        }
    }
    if (connection->encryptor.engine != 0 && connection->encryptor.mode == 1) {
        if (!ghiAppendDataToBuffer(&connection->encodeBuffer, func_ov011_0216fecc(message),
                                   func_ov011_0216fed0(message)) ||
            !ghiAppendDataToBuffer(&connection->encodeBuffer, padding, padLength) ||
            !ghiEncryptDataToBuffer(&connection->sendBuffer, connection->encodeBuffer.data,
                                    connection->encodeBuffer.length)) {
            return FALSE;
        }
        ghiResetBuffer(&connection->encodeBuffer);
        if (!ghiSendBufferedData(connection)) {
            return FALSE;
        }
        if (connection->sendBuffer.pos == connection->sendBuffer.length) {
            ghiResetBuffer(&connection->sendBuffer);
        }
        return TRUE;
    }
    if (!ghiTrySendThenBuffer(connection, func_ov011_0216fecc(message), func_ov011_0216fed0(message))) {
        return FALSE;
    }
    switch (ghiTrySendThenBuffer(connection, padding, padLength)) {
    case 0:
        return FALSE;
    default:
        return TRUE;
    }
}

// Sends a file of the disk, a piece at a time. Returns 0 for an error, 1 once it is all sent and 2 to go on later
static int ghiPostFileDiskStateDoPosting(GHIPostItemState *state, GHIConnection *connection) {
    char buffer[0x1000];
    int count;
    int result;

    do {
        count = __fread(buffer, 1, sizeof(buffer), state->file);
        if (count <= 0) {
            connection->completed = TRUE;
            connection->result = 14;
            return 0;
        }
        state->position += count;
        if (state->position > state->fileSize) {
            connection->completed = TRUE;
            connection->result = 14;
            return 0;
        }
        result = ghiTrySendThenBuffer(connection, buffer, count);
        if (!result) {
            return 0;
        }
        if (state->position == state->fileSize) {
            if (connection->post->mixed) {
                char padding[3] = {0, 0, 0};
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
static int ghiPostFileMemoryStateDoPosting(GHIPostItemState *state, GHIConnection *connection) {
    GHIPostItem *item = state->item;
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
                    char padding[3] = {0, 0, 0};
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
                char padding[3] = {0, 0, 0};
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
static int ghiPostStateDoPosting(GHIPostItemState *state, GHIConnection *connection, BOOL first, BOOL last) {
    char buffer[0x800];
    GHIDimeHeader headers[2];
    int length = 0;
    int result;

    if (state->position == -1) {
        state->position = 0;
        if (!connection->post->hasFiles && !connection->post->hasXml) {
            if (first) {
                func_0207a290(buffer, "%s=", state->item->name);
            } else {
                func_0207a290(buffer, "&%s=", state->item->name);
            }
        } else {
            GHIPostItem *item = state->item;

            if (item->type == GHI_POST_STRING) {
                func_0207a290(buffer, "%sContent-Disposition: form-data; name=\"%s\"\r\n\r\n",
                        first ? GHI_BOUNDARY_LINE "\r\n" : "\r\n" GHI_BOUNDARY_LINE "\r\n", item->name);
            } else if (item->type == GHI_POST_XML) {
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
                    headers[1].dataLength = GHI_SWAP32(func_ov011_0216fed0(state->item->u.xml.message));
                    ghiCopy(buffer, &headers[1], sizeof(GHIDimeHeader));
                    length += sizeof(GHIDimeHeader);
                    count = func_ov011_0216ee7c(buffer + length, GHI_MESSAGE_ID, sizeof(buffer) - length);
                    length += count;
                    padding = 4 - (count & 3);
                    if (padding != 4) {
                        while (padding-- > 0) {
                            buffer[length++] = 0;
                        }
                    }
                    count = func_ov011_0216ee7c(buffer + length, GHI_MESSAGE_TYPE, sizeof(buffer) - length);
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
            } else if (item->type == GHI_POST_DISK_FILE || item->type == GHI_POST_MEMORY_FILE) {
                int dataLength;
                char *fileName;
                char *contentType;

                if (item->type == GHI_POST_DISK_FILE) {
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
                    count = func_ov011_0216ee7c(buffer + length, state->item->name, sizeof(buffer) - length);
                    length += count;
                    padding = 4 - (count & 3);
                    if (padding != 4) {
                        while (padding-- > 0) {
                            buffer[length++] = 0;
                        }
                    }
                    count = func_ov011_0216ee7c(buffer + length, contentType, sizeof(buffer) - length);
                    length += count;
                    padding = 4 - (count & 3);
                    if (padding != 4) {
                        while (padding-- > 0) {
                            buffer[length++] = 0;
                        }
                    }
                } else {
                    func_0207a290(buffer,
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

    if (state->item->type == GHI_POST_STRING) {
        return ghiPostStringStateDoPosting(state, connection);
    }
    if (state->item->type == GHI_POST_XML) {
        return ghiPostXmlStateDoPosting(state, connection);
    }
    if (state->item->type == GHI_POST_DISK_FILE) {
        return ghiPostFileDiskStateDoPosting(state, connection);
    }
    return ghiPostFileMemoryStateDoPosting(state, connection);
}

// Sends the post of a connection, an item at a time. Returns 0 for an error, 1 once it is all sent, 2 to go on later and
// 3 once the head of an XML message is sent
int ghiPostDoPosting(GHIConnection *connection) {
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
            connection->postingState.isXml = FALSE;
        }
    }

    while (posting->index < count) {
        GHIPostItemState *state = func_ov011_0217158c(posting->states, posting->index);
        int index = posting->index;
        BOOL last = index == func_ov011_02171588(posting->states) - 1;
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
