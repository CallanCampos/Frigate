#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "gfl/std.h"
#include "nitro/os.h"

// SOCL's lists of sockets: the live ones, and the closed ones that SOCLi_TrashSocket frees. The singly linked lists
// chain through SOCLSocket.next. pokediamond's NitroWiFi names the object socl_list.o; this version adds the socket
// counters and the address checks that CPS asks for, whose names are guesses.

static BOOL SOCLi_IsValidHostIp(u32 net_mask, u32 ip);
static BOOL SOCLi_IsValidRemoteIp(u32 net_mask, u32 ip, u32 my_ip);
static void SOCLi_SocketRegisterList(SOCLSocket **list, SOCLSocket *socket);
static void SOCLi_SocketUnregisterList(SOCLSocket **list, SOCLSocket *socket);
static SOCLSocket **SOCLi_SocketGetNextPtr(SOCLSocket **list, SOCLSocket *socket);

s32 SOCLiSocketCount[2];
SOCLSocket *SOCLiSocketList;
SOCLSocket *SOCLiSocketListTrash;

void SOCLi_ResetSocketCount(void) {
    sys_memset(SOCLiSocketCount, 0, sizeof(SOCLiSocketCount));
}

// Whether ip can be a remote address for a host with this net mask and address: not the network or broadcast address
// of the host's own subnet, not 0, not 255.255.255.255, not loopback, not multicast
static BOOL SOCLi_IsValidRemoteIp(u32 net_mask, u32 ip, u32 my_ip) {
    BOOL not_multicast = FALSE;
    BOOL not_loopback = FALSE;
    BOOL ok = FALSE;

    if (ip != 0xffffffff && ip != 0 && my_ip != ip) {
        if (((my_ip ^ ip) & net_mask) != 0 || (ip & ~net_mask) != 0) {
            BOOL unicast = TRUE;

            if (((my_ip ^ ip) & net_mask) == 0 && (ip & ~net_mask) == ~net_mask) {
                unicast = FALSE;
            }
            if (unicast) {
                ok = TRUE;
            }
        }
    }
    if (ok) {
        if (((ip & 0xff000000) ^ 0x7f000000) != 0) {
            not_loopback = TRUE;
        }
    }
    if (not_loopback) {
        if (((ip & 0xf0000000) ^ 0xe0000000) != 0) {
            not_multicast = TRUE;
        }
    }
    return not_multicast;
}

// Whether ip can be the host's own address for this net mask: not the subnet's network or broadcast address, not
// loopback, not multicast
static BOOL SOCLi_IsValidHostIp(u32 net_mask, u32 ip) {
    BOOL not_multicast = FALSE;
    BOOL not_loopback = FALSE;
    BOOL host_ok = FALSE;
    u32 host = ip & ~net_mask;

    if ((host ^ ~net_mask) != 0 && host != 0) {
        host_ok = TRUE;
    }
    if (host_ok) {
        if (((ip & 0xff000000) ^ 0x7f000000) != 0) {
            not_loopback = TRUE;
        }
    }
    if (not_loopback) {
        if (((ip & 0xf0000000) ^ 0xe0000000) != 0) {
            not_multicast = TRUE;
        }
    }
    return not_multicast;
}

BOOL SOCLi_AddressCheckCallback(void) {
    if (CPSNetMask == 0 || ((~CPSNetMask + 1) & ~CPSNetMask) != 0) {
        return FALSE;
    }
    if (!SOCLi_IsValidHostIp(CPSNetMask, CPSMyIp)) {
        return FALSE;
    }
    if (CPSGatewayIp != 0) {
        if (!SOCLi_IsValidHostIp(CPSNetMask, CPSGatewayIp) ||
            (CPSNetMask != 0xffffffff && (CPSMyIp & CPSNetMask) != (CPSGatewayIp & CPSNetMask)) ||
            CPSMyIp == CPSGatewayIp) {
            CPSGatewayIp = 0;
        }
    }
    if (!SOCLi_IsValidRemoteIp(CPSNetMask, CPSDnsIp[0], CPSMyIp)) {
        CPSDnsIp[0] = 0;
    }
    if (!SOCLi_IsValidRemoteIp(CPSNetMask, CPSDnsIp[1], CPSMyIp)) {
        CPSDnsIp[1] = 0;
    }
    return TRUE;
}

void SOCLi_SocketRegister(SOCLSocket *socket) {
    SOCLi_SocketRegisterList(&SOCLiSocketList, socket);
}

static void SOCLi_SocketRegisterList(SOCLSocket **list, SOCLSocket *socket) {
    socket->next = *list;
    *list = socket;
}

void SOCLi_SocketRegisterTrash(SOCLSocket *socket) {
    SOCLi_SocketRegisterList(&SOCLiSocketListTrash, socket);
}

void SOCLi_SocketUnregister(SOCLSocket *socket) {
    SOCLi_SocketUnregisterList(&SOCLiSocketList, socket);
}

static void SOCLi_SocketUnregisterList(SOCLSocket **list, SOCLSocket *socket) {
    u32 irq = CPU_IRQDisable();
    SOCLSocket **link = SOCLi_SocketGetNextPtr(list, socket);

    if (link != NULL) {
        *link = socket->next;
    }
    CPU_SetIRQMask(irq);
}

// The pointer that points to the socket (the list's head or a socket's next), NULL when it is not in the list
static SOCLSocket **SOCLi_SocketGetNextPtr(SOCLSocket **list, SOCLSocket *socket) {
    SOCLSocket **link = list;
    SOCLSocket *s;
    u32 irq = CPU_IRQDisable();

    for (s = *link; s != NULL; s = *link) {
        if (s == socket) {
            CPU_SetIRQMask(irq);
            return link;
        }
        link = &s->next;
    }
    CPU_SetIRQMask(irq);
    return NULL;
}

void SOCLi_SocketUnregisterTrash(SOCLSocket *socket) {
    SOCLi_SocketUnregisterList(&SOCLiSocketListTrash, socket);
}

BOOL SOCL_SocketIsInvalid(const SOCLSocket *socket) {
    if ((int)socket <= 0 || SOCLi_SocketGetNextPtr(&SOCLiSocketList, (SOCLSocket *)socket) == NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL SOCL_SocketIsInTrash(const SOCLSocket *socket) {
    if (SOCLi_SocketGetNextPtr(&SOCLiSocketListTrash, (SOCLSocket *)socket) != NULL) {
        return TRUE;
    }
    return FALSE;
}
