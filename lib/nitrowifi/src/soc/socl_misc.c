#include "nitro/mi.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL's helpers: the rounding of sizes, the socket counters and the checks of the address configuration that CPS asks
// for. pokediamond's socl_misc.o has only SOCLi_RoundUp4; Black 1's decompilation (aletheia) has the same five
// functions in its socl_misc.c, with the SDK's names from TwlSDK's libsoc (socl_misc.o).

static BOOL SOCLi_IsIPv4ValidDNSAddr(u32 net_mask, u32 ip, u32 my_ip);
static BOOL SOCLi_IsIPv4ValidNodeAddr(u32 net_mask, u32 ip);

s32 SOCLiSocketCount[2];

u32 SOCLi_RoundUp4(u32 value) {
    return (value + 3) & ~3;
}

void SOCLi_InitResource(void) {
    MI_CpuFill8(SOCLiSocketCount, 0, sizeof(SOCLiSocketCount));
}

// Whether ip can be a remote address for a host with this net mask and address: not the network or broadcast address
// of the host's own subnet, not 0, not 255.255.255.255, not loopback, not multicast
static BOOL SOCLi_IsIPv4ValidDNSAddr(u32 net_mask, u32 ip, u32 my_ip) {
    BOOL not_multicast = FALSE;
    BOOL not_loopback = FALSE;
    BOOL ok = FALSE;

    if (ip != 0xffffffff && ip != 0 && my_ip != ip) {
        if (((my_ip ^ ip) & net_mask) != 0 || (ip & ~net_mask) != 0) {
            BOOL unicast;

            if (((my_ip ^ ip) & net_mask) == 0 && ((ip & ~net_mask) ^ ~net_mask) == 0) {
                unicast = FALSE;
            } else {
                unicast = TRUE;
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
static BOOL SOCLi_IsIPv4ValidNodeAddr(u32 net_mask, u32 ip) {
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

BOOL SOCLi_CheckNConfig(void) {
    if (CPSNetMask == 0 || ((~CPSNetMask + 1) & ~CPSNetMask) != 0) {
        return FALSE;
    }
    if (!SOCLi_IsIPv4ValidNodeAddr(CPSNetMask, CPSMyIp)) {
        return FALSE;
    }
    if (CPSGatewayIp != 0) {
        if (!SOCLi_IsIPv4ValidNodeAddr(CPSNetMask, CPSGatewayIp) ||
            (CPSNetMask != 0xffffffff && (CPSMyIp & CPSNetMask) != (CPSGatewayIp & CPSNetMask)) ||
            CPSMyIp == CPSGatewayIp) {
            CPSGatewayIp = 0;
        }
    }
    if (!SOCLi_IsIPv4ValidDNSAddr(CPSNetMask, CPSDnsIp[0], CPSMyIp)) {
        CPSDnsIp[0] = 0;
    }
    if (!SOCLi_IsIPv4ValidDNSAddr(CPSNetMask, CPSDnsIp[1], CPSMyIp)) {
        CPSDnsIp[1] = 0;
    }
    return TRUE;
}
