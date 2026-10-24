#include <string.h>
// Before dwc/ext.h: MWCC builds gpiProcessSearch's memset of a GPUniqueMatch in place, as the ROM does, only when
// <string.h> declares memset first
#include "dwc/ext.h"
#include "dwc/gpi.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiOperation.h"
#include "dwc/gpiSearch.h"
#include "dwc/gpiUtility.h"
#include "nitrowifi/soc.h"

// GameSpy GP's searches (gpiSearch.c), each on its own socket to the search manager, which answers once and closes. The
// functions' names, from pret/pokerevo's Wii build and GameSpy's later SDK, are in dwc/gp.h

static GPResult gpiStartProfileSearch(GPConnection *connection, GPIOperation *operation);
static GPResult gpiInitSearchData(GPConnection *connection, GPISearchData **searchData, int type);
static GPResult gpiStartSearch(GPConnection *connection, GPISearchData *data, GPEnum blocking, GPCallback callback,
                               void *param);
static GPResult gpiProcessSearch(GPConnection *connection, GPIOperation *operation);

// The host of the search manager
static char sSearchManagerHostname[64] = "gpsp.gs.nintendowifi.net";

// Opens the search's socket to the search manager, which gpiProcessSearch waits on to connect
static GPResult gpiStartProfileSearch(GPConnection *connection, GPIOperation *operation) {
    GPISearchData *search = operation->data;
    SOCHostEnt *host;
    int error;

    search->inputBuffer.size = 0x1000;
    search->inputBuffer.buffer = gsimalloc(0x1001);
    if (search->inputBuffer.buffer == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    search->sock = socket(2, 1, 0);
    if (search->sock == -1) {
        gpiSetError(connection, 5, "There was an error creating a socket.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    if (SetSockBlocking(search->sock, 0) == 0) {
        gpiSetError(connection, 5, "There was an error making a socket non-blocking.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    host = SOC_GetHostByName(sSearchManagerHostname);
    if (host == NULL) {
        gpiSetError(connection, 5, "Could not resolve search mananger host name.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    {
        SOCSockAddrIn address = { 0 };

        address.family = 2;
        address.addr.addr = *(gsi_u32 *)host->addrList[0];
        // Port 29901, in network order
        address.port = 0xcd74;
        if (connect(search->sock, (SOCKADDR *)&address, 8) == -1) {
            error = GOAGetLastError(search->sock);
            if (error != -6 && error != -26 && error != -76) {
                gpiSetError(connection, 5, "There was an error connecting a socket.");
                gpiCallErrorCallback(connection, 3, 1);
                return 3;
            }
        }
    }
    operation->state = GPI_CONNECTING;
    search->searchStartTime = current_time();
    return 0;
}

static GPResult gpiInitSearchData(GPConnection *connection, GPISearchData **searchData, int type) {
    GPISearchData *data = gsimalloc(sizeof(GPISearchData));

    if (data == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    memset(data, 0, sizeof(GPISearchData));
    data->type = type;
    data->sock = -1;
    data->inputBuffer.buffer = NULL;
    data->inputBuffer.len = 0;
    data->inputBuffer.pos = 0;
    data->inputBuffer.size = 0;
    data->outputBuffer.len = 0;
    data->outputBuffer.pos = 0;
    data->outputBuffer.size = 0x1000;
    data->outputBuffer.buffer = gsimalloc(0x1001);
    if (data->outputBuffer.buffer == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    data->processing = 0;
    data->remove = 0;
    *searchData = data;
    return 0;
}

// Starts a search as an operation, and runs it to its end if it blocks
static GPResult gpiStartSearch(GPConnection *connection, GPISearchData *data, GPEnum blocking, GPCallback callback,
                               void *param) {
    GPIOperation *operation;
    int result;

    ((GPIConnection *)*connection)->numSearches++;
    result = gpiAddOperation(connection, GPI_PROFILE_SEARCH, data, &operation, blocking, callback, param);
    if (result != 0) {
        return result;
    }
    result = gpiStartProfileSearch(connection, operation);
    if (result != 0) {
        return result;
    }
    if (operation->blocking != 0) {
        result = gpiProcess(connection, operation->id);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

GPResult gpiProfileSearch(GPConnection *connection, const char nick[GP_NICK_LEN],
                          const char uniquenick[GP_UNIQUENICK_LEN], const char email[GP_EMAIL_LEN],
                          const char firstname[GP_FIRSTNAME_LEN], const char lastname[GP_LASTNAME_LEN], int icquin,
                          int skip, GPEnum blocking, GPCallback callback, void *param) {
    GPISearchData *search;
    int result;

    if ((nick == NULL || nick[0] == '\0') && (email == NULL || email[0] == '\0') &&
        (firstname == NULL || firstname[0] == '\0') && (lastname == NULL || lastname[0] == '\0') && icquin == 0 &&
        (uniquenick == NULL || uniquenick[0] == '\0')) {
        gpiSetErrorString(connection, "No search criteria.");
        return 2;
    }
    result = gpiInitSearchData(connection, &search, GPI_SEARCH_PROFILE);
    if (result != 0) {
        return result;
    }
    if (nick == NULL) {
        search->nick[0] = '\0';
    } else {
        strzcpy(search->nick, nick, sizeof(search->nick));
    }
    if (uniquenick == NULL) {
        search->uniquenick[0] = '\0';
    } else {
        strzcpy(search->uniquenick, uniquenick, sizeof(search->uniquenick));
    }
    if (email == NULL) {
        search->email[0] = '\0';
    } else {
        strzcpy(search->email, email, sizeof(search->email));
    }
    _strlwr(search->email);
    if (firstname == NULL) {
        search->firstname[0] = '\0';
    } else {
        strzcpy(search->firstname, firstname, sizeof(search->firstname));
    }
    if (lastname == NULL) {
        search->lastname[0] = '\0';
    } else {
        strzcpy(search->lastname, lastname, sizeof(search->lastname));
    }
    search->icquin = icquin;
    if (skip < 0) {
        skip = 0;
    }
    search->skip = skip;
    result = gpiStartSearch(connection, search, blocking, callback, param);
    if (result != 0) {
        return result;
    }
    return 0;
}

GPResult gpiOthersBuddyList(GPConnection *connection, int *profiles, int numOfProfiles, GPEnum blocking,
                            GPCallback callback, void *param) {
    GPISearchData *search;
    int result;

    result = gpiInitSearchData(connection, &search, GPI_SEARCH_OTHERS_BUDDY_LIST);
    if (result != 0) {
        return result;
    }
    search->revBuddyProfileIds = profiles;
    search->numOfRevBuddyProfiles = numOfProfiles;
    result = gpiStartSearch(connection, search, blocking, callback, param);
    if (result != 0) {
        return result;
    }
    return 0;
}

// Moves a search on: once connected it sends the request, and once the answer has ended ("\\final\\") it reads it
// into the argument of the app's callback. A search that doesn't block gives up after a minute, and one that blocks
// loops until it is done
static GPResult gpiProcessSearch(GPConnection *connection, GPIOperation *operation) {
    int state;
    GPISearchData *search;
    char key[512];
    char value[512];
    GPIBool done;
    int index;
    int oldIndex;
    GPIBool loop;
    GPEnum more;
    GPICallback callback;
    GPIConnection *iconnection = *connection;
    int bytesRead;
    GPIBool connClosed;
    GPResult result;
    void *tempPtr;
    GPIBool doneParsingMatch;
    int rcode;
    int pid;
    GPProfileSearchMatch *match;
    GPUniqueMatch *uniqueMatch;
    char encoded[45];

    search = operation->data;
    if (operation->blocking) {
        loop = GPITrue;
    } else {
        loop = GPIFalse;
    }
    if (!operation->blocking && current_time() - search->searchStartTime > 60000) {
        search->remove = 1;
        gpiSetError(connection, 0xd02, "The search timed out");
        gpiCallErrorCallback(connection, 3, 0);
        return 3;
    }
    do {
        result = gpiSendFromBuffer(connection, search->sock, &search->outputBuffer, &connClosed, 1, "SM");
        if (result != 0) {
            return result;
        }
        if (operation->state == GPI_CONNECTING) {
            result = gpiCheckSocketConnect(connection, search->sock, &state);
            if (result != 0) {
                return result;
            }
            if (state == 4) {
                gpiSetError(connection, 0xd01, "Could not connect to the search manager.");
                gpiCallErrorCallback(connection, 4, 0);
                return 4;
            }
            if (state == 3) {
                if (search->type == GPI_SEARCH_PROFILE) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\search\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\sesskey\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->sessKey);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\profileid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->profileid);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\namespaceid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->namespaceID);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\partnerid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->partnerID);
                    if (search->nick[0] != '\0') {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\nick\\");
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, search->nick);
                    }
                    if (search->uniquenick[0] != '\0') {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\uniquenick\\");
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, search->uniquenick);
                    }
                    if (search->email[0] != '\0') {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\email\\");
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, search->email);
                    }
                    if (search->firstname[0] != '\0') {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\firstname\\");
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, search->firstname);
                    }
                    if (search->lastname[0] != '\0') {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\lastname\\");
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, search->lastname);
                    }
                    if (search->icquin != 0) {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\icquin\\");
                        gpiAppendIntToBuffer(connection, &search->outputBuffer, search->icquin);
                    }
                    if (search->skip > 0) {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\skip\\");
                        gpiAppendIntToBuffer(connection, &search->outputBuffer, search->skip);
                    }
                } else if (search->type == GPI_SEARCH_PROFILE_UNIQUENICK) {
                    int i;

                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\searchunique\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\sesskey\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->sessKey);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\profileid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->profileid);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\uniquenick\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->uniquenick);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\namespaces\\");
                    if (search->numNamespaces > 0) {
                        for (i = 0; i < search->numNamespaces; i++) {
                            if (i > 0) {
                                gpiAppendCharToBuffer(connection, &search->outputBuffer, ',');
                            }
                            gpiAppendIntToBuffer(connection, &search->outputBuffer, search->namespaceIDs[i]);
                        }
                    } else {
                        gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->namespaceID);
                    }
                } else if (search->type == GPI_SEARCH_IS_VALID) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\valid\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\email\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->email);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\partnerid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->partnerID);
                } else if (search->type == GPI_SEARCH_NICKS) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\nicks\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\email\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->email);
                    gpiEncodeString(search->password, encoded);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\passenc\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, encoded);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\namespaceid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->namespaceID);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\partnerid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->partnerID);
                } else if (search->type == GPI_SEARCH_PLAYERS) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\pmatch\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\sesskey\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->sessKey);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\profileid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->profileid);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\productid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, search->productID);
                } else if (search->type == GPI_SEARCH_CHECK) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\check\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\nick\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->nick);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\email\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->email);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\partnerid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->partnerID);
                    gpiEncodeString(search->password, encoded);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\passenc\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, encoded);
                } else if (search->type == GPI_SEARCH_NEWUSER) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\newuser\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\nick\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->nick);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\email\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->email);
                    gpiEncodeString(search->password, encoded);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\passenc\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, encoded);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\productID\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->productID);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\namespaceid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->namespaceID);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\uniquenick\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->uniquenick);
                    if (search->cdkey[0] != '\0') {
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\cdkey\\");
                        gpiAppendStringToBuffer(connection, &search->outputBuffer, search->cdkey);
                    }
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\partnerid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->partnerID);
                } else if (search->type == GPI_SEARCH_OTHERS_BUDDY) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\others\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\sesskey\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->sessKey);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\profileid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->profileid);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\namespaceid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->namespaceID);
                } else if (search->type == GPI_SEARCH_OTHERS_BUDDY_LIST) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\otherslist\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\sesskey\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->sessKey);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\profileid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->profileid);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\numopids\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, search->numOfRevBuddyProfiles);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\opids\\");
                    if (search->revBuddyProfileIds != NULL) {
                        int i;

                        gpiAppendIntToBuffer(connection, &search->outputBuffer, search->revBuddyProfileIds[0]);
                        for (i = 1; i < search->numOfRevBuddyProfiles; i++) {
                            gpiAppendStringToBuffer(connection, &search->outputBuffer, "|");
                            gpiAppendIntToBuffer(connection, &search->outputBuffer, search->revBuddyProfileIds[i]);
                        }
                    }
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\namespaceid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->namespaceID);
                } else if (search->type == GPI_SEARCH_SUGGEST_UNIQUE) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\uniquesearch\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\preferrednick\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, search->uniquenick);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\namespaceid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->namespaceID);
                } else if (search->type == GPI_SEARCH_PROFILE_LIST) {
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\profilelist\\");
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\sesskey\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->sessKey);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\profileid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, iconnection->profileid);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\searchprofileid\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, search->profile);
                    gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\maxresults\\");
                    gpiAppendIntToBuffer(connection, &search->outputBuffer, search->maxResults);
                }
                gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\gamename\\");
                gpiAppendStringToBuffer(connection, &search->outputBuffer, __GSIACGamename);
                gpiAppendStringToBuffer(connection, &search->outputBuffer, "\\final\\");
                operation->state = GPI_WAITING;
            }
        } else if (operation->state == GPI_WAITING) {
            result = gpiRecvToBuffer(connection, search->sock, &search->inputBuffer, &bytesRead, &connClosed, "SM");
            if (result != 0) {
                if (result == 3) {
                    gpiSetError(connection, 0xd01, "There was an error reading from the server.");
                    gpiCallErrorCallback(connection, 3, 0);
                    return 3;
                }
                return result;
            }
            if (operation->blocking && current_time() - search->searchStartTime > 60000) {
                search->remove = 1;
                gpiSetError(connection, 0xd02, "The search timed out");
                gpiCallErrorCallback(connection, 3, 0);
                return 3;
            }
            if (strstr(search->inputBuffer.buffer, "\\final\\") != NULL) {
                index = 0;
                operation->state = GPI_FINISHING;
                if (gpiCheckForError(connection, search->inputBuffer.buffer, GPITrue)) {
                    search->remove = 1;
                    return 4;
                }
                if (search->type == GPI_SEARCH_PROFILE || search->type == GPI_SEARCH_PROFILE_UNIQUENICK) {
                    GPProfileSearchResponseArg response;

                    response.result = 0;
                    response.numMatches = 0;
                    response.matches = NULL;
                    response.more = GP_DONE;
                    done = GPIFalse;
                    do {
                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "bsrdone") == 0) {
                            result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                            if (result != 0) {
                                return result;
                            }
                            if (strcmp(key, "more") == 0) {
                                if (strcmp(value, "0") != 0) {
                                    response.more = GP_MORE;
                                }
                            }
                            done = GPITrue;
                        } else if (strcmp(key, "bsr") == 0) {
                            response.numMatches++;
                            response.matches =
                                gsirealloc(response.matches, sizeof(GPProfileSearchMatch) * response.numMatches);
                            if (response.matches == NULL) {
                                gpiSetErrorString(connection, "Out of memory.");
                                return 1;
                            }
                            match = &response.matches[response.numMatches - 1];
                            memset(match, 0, sizeof(GPProfileSearchMatch));
                            match->profile = atol(value);
                            doneParsingMatch = GPIFalse;
                            do {
                                oldIndex = index;
                                result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                                if (result != 0) {
                                    return result;
                                }
                                if (strcmp(key, "nick") == 0) {
                                    strzcpy(match->nick, value, sizeof(match->nick));
                                } else if (strcmp(key, "uniquenick") == 0) {
                                    strzcpy(match->uniquenick, value, sizeof(match->uniquenick));
                                } else if (strcmp(key, "namespaceid") == 0) {
                                    match->namespaceID = atol(value);
                                } else if (strcmp(key, "firstname") == 0) {
                                    strzcpy(match->firstname, value, sizeof(match->firstname));
                                } else if (strcmp(key, "lastname") == 0) {
                                    strzcpy(match->lastname, value, sizeof(match->lastname));
                                } else if (strcmp(key, "email") == 0) {
                                    strzcpy(match->email, value, sizeof(match->email));
                                } else if (strcmp(key, "bsr") == 0 || strcmp(key, "bsrdone") == 0) {
                                    doneParsingMatch = GPITrue;
                                    index = oldIndex;
                                }
                            } while (!doneParsingMatch);
                        } else {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                    } while (!done);
                    // The callback may change more to stop the next search
                    more = response.more;
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        callback.callback(connection, &response, callback.param);
                    }
                    if (more == GP_MORE && response.more == GP_MORE) {
                        result = gpiProfileSearch(connection, search->nick, search->uniquenick, search->email,
                                                  search->firstname, search->lastname, search->icquin,
                                                  response.numMatches + search->skip, operation->blocking,
                                                  operation->callback.callback, operation->callback.param);
                        if (result != 0) {
                            return result;
                        }
                    }
                    gsifree(response.matches);
                    response.matches = NULL;
                } else if (search->type == GPI_SEARCH_IS_VALID) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPIsValidEmailResponseArg *data;

                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "vr") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        data = gsimalloc(sizeof(GPIsValidEmailResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = 0;
                        strzcpy(data->email, search->email, sizeof(data->email));
                        if (value[0] == '0') {
                            data->isValid = 0;
                        } else {
                            data->isValid = 1;
                        }
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_NORMAL);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_NICKS) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPGetUserNicksResponseArg *data;

                        data = gsimalloc(sizeof(GPGetUserNicksResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = 0;
                        gsiSafeStrcpyA(data->email, search->email, sizeof(data->email));
                        data->numNicks = 0;
                        data->nicks = NULL;
                        data->uniquenicks = NULL;
                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "nr") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        done = GPIFalse;
                        do {
                            result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                            if (result != 0) {
                                return result;
                            }
                            if (strcmp(key, "nick") == 0) {
                                tempPtr = gsirealloc(data->nicks, sizeof(char *) * (data->numNicks + 1));
                                if (tempPtr == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                data->nicks = tempPtr;
                                tempPtr = gsimalloc(GP_NICK_LEN);
                                if (tempPtr == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                data->nicks[data->numNicks] = tempPtr;
                                strzcpy(data->nicks[data->numNicks], value, GP_NICK_LEN);
                                data->numNicks++;
                            } else if (strcmp(key, "uniquenick") == 0) {
                                if (data->numNicks <= 0) {
                                    continue;
                                }
                                tempPtr = gsirealloc(data->uniquenicks, sizeof(char *) * data->numNicks);
                                if (tempPtr == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                data->uniquenicks = tempPtr;
                                tempPtr = gsimalloc(GP_UNIQUENICK_LEN);
                                if (tempPtr == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                data->uniquenicks[data->numNicks - 1] = tempPtr;
                                strzcpy(data->uniquenicks[data->numNicks - 1], value, GP_UNIQUENICK_LEN);
                            } else if (strcmp(key, "ndone") == 0) {
                                done = GPITrue;
                            } else {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                        } while (!done);
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_NICKS);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_PLAYERS) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPFindPlayersResponseArg *data;
                        GPFindPlayerMatch *playerMatch;

                        data = gsimalloc(sizeof(GPFindPlayersResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->productID = search->productID;
                        data->result = 0;
                        data->numMatches = 0;
                        data->matches = NULL;
                        done = GPIFalse;
                        do {
                            result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                            if (result != 0) {
                                return result;
                            }
                            if (strcmp(key, "psrdone") == 0) {
                                done = GPITrue;
                            } else if (strcmp(key, "psr") == 0) {
                                data->numMatches++;
                                data->matches = gsirealloc(data->matches, sizeof(GPFindPlayerMatch) * data->numMatches);
                                if (data->matches == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                playerMatch = &data->matches[data->numMatches - 1];
                                memset(playerMatch, 0, sizeof(GPFindPlayerMatch));
                                playerMatch->status = 1;
                                playerMatch->profile = atol(value);
                                doneParsingMatch = GPIFalse;
                                do {
                                    oldIndex = index;
                                    result =
                                        gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                                    if (result != 0) {
                                        return result;
                                    }
                                    if (strcmp(key, "status") == 0) {
                                        strzcpy(playerMatch->statusString, value, sizeof(playerMatch->statusString));
                                    } else if (strcmp(key, "nick") == 0) {
                                        strzcpy(playerMatch->nick, value, sizeof(playerMatch->nick));
                                    }
                                    if (strcmp(key, "statuscode") == 0) {
                                        playerMatch->status = atol(value);
                                    } else if (strcmp(key, "psr") == 0 || strcmp(key, "psrdone") == 0) {
                                        doneParsingMatch = GPITrue;
                                        index = oldIndex;
                                    }
                                } while (!doneParsingMatch);
                            } else {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                        } while (!done);
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_PMATCH);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_CHECK) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPCheckResponseArg *data;

                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "cur") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        rcode = atol(value);
                        if (rcode) {
                            iconnection->errorCode = rcode;
                            pid = 0;
                        } else {
                            if (!gpiValueForKey(search->inputBuffer.buffer, "\\pid\\", value, sizeof(value))) {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                            pid = atol(value);
                        }
                        data = gsimalloc(sizeof(GPCheckResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = rcode;
                        data->profile = pid;
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_NORMAL);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_NEWUSER) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPNewUserResponseArg *data;

                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "nur") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        rcode = atol(value);
                        if (rcode) {
                            iconnection->errorCode = rcode;
                        }
                        if (!gpiValueForKey(search->inputBuffer.buffer, "\\pid\\", value, sizeof(value))) {
                            if (rcode == 0) {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                            pid = 0;
                        } else {
                            pid = atol(value);
                        }
                        data = gsimalloc(sizeof(GPNewUserResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = rcode;
                        data->profile = pid;
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_NORMAL);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_OTHERS_BUDDY) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPGetReverseBuddiesResponseArg *data;

                        data = gsimalloc(sizeof(GPGetReverseBuddiesResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = 0;
                        data->numProfiles = 0;
                        data->profiles = NULL;
                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "others") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        done = GPIFalse;
                        do {
                            result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                            if (result != 0) {
                                return result;
                            }
                            if (strcmp(key, "odone") == 0) {
                                done = GPITrue;
                            } else if (strcmp(key, "o") == 0) {
                                tempPtr =
                                    gsirealloc(data->profiles, sizeof(GPProfileSearchMatch) * (data->numProfiles + 1));
                                if (tempPtr == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                data->profiles = tempPtr;
                                match = &data->profiles[data->numProfiles];
                                memset(match, 0, sizeof(GPProfileSearchMatch));
                                data->numProfiles++;
                                match->profile = atol(value);
                                doneParsingMatch = GPIFalse;
                                do {
                                    oldIndex = index;
                                    result =
                                        gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                                    if (result != 0) {
                                        return result;
                                    }
                                    if (strcmp(key, "nick") == 0) {
                                        strzcpy(match->nick, value, sizeof(match->nick));
                                    } else if (strcmp(key, "uniquenick") == 0) {
                                        strzcpy(match->uniquenick, value, sizeof(match->uniquenick));
                                    } else if (strcmp(key, "first") == 0) {
                                        strzcpy(match->firstname, value, sizeof(match->firstname));
                                    } else if (strcmp(key, "last") == 0) {
                                        strzcpy(match->lastname, value, sizeof(match->lastname));
                                    } else if (strcmp(key, "email") == 0) {
                                        strzcpy(match->email, value, sizeof(match->email));
                                    } else if (strcmp(key, "o") == 0 || strcmp(key, "odone") == 0) {
                                        doneParsingMatch = GPITrue;
                                        index = oldIndex;
                                    }
                                } while (!doneParsingMatch);
                            } else {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                        } while (!done);
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_REVERSE_BUDDIES);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_OTHERS_BUDDY_LIST) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPGetReverseBuddiesListResponseArg *data;

                        data = gsimalloc(sizeof(GPGetReverseBuddiesListResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = 0;
                        data->numOfUniqueMatches = 0;
                        data->matches = NULL;
                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "otherslist") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        done = GPIFalse;
                        do {
                            result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                            if (result != 0) {
                                return result;
                            }
                            if (strcmp(key, "oldone") == 0) {
                                done = GPITrue;
                            } else if (strcmp(key, "o") == 0) {
                                tempPtr =
                                    gsirealloc(data->matches, sizeof(GPUniqueMatch) * (data->numOfUniqueMatches + 1));
                                if (tempPtr == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                data->matches = tempPtr;
                                uniqueMatch = &data->matches[data->numOfUniqueMatches];
                                memset(uniqueMatch, 0, sizeof(GPUniqueMatch));
                                data->numOfUniqueMatches++;
                                uniqueMatch->profile = atol(value);
                                doneParsingMatch = GPIFalse;
                                do {
                                    oldIndex = index;
                                    result =
                                        gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                                    if (result != 0) {
                                        return result;
                                    }
                                    if (strcmp(key, "uniquenick") == 0) {
                                        strzcpy(uniqueMatch->uniqueNick, value, sizeof(uniqueMatch->uniqueNick));
                                    } else if (strcmp(key, "o") == 0 || strcmp(key, "oldone") == 0) {
                                        doneParsingMatch = GPITrue;
                                        index = oldIndex;
                                    }
                                } while (!doneParsingMatch);
                            } else {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                        } while (!done);
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_REVERSE_BUDDIES_LIST);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_SUGGEST_UNIQUE) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        int count = 0;
                        GPSuggestUniqueNickResponseArg *data;

                        data = gsimalloc(sizeof(GPSuggestUniqueNickResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = 0;
                        data->numSuggestedNicks = 0;
                        data->suggestedNicks = NULL;
                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "us") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        data->numSuggestedNicks = atol(value);
                        data->suggestedNicks = gsimalloc(sizeof(char *) * data->numSuggestedNicks);
                        if (data->suggestedNicks == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        done = GPIFalse;
                        do {
                            result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                            if (result != 0) {
                                return result;
                            }
                            if (strcmp(key, "nick") == 0) {
                                data->suggestedNicks[count] = gsimalloc(GP_UNIQUENICK_LEN);
                                if (data->suggestedNicks[count] == NULL) {
                                    gpiSetErrorString(connection, "Out of memory.");
                                    return 1;
                                }
                                strzcpy(data->suggestedNicks[count], value, GP_UNIQUENICK_LEN);
                                count++;
                            } else if (strcmp(key, "usdone") == 0) {
                                data->numSuggestedNicks = count;
                                done = GPITrue;
                            } else {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                        } while (!done);
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_SUGGESTED_UNIQUE);
                        if (result != 0) {
                            return result;
                        }
                    }
                } else if (search->type == GPI_SEARCH_PROFILE_LIST) {
                    callback = operation->callback;
                    if (callback.callback != NULL) {
                        GPGetProfileListResponseArg *data;

                        data = gsimalloc(sizeof(GPGetProfileListResponseArg));
                        if (data == NULL) {
                            gpiSetErrorString(connection, "Out of memory.");
                            return 1;
                        }
                        data->result = 0;
                        data->numProfiles = 0;
                        data->profiles = NULL;
                        data->hidden = 0;
                        data->profile = ((GPISearchData *)operation->data)->profile;
                        result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                        if (result != 0) {
                            return result;
                        }
                        if (strcmp(key, "profilelist") != 0) {
                            gpiSetError(connection, 1, "Error reading from the search server.");
                            gpiCallErrorCallback(connection, 3, 1);
                            return 3;
                        }
                        done = GPIFalse;
                        do {
                            result = gpiReadKeyAndValue(connection, search->inputBuffer.buffer, &index, key, value);
                            if (result != 0) {
                                return result;
                            }
                            if (strcmp(key, "pldone") == 0) {
                                done = GPITrue;
                            } else if (strcmp(key, "hidden") == 0) {
                                data->hidden = 1;
                                done = GPITrue;
                            } else if (strcmp(key, "count") == 0) {
                                int count = atol(value);

                                if (count > 0) {
                                    tempPtr = gsimalloc(sizeof(int) * count);
                                    if (tempPtr == NULL) {
                                        gpiSetErrorString(connection, "Out of memory.");
                                        return 1;
                                    }
                                    data->profiles = tempPtr;
                                }
                            } else if (strcmp(key, "o") == 0) {
                                data->profiles[data->numProfiles] = atol(value);
                                data->numProfiles++;
                            } else {
                                gpiSetError(connection, 1, "Error reading from the search server.");
                                gpiCallErrorCallback(connection, 3, 1);
                                return 3;
                            }
                        } while (!done);
                        result = gpiAddCallback(connection, callback, data, operation, GPI_ADD_PROFILE_LIST);
                        if (result != 0) {
                            return result;
                        }
                    }
                }
                search->remove = 1;
                loop = GPIFalse;
            }
        }
        if (loop) {
            msleep(10);
        }
    } while (loop);
    return 0;
}

// Processes each search that isn't done, and removes the ones that ended. A search's callback may start another search,
// so the ones to process are listed first
GPResult gpiProcessSearches(GPConnection *connection) {
    GPIConnection *iconnection = *connection;
    GPIOperation **list;
    GPIOperation *operation;
    int count = 0;
    int i;
    int result;

    if (iconnection->numSearches > 0) {
        list = gsimalloc(iconnection->numSearches * 4);
        if (list == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        for (operation = iconnection->operationList; operation != NULL; operation = operation->pnext) {
            if (operation->type == GPI_PROFILE_SEARCH && operation->state != GPI_FINISHING &&
                ((GPISearchData *)operation->data)->processing == 0) {
                list[count++] = operation;
                ((GPISearchData *)operation->data)->processing = 1;
            }
        }
        for (i = 0; i < count; i++) {
            result = gpiProcessSearch(connection, list[i]);
            if (result != 0) {
                list[i]->result = result;
            }
        }
        for (i = 0; i < count; i++) {
            GPISearchData *search = list[i]->data;

            search->processing = 0;
            if (search->remove != 0) {
                gpiRemoveOperation(connection, list[i]);
            }
        }
        gsifree(list);
    }
    return 0;
}
