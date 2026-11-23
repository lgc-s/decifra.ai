#include "ui/screens.h"

#include "render/character.h"
#include "render/gfx.h"

/* Escolha do personagem, ao começar um novo jogo. O progresso só é apagado ao confirmar. */

void character_update(Game *g, float dt) {
    (void)dt;
    if (g->in.left_p)  { g->char_cursor = (g->char_cursor + NUM_CHARACTERS - 1) % NUM_CHARACTERS; audio_play(&g->audio, SFX_SELECT); }
    if (g->in.right_p) { g->char_cursor = (g->char_cursor + 1) % NUM_CHARACTERS;                  audio_play(&g->audio, SFX_SELECT); }
    if (g->in.back) { game_fade_to(g, FA_TO_MENU, 0); return; }
    if (g->in.confirm) {
        audio_play(&g->audio, SFX_CONFIRM);
        game_new_game(g, g->char_cursor);
    }
}

void character_draw(const Game *g) {
    ui_dim(150);
    gfx_text_center("ESCOLHA SEU PERSONAGEM", VIEW_W / 2, 12, 20, UI_ACCENT);
    gfx_text_center("Quem vai auditar a NÓVA?", VIEW_W / 2, 36, 10, UI_DIM);

    const int cw = 84, gap = 8, x0 = (VIEW_W - (NUM_CHARACTERS * cw + (NUM_CHARACTERS - 1) * gap)) / 2, y0 = 54, ch = 120;
    for (int i = 0; i < NUM_CHARACTERS; i++) {
        bool sel = i == g->char_cursor;
        int x = x0 + i * (cw + gap);
        gfx_panel(x, y0, cw, ch, sel ? (Color){ 26, 34, 70, 245 } : UI_PANEL, sel ? UI_ACCENT : (Color){ 70, 80, 130, 255 });
        int bob = sel ? (((int)(g->time * 4.0f)) % 2) : 0;           /* só o selecionado se mexe */
        character_draw_preview(x + cw / 2, y0 + 22, i, 5, bob);
        gfx_text_center(character_style(i)->name, x + cw / 2, y0 + ch - 24, 20, sel ? UI_TEXT : UI_DIM);
        if (!sel) DrawRectangle(x + 1, y0 + 1, cw - 2, ch - 2, (Color){ 0, 0, 0, 90 });
        else DrawRectangleLines(x - 1, y0 - 1, cw + 2, ch + 2, UI_ACCENT);
    }
    gfx_text_center("Esq/Dir: escolher     Z / Enter: confirmar     X / Esc: voltar", VIEW_W / 2, VIEW_H - 26, 10, UI_DIM);
}