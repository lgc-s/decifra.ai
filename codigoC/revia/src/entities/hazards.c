#include "entities/hazards.h"

#include <string.h>

#define RAIN_HEADROOM   10        /* tiles livres acima da superfície onde o objeto cai */
#define RAIN_ACTIVE_R   220.0f    /* só começa a chover quando o jogador chega perto (px) */
#define BUG_MIN_RUN     5         /* comprimento mínimo de chão plano para um bug (tiles) */
#define BUG_MAX_SEG     12        /* comprimento máximo de uma patrulha (tiles) */
#define BUG_HEADROOM    4         /* tiles livres acima do chão (dá para pular por cima) */
#define MAX_SEGS        128

static uint32_t rnd(uint32_t *s) {
    *s = *s * 1664525u + 1013904223u;
    return *s >> 8;
}

static float rndf(uint32_t *s, float lo, float hi) {
    return lo + (hi - lo) * (float)(rnd(s) % 1000u) / 999.0f;
}

/* Vazio de verdade (não conta espinho, plataforma frágil nem ilusória). Acima do mapa é aberto. */
static bool is_empty(const Level *lv, int tx, int ty) {
    if (tx < 0 || tx >= lv->width) return false;
    if (ty < 0 || ty >= lv->height) return true;
    return lv->tiles[ty][tx] == TILE_EMPTY;
}

static bool is_floor(const Level *lv, int tx, int ty) {
    if (tx < 0 || tx >= lv->width || ty < 0 || ty >= lv->height) return false;
    return lv->tiles[ty][tx] == TILE_SOLID;
}

static void protect(bool *prot, const Level *lv, float px, int radius) {
    int c = (int)(px / TILE_SIZE);
    for (int tx = c - radius; tx <= c + radius; tx++)
        if (tx >= 0 && tx < lv->width) prot[tx] = true;
}

static float density(int act, bool rain) {
    static const float RAIN[4] = { 2.5f, 4.0f, 4.5f, 5.0f };      /* por 100 tiles de largura */
    static const float BUGS[4] = { 2.0f, 3.5f, 4.0f, 4.5f };
    int i = act < 1 ? 0 : (act > 4 ? 3 : act - 1);
    return rain ? RAIN[i] : BUGS[i];
}

static int count_for(const Level *lv, bool rain, Difficulty diff, int cap) {
    float scale = diff == DIFF_ASSIST ? 0.5f : (diff == DIFF_CHALLENGE ? 1.5f : 1.0f);
    int n = (int)((float)lv->width * density(lv->act, rain) / 100.0f * scale + 0.5f);
    if (n < 1) n = 1;
    return n > cap ? cap : n;
}

void hazards_init(Hazards *hz, const Level *lv, Difficulty diff) {
    memset(hz, 0, sizeof *hz);
    hz->warn_time = diff == DIFF_ASSIST ? 1.5f : (diff == DIFF_CHALLENGE ? 0.7f : 1.0f);

    /* colunas onde nada é colocado: início, checkpoints, saída, terminais, cristais */
    bool prot[LEVEL_MAX_W];
    memset(prot, 0, sizeof prot);
    protect(prot, lv, lv->spawn.x, 5);
    for (int i = 0; i < lv->ncheckpoints; i++) {
        protect(prot, lv, lv->checkpoints[i].center.x, 4);
        protect(prot, lv, lv->checkpoints[i].respawn.x, 4);
    }
    if (lv->has_exit) protect(prot, lv, lv->exit_pos.x, 5);
    for (int i = 0; i < lv->ninteractables; i++) protect(prot, lv, lv->interactables[i].pos.x, 3);
    for (int i = 0; i < lv->ncrystals; i++) protect(prot, lv, lv->crystals[i].pos.x, 3);

    uint32_t seed = 9176u * (uint32_t)(lv->act < 1 ? 1 : lv->act) + (uint32_t)lv->width * 31u;

    /* ---------- objetos que caem do céu ---------- */
    int want = count_for(lv, true, diff, HZ_MAX_RAIN);
    int x_lo = 4, x_hi = lv->width - 4;
    for (int k = 0; k < want && x_hi > x_lo && lv->height > 8; k++) {
        int s0 = x_lo + k * (x_hi - x_lo) / want;
        int s1 = x_lo + (k + 1) * (x_hi - x_lo) / want;
        /* candidatas: superfícies sólidas com céu livre em cima e área aberta ao redor */
        int cand_x[96], cand_y[96], nc = 0;
        for (int tx = s0; tx < s1 && nc < 96; tx++) {
            if (prot[tx]) continue;
            for (int ty = 6; ty < lv->height && nc < 96; ty++) {
                if (!is_floor(lv, tx, ty)) continue;
                bool clear = true;
                for (int h = 1; h <= RAIN_HEADROOM && clear; h++) clear = is_empty(lv, tx, ty - h);
                for (int dx = -2; dx <= 2 && clear; dx++)     /* área aberta ao redor: dá para desviar */
                    for (int h = 1; h <= 4 && clear; h++) clear = is_empty(lv, tx + dx, ty - h);
                if (clear) { cand_x[nc] = tx; cand_y[nc] = ty; nc++; }
            }
        }
        if (nc == 0) continue;
        int pick = (int)(rnd(&seed) % (uint32_t)nc);
        int tx = cand_x[pick], ty = cand_y[pick];
        Rain *r = &hz->rain[hz->nrain++];
        r->x = (float)tx * TILE_SIZE + TILE_SIZE * 0.5f;
        r->y_land = (float)ty * TILE_SIZE;
        r->y_start = r->y_land - (float)RAIN_HEADROOM * TILE_SIZE - RAIN_H;
        r->y = r->y_start;
        r->rng = seed ^ (uint32_t)(tx * 2654435761u);
        r->wait = rndf(&r->rng, 0.4f, 2.2f);               /* defasagem inicial: não caem todos juntos */
        r->state = RAIN_WAIT;
    }

    /* ---------- bugs que andam no chão ---------- */
    typedef struct { int tx0, tx1, ty; } Seg;
    static Seg segs[MAX_SEGS];
    int nseg = 0;
    for (int ty = 6; ty < lv->height; ty++) {
        int tx = 1;
        while (tx < lv->width - 1) {
            bool ok = !prot[tx] && is_floor(lv, tx, ty);
            for (int h = 1; h <= BUG_HEADROOM && ok; h++) ok = is_empty(lv, tx, ty - h);
            if (!ok) { tx++; continue; }
            int a = tx;
            while (tx < lv->width - 1) {
                bool ok2 = !prot[tx] && is_floor(lv, tx, ty);
                for (int h = 1; h <= BUG_HEADROOM && ok2; h++) ok2 = is_empty(lv, tx, ty - h);
                if (!ok2) break;
                tx++;
            }
            int b = tx - 1;                              /* chão plano contínuo de a até b */
            for (int s = a; b - s + 1 >= BUG_MIN_RUN && nseg < MAX_SEGS; s += BUG_MAX_SEG) {
                int e = s + BUG_MAX_SEG - 1;
                if (e > b) e = b;
                if (e - s + 1 < BUG_MIN_RUN) break;
                segs[nseg].tx0 = s; segs[nseg].tx1 = e; segs[nseg].ty = ty; nseg++;
            }
        }
    }
    /* ordena por x para espalhar a escolha pela fase inteira */
    for (int i = 1; i < nseg; i++) {
        Seg v = segs[i]; int k = i - 1;
        while (k >= 0 && segs[k].tx0 > v.tx0) { segs[k + 1] = segs[k]; k--; }
        segs[k + 1] = v;
    }
    int nb = count_for(lv, false, diff, HZ_MAX_BUGS);
    if (nb > nseg) nb = nseg;
    float base = diff == DIFF_ASSIST ? 18.0f : (diff == DIFF_CHALLENGE ? 38.0f : 28.0f);
    for (int k = 0; k < nb; k++) {
        const Seg *sg = &segs[(k * nseg + nseg / 2) / nb];
        Bug *b = &hz->bugs[hz->nbugs++];
        b->x0 = (float)sg->tx0 * TILE_SIZE;
        b->x1 = (float)(sg->tx1 + 1) * TILE_SIZE - BUG_W;
        b->y = (float)sg->ty * TILE_SIZE;
        b->speed = base * rndf(&seed, 0.8f, 1.2f);
        b->dir = (rnd(&seed) & 1u) ? 1 : -1;
        b->x = b->x0 + (b->x1 - b->x0) * rndf(&seed, 0.0f, 1.0f);
    }
}

static bool overlap(float ax, float ay, float aw, float ah, Rectf b) {
    return ax < b.x + b.w && ax + aw > b.x && ay < b.y + b.h && ay + ah > b.y;
}

void hazards_update(Hazards *hz, const Level *lv, Player *p, float dt, HazardEvents *ev) {
    (void)lv;
    memset(ev, 0, sizeof *ev);
    bool alive = p->state != PS_DEAD;
    Rectf hb = { p->pos.x + 1.0f, p->pos.y + 2.0f, PH_W - 2.0f, PH_H - 3.0f };   /* mesma folga dos espinhos */

    for (int i = 0; i < hz->nbugs; i++) {
        Bug *b = &hz->bugs[i];
        b->x += (float)b->dir * b->speed * dt;
        if (b->x <= b->x0) { b->x = b->x0; b->dir = 1; }
        if (b->x >= b->x1) { b->x = b->x1; b->dir = -1; }
        if (alive && overlap(b->x + 1.0f, b->y - BUG_H, BUG_W - 2.0f, BUG_H, hb)) {
            player_kill(p);
            alive = false;
        }
    }

    float pcx = p->pos.x + PH_W * 0.5f;
    for (int i = 0; i < hz->nrain; i++) {
        Rain *r = &hz->rain[i];
        float dx = r->x - pcx;
        switch (r->state) {
        case RAIN_WAIT:
            if (dx < -RAIN_ACTIVE_R || dx > RAIN_ACTIVE_R) { r->t = 0; break; }
            r->t += dt;
            if (r->t >= r->wait) { r->state = RAIN_WARN; r->t = 0; }
            break;
        case RAIN_WARN:
            r->t += dt;
            if (r->t >= hz->warn_time) { r->state = RAIN_FALL; r->t = 0; r->y = r->y_start; }
            break;
        case RAIN_FALL: {
            r->t += dt;
            float v = 70.0f + 300.0f * r->t;                 /* acelera até uma velocidade máxima */
            if (v > 230.0f) v = 230.0f;
            r->y += v * dt;
            if (alive && overlap(r->x - RAIN_W * 0.5f, r->y, RAIN_W, RAIN_H, hb)) {
                player_kill(p);
                alive = false;
            }
            if (r->y + RAIN_H >= r->y_land) {
                r->y = r->y_land - RAIN_H;
                r->state = RAIN_LAND; r->t = 0;
                if (!ev->landed && dx > -260.0f && dx < 260.0f) {
                    ev->landed = true; ev->land_x = r->x; ev->land_y = r->y_land;
                }
            }
            break; }
        case RAIN_LAND:
            r->t += dt;
            if (r->t >= 0.30f) {
                r->state = RAIN_WAIT; r->t = 0;
                r->wait = rndf(&r->rng, 1.6f, 3.2f);
            }
            break;
        default: r->state = RAIN_WAIT; break;
        }
    }
}