#ifndef POKEBW2_DWC_GSDEBUG_H
#define POKEBW2_DWC_GSDEBUG_H

#include "dwc/nonport.h"
#include <stdarg.h>

// GameSpy's debug output (gsDebug.c), in overlay 11. A message has a category, a type and a level (bits); it is shown
// when the table of levels has its level for its category and type, through the callback that the game set or else
// on the screen (screen.c)

// The levels a message is reported at, and the masks of the levels a category and type shows
typedef gsi_u8 GSIDebugLevel;
#define GSIDebugLevel_HotError ((GSIDebugLevel)(1 << 0))
#define GSIDebugLevel_WarmError ((GSIDebugLevel)(1 << 1))
#define GSIDebugLevel_Warning ((GSIDebugLevel)(1 << 2))
#define GSIDebugLevel_Notice ((GSIDebugLevel)(1 << 3))
#define GSIDebugLevel_Comment ((GSIDebugLevel)(1 << 4))
#define GSIDebugLevel_RawDump ((GSIDebugLevel)(1 << 5))
#define GSIDebugLevel_StackTrace ((GSIDebugLevel)(1 << 6))
#define GSIDebugLevel_None ((GSIDebugLevel)0)
#define GSIDebugLevel_Normal ((GSIDebugLevel)0x07)
#define GSIDebugLevel_Debug ((GSIDebugLevel)0x0F)
#define GSIDebugLevel_Verbose ((GSIDebugLevel)0x1F)
#define GSIDebugLevel_Hardcore ((GSIDebugLevel)0xFF)

typedef enum {
    GSIDebugType_Network,
    GSIDebugType_File,
    GSIDebugType_Memory,
    GSIDebugType_State,
    GSIDebugType_Misc,
    GSIDebugType_Count
} GSIDebugType;

// The SDKs that report. BW2's GameSpy has two categories after GSIDebugCat_CDKey that its code doesn't use and the ROM
// doesn't name, as its table of levels has 13 rows, so Common, the last, is 12
typedef enum {
    GSIDebugCat_App,
    GSIDebugCat_GP,
    GSIDebugCat_Peer,
    GSIDebugCat_QR2,
    GSIDebugCat_SB,
    GSIDebugCat_Voice,
    GSIDebugCat_AD,
    GSIDebugCat_NatNeg,
    GSIDebugCat_HTTP,
    GSIDebugCat_CDKey,
    GSIDebugCat_Common = 12,
    GSIDebugCat_Count
} GSIDebugCategory;

typedef void (*GSIDebugCallback)(GSIDebugCategory category, GSIDebugType type, GSIDebugLevel level, const char *format,
                                 va_list args);

void gsDebugVaList(GSIDebugCategory category, GSIDebugType type, GSIDebugLevel level, const char *format, va_list args);
void gsDebugFormat(GSIDebugCategory category, GSIDebugType type, GSIDebugLevel level, const char *format, ...);
// Shows size bytes, 16 to a line, as hexadecimal digits and as characters
void gsDebugBinary(GSIDebugCategory category, GSIDebugType type, GSIDebugLevel level, const char *data, gsi_i32 size);

#endif // POKEBW2_DWC_GSDEBUG_H
