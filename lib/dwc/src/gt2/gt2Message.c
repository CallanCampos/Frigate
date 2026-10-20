#include <string.h>
// Before dwc/ext.h: MWCC builds the packets' 2-byte memcpys in place, as the ROM does, only when
// <string.h> declares them first
#include "dwc/ext.h"
#include "dwc/gt2Auth.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"

// GameSpy GT2's messages (gt2Message.c). The functions' names, from pret/pokerevo's Wii build, are in
// dwc/gt2Message.h and beside the static ones

static int func_ov011_0217d3ac(const u8 *buffer, int index);
static void func_ov011_0217d3c4(u8 *buffer, int index, u16 value);
static s16 func_ov011_0217d3d0(int a, int b);
static BOOL func_ov011_0217d3d8(GTI2Connection *connection, int result, int reason);
static BOOL func_ov011_0217d434(GTI2Connection *connection);
static BOOL func_ov011_0217d440(GTI2Connection *connection);
static BOOL func_ov011_0217d45c(GTI2Connection *connection, int ack);
static BOOL func_ov011_0217d4f4(GTI2Connection *connection, const void *data, int length);
static BOOL func_ov011_0217d54c(GTI2Connection *connection, const void *data, int length);
static BOOL func_ov011_0217d5ac(GTI2Connection *connection, const u8 *data, int length);
static BOOL func_ov011_0217d61c(GTI2Connection *connection, const u8 *data, int length);
static BOOL func_ov011_0217d6b4(GTI2Connection *connection, const u8 *data, int length);
static BOOL func_ov011_0217d770(GTI2Connection *connection);
static BOOL func_ov011_0217d7a4(GTI2Connection *connection, const void *data, int length);
static BOOL func_ov011_0217d7ec(GTI2Connection *connection);
static BOOL func_ov011_0217d81c(GTI2Connection *connection, int type, const u8 *data, int length);
static int func_ov011_0217d8b4(const void *a, const void *b);
static BOOL func_ov011_0217d8c0(GTI2Connection *connection, int type, int sequence, const u8 *data, int length,
                                BOOL *dropped);
static void func_ov011_0217d9d0(GTI2Connection *connection, GTI2IncomingMessage *message, int index);
static BOOL func_ov011_0217da2c(GTI2Connection *connection);
static void func_ov011_0217da7c(GTI2Connection *connection);
static BOOL func_ov011_0217da9c(GTI2Connection *connection, int type, u8 *data, int length);
static BOOL func_ov011_0217dba8(GTI2Connection *connection, const u8 *data, int length);
static BOOL func_ov011_0217dbdc(GTI2Connection *connection, const u8 *data, int length);
static BOOL func_ov011_0217dc6c(GTI2Connection *connection, u8 *data, int length);
static BOOL func_ov011_0217dc74(GTI2Connection *connection, const u8 *data, int length);
static BOOL func_ov011_0217dcd0(GTI2Connection *connection);
static BOOL func_ov011_0217dcf8(GTI2Connection *connection, int type, u8 *data, int length);
static BOOL func_ov011_0217dd68(GTI2Socket *socket, u8 *data, int length, u32 ip, u16 port);
static BOOL func_ov011_0217e140(GTI2Connection *connection, u16 sequence, int length);
static BOOL func_ov011_0217e188(GTI2Connection *connection, int type, int size, BOOL *dropped);
static BOOL func_ov011_0217e244(GTI2Connection *connection);
static BOOL func_ov011_0217e27c(GTI2Connection *connection, const void *data, int length);
static BOOL func_ov011_0217e30c(GTI2Connection *connection, const u8 *response, const u8 *challenge);
static BOOL func_ov011_0217e374(GTI2Connection *connection, const u8 *response, const void *message, int length);
static BOOL func_ov011_0217e4e0(GTI2Connection *connection, const u8 *data, int length);
static BOOL func_ov011_0217e608(GTI2Connection *connection, u16 first, u16 last);
static BOOL func_ov011_0217e688(GTI2Connection *connection, u8 *data, int length);
static BOOL func_ov011_0217e6a4(GTI2Socket *socket, u32 ip, u16 port);

// Reads the big-endian u16 at an index
static int func_ov011_0217d3ac(const u8 *buffer, int index) {
    return (u16)((buffer[index] << 8) & 0xFF00) | buffer[index + 1];
}

// Writes a big-endian u16 at an index
static void func_ov011_0217d3c4(u8 *buffer, int index, u16 value) {
    buffer[index] = value >> 8;
    buffer[index + 1] = value;
}

// How far one serial number is past another; they wrap around
static s16 func_ov011_0217d3d0(int a, int b) {
    return a - b;
}

// Closes a connection after a message that ends it: a connecting one ends its attempt with the result, an open
// one closes with the reason
static BOOL func_ov011_0217d3d8(GTI2Connection *connection, int result, int reason) {
    int state = connection->state;

    if (state < 5) {
        if (connection->initiated) {
            func_ov011_0217d0b4(connection);
            if (!func_ov011_0217cab0(connection, result, NULL, 0)) {
                return FALSE;
            }
        } else {
            if (state == 4) {
                connection->freeAtAcceptReject = TRUE;
            }
            func_ov011_0217d0b4(connection);
        }
    } else if (state != 7) {
        func_ov011_0217d0b4(connection);
        if (!func_ov011_0217cb6c(connection, reason)) {
            return FALSE;
        }
    }
    return TRUE;
}

// Closes a connection after a message that breaks the protocol
static BOOL func_ov011_0217d434(GTI2Connection *connection) {
    return func_ov011_0217d3d8(connection, 7, 2);
}

// Closes a connection that ran out of memory, telling the peer
static BOOL func_ov011_0217d440(GTI2Connection *connection) {
    if (func_ov011_0217e694(connection)) {
        return func_ov011_0217d3d8(connection, 1, 4);
    }
    return FALSE;
}

// gti2HandleESN: drops the reliable messages that the peer's expected serial number acks
static BOOL func_ov011_0217d45c(GTI2Connection *connection, int ack) {
    int count = func_ov011_02171588(connection->outgoingMessages);
    int i;
    int base;

    if (count == 0) {
        return TRUE;
    }
    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *sent = func_ov011_0217158c(connection->outgoingMessages, i);

        if (func_ov011_0217d3d0(sent->serialNumber, ack) >= 0) {
            break;
        }
    }
    if (i == 0) {
        return TRUE;
    }
    while (i-- != 0) {
        func_ov011_02171668(connection->outgoingMessages, i);
    }
    count = func_ov011_02171588(connection->outgoingMessages);
    if (count == 0) {
        connection->outgoing.length = 0;
        return TRUE;
    }
    base = ((GTI2OutgoingMessage *)func_ov011_0217158c(connection->outgoingMessages, 0))->offset;
    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *sent = func_ov011_0217158c(connection->outgoingMessages, i);

        sent->offset -= base;
    }
    func_ov011_0217c9c8(&connection->outgoing, 0, base);
    return TRUE;
}

// Delivers the app's unreliable message, through the receive filters if there are any
static BOOL func_ov011_0217d4f4(GTI2Connection *connection, const void *data, int length) {
    if (connection->state != 5 && connection->state != 6) {
        return TRUE;
    }
    if (func_ov011_02171588(connection->receiveFilters) != 0) {
        return func_ov011_0217cc8c(connection, 0, data, length, FALSE) != 0;
    }
    return func_ov011_0217cb10(connection, data, length, FALSE) != 0;
}

// Delivers the app's reliable message
static BOOL func_ov011_0217d54c(GTI2Connection *connection, const void *data, int length) {
    if (connection->state != 5 && connection->state != 6) {
        if (!func_ov011_0217d434(connection)) {
            return FALSE;
        }
    } else {
        if (func_ov011_02171588(connection->receiveFilters) != 0) {
            return func_ov011_0217cc8c(connection, 0, data, length, TRUE) != 0;
        }
        if (!func_ov011_0217cb10(connection, data, length, TRUE)) {
            return FALSE;
        }
    }
    return TRUE;
}

// The client's challenge, on the side that listens: answers with the server's challenge
static BOOL func_ov011_0217d5ac(GTI2Connection *connection, const u8 *data, int length) {
    u8 response[32];
    u8 challenge[32];

    if (connection->state != 2) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (length < 32) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    gti2GetResponse(response, data);
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    if (!func_ov011_0217e30c(connection, response, challenge)) {
        return FALSE;
    }
    connection->state = 3;
    return TRUE;
}

// gti2HandleServerChallenge: checks the response to this side's challenge and answers the server's
static BOOL func_ov011_0217d61c(GTI2Connection *connection, const u8 *data, int length) {
    u8 response[32];

    if (connection->state != 0) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (length < 64) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (!gti2CheckResponse(data, connection->response)) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    gti2GetResponse(response, data + 32);
    if (!func_ov011_0217e374(connection, response, connection->initialMessage, connection->initialMessageLength)) {
        return FALSE;
    }
    if (connection->initialMessage != NULL) {
        gsifree(connection->initialMessage);
        connection->initialMessage = NULL;
    }
    connection->state = 1;
    return TRUE;
}

// gti2HandleClientResponse: checks the response and gives the app the attempt
static BOOL func_ov011_0217d6b4(GTI2Connection *connection, const u8 *data, int length) {
    if (connection->state != 3) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (length < 32) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (!gti2CheckResponse(data, connection->response)) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (connection->socket->onConnectAttempt == NULL) {
        if (!func_ov011_0217e694(connection)) {
            return FALSE;
        }
        func_ov011_0217d0b4(connection);
        return TRUE;
    }
    connection->state = 4;
    if (func_ov011_0217ca30(connection->socket, connection, connection->ip, connection->port,
                            current_time() - connection->challengeTime, data + 32, length - 32)) {
        return TRUE;
    }
    return FALSE;
}

// The server accepted the connection
static BOOL func_ov011_0217d770(GTI2Connection *connection) {
    if (connection->state != 1) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    connection->state = 5;
    return func_ov011_0217cab0(connection, 0, NULL, 0) != 0;
}

// The server rejected the connection, with its message
static BOOL func_ov011_0217d7a4(GTI2Connection *connection, const void *data, int length) {
    if (connection->state != 1) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    func_ov011_0217d0b4(connection);
    if (!func_ov011_0217e694(connection)) {
        return FALSE;
    }
    if (func_ov011_0217cab0(connection, 2, data, length)) {
        return TRUE;
    }
    return FALSE;
}

// The peer closed the connection
static BOOL func_ov011_0217d7ec(GTI2Connection *connection) {
    int reason;

    if (!func_ov011_0217e694(connection)) {
        return FALSE;
    }
    reason = 0;
    if (connection->state != 6) {
        reason = 1;
    }
    if (!func_ov011_0217d3d8(connection, 2, reason)) {
        return FALSE;
    }
    return TRUE;
}

// gti2DeliverReliableMessage, by its type
static BOOL func_ov011_0217d81c(GTI2Connection *connection, int type, const u8 *data, int length) {
    connection->expectedSerialNumber++;
    if (type == 0) {
        if (!func_ov011_0217d54c(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 1) {
        if (!func_ov011_0217d5ac(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 2) {
        if (!func_ov011_0217d61c(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 3) {
        if (!func_ov011_0217d6b4(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 4) {
        if (!func_ov011_0217d770(connection)) {
            return FALSE;
        }
    } else if (type == 5) {
        if (!func_ov011_0217d7a4(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 6) {
        if (!func_ov011_0217d7ec(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// gti2IncomingBufferMessageCompare: by serial number
static int func_ov011_0217d8b4(const void *a, const void *b) {
    const GTI2IncomingMessage *messageA = a;
    const GTI2IncomingMessage *messageB = b;

    return func_ov011_0217d3d0(messageA->serialNumber, messageB->serialNumber);
}

// gti2BufferIncomingMessage: keeps a reliable message that arrived ahead of its turn, and nacks the ones it
// skipped; dropped when there is no room
static BOOL func_ov011_0217d8c0(GTI2Connection *connection, int type, int sequence, const u8 *data, int length,
                                BOOL *dropped) {
    GTI2IncomingMessage message;
    int count = func_ov011_02171588(connection->incomingMessages);
    int i;

    for (i = 0; i < count; i++) {
        GTI2IncomingMessage *pending = func_ov011_0217158c(connection->incomingMessages, i);

        if (pending->serialNumber == sequence) {
            *dropped = FALSE;
            return TRUE;
        }
        if (func_ov011_0217d3d0(pending->serialNumber, sequence) > 0) {
            break;
        }
    }
    if (func_ov011_0217c960(&connection->incoming) < length) {
        *dropped = TRUE;
        return TRUE;
    }
    message.offset = connection->incoming.length;
    message.length = length;
    message.type = type;
    message.serialNumber = sequence;
    ArrayInsertSorted(connection->incomingMessages, &message, func_ov011_0217d8b4);
    if (count + 1 != func_ov011_02171588(connection->incomingMessages)) {
        *dropped = TRUE;
        return TRUE;
    }
    func_ov011_0217c990(&connection->incoming, data, length);
    if (count == 0) {
        if (!func_ov011_0217e608(connection, connection->expectedSerialNumber, sequence - 1)) {
            return FALSE;
        }
    } else {
        GTI2IncomingMessage *inserted = func_ov011_0217158c(connection->incomingMessages, count);

        if (inserted->serialNumber == sequence) {
            GTI2IncomingMessage *previous = func_ov011_0217158c(connection->incomingMessages, count - 1);

            if ((u16)func_ov011_0217d3d0(sequence, previous->serialNumber) > 1) {
                if (!func_ov011_0217e608(connection, previous->serialNumber + 1, sequence - 1)) {
                    return FALSE;
                }
            }
        }
    }
    *dropped = FALSE;
    return TRUE;
}

// Removes a buffered incoming message from the buffer
static void func_ov011_0217d9d0(GTI2Connection *connection, GTI2IncomingMessage *message, int index) {
    int offset = message->offset;
    int length = message->length;
    int count;
    int i;
    int end = 0;

    func_ov011_02171668(connection->incomingMessages, index);
    count = func_ov011_02171588(connection->incomingMessages);
    for (i = 0; i < count; i++) {
        GTI2IncomingMessage *pending = func_ov011_0217158c(connection->incomingMessages, i);

        if (pending->offset > offset) {
            pending->offset -= length;
            if (end <= pending->offset + pending->length) {
                end = pending->offset + pending->length;
            }
        }
    }
    func_ov011_0217c9c8(&connection->incoming, offset, length);
}

// Delivers the buffered messages whose turn has come
static BOOL func_ov011_0217da2c(GTI2Connection *connection) {
    int i;

restart:
    for (i = func_ov011_02171588(connection->incomingMessages) - 1; i >= 0; i--) {
        GTI2IncomingMessage *pending = func_ov011_0217158c(connection->incomingMessages, i);

        if (pending->serialNumber == connection->expectedSerialNumber) {
            if (!func_ov011_0217d81c(connection, pending->type, connection->incoming.data + pending->offset,
                                     pending->length)) {
                return FALSE;
            }
            func_ov011_0217d9d0(connection, pending, i);
            goto restart;
        }
    }
    return TRUE;
}

// Starts the wait before an ack
static void func_ov011_0217da7c(GTI2Connection *connection) {
    if (!connection->pendingAck) {
        connection->pendingAck = TRUE;
        connection->pendingAckTime = current_time();
    }
}

// gti2HandleReliableMessage: delivers it in its turn, keeps it for later or drops a repeat
static BOOL func_ov011_0217da9c(GTI2Connection *connection, int type, u8 *data, int length) {
    int headerSize = connection->socket->protocolOffset + 7;
    int sequence;
    int ack;
    BOOL dropped;

    if (length < headerSize) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    sequence = func_ov011_0217d3ac(data + connection->socket->protocolOffset, 3);
    ack = func_ov011_0217d3ac(data + connection->socket->protocolOffset, 5);
    if (connection->socket->protocol == 2 && type == 0) {
        data[connection->socket->protocolOffset + 5] = data[0];
        data[connection->socket->protocolOffset + 6] = data[1];
        data += headerSize - connection->socket->protocolOffset;
        length -= headerSize - connection->socket->protocolOffset;
    } else {
        data += headerSize;
        length -= headerSize;
    }
    if (!func_ov011_0217d45c(connection, ack)) {
        return FALSE;
    }
    if (sequence == connection->expectedSerialNumber) {
        func_ov011_0217da7c(connection);
        if (!func_ov011_0217d81c(connection, type, data, length)) {
            return FALSE;
        }
        if (func_ov011_0217da2c(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (func_ov011_0217d3d0(sequence, connection->expectedSerialNumber) < 0) {
        func_ov011_0217da7c(connection);
        return TRUE;
    }
    if (!func_ov011_0217d8c0(connection, type, sequence, data, length, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        if (!func_ov011_0217d440(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// An ack
static BOOL func_ov011_0217dba8(GTI2Connection *connection, const u8 *data, int length) {
    if (length != 2) {
        if (func_ov011_0217d434(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    return func_ov011_0217d45c(connection, func_ov011_0217d3ac(data, 0)) != 0;
}

// gti2HandleNack: resends the messages from the first serial number to the last
static BOOL func_ov011_0217dbdc(GTI2Connection *connection, const u8 *data, int length) {
    int first = func_ov011_0217d3ac(data, 0);
    int last;
    int count;
    int i;

    if (length == 2) {
        last = first;
    } else if (length == 4) {
        last = func_ov011_0217d3ac(data, 2);
    } else {
        return func_ov011_0217d434(connection) != 0;
    }
    count = func_ov011_02171588(connection->outgoingMessages);
    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *message = func_ov011_0217158c(connection->outgoingMessages, i);

        if (func_ov011_0217d3d0(message->serialNumber, first) >= 0 &&
            func_ov011_0217d3d0(message->serialNumber, last) <= 0) {
            if (!func_ov011_0217e6f8(connection, message)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// A ping, which is answered with a pong
static BOOL func_ov011_0217dc6c(GTI2Connection *connection, u8 *data, int length) {
    return func_ov011_0217e688(connection, data, length);
}

// A pong, with the time its ping was sent
static BOOL func_ov011_0217dc74(GTI2Connection *connection, const u8 *data, int length) {
    u32 sendTime;
    const u8 *src;
    u8 *dest;

    if (connection->callbacks.onPing == NULL) {
        return TRUE;
    }
    if (length != 8) {
        return TRUE;
    }
    if (memcmp(data, "time", 4) != 0) {
        return TRUE;
    }
    src = data + 4;
    dest = (u8 *)&sendTime;
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
    if (func_ov011_0217cbbc(connection, current_time() - sendTime)) {
        return TRUE;
    }
    return FALSE;
}

// The peer's note that the connection is closed
static BOOL func_ov011_0217dcd0(GTI2Connection *connection) {
    int reason;

    if (connection->state == 7) {
        return TRUE;
    }
    reason = 0;
    if (connection->state != 6) {
        reason = 1;
    }
    if (!func_ov011_0217d3d8(connection, 2, reason)) {
        return FALSE;
    }
    return TRUE;
}

// gti2HandleUnreliableMessage, by its type
static BOOL func_ov011_0217dcf8(GTI2Connection *connection, int type, u8 *data, int length) {
    int offset = connection->socket->protocolOffset + 3;
    int rest = length - offset;

    if (type == 100) {
        if (!func_ov011_0217dba8(connection, data + offset, rest)) {
            return FALSE;
        }
    } else if (type == 101) {
        if (!func_ov011_0217dbdc(connection, data + offset, rest)) {
            return FALSE;
        }
    } else if (type == 102) {
        if (!func_ov011_0217dc6c(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 103) {
        if (!func_ov011_0217dc74(connection, data + offset, rest)) {
            return FALSE;
        }
    } else if (type == 104) {
        if (!func_ov011_0217dcd0(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// gti2HandleMessage: a datagram from an address, GT2's or the app's
static BOOL func_ov011_0217dd68(GTI2Socket *socket, u8 *data, int length, u32 ip, u16 port) {
    int payloadLength = length - socket->protocolOffset;
    u8 *payload = data + socket->protocolOffset;
    GTI2Connection *connection = func_ov011_0217e7a4(socket, ip, port);
    BOOL handled;
    BOOL hasMagic;
    int result;
    int type;

    if (socket->onReceiveDump != NULL) {
        if (!func_ov011_0217cd0c(socket, connection, ip, port, FALSE, data, length, FALSE)) {
            return FALSE;
        }
    }
    if (payloadLength > 2 && memcmp(payload, "\xfe\xfe", 2) == 0) {
        hasMagic = TRUE;
    } else {
        hasMagic = FALSE;
    }
    if (connection == NULL) {
        if (!func_ov011_0217cd98(socket, ip, port, data, length, &handled)) {
            return FALSE;
        }
        if (handled) {
            return TRUE;
        }
        // Only the client's challenge opens a connection; anything else but the note of a closed one is answered
        // with that note
        if (!hasMagic || payload[2] != 1) {
            if (!hasMagic || payload[2] != 0x68) {
                if (!func_ov011_0217e6a4(socket, ip, port)) {
                    return FALSE;
                }
            }
            return TRUE;
        }
        if (socket->onConnectAttempt == NULL) {
            return TRUE;
        }
        result = func_ov011_0217ce1c(socket, &connection, ip, port);
        if (result != 0) {
            if (result != 5) {
                if (!func_ov011_0217e6a4(socket, ip, port)) {
                    return FALSE;
                }
            }
            return TRUE;
        }
    }
    if (connection->state == 7) {
        if (!hasMagic || payload[2] != 0x68) {
            if (!func_ov011_0217e694(connection)) {
                return FALSE;
            }
        }
        return TRUE;
    }
    if (hasMagic && payloadLength >= 4 && memcmp(payload + 2, "\xfe\xfe", 2) == 0) {
        payload += 2;
        length -= 2;
        data[3] = data[1];
        data[2] = data[0];
        hasMagic = FALSE;
        data += 2;
    }
    if (!hasMagic) {
        if (connection->state < 5) {
            if (!func_ov011_0217cd98(socket, ip, port, data, length, &handled)) {
                return FALSE;
            }
        } else {
            if (!func_ov011_0217d4f4(connection, data, length)) {
                return FALSE;
            }
        }
        return TRUE;
    }
    type = payload[2];
    if (type < 8) {
        if (func_ov011_0217da9c(connection, type, data, length)) {
            return TRUE;
        }
        return FALSE;
    }
    if (func_ov011_0217dcf8(connection, type, data, length)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov011_0217df34(GTI2Socket *socket, u32 ip, u16 port) {
    GTI2Connection *connection = func_ov011_0217e7a4(socket, ip, port);

    if (socket->onReceiveDump != NULL) {
        if (!func_ov011_0217cd0c(socket, connection, ip, port, TRUE, NULL, 0, FALSE)) {
            return FALSE;
        }
    }
    if (connection == NULL) {
        return TRUE;
    }
    if (connection->state == 0) {
        if (connection->timeout == 0 || current_time() - connection->startTime < connection->timeout) {
            return TRUE;
        }
        if (!func_ov011_0217d3d8(connection, 6, 1)) {
            return FALSE;
        }
    } else {
        if (!func_ov011_0217d3d8(connection, 2, 1)) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL func_ov011_0217dfc4(GTI2Socket *socket, u32 ip, u16 port, BOOL send) {
    GTI2Connection *connection = func_ov011_0217e7a4(socket, ip, port);

    if (socket->onReceiveDump != NULL) {
        if (!func_ov011_0217cd0c(socket, connection, ip, port, TRUE, NULL, 0, send)) {
            return FALSE;
        }
    }
    if (connection == NULL) {
        return TRUE;
    }
    return func_ov011_0217d3d8(connection, 6, 1) != 0;
}

BOOL func_ov011_0217e020(GTI2Socket *socket) {
    u8 buffer[1500];
    SocAddrIn address;
    int addressSize;
    int length;
    int error;

    if (CanReceiveOnSocket(socket->handle) != 0) {
        do {
            addressSize = sizeof(address);
            length = recvfrom(socket->handle, buffer, sizeof(buffer), 0, &address, &addressSize);
            if (length == -1) {
                error = GOAGetLastError(socket->handle);
                if (error == -15) {
                    if (!func_ov011_0217df34(socket, address.addr, GTI2_SWAP16(address.port))) {
                        return FALSE;
                    }
                } else if (error == -23) {
                    if (!func_ov011_0217dfc4(socket, address.addr, GTI2_SWAP16(address.port), FALSE)) {
                        return FALSE;
                    }
                } else if (error != -35) {
                    func_ov011_0217ecf0(socket);
                    return FALSE;
                }
            } else {
                if (!func_ov011_0217dd68(socket, buffer, length, address.addr, GTI2_SWAP16(address.port))) {
                    return FALSE;
                }
            }
        } while (CanReceiveOnSocket(socket->handle));
    }
    return TRUE;
}

// Keeps a reliable message that was sent, for its ack
static BOOL func_ov011_0217e140(GTI2Connection *connection, u16 sequence, int length) {
    GTI2OutgoingMessage message = { 0, 0, 0, 0 };
    int count;

    message.offset = connection->outgoing.length;
    message.length = length;
    message.serialNumber = sequence;
    message.sendTime = current_time();
    count = func_ov011_02171588(connection->outgoingMessages);
    func_ov011_021715a4(connection->outgoingMessages, &message);
    return count + 1 == func_ov011_02171588(connection->outgoingMessages);
}

// gti2BeginReliableMessage: the header of a reliable message in the outgoing buffer, dropped when there is no room
static BOOL func_ov011_0217e188(GTI2Connection *connection, int type, int size, BOOL *dropped) {
    u16 payloadLength = size - connection->socket->protocolOffset;

    if (func_ov011_0217c960(&connection->outgoing) < size) {
        if (!func_ov011_0217d440(connection)) {
            return FALSE;
        }
        *dropped = TRUE;
        return TRUE;
    }
    if (!func_ov011_0217e140(connection, connection->serialNumber, size)) {
        if (!func_ov011_0217d440(connection)) {
            return FALSE;
        }
        *dropped = TRUE;
        return TRUE;
    }
    if (connection->socket->protocol == 2) {
        func_ov011_0217c990(&connection->outgoing, &payloadLength, connection->socket->protocolOffset);
    }
    func_ov011_0217c990(&connection->outgoing, "\xfe\xfe", 2);
    func_ov011_0217c968(&connection->outgoing, type);
    func_ov011_0217c974(&connection->outgoing, connection->serialNumber++);
    func_ov011_0217c974(&connection->outgoing, connection->expectedSerialNumber);
    *dropped = FALSE;
    return TRUE;
}

// Sends the reliable message just built
static BOOL func_ov011_0217e244(GTI2Connection *connection) {
    GTI2OutgoingMessage *last =
        func_ov011_0217158c(connection->outgoingMessages, func_ov011_02171588(connection->outgoingMessages) - 1);

    if (!func_ov011_0217cf18(connection, connection->outgoing.data + last->offset, last->length)) {
        return FALSE;
    }
    connection->pendingAck = FALSE;
    return TRUE;
}

// Sends the app's reliable message
static BOOL func_ov011_0217e27c(GTI2Connection *connection, const void *data, int length) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 0, length + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    func_ov011_0217c990(&connection->outgoing, data, length);
    return func_ov011_0217e244(connection) != 0;
}

BOOL func_ov011_0217e2c0(GTI2Connection *connection, const u8 *challenge) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 1, connection->socket->protocolOffset + 0x27, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    func_ov011_0217c990(&connection->outgoing, challenge, 32);
    return func_ov011_0217e244(connection) != 0;
}

// The server's challenge, with the response to the client's
static BOOL func_ov011_0217e30c(GTI2Connection *connection, const u8 *response, const u8 *challenge) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 2, connection->socket->protocolOffset + 0x47, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    func_ov011_0217c990(&connection->outgoing, response, 32);
    func_ov011_0217c990(&connection->outgoing, challenge, 32);
    if (!func_ov011_0217e244(connection)) {
        return FALSE;
    }
    connection->challengeTime = connection->lastSendTime;
    return TRUE;
}

// The client's response, with the connect attempt's message
static BOOL func_ov011_0217e374(GTI2Connection *connection, const u8 *response, const void *message, int length) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 3, length + 0x27 + connection->socket->protocolOffset, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    func_ov011_0217c990(&connection->outgoing, response, 32);
    func_ov011_0217c990(&connection->outgoing, message, length);
    if (func_ov011_0217e244(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov011_0217e3cc(GTI2Connection *connection) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 4, connection->socket->protocolOffset + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    if (func_ov011_0217e244(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov011_0217e40c(GTI2Connection *connection, const void *message, int length) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 5, length + 7 + connection->socket->protocolOffset, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    func_ov011_0217c990(&connection->outgoing, message, length);
    if (func_ov011_0217e244(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov011_0217e460(GTI2Connection *connection) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 6, connection->socket->protocolOffset + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    if (func_ov011_0217e244(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov011_0217e4a0(GTI2Connection *connection) {
    BOOL dropped;

    if (!func_ov011_0217e188(connection, 7, connection->socket->protocolOffset + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    if (func_ov011_0217e244(connection)) {
        return TRUE;
    }
    return FALSE;
}

// gti2SendAppUnreliable: a message that starts with 0xfe 0xfe gets them twice, so it isn't read as GT2's
static BOOL func_ov011_0217e4e0(GTI2Connection *connection, const u8 *data, int length) {
    int base;
    u8 *start;
    int total;

    if (length < 2 || memcmp(data + connection->socket->protocolOffset, "\xfe\xfe", 2) != 0) {
        if (!func_ov011_0217cf18(connection, data, length)) {
            return FALSE;
        }
        return TRUE;
    }
    total = length + 2;
    if (func_ov011_0217c960(&connection->outgoing) < total) {
        return TRUE;
    }
    start = connection->outgoing.data;
    base = connection->outgoing.length;
    if (connection->socket->protocol == 2) {
        func_ov011_0217c990(&connection->outgoing, data, 2);
    }
    func_ov011_0217c990(&connection->outgoing, "\xfe\xfe", 2);
    func_ov011_0217c990(&connection->outgoing, data + connection->socket->protocolOffset,
                        length - connection->socket->protocolOffset);
    if (!func_ov011_0217cf18(connection, start + base, total)) {
        return FALSE;
    }
    func_ov011_0217c9c8(&connection->outgoing, -1, total);
    return TRUE;
}

BOOL func_ov011_0217e598(GTI2Connection *connection) {
    u16 prefix;
    u8 packet[10];
    int length = 0;

    if (connection->socket->protocol == 2) {
        prefix = 5;
        memcpy(packet, &prefix, 2);
        length += 2;
    }
    memcpy(packet + length, "\xfe\xfe", 2);
    packet[length + 2] = 0x64;
    func_ov011_0217d3c4(packet, length + 3, connection->expectedSerialNumber);
    if (!func_ov011_0217cf18(connection, packet, length + 5)) {
        return FALSE;
    }
    connection->pendingAck = FALSE;
    return TRUE;
}

// gti2SendNack, of the serial numbers from first to last
static BOOL func_ov011_0217e608(GTI2Connection *connection, u16 first, u16 last) {
    u16 prefix;
    u8 packet[10];
    int length = 0;

    if (connection->socket->protocol == 2) {
        prefix = 7;
        memcpy(packet, &prefix, 2);
        length += 2;
    }
    memcpy(packet + length, "\xfe\xfe", 2);
    packet[length + 2] = 0x65;
    func_ov011_0217d3c4(packet, length + 3, first);
    length += 5;
    if (first != last) {
        func_ov011_0217d3c4(packet, length, last);
        length += 2;
    }
    if (!func_ov011_0217cf18(connection, packet, length)) {
        return FALSE;
    }
    return TRUE;
}

// Answers a ping with a pong
static BOOL func_ov011_0217e688(GTI2Connection *connection, u8 *data, int length) {
    data[2] = 0x67;
    return func_ov011_0217cf18(connection, data, length);
}

BOOL func_ov011_0217e694(GTI2Connection *connection) {
    return func_ov011_0217e6a4(connection->socket, connection->ip, connection->port);
}

// Sends an address the note that its connection is closed
static BOOL func_ov011_0217e6a4(GTI2Socket *socket, u32 ip, u16 port) {
    u16 prefix;
    u8 packet[5];
    int length = 0;

    if (socket->protocol == 2) {
        prefix = 3;
        memcpy(packet, &prefix, 2);
        length += 2;
    }
    memcpy(packet + length, "\xfe\xfe", 2);
    packet[length + 2] = 0x68;
    if (!func_ov011_0217eb34(socket, ip, port, packet, length + 3)) {
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_0217e6f8(GTI2Connection *connection, GTI2OutgoingMessage *message) {
    func_ov011_0217d3c4(connection->outgoing.data, message->offset + connection->socket->protocolOffset + 5,
                        connection->expectedSerialNumber);
    if (!func_ov011_0217cf18(connection, connection->outgoing.data + message->offset, message->length)) {
        return FALSE;
    }
    message->sendTime = connection->lastSendTime;
    if (connection->outgoing.data[message->offset + connection->socket->protocolOffset + 2] == 2) {
        connection->challengeTime = connection->lastSendTime;
    }
    return TRUE;
}

BOOL func_ov011_0217e754(GTI2Connection *connection, const void *data, int length, BOOL reliable) {
    if (reliable) {
        return func_ov011_0217e27c(connection, data, length);
    }
    return func_ov011_0217e4e0(connection, data, length);
}
