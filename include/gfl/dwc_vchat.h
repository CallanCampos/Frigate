#ifndef POKEBW2_GFL_DWC_VCHAT_H
#define POKEBW2_GFL_DWC_VCHAT_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/mic.h"

// dwc_rap_vchat.c, in overlay 29 ahead of Nintendo's voice chat library: the game's side of the Wi-Fi voice chat. The
// ROM names no file

// Runs the voice chat a frame; stalled is whether the connection is too slow to send the voice
void DWCRapVChat_Update(BOOL stalled);
// Hands a packet that came from a machine to the library, returning whether it was the library's
BOOL DWCRapVChat_ReceivePacket(int aid, u8 *data, int size);
// Starts the voice chat in a heap, coded as the library's codec number says, for some machines
void DWCRapVChat_Start(HeapID heapId, int codec, int count);
void DWCRapVChat_RequestEnd(void);
// Sets the function called once the voice chat has ended
void DWCRapVChat_SetEndCallback(void (*callback)(void));
void DWCRapVChat_End(void);
BOOL DWCRapVChat_GetSpeaking(void);
BOOL DWCRapVChat_AddHeard(u32 netIds, int myNetId);
BOOL DWCRapVChat_RemoveHeard(int myNetId);
void DWCRapVChat_SetMuted(BOOL muted);

#endif // POKEBW2_GFL_DWC_VCHAT_H
