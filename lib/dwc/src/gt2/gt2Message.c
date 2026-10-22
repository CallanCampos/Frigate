#include <string.h>
// Before dwc/ext.h: MWCC builds the packets' 2-byte memcpys in place, as the ROM does, only when
// <string.h> declares them first
#include "dwc/ext.h"
#include "dwc/gt2Auth.h"
#include "dwc/gt2Buffer.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Connection.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"

// GameSpy GT2's messages (gt2Message.c)

static int gti2UShortFromBuffer(const GT2Byte *buffer, int index);
static void gti2UShortToBuffer(GT2Byte *buffer, int index, unsigned short value);
static gsi_i16 gti2SNDiff(int a, int b);
static GT2Bool gti2ConnectionError(GT2Connection connection, GT2Result result, GT2CloseReason reason);
static GT2Bool gti2ConnectionCommunicationError(GT2Connection connection);
static GT2Bool gti2ConnectionMemoryError(GT2Connection connection);
static GT2Bool gti2HandleESN(GT2Connection connection, int ack);
static GT2Bool gti2HandleAppUnreliable(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2HandleAppReliable(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2HandleClientChallenge(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2HandleServerChallenge(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2HandleClientResponse(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2HandleAccept(GT2Connection connection);
static GT2Bool gti2HandleReject(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2HandleClose(GT2Connection connection);
static GT2Bool gti2DeliverReliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte *data, int length);
static int gti2IncomingBufferMessageCompare(const void *a, const void *b);
static GT2Bool gti2BufferIncomingMessage(GT2Connection connection, GTI2MessageType type, int sequence, GT2Byte *data,
                                         int length, GT2Bool *dropped);
static void gti2RemoveHoldMessage(GT2Connection connection, GTI2IncomingBufferMessage *message, int index);
static GT2Bool gti2DeliverHoldMessages(GT2Connection connection);
static void gti2SetPendingAck(GT2Connection connection);
static GT2Bool gti2HandleReliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte *data, int length);
static GT2Bool gti2HandleAck(GT2Connection connection, const GT2Byte *data, int length);
static GT2Bool gti2HandleNack(GT2Connection connection, const GT2Byte *data, int length);
static GT2Bool gti2HandlePing(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2HandlePong(GT2Connection connection, const GT2Byte *data, int length);
static GT2Bool gti2HandleClosed(GT2Connection connection);
static GT2Bool gti2HandleUnreliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte *data, int length);
static GT2Bool gti2HandleMessage(GT2Socket socket, GT2Byte *data, int length, unsigned int ip, unsigned short port);
static GT2Bool gti2StoreOutgoingReliableMessageInfo(GT2Connection connection, unsigned short sequence, int length);
static GT2Bool gti2BeginReliableMessage(GT2Connection connection, GTI2MessageType type, int size, GT2Bool *dropped);
static GT2Bool gti2EndReliableMessage(GT2Connection connection);
static GT2Bool gti2SendAppReliable(GT2Connection connection, const GT2Byte *data, int length);
static GT2Bool gti2SendServerChallenge(GT2Connection connection, const char response[], const char challenge[]);
static GT2Bool gti2SendClientResponse(GT2Connection connection, const char response[], const char *message, int length);
static GT2Bool gti2SendAppUnreliable(GT2Connection connection, const GT2Byte *data, int length);
static GT2Bool gti2SendNack(GT2Connection connection, unsigned short first, unsigned short last);
static GT2Bool gti2SendPong(GT2Connection connection, GT2Byte *data, int length);
static GT2Bool gti2SendClosedOnSocket(GT2Socket socket, unsigned int ip, unsigned short port);

// Reads the big-endian gsi_u16 at an index
static int gti2UShortFromBuffer(const GT2Byte *buffer, int index) {
    return (gsi_u16)((buffer[index] << 8) & 0xFF00) | buffer[index + 1];
}

// Writes a big-endian gsi_u16 at an index
static void gti2UShortToBuffer(GT2Byte *buffer, int index, unsigned short value) {
    buffer[index] = value >> 8;
    buffer[index + 1] = value;
}

// How far one serial number is past another; they wrap around
static gsi_i16 gti2SNDiff(int a, int b) {
    return a - b;
}

// Closes a connection after a message that ends it: a connecting one ends its attempt with the result, an open
// one closes with the reason
static GT2Bool gti2ConnectionError(GT2Connection connection, GT2Result result, GT2CloseReason reason) {
    int state = connection->state;

    if (state < 5) {
        if (connection->initiated) {
            gti2ConnectionClosed(connection);
            if (!gti2ConnectedCallback(connection, result, NULL, 0)) {
                return gsi_false;
            }
        } else {
            if (state == 4) {
                connection->freeAtAcceptReject = gsi_true;
            }
            gti2ConnectionClosed(connection);
        }
    } else if (state != 7) {
        gti2ConnectionClosed(connection);
        if (!gti2ClosedCallback(connection, reason)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// Closes a connection after a message that breaks the protocol
static GT2Bool gti2ConnectionCommunicationError(GT2Connection connection) {
    return gti2ConnectionError(connection, 7, 2);
}

// Closes a connection that ran out of memory, telling the peer
static GT2Bool gti2ConnectionMemoryError(GT2Connection connection) {
    if (gti2SendClosed(connection)) {
        return gti2ConnectionError(connection, 1, 4);
    }
    return gsi_false;
}

// Drops the reliable messages that the peer's expected serial number acks
static GT2Bool gti2HandleESN(GT2Connection connection, int ack) {
    int count = func_ov011_02171588(connection->outgoingMessages);
    int i;
    int base;

    if (count == 0) {
        return gsi_true;
    }
    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *sent = func_ov011_0217158c(connection->outgoingMessages, i);

        if (gti2SNDiff(sent->serialNumber, ack) >= 0) {
            break;
        }
    }
    if (i == 0) {
        return gsi_true;
    }
    while (i-- != 0) {
        func_ov011_02171668(connection->outgoingMessages, i);
    }
    count = func_ov011_02171588(connection->outgoingMessages);
    if (count == 0) {
        connection->outgoing.length = 0;
        return gsi_true;
    }
    base = ((GTI2OutgoingBufferMessage *)func_ov011_0217158c(connection->outgoingMessages, 0))->offset;
    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *sent = func_ov011_0217158c(connection->outgoingMessages, i);

        sent->offset -= base;
    }
    gti2BufferShorten(&connection->outgoing, 0, base);
    return gsi_true;
}

// Delivers the app's unreliable message, through the receive filters if there are any
static GT2Bool gti2HandleAppUnreliable(GT2Connection connection, GT2Byte *data, int length) {
    if (connection->state != 5 && connection->state != 6) {
        return gsi_true;
    }
    if (func_ov011_02171588(connection->receiveFilters) != 0) {
        return gti2ReceiveFilterCallback(connection, 0, data, length, gsi_false) != 0;
    }
    return gti2ReceivedCallback(connection, data, length, gsi_false) != 0;
}

// Delivers the app's reliable message
static GT2Bool gti2HandleAppReliable(GT2Connection connection, GT2Byte *data, int length) {
    if (connection->state != 5 && connection->state != 6) {
        if (!gti2ConnectionCommunicationError(connection)) {
            return gsi_false;
        }
    } else {
        if (func_ov011_02171588(connection->receiveFilters) != 0) {
            return gti2ReceiveFilterCallback(connection, 0, data, length, gsi_true) != 0;
        }
        if (!gti2ReceivedCallback(connection, data, length, gsi_true)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// The client's challenge, on the side that listens: answers with the server's challenge
static GT2Bool gti2HandleClientChallenge(GT2Connection connection, GT2Byte *data, int length) {
    gsi_u8 response[32];
    gsi_u8 challenge[32];

    if (connection->state != 2) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (length < 32) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    gti2GetResponse(response, data);
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    if (!gti2SendServerChallenge(connection, response, challenge)) {
        return gsi_false;
    }
    connection->state = 3;
    return gsi_true;
}

// Checks the response to this side's challenge and answers the server's
static GT2Bool gti2HandleServerChallenge(GT2Connection connection, GT2Byte *data, int length) {
    gsi_u8 response[32];

    if (connection->state != 0) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (length < 64) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (!gti2CheckResponse(data, connection->response)) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    gti2GetResponse(response, data + 32);
    if (!gti2SendClientResponse(connection, response, connection->initialMessage, connection->initialMessageLength)) {
        return gsi_false;
    }
    if (connection->initialMessage != NULL) {
        gsifree(connection->initialMessage);
        connection->initialMessage = NULL;
    }
    connection->state = 1;
    return gsi_true;
}

// Checks the response and gives the app the attempt
static GT2Bool gti2HandleClientResponse(GT2Connection connection, GT2Byte *data, int length) {
    if (connection->state != 3) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (length < 32) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (!gti2CheckResponse(data, connection->response)) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (connection->socket->onConnectAttempt == NULL) {
        if (!gti2SendClosed(connection)) {
            return gsi_false;
        }
        gti2ConnectionClosed(connection);
        return gsi_true;
    }
    connection->state = 4;
    if (gti2ConnectAttemptCallback(connection->socket, connection, connection->ip, connection->port,
                                   current_time() - connection->challengeTime, data + 32, length - 32)) {
        return gsi_true;
    }
    return gsi_false;
}

// The server accepted the connection
static GT2Bool gti2HandleAccept(GT2Connection connection) {
    if (connection->state != 1) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    connection->state = 5;
    return gti2ConnectedCallback(connection, 0, NULL, 0) != 0;
}

// The server rejected the connection, with its message
static GT2Bool gti2HandleReject(GT2Connection connection, GT2Byte *data, int length) {
    if (connection->state != 1) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    gti2ConnectionClosed(connection);
    if (!gti2SendClosed(connection)) {
        return gsi_false;
    }
    if (gti2ConnectedCallback(connection, 2, data, length)) {
        return gsi_true;
    }
    return gsi_false;
}

// The peer closed the connection
static GT2Bool gti2HandleClose(GT2Connection connection) {
    int reason;

    if (!gti2SendClosed(connection)) {
        return gsi_false;
    }
    reason = 0;
    if (connection->state != 6) {
        reason = 1;
    }
    if (!gti2ConnectionError(connection, 2, reason)) {
        return gsi_false;
    }
    return gsi_true;
}

// Delivers a reliable message, by its type
static GT2Bool gti2DeliverReliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte *data, int length) {
    connection->expectedSerialNumber++;
    if (type == 0) {
        if (!gti2HandleAppReliable(connection, data, length)) {
            return gsi_false;
        }
    } else if (type == 1) {
        if (!gti2HandleClientChallenge(connection, data, length)) {
            return gsi_false;
        }
    } else if (type == 2) {
        if (!gti2HandleServerChallenge(connection, data, length)) {
            return gsi_false;
        }
    } else if (type == 3) {
        if (!gti2HandleClientResponse(connection, data, length)) {
            return gsi_false;
        }
    } else if (type == 4) {
        if (!gti2HandleAccept(connection)) {
            return gsi_false;
        }
    } else if (type == 5) {
        if (!gti2HandleReject(connection, data, length)) {
            return gsi_false;
        }
    } else if (type == 6) {
        if (!gti2HandleClose(connection)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// Compares two held messages by serial number
static int gti2IncomingBufferMessageCompare(const void *a, const void *b) {
    const GTI2IncomingBufferMessage *messageA = a;
    const GTI2IncomingBufferMessage *messageB = b;

    return gti2SNDiff(messageA->serialNumber, messageB->serialNumber);
}

// Keeps a reliable message that arrived ahead of its turn, and nacks the ones it
// skipped; dropped when there is no room
static GT2Bool gti2BufferIncomingMessage(GT2Connection connection, GTI2MessageType type, int sequence, GT2Byte *data,
                                         int length, GT2Bool *dropped) {
    GTI2IncomingBufferMessage message;
    int count = func_ov011_02171588(connection->incomingMessages);
    int i;

    for (i = 0; i < count; i++) {
        GTI2IncomingBufferMessage *pending = func_ov011_0217158c(connection->incomingMessages, i);

        if (pending->serialNumber == sequence) {
            *dropped = gsi_false;
            return gsi_true;
        }
        if (gti2SNDiff(pending->serialNumber, sequence) > 0) {
            break;
        }
    }
    if (gti2GetBufferFreeSpace(&connection->incoming) < length) {
        *dropped = gsi_true;
        return gsi_true;
    }
    message.offset = connection->incoming.length;
    message.length = length;
    message.type = type;
    message.serialNumber = sequence;
    ArrayInsertSorted(connection->incomingMessages, &message, gti2IncomingBufferMessageCompare);
    if (count + 1 != func_ov011_02171588(connection->incomingMessages)) {
        *dropped = gsi_true;
        return gsi_true;
    }
    gti2BufferWriteData(&connection->incoming, data, length);
    if (count == 0) {
        if (!gti2SendNack(connection, connection->expectedSerialNumber, sequence - 1)) {
            return gsi_false;
        }
    } else {
        GTI2IncomingBufferMessage *inserted = func_ov011_0217158c(connection->incomingMessages, count);

        if (inserted->serialNumber == sequence) {
            GTI2IncomingBufferMessage *previous = func_ov011_0217158c(connection->incomingMessages, count - 1);

            if ((gsi_u16)gti2SNDiff(sequence, previous->serialNumber) > 1) {
                if (!gti2SendNack(connection, previous->serialNumber + 1, sequence - 1)) {
                    return gsi_false;
                }
            }
        }
    }
    *dropped = gsi_false;
    return gsi_true;
}

// Removes a buffered incoming message from the buffer
static void gti2RemoveHoldMessage(GT2Connection connection, GTI2IncomingBufferMessage *message, int index) {
    int offset = message->offset;
    int length = message->length;
    int count;
    int i;
    int end = 0;

    func_ov011_02171668(connection->incomingMessages, index);
    count = func_ov011_02171588(connection->incomingMessages);
    for (i = 0; i < count; i++) {
        GTI2IncomingBufferMessage *pending = func_ov011_0217158c(connection->incomingMessages, i);

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
static GT2Bool gti2DeliverHoldMessages(GT2Connection connection) {
    int i;

restart:
    for (i = func_ov011_02171588(connection->incomingMessages) - 1; i >= 0; i--) {
        GTI2IncomingBufferMessage *pending = func_ov011_0217158c(connection->incomingMessages, i);

        if (pending->serialNumber == connection->expectedSerialNumber) {
            if (!gti2DeliverReliableMessage(connection, pending->type, connection->incoming.data + pending->offset,
                                            pending->length)) {
                return gsi_false;
            }
            gti2RemoveHoldMessage(connection, pending, i);
            goto restart;
        }
    }
    return gsi_true;
}

// Starts the wait before an ack
static void gti2SetPendingAck(GT2Connection connection) {
    if (!connection->pendingAck) {
        connection->pendingAck = gsi_true;
        connection->pendingAckTime = current_time();
    }
}

// Delivers it in its turn, keeps it for later or drops a repeat
static GT2Bool gti2HandleReliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte *data, int length) {
    int headerSize = connection->socket->protocolOffset + 7;
    int sequence;
    int ack;
    gsi_bool dropped;

    if (length < headerSize) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    sequence = gti2UShortFromBuffer(data + connection->socket->protocolOffset, 3);
    ack = gti2UShortFromBuffer(data + connection->socket->protocolOffset, 5);
    if (connection->socket->protocol == GTI2VdpProtocol && type == 0) {
        data[connection->socket->protocolOffset + 5] = data[0];
        data[connection->socket->protocolOffset + 6] = data[1];
        data += headerSize - connection->socket->protocolOffset;
        length -= headerSize - connection->socket->protocolOffset;
    } else {
        data += headerSize;
        length -= headerSize;
    }
    if (!gti2HandleESN(connection, ack)) {
        return gsi_false;
    }
    if (sequence == connection->expectedSerialNumber) {
        gti2SetPendingAck(connection);
        if (!gti2DeliverReliableMessage(connection, type, data, length)) {
            return gsi_false;
        }
        if (gti2DeliverHoldMessages(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (gti2SNDiff(sequence, connection->expectedSerialNumber) < 0) {
        gti2SetPendingAck(connection);
        return gsi_true;
    }
    if (!gti2BufferIncomingMessage(connection, type, sequence, data, length, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        if (!gti2ConnectionMemoryError(connection)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// An ack
static GT2Bool gti2HandleAck(GT2Connection connection, const GT2Byte *data, int length) {
    if (length != 2) {
        if (gti2ConnectionCommunicationError(connection)) {
            return gsi_true;
        }
        return gsi_false;
    }
    return gti2HandleESN(connection, gti2UShortFromBuffer(data, 0)) != 0;
}

// Resends the messages from the first serial number to the last
static GT2Bool gti2HandleNack(GT2Connection connection, const GT2Byte *data, int length) {
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
        GTI2OutgoingBufferMessage *message = func_ov011_0217158c(connection->outgoingMessages, i);

        if (gti2SNDiff(message->serialNumber, first) >= 0 && gti2SNDiff(message->serialNumber, last) <= 0) {
            if (!gti2ResendMessage(connection, message)) {
                return gsi_false;
            }
        }
    }
    return gsi_true;
}

// A ping, which is answered with a pong
static GT2Bool gti2HandlePing(GT2Connection connection, GT2Byte *data, int length) {
    return gti2SendPong(connection, data, length);
}

// A pong, with the time its ping was sent
static GT2Bool gti2HandlePong(GT2Connection connection, const GT2Byte *data, int length) {
    gsi_u32 sendTime;
    const gsi_u8 *src;
    gsi_u8 *dest;

    if (connection->callbacks.ping == NULL) {
        return gsi_true;
    }
    if (length != 8) {
        return gsi_true;
    }
    if (memcmp(data, "time", 4) != 0) {
        return gsi_true;
    }
    src = data + 4;
    dest = (gsi_u8 *)&sendTime;
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
    if (gti2PingCallback(connection, current_time() - sendTime)) {
        return gsi_true;
    }
    return gsi_false;
}

// The peer's note that the connection is closed
static GT2Bool gti2HandleClosed(GT2Connection connection) {
    int reason;

    if (connection->state == 7) {
        return gsi_true;
    }
    reason = 0;
    if (connection->state != 6) {
        reason = 1;
    }
    if (!gti2ConnectionError(connection, 2, reason)) {
        return gsi_false;
    }
    return gsi_true;
}

// Handles an unreliable message, by its type
static GT2Bool gti2HandleUnreliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte *data, int length) {
    int offset = connection->socket->protocolOffset + 3;
    int rest = length - offset;

    if (type == 100) {
        if (!gti2HandleAck(connection, data + offset, rest)) {
            return gsi_false;
        }
    } else if (type == 101) {
        if (!gti2HandleNack(connection, data + offset, rest)) {
            return gsi_false;
        }
    } else if (type == 102) {
        if (!gti2HandlePing(connection, data, length)) {
            return gsi_false;
        }
    } else if (type == 103) {
        if (!gti2HandlePong(connection, data + offset, rest)) {
            return gsi_false;
        }
    } else if (type == 104) {
        if (!gti2HandleClosed(connection)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// A datagram from an address, GT2's or the app's
static GT2Bool gti2HandleMessage(GT2Socket socket, GT2Byte *data, int length, unsigned int ip, unsigned short port) {
    int payloadLength = length - socket->protocolOffset;
    gsi_u8 *payload = data + socket->protocolOffset;
    GT2Connection connection = gti2SocketFindConnection(socket, ip, port);
    gsi_bool handled;
    gsi_bool hasMagic;
    int result;
    int type;

    if (socket->onReceiveDump != NULL) {
        if (!gti2DumpCallback(socket, connection, ip, port, gsi_false, data, length, gsi_false)) {
            return gsi_false;
        }
    }
    if (payloadLength > 2 && memcmp(payload, "\xfe\xfe", 2) == 0) {
        hasMagic = gsi_true;
    } else {
        hasMagic = gsi_false;
    }
    if (connection == NULL) {
        if (!gti2UnrecognizedMessageCallback(socket, ip, port, data, length, &handled)) {
            return gsi_false;
        }
        if (handled) {
            return gsi_true;
        }
        // Only the client's challenge opens a connection; anything else but the note of a closed one is answered
        // with that note
        if (!hasMagic || payload[2] != 1) {
            if (!hasMagic || payload[2] != 0x68) {
                if (!gti2SendClosedOnSocket(socket, ip, port)) {
                    return gsi_false;
                }
            }
            return gsi_true;
        }
        if (socket->onConnectAttempt == NULL) {
            return gsi_true;
        }
        result = gti2NewIncomingConnection(socket, &connection, ip, port);
        if (result != 0) {
            if (result != 5) {
                if (!gti2SendClosedOnSocket(socket, ip, port)) {
                    return gsi_false;
                }
            }
            return gsi_true;
        }
    }
    if (connection->state == 7) {
        if (!hasMagic || payload[2] != 0x68) {
            if (!gti2SendClosed(connection)) {
                return gsi_false;
            }
        }
        return gsi_true;
    }
    if (hasMagic && payloadLength >= 4 && memcmp(payload + 2, "\xfe\xfe", 2) == 0) {
        payload += 2;
        length -= 2;
        data[3] = data[1];
        data[2] = data[0];
        hasMagic = gsi_false;
        data += 2;
    }
    if (!hasMagic) {
        if (connection->state < 5) {
            if (!gti2UnrecognizedMessageCallback(socket, ip, port, data, length, &handled)) {
                return gsi_false;
            }
        } else {
            if (!gti2HandleAppUnreliable(connection, data, length)) {
                return gsi_false;
            }
        }
        return gsi_true;
    }
    type = payload[2];
    if (type < 8) {
        if (gti2HandleReliableMessage(connection, type, data, length)) {
            return gsi_true;
        }
        return gsi_false;
    }
    if (gti2HandleUnreliableMessage(connection, type, data, length)) {
        return gsi_true;
    }
    return gsi_false;
}

GT2Bool gti2HandleConnectionReset(GT2Socket socket, unsigned int ip, unsigned short port) {
    GT2Connection connection = gti2SocketFindConnection(socket, ip, port);

    if (socket->onReceiveDump != NULL) {
        if (!gti2DumpCallback(socket, connection, ip, port, gsi_true, NULL, 0, gsi_false)) {
            return gsi_false;
        }
    }
    if (connection == NULL) {
        return gsi_true;
    }
    if (connection->state == 0) {
        if (connection->timeout == 0 || current_time() - connection->startTime < connection->timeout) {
            return gsi_true;
        }
        if (!gti2ConnectionError(connection, 6, 1)) {
            return gsi_false;
        }
    } else {
        if (!gti2ConnectionError(connection, 2, 1)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

GT2Bool gti2HandleHostUnreachable(GT2Socket socket, unsigned int ip, unsigned short port, GT2Bool send) {
    GT2Connection connection = gti2SocketFindConnection(socket, ip, port);

    if (socket->onReceiveDump != NULL) {
        if (!gti2DumpCallback(socket, connection, ip, port, gsi_true, NULL, 0, send)) {
            return gsi_false;
        }
    }
    if (connection == NULL) {
        return gsi_true;
    }
    return gti2ConnectionError(connection, 6, 1) != 0;
}

GT2Bool gti2ReceiveMessages(GT2Socket socket) {
    gsi_u8 buffer[1500];
    SOCSockAddrIn address;
    int addressSize;
    int length;
    int error;

    if (CanReceiveOnSocket(socket->handle) != 0) {
        do {
            addressSize = sizeof(address);
            length = recvfrom(socket->handle, buffer, sizeof(buffer), 0, (SOCKADDR *)&address, &addressSize);
            if (length == -1) {
                error = GOAGetLastError(socket->handle);
                if (error == -15) {
                    if (!gti2HandleConnectionReset(socket, address.addr.addr, GTI2_SWAP16(address.port))) {
                        return gsi_false;
                    }
                } else if (error == -23) {
                    if (!gti2HandleHostUnreachable(socket, address.addr.addr, GTI2_SWAP16(address.port), gsi_false)) {
                        return gsi_false;
                    }
                } else if (error != -35) {
                    gti2SocketError(socket);
                    return gsi_false;
                }
            } else {
                if (!gti2HandleMessage(socket, buffer, length, address.addr.addr, GTI2_SWAP16(address.port))) {
                    return gsi_false;
                }
            }
        } while (CanReceiveOnSocket(socket->handle));
    }
    return gsi_true;
}

// Keeps a reliable message that was sent, for its ack
static GT2Bool gti2StoreOutgoingReliableMessageInfo(GT2Connection connection, unsigned short sequence, int length) {
    GTI2OutgoingBufferMessage message = { 0, 0, 0, 0 };
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
static GT2Bool gti2BeginReliableMessage(GT2Connection connection, GTI2MessageType type, int size, GT2Bool *dropped) {
    gsi_u16 payloadLength = size - connection->socket->protocolOffset;

    if (gti2GetBufferFreeSpace(&connection->outgoing) < size) {
        if (!gti2ConnectionMemoryError(connection)) {
            return gsi_false;
        }
        *dropped = gsi_true;
        return gsi_true;
    }
    if (!gti2StoreOutgoingReliableMessageInfo(connection, connection->serialNumber, size)) {
        if (!gti2ConnectionMemoryError(connection)) {
            return gsi_false;
        }
        *dropped = gsi_true;
        return gsi_true;
    }
    if (connection->socket->protocol == GTI2VdpProtocol) {
        gti2BufferWriteData(&connection->outgoing, (const GT2Byte *)&payloadLength, connection->socket->protocolOffset);
    }
    gti2BufferWriteData(&connection->outgoing, "\xfe\xfe", 2);
    gti2BufferWriteByte(&connection->outgoing, type);
    gti2BufferWriteUShort(&connection->outgoing, connection->serialNumber++);
    gti2BufferWriteUShort(&connection->outgoing, connection->expectedSerialNumber);
    *dropped = gsi_false;
    return gsi_true;
}

// Sends the reliable message just built
static GT2Bool gti2EndReliableMessage(GT2Connection connection) {
    GTI2OutgoingBufferMessage *last =
        func_ov011_0217158c(connection->outgoingMessages, func_ov011_02171588(connection->outgoingMessages) - 1);

    if (!gti2ConnectionSendData(connection, connection->outgoing.data + last->offset, last->length)) {
        return gsi_false;
    }
    connection->pendingAck = gsi_false;
    return gsi_true;
}

// Sends the app's reliable message
static GT2Bool gti2SendAppReliable(GT2Connection connection, const GT2Byte *data, int length) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 0, length + 7, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    gti2BufferWriteData(&connection->outgoing, data, length);
    return gti2EndReliableMessage(connection) != 0;
}

GT2Bool gti2SendClientChallenge(GT2Connection connection, const char challenge[]) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 1, connection->socket->protocolOffset + 0x27, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    gti2BufferWriteData(&connection->outgoing, challenge, 32);
    return gti2EndReliableMessage(connection) != 0;
}

// The server's challenge, with the response to the client's
static GT2Bool gti2SendServerChallenge(GT2Connection connection, const char response[], const char challenge[]) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 2, connection->socket->protocolOffset + 0x47, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    gti2BufferWriteData(&connection->outgoing, response, 32);
    gti2BufferWriteData(&connection->outgoing, challenge, 32);
    if (!gti2EndReliableMessage(connection)) {
        return gsi_false;
    }
    connection->challengeTime = connection->lastSendTime;
    return gsi_true;
}

// The client's response, with the connect attempt's message
static GT2Bool gti2SendClientResponse(GT2Connection connection, const char response[], const char *message,
                                      int length) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 3, length + 0x27 + connection->socket->protocolOffset, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    gti2BufferWriteData(&connection->outgoing, response, 32);
    gti2BufferWriteData(&connection->outgoing, message, length);
    if (gti2EndReliableMessage(connection)) {
        return gsi_true;
    }
    return gsi_false;
}

GT2Bool gti2SendAccept(GT2Connection connection) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 4, connection->socket->protocolOffset + 7, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    if (gti2EndReliableMessage(connection)) {
        return gsi_true;
    }
    return gsi_false;
}

GT2Bool gti2SendReject(GT2Connection connection, const GT2Byte *message, int length) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 5, length + 7 + connection->socket->protocolOffset, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    gti2BufferWriteData(&connection->outgoing, message, length);
    if (gti2EndReliableMessage(connection)) {
        return gsi_true;
    }
    return gsi_false;
}

GT2Bool gti2SendClose(GT2Connection connection) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 6, connection->socket->protocolOffset + 7, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    if (gti2EndReliableMessage(connection)) {
        return gsi_true;
    }
    return gsi_false;
}

GT2Bool gti2SendKeepAlive(GT2Connection connection) {
    gsi_bool dropped;

    if (!gti2BeginReliableMessage(connection, 7, connection->socket->protocolOffset + 7, &dropped)) {
        return gsi_false;
    }
    if (dropped) {
        return gsi_true;
    }
    if (gti2EndReliableMessage(connection)) {
        return gsi_true;
    }
    return gsi_false;
}

// A message that starts with 0xfe 0xfe gets them twice, so it isn't read as GT2's
static GT2Bool gti2SendAppUnreliable(GT2Connection connection, const GT2Byte *data, int length) {
    int base;
    gsi_u8 *start;
    int total;

    if (length < 2 || memcmp(data + connection->socket->protocolOffset, "\xfe\xfe", 2) != 0) {
        if (!gti2ConnectionSendData(connection, data, length)) {
            return gsi_false;
        }
        return gsi_true;
    }
    total = length + 2;
    if (gti2GetBufferFreeSpace(&connection->outgoing) < total) {
        return gsi_true;
    }
    start = connection->outgoing.data;
    base = connection->outgoing.length;
    if (connection->socket->protocol == GTI2VdpProtocol) {
        gti2BufferWriteData(&connection->outgoing, data, 2);
    }
    gti2BufferWriteData(&connection->outgoing, "\xfe\xfe", 2);
    gti2BufferWriteData(&connection->outgoing, data + connection->socket->protocolOffset,
                        length - connection->socket->protocolOffset);
    if (!gti2ConnectionSendData(connection, start + base, total)) {
        return gsi_false;
    }
    gti2BufferShorten(&connection->outgoing, -1, total);
    return gsi_true;
}

GT2Bool gti2SendAck(GT2Connection connection) {
    gsi_u16 prefix;
    gsi_u8 packet[10];
    int length = 0;

    if (connection->socket->protocol == GTI2VdpProtocol) {
        prefix = 5;
        memcpy(packet, &prefix, 2);
        length += 2;
    }
    memcpy(packet + length, "\xfe\xfe", 2);
    packet[length + 2] = 0x64;
    gti2UShortToBuffer(packet, length + 3, connection->expectedSerialNumber);
    if (!gti2ConnectionSendData(connection, packet, length + 5)) {
        return gsi_false;
    }
    connection->pendingAck = gsi_false;
    return gsi_true;
}

// Nacks the serial numbers from first to last
static GT2Bool gti2SendNack(GT2Connection connection, unsigned short first, unsigned short last) {
    gsi_u16 prefix;
    gsi_u8 packet[10];
    int length = 0;

    if (connection->socket->protocol == GTI2VdpProtocol) {
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
        return gsi_false;
    }
    return gsi_true;
}

// Answers a ping with a pong
static GT2Bool gti2SendPong(GT2Connection connection, GT2Byte *data, int length) {
    data[2] = 0x67;
    return gti2ConnectionSendData(connection, data, length);
}

GT2Bool gti2SendClosed(GT2Connection connection) {
    return gti2SendClosedOnSocket(connection->socket, connection->ip, connection->port);
}

// Sends an address the note that its connection is closed
static GT2Bool gti2SendClosedOnSocket(GT2Socket socket, unsigned int ip, unsigned short port) {
    gsi_u16 prefix;
    gsi_u8 packet[5];
    int length = 0;

    if (socket->protocol == GTI2VdpProtocol) {
        prefix = 3;
        memcpy(packet, &prefix, 2);
        length += 2;
    }
    memcpy(packet + length, "\xfe\xfe", 2);
    packet[length + 2] = 0x68;
    if (!gti2SocketSend(socket, ip, port, packet, length + 3)) {
        return gsi_false;
    }
    return gsi_true;
}

GT2Bool gti2ResendMessage(GT2Connection connection, GTI2OutgoingBufferMessage *message) {
    gti2UShortToBuffer(connection->outgoing.data, message->offset + connection->socket->protocolOffset + 5,
                       connection->expectedSerialNumber);
    if (!gti2ConnectionSendData(connection, connection->outgoing.data + message->offset, message->length)) {
        return gsi_false;
    }
    message->sendTime = connection->lastSendTime;
    if (connection->outgoing.data[message->offset + connection->socket->protocolOffset + 2] == 2) {
        connection->challengeTime = connection->lastSendTime;
    }
    return gsi_true;
}

GT2Bool gti2Send(GT2Connection connection, const GT2Byte *data, int length, GT2Bool reliable) {
    if (reliable) {
        return gti2SendAppReliable(connection, data, length);
    }
    return gti2SendAppUnreliable(connection, data, length);
}
