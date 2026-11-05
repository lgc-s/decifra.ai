#include "player/player.h"
#include "player/physics.h"
#include "world/collision.h"

#include <string.h>

void player_reset(Player *p, Vec2f spawn) {
    memset(p, 0, sizeof *p);
    p->pos = spawn;
    p->facing = 1;
    p->state = PS_NORMAL;
    p->dash_ready = true;
    p->stamina = 110.0f;
}

Rectf player_hitbox(const Player *p) {
    Rectf r = { p->pos.x, p->pos.y, PH_W, PH_H };
    return r;
}

void player_kill(Player *p) {
    if (p->state == PS_DEAD) return;
    p->state = PS_DEAD;
    p->vel.x = p->vel.y = 0;
    p->events |= EV_DEATH;
}

/* Salto a partir do chão. */
static void do_ground_jump(Player *p, float ax) {
    p->vel.y = -PH_JUMP_SPEED;
    p->vel.x += ax * PH_JUMP_BOOST;
    p->coyote = 0;
    p->jump_buf = 0;
    p->jump_cut = false;
    p->events |= EV_JUMP;
}

/* Salto lançado para fora da parede. */
static void do_wall_jump(Player *p, const Tuning *tu) {
    (void)tu;
    p->vel.x = -p->wall_dir * PH_WJ_X;
    p->vel.y = -PH_WJ_Y;
    p->facing = -p->wall_dir;
    p->state = PS_NORMAL;
    p->wj_lock = PH_WJ_LOCK;
    p->jump_buf = 0;
    p->jump_cut = false;
    p->stamina -= PH_STAMINA_JUMP;
    p->events |= EV_WALLJUMP;
}

static void sense(Player *p, const Level *lv) {
    p->on_ground = p->vel.y >= 0 && col_rect_solid(lv, p->pos.x, p->pos.y + PH_H, PH_W, 1.0f);
    p->wall_dir = 0;
    if (col_rect_solid(lv, p->pos.x - 1.0f, p->pos.y + 1.0f, 1.0f, PH_H - 2.0f)) p->wall_dir = -1;
    else if (col_rect_solid(lv, p->pos.x + PH_W, p->pos.y + 1.0f, 1.0f, PH_H - 2.0f)) p->wall_dir = 1;
}

static void start_dash(Player *p, const PlayerInput *in, const Tuning *tu, float ax, float ay) {
    Vec2f d = { ax, ay };
    if (d.x == 0 && d.y == 0) d.x = (float)p->facing;
    if (d.x != 0 && d.y != 0) { d.x *= 0.7071f; d.y *= 0.7071f; }
    (void)in;
    p->dash_dir = d;
    p->dash_time = PH_DASH_TIME;
    p->dash_cd = PH_DASH_COOLDOWN;
    p->state = PS_DASH;
    p->vel.x = d.x * PH_DASH_SPEED;
    p->vel.y = d.y * PH_DASH_SPEED;
    if (!tu->infinite_dash) p->dash_ready = false;
    p->jump_cut = true;                 /* o dash para cima não deve ser cortado */
    if (d.x != 0) p->facing = d.x > 0 ? 1 : -1;
    p->events |= EV_DASH;
}

static void update_dash(Player *p, const Tuning *tu, float ax, float dt) {
    (void)tu;
    p->dash_time -= dt;
    p->vel.x = p->dash_dir.x * PH_DASH_SPEED;
    p->vel.y = p->dash_dir.y * PH_DASH_SPEED;
    if (p->jump_buf > 0 && p->coyote > 0 && p->dash_dir.y >= 0) {
        /* pulo durante o dash no chão: um "hyper" curto e rápido */
        p->state = PS_NORMAL;
        do_ground_jump(p, ax);
        p->vel.x = p->dash_dir.x * PH_DASHJUMP_X;
        p->dash_time = 0;
    } else if (p->dash_time <= 0) {
        p->state = PS_NORMAL;
        p->vel.x = p->dash_dir.x * PH_DASH_END_SPEED;
        p->vel.y = p->dash_dir.y * PH_DASH_END_SPEED;
        if (p->vel.y < 0) p->vel.y *= 0.75f;
    }
}

static void update_climb(Player *p, const PlayerInput *in, const Level *lv, const Tuning *tu, float ay, float dt) {
    p->facing = p->wall_dir;
    p->vel.x = 0;
    if (ay < 0)      { p->vel.y = -PH_CLIMB_UP;   p->stamina -= PH_STAMINA_UP * dt; }
    else if (ay > 0) { p->vel.y = PH_CLIMB_DOWN; }
    else             { p->vel.y = 0;              p->stamina -= PH_STAMINA_HOLD * dt; }

    if (p->jump_buf > 0) {
        do_wall_jump(p, tu);
        return;
    }
    /* Chegou à borda do topo: pequeno impulso para subir no degrau. */
    if (ay < 0) {
        float side_x = p->wall_dir < 0 ? p->pos.x - 1.0f : p->pos.x + PH_W;
        if (!col_rect_solid(lv, side_x, p->pos.y, 1.0f, 3.0f)) {
            p->state = PS_NORMAL;
            p->vel.y = -PH_LEDGE_HOP_Y;
            p->vel.x = p->wall_dir * PH_LEDGE_HOP_X;
            p->wj_lock = 0.12f;
            p->events |= EV_LEDGE;
        }
    }
    (void)in;
}

static void update_normal(Player *p, const PlayerInput *in, const Tuning *tu, float ax, float dt) {
    /* Horizontal */
    float target = ax * PH_RUN_MAX;
    float accel;
    if (ax == 0)
        accel = p->on_ground ? PH_FRICTION_GROUND : PH_FRICTION_AIR;
    else if (signf(p->vel.x) == ax && absf(p->vel.x) > PH_RUN_MAX)
        accel = PH_OVER_DECEL;
    else
        accel = p->on_ground ? PH_RUN_ACCEL : PH_AIR_ACCEL;
    if (p->wj_lock > 0) accel *= 0.35f;      /* limita o controle logo após um wall jump */
    p->vel.x = approachf(p->vel.x, target, accel * dt);

    /* Vertical */
    if (p->on_ground && p->vel.y >= 0) {
        p->vel.y = 0;
    } else {
        float g = PH_GRAVITY;
        if (absf(p->vel.y) < PH_APEX_THRESH && in->jump_held) g *= 0.5f;
        float max_fall = PH_FALL_MAX;
        if (p->wall_dir != 0 && ax == (float)p->wall_dir && p->vel.y > 0)
            max_fall = PH_WALL_SLIDE;
        p->vel.y = approachf(p->vel.y, max_fall, g * dt);
    }

    /* Altura variável: soltar o botão durante a subida corta o pulo. */
    if (p->vel.y < 0 && !in->jump_held && !p->jump_cut) {
        p->vel.y *= PH_JUMP_CUT;
        p->jump_cut = true;
    }
    if (p->vel.y >= 0) p->jump_cut = false;

    /* Pulos (com coyote time e jump buffering) */
    if (p->jump_buf > 0) {
        if (p->coyote > 0)
            do_ground_jump(p, ax);
        else if (p->wall_dir != 0)
            do_wall_jump(p, tu);
    }
}

void player_update(Player *p, const PlayerInput *in, Level *lv, const Tuning *tu, float dt) {
    p->events = 0;
    if (p->state == PS_DEAD) return;
    p->anim_t += dt;

    float ax = (in->right ? 1.0f : 0.0f) - (in->left ? 1.0f : 0.0f);
    float ay = (in->down ? 1.0f : 0.0f) - (in->up ? 1.0f : 0.0f);

    sense(p, lv);

    p->coyote -= dt;
    p->jump_buf -= dt;
    p->dash_cd -= dt;
    p->wj_lock -= dt;
    if (in->jump_pressed) p->jump_buf = tu->jump_buffer_time;

    if (p->on_ground) {
        p->coyote = tu->coyote_time;
        p->stamina = tu->stamina_max;
        if (p->state != PS_DASH && p->dash_cd <= 0) p->dash_ready = true;
    }
    if (ax != 0 && p->state != PS_CLIMB && p->wj_lock <= 0) p->facing = ax > 0 ? 1 : -1;

    /* Início do dash */
    if (in->dash_pressed && p->dash_ready && p->dash_cd <= 0 && p->state != PS_DASH)
        start_dash(p, in, tu, ax, ay);

    if (p->state == PS_DASH) {
        update_dash(p, tu, ax, dt);
    } else {
        bool want_grab = in->grab_held && p->wall_dir != 0 && p->stamina > 0 && p->wj_lock <= 0;
        if (want_grab) p->state = PS_CLIMB;
        else if (p->state == PS_CLIMB) p->state = PS_NORMAL;

        if (p->state == PS_CLIMB) update_climb(p, in, lv, tu, ay, dt);
        if (p->state == PS_NORMAL) update_normal(p, in, tu, ax, dt);
    }

    /* Movimento com colisão */
    float vy_before = p->vel.y;
    if (phys_move_x(lv, &p->pos, PH_W, PH_H, p->vel.x * dt) && p->state != PS_DASH)
        p->vel.x = 0;
    bool hit_y = phys_move_y(lv, &p->pos, PH_W, PH_H, p->vel.y * dt);
    if (hit_y) {
        if (p->vel.y > 0) {
            if (vy_before > 120.0f) p->events |= EV_LAND;
        } else if (p->vel.y < 0) {
            /* Correção de quina: empurra até 3px para contornar a quina do teto. */
            bool nudged = false;
            for (int n = 1; n <= 3 && !nudged; n++)
                for (int s = -1; s <= 1 && !nudged; s += 2)
                    if (!col_rect_solid(lv, p->pos.x + s * n, p->pos.y - 1.0f, PH_W, PH_H)) {
                        p->pos.x += (float)(s * n);
                        nudged = true;
                    }
            if (nudged) goto after_y;
        }
        p->vel.y = 0;
        if (p->state == PS_DASH && p->dash_dir.y > 0) p->dash_time = 0;
    }
after_y:

    /* Plataformas frágeis tremem quando alguém está em cima. */
    if (p->on_ground) level_touch_below(lv, player_hitbox(p));

    /* Perigos */
    Rectf hb = { p->pos.x + 1.0f, p->pos.y + 2.0f, PH_W - 2.0f, PH_H - 3.0f };
    if (col_rect_hazard(lv, hb) || p->pos.y > lv->height * TILE_SIZE + 24.0f)
        player_kill(p);
}
