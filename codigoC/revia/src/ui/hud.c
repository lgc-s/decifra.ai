#include "ui/screens.h"

#include "render/gfx.h"
#include "render/world_draw.h"

#include <stdio.h>

void ui_dim(int alpha) {
    DrawRectangle(0, 0, VIEW_W, VIEW_H, (Color){ 4, 6, 16, (unsigned char)alpha });
}

static Color trust_color(int t) {
    if (t < 25) return (Color){ 255, 90, 100, 255 };
    if (t < 50) return (Color){ 255, 170, 80, 255 };
    if (t < 75) return (Color){ 250, 226, 100, 255 };
    return (Color){ 110, 230, 150, 255 };
}

/* Texto curto de objetivos do ato atual. */
static void objective_text(const Game *g, char *out, size_t n) {
    const Progress *p = &g->prog;
    bool quiz = (p->quiz_done >> (g->level.act - 1)) & 1u;
    switch (g->level.act) {
    case 1: snprintf(out, n, "Objetivo: avaliação %s", quiz ? "ok" : "pendente"); break;
    case 2: {
        int c = 0;
        for (int i = 0; i < 4; i++) c += (p->calls_done >> i) & 1u;
        snprintf(out, n, "Chamados %d/4  Avaliação %s", c, quiz ? "ok" : "pendente");
        break; }
    case 3: {
        int ev = (p->evidence & 1) + ((p->evidence >> 1) & 1) + ((p->evidence >> 2) & 1);
        snprintf(out, n, "Evidências %d/3  Simulação %s  Chamado %s  Avaliação %s", ev,
                 p->bias_played ? "ok" : "--", ((p->calls_done >> 4) & 1u) ? "ok" : "--", quiz ? "ok" : "--");
        break; }
    default: {
        int c = ((p->calls_done >> 5) & 1u) + ((p->calls_done >> 6) & 1u);
        snprintf(out, n, "Decisões finais %d/2  Avaliação %s", c, quiz ? "ok" : "pendente");
        break; }
    }
}

void hud_draw(const Game *g) {
    const Progress *p = &g->prog;

    /* Confiança Pública */
    gfx_panel(6, 6, 124, 28, (Color){ 10, 12, 28, 200 }, (Color){ 70, 84, 140, 255 });
    DrawText("CONFIANÇA PÚBLICA", 11, 8, 10, UI_DIM);
    gfx_bar(11, 22, 84, 6, (float)p->trust / 100.0f, trust_color(p->trust), (Color){ 30, 34, 60, 255 });
    DrawText(TextFormat("%d", p->trust), 100, 19, 10, trust_color(p->trust));

    /* colecionáveis */
    char buf[160];
    int n = progress_collected_count(p);
    snprintf(buf, sizeof buf, "%d/%d", n, NUM_COLLECTIBLES);
    gfx_panel(VIEW_W - 58, 6, 52, 16, (Color){ 10, 12, 28, 200 }, (Color){ 70, 84, 140, 255 });
    DrawRectangle(VIEW_W - 52, 11, 6, 6, (Color){ 255, 226, 120, 255 });
    DrawRectangle(VIEW_W - 51, 12, 4, 4, (Color){ 120, 80, 30, 255 });
    DrawText(buf, VIEW_W - 42, 9, 10, UI_TEXT);

    objective_text(g, buf, sizeof buf);
    DrawText(buf, 10, 38, 10, Fade(UI_TEXT, 0.8f));

    /* faixa de título da fase */
    if (g->banner_t > 0.0f && g->state == ST_PLAY) {
        float a = g->banner_t > 3.4f ? (4.0f - g->banner_t) / 0.6f : (g->banner_t < 0.8f ? g->banner_t / 0.8f : 1.0f);
        const char *act = act_title(g->level.act);
        int w = MeasureText(g->banner, 20);
        DrawRectangle(0, 62, VIEW_W, 40, (Color){ 6, 8, 20, (unsigned char)(150.0f * a) });
        gfx_text_center(act, VIEW_W / 2, 66, 10, Fade(UI_ACCENT, a));
        if (w > VIEW_W - 20) gfx_text_center(g->banner, VIEW_W / 2, 80, 10, Fade(UI_TEXT, a));
        else gfx_text_center(g->banner, VIEW_W / 2, 79, 20, Fade(UI_TEXT, a));
    }

    /* dica de interação */
    if (g->state == ST_PLAY && g->ev.near_interact >= 0 && g->player.state != PS_DEAD) {
        const Interactable *it = &g->level.interactables[g->ev.near_interact];
        const char *tip = it->kind == INT_CALL ? "Atender chamado" :
                          it->kind == INT_QUIZ ? "Fazer avaliação" :
                          it->kind == INT_BIAS ? "Abrir simulação" : "Interagir";
        gfx_text_center(TextFormat("E - %s", tip), VIEW_W / 2, VIEW_H - 26, 10, UI_TEXT);
    }
}

void hud_draw_toast(const Game *g) {
    if (g->toast_t <= 0.0f || !g->toast[0]) return;
    float a = g->toast_t < 0.5f ? g->toast_t / 0.5f : 1.0f;
    int w = MeasureText(g->toast, 10) + 16;
    if (w > VIEW_W - 16) w = VIEW_W - 16;
    int x = (VIEW_W - w) / 2, y = VIEW_H - 22;
    gfx_panel(x, y, w, 15, (Color){ 10, 12, 28, (unsigned char)(220.0f * a) }, Fade(UI_BORDER, a));
    DrawText(g->toast, x + 8, y + 3, 10, Fade(UI_TEXT, a));
}
