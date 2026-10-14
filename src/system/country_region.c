#include "types.h"
#include "system/country_region.h"

// The countries a player can say they live in, and the regions of the 18 countries that are divided into regions: the
// message file with their names, and the order a choice lists them in. The file's name is a guess: the ROM has no
// string for it. russia_districts_list, poland_provinces_list, sweden_county_list and state_number_list are swan's
// names, and the other lists are named after them.

typedef struct {
    u8 country;
    // Which country's place data, 18 for the world
    u8 placeDataId;
    // The message file with the names of the regions
    u16 msgFileId;
    const u8 *regionOrder;
    u8 regionCount;
    // The highest region number. Sweden's regions skip a number
    u8 regionMax;
} SubdividedCountry;

static const u8 argentina_provinces_list[] = {
    1, 3, 4, 5, 2, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
};

static const u8 australia_states_list[] = {
    1, 2, 3, 4, 5, 6, 7, 8,
};

static const u8 brazil_states_list[] = {
    1, 3, 4, 5, 6, 7, 9, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 21, 2, 23, 24, 25, 26, 27,
};

static const u8 canada_provinces_list[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
};

static const u8 china_provinces_list[] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 32, 13, 14, 19, 15,
    16, 17, 18, 33, 20, 21, 22, 23, 24, 25, 26, 27, 29, 28, 30, 31,
};

static const u8 germany_states_list[] = {
    1, 2, 3, 4, 5, 6, 7, 9, 8, 10, 11, 12, 13, 14, 15, 16,
};

static const u8 spain_communities_list[] = {
    1, 2, 3, 4, 5, 6, 7, 9, 8, 10, 11, 12, 16, 13, 14, 15, 17,
};

static const u8 finland_provinces_list[] = {
    1, 3, 2, 5, 4, 6,
};

static const u8 france_regions_list[] = {
    2, 1, 4, 13, 12, 22, 6, 5, 11, 3, 18, 19, 21, 8, 17, 7, 15, 10, 16, 14, 20, 9,
};

static const u8 uk_regions_list[] = {
    2, 1, 3, 4, 6, 5, 7, 8, 9, 10, 11, 12,
};

static const u8 india_states_list[] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 24, 22, 23, 25, 26, 27, 28, 29, 30, 31, 32, 34, 33, 35,
};

static const u8 italy_regions_list[] = {
    1, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 17, 16, 18, 19, 20,
};

static const u8 japan_prefectures_list[] = {
    26, 8, 5,  15, 41, 21, 43, 10, 24, 13, 37, 2,  4,  1,  3,  31, 11, 20, 6,  40, 49, 17, 42, 46, 29,
    27, 7, 48, 23, 45, 32, 18, 47, 36, 50, 30, 44, 14, 28, 35, 25, 12, 39, 16, 34, 19, 33, 9,  38, 22,
};

static const u8 norway_counties_list[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 22, 14, 15, 16, 17, 18, 19, 20, 21,
};

static const u8 poland_provinces_list[] = {
    15, 13, 2, 6, 5, 1, 3, 4, 7, 8, 10, 11, 12, 9, 14, 16,
};

static const u8 russia_districts_list[] = {
    1, 2, 8, 3, 5, 6, 7, 4,
};

static const u8 sweden_county_list[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
};

static const u8 state_number_list[] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
    27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
};

// The first entry stands for a country that isn't divided
static const SubdividedCountry sSubdividedCountries[] = {
    { COUNTRY_NONE, 18, 0x1b6, NULL, 0, 0 },
    { 9, 0, 0x1a3, argentina_provinces_list, NELEMS(argentina_provinces_list), 24 },
    { 12, 1, 0x1a4, australia_states_list, NELEMS(australia_states_list), 8 },
    { 28, 2, 0x1a5, brazil_states_list, NELEMS(brazil_states_list), 27 },
    { 36, 3, 0x1a6, canada_provinces_list, NELEMS(canada_provinces_list), 13 },
    { 43, 4, 0x1a7, china_provinces_list, NELEMS(china_provinces_list), 33 },
    { 79, 5, 0x1a8, germany_states_list, NELEMS(germany_states_list), 16 },
    { 195, 6, 0x1a9, spain_communities_list, NELEMS(spain_communities_list), 17 },
    { 72, 7, 0x1aa, finland_provinces_list, NELEMS(finland_provinces_list), 6 },
    { 73, 8, 0x1ab, france_regions_list, NELEMS(france_regions_list), 22 },
    { 218, 9, 0x1ac, uk_regions_list, NELEMS(uk_regions_list), 12 },
    { 95, 10, 0x1ad, india_states_list, NELEMS(india_states_list), 35 },
    { 102, 11, 0x1ae, italy_regions_list, NELEMS(italy_regions_list), 20 },
    { COUNTRY_JAPAN, 12, 0x1af, japan_prefectures_list, NELEMS(japan_prefectures_list), 50 },
    { 155, 13, 0x1b0, norway_counties_list, NELEMS(norway_counties_list), 22 },
    { 166, 14, 0x1b1, poland_provinces_list, NELEMS(poland_provinces_list), 16 },
    { 174, 15, 0x1b2, russia_districts_list, NELEMS(russia_districts_list), 8 },
    { 200, 16, 0x1b3, sweden_county_list, NELEMS(sweden_county_list), 22 },
    { 220, 17, 0x1b5, state_number_list, NELEMS(state_number_list), 51 },
};

const u8 gSelectableCountries[130] = {
    1,   2,   3,   6,   8,   9,   12,  13,  15,  16,  17,  18,  20,  21,  22,  23,  25,  27,  28,  29,  31,  33,
    34,  35,  36,  40,  42,  43,  45,  47,  48,  49,  51,  53,  54,  58,  60,  61,  62,  63,  64,  71,  72,  73,
    74,  76,  79,  80,  81,  82,  83,  84,  85,  87,  88,  90,  91,  92,  93,  94,  95,  96,  98,  99,  101, 102,
    103, 105, 106, 109, 111, 115, 117, 118, 121, 125, 128, 130, 132, 134, 138, 139, 141, 145, 147, 148, 149, 150,
    151, 155, 156, 157, 160, 161, 163, 164, 166, 167, 173, 174, 181, 185, 186, 188, 189, 190, 191, 194, 170, 195,
    196, 198, 199, 200, 201, 203, 219, 205, 206, 210, 211, 215, 217, 218, 220, 221, 222, 224, 226, 227,
};

const int gSelectableCountryCount = NELEMS(gSelectableCountries);

u32 Country_GetSubdividedCount(void) {
    return NELEMS(sSubdividedCountries);
}

u32 getSubdividedCountryArrayNum(u32 country) {
    u32 i;

    for (i = 0; i < NELEMS(sSubdividedCountries); i++) {
        if (country == sSubdividedCountries[i].country) {
            return i;
        }
    }
    return 0;
}

u32 CountryHasProvinces(u32 country) {
    u32 i;

    for (i = 0; i < NELEMS(sSubdividedCountries); i++) {
        if (country == sSubdividedCountries[i].country) {
            return sSubdividedCountries[i].regionMax;
        }
    }
    return 0;
}

u32 fetchFileNumOfDividedCountry(u32 country) {
    return getFileNumForSubdividedRegion(getSubdividedCountryArrayNum(country));
}

u32 getFileNumForSubdividedRegion(u32 index) {
    if (index < NELEMS(sSubdividedCountries)) {
        return sSubdividedCountries[index].msgFileId;
    }
    return 0x1b6;
}

u32 Country_GetSubdividedCountryCode(u32 index) {
    if (index < NELEMS(sSubdividedCountries)) {
        return sSubdividedCountries[index].country;
    }
    return COUNTRY_NONE;
}

u32 Country_GetPlaceDataID(u32 index) {
    if (index < NELEMS(sSubdividedCountries)) {
        return sSubdividedCountries[index].placeDataId;
    }
    return 18;
}

u32 Country_GetJapan(void) {
    return COUNTRY_JAPAN;
}

BOOL Country_IsValidPlace(u32 country, u32 region, BOOL japanOnly) {
    int i;

    if (country >= COUNTRY_CODE_COUNT) {
        return FALSE;
    }
    if (country != COUNTRY_NONE) {
        if (region == 0 && CountryHasProvinces(country)) {
            return FALSE;
        }
        // The original tests the country again, and its loop test is a 0/1 value, which keeps the loop entered at
        // its test (docs/matching.md)
        if (country != COUNTRY_NONE) {
            for (i = 0; i < gSelectableCountryCount ? TRUE : FALSE; i++) {
                if (gSelectableCountries[i] == country) {
                    break;
                }
            }
            if (i >= gSelectableCountryCount) {
                return FALSE;
            }
        }
    }
    if (CountryHasProvinces(country) < region) {
        return FALSE;
    }
    if (japanOnly == TRUE && country != COUNTRY_JAPAN && country != COUNTRY_NONE) {
        return FALSE;
    }
    return TRUE;
}

u8 Country_GetValidCountry(u8 country, u8 region, u8 japanOnly) {
    if (!Country_IsValidPlace(country, region, japanOnly)) {
        country = COUNTRY_NONE;
    }
    return country;
}

u8 Country_GetValidRegion(u8 country, u8 region, u8 japanOnly) {
    if (!Country_IsValidPlace(country, region, japanOnly)) {
        region = 0;
    }
    return region;
}

BOOL Country_GetRegionOrder(u32 country, const u8 **order, u32 *count) {
    u32 index = getSubdividedCountryArrayNum(country);

    if (index == 0) {
        return FALSE;
    }
    *order = sSubdividedCountries[index].regionOrder;
    *count = sSubdividedCountries[index].regionCount;
    return TRUE;
}
