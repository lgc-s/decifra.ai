/* Partículas: poeira, rastro do dash, explosões e partículas ambientais. */
#ifndef FX_H
#define FX_H

#include "raylib.h"
#include "render/palette.h"

#define FX_MAX 640

typedef struct {
    float x, y, vx, vy, life, max_life, size, gravity;
    Color c;
} Particle;

typedef struct {
    Particle p[FX_MAX];
    int n;
    unsigned rng;
    float ambient_acc;
} Fx;

void fx_init(Fx *fx);
void fx_clear(Fx *fx);
void fx_burst(Fx *fx, float x, float y, int count, float speed, Color c, float life);
void fx_dust(Fx *fx, float x, float y, int dir, Color c);
void fx_trail(Fx *fx, float x, float y, Color c);
/* Partículas ambientais dentro da área visível (cam_x, cam_y, VIEW_W x VIEW_H). */
void fx_update(Fx *fx, float dt, float cam_x, float cam_y, const Palette *pal, bool ambient);
void fx_draw(const Fx *fx, int ox, int oy);

#endif
