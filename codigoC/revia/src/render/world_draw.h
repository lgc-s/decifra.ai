/* Desenho do mundo de jogo (somente leitura sobre o estado). */
#ifndef WORLD_DRAW_H
#define WORLD_DRAW_H

#include "render/palette.h"
#include "player/player.h"

struct Game;

/* Céu em degradê + silhuetas de parallax (estilo definido pela paleta). */
void draw_background(const Palette *pal, float cam_x, float cam_y, float t);
/* Mundo completo: fundo, tiles, entidades, jogador e partículas. */
void draw_world(const struct Game *g);
/* Cena de fundo para menus: fundo rolando devagar, partículas e o protagonista. */
void draw_menu_scene(const struct Game *g, float scroll);

void draw_player(const Player *p, const Palette *pal, int character, float t, int ox, int oy);
void draw_vignette(void);
void draw_glow(int cx, int cy, float radius, Color c, float strength);

#endif
