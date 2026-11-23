#include "ui/screens.h"

#include "render/gfx.h"

/* phase 0 pergunta | 1 feedback | 2 sessão concluída
 *
 * Regras: errar não reinicia a avaliação. A questão conta como errada e o jogador segue até a última.
 * No fim, a Confiança Pública recebe a soma (acertos * QUIZ_TRUST_RIGHT - erros * QUIZ_TRUST_WRONG)
 * e a avaliação é dada como concluída. */

static void complete_session(Game *g) {
    const QuizRun *r = &g->quiz.run;
    int s = r->session;
    bool first = !((g->prog.quiz_done >> s) & 1u);
    int before = g->prog.trust;
    g->prog.quiz_done |= 1u << s;
    g->prog.quiz_correct_mask[s] = r->correct_mask;
    if (first) progress_add_trust(&g->prog, quiz_trust_delta(r));
    g->quiz.first = first;
    g->quiz.delta = g->prog.trust - before;          /* variação real (a confiança fica entre 0 e 100) */
    g->quiz.phase = 2;
    audio_play(&g->audio, g->quiz.delta < 0 ? SFX_WRONG : SFX_COLLECT);
    game_save(g);
}

void quiz_update(Game *g, float dt) {
    QuizRun *r = &g->quiz.run;
    g->quiz.t += dt;

    if (g->quiz.phase == 0) {
        if (g->in.up_p)   { g->quiz.cursor = (g->quiz.cursor + 3) % 4; audio_play(&g->audio, SFX_SELECT); }
        if (g->in.down_p) { g->quiz.cursor = (g->quiz.cursor + 1) % 4; audio_play(&g->audio, SFX_SELECT); }
        /* só dá para sair antes da 1ª resposta; depois disso a avaliação precisa terminar */
        if (g->in.back && r->cur == 0) { game_back_to_play(g); return; }
        if (g->in.confirm) {
            bool ok = quiz_answer(r, g->quiz.cursor);
            audio_play(&g->audio, ok ? SFX_CORRECT : SFX_WRONG);
            if (!ok && g->prog.quiz_attempts[r->session] < 255) g->prog.quiz_attempts[r->session]++;
            g->quiz.phase = 1;
        }
    } else if (g->quiz.phase == 1) {
        if (!g->in.confirm) return;
        audio_play(&g->audio, SFX_SELECT);
        quiz_next(r);                            /* acertou ou errou: segue para a próxima */
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

    /* progresso: um quadrado por questão (verde acertou, vermelho errou, amarelo atual) */
    for (int i = 0; i < QUIZ_LEN; i++) {
        Color c = (Color){ 50, 58, 100, 255 };
        if ((r->correct_mask >> i) & 1u) c = UI_GOOD;
        else if ((r->wrong_mask >> i) & 1u) c = UI_BAD;
        else if (i == r->cur && g->quiz.phase != 2) c = UI_WARN;
        DrawRectangle(VIEW_W - 12 - (QUIZ_LEN - i) * 9, 11, 7, 7, c);
    }
    if (g->quiz.phase != 2)
        gfx_text_right(TextFormat("Acertos %d  Erros %d", r->correct_count, r->wrong_count),
                       VIEW_W - 12 - QUIZ_LEN * 9 - 8, 10, 10, UI_DIM);

    if (g->quiz.phase == 2) {
        int d = g->quiz.delta;
        Color dc = d > 0 ? UI_GOOD : (d < 0 ? UI_BAD : UI_WARN);
        gfx_text_center("AVALIAÇÃO CONCLUÍDA", VIEW_W / 2, 52, 20, UI_GOOD);
        gfx_text_center(TextFormat("Acertos: %d   Erros: %d   (de %d)", r->correct_count, r->wrong_count, QUIZ_LEN),
                        VIEW_W / 2, 86, 10, UI_TEXT);
        gfx_text_center(TextFormat("%d x +%d  e  %d x -%d  =  %+d", r->correct_count, QUIZ_TRUST_RIGHT,
                                   r->wrong_count, QUIZ_TRUST_WRONG, quiz_trust_delta(r)),
                        VIEW_W / 2, 102, 10, UI_DIM);
        if (g->quiz.first)
            gfx_text_center(TextFormat("Confiança Pública %+d  (agora %d)", d, g->prog.trust), VIEW_W / 2, 124, 10, dc);
        else
            gfx_text_center("Avaliação já feita antes: sem efeito na confiança", VIEW_W / 2, 124, 10, UI_DIM);
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
        gfx_text_center(r->cur == 0 ? "Cima/Baixo: escolher     Z / Enter: responder     X / Esc: sair"
                                    : "Cima/Baixo: escolher     Z / Enter: responder",
                        VIEW_W / 2, VIEW_H - 20, 10, UI_DIM);
    } else {
        if (r->last_correct)
            DrawText(TextFormat("Correto!  Confiança Pública +%d", QUIZ_TRUST_RIGHT), x, y + 2, 10, UI_GOOD);
        else
            DrawText(TextFormat("Errou.  Confiança Pública -%d  (conta como erro)", QUIZ_TRUST_WRONG), x, y + 2, 10, UI_BAD);
        gfx_text_wrap(q->explanation, x, y + 14, w, 10, UI_DIM, -1);
        gfx_text_center(r->cur + 1 >= QUIZ_LEN ? "Z / Enter: ver o resultado" : "Z / Enter: próxima",
                        VIEW_W / 2, VIEW_H - 20, 10, UI_DIM);
    }
}