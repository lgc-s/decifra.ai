/* Consultas de colisão contra a grade de tiles. */
#ifndef COLLISION_H
#define COLLISION_H

#include "world/level.h"

/* true se o tile (tx,ty) sustenta/bloqueia. Laterais do mapa são paredes;
 * acima e abaixo o espaço é aberto (cair abaixo do mapa mata). */
bool col_tile_solid(const Level *lv, int tx, int ty);

/* true se o retângulo toca algum tile sólido. */
bool col_rect_solid(const Level *lv, float x, float y, float w, float h);

/* true se o retângulo toca a parte perigosa de algum espinho. */
bool col_rect_hazard(const Level *lv, Rectf r);

#endif
