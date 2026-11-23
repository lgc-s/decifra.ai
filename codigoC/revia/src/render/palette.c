#include "render/palette.h"

#define RGB(r, g, b) { r, g, b, 255 }

static const Palette PALETTES[PALETTE_COUNT] = {
    /* 0 Departamento: azul-noite de escritório */
    { RGB(20,26,48), RGB(58,70,112), RGB(36,46,80), RGB(48,62,100),
      RGB(62,76,120), RGB(54,66,108), RGB(150,182,236),
      RGB(90,230,255), RGB(240,90,110), RGB(150,190,255), 0 },
    /* 1 Trânsito: entardecer alaranjado */
    { RGB(40,24,52), RGB(214,110,72), RGB(78,42,76), RGB(104,56,84),
      RGB(70,46,76), RGB(60,40,68), RGB(255,176,84),
      RGB(255,200,90), RGB(255,80,90), RGB(255,214,140), 1 },
    /* 2 Crédito: verde e dourado */
    { RGB(12,34,38), RGB(40,100,86), RGB(22,62,60), RGB(30,82,72),
      RGB(36,72,66), RGB(30,62,58), RGB(236,204,96),
      RGB(246,210,90), RGB(240,90,90), RGB(220,240,150), 2 },
    /* 3 Notícias: violeta e magenta */
    { RGB(28,18,56), RGB(124,62,136), RGB(54,34,90), RGB(76,46,112),
      RGB(66,42,102), RGB(56,36,90), RGB(230,140,230),
      RGB(255,140,230), RGB(255,90,110), RGB(240,190,255), 3 },
    /* 4 Saúde: clínica, tons claros de verde-água */
    { RGB(150,204,212), RGB(232,246,242), RGB(122,176,190), RGB(96,152,172),
      RGB(66,116,138), RGB(58,104,126), RGB(242,252,250),
      RGB(70,200,170), RGB(230,60,84), RGB(255,255,255), 4 },
    /* 5 Escândalo: vermelho-escuro e cinza */
    { RGB(12,8,16), RGB(74,22,36), RGB(34,18,30), RGB(54,24,42),
      RGB(48,34,50), RGB(40,28,44), RGB(214,62,74),
      RGB(255,84,96), RGB(255,120,60), RGB(190,170,170), 5 },
    /* 6 Desfecho: amanhecer */
    { RGB(34,26,70), RGB(250,172,132), RGB(74,50,104), RGB(112,70,122),
      RGB(72,54,102), RGB(62,46,92), RGB(255,216,150),
      RGB(255,222,130), RGB(255,100,100), RGB(255,236,190), 6 },
};

const Palette *palette_get(int id) {
    if (id < 0 || id >= PALETTE_COUNT) id = 0;
    return &PALETTES[id];
}

Color color_mix(Color a, Color b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    Color c;
    c.r = (unsigned char)(a.r + (b.r - a.r) * t);
    c.g = (unsigned char)(a.g + (b.g - a.g) * t);
    c.b = (unsigned char)(a.b + (b.b - a.b) * t);
    c.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return c;
}

void palette_approach(Palette *cur, const Palette *t, float k) {
    cur->sky_top  = color_mix(cur->sky_top,  t->sky_top,  k);
    cur->sky_bot  = color_mix(cur->sky_bot,  t->sky_bot,  k);
    cur->far_c    = color_mix(cur->far_c,    t->far_c,    k);
    cur->mid_c    = color_mix(cur->mid_c,    t->mid_c,    k);
    cur->tile     = color_mix(cur->tile,     t->tile,     k);
    cur->tile_alt = color_mix(cur->tile_alt, t->tile_alt, k);
    cur->tile_top = color_mix(cur->tile_top, t->tile_top, k);
    cur->accent   = color_mix(cur->accent,   t->accent,   k);
    cur->spike    = color_mix(cur->spike,    t->spike,    k);
    cur->mote     = color_mix(cur->mote,     t->mote,     k);
    cur->style    = t->style;
}
