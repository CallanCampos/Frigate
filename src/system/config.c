#include "types.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "save/config.h"

// The options (the ROM's string for the file, "config.c"). The names of the fields are guessed.

// The layout of this table is not known: three groups of five words
static const u32 sKeyRemaps[] = { 0, 0, 0, 8, 1024, 1, 0, 0, 0, 512, 1, 1, 0, 0, 0 };

void initKeyBlk(void) {
    setKeyBlkStarted(sKeyRemaps);
}

Config *func_0200898c(u32 heapId) {
    Config *config = GFL_HeapAllocate(heapId, sizeof(Config), FALSE, "config.c", 77);
    func_020089c0(config);
    return config;
}

void initConfig(const Config *config, Config *dest) {
    MI_CpuCopy8(config, dest, sizeof(Config));
}

// The original clears each option after the memset
void func_020089c0(Config *config) {
    MI_CpuFill8(config, 0, sizeof(Config));
    config->textSpeed = 1;
    config->soundFlags = 0;
    config->unk6 = 0;
    config->unk7 = 0;
    config->kanji = 0;
    config->unk9 = 1;
    config->cgearOn = 0;
}

u32 func_02008a14(const Config *config) {
    return config->textSpeed;
}

void func_02008a1c(Config *config, u32 value) {
    config->textSpeed = value;
}

u32 getSoundFlags(const Config *config) {
    return config->soundFlags;
}

void func_02008a38(Config *config, u32 value) {
    config->soundFlags = value;
}

BOOL func_02008a4c(const Config *config) {
    return config->unk7;
}

void func_02008a54(Config *config, u32 value) {
    config->unk7 = value;
}

BOOL func_02008a68(const Config *config) {
    return config->unk6;
}

void func_02008a70(Config *config, u32 value) {
    config->unk6 = value;
}

u32 func_02008a84(const Config *config) {
    return config->kanji;
}

void func_02008a8c(Config *config, u32 value) {
    config->kanji = value;
    GFL_MsgDataSetDefaultLangID(config->kanji);
}

void func_02008ab4(const Config *config) {
    GFL_MsgDataSetDefaultLangID(config->kanji);
}

u32 func_02008ac8(const Config *config) {
    return config->unk9;
}

void func_02008ad0(Config *config, u32 value) {
    config->unk9 = value;
}

u32 func_02008ae8(const Config *config) {
    return config->cgearOn;
}

void func_02008af0(Config *config, u32 value) {
    config->cgearOn = value;
}
