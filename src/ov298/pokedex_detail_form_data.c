#include "types.h"
#include "app/pokedex_detail.h"

// Three tables of the Pokédex detail screen's forms page. The linker placed them ahead of pokedex_detail_form.c's
// .rodata, before its local initializers, which no order of declarations in one file gives, so they come from an
// object of their own linked just before it, as field_goout_effect_data.c's tables do in overlay 36. The ROM names no
// such file; the name is a guess after the page it serves

// The messages of the form count, the shiny count, the species, male, female and shiny
const u16 POKEDEX_DETAIL_FORM_STRBUF_MESSAGES[6] = { 187, 188, 159, 114, 115, 189 };

// The slider's bar and knob, and the color marks of the two entries shown
const PokedexDetailFormActorData POKEDEX_DETAIL_FORM_ACTORS[4] = {
    { 144, 48, 21, 1, 3, POKEDEX_DETAIL_FORM_RES_MAIN_CHARS, POKEDEX_DETAIL_FORM_RES_MAIN_PALETTE,
      POKEDEX_DETAIL_FORM_RES_MAIN_CELL_ANIMS },
    { 156, 48, 22, 0, 3, POKEDEX_DETAIL_FORM_RES_MAIN_CHARS, POKEDEX_DETAIL_FORM_RES_MAIN_PALETTE,
      POKEDEX_DETAIL_FORM_RES_MAIN_CELL_ANIMS },
    { 0, 0, 0, 0, 3, POKEDEX_DETAIL_FORM_RES_COLOR_CHARS, POKEDEX_DETAIL_FORM_RES_COLOR_PALETTE,
      POKEDEX_DETAIL_FORM_RES_COLOR_CELL_ANIMS },
    { 128, 0, 0, 0, 3, POKEDEX_DETAIL_FORM_RES_COLOR_CHARS, POKEDEX_DETAIL_FORM_RES_COLOR_PALETTE,
      POKEDEX_DETAIL_FORM_RES_COLOR_CELL_ANIMS },
};

// Where a sprite stands: in the middle, off the screen to the left and right, and on the left and right
const PokedexDetailFormPos POKEDEX_DETAIL_FORM_DEFAULT_POSITIONS[5] = {
    { 0.0f, -13.9f, 0.0f },   { -64.0f, -13.9f, 0.0f }, { 64.0f, -13.9f, 0.0f },
    { -16.0f, -13.9f, 0.0f }, { 16.0f, -13.9f, 0.0f },
};
