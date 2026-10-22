#ifndef POKEBW2_DWC_GSPLATFORMSOCKET_H
#define POKEBW2_DWC_GSPLATFORMSOCKET_H

#include "dwc/nonport.h"
#include "nitrowifi/soc.h"

// GameSpy's sockets (gsPlatformSocket.c), in overlay 11: the BSD calls that GameSpy's code makes, over NitroWiFi's
// SOC calls. A call that fails returns -1 and keeps NitroWiFi's error for GOAGetLastError. The addresses are given as
// NitroWiFi's socket addresses, with their size

// The BSD names that GameSpy gives NitroWiFi's types on the DS
typedef int SOCKET;
typedef SOCSockAddr SOCKADDR;
typedef SOCSockAddrIn SOCKADDR_IN;
typedef SOCInAddr IN_ADDR;
typedef SOCHostEnt HOSTENT;

int gsiSocket(int family, int type, int protocol);
int closesocket(SOCKET sock);
// Closes a socket of the HTTP code: gsi_true once it is closed, gsi_false while it is still closing; secure is the
// connection's flag that it flips on a close that worked and sets on an error
gsi_bool gsiCloseSocketSecure(SOCKET sock, gsi_bool *secure);
int shutdown(SOCKET sock, int how);
int bind(SOCKET sock, const SOCKADDR *address, int size);
int connect(SOCKET sock, const SOCKADDR *address, int size);
int recv(SOCKET sock, char *buffer, int size, int flags);
int recvfrom(SOCKET sock, char *buffer, int size, int flags, SOCKADDR *address, int *addressSize);
int send(SOCKET sock, const char *buffer, int size, int flags);
int sendto(SOCKET sock, const char *buffer, int size, int flags, const SOCKADDR *address, int addressSize);
// Sets nothing: NitroWiFi has no socket options
int setsockopt(SOCKET sock, int level, int name, const char *value, int size);
int getsockname(SOCKET sock, SOCKADDR *address, int *size);
// The address of a text of four numbers, or -1
unsigned long inet_addr(const char *host);
int GOAGetLastError(SOCKET sock);
// Polls a socket at once: sets each flag that is not NULL to whether it can be read, written or has failed, and
// returns how many of them are set, or -1
int GSISocketSelect(SOCKET sock, int *readable, int *writable, int *failed);
int SetSockBlocking(SOCKET sock, int blocking);
int SetReceiveBufferSize(SOCKET sock, int size);
int CanReceiveOnSocket(SOCKET sock);
int CanSendOnSocket(SOCKET sock);
// The host entry of the console, with its address, or NULL while it has none
HOSTENT *getlocalhost(void);
// Whether an address (in network order) is a private one: 10.x.x.x, 172.16.x.x to 172.31.x.x or 192.168.x.x
int IsPrivateIP(IN_ADDR *address);

#endif // POKEBW2_DWC_GSPLATFORMSOCKET_H
