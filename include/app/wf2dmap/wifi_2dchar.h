#ifndef POKEBW2_APP_WF2DMAP_WIFI_2DCHAR_H
#define POKEBW2_APP_WF2DMAP_WIFI_2DCHAR_H

// The characters of the Wi-Fi Club's 2D map (overlay 203): their graphics and their cell actors with a shadow, named
// after the ROM's "wifi_2dchar.c". The older games have the same file; names follow its style

#include "types.h"
#include "gfl/clact.h"

// The characters. The Union Room ones are 0 to 15; that the other two are the male and female player is a guess
#define WF_2DC_UNIONCHARA_NUM 16
#define WF_2DC_CHARA_HERO 16
#define WF_2DC_CHARA_HEROINE 17
#define WF_2DC_CHARA_NUM 18

// How a character is animated. Names from the older games; the code shows that _NORMAL starts on another frame and
// that _TURN only ever stands
enum {
    WF_2DC_MOVERUN,
    WF_2DC_MOVENORMAL,
    WF_2DC_MOVETURN,
};

// The animations. The number in a name is how many frames it lasts
enum {
    // Stand
    WF_2DC_ANMWAY,
    // Spin until another animation starts
    WF_2DC_ANMROTA,
    WF_2DC_ANMWALK,
    WF_2DC_ANMTURN,
    WF_2DC_ANMRUN,
    WF_2DC_ANMWALLWALK,
    WF_2DC_ANMSLOWWALK,
    WF_2DC_ANMHIGHWALK2,
    WF_2DC_ANMHIGHWALK4,
    WF_2DC_ANMNONE,
    WF_2DC_ANMNUM,
};

typedef struct {
    s16 x;
    s16 y;
    s16 priority;
    s16 bgPriority;
} WF_2DC_WKDATA;

typedef struct WF_2DCSYS WF_2DCSYS;
typedef struct WF_2DCWK WF_2DCWK;

// unk1 is never used
WF_2DCSYS *WF_2DC_SysInit(ClActUnit *unit, void *unk1, u32 objNum, u32 heapId);
void WF_2DC_SysExit(WF_2DCSYS *sys);
void WF_2DC_ResSet(WF_2DCSYS *sys, u32 charaId, u32 vramType, u32 moveType, u32 heapId);
void WF_2DC_AllResDel(WF_2DCSYS *sys);
// Loads the Union Room characters, whose palettes go to plttOffset
void WF_2DC_UnionResSet(WF_2DCSYS *sys, u32 vramType, u32 moveType, u32 plttOffset, u32 heapId);
void WF_2DC_UnionResDel(WF_2DCSYS *sys);
void WF_2DC_ShadowResSet(WF_2DCSYS *sys, u32 vramType, u32 priority, u32 heapId);
void WF_2DC_ShadowResDel(WF_2DCSYS *sys);
WF_2DCWK *WF_2DC_WkAdd(WF_2DCSYS *sys, const WF_2DC_WKDATA *data, u32 charaId, u32 heapId);
void WF_2DC_WkDel(WF_2DCWK *wk);
void WF_2DC_WkMatrixSet(WF_2DCWK *wk, s16 x, s16 y);
void WF_2DC_WkDrawPriSet(WF_2DCWK *wk, u16 priority);
u32 WF_2DC_WkDrawPriGet(const WF_2DCWK *wk);
void WF_2DC_WkDrawFlagSet(WF_2DCWK *wk, BOOL flag);
void WF_2DC_WkShadowDrawFlagSet(WF_2DCWK *wk, BOOL flag);
BOOL WF_2DC_WkDrawFlagGet(const WF_2DCWK *wk);
// WF_2DC_MOVE*
u32 WF_2DC_WkMoveTypeGet(const WF_2DCWK *wk);
// WF_2DC_ANM*
u32 WF_2DC_WkAnmTypeGet(const WF_2DCWK *wk);
void WF_2DC_WkAnmFrameSet(WF_2DCWK *wk, u16 frame);
u16 WF_2DC_WkAnmFrameGet(const WF_2DCWK *wk);
// x and y are the character's: the shadow goes under its feet
void WF_2DC_WkShadowMatrixSet(WF_2DCWK *wk, s16 x, s16 y);
void WF_2DC_WkPatAnmStart(WF_2DCWK *wk, u32 anmType, u32 way);
void WF_2DC_WkPatAnmMain(WF_2DCWK *wk);

#endif // POKEBW2_APP_WF2DMAP_WIFI_2DCHAR_H
