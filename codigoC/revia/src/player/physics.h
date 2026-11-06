/* Movimento de corpos retangulares contra o mapa, eixo por eixo. */
#ifndef PHYSICS_H
#define PHYSICS_H

#include "world/level.h"

/* Move em X/Y por 'amount' pixels em passos de até 1px.
 * Ao bater, encosta exatamente na borda do tile e devolve true. */
bool phys_move_x(const Level *lv, Vec2f *pos, float w, float h, float amount);
bool phys_move_y(const Level *lv, Vec2f *pos, float w, float h, float amount);

#endif
