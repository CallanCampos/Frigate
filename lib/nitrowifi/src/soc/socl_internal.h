#ifndef POKEBW2_NITROWIFI_SOCL_INTERNAL_H
#define POKEBW2_NITROWIFI_SOCL_INTERNAL_H

#include "nitrowifi/soc.h"

// SOCL's state, shared by its files

// socl_startup.c
extern SOCLConfig *SOCLiConfigPtr;     // The config SOCL_Startup was given, NULL while SOCL is down
extern u32 SOCLiDefaultLanBufferSize;  // The LAN buffer's size when the config gives none
extern CPSConfig SOCLiCPSConfig;

// socl_command.c
int SOCLi_StartupCommandPacketQueue(u32 max);

// socl_close.c
void SOCLi_TrashSocket(void);

// socl_cleanup.c
// Shuts CPS down after a failed SOCL_Startup (no counterpart in pokediamond's NitroWiFi)
void func_ov011_02157688(void);

// socl_list.c
// Prepares the socket lists before SOCL starts (no counterpart in pokediamond's NitroWiFi)
void func_ov011_02157980(void);
// CPS's address check callback (no counterpart in pokediamond's NitroWiFi)
BOOL func_ov011_02157a48(void);

#endif // POKEBW2_NITROWIFI_SOCL_INTERNAL_H
