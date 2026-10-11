#ifndef POKEBW2_FIELD_WEATHER_PARTICLES_H
#define POKEBW2_FIELD_WEATHER_PARTICLES_H

#include "types.h"
#include "field/field_weather.h"

// weather_particles.c, overlay 75: the weathers 1, 4, 5 and 8, all with particles. The ROM doesn't name the file; the
// name is a guess, and so are the weathers' names, which only say what the particles do. Overlay 36's weather table
// (WEATHER_OVERLAYS) loads it

// Weather 1: flakes that sway down
extern WeatherData FLAKE_WEATHER;
// Weather 4: flakes that rush across a scrolling background
extern WeatherData BLIZZARD_WEATHER;
// Weather 5: flakes that fall, then flutter
extern WeatherData FLUTTER_WEATHER;
// Weather 8: specks
extern WeatherData SPECK_WEATHER;

#endif // POKEBW2_FIELD_WEATHER_PARTICLES_H
