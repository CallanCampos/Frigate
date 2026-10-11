#ifndef POKEBW2_FIELD_WEATHER_TINT_H
#define POKEBW2_FIELD_WEATHER_TINT_H

#include "types.h"
#include "field/field_weather.h"

// weather_tint.c, overlay 78: the weathers 9, 10, 11, 13 and 14, which have no particles. Each fades in a tint of the
// screen (a level and an RGB555 colour) and plays a sound. Weather 9's are fixed; the others take theirs from a table
// by the camera's defaults index. The ROM doesn't name the file; the name is a guess. Overlay 36's weather table
// (WEATHER_OVERLAYS) loads it
extern WeatherData TINT_WEATHER_9;
extern WeatherData TINT_WEATHER_10;
extern WeatherData TINT_WEATHER_11;
extern WeatherData TINT_WEATHER_13;
extern WeatherData TINT_WEATHER_14;

#endif // POKEBW2_FIELD_WEATHER_TINT_H
