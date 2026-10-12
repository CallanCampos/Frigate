#ifndef POKEBW2_SAVE_CHATTER_H
#define POKEBW2_SAVE_CHATTER_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Chatot's recorded cry, save block 0x24: 2000 samples of four bits
#define PERAP_VOICE_SAMPLES 2000
// The decoded cry: every sample four times, a second of sound, with silence after it
#define PERAP_VOICE_DECODED_SAMPLES (PERAP_VOICE_SAMPLES * 4)
#define PERAP_VOICE_DECODED_SIZE 8180

typedef struct {
    BOOL exists;
    u8 voice[PERAP_VOICE_SAMPLES / 2];
} PerapVoice;

u32 PerapVoice_GetSize(void);
void initChatotChatter(PerapVoice *chatter);
PerapVoice *allocChatotChatterBlk(HeapID heapId);
PerapVoice *getChatterBlockAddress(SaveControl *save);
BOOL doesChatotExist(PerapVoice *chatter);
void setChatotNotPresent(PerapVoice *chatter);
void setChatotPresent(PerapVoice *chatter);
void *getChatterVoice(PerapVoice *chatter);
// Decodes a recorded cry into PERAP_VOICE_DECODED_SIZE signed 8-bit samples
void PerapVoice_Decode(s8 *dest, const s8 *src);
// Stores a recording of PERAP_VOICE_DECODED_SAMPLES signed 8-bit samples as the cry
void PerapVoice_Encode(PerapVoice *chatter, const s8 *src);
void moveChatter(PerapVoice *dest, const PerapVoice *src);
// Forgets the recorded cry once no Chatot is in the party
void checkChatotInParty(PerapVoice *chatter, PokeParty *party);
// What a Chatter with the cry does, drawn from the cry: 0 for nothing, or 1 or 2
u8 PerapVoice_GetChatterEffect(PerapVoice *chatter);

#endif // POKEBW2_SAVE_CHATTER_H
