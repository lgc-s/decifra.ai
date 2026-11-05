#include "ui/screens.h"

#include "narrative/missions.h"
#include "render/gfx.h"

void call_update(Game *g, float dt) {
    (void)dt;
    const Call *c = call_get(g->call.id);
    if (!c) { game_back_to_play(g); return; }

    if (g->call.phase == 0) {
        if (g->in.up_p)   { g->call.cursor = (g->call.cursor + c->nopts - 1) % c->nopts; audio_play(&g->audio, SFX_SELECT); }
        if (g->in.down_p) { g->call.cursor = (g->call.cursor + 1) % c->nopts;            audio_play(&g->audio, SFX_SELECT); }
        if (g->in.back) { game_back_to_play(g); return; }          /* adia o chamado */
        if (g->in.confirm) {
            int nominal = c->opts[g->call.cursor].trust;
            call_apply(&g->prog, g->call.id, g->call.cursor);
            g->call.delta = nominal;
            g->call.phase = 1;
            audio_play(&g->audio, nominal >= 0 ? SFX_CORRECT : SFX_WRONG);
            game_save(g);
        }
    } else if (g->in.confirm || g->in.back || g->in.interact) {
        audio_play(&g->audio, SFX_SELECT);
        game_back_to_play(g);
    }
}

void call_draw(const Game *g) {
    const Call *c = call_get(g->call.id);
    if (!c) return;
    const int x = 12, w = VIEW_W - 24;
    gfx_panel(6, 6, VIEW_W - 12, VIEW_H - 12, UI_PANEL, UI_BORDER);
    DrawRectangle(7, 7, VIEW_W - 14, 14, (Color){ 30, 44, 92, 255 });
    DrawText(TextFormat("CHAMADO  -  %s", c->region), x, 10, 10, UI_ACCENT);
    gfx_text_right(TextFormat("Confiança %d", g->prog.trust), VIEW_W - 12, 10, 10, UI_TEXT);

    int y = 28;
    DrawText(c->title, x, y, 20, UI_TEXT);
    y += 26;

    if (g->call.phase == 0) {
        y = gfx_text_wrap(c->context, x, y, w, 10, UI_DIM, -1);
        y = gfx_text_wrap(c->problem, x, y + 2, w, 10, UI_TEXT, -1) + 6;
        for (int i = 0; i < c->nopts; i++) {
            bool sel = i == g->call.cursor;
            int h = gfx_text_wrap_height(c->opts[i].label, w - 24, 10);
            if (sel) DrawRectangle(x - 2, y - 2, w + 4, h + 2, (Color){ 40, 56, 110, 200 });
            DrawText(TextFormat("%c", 'A' + i), x + 2, y, 10, sel ? UI_ACCENT : UI_DIM);
            gfx_text_wrap(c->opts[i].label, x + 18, y, w - 24, 10, sel ? UI_TEXT : (Color){ 190, 198, 224, 255 }, -1);
            y += h + 3;
        }
        gfx_text_center("Cima/Baixo: escolher     Z / Enter: decidir     X / Esc: adiar", VIEW_W / 2, VIEW_H - 20, 10, UI_DIM);
    } else {
        const CallOption *o = &c->opts[g->call.cursor];
        DrawText("Sua decisão:", x, y, 10, UI_DIM);
        y = gfx_text_wrap(o->label, x, y + 10, w, 10, UI_ACCENT, -1) + 4;
        y = gfx_text_wrap(o->consequence, x, y, w, 10, UI_TEXT, -1) + 4;
        Color dc = g->call.delta >= 0 ? UI_GOOD : UI_BAD;
        DrawText(TextFormat("Confiança Pública %s%d", g->call.delta >= 0 ? "+" : "", g->call.delta), x, y, 10, dc);
        y += 16;
        gfx_panel(x - 2, y - 2, w + 4, VIEW_H - y - 28, (Color){ 22, 30, 62, 255 }, (Color){ 70, 90, 160, 255 });
        DrawText("O QUE APRENDER", x + 4, y + 2, 10, UI_WARN);
        gfx_text_wrap(c->explanation, x + 4, y + 14, w - 8, 10, UI_TEXT, -1);
        gfx_text_center("Z / Enter: continuar", VIEW_W / 2, VIEW_H - 20, 10, UI_DIM);
    }
}
