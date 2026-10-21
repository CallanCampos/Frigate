#ifndef POKEBW2_FIELD_FLD_FACEUP_H
#define POKEBW2_FIELD_FLD_FACEUP_H

#include "types.h"
#include "struct_decls.h"

// The face-up, overlay 155 (fld_faceup.c): a close-up of a character's face over the field on BGs 2 and 3, whose eyes
// blink and whose mouth moves while the script's message prints. The scenes with N play it through scrcmd_ndemo.c

// Fades in the face of type 0 to 2 and returns the event that does it, or NULL if a face is already up. With
// endBlack, the end leaves the screen black
GameEvent *EventFaceUpStart_Create(u8 type, u8 unused, u16 endBlack, GameSystem *gsys, FieldScriptEnv *env);
// The event that fades the face out and frees it, or NULL if there is none
GameEvent *EventFaceUpEnd_Create(GameSystem *gsys);
// Frees the face at once
void FaceUp_FreeNow(Field *field);
// Lets the mouth move with the next message
void FaceUp_StartTalk(Field *field);

#endif // POKEBW2_FIELD_FLD_FACEUP_H
