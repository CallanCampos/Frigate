#ifndef POKEBW2_APP_PDC_MSG_H
#define POKEBW2_APP_PDC_MSG_H

// Overlay 172's pdc_msg.c, the Entree Forest capture's message window on the bottom screen: a message with
// the Pokémon's or the player's name, printed at once or a character at a time, that fades in and out

#include "types.h"
#include "app/pdc.h"
#include "gfl/heap.h"
#include "save/config.h"
#include "struct_decls.h"

typedef struct PdcMsg PdcMsg;

// Overlay 172
PdcMsg *PdcMsg_Create(Font *font, const Config *config, TCBExManager *tcbManager, HeapID heapId);
void PdcMsg_Free(PdcMsg *msg);
// The messages: "The Pokémon your Pokémon saw in the dream appeared!", "What will the player do?", "Come here,
// Pokémon!", "Yes! Pokémon has joined your party!" and "The player left...". Each starts the window's fade in
void PdcMsg_StartSawDream(PdcMsg *msg, PdcWork *pdc);
void PdcMsg_StartWhatWill(PdcMsg *msg, PdcWork *pdc);
void PdcMsg_StartComeHere(PdcMsg *msg, PdcWork *pdc);
void PdcMsg_StartJoinedParty(PdcMsg *msg, PdcWork *pdc);
void PdcMsg_StartLeft(PdcMsg *msg, PdcWork *pdc);
// Runs a frame of the message, which returns whether it is done
BOOL PdcMsg_Step(PdcMsg *msg);
// Starts fading the window out, and whether that is done
void PdcMsg_StartHide(PdcMsg *msg);
BOOL PdcMsg_IsHideDone(PdcMsg *msg);
// Whether the message has finished printing, once
BOOL PdcMsg_ConsumeFinished(PdcMsg *msg);

#endif // POKEBW2_APP_PDC_MSG_H
