#include "dwc/wcm.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL's startup: it starts CPS with a configuration made from SOCL's, then the command packet queue and the socket
// that sends UDP datagrams. pokediamond's NitroWiFi names the object socl_startup.o and every function in it.
static int SOCLi_StartupSOCL(void);
static void SOCLi_StartupCPS(void);
static void SOCLi_SetMyIP(void);
static void SOCLi_DhcpTimeout(void);
static BOOL SOCL_LinkIsOn(void);

// The explicit zeros and this order lay the globals out as the original's .bss, which the code reaches from one base
SOCLSocket *SOCLiUDPSendSocket = NULL;
u32 SOCLiDhcpState = 0;
CPSInAddr SOCLiRequestedIP = 0;
u32 SOCLiYieldWait = 0;
CPSConfig SOCLiCPSConfig = { 0 };
// SOCLiConfigPtr's NULL is in .data, before the default buffer size
#pragma explicit_zero_data on
SOCLConfig *SOCLiConfigPtr = NULL;
#pragma explicit_zero_data reset
u32 SOCLiDefaultLanBufferSize = 0x4000;

int SOCL_Startup(SOCLConfig *config) {
    int result;

    OSi_ReferSymbol(SOCLi_Signature);
    if (SOCLiConfigPtr != NULL) {
        return 0;
    }
    SOCLi_ResetSocketCount();
    SOCLiConfigPtr = config;
    SOCLi_StartupCPS();
    result = SOCLi_StartupSOCL();
    if (result < 0) {
        SOCLi_CleanupCPS();
        SOCLiConfigPtr = NULL;
    }
    return result;
}

static int SOCLi_StartupSOCL(void) {
    int result = SOCLi_StartupCommandPacketQueue(SOCLiConfigPtr->cmd_packet_max);

    if (result >= 0) {
        SOCLiUDPSendSocket = (SOCLSocket *)SOCL_CreateSocket(&SOCLSocketParamUDPSend);
    }
    return result;
}

static void SOCLi_StartupCPS(void) {
    CPSConfig *cps = &SOCLiCPSConfig;
    SOCLConfig *config = SOCLiConfigPtr;

    MI_CpuFill8(cps, 0, sizeof(CPSConfig));
    cps->alloc = config->alloc;
    cps->free = config->free;
    cps->link_is_on = SOCL_LinkIsOn;
    cps->random_seed = 0;
    cps->yield_wait = SOCLiYieldWait;

    if (config->lan_buffer_size) {
        cps->lan_buflen = config->lan_buffer_size;
    } else {
        cps->lan_buflen = SOCLiDefaultLanBufferSize;
    }

    if (config->lan_buffer) {
        cps->lan_buf = config->lan_buffer;
    } else {
        cps->lan_buf = SOCLiConfigPtr->alloc(cps->lan_buflen);
    }

    {
        s32 mtu, rwin;

        mtu = config->mtu ? config->mtu : 576;
        if (mtu > 1500) {
            mtu = 1500;
        } else if (mtu < 576) {
            mtu = 576;
        }
        rwin = config->rwin ? config->rwin : 0x10c0;
        cps->mymss = mtu - 40;
        SOCLSocketParamTCP.buffer.rcvbuf_size = rwin;
        SOCLSocketParamTCP.buffer.rcvbuf_consume_min = cps->mymss * 2;
    }

    CPSMyIp = 0;
    if (config->use_dhcp) {
        SOCLiDhcpState = SOCLI_DHCP_REQUESTED;
        cps->mode = 0;
        cps->dhcp_callback = SOCLi_DhcpTimeout;
        cps->requested_ip = SOCLiRequestedIP;
    } else {
        SOCLiDhcpState = 0;
        cps->mode = CPS_MODE_STATIC_IP;
        cps->dhcp_callback = SOCLi_SetMyIP;
    }
    CPS_SetThreadPriority(config->cps_thread_prio ? config->cps_thread_prio : 11);
    WCM_SetRecvDCFCallback(CPSi_RecvCallback);
    CPS_SetScavengerCallback(SOCLi_TrashSocket);
    CPS_SetAddressCheckCallback(SOCLi_AddressCheckCallback);
    CPS_Startup(cps);
}

// The address of the config's own, which CPS calls for when it does without DHCP
static void SOCLi_SetMyIP(void) {
    SOCLConfig *config = SOCLiConfigPtr;

    CPSMyIp = config->host_ip.my_ip;
    CPSNetMask = config->host_ip.net_mask;
    CPSGatewayIp = config->host_ip.gateway_ip;
    CPSDnsIp[0] = config->host_ip.dns_ip[0];
    CPSDnsIp[1] = config->host_ip.dns_ip[1];
    SOCLiDhcpState |= SOCLI_DHCP_DONE;
}

static void SOCLi_DhcpTimeout(void) {
    SOCLiDhcpState |= SOCLI_DHCP_DONE;
}

static BOOL SOCL_LinkIsOn(void) {
    if (WCM_GetApMacAddress() != NULL) {
        return TRUE;
    }
    return FALSE;
}
