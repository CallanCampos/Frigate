#ifndef POKEBW2_FIELD_UNION_MSG_H
#define POKEBW2_FIELD_UNION_MSG_H

#include "types.h"
#include "field/field.h"
#include "field/union_comm.h"
#include "struct_decls.h"

// union_msg.c, overlay 34, which the ROM names: the Union Room's messages, menus and their tables. Names are not
// swan's. The first functions are lookups of message IDs, by gender, language or a random pick

// The rules of a regulation printed (PokeRegulation_CreatePrintMsg): its message data and, for each of the six rules, the name of the
// rule and its value. alloc.rpm of the system holds one
typedef struct {
    MsgData *msgData;
    StrBuf *rule[6];
    StrBuf *value[6];
    u8 unk34;
} RegulationRulesText;

// Main code that overlay 34 is the only user of yet
void *PokeRegulation_CreatePrintMsg(Regulation *regulation, WordSet *wordset, HeapID heapId, BOOL battleType);
void PokeRegulation_FreePrintMsg(void *rulesText);
StrBuf *makeStrbufLoadBattleTypeText(Regulation *regulation, HeapID heapId);

// Overlay 36's windows and menus that overlay 34 uses and field.h doesn't declare yet
void func_ov036_02188b08(void *window);
void func_ov036_02188b18(void *window, u32 x, u32 y, u32 messageId);
void func_ov036_02188b44(void *window, u32 x, u32 y, StrBuf *strbuf);
void func_ov036_02188b70(void *window, u32 x, u16 y, StrBuf *strbuf, u32 color);
BOOL func_ov036_0218798c(void *msgBGSys);
ListMenuOption *func_ov036_02187f0c(const void *rows, u32 count, MsgData *msgData, HeapID heapId);

// The message IDs of the Union Room's texts, for a gender (0 or 1) or a language. The tables are of two IDs
u16 func_ov034_0217aa7c(UnionSystem *unisys);
u16 func_ov034_0217aa94(u8 gender);
u16 func_ov034_0217aaa0(u8 gender);
u16 func_ov034_0217aaac(u8 gender);
u16 func_ov034_0217aab8(u32 row, u8 gender);
u16 func_ov034_0217aacc(u32 row, u8 gender);
u16 func_ov034_0217aae0(u32 row);
u16 func_ov034_0217aaf0(u32 row);
u16 func_ov034_0217ab00(u8 gender);
u16 func_ov034_0217ab0c(u8 gender);
u16 func_ov034_0217ab18(u8 gender);
u16 func_ov034_0217ab24(u8 gender);
u16 func_ov034_0217ab30(UnionSystem *unisys);
u16 func_ov034_0217ab48(UnionSystem *unisys);
u16 func_ov034_0217ab60(u8 gender);
u16 func_ov034_0217ab7c(u8 gender);
u16 func_ov034_0217ab88(u8 gender);
u16 func_ov034_0217aba4(u8 gender);
u16 func_ov034_0217abc0(u8 gender);
u16 func_ov034_0217abdc(u8 gender);
u16 func_ov034_0217abf8(u8 gender);
u16 func_ov034_0217ac04(u8 gender);
u16 func_ov034_0217ac10(u8 gender);
u16 func_ov034_0217ac1c(u8 gender);
u16 func_ov034_0217ac28(u8 gender);
u16 func_ov034_0217ac34(u8 gender);
u16 func_ov034_0217ac40(u8 gender);
u16 func_ov034_0217ac4c(u8 gender);
u16 func_ov034_0217ac58(u8 gender);
u16 func_ov034_0217ac64(u8 gender);
u16 func_ov034_0217ac70(u8 gender);
u16 func_ov034_0217ac7c(u8 gender);
u16 func_ov034_0217ac88(u8 gender);
u16 func_ov034_0217ac94(u8 gender);
u16 func_ov034_0217aca0(u8 gender);
u16 func_ov034_0217acac(u8 gender);
u16 func_ov034_0217acb8(u8 gender);
// The message of a trainer card's origin: nothing, another country, another province or the same
u32 UnionMsg_GetOriginMessage(UnionSystem *unisys, u8 country, u8 province);
s32 UnionMsg_GetTableByte(s32 index);
// Frees the message system, the menus and the windows
void UnionMsg_FreeAll(UnionSystem *unisys);
// Creates the message system (the message data, the stream window, the word set) or reopens its window
void UnionMsg_CreateMsgSystem(UnionSystem *unisys, Field *field);
void UnionMsg_PrintMessage(UnionSystem *unisys, u32 messageId);
BOOL UnionMsg_IsPrintDone(UnionSystem *unisys);
// Creates the message system, sets the words and prints a message
void UnionMsg_CreateAndPrint(UnionSystem *unisys, Field *field, u32 messageId);
void UnionMsg_CreateYesNo(UnionSystem *unisys, Field *field);
void UnionMsg_FreeYesNo(UnionSystem *unisys);
BOOL UnionMsg_UpdateYesNo(UnionSystem *unisys, BOOL *yes);
void UnionMsg_CreateMainMenu(UnionSystem *unisys, Field *field);
void UnionMsg_FreeMainMenu(UnionSystem *unisys);
s32 UnionMsg_UpdateMainMenu(UnionSystem *unisys, Field *field);
void UnionMsg_CreateRuleMenu(UnionSystem *unisys, Field *field, int kind, u8 *ruleSel);
void UnionMsg_FreeRuleMenu(UnionSystem *unisys);
u32 UnionMsg_UpdateRuleMenu(UnionSystem *unisys, BOOL *picked, u8 *ruleSel, BOOL *isFinal, Field *field);
void UnionMsg_CreateInfoWindow(UnionSystem *unisys, Field *field);
void UnionMsg_FreeInfoWindow(UnionSystem *unisys);
void UnionMsg_CreateRulesWindow(UnionSystem *unisys, Field *field, u32 okMask, u32 battleType, u32 mode);
BOOL UnionMsg_IsRulesPrinted(UnionSystem *unisys, Field *field);
void UnionMsg_FreeRulesWindow(UnionSystem *unisys);

#endif // POKEBW2_FIELD_UNION_MSG_H
