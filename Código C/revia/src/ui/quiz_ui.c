#include "ui/screens.h"

#include "render/gfx.h"

/* phase 0 pergunta | 1 feedback | 2 sessão concluída */

static void complete_session(Game *g) {
    int s = g->quiz.run.session;
    bool first = !((g->prog.quiz_done >> s) & 1u);
    g->prog.quiz_done |= 1u << s;
    g->prog.quiz_correct_mask[s] = g->quiz.run.correct_mask;
    if (first) progress_add_trust(&g->prog, 4);
    g->quiz.phase = 2;
    audio_play(&g->audio, SFX_COLLECT);
    game_save(g);
}

void quiz_update(Game *g, float dt) {
    QuizRun *r = &g->quiz.run;
    g->quiz.t += dt;

    if (g->quiz.phase == 0) {
        if (g->in.up_p)   { g->quiz.cursor = (g->quiz.cursor + 3) % 4; audio_play(&g->audio, SFX_SELECT); }
        if (g->in.down_p) { g->quiz.cursor = (g->quiz.cursor + 1) % 4; audio_play(&g->audio, SFX_SELECT); }
        if (g->in.back) { game_back_to_play(g); return; }
        if (g->in.confirm) {
            bool ok = quiz_answer(r, g->quiz.cursor);
            audio_play(&g->audio, ok ? SFX_CORRECT : SFX_WRONG);
            if (!ok && g->prog.quiz_attempts[r->session] < 255) g->prog.quiz_attempts[r->session]++;
            g->quiz.phase = 1;
        }
    } else if (g->quiz.phase == 1) {
        if (g->in.back) { game_back_to_play(g); return; }
        if (!g->in.confirm) return;
        audio_play(&g->audio, SFX_SELECT);
        if (r->failed) {                         /* regra: todas as 10 certas; recomeça com variações */
            game_start_quiz(g, r->session);
            return;
        }
        quiz_next(r);
        if (r->finished) complete_session(g);
        else { g->quiz.phase = 0; g->quiz.cursor = 0; }
    } else if (g->in.confirm || g->in.back) {
        game_back_to_play(g);
    }
}

void quiz_draw(const Game *g) {
    const QuizRun *r = &g->quiz.run;
    const int x = 12, w = VIEW_W - 24;
    gfx_panel(6, 6, VIEW_W - 12, VIEW_H - 12, UI_PANEL, UI_BORDER);
    DrawRectangle(7, 7, VIEW_W - 14, 14, (Color){ 30, 44, 92, 255 });
    DrawText(quiz_session_title(r->session), x, 10, 10, UI_ACCENT);

    /* progresso: um quadrado por questão */
    for (int i = 0; i < QUIZ_LEN; i++) {
        Color c = (Color){ 50, 58, 100, 255 };
        if ((r->correct_mask >> i) & 1u) c = UI_GOOD;
        else if (i == r->cur && r->failed) c = UI_BAD;
        else if (i == r->cur) c = UI_WARN;
        DrawRectangle(VIEW_W - 12 - (QUIZ_LEN - i) * 9, 11, 7, 7, c);
    }

    if (g->quiz.phase == 2) {
        gfx_text_center("AVALIAÇÃO CONCLUÍDA", VIEW_W / 2, 66, 20, UI_GOOD);
        gfx_text_center("10 de 10 respostas corretas.", VIEW_W / 2, 96, 10, UI_TEXT);
        gfx_text_center("Confiança Pública +4 (primeira conclusão)", VIEW_W / 2, 112, 10, UI_DIM);
        gfx_text_center("Z / Enter: voltar ao jogo", VIEW_W / 2, VIEW_H - 24, 10, UI_DIM);
        return;
    }

    const QuizQuestion *q = quiz_current(r);
    if (!q) return;
    DrawText(TextFormat("Pergunta %d de %d", r->cur + 1, QUIZ_LEN), x, 28, 10, UI_DIM);
    int y = gfx_text_wrap(q->question, x, 40, w, 10, UI_TEXT, -1) + 6;

    int correct = quiz_correct_display_index(r);
    for (int i = 0; i < 4; i++) {
        bool sel = i == g->quiz.cursor;
        const char *txt = quiz_option_text(r, i);
        int h = gfx_text_wrap_height(txt, w - 24, 10);
        Color bg = sel ? (Color){ 40, 56, 110, 200 } : (Color){ 0, 0, 0, 0 };
        Color fg = sel ? UI_TEXT : (Color){ 190, 198, 224, 255 };
        if (g->quiz.phase == 1) {
            if (i == correct) { bg = (Color){ 26, 90, 56, 220 }; fg = UI_TEXT; }
            else if (sel) { bg = (Color){ 110, 30, 44, 220 }; fg = UI_TEXT; }
            else fg = UI_DIM;
        }
        DrawRectangle(x - 2, y - 2, w + 4, h + 2, bg);
        DrawText(TextFormat("%c", 'A' + i), x + 2, y, 10, sel ? UI_ACCENT : UI_DIM);
        gfx_text_wrap(txt, x + 18, y, w - 24, 10, fg, -1);
        y += h + 3;
    }

    if (g->quiz.phase == 0) {
        gfx_text_center("Cima/Baixo: escolher     Z / Enter: responder     X / Esc: sair", VIEW_W / 2, VIEW_H - 20, 10, UI_DIM);
    } else {
        Color c = r->last_correct ? UI_GOOD : UI_BAD;
        DrawText(r->last_correct ? "Correto!" : "Errou. A sessão recomeça com perguntas variadas.", x, y + 2, 10, c);
        gfx_text_wrap(q->explanation, x, y + 14, w, 10, UI_DIM, -1);
        gfx_text_center(r->failed ? "Z / Enter: recomeçar a sessão" : "Z / Enter: próxima", VIEW_W / 2, VIEW_H - 20, 10, UI_DIM);
    }
}
