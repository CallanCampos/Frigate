#ifndef POKEBW2_APP_MUSICAL_STA_ACTING_H
#define POKEBW2_APP_MUSICAL_STA_ACTING_H

// Overlay 209's sta_acting.c: the musical's stage, which plays the program's script with the Pokémon, the
// background, the objects, the lights, the effects and the audience. Its init, main and term functions are declared
// in field/musical_stage_sys.h for overlay 12's proc

#include "types.h"
#include "app/musical/mus_item_data.h"
#include "app/musical/sta_act_obj.h"
#include "struct_decls.h"

StaActPokeSys *StaActing_GetPokeSys(StaActing *stage);
StaActPoke *StaActing_GetPoke(StaActing *stage, u8 pos);
// How far the stage has scrolled, in pixels
u16 StaActing_GetScrollOffset(StaActing *stage);
// The position of the Pokémon the spotlight follows, or 4 for none
u8 StaActing_GetLightUpPoke(StaActing *stage);
StaActEffectSys *StaActing_GetEffectSys(StaActing *stage);
// How many times the props' effects run this frame
u32 StaActing_GetUpdateCount(StaActing *stage);
StaActLightSys *StaActing_GetLightSys(StaActing *stage);
StaActLight *StaActing_GetLight(StaActing *stage, u8 index);
// How far the curtain has opened, from 0 (closed) to 0xe0
u16 StaActing_GetCurtainOffset(StaActing *stage);
void StaActing_SetCurtainOffset(StaActing *stage, u16 offset);
// Where the stage scrolls to
void StaActing_SetScrollTarget(StaActing *stage, u16 target);
// Whether a script scrolls the stage, which then doesn't follow the player's Pokémon
void StaActing_SetScriptScroll(StaActing *stage, BOOL scroll);
// Whether the stage stays where the Pokémon lined up at the end
void StaActing_SetScrollFixed(StaActing *stage, BOOL fixed);
// The buttons of the player's props
void StaActing_EnableButtons(StaActing *stage);
void StaActing_DisableButtons(StaActing *stage);
// When the other Pokémon use their props, chosen at random by a timing, 0 to 2, and for timing 1 only when the
// Pokémon at pos appeals
void StaActing_SetNpcItemTiming(StaActing *stage, u32 timing, u8 pos);
StaActObjSys *StaActing_GetObjSys(StaActing *stage);
StaActObj *StaActing_GetObj(StaActing *stage, u8 index);
void StaActing_SetObj(StaActing *stage, StaActObj *obj, u8 index);
StaActEffect *StaActing_GetEffect(StaActing *stage, u8 index);
void StaActing_SetEffect(StaActing *stage, StaActEffect *effect, u8 index);
void StaActing_SetLight(StaActing *stage, StaActLight *light, u8 index);
StaActAudience *StaActing_GetAudience(StaActing *stage);
// The points of the Pokémon at pos before the show, and those it won during it
u8 StaActing_GetPoints(StaActing *stage, u8 pos);
u8 StaActing_GetBonusPoints(StaActing *stage, u8 pos);
// The sparkles and the sound of the Pokémon at pos appealing
void StaActing_StartAppealEffect(StaActing *stage, u8 pos);
// Makes the audience look at the Pokémon at pos, or with focus FALSE at none
void StaActing_SetAudienceFocus(StaActing *stage, u8 pos, BOOL focus);
// Makes the audience look again at the lights
void StaActing_RefreshAudience(StaActing *stage);
// The applause, by the points of the Pokémon that did worst
void StaActing_PlayApplause(StaActing *stage);
// Runs the appeal script of a file for the Pokémon at pos, which is one of three by its points
void StaActing_StartAppealScript(StaActing *stage, u8 file, u8 pos);
// Runs the script of a file for the Pokémon of a mask
void StaActing_StartPokeScript(StaActing *stage, u8 file, u8 pokeMask);
// Prints a message of the stage's text, at once when wait is 0xff
void StaActing_PrintMessage(StaActing *stage, u16 msgId, u32 wait);
void StaActing_ClearMessage(StaActing *stage);
// The program's sound: names guessed from the flags the stage keeps for them
void StaActing_PlayStrm(StaActing *stage, u16 seq);
void StaActing_PlaySeq(StaActing *stage, u16 seq);
void StaActing_StopSeq(StaActing *stage);
void StaActing_PlayWave(StaActing *stage, u16 waveArc, u16 a2, u16 a3);
void StaActing_PlayBgm(StaActing *stage, u32 bgm);
void StaActing_StopBgm(StaActing *stage);
// Overlay 210's table of the props, which the item draw system holds
MusItemData *StaActing_GetItemData(StaActing *stage);
// Asks to use the prop at an equip position of the player's Pokémon
void StaActing_UseItem(StaActing *stage, u32 equipPos);
// Whether a prop of the player's Pokémon is being used
BOOL StaActing_IsUsingItem(StaActing *stage);

// A constant of sta_acting.c that only an accessor nothing calls reads, which puts it among the file's other data
extern const u32 STA_ACTING_UNUSED;

static inline u32 StaActing_GetUnused(void) {
    return STA_ACTING_UNUSED;
}

#endif // POKEBW2_APP_MUSICAL_STA_ACTING_H
