#ifndef POKEBW2_APP_DIGITALCARDCHECK_H
#define POKEBW2_APP_DIGITALCARDCHECK_H

#include "types.h"
#include "app/wifibattlematch_graphic.h"
#include "app/wifibattlematch_util.h"
#include "app/wifibattlematch_view.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Overlay 261's digitalcardcheck.c (named by its embedded string): the screen of the battle competitions' digital
// player card, which the player checks and can unregister from the competition. Swan has none of these; the names are
// ours

typedef struct DigitalCardCheck DigitalCardCheck;

// Which competition's card
enum {
    DIGITAL_CARD_CHECK_WIFI,
    DIGITAL_CARD_CHECK_LIVE,
};

typedef struct {
    // A DIGITAL_CARD_CHECK_*
    u32 mode;
    WifiBattleMatchMsgBox *msgBox;
    Font *font;
    // The Entree Forest's messages
    MsgData *msgData;
    PrintQueue *printQueue;
    WifiBattleMatchRes *seq;
    WifiBattleMatchGraphic *net;
    GameData *gameData;
} DigitalCardCheckParam;

DigitalCardCheck *DigitalCardCheck_Create(const DigitalCardCheckParam *param, HeapID heapId);
void DigitalCardCheck_Delete(DigitalCardCheck *check);
// Called every frame
void DigitalCardCheck_Update(DigitalCardCheck *check);
BOOL DigitalCardCheck_IsFinished(DigitalCardCheck *check);

#endif // POKEBW2_APP_DIGITALCARDCHECK_H
