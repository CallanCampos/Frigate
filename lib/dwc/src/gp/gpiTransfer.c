#include "dwc/gpiTransfer.h"
#include "dwc/ext.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiPeer.h"
#include "dwc/gpiUtility.h"
#include "nitro/std.h"

// What is left of GameSpy GP's file transfers (gpiTransfer.c). The functions' names, from pret/pokerevo's Wii build and
// GameSpy's later SDK, are in dwc/gp.h

static int gpiSendTransferReply(GPConnection *connection, const GPITransferID *transferID, GPIPeer *peer, int result,
                                const char *msg);

static int gpiSendTransferReply(GPConnection *connection, const GPITransferID *transferID, GPIPeer *peer, int result,
                                const char *msg) {
    char buffer[32];
    int status;

    if (msg == NULL) {
        msg = "";
    }
    status = gpiPeerStartTransferMessage(connection, peer, GPI_BM_FILE_SEND_REPLY, transferID);
    if (status != 0) {
        return status;
    }
    OS_SPrintf(buffer, "\\version\\%d\\result\\%d", 1, result);
    status = gpiSendOrBufferString(connection, peer, buffer);
    if (status != 0) {
        return status;
    }
    status = gpiPeerFinishTransferMessage(connection, peer, msg, -1);
    if (status != 0) {
        return status;
    }
    return 0;
}

// A transfer message names its transfer in "\xfer\<profile> <count> <time>"; the transfer is refused (2)
void gpiHandleTransferMessage(GPConnection *connection, GPIPeer *peer, int type, const char *headers,
                              const char *buffer, int len) {
    char value[0x40];
    GPITransferID transferID;

    if (gpiValueForKey(headers, "\\xfer\\", value, 0x40)) {
        if (STD_TSScanf(value, "%d %u %u", &transferID.profileid, &transferID.count, &transferID.time) == 3) {
            gpiSendTransferReply(connection, &transferID, peer, 2, NULL);
        }
    }
}
