#ifndef POKEBW2_FIELD_GAME_BEACON_SET_H
#define POKEBW2_FIELD_GAME_BEACON_SET_H

// Overlay 12's game_beacon_set.c (a descriptive name): the field's messages to nearby players, which it puts in the
// beacon that ARM9 main's g_GameBeaconSys broadcasts. Each message has a public function, which checks that the
// message may be sent, and a function that fills the beacon in

#include "types.h"
#include "pml/item.h"
#include "struct_decls.h"
#include "system/game_beacon.h"

void func_ov012_0215f958(u16 value);
void func_ov012_0215f994(u16 value);
void func_ov012_0215fa90(u16 itemId);
void func_ov012_0215faf0(u16 itemId);
void func_ov012_0215fb50(u16 itemId);
void func_ov012_0215fbb0(u16 itemId);
void func_ov012_0215fc10(u16 itemId);
void func_ov012_0215fc70(u16 itemId);
void func_ov012_0215fcd0(u32 passPower);
void func_ov012_0215fd50(u16 value);
void func_ov012_0215fdbc(void);
void func_ov012_0215fdec(u32 value);
void func_ov012_0215fe3c(void);
void func_ov012_0215fe6c(u16 value);
void func_ov012_0215febc(u16 value, const StrBuf *name);
void func_ov012_0215ff20(u16 value, const StrBuf *name);
void func_ov012_0215ff84(u16 value, const StrBuf *name);
void func_ov012_0215ffe8(const StrBuf *name);
void func_ov012_0216002c(u16 value);
void func_ov012_02160080(void);
void func_ov012_021600d0(u16 species);
void func_ov012_02160124(void);
void func_ov012_02160174(void);
void func_ov012_021601a4(u32 value);
void func_ov012_021601dc(void);
void func_ov012_02160210(void);
void func_ov012_02160240(u8 value);
void GameBeacon_BroadcastFerrisWheel(void);
void func_ov012_021602cc(void);
// That the player is in a musical with the Pokémon of that name
void func_ov012_021602fc(const StrBuf *name);
void func_ov012_02160340(u32 passPower);
void func_ov012_02160390(GameData *gameData, u16 value);
void func_ov012_0216045c(u8 value);
void func_ov012_02160488(u8 value);
void func_ov012_021604c4(u8 value);
void func_ov012_021604f4(u8 value, u8 kind);
void func_ov012_0216052c(u8 value, u8 extra);
// Sends a beacon of type 0x39, if GameBeaconSys_CanSendType allows it
void func_ov012_02160574(void);
void func_ov012_021605a4(u8 extra, u16 value);
void func_ov012_021605d4(u8 extra, u16 value);
void func_ov012_02160618(void);
void func_ov012_0216063c(u8 type, u16 value);

#endif // POKEBW2_FIELD_GAME_BEACON_SET_H
