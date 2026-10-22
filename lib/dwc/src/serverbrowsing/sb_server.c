#include <string.h>
// Before dwc/ext.h: MWCC builds the memcpy of a player or team count in place, as the ROM does, only when <string.h>
// declares memcpy first
#include "dwc/ext.h"
#include "dwc/qr2regkeys.h"
#include "dwc/sb_server.h"
#include "dwc/sb_serverlist.h"

// A game server of GameSpy's server browser (sb_server.c). The ROM names neither the file nor its functions: the file
// and the functions are GameSpy's, as pret's pokerevo Wii build has them (sb_server.s), and CheckValidKey, mytok,
// StringHash and SBServerGetPing, which the Wii's build doesn't name, are named after the later SDK, as are
// g_SBRefStrList, SBNullServer and mytok's thestr. The file is built with -ipa file, which reaches the three from one
// base, as the ROM does

static int StringHash(const char *s, int numbuckets);

// The table of the shared texts of every server, of SBRefString
static HashTable g_SBRefStrList = NULL;

static int RefStringHash(const void *elem, int numbuckets) {
    return StringHash(((const SBRefString *)elem)->str, numbuckets);
}

static int RefStringCompare(const void *entry1, const void *entry2) {
    return strcmpi(((const SBRefString *)entry1)->str, ((const SBRefString *)entry2)->str);
}

static void RefStringFree(void *elem) {
    gsifree((char *)((SBRefString *)elem)->str);
}

HashTable SBRefStrHash(SBServerList *slist) {
    if (g_SBRefStrList == NULL) {
        g_SBRefStrList = TableNew2(sizeof(SBRefString), 100, 2, RefStringHash, RefStringCompare, RefStringFree);
    }
    return g_SBRefStrList;
}

void SBRefStrHashCleanup(SBServerList *slist) {
    if (g_SBRefStrList != NULL && TableCount(g_SBRefStrList) == 0) {
        TableFree(g_SBRefStrList);
        g_SBRefStrList = NULL;
    }
}

void SBServerFree(void *elem) {
    SBServer server = *(SBServer *)elem;

    TableFree(server->keyvals);
    server->keyvals = NULL;
    gsifree(server);
}

void SBServerAddKeyValue(SBServer server, const char *keyname, const char *value) {
    SBKeyValuePair kv;

    kv.key = SBRefStr(NULL, keyname);
    kv.value = SBRefStr(NULL, value);
    TableEnter(server->keyvals, &kv);
    gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Misc, GSIDebugLevel_Comment, "SBServerAddKeyValue added %s\\%s\r\n",
                  keyname, value);
}

void SBServerAddIntKeyValue(SBServer server, const char *keyname, int value) {
    char stemp[20];

    OS_SPrintf(stemp, "%d", value);
    SBServerAddKeyValue(server, keyname, stemp);
}

const char *SBServerGetStringValueA(SBServer server, const char *keyname, const char *def) {
    SBKeyValuePair kv;
    SBKeyValuePair *ptr;

    if (server == NULL) {
        return NULL;
    }
    kv.key = keyname;
    ptr = TableLookup(server->keyvals, &kv);
    // An empty value counts as none
    if (ptr == NULL || strlen(ptr->value) == 0) {
        return def;
    }
    return ptr->value;
}

int SBServerGetIntValueA(SBServer server, const char *key, int idefault) {
    const char *s;
    const char *s2;

    if (server == NULL) {
        return idefault;
    }
    if (strcmp(key, "ping") == 0) {
        return SBServerGetPing(server);
    }
    s = SBServerGetStringValueA(server, key, NULL);
    if (s == NULL) {
        return idefault;
    }
    s2 = (*s != '-') ? s : s + 1;
    if (!isdigit((gsi_u8)*s2)) {
        return idefault;
    }
    return atol(s);
}

double SBServerGetFloatValueA(SBServer server, const char *key, double fdefault) {
    const char *s = SBServerGetStringValueA(server, key, NULL);

    if (s == NULL) {
        return fdefault;
    }
    return atof(s);
}

unsigned int SBServerGetPublicInetAddress(SBServer server) {
    return server->publicip;
}

unsigned short SBServerGetPublicQueryPort(SBServer server) {
    return SB_SWAP16(server->publicport);
}

unsigned short SBServerGetPublicQueryPortNBO(SBServer server) {
    return server->publicport;
}

SBBool SBServerHasPrivateAddress(SBServer server) {
    if ((server->flags & PRIVATE_IP_FLAG) == PRIVATE_IP_FLAG) {
        return gsi_true;
    }
    return gsi_false;
}

unsigned int SBServerGetPrivateInetAddress(SBServer server) {
    return server->privateip;
}

unsigned short SBServerGetPrivateQueryPort(SBServer server) {
    return SB_SWAP16(server->privateport);
}

void SBServerSetNext(SBServer server, SBServer next) {
    server->next = next;
}

SBServer SBServerGetNext(SBServer server) {
    return server->next;
}

// Whether a key of the old protocol's answer is one of the server's, not of the answer
static int CheckValidKey(char *key) {
    const char *InvalidKeys[] = { "queryid", "final" };
    gsi_u32 i;

    for (i = 0; i < NELEMS(InvalidKeys); i++) {
        if (strcmp(key, InvalidKeys[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

// strtok with one delimiter, which also gives the empty texts between two of them
static char *mytok(char *instr, char delim) {
    char *result;
    static char *thestr;

    if (instr != NULL) {
        thestr = instr;
    }
    result = thestr;
    while (*thestr != '\0' && *thestr != delim) {
        thestr++;
    }
    if (thestr == result) {
        result = NULL;
    }
    if (*thestr != '\0') {
        *thestr++ = '\0';
    }
    return result;
}

void SBServerParseKeyVals(SBServer server, char *keyvals) {
    char *k;
    char *v;

    k = mytok(++keyvals, '\\');
    while (k != NULL) {
        v = mytok(NULL, '\\');
        if (v == NULL) {
            v = "";
        }
        if (CheckValidKey(k) && !qr2_internal_is_master_only_key(k)) {
            SBServerAddKeyValue(server, k, v);
        }
        k = mytok(NULL, '\\');
    }
}

// QR2's answer: the server's keys and values, then the players' and the teams', each a count, the names of their keys
// and the values of every player or team, which are added as the key's name and the player's number
void SBServerParseQR2FullKeysSingle(SBServer server, char *data, int len) {
    int dlen;
    char *k;
    char *v;
    char *keys;
    int nkeys;
    gsi_u16 nunits;
    int pflag;
    int i;
    int j;
    char tempkey[128];

    while (*data != '\0') {
        dlen = NTSLengthSB(data, len);
        if (dlen < 0) {
            return;
        }
        k = data;
        data += dlen;
        len -= dlen;
        dlen = NTSLengthSB(data, len);
        if (dlen < 0) {
            return;
        }
        v = data;
        data += dlen;
        len -= dlen;
        if (!qr2_internal_is_master_only_key(k)) {
            SBServerAddKeyValue(server, k, v);
        }
    }
    data++;
    len--;
    for (pflag = 0; pflag < 2; pflag++) {
        if (len < 2) {
            return;
        }
        memcpy(&nunits, data, 2);
        nunits = SB_SWAP16(nunits);
        data += 2;
        len -= 2;
        keys = data;
        nkeys = 0;
        while (*data != '\0') {
            dlen = NTSLengthSB(data, len);
            if (dlen < 0) {
                return;
            }
            if (dlen > 100) {
                return;
            }
            nkeys++;
            data += dlen;
            len -= dlen;
        }
        data++;
        len--;
        for (i = 0; i < nunits; i++) {
            k = keys;
            for (j = 0; j < nkeys; j++) {
                dlen = NTSLengthSB(data, len);
                if (dlen < 0) {
                    return;
                }
                OS_SPrintf(tempkey, "%s%d", k, i);
                SBServerAddKeyValue(server, tempkey, data);
                data += dlen;
                len -= dlen;
                k += strlen(k) + 1;
            }
        }
    }
}

// A part of QR2's answer that is split in several: "splitnum", the part's number (with 0x80 on the last), and then
// sections of the server's keys and values, or of a key of the players or teams with a first number and its values
void SBServerParseQR2FullKeysSplit(SBServer server, char *data, int len) {
    int dlen;
    char *k;
    char *v;
    gsi_u32 packetNum = 0;
    gsi_bool isFinal = gsi_false;

    if (*data == '\0') {
        return;
    }
    dlen = NTSLengthSB(data, len);
    if (dlen < 0) {
        return;
    }
    k = data;
    data += dlen;
    len -= dlen;
    if (strnicmp("splitnum", k, strlen("splitnum")) != 0) {
        return;
    }
    if (len < 1) {
        return;
    }
    packetNum = (gsi_u8)*data;
    data++;
    len--;
    if ((packetNum & 0x80) == 0x80) {
        isFinal = gsi_true;
        packetNum ^= 0x80;
    }
    if (packetNum > 7) {
        return;
    }
    if (isFinal == gsi_true) {
        // The parts after the last one never come
        server->splitResponseBitmap |= (char)(0xff << packetNum);
    } else {
        server->splitResponseBitmap |= 1 << packetNum;
    }
    if (len < 1) {
        return;
    }
    while (len > 0) {
        int keyType = 0;
        int nindex = 0;

        keyType = *data;
        data++;
        len--;
        if (keyType < 0 || keyType > 2) {
            gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
                          "Split packet parse error, invalid key type! (%d)\r\n", keyType);
            return;
        }
        while (*data != '\0') {
            dlen = NTSLengthSB(data, len);
            if (dlen < 0) {
                return;
            }
            k = data;
            data += dlen;
            len -= dlen;
            if (keyType == 0) {
                dlen = NTSLengthSB(data, len);
                if (dlen < 0) {
                    return;
                }
                v = data;
                data += dlen;
                len -= dlen;
                if (!qr2_internal_is_master_only_key(k)) {
                    SBServerAddKeyValue(server, k, v);
                }
            } else {
                char tempkey[128];

                if (len < 1) {
                    return;
                }
                nindex = *data;
                data++;
                len--;
                while (*data != '\0') {
                    dlen = NTSLengthSB(data, len);
                    if (dlen < 0) {
                        return;
                    }
                    v = data;
                    data += dlen;
                    len -= dlen;
                    OS_SPrintf(tempkey, "%s%d", k, nindex);
                    SBServerAddKeyValue(server, tempkey, v);
                    nindex++;
                }
                if (len > 0) {
                    data++;
                    len--;
                }
            }
        }
        if (len > 0) {
            if (*data != '\0') {
                gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
                              "Split packet parse error, NULL byte expected!\r\n");
                return;
            }
            data++;
            len--;
        }
    }
}

static int StringHash(const char *s, int numbuckets) {
    gsi_u32 hashcode = 0;

    while (*s != '\0') {
        hashcode = hashcode * 0x9ccf9319 + tolower(*s);
        s++;
    }
    return hashcode % numbuckets;
}

static void KeyValFree(void *elem) {
    SBKeyValuePair *kv = elem;

    SBReleaseStr(NULL, kv->key);
    SBReleaseStr(NULL, kv->value);
}

static int KeyValHashKey(const void *elem, int numbuckets) {
    return StringHash(((const SBKeyValuePair *)elem)->key, numbuckets);
}

static int KeyValCompareKey(const void *entry1, const void *entry2) {
    // A NULL key is never the same
    if (((const SBKeyValuePair *)entry1)->key == NULL || ((const SBKeyValuePair *)entry2)->key == NULL) {
        return 1;
    }
    return strcmpi(((const SBKeyValuePair *)entry1)->key, ((const SBKeyValuePair *)entry2)->key);
}

int SBServerGetPing(SBServer server) {
    return server->updatetime;
}

SBServer SBAllocServer(SBServerList *slist, goa_uint32 publicip, unsigned short publicport) {
    SBServer server = gsimalloc(sizeof(struct _SBServer));

    if (server == NULL) {
        return NULL;
    }
    server->keyvals = TableNew2(sizeof(SBKeyValuePair), 8, 4, KeyValHashKey, KeyValCompareKey, KeyValFree);
    if (server->keyvals == NULL) {
        gsifree(server);
        return NULL;
    }
    server->state = 0;
    server->flags = 0;
    server->next = NULL;
    server->updatetime = 0;
    server->icmpip = 0;
    server->publicip = publicip;
    server->publicport = publicport;
    server->privateip = 0;
    server->privateport = 0;
    return server;
}

void SBServerSetFlags(SBServer server, unsigned char flags) {
    server->flags = flags;
}

void SBServerSetPrivateAddr(SBServer server, goa_uint32 ip, unsigned short port) {
    server->privateip = ip;
    server->privateport = port;
}

void SBServerSetICMPIP(SBServer server, goa_uint32 icmpip) {
    server->icmpip = icmpip;
}

void SBServerSetState(SBServer server, unsigned char state) {
    server->state = state;
}

unsigned char SBServerGetState(SBServer server) {
    return server->state;
}

int SBIsNullServer(SBServer server) {
    if (server == SBNullServer) {
        return gsi_true;
    }
    return gsi_false;
}

SBServer SBNullServer = NULL;
