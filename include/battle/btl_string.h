#ifndef POKEBW2_BATTLE_BTL_STRING_H
#define POKEBW2_BATTLE_BTL_STRING_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"

// Overlay 167's btl_string.c, a guessed name: the battle's message strings, which it loads and fills with the
// names and numbers their arguments give

void BtlString_Init(BtlMainModule *mainModule, u8 clientId, const BtlPokeCon *pokeCon, HeapID heapId);
void BtlString_Exit(void);
// A standard message with count arguments that follow
void BtlString_FormatStdMsg(StrBuf *strbuf, u16 message, u32 count, ...);
void BtlString_FormatStdMsgArgs(StrBuf *strbuf, u16 message, const u32 *args);
void BtlString_FormatSetMsg(StrBuf *strbuf, u16 message, const u32 *args);
// "<Pokémon> used <move>!"
void BtlString_FormatMoveMsg(StrBuf *strbuf, u8 monId, u16 move);
void func_ov167_021d5700(StrBuf *strbuf, u16 arg1, u32 arg2);
void BtlString_LoadUIMsg(StrBuf *strbuf, u16 message);
void func_ov167_021d5770(StrBuf *strbuf, u16 message, const u32 *args);
void func_ov167_021d57b0(StrBuf *strbuf, u16 message, const u32 *args);
// The six values of a level up's stat window, two digits each
void BtlString_FormatLevelUpGains(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense,
                                  int speed);
// The same with three digits each
void BtlString_FormatLevelUpStats(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense,
                                  int speed);
void BtlString_FormatMovieScriptMsg(StrBuf *strbuf, u32 message);
void BtlString_FormatMovieMsg(StrBuf *strbuf, u16 message);
void BtlString_SetStudioWords(void);

#endif // POKEBW2_BATTLE_BTL_STRING_H
