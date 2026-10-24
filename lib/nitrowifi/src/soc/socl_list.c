#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL's lists of sockets: the live ones, and the closed ones that SOCLi_TrashSocket frees. The singly linked lists
// chain through SOCLSocket.next. The file and function names are NitroWiFi's: pokediamond's NitroWiFi names the
// object socl_list.o, and Black 1's decompilation (aletheia) has the same functions in its socl_list.c.

static void SOCLi_SocketRegisterList(SOCLSocket **list, SOCLSocket *socket);
static void SOCLi_SocketUnregisterList(SOCLSocket **list, SOCLSocket *socket);
static SOCLSocket **SOCLi_SocketGetNextPtr(SOCLSocket **list, SOCLSocket *socket);

// MWCC lays these out in the reverse of their definition order
SOCLSocket *SOCLiSocketListTrash;
SOCLSocket *SOCLiSocketList;

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
    OSIntrMode irq = OS_DisableInterrupts();
    SOCLSocket **link = SOCLi_SocketGetNextPtr(list, socket);

    if (link != NULL) {
        *link = socket->next;
    }
    OS_RestoreInterrupts(irq);
}

// The pointer that points to the socket (the list's head or a socket's next), NULL when it is not in the list
static SOCLSocket **SOCLi_SocketGetNextPtr(SOCLSocket **list, SOCLSocket *socket) {
    SOCLSocket **link = list;
    SOCLSocket *s;
    OSIntrMode irq = OS_DisableInterrupts();

    for (s = *link; s != NULL; s = *link) {
        if (s == socket) {
            OS_RestoreInterrupts(irq);
            return link;
        }
        link = &s->next;
    }
    OS_RestoreInterrupts(irq);
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
