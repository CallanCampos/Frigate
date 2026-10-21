#ifndef POKEBW2_DWC_GSPLATFORMSOCKET_H
#define POKEBW2_DWC_GSPLATFORMSOCKET_H

#include "dwc/nonport.h"
#include "nitrowifi/soc.h"

// GameSpy's sockets (gsPlatformSocket.c), in overlay 11: the BSD calls that GameSpy's code makes, over NitroWiFi's
// SOC calls. A call that fails returns -1 and keeps NitroWiFi's error for GOAGetLastError. The addresses are
// NitroWiFi's SocAddrIn, given with their size

int gsiSocket(int family, int type, int protocol);
int closesocket(int sock);
// Closes a socket of the HTTP code: gsi_true once it is closed, gsi_false while it is still closing; secure is the
// connection's flag that it flips on a close that worked and sets on an error
gsi_bool gsiCloseSocketSecure(int sock, gsi_bool *secure);
int shutdown(int sock, int how);
int bind(int sock, const SocAddrIn *address, int size);
int connect(int sock, const SocAddrIn *address, int size);
int recv(int sock, void *buffer, int size, int flags);
int recvfrom(int sock, void *buffer, int size, int flags, SocAddrIn *address, int *addressSize);
int send(int sock, const void *buffer, int size, int flags);
int sendto(int sock, const void *buffer, int size, int flags, const SocAddrIn *address, int addressSize);
// Sets nothing: NitroWiFi has no socket options
int setsockopt(int sock, int level, int name, const void *value, int size);
int getsockname(int sock, SocAddrIn *address, int *size);
// The address of a text of four numbers, or -1
int inet_addr(const char *host);
int GOAGetLastError(int sock);
// Polls a socket at once: sets each flag that is not NULL to whether it can be read, written or has failed, and
// returns how many of them are set, or -1
int GSISocketSelect(int sock, gsi_bool *readable, gsi_bool *writable, gsi_bool *failed);
gsi_bool SetSockBlocking(int sock, gsi_bool blocking);
gsi_bool SetReceiveBufferSize(int sock, int size);
gsi_bool CanReceiveOnSocket(int sock);
gsi_bool CanSendOnSocket(int sock);
// The host entry of the console, with its address, or NULL while it has none
SocHostEnt *getlocalhost(void);
// Whether an address (in network order) is a private one: 10.x.x.x, 172.16.x.x to 172.31.x.x or 192.168.x.x
gsi_bool IsPrivateIP(const gsi_u32 *address);

#endif // POKEBW2_DWC_GSPLATFORMSOCKET_H
