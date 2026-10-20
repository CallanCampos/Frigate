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

Config *Config_Create(u32 heapId) {
    Config *config = GFL_HeapAllocate(heapId, sizeof(Config), FALSE, "config.c", 77);
    Config_Init(config);
    return config;
}

void initConfig(const Config *config, Config *dest) {
    MI_CpuCopy8(config, dest, sizeof(Config));
}

// The original clears each option after the memset
void Config_Init(Config *config) {
    MI_CpuFill8(config, 0, sizeof(Config));
    config->textSpeed = 1;
    config->soundFlags = 0;
    config->unk6 = 0;
    config->unk7 = 0;
    config->kanji = 0;
    config->unk9 = 1;
    config->cgearOn = 0;
}

u32 Config_GetTextSpeed(const Config *config) {
    return config->textSpeed;
}

void Config_SetTextSpeed(Config *config, u32 value) {
    config->textSpeed = value;
}

u32 getSoundFlags(const Config *config) {
    return config->soundFlags;
}

void Config_SetSoundFlags(Config *config, u32 value) {
    config->soundFlags = value;
}

BOOL Config_GetBattleScene(const Config *config) {
    return config->unk7;
}

void Config_SetBattleScene(Config *config, u32 value) {
    config->unk7 = value;
}

BOOL Config_GetBattleStyle(const Config *config) {
    return config->unk6;
}

void Config_SetBattleStyle(Config *config, u32 value) {
    config->unk6 = value;
}

u32 Config_GetKanji(const Config *config) {
    return config->kanji;
}

void Config_SetKanji(Config *config, u32 value) {
    config->kanji = value;
    GFL_MsgDataSetDefaultLangID(config->kanji);
}

void Config_ApplyMsgLanguage(const Config *config) {
    GFL_MsgDataSetDefaultLangID(config->kanji);
}

u32 Config_GetSaveBeforeIR(const Config *config) {
    return config->unk9;
}

void Config_SetSaveBeforeIR(Config *config, u32 value) {
    config->unk9 = value;
}

u32 Config_GetCGearOn(const Config *config) {
    return config->cgearOn;
}

void Config_SetCGearOn(Config *config, u32 value) {
    config->cgearOn = value;
}
