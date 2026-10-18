#include "dwc/gt2Utility.h"
#include "dwc/ext.h"

// GameSpy GT2's addresses as text (gt2Utility.c). The functions' names, from pret/pokerevo's Wii build, are in
// dwc/gt2Utility.h

// gt2AddressToString's two buffers, which it uses in turn; MWCC lays .bss out in the reverse order of the
// declarations
static char sStrings[2][22];
static int sStringIndex;

const char *func_ov011_0217ed14(u32 ip, u16 port, char *string) {
    if (string == NULL) {
        sStringIndex ^= 1;
        string = sStrings[sStringIndex];
    }
    if (ip != 0) {
        SocInAddr address;

        address.addr = ip;
        if (port != 0) {
            OS_SPrintf(string, "%s:%d", SOC_InetNtoA(address), port);
        } else {
            OS_SPrintf(string, "%s", SOC_InetNtoA(address));
        }
    } else if (port != 0) {
        OS_SPrintf(string, ":%d", port);
    } else {
        string[0] = '\0';
    }
    return string;
}

BOOL func_ov011_0217ed7c(const char *string, u32 *ipOut, u16 *portOut) {
    u32 ip = 0;
    u16 port;
    char host[0x104];
    char *colon;
    char *digit;
    long number;
    SocHostEnt *hostEnt;

    if (string == NULL || string[0] == '\0') {
        ip = 0;
        port = 0;
    } else {
        colon = strchr(string, ':');
        if (colon == NULL) {
            port = 0;
        } else {
            if (colon == string) {
                string = NULL;
            } else {
                memcpy(host, string, colon - string);
                host[colon - string] = '\0';
                string = host;
            }
            digit = colon + 1;
            while (*digit != '\0') {
                if (!isdigit(*digit)) {
                    return FALSE;
                }
                digit++;
            }
            number = atol(colon + 1);
            if (number < 0 || number > 0xffff) {
                return FALSE;
            }
            port = number;
        }
        if (string != NULL) {
            ip = func_ov011_0216df90(string);
            if (ip == -1) {
                hostEnt = SOC_GetHostByName(string);
                if (hostEnt == NULL) {
                    return FALSE;
                }
                ip = *(u32 *)hostEnt->addrList[0];
            }
        }
    }
    if (ipOut != NULL) {
        *ipOut = ip;
    }
    if (portOut != NULL) {
        *portOut = port;
    }
    return TRUE;
}

void func_ov011_0217ee60(const void **message, int *length) {
    if (*message == NULL) {
        *message = "";
        *length = 0;
    } else if (*length == -1) {
        *length = strlen(*message) + 1;
    }
}
