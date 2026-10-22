#ifndef POKEBW2_NNSYS_SND_INTERNAL_H
#define POKEBW2_NNSYS_SND_INTERNAL_H

#include "nitro/fs.h"
#include "nitro/pm.h"
#include "nitro/snd.h"
#include "nnsys/fnd.h"
#include "nnsys/snd.h"

// What NitroSystem's sound files share among themselves and don't export: the sequence player, sound archive and sound
// heap internals. The header's name is ours

// The sound heap (snd_heap.c): a frame heap whose levels each keep the list of blocks allocated since the level was
// saved
typedef struct NNSSndHeap {
    NNSFndHeapHandle handle;
    NNSFndList sectionList;
} NNSSndHeap;

// A stream (snd_stream.c). Its buffer is split into one part per channel, each played in a loop in interval blocks of
// chBufLen / interval bytes; curBuffer is the block the callback fills next
typedef struct NNSSndStrm {
    NNSFndLink link;
    PMSleepCallbackInfo preSleepInfo;
    PMSleepCallbackInfo postSleepInfo;
    NNSSndStrmFormat format;
    BOOL activeFlag : 1;
    BOOL startFlag : 1;
    u32 chBufLen;
    int interval;
    NNSSndStrmCallback callback;
    void *callbackArg;
    int curBuffer;
    int volume;
    int alarmNo;
    u32 chBitMask;
    int numChannels;
    u8 channelNo[NNS_SND_STRM_CHANNEL_MAX];
} NNSSndStrm;

// The sound archive's tables (snd_arc.c), which nnsys/snd.h's NNSSndArc points to. A file of the table records where
// the file was loaded, else NULL
typedef struct NNSSndArcOffsetTable {
    u32 count;
    u32 offset[];
} NNSSndArcOffsetTable;

// Each offset is from the start of the info block, 0 when there is no table
typedef struct NNSSndArcInfo {
    SNDBinaryBlockHeader blockHeader;
    u32 seqOffset;
    u32 seqArcOffset;
    u32 bankOffset;
    u32 waveArcOffset;
    u32 playerInfoOffset;
    u32 groupInfoOffset;
    u32 strmPlayerInfoOffset;
    u32 strmOffset;
} NNSSndArcInfo;

typedef struct NNSSndArcFileInfo {
    u32 offset;
    u32 size;
    void *mem;
    u32 reserved;
} NNSSndArcFileInfo;

typedef struct NNSSndArcFat {
    SNDBinaryBlockHeader blockHeader;
    u32 count;
    NNSSndArcFileInfo files[];
} NNSSndArcFat;

typedef struct NNSSndArcSymbol NNSSndArcSymbol;

// The archive's file, for opening it again (snd_arc.c): its file ID, and its path
FSFileID NNS_SndArcGetFileID(void);
const char *NNSi_SndArcGetFilePath(void);

// A sequence archive (snd_seqdata.c): its sequences, each at an offset from baseOffset, or
// NNS_SND_SEQ_ARC_INVALID_OFFSET
#define NNS_SND_SEQ_ARC_INVALID_OFFSET 0xffffffff

typedef struct NNSSndSeqArcSeqInfo {
    u32 offset;
    NNSSndSeqParam param;
} NNSSndSeqArcSeqInfo;

typedef struct NNSSndSeqArc {
    SNDBinaryFileHeader fileHeader;
    SNDBinaryBlockHeader blockHeader;
    u32 baseOffset;
    u32 count;
    NNSSndSeqArcSeqInfo info[];
} NNSSndSeqArc;

const NNSSndSeqArcSeqInfo *NNSi_SndSeqArcGetSeqInfo(const NNSSndSeqArc *seqArc, int index);

// Loading the archive's files (snd_arc_loader.c). With bSetAddr, a file loaded is recorded as the archive's, to be
// found by NNS_SndArcGetFileAddress; with pData, the address of the file, loaded or not, is given there
typedef enum NNSSndArcLoadResult {
    NNS_SND_ARC_LOAD_SUCESS,
    NNS_SND_ARC_LOAD_ERROR_INVALID_GROUP_NO,
    NNS_SND_ARC_LOAD_ERROR_INVALID_SEQ_NO,
    NNS_SND_ARC_LOAD_ERROR_INVALID_SEQARC_NO,
    NNS_SND_ARC_LOAD_ERROR_INVALID_BANK_NO,
    NNS_SND_ARC_LOAD_ERROR_INVALID_WAVEARC_NO,
    NNS_SND_ARC_LOAD_ERROR_FAILED_LOAD_SEQ,
    NNS_SND_ARC_LOAD_ERROR_FAILED_LOAD_SEQARC,
    NNS_SND_ARC_LOAD_ERROR_FAILED_LOAD_BANK,
    NNS_SND_ARC_LOAD_ERROR_FAILED_LOAD_WAVE,
} NNSSndArcLoadResult;

NNSSndArcLoadResult NNSi_SndArcLoadSeq(int seqNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                       const SNDSequenceData **pData);
NNSSndArcLoadResult NNSi_SndArcLoadBank(int bankNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                        SNDBankData **pData);

// A fader (snd_fader.c), which moves a value from origin to target over frame frames, one step per update
typedef struct NNSSndFader {
    int origin;
    int target;
    int counter;
    int frame;
} NNSSndFader;

void NNSi_SndFaderInit(NNSSndFader *fader);
void NNSi_SndFaderSet(NNSSndFader *fader, int target, int frame);
int NNSi_SndFaderGet(const NNSSndFader *fader);
void NNSi_SndFaderUpdate(NNSSndFader *fader);
BOOL NNSi_SndFaderIsFinished(const NNSSndFader *fader);

// The driver's state (snd_main.c), copied from the ARM7 by NNS_SndUpdateDriverInfo: a track's info, FALSE when there is
// no copy yet
BOOL NNSi_SndReadDriverTrackInfo(int playerNo, int trackNo, SNDTrackInfo *trackInfo);

// The resource manager (snd_resource_mgr.c): sets every channel, capture and alarm free
void NNSi_SndInitResourceMgr(void);

// The sequence players (snd_player.c). A player number has a list of the sequence players playing on it, by priority,
// up to playableSeqCount of them, and a list of free heaps that its sequences load into. Every sequence player in use
// is also in one list of all of them by priority
typedef enum NNSSndSeqPlayerStatus {
    NNS_SND_SEQ_PLAYER_STATUS_NONE,
    NNS_SND_SEQ_PLAYER_STATUS_PLAY,
    NNS_SND_SEQ_PLAYER_STATUS_FADEOUT,
} NNSSndSeqPlayerStatus;

// What a sequence player plays, set by the sound archive's player
typedef enum NNSSndPlayerSeqType {
    NNS_SND_PLAYER_SEQ_TYPE_INVALID,
    NNS_SND_PLAYER_SEQ_TYPE_SEQ,
    NNS_SND_PLAYER_SEQ_TYPE_SEQARC,
} NNSSndPlayerSeqType;

typedef struct NNSSndPlayer NNSSndPlayer;
typedef struct NNSSndPlayerHeap NNSSndPlayerHeap;

// startFlag is set once the driver has started the sequence, prepareFlag while it is prepared and not yet started
typedef struct NNSSndSeqPlayer {
    NNSSndHandle *handle;
    NNSSndPlayer *player;
    NNSSndPlayerHeap *heap;
    NNSFndLink playerLink;
    NNSFndLink prioLink;
    NNSSndFader fader;
    u8 status;
    u8 startFlag;
    u8 pauseFlag;
    u8 prepareFlag;
    u32 commandTag;
    u16 seqType;
    u16 pad2;
    // The sequence's number, or with NNS_SND_PLAYER_SEQ_TYPE_SEQARC the sequence archive's and the index in it
    u16 seqNo;
    u16 seqArcIndex;
    u8 playerNo;
    u8 prio;
    s16 volume;
    u8 initVolume;
    u8 extVolume;
} NNSSndSeqPlayer;

void NNSi_SndPlayerInit(void);
void NNSi_SndPlayerMain(void);
NNSSndSeqPlayer *NNSi_SndPlayerAllocSeqPlayer(NNSSndHandle *handle, int playerNo, int prio);
void NNSi_SndPlayerFreeSeqPlayer(NNSSndSeqPlayer *seqPlayer);
void NNSi_SndPlayerStartSeq(NNSSndSeqPlayer *seqPlayer, const void *seqBase, u32 seqOffset, const SNDBankData *bank);
NNSSndHeapHandle NNSi_SndPlayerAllocHeap(int playerNo, NNSSndSeqPlayer *seqPlayer);
void NNS_SndPlayerSetSeqNo(NNSSndHandle *handle, int seqNo);
void NNS_SndPlayerSetSeqArcNo(NNSSndHandle *handle, int seqArcNo, int index);

// The capture (snd_capture.c), run by NNS_SndMain and around sleep. The capture records the mixer's output into bufL
// and bufR and plays them back on the channels of chBitMask, with an alarm when alarmNo isn't negative. A reverb's
// volume follows the fader, and with fadeStopFlag the capture stops when the fader is done. format, sampleRate,
// callback, arg and interval are used only by functions the linker dropped. The layout differs from that of ntrtwl's
// reconstruction of NitroSystem, so the fields keep our names.
typedef enum NNSSndCaptureType {
    NNS_SND_CAPTURE_TYPE_REVERB,
    NNS_SND_CAPTURE_TYPE_EFFECT,
    NNS_SND_CAPTURE_TYPE_SAMPLING,
} NNSSndCaptureType;

typedef struct CaptureParam {
    BOOL active;
    NNSSndCaptureType type;
    int format;
    void *bufL;
    void *bufR;
    u32 bufSize;
    int sampleRate;
    int bufPos;
    u32 lockChBitFlag;
    u32 chBitMask;
    u32 capBitMask;
    int alarmNo;
    void *callback;
    void *arg;
    int interval;
    NNSSndFader fader;
    BOOL fadeStopFlag;
    int volume;
} CaptureParam;

void NNSi_SndCaptureInit(void);
void NNSi_SndCaptureMain(void);
void NNSi_SndCaptureBeginSleep(void);
void NNSi_SndCaptureEndSleep(void);

// The sound archive's streams (snd_arc_stream.c), run by NNS_SndMain
void NNSi_SndArcStrmMain(void);

#endif // POKEBW2_NNSYS_SND_INTERNAL_H
