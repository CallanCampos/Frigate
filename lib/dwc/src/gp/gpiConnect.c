#include "dwc/gpiConnect.h"
#include "dwc/ext.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiOperation.h"
#include "dwc/gpiPeer.h"
#include "dwc/gpiProfile.h"
#include "dwc/gpiUtility.h"
#include "dwc/gt2.h"
#include "dwc/md5.h"

// GameSpy GP's login to the connection manager (gpiConnect.c). The functions' names, from pret/pokerevo's Wii build,
// are in dwc/gp.h

// The port that the Udp Engine is first tried on, and the connection manager's port (29900, in network order)
#define GPI_PEER_PORT 6500
#define GPI_CONNECTION_MANAGER_PORT 0xcc74

static void randomString(char *buffer, int numChars);
static GPResult gpiStartConnect(GPConnection *connection, GPIOperation *operation);
static GPResult gpiSendLogin(GPConnection *connection, GPIConnectData *data);
static GPResult gpiSendNewuser(GPConnection *connection, GPIConnectData *data);
static GPIBool gpiDisconnectCleanupProfile(GPConnection *connection, GPIProfile *profile, void *data);

// The host of the connection manager, and the characters of randomString's challenges, which are defined after
// randomString, as their order in .data shows
static char sConnectionManagerHostname[64] = "gpcm.gs.nintendowifi.net";
static char sCharacters[63];

static void randomString(char *buffer, int numChars) {
    int i;

    for (i = 0; i < numChars; i++) {
        buffer[i] = sCharacters[rand() % (sizeof(sCharacters) - 1)];
    }
    buffer[i] = '\0';
}

static char sCharacters[63] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static GPResult gpiStartConnect(GPConnection *connection, GPIOperation *operation) {
    GPIConnection *iconnection = *connection;
    gsi_u16 port;
    int result;
    int error;

    strncpy(iconnection->mHeader, "gamespygp", 0x10);
    if (!Gt2_IsInitialized()) {
        port = GPI_PEER_PORT;
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice, "Initializing UDP Layer\n");
        result = Gt2_CreateSocket(port, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (result != 0) {
            while (result != 0 && port < GPI_PEER_PORT + 100) {
                gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Comment,
                              "Port %d failed, trying next port\n", port);
                port++;
                result = Gt2_CreateSocket(port, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            }
            if (result != 0) {
                gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError,
                              "Tryed all 100 ports after default port, giving up.\n");
                gpiSetError(connection, 8, "There was error starting the UDP layer.");
                gpiCallErrorCallback(connection, 3, 1);
                return 3;
            }
        }
        if (!iconnection->firewall) {
            iconnection->peerPort = port;
        }
    } else {
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "UDP Layer already initialized, using existing port.\n");
        iconnection->peerPort = Gt2_GetLocalPort();
    }
    if (Gt2_Connect(iconnection->mHeader, iconnection->mHeader, NULL, gpiPeerAcceptedCallback, gpiPeerLeftCallback,
                    gpiPeerPingReplyCallback, gpiPeerMessageCallback, connection) != 0) {
        gpiSetError(connection, 8, "There was an error starting the UDP Layer.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    if (iconnection->firewall) {
        iconnection->peerPort = 0;
    }
    iconnection->cmSocket = gsiSocket(2, 1, 0);
    if (iconnection->cmSocket == -1) {
        gpiSetError(connection, 5, "There was an error creating a socket.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    if (!SetSockBlocking(iconnection->cmSocket, 0)) {
        gpiSetError(connection, 5, "There was an error making a socket non-blocking.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    SOCSockAddrIn address = { 0 };
    address.family = 2;
    if (inet_addr(sConnectionManagerHostname) == -1) {
        SOCHostEnt *host = SOC_GetHostByName(sConnectionManagerHostname);
        if (host == NULL) {
            gpiSetError(connection, 5, "Could not resolve connection mananger host name.");
            gpiCallErrorCallback(connection, 3, 1);
            return 3;
        }
        address.addr.addr = *(gsi_u32 *)host->addrList[0];
    } else {
        address.addr.addr = inet_addr(sConnectionManagerHostname);
    }
    address.port = GPI_CONNECTION_MANAGER_PORT;
    if (connect(iconnection->cmSocket, (SOCKADDR *)&address, sizeof(address)) == -1) {
        error = GOAGetLastError(iconnection->cmSocket);
        if (error != -6 && error != -26 && error != -76) {
            gpiSetError(connection, 5, "There was an error connecting a socket.");
            gpiCallErrorCallback(connection, 3, 1);
            return 3;
        }
    }
    operation->state = GPI_CONNECTING;
    iconnection->connectState = GPI_CONNECTING;
    return 0;
}

GPResult gpiConnect(GPConnection *connection, const char nick[GP_NICK_LEN], const char uniquenick[GP_UNIQUENICK_LEN],
                    const char email[GP_EMAIL_LEN], const char password[GP_PASSWORD_LEN],
                    const char authtoken[GP_AUTHTOKEN_LEN], const char partnerchallenge[GP_PARTNERCHALLENGE_LEN],
                    const char cdkey[GP_CDKEY_LEN], GPEnum firewall, GPIBool newuser, GPEnum blocking,
                    GPCallback callback, void *param) {
    GPIConnection *iconnection = *connection;
    GPIConnectData *data;
    GPIOperation *operation;
    int result;

    if (iconnection->connectState == GPI_DISCONNECTED) {
        result = gpiReset(connection);
        if (result != 0) {
            return result;
        }
    }
    if (iconnection->connectState != GPI_NOT_CONNECTED) {
        gpiSetErrorString(connection, "Invalid connection.");
        return 2;
    }
    iconnection->firewall = gsi_true;
    strzcpy(iconnection->nick, nick, 0x1f);
    strzcpy(iconnection->uniquenick, uniquenick, 0x15);
    strzcpy(iconnection->email, email, 0x33);
    strzcpy(iconnection->password, password, 0x1f);
    _strlwr(iconnection->email);
    data = gsimalloc(sizeof(GPIConnectData));
    if (data == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    memset(data, 0, sizeof(GPIConnectData));
    data->newuser = newuser;
    if (authtoken[0] != '\0' && partnerchallenge[0] != '\0') {
        strzcpy(data->authtoken, authtoken, 0x100);
        strzcpy(data->partnerchallenge, partnerchallenge, 0x100);
    }
    if (cdkey != NULL) {
        strzcpy(data->cdkey, cdkey, 0x41);
    }
    result = gpiAddOperation(connection, GPI_CONNECT, data, &operation, blocking, callback, param);
    if (result != 0) {
        return result;
    }
    result = gpiStartConnect(connection, operation);
    if (result != 0) {
        operation->result = result;
        gpiFailedOpCallback(connection, operation);
        gpiDisconnect(connection, gsi_false);
        return result;
    }
    if (operation->blocking) {
        result = gpiProcess(connection, operation->id);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

static GPResult gpiSendLogin(GPConnection *connection, GPIConnectData *data) {
    GPIConnection *iconnection = *connection;
    const char *passphrase;
    char *user;
    char buffer[0x200];
    char userBuffer[0x52];
    char response[0x21];
    char partnerBuffer[0xb];
    GPIProfile *profile;

    randomString(data->userChallenge, 0x20);
    if (data->partnerchallenge[0] != '\0') {
        passphrase = data->partnerchallenge;
    } else {
        passphrase = iconnection->password;
    }
    MD5Digest((unsigned char *)passphrase, strlen(passphrase), data->passwordHash);
    if (iconnection->partnerID != 0) {
        OS_SPrintf(partnerBuffer, "%d@", iconnection->partnerID);
    } else {
        partnerBuffer[0] = '\0';
    }
    if (data->authtoken[0] != '\0') {
        user = data->authtoken;
    } else if (iconnection->uniquenick[0] != '\0') {
        OS_SPrintf(userBuffer, "%s%s", partnerBuffer, iconnection->uniquenick);
        user = userBuffer;
    } else {
        OS_SPrintf(userBuffer, "%s%s@%s", partnerBuffer, iconnection->nick, iconnection->email);
        user = userBuffer;
    }
    OS_SPrintf(buffer, "%s%s%s%s%s%s", data->passwordHash, "                                                ", user,
               data->userChallenge, data->serverChallenge, data->passwordHash);
    MD5Digest((unsigned char *)buffer, strlen(buffer), response);
    if (iconnection->infoCaching) {
        gpiFindProfileByUser(connection, iconnection->nick, iconnection->email, &profile);
        if (profile != NULL) {
            iconnection->userid = profile->userId;
            iconnection->profileid = profile->profileId;
        }
    }
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\login\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\challenge\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, data->userChallenge);
    if (data->authtoken[0] != '\0') {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\authtoken\\");
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, data->authtoken);
    } else if (iconnection->uniquenick[0] != '\0') {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\uniquenick\\");
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->uniquenick);
    } else {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\user\\");
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->nick);
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "@");
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->email);
    }
    if (iconnection->userid != 0) {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\userid\\");
        gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->userid);
    }
    if (iconnection->profileid != 0) {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
        gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->profileid);
    }
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\partnerid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->partnerID);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\response\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, response);
    if (iconnection->firewall == gsi_true) {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\firewall\\1");
    }
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\port\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->peerPort);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\productid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->productID);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\gamename\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, __GSIACGamename);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\namespaceid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->namespaceID);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sdkrevision\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, 0x3b);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\quiet\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->quietModeFlags);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\1");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    return 0;
}

static GPResult gpiSendNewuser(GPConnection *connection, GPIConnectData *data) {
    GPIConnection *iconnection = *connection;
    char cdkeyxor[0x41];
    char cdkeyenc[0x5a];
    char passwordenc[0x2d];
    gsi_u32 cdkeylen;
    gsi_u32 i;

    gpiEncodeString(iconnection->password, passwordenc);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\newuser\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\email\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->email);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\nick\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->nick);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\passwordenc\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, passwordenc);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\productid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->productID);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\gamename\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, __GSIACGamename);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\namespaceid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->namespaceID);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\uniquenick\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->uniquenick);
    if (data->cdkey[0] != '\0') {
        // The CD key is scrambled as gpiEncodeString scrambles a password
        cdkeylen = strlen(data->cdkey);
        Util_RandSeed(0x79707367);
        for (i = 0; i < cdkeylen; i++) {
            cdkeyxor[i] = data->cdkey[i] ^ (char)Util_RandInt(0, 0xff);
        }
        cdkeyxor[i] = '\0';
        B64Encode(cdkeyxor, cdkeyenc, cdkeylen, 1);
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\cdkeyenc\\");
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, cdkeyenc);
    }
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\partnerid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->partnerID);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\1");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    return 0;
}

// What a message that doesn't make sense gives
#define GPI_UNEXPECTED_DATA(connection, text)                                                                          \
    do {                                                                                                               \
        gpiSetError(connection, 1, text);                                                                              \
        gpiCallErrorCallback(connection, 3, 1);                                                                        \
        return 3;                                                                                                      \
    } while (0)

GPResult gpiProcessConnect(GPConnection *connection, GPIOperation *operation, const char *input) {
    GPIConnection *iconnection = *connection;
    GPIConnectData *data;
    GPIProfile *profile;
    GPICallback callback;
    GPConnectResponseArg *arg;
    char buffer[0x200];
    char userBuffer[0x52];
    char check[0x21];
    char uniquenick[0x15];
    char partnerBuffer[0xb];
    char *user;
    int result;
    int errorCode;

    if (gpiCheckForError(connection, input, gsi_false)) {
        errorCode = iconnection->errorCode;
        if (errorCode == 0x106 && iconnection->profileid != 0) {
            gpiRemoveProfileByID(connection, iconnection->profileid);
            iconnection->userid = 0;
            iconnection->profileid = 0;
        } else if (errorCode == 0x201) {
            if (gpiValueForKey(input, "\\pid\\", buffer, 0x200)) {
                iconnection->profileid = atol(buffer);
            }
        }
        gpiSetError(connection, iconnection->errorCode, iconnection->errorString);
        gpiCallErrorCallback(connection, 4, 1);
        return 4;
    }
    data = operation->data;
    switch (operation->state) {
    case GPI_CONNECTING:
        if (strncmp(input, "\\lc\\1", 5) != 0) {
            GPI_UNEXPECTED_DATA(connection, "Unexpected data was received from the server.");
        }
        if (!gpiValueForKey(input, "\\challenge\\", data->serverChallenge, 0x80)) {
            GPI_UNEXPECTED_DATA(connection, "Unexpected data was received from the server.");
        }
        if (data->newuser) {
            result = gpiSendNewuser(connection, data);
            if (result != 0) {
                return result;
            }
            operation->state = GPI_REQUESTING;
        } else {
            result = gpiSendLogin(connection, data);
            if (result != 0) {
                return result;
            }
            operation->state = GPI_LOGIN;
        }
        break;
    case GPI_REQUESTING:
        if (strncmp(input, "\\nur\\", 5) != 0) {
            GPI_UNEXPECTED_DATA(connection, "Unexpected data was received from the server.");
        }
        if (!gpiValueForKey(input, "\\userid\\", buffer, 0x200)) {
            GPI_UNEXPECTED_DATA(connection, "Unexepected data was received from the server.");
        }
        iconnection->userid = atol(buffer);
        if (!gpiValueForKey(input, "\\profileid\\", buffer, 0x200)) {
            GPI_UNEXPECTED_DATA(connection, "Unexepected data was received from the server.");
        }
        iconnection->profileid = atol(buffer);
        result = gpiSendLogin(connection, data);
        if (result != 0) {
            return result;
        }
        operation->state = GPI_LOGIN;
        break;
    case GPI_LOGIN:
        if (strncmp(input, "\\lc\\2", 5) != 0) {
            GPI_UNEXPECTED_DATA(connection, "Unexpected data was received from the server.");
        }
        if (!gpiValueForKey(input, "\\sesskey\\", buffer, 0x200)) {
            GPI_UNEXPECTED_DATA(connection, "Unexepected data was received from the server.");
        }
        iconnection->sessKey = atol(buffer);
        if (!gpiValueForKey(input, "\\userid\\", buffer, 0x200)) {
            GPI_UNEXPECTED_DATA(connection, "Unexepected data was received from the server.");
        }
        iconnection->userid = atol(buffer);
        if (!gpiValueForKey(input, "\\profileid\\", buffer, 0x200)) {
            GPI_UNEXPECTED_DATA(connection, "Unexepected data was received from the server.");
        }
        iconnection->profileid = atol(buffer);
        if (!gpiValueForKey(input, "\\uniquenick\\", uniquenick, 0x15)) {
            uniquenick[0] = '\0';
        }
        if (!gpiValueForKey(input, "\\lt\\", iconnection->loginTicket, 0x19)) {
            iconnection->loginTicket[0] = '\0';
        }
        if (iconnection->partnerID != 0) {
            OS_SPrintf(partnerBuffer, "%d@", iconnection->partnerID);
        } else {
            partnerBuffer[0] = '\0';
        }
        if (data->authtoken[0] != '\0') {
            user = data->authtoken;
        } else if (iconnection->uniquenick[0] != '\0') {
            OS_SPrintf(userBuffer, "%s%s", partnerBuffer, iconnection->uniquenick);
            user = userBuffer;
        } else {
            OS_SPrintf(userBuffer, "%s%s@%s", partnerBuffer, iconnection->nick, iconnection->email);
            user = userBuffer;
        }
        // The server proves that it knows the password with the challenges the other way around
        OS_SPrintf(buffer, "%s%s%s%s%s%s", data->passwordHash, "                                                ", user,
                   data->serverChallenge, data->userChallenge, data->passwordHash);
        MD5Digest((unsigned char *)buffer, strlen(buffer), check);
        if (!gpiValueForKey(input, "\\proof\\", buffer, 0x200)) {
            GPI_UNEXPECTED_DATA(connection, "Unexepected data was received from the server.");
        }
        if (memcmp(check, buffer, 0x20) != 0) {
            gpiSetError(connection, 0x108, "Could not authenticate server.");
            gpiCallErrorCallback(connection, 3, 1);
            return 3;
        }
        if (iconnection->infoCaching) {
            profile = gpiProfileListAdd(connection, iconnection->profileid);
            profile->profileId = iconnection->profileid;
            profile->userId = iconnection->userid;
        }
        iconnection->connectState = GPI_CONNECTED;
        callback = operation->callback;
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPConnectResponseArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPConnectResponseArg));
            arg->profile = iconnection->profileid;
            arg->result = 0;
            strzcpy(arg->uniquenick, uniquenick, 0x15);
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
        }
        gpiRemoveOperation(connection, operation);
        break;
    }
    return 0;
}

GPResult gpiCheckConnect(GPConnection *connection) {
    GPIConnection *iconnection = *connection;
    int state;
    int result;

    result = gpiCheckSocketConnect(connection, iconnection->cmSocket, &state);
    if (result != 0) {
        return result;
    }
    if (state == 4) {
        gpiSetError(connection, 0x107, "The server has refused the connection.");
        gpiCallErrorCallback(connection, 4, 1);
        return 4;
    }
    if (state == 0) {
        return 0;
    }
    iconnection->connectState = GPI_NEGOTIATING;
    return 0;
}

static GPIBool gpiDisconnectCleanupProfile(GPConnection *connection, GPIProfile *profile, void *data) {
    GPIConnection *iconnection = *connection;

    if (profile->buddyStatus != NULL) {
        profile->buddyOrBlockCache = gsi_true;
        gsifree(profile->buddyStatus->statusString);
        profile->buddyStatus->statusString = NULL;
        gsifree(profile->buddyStatus->locationString);
        profile->buddyStatus->locationString = NULL;
        gsifree(profile->buddyStatus);
        profile->buddyStatus = NULL;
    }
    if (profile->buddyStatusInfo != NULL) {
        profile->buddyOrBlockCache = gsi_true;
        gsifree(profile->buddyStatusInfo->richStatus);
        profile->buddyStatusInfo->richStatus = NULL;
        gsifree(profile->buddyStatusInfo->gameType);
        profile->buddyStatusInfo->gameType = NULL;
        gsifree(profile->buddyStatusInfo->gameVariant);
        profile->buddyStatusInfo->gameVariant = NULL;
        gsifree(profile->buddyStatusInfo->gameMapName);
        profile->buddyStatusInfo->gameMapName = NULL;
        if (profile->buddyStatusInfo->extendedInfoKeys != NULL) {
            func_ov011_02171558(profile->buddyStatusInfo->extendedInfoKeys);
            profile->buddyStatusInfo->extendedInfoKeys = NULL;
        }
        gsifree(profile->buddyStatusInfo);
        profile->buddyStatusInfo = NULL;
    }
    if (profile->blocked) {
        profile->buddyOrBlockCache = gsi_true;
    }
    gsifree(profile->authSig);
    profile->authSig = NULL;
    gsifree(profile->peerSig);
    profile->peerSig = NULL;
    profile->requestCount = 0;
    if (profile->cache == NULL ||
        (iconnection->infoCachingBuddyAndBlockOnly == gsi_true && !profile->buddyOrBlockCache)) {
        gpiRemoveProfile(connection, profile);
        return gsi_false;
    }
    return gsi_true;
}

void gpiDisconnect(GPConnection *connection, GPIBool tellServer) {
    GPIConnection *iconnection = *connection;
    GPIBool connClosed;

    if (iconnection->connectState == GPI_DISCONNECTED) {
        return;
    }
    if (iconnection->connectState != GPI_NOT_CONNECTED) {
        if (tellServer && iconnection->connectState == GPI_CONNECTED) {
            gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\logout\\\\sesskey\\");
            gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
            gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
        }
        gpiSendFromBuffer(connection, iconnection->cmSocket, &iconnection->outputBuffer, &connClosed, gsi_true, "CM");
        if (iconnection->cmSocket != -1) {
            shutdown(iconnection->cmSocket, 2);
            closesocket(iconnection->cmSocket);
            iconnection->cmSocket = -1;
        }
        iconnection->connectState = GPI_DISCONNECTED;
        iconnection->userid = 0;
        iconnection->profileid = 0;
    }
    if (Gt2_IsInitialized()) {
        Gt2_CancelConnect(iconnection->mHeader);
        if (Gt2_IsIdle() && Gt2_HasNoAppCallbacks()) {
            Gt2_CloseSocket();
        }
    }
    gsifree(iconnection->socketBuffer.buffer);
    iconnection->socketBuffer.buffer = NULL;
    gsifree(iconnection->inputBuffer);
    iconnection->inputBuffer = NULL;
    gsifree(iconnection->outputBuffer.buffer);
    iconnection->outputBuffer.buffer = NULL;
    gsifree(iconnection->updateproBuffer.buffer);
    iconnection->updateproBuffer.buffer = NULL;
    gsifree(iconnection->updateuiBuffer.buffer);
    iconnection->updateuiBuffer.buffer = NULL;
    while (iconnection->operationList != NULL) {
        gpiRemoveOperation(connection, iconnection->operationList);
    }
    iconnection->operationList = NULL;
    {
        GPIPeer *peer = iconnection->peerList;

        if (peer != NULL) {
            do {
                GPIPeer *delPeer = peer;

                peer = peer->pnext;
                gpiDestroyPeer(connection, delPeer);
            } while (peer != NULL);
        }
    }
    iconnection->peerList = NULL;
    while (!gpiProfileMap(connection, gpiDisconnectCleanupProfile, NULL)) {
    }
}

int gpiProcessRemoteAuthReply(GPConnection *connection, const char *input) {
    GPIConnection *iconnection = *connection;
    char buffer[0x200];

    if (gpiCheckForError(connection, input, gsi_true)) {
        return 4;
    }
    if (!gpiValueForKey(iconnection->inputBuffer, "\\namespaceid\\", buffer, 0x200)) {
        GPI_UNEXPECTED_DATA(connection, "Unexpected data was received from the server.");
    }
    iconnection->namespaceID = atol(buffer);
    if (!gpiValueForKey(iconnection->inputBuffer, "\\partnerid\\", buffer, 0x200)) {
        GPI_UNEXPECTED_DATA(connection, "Unexpected data was received from the server.");
    }
    iconnection->partnerID = atol(buffer);
    return 0;
}
