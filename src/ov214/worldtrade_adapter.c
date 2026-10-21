#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "pml/poke_party.h"
#include "system/app_keycursor.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/worldtrade.h"
#include "worldtrade_local.h"

// The Global Trade Station's helpers over the game's systems: Pokémon copies, window clearing, the text printing
// through the print queue and the message stream, and a small printer of numbers. The names of its functions are
// ours, guessed

#define PRINT_ENTRY_COUNT 24
#define NUM_FONT_ENTRY_COUNT 8

// The color the number printer prints in
#define NUM_FONT_COLOR 0x3dc8
// The color WorldTrade_Print prints in
#define PRINT_COLOR_DEFAULT 0x44f

struct WorldTradeNumFont {
    Font *font;
    PrintQueue *printQueue;
    WordSet *wordSet;
    MsgData *msgData;
    PrintWindow printWins[NUM_FONT_ENTRY_COUNT];
    BOOL active[NUM_FONT_ENTRY_COUNT];
    HeapID heapId;
};

void WorldTrade_SetBoxPkmNickname(WordSet *wordSet, u32 index, BoxPkm *pkm) {
    PartyPkm *partyPkm = boxPkmRegenToPartyPkm(pkm, HEAPID_WORLDTRADE);

    loadPokemonNicknameToStrbuf(wordSet, index, partyPkm);
    GFL_HeapFree(partyPkm);
}

PartyPkm *WorldTrade_AllocPartyPkm(HeapID heapId) {
    return GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), FALSE, "worldtrade_adapter.c", 71);
}

void WorldTrade_CopyPartyPkm(PartyPkm *src, PartyPkm *dest) {
    MI_CpuCopy8(src, dest, PokeParty_GetPkmRawSize());
}

void WorldTrade_ClearWindow(BmpWin *win, int mode) {
    BmpWin_ClearScreen(win);
    switch (mode) {
    case 0:
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
        break;
    case 1:
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win));
        break;
    }
}

StrBuf *WorldTrade_ExpandMessage(WordSet *wordSet, MsgData *msgData, u32 msgNo, HeapID heapId) {
    StrBuf *src = GFL_MsgDataLoadStrbufNew(msgData, msgNo);
    StrBuf *dest = GFL_StrBufCreate(0x200, heapId);

    GFL_WordSetFormatStrbuf(wordSet, dest, src);
    GFL_StrBufFree(src);
    return dest;
}

void WorldTrade_BoxPkmToPartyPkm(BoxPkm *pkm, PartyPkm *dest) {
    PartyPkm *partyPkm = boxPkmRegenToPartyPkm(pkm, HEAPID_WORLDTRADE);

    WorldTrade_CopyPartyPkm(partyPkm, dest);
    GFL_HeapFree(partyPkm);
}

BoxPkm *WorldTrade_GetBoxPkm(PartyPkm *pkm) {
    return PokeParty_ToBoxPkm(pkm);
}

int WorldTrade_GetStrWidth(WorldTradePrint *print, u8 font, StrBuf *str, int spacing) {
    return GFL_FontGetBlockWidth(str, print->font, (u16)spacing);
}

void WorldTrade_PrintInit(WorldTradePrint *print, TrainerDataSave *config) {
    int i;

    MI_CpuFill8(print, 0, sizeof(WorldTradePrint));
    print->tcbManager = GFL_TCBExMgrCreate(HEAPID_WORLDTRADE, HEAPID_WORLDTRADE, 32, 32);
    print->config = config;
    print->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_WORLDTRADE);
    print->printQueue = PrintQueue_CreateDefault(HEAPID_WORLDTRADE);
    print->keyCursor = KeyCursor_Create(15, 1, 1, HEAPID_WORLDTRADE);
    for (i = 0; i < PRINT_ENTRY_COUNT; i++) {
        WorldTradePrintEntry *entry = &print->entries[i];

        MI_CpuFill8(&entry->printWin, 0, sizeof(PrintWindow));
        entry->active = FALSE;
    }
}

void WorldTrade_PrintExit(WorldTradePrint *print) {
    KeyCursor_Free(print->keyCursor);
    PrintQueue_Free(print->printQueue);
    GFL_FontFree(print->font);
    GFL_TCBExMgrFree(print->tcbManager);
    MI_CpuFill8(print, 0, sizeof(WorldTradePrint));
}

void WorldTrade_PrintMain(WorldTradePrint *print) {
    int i;

    for (i = 0; i < PRINT_ENTRY_COUNT; i++) {
        WorldTradePrintEntry *entry = &print->entries[i];

        if (entry->active) {
            PrintWindow_Flush(&entry->printWin, print->printQueue);
            if (!entry->printWin.flushPending) {
                entry->active = FALSE;
            }
        }
    }

    if (print->stream != NULL) {
        KeyCursor_Update(print->keyCursor, print->stream, print->streamWin);
        switch (PrintStream_GetState(print->stream)) {
        case PRINT_STREAM_RUNNING:
            if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed()) {
                PrintStream_SkipWait(print->stream, 0);
            }
            break;
        case PRINT_STREAM_PAUSED:
            if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed()) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
                PrintStream_Continue(print->stream);
            }
            break;
        case PRINT_STREAM_DONE:
            PrintStream_Delete(print->stream);
            print->stream = NULL;
            print->streamWin = NULL;
            break;
        }
    }

    PrintQueue_Main(print->printQueue);
    GFL_TCBExMgrUpdate(print->tcbManager);
}

BOOL WorldTrade_PrintIsBusy(WorldTradePrint *print) {
    BOOL busy = FALSE;

    if (print->stream != NULL && PrintStream_GetState(print->stream) != PRINT_STREAM_DONE) {
        busy = TRUE;
    }
    return busy;
}

void WorldTrade_Print(BmpWin *win, u8 font, StrBuf *str, int x, int y, WorldTradePrint *print) {
    WorldTrade_PrintColor(win, font, str, x, y, 0, PRINT_COLOR_DEFAULT, print);
}

void WorldTrade_StreamPrint(BmpWin *win, u8 font, StrBuf *str, int x, int y, WorldTradePrint *print) {
    if (print->stream != NULL) {
        PrintStream_Delete(print->stream);
        print->stream = NULL;
    }
    print->stream = PrintStream_Create(win, x, y, str, print->font, TextSpeed_GetCurrentWait(), print->tcbManager, 0,
                                       HEAPID_WORLDTRADE, 15);
    print->streamWin = win;
}

void WorldTrade_PrintColor(BmpWin *win, u8 font, StrBuf *str, int x, int y, int unused, u16 color,
                           WorldTradePrint *print) {
    WorldTradePrintEntry *p_one = NULL;
    int i;

    // The last free entry
    for (i = 0; i < PRINT_ENTRY_COUNT; i++) {
        if (!print->entries[i].active) {
            p_one = &print->entries[i];
        }
    }
    GFL_ASSERT(p_one != NULL);

    p_one->printWin.window = win;
    p_one->printWin.flushPending = FALSE;
    PrintWindow_Print(&p_one->printWin, print->printQueue, x, y, str, print->font, color);
    p_one->active = TRUE;
}

void WorldTrade_PrintClear(WorldTradePrint *print) {
    int i;

    for (i = 0; i < PRINT_ENTRY_COUNT; i++) {
        WorldTradePrintEntry *entry = &print->entries[i];

        if (entry->active) {
            entry->active = FALSE;
        }
    }
    if (print->stream != NULL) {
        PrintStream_Delete(print->stream);
        print->stream = NULL;
        print->streamWin = NULL;
    }
}

WorldTradeNumFont *WorldTrade_NumFontCreate(u32 unused0, u32 unused1, u32 unused2, HeapID heapId) {
    WorldTradeNumFont *numFont =
        GFL_HeapAllocate(heapId, sizeof(WorldTradeNumFont), FALSE, "worldtrade_adapter.c", 477);

    MI_CpuFill8(numFont, 0, sizeof(WorldTradeNumFont));
    numFont->heapId = heapId;
    numFont->font = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, heapId);
    numFont->printQueue = PrintQueue_CreateDefault(heapId);
    numFont->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WORLDTRADE, heapId);
    numFont->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    return numFont;
}

void WorldTrade_NumFontDelete(WorldTradeNumFont *numFont) {
    GFL_WordSetSystemFree(numFont->wordSet);
    GFL_MsgDataFree(numFont->msgData);
    PrintQueue_Free(numFont->printQueue);
    GFL_FontFree(numFont->font);
    GFL_HeapFree(numFont);
}

void WorldTrade_NumFontMain(WorldTradeNumFont *numFont) {
    int i;

    for (i = 0; i < NUM_FONT_ENTRY_COUNT; i++) {
        if (numFont->active[i]) {
            PrintWindow_Flush(&numFont->printWins[i], numFont->printQueue);
            if (!numFont->printWins[i].flushPending) {
                numFont->active[i] = FALSE;
            }
        }
    }
    PrintQueue_Main(numFont->printQueue);
}

void WorldTrade_NumFontPrintNumber(WorldTradeNumFont *numFont, int num, int digits, int dispType, BmpWin *win, int x,
                                   int y) {
    StrBuf *str = GFL_StrBufCreate(32, numFont->heapId);
    StrBuf *format = GFL_StrBufCreate(32, numFont->heapId);
    int i;

    GFL_MsgDataLoadStrbuf(numFont->msgData, Worldtrade_Text_Empty_90, format);
    WordSetNumber(numFont->wordSet, 0, num, digits, dispType, TRUE);
    GFL_WordSetFormatStrbuf(numFont->wordSet, str, format);

    for (i = 0; i < NUM_FONT_ENTRY_COUNT; i++) {
        if (!numFont->active[i]) {
            numFont->printWins[i].window = win;
            numFont->printWins[i].flushPending = FALSE;
            PrintWindow_Print(&numFont->printWins[i], numFont->printQueue, x, y, str, numFont->font, NUM_FONT_COLOR);
            numFont->active[i] = TRUE;
            GFL_StrBufFree(format);
            GFL_StrBufFree(str);
            return;
        }
    }
    GFL_ASSERT(0);
}

void WorldTrade_NumFontPrintSlash(WorldTradeNumFont *numFont, int unused, BmpWin *win, int x, int y) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(numFont->msgData, Worldtrade_Text_Empty_91);
    int i;

    for (i = 0; i < NUM_FONT_ENTRY_COUNT; i++) {
        if (!numFont->active[i]) {
            numFont->printWins[i].window = win;
            numFont->printWins[i].flushPending = FALSE;
            PrintWindow_Print(&numFont->printWins[i], numFont->printQueue, x, y, str, numFont->font, NUM_FONT_COLOR);
            numFont->active[i] = TRUE;
            GFL_StrBufFree(str);
            return;
        }
    }
    GFL_ASSERT(0);
}
