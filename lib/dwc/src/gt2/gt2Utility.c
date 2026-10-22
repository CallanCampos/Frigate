#include "dwc/gt2Utility.h"
#include "dwc/ext.h"

// GameSpy GT2's addresses as text (gt2Utility.c)

// gt2AddressToString's two buffers, which it uses in turn; MWCC lays .bss out in the reverse order of the
// declarations
static char strAddressArray[2][22];
static int nIndex;

const char *gt2AddressToString(unsigned int ip, unsigned short port, char string[22]) {
    if (string == NULL) {
        nIndex ^= 1;
        string = strAddressArray[nIndex];
    }
    if (ip != 0) {
        SOCInAddr address;

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

GT2Bool gt2StringToAddress(const char *string, unsigned int *ipOut, unsigned short *portOut) {
    gsi_u32 ip = 0;
    gsi_u16 port;
    char host[0x104];
    char *colon;
    char *digit;
    long number;
    SOCHostEnt *hostEnt;

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
                    return gsi_false;
                }
                digit++;
            }
            number = atol(colon + 1);
            if (number < 0 || number > 0xffff) {
                return gsi_false;
            }
            port = number;
        }
        if (string != NULL) {
            ip = inet_addr(string);
            if (ip == -1) {
                hostEnt = SOC_GetHostByName(string);
                if (hostEnt == NULL) {
                    return gsi_false;
                }
                ip = *(gsi_u32 *)hostEnt->addrList[0];
            }
        }
    }
    if (ipOut != NULL) {
        *ipOut = ip;
    }
    if (portOut != NULL) {
        *portOut = port;
    }
    return gsi_true;
}

void gti2MessageCheck(const GT2Byte **message, int *length) {
    if (*message == NULL) {
        *message = "";
        *length = 0;
    } else if (*length == -1) {
        *length = strlen(*message) + 1;
    }
}
