#ifndef POKEBW2_FIELD_WEATHER_RAIN_H
#define POKEBW2_FIELD_WEATHER_RAIN_H

#include "types.h"
#include "field/field_weather.h"

// weather_rain.c, overlay 76: the rain weathers 2, 6 and 7, all with particles. The ROM doesn't name the file; the name
// is a guess, and so are the weathers' names, which only say what they look like: falling drops that splash, a
// thunderstorm whose lightning flashes the screen, and a rain that gusts. Overlay 36's weather table
// (WEATHER_OVERLAYS) loads it

// Weather 2: drops that fall and splash
extern WeatherData RAIN_WEATHER;
// Weather 6: faster drops, a scrolling background and lightning
extern WeatherData THUNDERSTORM_WEATHER;
// Weather 7: rain that gusts, with two scrolling backgrounds and thunder rumbling
extern WeatherData GUSTY_RAIN_WEATHER;

#endif // POKEBW2_FIELD_WEATHER_RAIN_H
