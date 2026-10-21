#include "dwc/gpiBuffer.h"
#include "dwc/ext.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiUtility.h"
#include "dwc/gt2.h"

// GameSpy GP's growing texts and their sending and receiving (gpiBuffer.c). The functions' names, from
// pret/pokerevo's Wii build, are in dwc/gp.h

// The room that the Udp Engine takes for its own header in each message to a peer
#define GPI_PEER_HEADER_SIZE 23

static int gpiSendData(GPConnection *connection, int sock, char *buffer, int bufferLen, gsi_bool *closed, int *sent,
                       const char *id);

// How many sends and receives the debug texts have counted
static struct {
    int sent;
    int received;
} sCount;

int gpiAppendCharToBuffer(GPConnection *connection, GPIBuffer *outputBuffer, int c) {
    char *output = outputBuffer->buffer;
    int len = outputBuffer->len;
    int size = outputBuffer->size;

    if (size == len) {
        size += 0x4000;
        output = gsirealloc(output, size + 1);
        if (output == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
    }
    output[len] = c;
    output[len + 1] = '\0';
    outputBuffer->len++;
    outputBuffer->buffer = output;
    outputBuffer->size = size;
    return 0;
}

int gpiAppendStringToBufferLen(GPConnection *connection, GPIBuffer *outputBuffer, const char *string, int stringLen) {
    int len;
    int size;
    char *output;

    if (string == NULL) {
        return 0;
    }
    len = outputBuffer->len;
    size = outputBuffer->size;
    output = outputBuffer->buffer;
    if (size - len < stringLen) {
        size += stringLen >= 0x4000 ? stringLen : 0x4000;
        output = gsirealloc(output, size + 1);
        if (output == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
    }
    memcpy(output + len, string, stringLen);
    output[len + stringLen] = '\0';
    outputBuffer->size = size;
    outputBuffer->len += stringLen;
    outputBuffer->buffer = output;
    return 0;
}

int gpiAppendStringToBuffer(GPConnection *connection, GPIBuffer *outputBuffer, const char *buffer) {
    return gpiAppendStringToBufferLen(connection, outputBuffer, buffer, strlen(buffer));
}

int gpiAppendIntToBuffer(GPConnection *connection, GPIBuffer *outputBuffer, int num) {
    char intValue[16];

    OS_SPrintf(intValue, "%d", num);
    return gpiAppendStringToBuffer(connection, outputBuffer, intValue);
}

static int gpiSendData(GPConnection *connection, int sock, char *buffer, int bufferLen, gsi_bool *closed, int *sent,
                       const char *id) {
    int rcode = send(sock, buffer, bufferLen, 0);
    int error;
    char *copy;

    if (rcode == -1) {
        error = GOAGetLastError(sock);
        if (error != -6 && error != -26 && error != -76) {
            if (id[0] == 'P' && id[1] == 'R') {
                return 3;
            }
            gpiSetError(connection, 5, "There was an error sending on a socket.");
            gpiCallErrorCallback(connection, 3, 0);
            return 3;
        }
        *sent = 0;
        *closed = gsi_false;
    } else if (rcode == 0) {
        gsDebugFormat(1, 0, 16, "SENDXXXX(%s): Connection closed\n", id);
        *sent = 0;
        *closed = gsi_true;
    } else {
        copy = gsimalloc(rcode + 1);
        memcpy(copy, buffer, rcode);
        copy[rcode] = '\0';
        gsDebugFormat(1, 0, 32, "SENT%04d(%s): %s\n", sCount.sent++, id, copy);
        gsifree(copy);
        *sent = rcode;
        *closed = gsi_false;
    }
    return 0;
}

// The Udp Engine sends a peer's messages as they are given, so a single character is never sent on its own
int gpiSendOrBufferChar(GPConnection *connection, GPIPeer *peer, int c) {
    return 0;
}

int gpiSendOrBufferStringLen(GPConnection *connection, GPIPeer *peer, const char *string, int stringLen) {
    GPIConnection *iconnection = *connection;
    int sent = 0;
    gsi_u32 freeSpace;

    if (stringLen == 0) {
        return 0;
    }
    if (peer->outputBuffer.len - peer->outputBuffer.pos == 0 && func_ov011_02171588(peer->messages) == 0) {
        // Gt2_Close gives the free space of the connection's outgoing buffer
        if (stringLen <= Gt2_Close(peer->ip, peer->port) - GPI_PEER_HEADER_SIZE) {
            Gt2_Send(peer->ip, peer->port, iconnection->mHeader, string, stringLen, gsi_true);
            sent = stringLen;
            stringLen = 0;
        } else {
            freeSpace = Gt2_Close(peer->ip, peer->port);
            if (freeSpace > GPI_PEER_HEADER_SIZE) {
                sent = freeSpace - GPI_PEER_HEADER_SIZE;
                Gt2_Send(peer->ip, peer->port, iconnection->mHeader, string, sent, gsi_true);
                stringLen -= sent;
            }
        }
    }
    if (stringLen != 0) {
        int result = gpiAppendStringToBufferLen(connection, &peer->outputBuffer, string + sent, stringLen);

        if (result != 0) {
            return result;
        }
    }
    return 0;
}

int gpiSendOrBufferString(GPConnection *connection, GPIPeer *peer, char *string) {
    return gpiSendOrBufferStringLen(connection, peer, string, strlen(string));
}

int gpiRecvToBuffer(GPConnection *connection, int sock, GPIBuffer *inputBuffer, int *bytesRead, gsi_bool *connClosed,
                    const char *id) {
    char *buffer = inputBuffer->buffer;
    int len = inputBuffer->len;
    int size = inputBuffer->size;
    int total = 0;
    gsi_bool closed = gsi_false;
    int rcode;
    int error;
    char *copy;

    do {
        if (len + 0x4000 > size) {
            size = len + 0x4000;
            buffer = gsirealloc(buffer, size + 1);
            if (buffer == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            inputBuffer->buffer = buffer;
            inputBuffer->size = size;
        }
        rcode = recv(sock, buffer + len, size - len, 0);
        if (rcode == -1) {
            error = GOAGetLastError(sock);
            if (error != -6 && error != -26 && error != -76) {
                gpiSetErrorString(connection, "There was an error reading from a socket.");
                return 3;
            }
        } else if (rcode == 0) {
            closed = gsi_true;
            gsDebugFormat(1, 0, 16, "RECVXXXX(%s): Connection closed\n", id);
        } else {
            copy = gsimalloc(rcode + 1);
            memcpy(copy, buffer + len, rcode);
            copy[rcode] = '\0';
            gsDebugFormat(1, 0, 32, "RECV%04d(%s): %s\n", sCount.received++, id, copy);
            gsifree(copy);
            len += rcode;
            total += rcode;
        }
        buffer[len] = '\0';
    } while (rcode >= 0 && !closed && total < 0x20000);
    if (total != 0) {
        gsDebugFormat(1, 0, 32, "RECVTOTL(%s): %d\n", id, total);
    }
    inputBuffer->buffer = buffer;
    inputBuffer->len = len;
    inputBuffer->size = size;
    *bytesRead = total;
    *connClosed = closed;
    return 0;
}

int gpiSendFromBuffer(GPConnection *connection, int sock, GPIBuffer *outputBuffer, gsi_bool *connClosed,
                      gsi_bool clipSentData, const char *id) {
    char *buffer = outputBuffer->buffer;
    int len = outputBuffer->len;
    int pos = outputBuffer->pos;
    int total;
    int remaining = len - pos;
    gsi_bool closed;
    int sent;
    int rcode;

    total = 0;
    if (remaining == 0) {
        return 0;
    }
    do {
        rcode = gpiSendData(connection, sock, buffer + (pos + total), remaining, &closed, &sent, id);
        if (rcode != 0) {
            return rcode;
        }
        if (sent != 0) {
            total += sent;
            remaining -= sent;
        }
    } while (sent != 0 && remaining != 0);
    if (clipSentData) {
        if (total > 0) {
            memmove(buffer, buffer + total, remaining + 1);
            len -= total;
        }
    } else {
        pos += total;
    }
    outputBuffer->pos = pos;
    outputBuffer->len = len;
    if (connClosed != NULL) {
        *connClosed = closed;
    }
    return 0;
}

int gpiSendBufferToPeer(GPConnection *connection, gsi_u32 ip, gsi_u16 port, GPIBuffer *outputBuffer,
                        gsi_bool *connClosed, gsi_bool clipSentData) {
    int len;
    int remaining;
    char *buffer;
    int pos;
    int total;
    gsi_u32 freeSpace;
    int state;
    GPIConnection *iconnection = *connection;

    buffer = outputBuffer->buffer;
    len = outputBuffer->len;
    pos = outputBuffer->pos;
    total = 0;
    remaining = len - pos;
    if (remaining == 0) {
        return 0;
    }
    // Gt2_Close gives the free space of the connection's outgoing buffer
    if (remaining <= Gt2_Close(ip, port) - GPI_PEER_HEADER_SIZE) {
        Gt2_Send(ip, port, iconnection->mHeader, buffer + pos, remaining, gsi_true);
        total = remaining;
        remaining = 0;
    } else {
        do {
            freeSpace = Gt2_Close(ip, port) - GPI_PEER_HEADER_SIZE;
            if (freeSpace <= GPI_PEER_HEADER_SIZE) {
                break;
            }
            if (Gt2_Send(ip, port, iconnection->mHeader, buffer + (pos + total), freeSpace, gsi_true) == 8) {
                break;
            }
            total += freeSpace;
            remaining -= freeSpace;
        } while (remaining != 0);
    }
    if (clipSentData) {
        if (total != 0) {
            memmove(buffer, buffer + total, remaining + 1);
            len -= total;
        }
    } else {
        pos += total;
    }
    outputBuffer->len = len;
    outputBuffer->pos = pos;
    Gt2_GetConnectionState(ip, port, &state);
    if (state == 3) {
        *connClosed = gsi_true;
    } else {
        *connClosed = gsi_false;
    }
    return 0;
}

int gpiReadMessageFromBuffer(GPConnection *connection, GPIBuffer *inputBuffer, char **message, int *type, int *len) {
    char *str;
    char intValue[16];
    int messageLen;

    *message = NULL;
    if (inputBuffer->len < 5) {
        return 0;
    }
    str = strchr(inputBuffer->buffer, '\n');
    if (str != NULL) {
        if (strncmp(str - 5, "\\msg\\", 5) != 0) {
            return 3;
        }
        *str = '\0';
        if (!gpiValueForKey(inputBuffer->buffer, "\\m\\", intValue, 16)) {
            return 3;
        }
        *type = atol(intValue);
        if (!gpiValueForKey(inputBuffer->buffer, "\\len\\", intValue, 16)) {
            return 3;
        }
        messageLen = atol(intValue);
        if (inputBuffer->len > (str - inputBuffer->buffer) + (messageLen + 1)) {
            if (str[messageLen + 1] != '\0') {
                return 3;
            }
            *message = str + 1;
            *len = messageLen;
            inputBuffer->pos = (str - inputBuffer->buffer) + (messageLen + 1) + 1;
        } else {
            *str = '\n';
        }
    }
    return 0;
}

int gpiClipBufferToPosition(GPConnection *connection, GPIBuffer *buffer) {
    if (buffer == NULL || buffer->buffer == NULL || buffer->pos == 0) {
        return 0;
    }
    buffer->len -= buffer->pos;
    if (buffer->len != 0) {
        memmove(buffer->buffer, buffer->buffer + buffer->pos, buffer->len);
    }
    buffer->buffer[buffer->len] = '\0';
    buffer->pos = 0;
    return 0;
}
