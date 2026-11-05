/* Tipos matemáticos simples, independentes da Raylib.
 * A lógica do jogo (física, fases, quiz...) usa só estes tipos, o que a
 * mantém testável sem janela gráfica. */
#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef struct { float x, y; } Vec2f;
typedef struct { float x, y, w, h; } Rectf;

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float absf(float v) { return v < 0 ? -v : v; }
static inline float signf(float v) { return v > 0 ? 1.0f : (v < 0 ? -1.0f : 0.0f); }

/* Move v em direção a target em no máximo step (sem ultrapassar). */
static inline float approachf(float v, float target, float step) {
    if (v < target) { v += step; return v > target ? target : v; }
    v -= step;
    return v < target ? target : v;
}

#endif
