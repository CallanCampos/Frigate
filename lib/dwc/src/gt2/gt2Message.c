#include <string.h>
// Before dwc/ext.h: MWCC builds the packets' 2-byte memcpys in place, as the ROM does, only when
// <string.h> declares them first
#include "dwc/ext.h"
#include "dwc/gt2Auth.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"

// GameSpy GT2's messages (gt2Message.c)

static int gti2UShortFromBuffer(const u8 *buffer, int index);
static void gti2UShortToBuffer(u8 *buffer, int index, u16 value);
static s16 gti2SNDiff(int a, int b);
static BOOL gti2ConnectionError(GTI2Connection *connection, int result, int reason);
static BOOL gti2ConnectionCommunicationError(GTI2Connection *connection);
static BOOL gti2ConnectionMemoryError(GTI2Connection *connection);
static BOOL gti2HandleESN(GTI2Connection *connection, int ack);
static BOOL gti2HandleAppUnreliable(GTI2Connection *connection, const void *data, int length);
static BOOL gti2HandleAppReliable(GTI2Connection *connection, const void *data, int length);
static BOOL gti2HandleClientChallenge(GTI2Connection *connection, const u8 *data, int length);
static BOOL gti2HandleServerChallenge(GTI2Connection *connection, const u8 *data, int length);
static BOOL gti2HandleClientResponse(GTI2Connection *connection, const u8 *data, int length);
static BOOL gti2HandleAccept(GTI2Connection *connection);
static BOOL gti2HandleReject(GTI2Connection *connection, const void *data, int length);
static BOOL gti2HandleClose(GTI2Connection *connection);
static BOOL gti2DeliverReliableMessage(GTI2Connection *connection, int type, const u8 *data, int length);
static int gti2IncomingBufferMessageCompare(const void *a, const void *b);
static BOOL gti2BufferIncomingMessage(GTI2Connection *connection, int type, int sequence, const u8 *data, int length,
                                      BOOL *dropped);
static void gti2RemoveHoldMessage(GTI2Connection *connection, GTI2IncomingMessage *message, int index);
static BOOL gti2DeliverHoldMessages(GTI2Connection *connection);
static void gti2SetPendingAck(GTI2Connection *connection);
static BOOL gti2HandleReliableMessage(GTI2Connection *connection, int type, u8 *data, int length);
static BOOL gti2HandleAck(GTI2Connection *connection, const u8 *data, int length);
static BOOL gti2HandleNack(GTI2Connection *connection, const u8 *data, int length);
static BOOL gti2HandlePing(GTI2Connection *connection, u8 *data, int length);
static BOOL gti2HandlePong(GTI2Connection *connection, const u8 *data, int length);
static BOOL gti2HandleClosed(GTI2Connection *connection);
static BOOL gti2HandleUnreliableMessage(GTI2Connection *connection, int type, u8 *data, int length);
static BOOL gti2HandleMessage(GTI2Socket *socket, u8 *data, int length, u32 ip, u16 port);
static BOOL gti2StoreOutgoingReliableMessageInfo(GTI2Connection *connection, u16 sequence, int length);
static BOOL gti2BeginReliableMessage(GTI2Connection *connection, int type, int size, BOOL *dropped);
static BOOL gti2EndReliableMessage(GTI2Connection *connection);
static BOOL gti2SendAppReliable(GTI2Connection *connection, const void *data, int length);
static BOOL gti2SendServerChallenge(GTI2Connection *connection, const u8 *response, const u8 *challenge);
static BOOL gti2SendClientResponse(GTI2Connection *connection, const u8 *response, const void *message, int length);
static BOOL gti2SendAppUnreliable(GTI2Connection *connection, const u8 *data, int length);
static BOOL gti2SendNack(GTI2Connection *connection, u16 first, u16 last);
static BOOL gti2SendPong(GTI2Connection *connection, u8 *data, int length);
static BOOL gti2SendClosedOnSocket(GTI2Socket *socket, u32 ip, u16 port);

// Reads the big-endian u16 at an index
static int gti2UShortFromBuffer(const u8 *buffer, int index) {
    return (u16)((buffer[index] << 8) & 0xFF00) | buffer[index + 1];
}

// Writes a big-endian u16 at an index
static void gti2UShortToBuffer(u8 *buffer, int index, u16 value) {
    buffer[index] = value >> 8;
    buffer[index + 1] = value;
}

// How far one serial number is past another; they wrap around
static s16 gti2SNDiff(int a, int b) {
    return a - b;
}

// Closes a connection after a message that ends it: a connecting one ends its attempt with the result, an open
// one closes with the reason
static BOOL gti2ConnectionError(GTI2Connection *connection, int result, int reason) {
    int state = connection->state;

    if (state < 5) {
        if (connection->initiated) {
            gti2ConnectionClosed(connection);
            if (!gti2ConnectedCallback(connection, result, NULL, 0)) {
                return FALSE;
            }
        } else {
            if (state == 4) {
                connection->freeAtAcceptReject = TRUE;
            }
            gti2ConnectionClosed(connection);
        }
    } else if (state != 7) {
        gti2ConnectionClosed(connection);
        if (!gti2ClosedCallback(connection, reason)) {
            return FALSE;
        }
    }
    return TRUE;
}

// Closes a connection after a message that breaks the protocol
static BOOL gti2ConnectionCommunicationError(GTI2Connection *connection) {
    return gti2ConnectionError(connection, 7, 2);
}

// Closes a connection that ran out of memory, telling the peer
static BOOL gti2ConnectionMemoryError(GTI2Connection *connection) {
    if (gti2SendClosed(connection)) {
        return gti2ConnectionError(connection, 1, 4);
    }
    return FALSE;
}

// Drops the reliable messages that the peer's expected serial number acks
static BOOL gti2HandleESN(GTI2Connection *connection, int ack) {
    int count = func_ov011_02171588(connection->outgoingMessages);
    int i;
    int base;

    if (count == 0) {
        return TRUE;
    }
    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *sent = func_ov011_0217158c(connection->outgoingMessages, i);

        if (gti2SNDiff(sent->serialNumber, ack) >= 0) {
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
    gti2BufferShorten(&connection->outgoing, 0, base);
    return TRUE;
}

// Delivers the app's unreliable message, through the receive filters if there are any
static BOOL gti2HandleAppUnreliable(GTI2Connection *connection, const void *data, int length) {
    if (connection->state != 5 && connection->state != 6) {
        return TRUE;
    }
    if (func_ov011_02171588(connection->receiveFilters) != 0) {
        return gti2ReceiveFilterCallback(connection, 0, data, length, FALSE) != 0;
    }
    return gti2ReceivedCallback(connection, data, length, FALSE) != 0;
}

// Delivers the app's reliable message
static BOOL gti2HandleAppReliable(GTI2Connection *connection, const void *data, int length) {
    if (connection->state != 5 && connection->state != 6) {
        if (!gti2ConnectionCommunicationError(connection)) {
            return FALSE;
        }
    } else {
        if (func_ov011_02171588(connection->receiveFilters) != 0) {
            return gti2ReceiveFilterCallback(connection, 0, data, length, TRUE) != 0;
        }
        if (!gti2ReceivedCallback(connection, data, length, TRUE)) {
            return FALSE;
        }
    }
    return TRUE;
}

// The client's challenge, on the side that listens: answers with the server's challenge
static BOOL gti2HandleClientChallenge(GTI2Connection *connection, const u8 *data, int length) {
    u8 response[32];
    u8 challenge[32];

    if (connection->state != 2) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (length < 32) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    gti2GetResponse(response, data);
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    if (!gti2SendServerChallenge(connection, response, challenge)) {
        return FALSE;
    }
    connection->state = 3;
    return TRUE;
}

// Checks the response to this side's challenge and answers the server's
static BOOL gti2HandleServerChallenge(GTI2Connection *connection, const u8 *data, int length) {
    u8 response[32];

    if (connection->state != 0) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (length < 64) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (!gti2CheckResponse(data, connection->response)) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    gti2GetResponse(response, data + 32);
    if (!gti2SendClientResponse(connection, response, connection->initialMessage, connection->initialMessageLength)) {
        return FALSE;
    }
    if (connection->initialMessage != NULL) {
        gsifree(connection->initialMessage);
        connection->initialMessage = NULL;
    }
    connection->state = 1;
    return TRUE;
}

// Checks the response and gives the app the attempt
static BOOL gti2HandleClientResponse(GTI2Connection *connection, const u8 *data, int length) {
    if (connection->state != 3) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (length < 32) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (!gti2CheckResponse(data, connection->response)) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (connection->socket->onConnectAttempt == NULL) {
        if (!gti2SendClosed(connection)) {
            return FALSE;
        }
        gti2ConnectionClosed(connection);
        return TRUE;
    }
    connection->state = 4;
    if (gti2ConnectAttemptCallback(connection->socket, connection, connection->ip, connection->port,
                                   current_time() - connection->challengeTime, data + 32, length - 32)) {
        return TRUE;
    }
    return FALSE;
}

// The server accepted the connection
static BOOL gti2HandleAccept(GTI2Connection *connection) {
    if (connection->state != 1) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    connection->state = 5;
    return gti2ConnectedCallback(connection, 0, NULL, 0) != 0;
}

// The server rejected the connection, with its message
static BOOL gti2HandleReject(GTI2Connection *connection, const void *data, int length) {
    if (connection->state != 1) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    gti2ConnectionClosed(connection);
    if (!gti2SendClosed(connection)) {
        return FALSE;
    }
    if (gti2ConnectedCallback(connection, 2, data, length)) {
        return TRUE;
    }
    return FALSE;
}

// The peer closed the connection
static BOOL gti2HandleClose(GTI2Connection *connection) {
    int reason;

    if (!gti2SendClosed(connection)) {
        return FALSE;
    }
    reason = 0;
    if (connection->state != 6) {
        reason = 1;
    }
    if (!gti2ConnectionError(connection, 2, reason)) {
        return FALSE;
    }
    return TRUE;
}

// Delivers a reliable message, by its type
static BOOL gti2DeliverReliableMessage(GTI2Connection *connection, int type, const u8 *data, int length) {
    connection->expectedSerialNumber++;
    if (type == 0) {
        if (!gti2HandleAppReliable(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 1) {
        if (!gti2HandleClientChallenge(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 2) {
        if (!gti2HandleServerChallenge(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 3) {
        if (!gti2HandleClientResponse(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 4) {
        if (!gti2HandleAccept(connection)) {
            return FALSE;
        }
    } else if (type == 5) {
        if (!gti2HandleReject(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 6) {
        if (!gti2HandleClose(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// Compares two held messages by serial number
static int gti2IncomingBufferMessageCompare(const void *a, const void *b) {
    const GTI2IncomingMessage *messageA = a;
    const GTI2IncomingMessage *messageB = b;

    return gti2SNDiff(messageA->serialNumber, messageB->serialNumber);
}

// Keeps a reliable message that arrived ahead of its turn, and nacks the ones it
// skipped; dropped when there is no room
static BOOL gti2BufferIncomingMessage(GTI2Connection *connection, int type, int sequence, const u8 *data, int length,
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
        if (gti2SNDiff(pending->serialNumber, sequence) > 0) {
            break;
        }
    }
    if (gti2GetBufferFreeSpace(&connection->incoming) < length) {
        *dropped = TRUE;
        return TRUE;
    }
    message.offset = connection->incoming.length;
    message.length = length;
    message.type = type;
    message.serialNumber = sequence;
    ArrayInsertSorted(connection->incomingMessages, &message, gti2IncomingBufferMessageCompare);
    if (count + 1 != func_ov011_02171588(connection->incomingMessages)) {
        *dropped = TRUE;
        return TRUE;
    }
    gti2BufferWriteData(&connection->incoming, data, length);
    if (count == 0) {
        if (!gti2SendNack(connection, connection->expectedSerialNumber, sequence - 1)) {
            return FALSE;
        }
    } else {
        GTI2IncomingMessage *inserted = func_ov011_0217158c(connection->incomingMessages, count);

        if (inserted->serialNumber == sequence) {
            GTI2IncomingMessage *previous = func_ov011_0217158c(connection->incomingMessages, count - 1);

            if ((u16)gti2SNDiff(sequence, previous->serialNumber) > 1) {
                if (!gti2SendNack(connection, previous->serialNumber + 1, sequence - 1)) {
                    return FALSE;
                }
            }
        }
    }
    *dropped = FALSE;
    return TRUE;
}

// Removes a buffered incoming message from the buffer
static void gti2RemoveHoldMessage(GTI2Connection *connection, GTI2IncomingMessage *message, int index) {
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
    gti2BufferShorten(&connection->incoming, offset, length);
}

// Delivers the buffered messages whose turn has come
static BOOL gti2DeliverHoldMessages(GTI2Connection *connection) {
    int i;

restart:
    for (i = func_ov011_02171588(connection->incomingMessages) - 1; i >= 0; i--) {
        GTI2IncomingMessage *pending = func_ov011_0217158c(connection->incomingMessages, i);

        if (pending->serialNumber == connection->expectedSerialNumber) {
            if (!gti2DeliverReliableMessage(connection, pending->type, connection->incoming.data + pending->offset,
                                            pending->length)) {
                return FALSE;
            }
            gti2RemoveHoldMessage(connection, pending, i);
            goto restart;
        }
    }
    return TRUE;
}

// Starts the wait before an ack
static void gti2SetPendingAck(GTI2Connection *connection) {
    if (!connection->pendingAck) {
        connection->pendingAck = TRUE;
        connection->pendingAckTime = current_time();
    }
}

// Delivers it in its turn, keeps it for later or drops a repeat
static BOOL gti2HandleReliableMessage(GTI2Connection *connection, int type, u8 *data, int length) {
    int headerSize = connection->socket->protocolOffset + 7;
    int sequence;
    int ack;
    BOOL dropped;

    if (length < headerSize) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    sequence = gti2UShortFromBuffer(data + connection->socket->protocolOffset, 3);
    ack = gti2UShortFromBuffer(data + connection->socket->protocolOffset, 5);
    if (connection->socket->protocol == 2 && type == 0) {
        data[connection->socket->protocolOffset + 5] = data[0];
        data[connection->socket->protocolOffset + 6] = data[1];
        data += headerSize - connection->socket->protocolOffset;
        length -= headerSize - connection->socket->protocolOffset;
    } else {
        data += headerSize;
        length -= headerSize;
    }
    if (!gti2HandleESN(connection, ack)) {
        return FALSE;
    }
    if (sequence == connection->expectedSerialNumber) {
        gti2SetPendingAck(connection);
        if (!gti2DeliverReliableMessage(connection, type, data, length)) {
            return FALSE;
        }
        if (gti2DeliverHoldMessages(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    if (gti2SNDiff(sequence, connection->expectedSerialNumber) < 0) {
        gti2SetPendingAck(connection);
        return TRUE;
    }
    if (!gti2BufferIncomingMessage(connection, type, sequence, data, length, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        if (!gti2ConnectionMemoryError(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// An ack
static BOOL gti2HandleAck(GTI2Connection *connection, const u8 *data, int length) {
    if (length != 2) {
        if (gti2ConnectionCommunicationError(connection)) {
            return TRUE;
        }
        return FALSE;
    }
    return gti2HandleESN(connection, gti2UShortFromBuffer(data, 0)) != 0;
}

// Resends the messages from the first serial number to the last
static BOOL gti2HandleNack(GTI2Connection *connection, const u8 *data, int length) {
    int first = gti2UShortFromBuffer(data, 0);
    int last;
    int count;
    int i;

    if (length == 2) {
        last = first;
    } else if (length == 4) {
        last = gti2UShortFromBuffer(data, 2);
    } else {
        return gti2ConnectionCommunicationError(connection) != 0;
    }
    count = func_ov011_02171588(connection->outgoingMessages);
    for (i = 0; i < count; i++) {
        GTI2OutgoingMessage *message = func_ov011_0217158c(connection->outgoingMessages, i);

        if (gti2SNDiff(message->serialNumber, first) >= 0 && gti2SNDiff(message->serialNumber, last) <= 0) {
            if (!gti2ResendMessage(connection, message)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// A ping, which is answered with a pong
static BOOL gti2HandlePing(GTI2Connection *connection, u8 *data, int length) {
    return gti2SendPong(connection, data, length);
}

// A pong, with the time its ping was sent
static BOOL gti2HandlePong(GTI2Connection *connection, const u8 *data, int length) {
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
    if (gti2PingCallback(connection, current_time() - sendTime)) {
        return TRUE;
    }
    return FALSE;
}

// The peer's note that the connection is closed
static BOOL gti2HandleClosed(GTI2Connection *connection) {
    int reason;

    if (connection->state == 7) {
        return TRUE;
    }
    reason = 0;
    if (connection->state != 6) {
        reason = 1;
    }
    if (!gti2ConnectionError(connection, 2, reason)) {
        return FALSE;
    }
    return TRUE;
}

// Handles an unreliable message, by its type
static BOOL gti2HandleUnreliableMessage(GTI2Connection *connection, int type, u8 *data, int length) {
    int offset = connection->socket->protocolOffset + 3;
    int rest = length - offset;

    if (type == 100) {
        if (!gti2HandleAck(connection, data + offset, rest)) {
            return FALSE;
        }
    } else if (type == 101) {
        if (!gti2HandleNack(connection, data + offset, rest)) {
            return FALSE;
        }
    } else if (type == 102) {
        if (!gti2HandlePing(connection, data, length)) {
            return FALSE;
        }
    } else if (type == 103) {
        if (!gti2HandlePong(connection, data + offset, rest)) {
            return FALSE;
        }
    } else if (type == 104) {
        if (!gti2HandleClosed(connection)) {
            return FALSE;
        }
    }
    return TRUE;
}

// A datagram from an address, GT2's or the app's
static BOOL gti2HandleMessage(GTI2Socket *socket, u8 *data, int length, u32 ip, u16 port) {
    int payloadLength = length - socket->protocolOffset;
    u8 *payload = data + socket->protocolOffset;
    GTI2Connection *connection = gti2SocketFindConnection(socket, ip, port);
    BOOL handled;
    BOOL hasMagic;
    int result;
    int type;

    if (socket->onReceiveDump != NULL) {
        if (!gti2DumpCallback(socket, connection, ip, port, FALSE, data, length, FALSE)) {
            return FALSE;
        }
    }
    if (payloadLength > 2 && memcmp(payload, "\xfe\xfe", 2) == 0) {
        hasMagic = TRUE;
    } else {
        hasMagic = FALSE;
    }
    if (connection == NULL) {
        if (!gti2UnrecognizedMessageCallback(socket, ip, port, data, length, &handled)) {
            return FALSE;
        }
        if (handled) {
            return TRUE;
        }
        // Only the client's challenge opens a connection; anything else but the note of a closed one is answered
        // with that note
        if (!hasMagic || payload[2] != 1) {
            if (!hasMagic || payload[2] != 0x68) {
                if (!gti2SendClosedOnSocket(socket, ip, port)) {
                    return FALSE;
                }
            }
            return TRUE;
        }
        if (socket->onConnectAttempt == NULL) {
            return TRUE;
        }
        result = gti2NewIncomingConnection(socket, &connection, ip, port);
        if (result != 0) {
            if (result != 5) {
                if (!gti2SendClosedOnSocket(socket, ip, port)) {
                    return FALSE;
                }
            }
            return TRUE;
        }
    }
    if (connection->state == 7) {
        if (!hasMagic || payload[2] != 0x68) {
            if (!gti2SendClosed(connection)) {
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
            if (!gti2UnrecognizedMessageCallback(socket, ip, port, data, length, &handled)) {
                return FALSE;
            }
        } else {
            if (!gti2HandleAppUnreliable(connection, data, length)) {
                return FALSE;
            }
        }
        return TRUE;
    }
    type = payload[2];
    if (type < 8) {
        if (gti2HandleReliableMessage(connection, type, data, length)) {
            return TRUE;
        }
        return FALSE;
    }
    if (gti2HandleUnreliableMessage(connection, type, data, length)) {
        return TRUE;
    }
    return FALSE;
}

BOOL gti2HandleConnectionReset(GTI2Socket *socket, u32 ip, u16 port) {
    GTI2Connection *connection = gti2SocketFindConnection(socket, ip, port);

    if (socket->onReceiveDump != NULL) {
        if (!gti2DumpCallback(socket, connection, ip, port, TRUE, NULL, 0, FALSE)) {
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
        if (!gti2ConnectionError(connection, 6, 1)) {
            return FALSE;
        }
    } else {
        if (!gti2ConnectionError(connection, 2, 1)) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL gti2HandleHostUnreachable(GTI2Socket *socket, u32 ip, u16 port, BOOL send) {
    GTI2Connection *connection = gti2SocketFindConnection(socket, ip, port);

    if (socket->onReceiveDump != NULL) {
        if (!gti2DumpCallback(socket, connection, ip, port, TRUE, NULL, 0, send)) {
            return FALSE;
        }
    }
    if (connection == NULL) {
        return TRUE;
    }
    return gti2ConnectionError(connection, 6, 1) != 0;
}

BOOL gti2ReceiveMessages(GTI2Socket *socket) {
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
                    if (!gti2HandleConnectionReset(socket, address.addr, GTI2_SWAP16(address.port))) {
                        return FALSE;
                    }
                } else if (error == -23) {
                    if (!gti2HandleHostUnreachable(socket, address.addr, GTI2_SWAP16(address.port), FALSE)) {
                        return FALSE;
                    }
                } else if (error != -35) {
                    gti2SocketError(socket);
                    return FALSE;
                }
            } else {
                if (!gti2HandleMessage(socket, buffer, length, address.addr, GTI2_SWAP16(address.port))) {
                    return FALSE;
                }
            }
        } while (CanReceiveOnSocket(socket->handle));
    }
    return TRUE;
}

// Keeps a reliable message that was sent, for its ack
static BOOL gti2StoreOutgoingReliableMessageInfo(GTI2Connection *connection, u16 sequence, int length) {
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

// The header of a reliable message in the outgoing buffer, dropped when there is no room
static BOOL gti2BeginReliableMessage(GTI2Connection *connection, int type, int size, BOOL *dropped) {
    u16 payloadLength = size - connection->socket->protocolOffset;

    if (gti2GetBufferFreeSpace(&connection->outgoing) < size) {
        if (!gti2ConnectionMemoryError(connection)) {
            return FALSE;
        }
        *dropped = TRUE;
        return TRUE;
    }
    if (!gti2StoreOutgoingReliableMessageInfo(connection, connection->serialNumber, size)) {
        if (!gti2ConnectionMemoryError(connection)) {
            return FALSE;
        }
        *dropped = TRUE;
        return TRUE;
    }
    if (connection->socket->protocol == 2) {
        gti2BufferWriteData(&connection->outgoing, &payloadLength, connection->socket->protocolOffset);
    }
    gti2BufferWriteData(&connection->outgoing, "\xfe\xfe", 2);
    gti2BufferWriteByte(&connection->outgoing, type);
    gti2BufferWriteUShort(&connection->outgoing, connection->serialNumber++);
    gti2BufferWriteUShort(&connection->outgoing, connection->expectedSerialNumber);
    *dropped = FALSE;
    return TRUE;
}

// Sends the reliable message just built
static BOOL gti2EndReliableMessage(GTI2Connection *connection) {
    GTI2OutgoingMessage *last =
        func_ov011_0217158c(connection->outgoingMessages, func_ov011_02171588(connection->outgoingMessages) - 1);

    if (!gti2ConnectionSendData(connection, connection->outgoing.data + last->offset, last->length)) {
        return FALSE;
    }
    connection->pendingAck = FALSE;
    return TRUE;
}

// Sends the app's reliable message
static BOOL gti2SendAppReliable(GTI2Connection *connection, const void *data, int length) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 0, length + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    gti2BufferWriteData(&connection->outgoing, data, length);
    return gti2EndReliableMessage(connection) != 0;
}

BOOL gti2SendClientChallenge(GTI2Connection *connection, const u8 *challenge) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 1, connection->socket->protocolOffset + 0x27, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    gti2BufferWriteData(&connection->outgoing, challenge, 32);
    return gti2EndReliableMessage(connection) != 0;
}

// The server's challenge, with the response to the client's
static BOOL gti2SendServerChallenge(GTI2Connection *connection, const u8 *response, const u8 *challenge) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 2, connection->socket->protocolOffset + 0x47, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    gti2BufferWriteData(&connection->outgoing, response, 32);
    gti2BufferWriteData(&connection->outgoing, challenge, 32);
    if (!gti2EndReliableMessage(connection)) {
        return FALSE;
    }
    connection->challengeTime = connection->lastSendTime;
    return TRUE;
}

// The client's response, with the connect attempt's message
static BOOL gti2SendClientResponse(GTI2Connection *connection, const u8 *response, const void *message, int length) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 3, length + 0x27 + connection->socket->protocolOffset, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    gti2BufferWriteData(&connection->outgoing, response, 32);
    gti2BufferWriteData(&connection->outgoing, message, length);
    if (gti2EndReliableMessage(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL gti2SendAccept(GTI2Connection *connection) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 4, connection->socket->protocolOffset + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    if (gti2EndReliableMessage(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL gti2SendReject(GTI2Connection *connection, const void *message, int length) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 5, length + 7 + connection->socket->protocolOffset, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    gti2BufferWriteData(&connection->outgoing, message, length);
    if (gti2EndReliableMessage(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL gti2SendClose(GTI2Connection *connection) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 6, connection->socket->protocolOffset + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    if (gti2EndReliableMessage(connection)) {
        return TRUE;
    }
    return FALSE;
}

BOOL gti2SendKeepAlive(GTI2Connection *connection) {
    BOOL dropped;

    if (!gti2BeginReliableMessage(connection, 7, connection->socket->protocolOffset + 7, &dropped)) {
        return FALSE;
    }
    if (dropped) {
        return TRUE;
    }
    if (gti2EndReliableMessage(connection)) {
        return TRUE;
    }
    return FALSE;
}

// A message that starts with 0xfe 0xfe gets them twice, so it isn't read as GT2's
static BOOL gti2SendAppUnreliable(GTI2Connection *connection, const u8 *data, int length) {
    int base;
    u8 *start;
    int total;

    if (length < 2 || memcmp(data + connection->socket->protocolOffset, "\xfe\xfe", 2) != 0) {
        if (!gti2ConnectionSendData(connection, data, length)) {
            return FALSE;
        }
        return TRUE;
    }
    total = length + 2;
    if (gti2GetBufferFreeSpace(&connection->outgoing) < total) {
        return TRUE;
    }
    start = connection->outgoing.data;
    base = connection->outgoing.length;
    if (connection->socket->protocol == 2) {
        gti2BufferWriteData(&connection->outgoing, data, 2);
    }
    gti2BufferWriteData(&connection->outgoing, "\xfe\xfe", 2);
    gti2BufferWriteData(&connection->outgoing, data + connection->socket->protocolOffset,
                        length - connection->socket->protocolOffset);
    if (!gti2ConnectionSendData(connection, start + base, total)) {
        return FALSE;
    }
    gti2BufferShorten(&connection->outgoing, -1, total);
    return TRUE;
}

BOOL gti2SendAck(GTI2Connection *connection) {
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
    gti2UShortToBuffer(packet, length + 3, connection->expectedSerialNumber);
    if (!gti2ConnectionSendData(connection, packet, length + 5)) {
        return FALSE;
    }
    connection->pendingAck = FALSE;
    return TRUE;
}

// Nacks the serial numbers from first to last
static BOOL gti2SendNack(GTI2Connection *connection, u16 first, u16 last) {
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
    gti2UShortToBuffer(packet, length + 3, first);
    length += 5;
    if (first != last) {
        gti2UShortToBuffer(packet, length, last);
        length += 2;
    }
    if (!gti2ConnectionSendData(connection, packet, length)) {
        return FALSE;
    }
    return TRUE;
}

// Answers a ping with a pong
static BOOL gti2SendPong(GTI2Connection *connection, u8 *data, int length) {
    data[2] = 0x67;
    return gti2ConnectionSendData(connection, data, length);
}

BOOL gti2SendClosed(GTI2Connection *connection) {
    return gti2SendClosedOnSocket(connection->socket, connection->ip, connection->port);
}

// Sends an address the note that its connection is closed
static BOOL gti2SendClosedOnSocket(GTI2Socket *socket, u32 ip, u16 port) {
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
    if (!gti2SocketSend(socket, ip, port, packet, length + 3)) {
        return FALSE;
    }
    return TRUE;
}

BOOL gti2ResendMessage(GTI2Connection *connection, GTI2OutgoingMessage *message) {
    gti2UShortToBuffer(connection->outgoing.data, message->offset + connection->socket->protocolOffset + 5,
                       connection->expectedSerialNumber);
    if (!gti2ConnectionSendData(connection, connection->outgoing.data + message->offset, message->length)) {
        return FALSE;
    }
    message->sendTime = connection->lastSendTime;
    if (connection->outgoing.data[message->offset + connection->socket->protocolOffset + 2] == 2) {
        connection->challengeTime = connection->lastSendTime;
    }
    return TRUE;
}

BOOL gti2Send(GTI2Connection *connection, const void *data, int length, BOOL reliable) {
    if (reliable) {
        return gti2SendAppReliable(connection, data, length);
    }
    return gti2SendAppUnreliable(connection, data, length);
}
