#include "render/world_draw.h"
#include "core/game.h"
#include "narrative/dialogue.h"
#include "render/gfx.h"
#include "render/character.h"
#include "world/collision.h"

#include <math.h>
#include <string.h>

static int ifl(float v) { return (int)floorf(v); }

static Color shade(Color c, float k) {         /* k<1 escurece, k>1 clareia */
    int r = (int)(c.r * k), g = (int)(c.g * k), b = (int)(c.b * k);
    c.r = (unsigned char)(r > 255 ? 255 : r);
    c.g = (unsigned char)(g > 255 ? 255 : g);
    c.b = (unsigned char)(b > 255 ? 255 : b);
    return c;
}

void draw_glow(int cx, int cy, float radius, Color c, float strength) {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 4; i++)
        DrawCircle(cx, cy, radius * (1.0f - 0.22f * (float)i), Fade(c, 0.05f * strength));
    EndBlendMode();
}

/* ------------------------------------------------------------------ */
/* Fundo com parallax                                                  */
/* ------------------------------------------------------------------ */

static void window_grid(int x, int y, int w, int h, int layer, int seed, Color lit, float t) {
    for (int wy = y + 3; wy < y + h - 2; wy += 5) {
        for (int wx = x + 3; wx < x + w - 3; wx += 4) {
            unsigned hv = gfx_hash(wx / 4 + seed * 13, wy / 5 + layer * 7);
            if (hv % 4 != 0) continue;
            float flick = (hv % 11 == 0) ? (0.6f + 0.4f * sinf(t * 3.0f + (float)hv)) : 1.0f;
            DrawRectangle(wx, wy, 2, 2, Fade(lit, 0.55f * flick));
        }
    }
}

static void skyline(const Palette *pal, Color col, float scroll, int base, int cell, int minh, int maxh, int layer, float t) {
    int first = (int)floorf(scroll / (float)cell);
    int count = VIEW_W / cell + 3;
    for (int k = 0; k < count; k++) {
        int i = first + k;
        unsigned hv = gfx_hash(i, layer);
        int x = i * cell - (int)scroll;
        int w = cell - 4 - (int)(hv % 5);
        int h = minh + (int)((hv >> 3) % (unsigned)(maxh - minh));
        int y = base - h;
        switch (pal->style) {
        case 1:   /* torres altas e finas, com antena */
            w = cell - 8;
            DrawRectangle(x, y - 6, w, h + 6, col);
            DrawRectangle(x + w / 2, y - 12, 1, 6, col);
            window_grid(x, y - 6, w, h + 6, layer, i, pal->mote, t);
            break;
        case 2:   /* gráfico de barras: dados de crédito */
            DrawRectangle(x, y, w, h, col);
            DrawRectangle(x, y, w, 1, Fade(pal->accent, layer ? 0.7f : 0.35f));
            if (hv % 3 == 0) DrawCircle(x + w / 2, y - 3, 2.0f, Fade(pal->accent, 0.6f));
            break;
        case 3:   /* colunas de jornal: linhas de texto */
            DrawRectangle(x, y, w, h, col);
            for (int ly = y + 4; ly < base - 2; ly += 4)
                if (gfx_hash(i, ly) % 5 != 0) DrawRectangle(x + 2, ly, w - 4 - (int)(gfx_hash(ly, i) % 4), 1, Fade(pal->mote, 0.22f));
            break;
        case 4:   /* clínica: baixos, largos, com cruz */
            h = h * 2 / 3 + 8;
            y = base - h;
            DrawRectangle(x, y, w + 4, h, col);
            if (hv % 3 == 0) {
                DrawRectangle(x + w / 2, y + 3, 2, 8, Fade(pal->spike, 0.7f));
                DrawRectangle(x + w / 2 - 3, y + 6, 8, 2, Fade(pal->spike, 0.7f));
            }
            window_grid(x, y, w + 4, h, layer, i, WHITE, t);
            break;
        case 5: { /* quebrado: topo irregular e janelas apagadas */
            DrawRectangle(x, y, w, h, col);
            int notch = 3 + (int)(hv % 6);
            DrawRectangle(x + w - notch - 2, y, notch + 2, 4 + (int)(hv % 5), pal->sky_bot);
            if (hv % 4 == 0) DrawRectangle(x + 2, y + 6, 2, 2, Fade(pal->accent, 0.5f));
            break; }
        default:  /* 0 e 6: prédios de escritório */
            DrawRectangle(x, y, w, h, col);
            window_grid(x, y, w, h, layer, i, pal->mote, t);
            break;
        }
    }
}

void draw_background(const Palette *pal, float cam_x, float cam_y, float t) {
    DrawRectangleGradientV(0, 0, VIEW_W, VIEW_H, pal->sky_top, pal->sky_bot);

    /* astro no céu: lua (escritório), sol baixo (desfecho/trânsito) */
    if (pal->style == 0 || pal->style == 6 || pal->style == 1) {
        int sx = 300 - (int)(cam_x * 0.03f), sy = pal->style == 0 ? 36 : VIEW_H - 78;
        Color sc = pal->style == 0 ? (Color){ 220, 230, 255, 255 } : (Color){ 255, 232, 170, 255 };
        draw_glow(sx, sy, 34.0f, sc, 1.2f);
        DrawCircle(sx, sy, pal->style == 0 ? 9.0f : 14.0f, Fade(sc, 0.95f));
    }
    /* estrelas discretas no alto */
    for (int i = 0; i < 40; i++) {
        unsigned h = gfx_hash(i, 99);
        int x = (int)(h % VIEW_W);
        int y = (int)((h >> 9) % (VIEW_H / 2));
        float tw = 0.4f + 0.6f * fabsf(sinf(t * 1.5f + (float)i));
        if (pal->style != 4) DrawRectangle(x, y, 1, 1, Fade(WHITE, 0.35f * tw));
    }
    float vy = 0.12f * cam_y;
    skyline(pal, pal->far_c, cam_x * 0.12f, VIEW_H + 6 - (int)vy, 28, 70, 118, 0, t);
    skyline(pal, pal->mid_c, cam_x * 0.28f, VIEW_H + 22 - (int)(vy * 2.0f), 22, 44, 92, 1, t);
}

/* ------------------------------------------------------------------ */
/* Tiles                                                               */
/* ------------------------------------------------------------------ */

static void draw_solid(const Level *lv, const Palette *pal, int tx, int ty, int px, int py) {
    unsigned hv = gfx_hash(tx, ty);
    Color base = (hv & 1u) ? pal->tile : pal->tile_alt;

    /* mais fundo = mais escuro */
    int depth = 0;
    while (depth < 6 && col_tile_solid(lv, tx, ty - 1 - depth)) depth++;
    base = shade(base, 1.0f - 0.07f * (float)depth);
    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, base);

    if (!col_tile_solid(lv, tx, ty - 1)) {
        DrawRectangle(px, py, TILE_SIZE, 2, pal->tile_top);
        if (hv % 5 == 0) DrawRectangle(px + 2 + (int)(hv % 3), py - 1, 2, 1, pal->tile_top);
        DrawRectangle(px, py + 2, TILE_SIZE, 1, shade(pal->tile_top, 0.7f));
    } else if (hv % 9 == 0) {
        DrawRectangle(px + 2 + (int)(hv % 4), py + 3, 2, 1, shade(base, 0.8f));
    }
    if (tx > 0 && !col_tile_solid(lv, tx - 1, ty)) DrawRectangle(px, py, 1, TILE_SIZE, shade(base, 1.25f));
    if (!col_tile_solid(lv, tx + 1, ty)) DrawRectangle(px + TILE_SIZE - 1, py, 1, TILE_SIZE, shade(base, 0.75f));
}

static void draw_spike(const Palette *pal, int px, int py) {
    for (int k = 0; k < 2; k++) {
        int bx = px + k * 4;
        DrawTriangle((Vector2){ (float)bx + 2, (float)py + 2 }, (Vector2){ (float)bx, (float)py + 8 },
                     (Vector2){ (float)bx + 4, (float)py + 8 }, pal->spike);
        DrawRectangle(bx + 2, py + 3, 1, 3, shade(pal->spike, 1.4f));
    }
}

static void draw_crumble(const Level *lv, const Palette *pal, int tx, int ty, int px, int py, float t) {
    float c = lv->crumble[ty][tx];
    if (c < 0.0f) {      /* ausente: contorno tracejado que indica o retorno */
        if (((int)(t * 8) + tx) % 2 == 0) DrawRectangleLines(px + 1, py + 1, TILE_SIZE - 2, TILE_SIZE - 2, Fade(pal->accent, 0.18f));
        return;
    }
    int jx = 0, jy = 0;
    if (c > 0.0f) { jx = (int)(gfx_hash((int)(t * 60), tx) % 3) - 1; jy = (int)(gfx_hash(ty, (int)(t * 60)) % 2); }
    Color body = shade(pal->tile, 1.15f);
    DrawRectangle(px + jx, py + jy, TILE_SIZE, TILE_SIZE, body);
    DrawRectangle(px + jx, py + jy, TILE_SIZE, 2, pal->accent);
    /* rachaduras: "dado incompleto" */
    DrawRectangle(px + jx + 2, py + jy + 3, 1, 3, shade(body, 0.55f));
    DrawRectangle(px + jx + 5, py + jy + 4, 1, 3, shade(body, 0.55f));
    DrawRectangle(px + jx + 3, py + jy + 6, 3, 1, shade(body, 0.55f));
}

static void draw_ghost(const Palette *pal, int px, int py, float t, int tx) {
    float a = 0.35f + 0.25f * sinf(t * 5.0f + (float)tx);
    DrawRectangle(px, py, TILE_SIZE, 3, Fade(pal->accent, a));
    DrawRectangle(px, py + 3, TILE_SIZE, 1, Fade(pal->accent, a * 0.5f));
}

/* ------------------------------------------------------------------ */
/* Entidades                                                           */
/* ------------------------------------------------------------------ */

static void draw_checkpoint(const Palette *pal, const Checkpoint *cp, bool active, int ox, int oy, float t) {
    int x = (int)cp->center.x + ox, y = (int)cp->center.y + oy;
    DrawRectangle(x - 1, y - 6, 2, 10, (Color){ 70, 76, 100, 255 });
    DrawRectangle(x - 3, y + 3, 6, 1, (Color){ 50, 54, 78, 255 });
    Color lamp = active ? pal->accent : (Color){ 110, 116, 140, 255 };
    DrawRectangle(x - 2, y - 9, 4, 4, lamp);
    if (active) {
        float pulse = 0.8f + 0.2f * sinf(t * 4.0f);
        draw_glow(x, y - 7, 15.0f * pulse, pal->accent, 1.4f);
    }
}

static void draw_crystal(const Palette *pal, const Crystal *cr, int ox, int oy, float t) {
    int x = (int)cr->pos.x + ox;
    int y = (int)(cr->pos.y + sinf(t * 3.0f + cr->pos.x) * 1.5f) + oy;
    if (cr->cooldown > 0) {
        DrawPoly((Vector2){ (float)x, (float)y }, 4, 3.0f, 0.0f, Fade(pal->accent, 0.25f));
        return;
    }
    draw_glow(x, y, 12.0f, pal->accent, 1.2f);
    DrawPoly((Vector2){ (float)x, (float)y }, 4, 5.0f, 0.0f, pal->accent);
    DrawPoly((Vector2){ (float)x, (float)y }, 4, 2.5f, 0.0f, (Color){ 240, 252, 255, 255 });
}

static void draw_collectible(const Palette *pal, const Collectible *co, int ox, int oy, float t) {
    if (co->taken) return;
    int x = (int)co->pos.x + ox;
    int y = (int)(co->pos.y + sinf(t * 2.5f + co->pos.x) * 2.0f) + oy;
    Color c = (Color){ 255, 226, 120, 255 };
    draw_glow(x, y, 14.0f, c, 1.3f);
    /* "chip de dados" */
    DrawRectangle(x - 3, y - 3, 6, 6, c);
    DrawRectangle(x - 2, y - 2, 4, 4, (Color){ 120, 80, 30, 255 });
    DrawRectangle(x - 1, y - 1, 2, 2, c);
    for (int i = -2; i <= 2; i += 2) {
        DrawRectangle(x + i, y - 5, 1, 2, c);
        DrawRectangle(x + i, y + 4, 1, 2, c);
    }
    (void)pal;
}

static void draw_person(int x, int y, Color coat, float t) {
    int br = ((int)(t * 1.6f)) % 2;
    DrawRectangle(x - 2, y - 7 + br, 4, 4, (Color){ 240, 196, 160, 255 });       /* cabeça */
    DrawRectangle(x - 2, y - 7 + br, 4, 2, (Color){ 52, 40, 60, 255 });          /* cabelo */
    DrawRectangle(x - 3, y - 3 + br, 6, 6, coat);                                /* jaleco */
    DrawRectangle(x - 2, y + 3, 2, 2, (Color){ 40, 44, 80, 255 });
    DrawRectangle(x, y + 3, 2, 2, (Color){ 40, 44, 80, 255 });
}

static void draw_sign(int x, int y) {
    DrawRectangle(x, y - 1, 1, 6, (Color){ 100, 82, 64, 255 });
    DrawRectangle(x - 5, y - 8, 11, 7, (Color){ 196, 168, 120, 255 });
    DrawRectangle(x - 4, y - 6, 9, 1, (Color){ 100, 82, 64, 255 });
    DrawRectangle(x - 4, y - 4, 6, 1, (Color){ 100, 82, 64, 255 });
}

static void draw_terminal(int x, int y, const Palette *pal, Color screen, float t, const char *mark) {
    DrawRectangle(x - 4, y - 7, 9, 8, (Color){ 40, 46, 70, 255 });
    DrawRectangle(x - 3, y - 6, 7, 5, screen);
    DrawRectangle(x - 1, y + 1, 3, 2, (Color){ 40, 46, 70, 255 });
    DrawRectangle(x - 3, y + 3, 7, 1, (Color){ 40, 46, 70, 255 });
    if (mark[0] == '?') {                                   /* ponto de interrogação em pixels */
        Color ink = { 20, 24, 40, 255 };
        DrawRectangle(x - 1, y - 6, 3, 1, ink);
        DrawRectangle(x + 1, y - 5, 1, 2, ink);
        DrawRectangle(x, y - 3, 1, 1, ink);
        DrawRectangle(x, y - 1, 1, 1, ink);
    } else if (((int)(t * 2)) % 2) DrawRectangle(x - 2, y - 3, 2, 1, (Color){ 20, 24, 40, 255 });
    draw_glow(x, y - 4, 11.0f, pal->accent, 0.7f);
}

static void draw_server(int x, int y, const Palette *pal, float t) {
    DrawRectangle(x - 5, y - 10, 11, 14, (Color){ 38, 42, 66, 255 });
    for (int i = 0; i < 3; i++) {
        DrawRectangle(x - 4, y - 9 + i * 4, 9, 3, (Color){ 56, 62, 94, 255 });
        bool on = ((int)(t * 4) + i) % 3 != 0;
        DrawRectangle(x + 2, y - 8 + i * 4, 2, 1, on ? pal->accent : (Color){ 80, 40, 40, 255 });
    }
    draw_glow(x, y - 3, 14.0f, pal->accent, 0.8f);
}

static void draw_phone(int x, int y, bool done, const Palette *pal, float t) {
    Color body = done ? (Color){ 90, 96, 120, 255 } : (Color){ 235, 236, 245, 255 };
    DrawRectangle(x - 3, y - 6, 6, 10, body);
    DrawRectangle(x - 2, y - 5, 4, 6, done ? (Color){ 60, 64, 90, 255 } : pal->accent);
    if (!done) {
        int k = ((int)(t * 6)) % 3;
        DrawRectangle(x + 4 + k, y - 5, 1, 4, Fade(pal->accent, 0.8f - 0.25f * (float)k));
        DrawRectangle(x - 5 - k, y - 5, 1, 4, Fade(pal->accent, 0.8f - 0.25f * (float)k));
    }
}

static void draw_interactable(const Game *g, const Interactable *it, bool near, int ox, int oy) {
    const Palette *pal = &g->pal;
    int x = (int)it->pos.x + ox, y = (int)it->pos.y + oy + 3;   /* pés na base do tile */
    float t = g->time;
    switch (it->kind) {
    case INT_DIALOGUE: {
        const Dialogue *d = dialogue_get(it->id);
        const char *who = d ? d->lines[0].speaker : "";
        if (strcmp(who, "Supervisão") == 0) draw_person(x, y, (Color){ 120, 170, 240, 255 }, t);
        else if (strcmp(who, "Placa") == 0) draw_sign(x, y);
        else draw_terminal(x, y, pal, (Color){ 140, 230, 200, 255 }, t, "");
        break; }
    case INT_CALL:
        draw_phone(x, y - 1, (g->prog.calls_done >> it->id) & 1u, pal, t);
        break;
    case INT_QUIZ:
        draw_terminal(x, y, pal, (Color){ 255, 226, 120, 255 }, t, "?");
        break;
    case INT_BIAS:
        draw_server(x, y, pal, t);
        break;
    }
    if (near) {
        int by = y - 20 + (int)(sinf(t * 6.0f) * 1.5f);
        gfx_panel(x - 6, by, 12, 11, (Color){ 12, 14, 30, 230 }, UI_ACCENT);
        DrawText("E", x - 2, by + 1, 10, UI_TEXT);
    }
}

static void draw_exit(const Game *g, int ox, int oy) {
    const Level *lv = &g->level;
    if (!lv->has_exit) return;
    char msg[8];
    bool ready = progress_act_ready(&g->prog, lv->act, msg, sizeof msg);
    int x = (int)lv->exit_pos.x + ox, y = (int)lv->exit_pos.y + oy + 4;
    Color door = ready ? g->pal.accent : (Color){ 90, 94, 120, 255 };
    DrawRectangle(x - 7, y - 18, 14, 18, shade(door, 0.45f));
    DrawRectangle(x - 5, y - 16, 10, 16, ready ? Fade(door, 0.9f) : (Color){ 40, 44, 66, 255 });
    if (ready) {
        draw_glow(x, y - 9, 24.0f + 2.0f * sinf(g->time * 3.0f), door, 1.6f);
    } else {
        DrawRectangle(x - 2, y - 10, 4, 4, (Color){ 150, 154, 176, 255 });     /* cadeado */
        DrawRectangle(x - 1, y - 7, 2, 3, (Color){ 150, 154, 176, 255 });
    }
}

/* Perigos móveis: bugs no chão e objetos que caem do céu (com aviso piscando). */
static void draw_hazards(const Game *g, int ox, int oy) {
    const Hazards *hz = &g->haz;
    const Palette *pal = &g->pal;
    float t = g->time;

    for (int i = 0; i < hz->nrain; i++) {
        const Rain *r = &hz->rain[i];
        int cx = (int)r->x + ox, ly = (int)r->y_land + oy;
        if (cx < -16 || cx > VIEW_W + 16) continue;

        if (r->state == RAIN_WARN || r->state == RAIN_FALL) {          /* faixa por onde o objeto vai cair */
            for (int yy = (int)r->y_start + oy; yy < ly - 8; yy += 6)
                DrawRectangle(cx, yy, 1, 2, Fade(pal->spike, 0.22f));
        }
        if (r->state == RAIN_WARN) {                                    /* "!" piscando cada vez mais rápido */
            float k = r->t / hz->warn_time;
            if (((int)(t * (6.0f + 12.0f * k))) % 2 == 0) {
                DrawRectangle(cx - 5, ly - 2, 10, 2, pal->spike);
                DrawRectangle(cx - 1, ly - 13, 2, 6, pal->spike);
                DrawRectangle(cx - 1, ly - 5, 2, 2, pal->spike);
            } else {
                DrawRectangle(cx - 5, ly - 2, 10, 2, Fade(pal->spike, 0.35f));
            }
        }
        if (r->state == RAIN_FALL) {                                    /* objeto com rastro */
            int px = cx - 3, py = (int)r->y + oy;
            for (int k = 1; k <= 3; k++) DrawRectangle(cx - 1, py - k * 5, 2, 3, Fade(pal->spike, 0.35f - 0.09f * (float)k));
            DrawRectangle(px, py, 6, 7, pal->spike);
            DrawRectangle(px, py, 6, 1, shade(pal->spike, 1.5f));
            DrawRectangle(px + 1, py + 2, 1, 1, WHITE);                 /* "x" de erro */
            DrawRectangle(px + 4, py + 2, 1, 1, WHITE);
            DrawRectangle(px + 2, py + 3, 2, 1, WHITE);
            DrawRectangle(px + 1, py + 4, 1, 1, WHITE);
            DrawRectangle(px + 4, py + 4, 1, 1, WHITE);
        }
        if (r->state == RAIN_LAND) {                                    /* impacto */
            int sp = 2 + (int)(r->t * 30.0f);
            DrawRectangle(cx - sp - 3, ly - 2, 3, 1, Fade(pal->spike, 0.7f));
            DrawRectangle(cx + sp, ly - 2, 3, 1, Fade(pal->spike, 0.7f));
            DrawRectangle(cx - 2, ly - 3, 4, 1, Fade(pal->spike, 0.5f));
        }
    }

    for (int i = 0; i < hz->nbugs; i++) {
        const Bug *b = &hz->bugs[i];
        int bx = (int)b->x + ox, by = (int)b->y + oy;
        if (bx < -12 || bx > VIEW_W + 12) continue;
        int step = ((int)(t * 8.0f)) % 2;
        Color body = pal->spike, dark = shade(body, 0.55f);
        DrawRectangle(bx + 1, by - 6, 6, 5, body);                      /* corpo */
        DrawRectangle(bx, by - 5, 8, 3, body);
        DrawRectangle(bx + 1, by - 6, 6, 1, shade(body, 1.4f));
        int ex = b->dir > 0 ? bx + 5 : bx + 1;                          /* olhos olham para onde anda */
        DrawRectangle(ex, by - 5, 2, 2, WHITE);
        DrawRectangle(ex + (b->dir > 0 ? 1 : 0), by - 5, 1, 1, BLACK);
        DrawRectangle(b->dir > 0 ? bx + 7 : bx, by - 8, 1, 2, dark);    /* antena */
        for (int k = 0; k < 3; k++)                                     /* pernas */
            DrawRectangle(bx + 1 + k * 3, by - 1, 1, 1 + ((k + step) % 2), dark);
    }
}

/* ------------------------------------------------------------------ */
/* Protagonista                                                        */
/* ------------------------------------------------------------------ */

void draw_player(const Player *p, const Palette *pal, int character, float t, int ox, int oy){
    if (p->state == PS_DEAD) return;
    int x = ifl(p->pos.x) + ox, y = ifl(p->pos.y) + oy;
    int f = p->facing >= 0 ? 1 : -1;
    const CharacterStyle *cs = character_style(character);       /* cores do personagem escolhido */
    const Color coat = cs->coat, coat2 = cs->coat2;
    const Color pants = cs->pants, skin = cs->skin;
    const Color hair = cs->hair, eye = { 24, 24, 40, 255 };
    Color badge = p->dash_ready ? pal->accent : (Color){ 130, 110, 150, 255 };

#define R(rx, ry, w, h, col) DrawRectangle(f > 0 ? x + (rx) : x + 6 - (rx) - (w), y + (ry), (w), (h), (col))
    bool run = fabsf(p->vel.x) > 10.0f && p->on_ground;
    int step = run ? ((int)(t * 14.0f)) % 2 : 0;
    int br = (!run && p->on_ground && p->state == PS_NORMAL && ((int)(t * 2.0f)) % 2) ? 1 : 0;

    if (p->state == PS_DASH) {                         /* rastro esticado */
        R(-3, 4, 3, 3, coat2);
        R(-5, 5, 2, 1, Fade(badge, 0.7f));
    }
    /* cordão do crachá, balança com a velocidade */
    int sway = (int)(p->vel.x * -0.01f * (float)f) + (((int)(t * 6.0f)) % 2);
    R(-1 + (sway > 0 ? 0 : -1), 5, 1, 2, badge);

    /* cabeça */
    R(1, br, 4, 4, skin);
    R(1, br, 4, 2, hair);
    R(0, 1 + br, 1, 2, hair);
    R(4, 2 + br, 1, 1, eye);
    /* tronco e crachá */
    R(0, 4, 6, 4, coat);
    R(0, 7, 6, 1, coat2);
    R(1, 5, 2, 1, badge);
    /* braços */
    if (p->state == PS_CLIMB) {
        R(4, 1, 2, 4, coat);
    } else if (!p->on_ground && p->vel.y > 0) {
        R(-1, 3, 1, 3, coat);
        R(6, 3, 1, 3, coat);
    }
    /* pernas */
    if (!p->on_ground || p->state == PS_CLIMB) {
        R(1, 8, 4, 2, pants);
    } else if (run) {
        if (step == 0) { R(1, 8, 2, 2, pants); R(3, 8, 2, 2, pants); }
        else           { R(0, 8, 2, 2, pants); R(4, 8, 2, 2, pants); }
    } else {
        R(1, 8, 2, 2, pants); R(3, 8, 2, 2, pants);
    }
#undef R
}

/* ------------------------------------------------------------------ */
/* Cena completa                                                       */
/* ------------------------------------------------------------------ */

void draw_vignette(void) {
    DrawRectangleGradientV(0, 0, VIEW_W, 30, (Color){ 0, 0, 0, 80 }, (Color){ 0, 0, 0, 0 });
    DrawRectangleGradientV(0, VIEW_H - 30, VIEW_W, 30, (Color){ 0, 0, 0, 0 }, (Color){ 0, 0, 0, 80 });
}

void draw_world(const Game *g) {
    const Level *lv = &g->level;
    const Palette *pal = &g->pal;
    const GameCamera *c = &g->cam;

    int jx = 0, jy = 0;
    if (c->shake > 0.3f) {
        int s = (int)c->shake;
        unsigned hv = gfx_hash((int)(g->time * 60.0f), 5);
        jx = (int)(hv % (unsigned)(2 * s + 1)) - s;
        jy = (int)((hv >> 8) % (unsigned)(2 * s + 1)) - s;
    }
    int ox = -ifl(c->pos.x) + jx, oy = -ifl(c->pos.y) + jy;

    draw_background(pal, c->pos.x, c->pos.y, g->time);

    int tx0 = ifl(c->pos.x / TILE_SIZE) - 1, tx1 = tx0 + VIEW_W / TILE_SIZE + 3;
    int ty0 = ifl(c->pos.y / TILE_SIZE) - 1, ty1 = ty0 + VIEW_H / TILE_SIZE + 3;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 > lv->width) tx1 = lv->width;
    if (ty1 > lv->height) ty1 = lv->height;

    for (int ty = ty0; ty < ty1; ty++) {
        for (int tx = tx0; tx < tx1; tx++) {
            unsigned char t = lv->tiles[ty][tx];
            if (t == TILE_EMPTY) continue;
            int px = tx * TILE_SIZE + ox, py = ty * TILE_SIZE + oy;
            switch (t) {
            case TILE_SOLID:  draw_solid(lv, pal, tx, ty, px, py); break;
            case TILE_SPIKE:  draw_spike(pal, px, py); break;
            case TILE_CRUMBLE: draw_crumble(lv, pal, tx, ty, px, py, g->time); break;
            case TILE_GHOST:  draw_ghost(pal, px, py, g->time, tx); break;
            default: break;
            }
        }
    }

    for (int i = 0; i < lv->ncheckpoints; i++)
        draw_checkpoint(pal, &lv->checkpoints[i], i == lv->active_checkpoint, ox, oy, g->time);
    draw_exit(g, ox, oy);
    for (int i = 0; i < lv->ninteractables; i++)
        draw_interactable(g, &lv->interactables[i], i == g->ev.near_interact && g->state == ST_PLAY, ox, oy);
    for (int i = 0; i < lv->ncrystals; i++) draw_crystal(pal, &lv->crystals[i], ox, oy, g->time);
    for (int i = 0; i < lv->ncollectibles; i++) draw_collectible(pal, &lv->collectibles[i], ox, oy, g->time);

    draw_hazards(g, ox, oy);

    draw_player(&g->player, pal, g->prog.character, g->time, ox, oy);
    fx_draw(&g->fx, ox, oy);
    draw_vignette();
}

void draw_menu_scene(const Game *g, float scroll) {
    const Palette *pal = &g->pal;
    draw_background(pal, scroll, 20.0f, g->time);
    /* chão silhueta */
    int gy = VIEW_H - 38;
    DrawRectangle(0, gy, VIEW_W, VIEW_H - gy, pal->tile);
    DrawRectangle(0, gy, VIEW_W, 2, pal->tile_top);
    for (int x = 0; x < VIEW_W; x += 8)
        if (gfx_hash(x / 8, 3) % 4 == 0) DrawRectangle(x, gy + 6, 3, 1, shade(pal->tile, 0.8f));
    Player p;
    player_reset(&p, (Vec2f){ 52.0f, (float)gy - PH_H });
    p.on_ground = true;
    p.vel.x = 0;
    p.dash_ready = true;
    draw_player(&p, pal, g->prog.character, g->time, 0, 0);
    fx_draw(&g->fx, 0, 0);
    draw_vignette();
}
