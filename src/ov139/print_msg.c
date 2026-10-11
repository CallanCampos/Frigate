// print_msg.c, a guessed name: prints strings into a window through the print queue, aligned left, right or centered

#include "app/ui/print_msg.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_main_12.h"

static u16 GetStrWidthForAlign(const StrBuf *strbuf, Font *font, u32 align);

void PrintStrAligned(PrintWindow *window, PrintQueue *queue, u16 x, u16 y, const StrBuf *strbuf, Font *font,
                         u16 color, u32 align) {
    x -= GetStrWidthForAlign(strbuf, font, align);
    PrintWindow_Print(window, queue, x, y, strbuf, font, color);
}

// Prints "number1/number2" with the slash centered at x
void PrintFraction(PrintWindow *window, PrintQueue *queue, Font *font, u16 x, u16 y, u16 color, s32 number1,
                         s32 number2, HeapID heapId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_12, heapId);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);
    StrBuf *buf = GFL_StrBufCreate(10, heapId);
    StrBuf *label = GFL_MsgDataLoadStrbufNew(msgData, BtlMain12_Text_Slash);
    u16 width = GFL_FontGetBlockWidth(label, font, 0);
    StrBuf *numberFormat;

    PrintStrAligned(window, queue, x, y, label, font, color, PRINT_ALIGN_CENTER);
    x -= GetStrWidthForAlign(label, font, PRINT_ALIGN_CENTER);
    GFL_StrBufFree(label);

    numberFormat = GFL_MsgDataLoadStrbufNew(msgData, BtlMain12_Text_Numerator);
    WordSetNumber(wordSet, 0, number1, 8, 0, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, buf, numberFormat);
    PrintStrAligned(window, queue, x, y, buf, font, color, PRINT_ALIGN_RIGHT);
    GFL_StrBufFree(numberFormat);

    numberFormat = GFL_MsgDataLoadStrbufNew(msgData, BtlMain12_Text_Denominator);
    WordSetNumber(wordSet, 0, number2, 8, 0, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, buf, numberFormat);
    PrintStrAligned(window, queue, x + width, y, buf, font, color, PRINT_ALIGN_LEFT);
    GFL_StrBufFree(numberFormat);

    GFL_StrBufFree(buf);
    GFL_WordSetSystemFree(wordSet);
    GFL_MsgDataFree(msgData);
}

// The offset to subtract from x: the whole width to align right, half of it to center
static u16 GetStrWidthForAlign(const StrBuf *strbuf, Font *font, u32 align) {
    if (align == PRINT_ALIGN_RIGHT) {
        return GFL_FontGetBlockWidth(strbuf, font, 0);
    }
    if (align == PRINT_ALIGN_CENTER) {
        return GFL_FontGetBlockWidth(strbuf, font, 0) >> 1;
    }
    return 0;
}
