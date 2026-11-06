/* Protagonista (o Decifrador): estado, entrada e atualização física.
 * Este módulo não conhece a Raylib; recebe a entrada já traduzida. */
#ifndef PLAYER_H
#define PLAYER_H

#include "core/config.h"
#include "world/level.h"

typedef struct {
    bool left, right, up, down;
    bool jump_pressed, jump_held;
    bool dash_pressed;
    bool grab_held;
} PlayerInput;

typedef enum { PS_NORMAL = 0, PS_DASH, PS_CLIMB, PS_DEAD } PlayerState;

/* Eventos gerados no último passo (para som e partículas). */
enum {
    EV_JUMP = 1, EV_WALLJUMP = 2, EV_DASH = 4,
    EV_LAND = 8, EV_DEATH = 16, EV_LEDGE = 32
};

typedef struct {
    Vec2f pos;               /* canto superior esquerdo da hitbox */
    Vec2f vel;
    int facing;              /* -1 esquerda, 1 direita */
    bool on_ground;
    int wall_dir;            /* -1 parede à esquerda, 1 à direita, 0 nenhuma */
    PlayerState state;
    float coyote, jump_buf, dash_time, dash_cd, wj_lock, stamina;
    bool dash_ready, jump_cut;
    Vec2f dash_dir;
    float anim_t;
    int events;
} Player;

void  player_reset(Player *p, Vec2f spawn);
void  player_update(Player *p, const PlayerInput *in, Level *lv, const Tuning *tu, float dt);
void  player_kill(Player *p);
Rectf player_hitbox(const Player *p);

#endif
