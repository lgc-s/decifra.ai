#include "ui/screens.h"

#include "narrative/decisions.h"
#include "render/gfx.h"

#include <stdio.h>

static void format_time(float secs, char *out, size_t n) {
    int s = (int)secs;
    snprintf(out, n, "%d:%02d:%02d", s / 3600, (s / 60) % 60, s % 60);
}

/* ------------------------------------------------------------------ */
/* Conclusão de ato                                                    */
/* ------------------------------------------------------------------ */

void actdone_update(Game *g, float dt) {
    g->actdone.t += dt;
    if (g->actdone.t < 0.7f || !g->in.confirm) return;
    audio_play(&g->audio, SFX_CONFIRM);
    if (g->level.act < NUM_ACTS) game_start_act(g, g->level.act + 1);
    else game_fade_to(g, FA_TO_STATE, ST_ENDING);
}

void actdone_draw(const Game *g) {
    const Progress *p = &g->prog;
    gfx_panel(60, 28, VIEW_W - 120, 160, UI_PANEL, UI_BORDER);
    gfx_text_center(TextFormat("ATO %d CONCLUÍDO", g->level.act), VIEW_W / 2, 38, 20, UI_GOOD);
    gfx_text_center(g->level.name, VIEW_W / 2, 62, 10, UI_DIM);

    char tm[16];
    format_time(p->play_time, tm, sizeof tm);
    int y = 84;
    DrawText("Confiança Pública", 76, y, 10, UI_TEXT);
    gfx_bar(190, y + 2, 90, 6, (float)p->trust / 100.0f, UI_ACCENT, (Color){ 30, 34, 60, 255 });
    DrawText(TextFormat("%d", p->trust), 286, y, 10, UI_ACCENT);
    DrawText(TextFormat("Chamados concluídos:  %d/%d", progress_calls_done_count(p), NUM_CALLS), 76, y + 16, 10, UI_TEXT);
    DrawText(TextFormat("Chips de dados:  %d/%d", progress_collected_count(p), NUM_COLLECTIBLES), 76, y + 30, 10, UI_TEXT);
    DrawText(TextFormat("Quedas: %u     Tempo: %s", p->deaths, tm), 76, y + 44, 10, UI_DIM);

    if (g->actdone.t > 0.7f)
        gfx_text_center(g->level.act < NUM_ACTS ? "Z / Enter: próximo ato" : "Z / Enter: ver o desfecho",
                        VIEW_W / 2, 164, 10, UI_ACCENT);
}

/* ------------------------------------------------------------------ */
/* Finais                                                              */
/* ------------------------------------------------------------------ */

void ending_update(Game *g, float dt) {
    g->ending.t += dt;
    if (g->ending.t > 9.0f && g->in.confirm) game_fade_to(g, FA_TO_STATE, ST_CREDITS);
}

void ending_draw(const Game *g) {
    const Progress *p = &g->prog;
    int id = p->ending < 0 ? decisions_compute_ending(p) : p->ending;
    const EndingInfo *e = ending_info(id);
    if (!e) return;
    float t = g->ending.t;

    DrawRectangle(0, 0, VIEW_W, VIEW_H, (Color){ 4, 6, 16, 130 });
    float a = t < 1.0f ? t : 1.0f;
    gfx_text_center(TextFormat("FINAL %d DE %d", id + 1, NUM_ENDINGS), VIEW_W / 2, 24, 10, Fade(UI_DIM, a));
    gfx_text_center(e->title, VIEW_W / 2, 40, 20, Fade(UI_TEXT, a));

        for (int i = 0; i < 3; i++) {
        float st = 1.6f + (float)i * 1.8f;
        if (t < st) break;
        float la = (t - st) > 1.0f ? 1.0f : (t - st);
        gfx_text_wrap(e->lines[i], 40, 68 + i * 27, VIEW_W - 80, 10, Fade(UI_TEXT, la), -1);
    }
    
    if (t > 6.6f) {                                         /* a frase que fica na cabeça do jogador */
        float pa = (t - 6.6f) > 1.0f ? 1.0f : (t - 6.6f);
        gfx_text_center(e->punch, VIEW_W / 2, 150, 20, Fade(id == ENDING_CLARITY ? UI_GOOD : (id == ENDING_REPAIR ? UI_WARN : UI_BAD), pa));
    }

    if (t > 8.0f) {
        char tm[16];
        format_time(p->play_time, tm, sizeof tm);
        gfx_text_center(TextFormat("Confiança Pública: %d   Decisões acertadas: %d/%d   Viés %s",
                                   p->trust, decisions_good_count(p), NUM_CALLS, p->bias_solved ? "corrigido" : "não corrigido"),
                        VIEW_W / 2, 172, 10, UI_WARN);
        gfx_text_center(TextFormat("Tempo: %s   Quedas: %u   Chips: %d/%d", tm, p->deaths, progress_collected_count(p), NUM_COLLECTIBLES),
                        VIEW_W / 2, 185, 10, UI_DIM);
    }

    if (t > 9.0f && ((int)(t * 2.0f)) % 2 == 0)
        gfx_text_center("Z / Enter: continuar", VIEW_W / 2, 200, 10, UI_ACCENT);
}
