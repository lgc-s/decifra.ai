#include "render/character.h"

#define SKIN { 242, 198, 160, 255 }

static const CharacterStyle STYLES[NUM_CHARACTERS] = {
    { "Alex", {  70, 205, 180, 255 }, {  50, 160, 142, 255 }, { 44, 52, 92, 255 }, SKIN, {  72,  46,  30, 255 } },  /* jaleco verde-água, cabelo castanho */
    { "Bia",  {  95, 145, 255, 255 }, {  68, 108, 205, 255 }, { 40, 44, 70, 255 }, SKIN, {  22,  22,  32, 255 } },  /* jaleco azul, cabelo preto */
    { "Caio", { 232,  92, 172, 255 }, { 186,  66, 134, 255 }, { 50, 50, 80, 255 }, SKIN, { 190,  72,  32, 255 } },  /* jaleco magenta, cabelo ruivo */
    { "Dani", { 255, 172,  62, 255 }, { 208, 128,  38, 255 }, { 44, 56, 70, 255 }, SKIN, { 236, 206, 100, 255 } },  /* jaleco laranja, cabelo loiro */
};

const CharacterStyle *character_style(int id) {
    return &STYLES[(id >= 0 && id < NUM_CHARACTERS) ? id : 0];
}

void character_draw_preview(int cx, int y, int id, int s, int bob) {
    const CharacterStyle *c = character_style(id);
    int x = cx - 3 * s;                               /* o sprite tem 6 px de largura */
    const Color eye = { 24, 24, 40, 255 }, badge = { 120, 230, 255, 255 };
#define P(rx, ry, w, h, col) DrawRectangle(x + (rx) * s, y + (ry) * s, (w) * s, (h) * s, (col))
    P(1, bob, 4, 4, c->skin);                         /* cabeça */
    P(1, bob, 4, 2, c->hair);
    P(0, 1 + bob, 1, 2, c->hair);
    P(4, 2 + bob, 1, 1, eye);
    P(0, 4, 6, 4, c->coat);                           /* tronco e crachá */
    P(0, 7, 6, 1, c->coat2);
    P(1, 5, 2, 1, badge);
    P(1, 8, 2, 2, c->pants);                          /* pernas */
    P(3, 8, 2, 2, c->pants);
#undef P
}