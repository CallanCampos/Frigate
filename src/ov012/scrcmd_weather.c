#include "types.h"
#include "field/field.h"
#include "field/field_environment.h"
#include "field/field_map.h"
#include "field/field_script.h"
#include "field/field_weather.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/zone_weather.h"

// The script command that sets the field's weather (a descriptive name). s0136_FieldSetWeather is swan's name

// The zone's own weather
#define WEATHER_OF_ZONE 0xffff

BOOL s0136_FieldSetWeather(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    FieldWeather *weather;
    u32 weatherId;
    u16 immediate;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    weather = Field_GetWeatherSystem(field);
    weatherId = ScriptReadAny(vm, env);
    immediate = ScriptReadAny(vm, env);
    if (weatherId == WEATHER_OF_ZONE) {
        weatherId = GetWeatherAll(gsys, Field_GetPlayerStateZoneID(field));
    }
    if (immediate) {
        func_ov036_02199210(weather, weatherId);
    } else {
        func_ov036_02199208(weather, weatherId);
    }
    SetNowWeather(gameData, weatherId);
    return FALSE;
}
