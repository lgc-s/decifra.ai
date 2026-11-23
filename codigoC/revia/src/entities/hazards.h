/* Perigos móveis das fases: "bugs" que andam no chão (é preciso pular) e objetos
 * (dados corrompidos) que caem do céu depois de um aviso piscando no chão.
 * Tocar em qualquer um deles faz o Decifrador renascer no último checkpoint.
 *
 * Os perigos são posicionados automaticamente ao carregar a fase (sem mexer nos mapas .txt),
 * longe de início, checkpoints, saída, terminais e cristais. A lógica não conhece a Raylib. */
#ifndef HAZARDS_H
#define HAZARDS_H

#include <stdbool.h>
#include <stdint.h>
#include "core/config.h"
#include "player/player.h"
#include "world/level.h"

#define HZ_MAX_BUGS 14
#define HZ_MAX_RAIN 18

#define BUG_W 8.0f
#define BUG_H 6.0f
#define RAIN_W 6.0f
#define RAIN_H 7.0f

typedef enum { RAIN_WAIT = 0, RAIN_WARN, RAIN_FALL, RAIN_LAND } RainState;

typedef struct {
    float x0, x1;        /* limites da patrulha (borda esquerda do bug), em pixels */
    float x;             /* borda esquerda atual */
    float y;             /* topo do chão onde anda */
    float speed;
    int   dir;           /* -1 esquerda, 1 direita */
} Bug;

typedef struct {
    float x;             /* centro da coluna, em pixels */
    float y_land;        /* topo da superfície onde o objeto cai */
    float y_start;       /* topo do objeto no início da queda */
    float y;             /* topo do objeto durante a queda */
    float t, wait;       /* tempo no estado atual / duração da espera */
    int   state;         /* RainState */
    uint32_t rng;
} Rain;

typedef struct {
    Bug  bugs[HZ_MAX_BUGS]; int nbugs;
    Rain rain[HZ_MAX_RAIN]; int nrain;
    float warn_time;     /* segundos de aviso antes da queda (depende da dificuldade) */
    float fall_max;      /* velocidade máxima da queda, px/s (depende da dificuldade) */
    float wait_scale;    /* multiplica o tempo entre quedas (menor = mais frequente) */
} Hazards;

typedef struct {
    bool  landed;        /* algum objeto bateu no chão neste passo */
    float land_x, land_y;
} HazardEvents;

/* Posiciona os perigos da fase (determinístico por ato e dificuldade). */
void hazards_init(Hazards *hz, const Level *lv, Difficulty diff);

/* Avança um passo. Mata o jogador (player_kill) se ele encostar em um perigo. */
void hazards_update(Hazards *hz, const Level *lv, Player *p, float dt, HazardEvents *ev);

#endif