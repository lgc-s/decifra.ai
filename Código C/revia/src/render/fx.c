#include "render/fx.h"
#include "core/config.h"

#include <math.h>
#include <string.h>

static float frand(Fx *fx) {                 /* 0..1 */
    fx->rng = fx->rng * 1664525u + 1013904223u;
    return (float)((fx->rng >> 8) & 0xFFFF) / 65535.0f;
}

void fx_init(Fx *fx) { memset(fx, 0, sizeof *fx); fx->rng = 90210u; }
void fx_clear(Fx *fx) { fx->n = 0; }

static Particle *fx_add(Fx *fx) {
    if (fx->n >= FX_MAX) return 0;           /* cheio: descarta, sem custo */
    return &fx->p[fx->n++];
}

void fx_burst(Fx *fx, float x, float y, int count, float speed, Color c, float life) {
    for (int i = 0; i < count; i++) {
        Particle *p = fx_add(fx);
        if (!p) return;
        float a = frand(fx) * 6.2831853f;
        float s = speed * (0.35f + 0.65f * frand(fx));
        p->x = x; p->y = y;
        p->vx = cosf(a) * s; p->vy = sinf(a) * s;
        p->life = p->max_life = life * (0.6f + 0.4f * frand(fx));
        p->size = frand(fx) < 0.3f ? 2.0f : 1.0f;
        p->gravity = 60.0f;
        p->c = c;
    }
}

void fx_dust(Fx *fx, float x, float y, int dir, Color c) {
    for (int i = 0; i < 4; i++) {
        Particle *p = fx_add(fx);
        if (!p) return;
        p->x = x + (frand(fx) - 0.5f) * 4.0f; p->y = y;
        p->vx = -dir * (6.0f + 14.0f * frand(fx)) + (frand(fx) - 0.5f) * 10.0f;
        p->vy = -4.0f - 10.0f * frand(fx);
        p->life = p->max_life = 0.3f + 0.2f * frand(fx);
        p->size = 1.0f; p->gravity = 10.0f; p->c = c;
    }
}

void fx_trail(Fx *fx, float x, float y, Color c) {
    Particle *p = fx_add(fx);
    if (!p) return;
    p->x = x; p->y = y; p->vx = 0; p->vy = 0;
    p->life = p->max_life = 0.22f;
    p->size = 3.0f; p->gravity = 0; p->c = c;
}

void fx_update(Fx *fx, float dt, float cam_x, float cam_y, const Palette *pal, bool ambient) {
    for (int i = 0; i < fx->n; ) {
        Particle *p = &fx->p[i];
        p->life -= dt;
        if (p->life <= 0) { fx->p[i] = fx->p[--fx->n]; continue; }
        p->vy += p->gravity * dt;
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        i++;
    }
    if (!ambient) return;

    /* Partículas ambientais: sobem (dados flutuando) ou caem (cinzas, no Escândalo). */
    fx->ambient_acc += dt * 14.0f;
    while (fx->ambient_acc >= 1.0f) {
        fx->ambient_acc -= 1.0f;
        Particle *p = fx_add(fx);
        if (!p) break;
        bool ash = pal->style == 5;
        p->x = cam_x + frand(fx) * VIEW_W;
        p->y = ash ? cam_y - 4.0f : cam_y + VIEW_H * (0.4f + 0.6f * frand(fx));
        p->vx = (frand(fx) - 0.5f) * 8.0f;
        p->vy = ash ? 10.0f + 14.0f * frand(fx) : -(5.0f + 9.0f * frand(fx));
        p->life = p->max_life = 3.0f + 2.5f * frand(fx);
        p->size = 1.0f; p->gravity = 0;
        p->c = pal->mote;
    }
}

void fx_draw(const Fx *fx, int ox, int oy) {
    for (int i = 0; i < fx->n; i++) {
        const Particle *p = &fx->p[i];
        float t = p->life / p->max_life;
        Color c = p->c;
        /* surge e some suavemente */
        float a = t < 0.35f ? t / 0.35f : (t > 0.9f ? (1.0f - t) / 0.1f : 1.0f);
        c.a = (unsigned char)(255.0f * a * (p->size >= 3.0f ? 0.5f : 0.85f));
        int s = (int)p->size;
        DrawRectangle((int)p->x + ox, (int)p->y + oy, s, s, c);
    }
}
