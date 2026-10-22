#include "competition_internal.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"

// The competition client itself: making, setting up and ending it, the IDs of its session and its connection, and the
// URL of the service. The file and the names in it are GameSpy's own, as its sciInterface.c (UniSpySDK) has them:
// sciInterfaceCreate to sciInterfaceSetConnectionId are its functions in its order, and it defines scServiceURL. The
// ROM doesn't name the file. ScClient_SetServers and ScClient_BuildUrl are Nintendo's, and their names are ours.
// ScClient_BuildUrl shares sciInterfaceCreate's URL literal, as only functions of one file can, so it ends this file;
// ScClient_SetServers, before sciInterfaceCreate, could as well end dwc_sc.c, and is here with the URL it sets.

// The URL of the competition service, set by ScClient_SetServers or once it is asked for
char scServiceURL[128];

void ScClient_SetServers(int server) {
    switch (server) {
    case 1:
        OS_SNPrintf(scServiceURL, 128,
                    "https://%s.secure2.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(scServiceURL);
        OS_SNPrintf(wsAuthServiceURL, 128, "https://%s.secure2.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(wsAuthServiceURL);
        break;
    case 0:
    default:
        OS_SNPrintf(scServiceURL, 128,
                    "https://%s.secure.atlas.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(scServiceURL);
        OS_SNPrintf(wsAuthServiceURL, 128, "https://%s.secure.auth.gs.nintendowifi.net/AuthService/AuthService.asmx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(wsAuthServiceURL);
        break;
    }
}

int sciInterfaceCreate(SCInterface **client) {
    if (__GSIACResult == GSIACAvailable) {
        if (scServiceURL[0] == '\0') {
            OS_SNPrintf(scServiceURL, 128,
                        "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                        __GSIACGamename);
        }
    } else {
        return 1;
    }
    *client = gsimalloc(sizeof(SCInterface));
    if (*client == NULL) {
        return 5;
    }
    memset(*client, 0, sizeof(SCInterface));
    return 0;
}

int sciInterfaceInit(SCInterface *client) {
    int error = sciWsInit(client, client);

    if (error != 0) {
        return error;
    }
    memset(client->mSessionId, 0, sizeof(client->mSessionId));
    memset(client->mConnectionId, 0, sizeof(client->mConnectionId));
    client->valid = 1;
    return 0;
}

void sciInterfaceDestroy(SCInterface *client) {
    sciWsDestroy(client);
    memset(client->mSessionId, 0, sizeof(client->mSessionId));
    memset(client->mConnectionId, 0, sizeof(client->mConnectionId));
    // Invalid IDs carry the binary marker 0xDEAD.
    ((gsi_u8 *)client->mSessionId)[0] = 0xde;
    ((gsi_u8 *)client->mSessionId)[1] = 0xad;
    ((gsi_u8 *)client->mConnectionId)[0] = 0xde;
    ((gsi_u8 *)client->mConnectionId)[1] = 0xad;
    client->valid = 0;
}

void sciInterfaceSetSessionId(SCInterface *client, const char *csid) {
    if (csid == NULL) {
        client->mSessionId[0] = '\0';
    } else {
        strcpy(client->mSessionId, csid);
    }
}

void sciInterfaceSetConnectionId(SCInterface *client, const char *ccid) {
    if (ccid == NULL) {
        client->mConnectionId[0] = '\0';
    } else {
        strcpy(client->mConnectionId, ccid);
    }
}

char *ScClient_BuildUrl(void) {
    if (__GSIACResult == GSIACAvailable) {
        if (scServiceURL[0] == '\0') {
            OS_SNPrintf(scServiceURL, 128,
                        "http://%s.comp.pubsvs.gs.nintendowifi.net/CompetitionService/CompetitionService.asmx",
                        __GSIACGamename);
        }
        return scServiceURL;
    }
    gsDebugFormat(GSIDebugCat_App, GSIDebugType_State, GSIDebugLevel_HotError, "No availability\r\n");
    return NULL;
}
