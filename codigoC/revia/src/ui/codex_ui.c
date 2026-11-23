#include "ui/screens.h"

#include "education/codex.h"
#include "render/gfx.h"

void codex_update(Game *g, float dt) {
    (void)dt;
    if (g->in.up_p)   { g->codex_cursor = (g->codex_cursor + NUM_COLLECTIBLES - 1) % NUM_COLLECTIBLES; audio_play(&g->audio, SFX_SELECT); }
    if (g->in.down_p) { g->codex_cursor = (g->codex_cursor + 1) % NUM_COLLECTIBLES;                    audio_play(&g->audio, SFX_SELECT); }
    if (g->in.back || g->in.confirm) game_close_sub(g);
}

void codex_draw(const Game *g) {
    gfx_panel(8, 8, VIEW_W - 16, VIEW_H - 16, UI_PANEL, UI_BORDER);
    DrawRectangle(9, 9, VIEW_W - 18, 14, (Color){ 30, 44, 92, 255 });
    DrawText("ENCICLOPÉDIA  -  MANUAL DO DECIFRADOR", 16, 12, 10, UI_ACCENT);
    gfx_text_right(TextFormat("%d/%d chips", progress_collected_count(&g->prog), NUM_COLLECTIBLES), VIEW_W - 16, 12, 10, UI_TEXT);

    for (int i = 0; i < NUM_COLLECTIBLES; i++) {
        bool have = (g->prog.collected >> i) & 1u;
        bool sel = i == g->codex_cursor;
        int y = 30 + i * 14;
        if (sel) DrawRectangle(12, y - 2, 156, 13, (Color){ 40, 56, 110, 200 });
        const CodexEntry *e = codex_get(i);
        DrawText(TextFormat("%d", i / 3 + 1), 16, y, 10, UI_DIM);
        DrawText(have ? e->title : "???", 28, y, 10, have ? (sel ? UI_ACCENT : UI_TEXT) : (Color){ 90, 98, 130, 255 });
    }

    int x = 180, w = VIEW_W - x - 16;
    gfx_panel(x - 4, 28, w + 8, VIEW_H - 62, (Color){ 20, 26, 58, 255 }, (Color){ 70, 90, 160, 255 });
    const CodexEntry *e = codex_get(g->codex_cursor);
    bool have = (g->prog.collected >> g->codex_cursor) & 1u;
    if (have && e) {
        int tsize = MeasureText(e->title, 20) <= w - 4 ? 20 : 10;       /* título longo usa fonte menor */
        DrawText(e->title, x + 2, 34, tsize, UI_TEXT);
        gfx_text_wrap(e->text, x + 2, tsize == 20 ? 62 : 52, w - 4, 10, UI_TEXT, -1);
    } else {
        DrawText("Conceito bloqueado", x + 2, 34, 10, UI_DIM);
        gfx_text_wrap(TextFormat("Encontre o chip de dados dourado no Ato %d para desbloquear este conceito.", g->codex_cursor / 3 + 1),
                      x + 2, 50, w - 4, 10, UI_DIM, -1);
    }
    gfx_text_center("Cima/Baixo: navegar     X / Esc: fechar", VIEW_W / 2, VIEW_H - 22, 10, UI_DIM);
}
