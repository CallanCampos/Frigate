#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "dwc/wcm.h"
#include "gfl/std.h"
#include "nitro/os.h"

// SOCL's shutdown and its address calls: SOCL_Cleanup waits for SOCL_CalmDown to close every socket (SOCL_CloseAll,
// then the UDP send socket) and ends CPS. The resolver and host address calls follow. pokediamond's NitroWiFi names the
// object socl_cleanup.o and its functions; this version split CPS's shutdown out of SOCL_Cleanup.

static int SOCL_CloseAll(void);

int SOCL_Cleanup(void) {
    int result;

    if (SOCLiRequestedIP == 0) {
        SOCLiRequestedIP = CPSMyIp;
    }
    if (SOCL_CalmDown() == SOCL_EINPROGRESS) {
        do {
            func_0207aa04(100);
        } while (SOCL_CalmDown() == SOCL_EINPROGRESS);
    }
    result = SOCLi_CleanupCommandPacketQueue();
    if (result >= 0) {
        SOCLi_CleanupCPS();
        if (SOCLiConfigPtr->lan_buffer == NULL) {
            SOCLiConfigPtr->free(SOCLiCPSConfig.lan_buf);
        }
        SOCLiConfigPtr = NULL;
        SOCLi_ResetSocketCount();
    }
    return result;
}

void SOCLi_CleanupCPS(void) {
    CPS_Cleanup();
    CPS_SetScavengerCallback(NULL);
    CPS_SetAddressCheckCallback(NULL);
    WCM_SetRecvDCFCallback(NULL);
}

// Closes the sockets that SOCL_Close has not run on, except the UDP send socket; EINPROGRESS while any is left
static int SOCL_CloseAll(void) {
    SOCLSocket *socket;
    u32 irq;

    while (TRUE) {
        irq = CPU_IRQDisable();
        for (socket = SOCLiSocketList; socket != NULL; socket = socket->next) {
            if (socket != SOCLiUDPSendSocket && !(socket->state & SOCLI_STATE_CLOSE_CALLED)) {
                break;
            }
        }
        CPU_SetIRQMask(irq);
        if (socket == NULL) {
            break;
        }
        SOCL_Close((int)socket);
    }
    if (SOCLiSocketList == NULL || (SOCLiSocketList == SOCLiUDPSendSocket && SOCLiSocketList->next == NULL)) {
        if (SOCLiSocketListTrash == NULL) {
            return 0;
        }
    }
    return SOCL_EINPROGRESS;
}

int SOCL_CalmDown(void) {
    int result;

    if (SOCLiUDPSendSocket != NULL) {
        result = SOCL_CloseAll();
        if (result == 0) {
            SOCL_Close((int)SOCLiUDPSendSocket);
            if (SOCL_IsClosed((int)SOCLiUDPSendSocket)) {
                SOCLiUDPSendSocket = NULL;
            }
            result = SOCL_EINPROGRESS;
        }
        SOCLi_TrashSocket();
    } else if (CPS_CalmDown()) {
        WCM_SetRecvDCFCallback(NULL);
        result = 0;
    } else {
        result = SOCL_EINPROGRESS;
    }
    return result;
}

// Asks the DNS servers of CPS for an address, on a socket of its own
CPSInAddr SOCL_Resolve(const char *name) {
    CPSSoc soc;
    u8 *buf;
    CPSInAddr ip;

    if (SOCLiConfigPtr == NULL) {
        return 0;
    }
    if (name == NULL) {
        return 0;
    }
    buf = SOCLiConfigPtr->alloc(0x1708);
    if (buf == NULL) {
        return 0;
    }
    sys_memset(&soc, 0, sizeof(CPSSoc));
    soc.rcvbuf = buf;
    soc.rcvbuf_size = 0xb68;
    soc.sndbuf = buf + 0xb68;
    soc.sndbuf_size = 0xb9e;
    CPS_SocRegister(&soc);
    ip = CPS_Resolve(name);
    CPS_SocUnRegister();
    SOCLiConfigPtr->free(buf);
    return ip;
}

// Resolves a dotted address only: the DNS servers are cleared meanwhile, so CPS_Resolve does not ask them
CPSInAddr SOCL_InetAtoH(const char *name) {
    CPSInAddr dns1, dns2;
    CPSInAddr ip;
    u32 irq = CPU_IRQDisable();

    dns1 = CPSDnsIp[0];
    dns2 = CPSDnsIp[1];
    CPSDnsIp[0] = 0;
    CPSDnsIp[1] = 0;
    ip = CPS_Resolve(name);
    CPSDnsIp[0] = dns1;
    CPSDnsIp[1] = dns2;
    CPU_SetIRQMask(irq);
    return ip;
}

int SOCL_SetResolver(CPSInAddr dns1, CPSInAddr dns2) {
    if (SOCL_GetHostID() == 0) {
        return SOCL_ENETRESET;
    }
    CPSDnsIp[0] = dns1;
    if (dns1 == 0) {
        CPSMode &= ~CPS_MODE_STATIC_DNS;
        CPSDnsIp[1] = 0;
    } else {
        CPSMode |= CPS_MODE_STATIC_DNS;
        CPSDnsIp[1] = dns2;
    }
    return 0;
}

// The host address, 0 while DHCP has none (the caller waits a little when DHCP is still running)
CPSInAddr SOCL_GetHostID(void) {
    CPSInAddr ip = CPSMyIp;

    if (ip == 0) {
        if ((SOCLiDhcpState & (SOCLI_DHCP_REQUESTED | SOCLI_DHCP_DONE)) == SOCLI_DHCP_REQUESTED) {
            if (CPU_GetMode() != 0x12) {
                func_0207aa04(10);
            }
        }
    } else if (SOCLiRequestedIP == 0) {
        SOCLiRequestedIP = ip;
    }
    return CPSMyIp;
}
