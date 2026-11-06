#include "ui/screens.h"

#include "render/gfx.h"

#include <string.h>

#define CHARS_PER_SEC 75.0f

void dialogue_update(Game *g, float dt) {
    if (!g->dlg.d) { game_back_to_play(g); return; }
    const DialogueLine *ln = &g->dlg.d->lines[g->dlg.line];
    int total = (int)strlen(ln->text);

    int before = (int)g->dlg.shown;
    g->dlg.shown += dt * CHARS_PER_SEC;
    if ((int)g->dlg.shown / 4 != before / 4 && (int)g->dlg.shown < total) audio_play(&g->audio, SFX_TALK);

    if (g->in.back) { game_close_dialogue(g); return; }          /* pula tudo */
    if (!(g->in.confirm || g->in.interact)) return;

    if ((int)g->dlg.shown < total) {                              /* completa a fala atual */
        g->dlg.shown = (float)total;
        return;
    }
    audio_play(&g->audio, SFX_SELECT);
    if (++g->dlg.line >= g->dlg.d->nlines) game_close_dialogue(g);
    else g->dlg.shown = 0;
}

void dialogue_draw(const Game *g) {
    if (!g->dlg.d) return;
    const DialogueLine *ln = &g->dlg.d->lines[g->dlg.line];
    int x = 10, w = VIEW_W - 20, h = 66, y = VIEW_H - h - 8;

    gfx_panel(x, y, w, h, UI_PANEL, UI_BORDER);
    int tw = MeasureText(ln->speaker, 10) + 14;
    gfx_panel(x + 8, y - 8, tw, 14, (Color){ 40, 56, 110, 255 }, UI_BORDER);
    DrawText(ln->speaker, x + 15, y - 5, 10, UI_ACCENT);

    gfx_text_wrap(ln->text, x + 10, y + 12, w - 22, 10, UI_TEXT, (int)g->dlg.shown);

    int total = (int)strlen(ln->text);
    if ((int)g->dlg.shown >= total && ((int)(g->time * 3.0f)) % 2 == 0) {
        DrawTriangle((Vector2){ (float)(x + w - 18), (float)(y + h - 12) }, (Vector2){ (float)(x + w - 14), (float)(y + h - 6) },
                     (Vector2){ (float)(x + w - 10), (float)(y + h - 12) }, UI_ACCENT);
    }
    gfx_text_right(TextFormat("%d/%d", g->dlg.line + 1, g->dlg.d->nlines), x + w - 6, y + 3, 10, UI_DIM);
}
