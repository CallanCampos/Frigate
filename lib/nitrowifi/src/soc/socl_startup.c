#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "dwc/wcm.h"
#include "gfl/std.h"
#include "nitro/os.h"

// SOCL's startup: it starts CPS with a configuration made from SOCL's, then the command packet queue and the socket
// that sends UDP datagrams. pokediamond's NitroWiFi names the object socl_startup.o and every function in it.
static int SOCLi_StartupSOCL(void);
static void SOCLi_StartupCPS(void);
static void SOCLi_SetMyIP(void);
static void SOCLi_DhcpTimeout(void);
static BOOL SOCL_LinkIsOn(void);

// NitroWiFi's signature, "[SDK+NINTENDO:WiFi3.3.30052.201006221129]", in the main module
extern const char SOCLi_Signature[];

SOCLConfig *SOCLiConfigPtr = NULL;
u32 SOCLiDefaultLanBufferSize = 0x4000;

static u32 SOCLiYieldWait;
CPSInAddr SOCLiRequestedIP;
u32 SOCLiDhcpState;
SOCLSocket *SOCLiUDPSendSocket;
CPSConfig SOCLiCPSConfig;

int SOCL_Startup(SOCLConfig *config) {
    int result;

    func_02004eb0(SOCLi_Signature);
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
    SOCLConfig *config = SOCLiConfigPtr;
    CPSConfig *cps = &SOCLiCPSConfig;
    s32 mtu;
    s32 rwin;

    sys_memset(cps, 0, sizeof(CPSConfig));
    cps->alloc = config->alloc;
    cps->free = config->free;
    cps->link_is_on = SOCL_LinkIsOn;
    cps->random_seed = 0;
    cps->yield_wait = SOCLiYieldWait;
    cps->lan_buflen = config->lan_buffer_size ? config->lan_buffer_size : SOCLiDefaultLanBufferSize;
    cps->lan_buf = config->lan_buffer ? config->lan_buffer : SOCLiConfigPtr->alloc(cps->lan_buflen);

    mtu = config->mtu;
    if (mtu == 0) {
        mtu = 576;
    }
    if (mtu > 1500) {
        mtu = 1500;
    } else if (mtu < 576) {
        mtu = 576;
    }
    rwin = config->rwin;
    if (rwin == 0) {
        rwin = 0x10c0;
    }
    cps->mymss = mtu - 40;
    SOCLSocketParamTCP.buffer.rcvbuf_size = rwin;
    SOCLSocketParamTCP.buffer.rcvbuf_consume_min = cps->mymss * 2;

    CPSMyIp = 0;
    if (config->use_dhcp) {
        cps->mode = 0;
        SOCLiDhcpState = SOCLI_DHCP_REQUESTED;
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
