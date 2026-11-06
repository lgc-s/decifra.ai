#include "ui/screens.h"

#include "render/gfx.h"

#include <math.h>
#include <stdio.h>

enum { B_NA, B_NB, B_GROUP, B_PROXY, B_LABELS, B_RUN, B_COUNT };

static void invalidate(Game *g) { g->bias.g.result.ran = false; }

void bias_update(Game *g, float dt) {
    BiasGame *b = &g->bias.g;
    if (g->bias.anim < 1.0f) g->bias.anim += dt * 1.6f;
    if (g->in.up_p)   { g->bias.cursor = (g->bias.cursor + B_COUNT - 1) % B_COUNT; audio_play(&g->audio, SFX_SELECT); }
    if (g->in.down_p) { g->bias.cursor = (g->bias.cursor + 1) % B_COUNT;           audio_play(&g->audio, SFX_SELECT); }
    if (g->in.back) { game_back_to_play(g); return; }

    int dir = (g->in.right_p ? 1 : 0) - (g->in.left_p ? 1 : 0);
    switch (g->bias.cursor) {
    case B_NA:
    case B_NB: {
        if (!dir) break;
        int *n = g->bias.cursor == B_NA ? &b->n_a : &b->n_b;
        int v = *n + dir * 2;
        *n = v < 0 ? 0 : (v > BIAS_POOL ? BIAS_POOL : v);
        invalidate(g);
        audio_play(&g->audio, SFX_SELECT);
        break; }
    case B_GROUP:  if (g->in.confirm || dir) { b->use_group = !b->use_group;  invalidate(g); audio_play(&g->audio, SFX_SELECT); } break;
    case B_PROXY:  if (g->in.confirm || dir) { b->use_proxy = !b->use_proxy;  invalidate(g); audio_play(&g->audio, SFX_SELECT); } break;
    case B_LABELS: if (g->in.confirm || dir) { b->fix_labels = !b->fix_labels; invalidate(g); audio_play(&g->audio, SFX_SELECT); } break;
    case B_RUN:
        if (!g->in.confirm) break;
        bias_run(b);
        g->bias.anim = 0.0f;
        g->prog.bias_played = 1;
        if (b->result.solved && !g->prog.bias_solved) {
            g->prog.bias_solved = 1;
            progress_add_trust(&g->prog, 10);
            game_toast(g, "Viés corrigido! Confiança Pública +10");
        }
        audio_play(&g->audio, b->result.solved ? SFX_CORRECT : SFX_WRONG);
        game_save(g);
        break;
    }
}

static const char *explain(const BiasGame *b) {
    const BiasResult *r = &b->result;
    if (r->solved) {
        if (b->fix_labels)
            return "Justo! Corrigir os rótulos históricos atacou a causa: o histórico tratava o Bairro Sul com mais rigor.";
        return "Dentro do limite. Sem o bairro, o modelo não separa os grupos, mas o viés ainda está nos dados antigos.";
    }
    if (b->use_proxy && !b->use_group)
        return "O CEP funciona como proxy do bairro: mesmo sem a coluna, o viés voltou.";
    if (b->use_group || b->use_proxy)
        return "O modelo usa o bairro e aprendeu o rigor do histórico: mais negados no Sul, com as mesmas qualificações.";
    if (r->accuracy < BIAS_MIN_ACCURACY)
        return "Poucos exemplos: o modelo errou demais. Dados insuficientes também prejudicam.";
    return "Ainda há diferença entre os bairros. Revise as amostras e os rótulos.";
}

static void draw_dots(const BiasGame *b, bool group_b, int x, int y) {
    int n = group_b ? b->n_b : b->n_a;
    for (int i = 0; i < BIAS_POOL; i++) {
        int dx = x + i * 8;
        if (i < n) DrawRectangle(dx, y, 6, 6, bias_label(b, group_b, i) ? (Color){ 90, 220, 140, 255 } : (Color){ 240, 90, 100, 255 });
        else DrawRectangleLines(dx, y, 6, 6, (Color){ 60, 68, 110, 255 });
    }
}

void bias_draw(const Game *g) {
    const BiasGame *b = &g->bias.g;
    const BiasResult *r = &b->result;
    gfx_panel(4, 4, VIEW_W - 8, VIEW_H - 8, UI_PANEL, UI_BORDER);
    DrawRectangle(5, 5, VIEW_W - 10, 14, (Color){ 30, 44, 92, 255 });
    DrawText("SIMULAÇÃO DE CRÉDITO  -  NÓVA", 12, 7, 10, UI_ACCENT);
    gfx_text_right(g->prog.bias_solved ? "viés corrigido" : "viés ativo", VIEW_W - 12, 7, 10, g->prog.bias_solved ? UI_GOOD : UI_BAD);

    /* conjunto de treino: um ponto por pedido antigo */
    DrawText("Bairro Norte (A)", 12, 22, 10, UI_TEXT);
    DrawText("verde: aprovado   vermelho: negado", 120, 22, 10, UI_DIM);
    draw_dots(b, false, 12, 35);
    DrawText("Bairro Sul (B)", 12, 45, 10, UI_TEXT);
    draw_dots(b, true, 12, 58);
    DrawText(TextFormat("Aprovação no treino:  A %.0f%%   B %.0f%%",
                        (double)bias_train_rate(b, false), (double)bias_train_rate(b, true)), 12, 68, 10, UI_WARN);

    /* controles */
    char row[B_COUNT][64];
    snprintf(row[B_NA], sizeof row[0], "Amostras do Norte (A):  < %d >", b->n_a);
    snprintf(row[B_NB], sizeof row[0], "Amostras do Sul (B):  < %d >", b->n_b);
    snprintf(row[B_GROUP], sizeof row[0], "Usar a coluna Bairro:  %s", b->use_group ? "SIM" : "NÃO");
    snprintf(row[B_PROXY], sizeof row[0], "Usar o CEP:  %s", b->use_proxy ? "SIM" : "NÃO");
    snprintf(row[B_LABELS], sizeof row[0], "Corrigir rótulos históricos:  %s", b->fix_labels ? "SIM" : "NÃO");
    snprintf(row[B_RUN], sizeof row[0], "[ EXECUTAR SIMULAÇÃO ]");
    for (int i = 0; i < B_COUNT; i++) {
        bool sel = i == g->bias.cursor;
        int y = 83 + i * 12;
        if (sel) DrawRectangle(10, y - 1, 192, 12, (Color){ 40, 56, 110, 200 });
        DrawText(row[i], 14, y, 10, sel ? UI_ACCENT : UI_TEXT);
    }

    /* resultados */
    gfx_panel(208, 80, VIEW_W - 220, 72, (Color){ 20, 26, 58, 255 }, (Color){ 70, 90, 160, 255 });
    if (!r->ran) {
        gfx_text_wrap("Ajuste os dados e execute a simulação. No teste, os dois bairros têm as mesmas qualificações.",
                      214, 85, VIEW_W - 232, 10, UI_DIM, -1);
    } else {
        float k = g->bias.anim > 1.0f ? 1.0f : g->bias.anim;
        DrawText("Aprovados no teste", 214, 83, 10, UI_DIM);
        DrawText("A", 214, 95, 10, UI_TEXT);
        gfx_bar(226, 98, 90, 6, r->rate_a / 100.0f * k, (Color){ 90, 220, 140, 255 }, (Color){ 40, 46, 84, 255 });
        DrawText(TextFormat("%.0f%%", (double)(r->rate_a * k)), 322, 95, 10, UI_TEXT);
        DrawText("B", 214, 107, 10, UI_TEXT);
        gfx_bar(226, 110, 90, 6, r->rate_b / 100.0f * k, (Color){ 240, 150, 90, 255 }, (Color){ 40, 46, 84, 255 });
        DrawText(TextFormat("%.0f%%", (double)(r->rate_b * k)), 322, 107, 10, UI_TEXT);
        DrawText(TextFormat("Diferença: %.0f pts (máx. %.0f)", (double)r->disparity, (double)BIAS_MAX_DISPARITY), 214, 120, 10,
                 r->disparity <= BIAS_MAX_DISPARITY ? UI_GOOD : UI_BAD);
        DrawText(TextFormat("Acerto: %.0f%% (mín. %.0f%%)", (double)r->accuracy, (double)BIAS_MIN_ACCURACY), 214, 132, 10,
                 r->accuracy >= BIAS_MIN_ACCURACY ? UI_GOOD : UI_BAD);
    }

    /* faixa de explicação */
    gfx_panel(8, 156, VIEW_W - 16, 42, (Color){ 20, 26, 58, 255 }, (Color){ 70, 90, 160, 255 });
    if (r->ran && g->bias.anim >= 1.0f)
        gfx_text_wrap(explain(b), 14, 160, VIEW_W - 28, 10, UI_TEXT, -1);
    else if (!r->ran)
        gfx_text_wrap("Monte o conjunto de treino e execute. Meta: no máximo 15 pontos de diferença entre os bairros, com acerto mínimo de 80%.",
                      14, 160, VIEW_W - 28, 10, UI_DIM, -1);
    gfx_text_center("Cima/Baixo: escolher  Esq/Dir: ajustar  Enter: alternar  Esc: sair", VIEW_W / 2, 200, 10, UI_DIM);
}
