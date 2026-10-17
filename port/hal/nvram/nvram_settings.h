// The host console's firmware settings (port/hal/nvram): the user settings that the DS's firmware keeps in its flash
// and copies to the shared memory at boot (GBATEK, "DS Firmware User Settings"), and the console's MAC address. The
// host's ARM7 (nvram7.c) puts them in the system area when it boots and builds its flash image from them, so a change
// takes effect from the next boot. Plain C types only, so that the program can set them from its own configuration.
#ifndef PORT_HAL_NVRAM_NVRAM_SETTINGS_H
#define PORT_HAL_NVRAM_NVRAM_SETTINGS_H

#include <stdint.h>

// The firmware's limits on the nickname and the message, in UTF-16 characters
#define PORT_NVRAM_NICKNAME_MAX 10
#define PORT_NVRAM_COMMENT_MAX 26

// The languages of the settings
enum {
    PORT_NVRAM_LANGUAGE_JAPANESE = 0,
    PORT_NVRAM_LANGUAGE_ENGLISH = 1,
    PORT_NVRAM_LANGUAGE_FRENCH = 2,
    PORT_NVRAM_LANGUAGE_GERMAN = 3,
    PORT_NVRAM_LANGUAGE_ITALIAN = 4,
    PORT_NVRAM_LANGUAGE_SPANISH = 5,
};

typedef struct PortNvramSettings {
    uint8_t language;      // PORT_NVRAM_LANGUAGE_*
    uint8_t favoriteColor; // 0-15, an entry of OS_GetFavoriteColorTable
    uint8_t birthMonth;    // 1-12
    uint8_t birthDay;      // 1-31
    uint16_t nickname[PORT_NVRAM_NICKNAME_MAX];
    uint8_t nicknameLength;
    uint16_t comment[PORT_NVRAM_COMMENT_MAX];
    uint8_t commentLength;
    // The difference in seconds that the player's changes of the clock in the DS's settings made, which the game keeps
    // in the save and compares (OS_GetOwnerRtcOffset)
    int64_t rtcOffset;
    uint8_t macAddress[6];
} PortNvramSettings;

// The settings, for reading and changing before the game runs. They start as an English console's: the nickname
// "Player", a birthday of June 23 (the games' release day), the favorite color blue (11), no message, no clock offset,
// and a locally administered MAC address
PortNvramSettings *port_nvram_settings(void);

#endif // PORT_HAL_NVRAM_NVRAM_SETTINGS_H
