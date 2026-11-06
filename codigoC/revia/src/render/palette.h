/* Paletas de cor por região. Cada região do jogo tem sua identidade visual. */
#ifndef PALETTE_H
#define PALETTE_H

#include "raylib.h"

#define PALETTE_COUNT 7

typedef struct {
    Color sky_top, sky_bot;
    Color far_c, mid_c;            /* camadas de parallax */
    Color tile, tile_alt, tile_top;
    Color accent, spike, mote;
    int   style;                   /* formato do skyline (0..6) */
} Palette;

const Palette *palette_get(int id);
Color color_mix(Color a, Color b, float t);
/* Aproxima 'cur' de 'target' (k em 0..1): transição suave entre regiões. */
void palette_approach(Palette *cur, const Palette *target, float k);

#endif
