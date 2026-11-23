#include "ui/screens.h"

#include "render/gfx.h"

#include <math.h>
#include <stdio.h>

/* Menos controles, mais claro: o Norte fica fixo em 30 casos; o jogador mexe só no que importa. */
enum { B_NB, B_GROUP, B_PROXY, B_LABELS, B_RUN, B_COUNT };

static void invalidate(Game *g) { g->bias.g.result.ran = false; }

void bias_update(Game *g, float dt) {
    BiasGame *b = &g->bias.g;
    if (g->bias.anim < 1.0f) g->bias.anim += dt * 1.6f;
    if (g->in.up_p)   { g->bias.cursor = (g->bias.cursor + B_COUNT - 1) % B_COUNT; audio_play(&g->audio, SFX_SELECT); }
    if (g->in.down_p) { g->bias.cursor = (g->bias.cursor + 1) % B_COUNT;           audio_play(&g->audio, SFX_SELECT); }
    if (g->in.back) { game_back_to_play(g); return; }

    int dir = (g->in.right_p ? 1 : 0) - (g->in.left_p ? 1 : 0);
    switch (g->bias.cursor) {
    case B_NB: {
        if (!dir) break;
        int v = b->n_b + dir * 2;
        b->n_b = v < 0 ? 0 : (v > BIAS_POOL ? BIAS_POOL : v);
        invalidate(g);
        audio_play(&g->audio, SFX_SELECT);
        break; }
    case B_GROUP:  if (g->in.confirm || dir) { b->use_group = !b->use_group;  invalidate(g); audio_play(&g->audio, SFX_SELECT); } break;
    case B_PROXY:  if (g->in.confirm || dir) { b->use_proxy = !b->use_proxy;  invalidate(g); audio_play(&g->audio, SFX_SELECT); } break;
    case B_LABELS: if (g->in.confirm || dir) { b->fix_labels = !b->fix_labels; invalidate(g); audio_play(&g->audio, SFX_SELECT); } break;
    case B_RUN:
        if (!g->in.confirm) break;
        if (b->result.ran && b->result.solved && g->bias.anim >= 1.0f) {   /* já deu certo: Enter conclui e volta ao jogo */
            audio_play(&g->audio, SFX_CONFIRM);
            game_save(g);
            game_back_to_play(g);
            return;
        }
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
            return "Justo! Corrigir o histórico atacou a causa: o passado tratava o Bairro Sul com mais rigor.";
        return "Dentro do limite. Sem o bairro o modelo não separa os grupos, mas o viés ainda está nos dados antigos.";
    }
    if (b->use_proxy && !b->use_group)
        return "O CEP funciona como proxy do bairro: mesmo sem a coluna, o viés voltou.";
    if (b->use_group || b->use_proxy)
        return "O modelo usa o bairro e aprendeu o rigor do histórico: mais negados no Sul, com as mesmas qualificações.";
    if (r->accuracy < BIAS_MIN_ACCURACY)
        return "Poucos exemplos: o modelo errou demais. Dados insuficientes também prejudicam.";
    return "Ainda há diferença entre os bairros. Revise os dados e o histórico.";
}

static void draw_rate(const char *name, int y, float pct, Color fill) {
    DrawText(name, 212, y, 10, UI_TEXT);
    gfx_bar(246, y + 3, 90, 6, pct / 100.0f, fill, (Color){ 40, 46, 84, 255 });
    DrawText(TextFormat("%.0f%%", (double)pct), 342, y, 10, UI_TEXT);
}

void bias_draw(const Game *g) {
    const BiasGame *b = &g->bias.g;
    const BiasResult *r = &b->result;
    const Color green = { 90, 220, 140, 255 }, orange = { 240, 150, 90, 255 };
    bool shown = r->ran && g->bias.anim >= 1.0f;
    float k = g->bias.anim > 1.0f ? 1.0f : g->bias.anim;

    gfx_panel(4, 4, VIEW_W - 8, VIEW_H - 8, UI_PANEL, UI_BORDER);
    DrawRectangle(5, 5, VIEW_W - 10, 14, (Color){ 30, 44, 92, 255 });
    DrawText("SIMULAÇÃO DE CRÉDITO  -  NÓVA", 12, 7, 10, UI_ACCENT);
    gfx_text_right(g->prog.bias_solved ? "viés corrigido" : "viés ativo", VIEW_W - 12, 7, 10, g->prog.bias_solved ? UI_GOOD : UI_BAD);
    gfx_text_center("Meta: diferença até 15 pts entre os bairros e acerto de 80%+",
                    VIEW_W / 2, 24, 10, UI_DIM);

    /* esquerda: o que o modelo vai usar */
    gfx_panel(8, 38, 192, 100, (Color){ 20, 26, 58, 255 }, (Color){ 70, 90, 160, 255 });
    DrawText("DADOS DO MODELO", 14, 42, 10, UI_DIM);
    char row[B_COUNT][48];
    snprintf(row[B_NB], sizeof row[0], "Casos do Sul (B):  < %d >", b->n_b);
    snprintf(row[B_GROUP], sizeof row[0], "Usar o Bairro:  %s", b->use_group ? "SIM" : "NÃO");
    snprintf(row[B_PROXY], sizeof row[0], "Usar o CEP:  %s", b->use_proxy ? "SIM" : "NÃO");
    snprintf(row[B_LABELS], sizeof row[0], "Corrigir o histórico:  %s", b->fix_labels ? "SIM" : "NÃO");
    snprintf(row[B_RUN], sizeof row[0], "%s", shown && r->solved ? "[ CONCLUIR ]" : "[ TESTAR O MODELO ]");
    for (int i = 0; i < B_COUNT; i++) {
        bool sel = i == g->bias.cursor;
        int y = 56 + i * 15 + (i == B_RUN ? 4 : 0);
        if (sel) DrawRectangle(11, y - 2, 186, 13, (Color){ 40, 56, 110, 200 });
        DrawText(row[i], 16, y, 10, sel ? UI_ACCENT : UI_TEXT);
    }

    /* direita: um único quadro, que muda de "histórico" para "resultado do teste" */
    gfx_panel(206, 38, VIEW_W - 218, 100, (Color){ 20, 26, 58, 255 }, (Color){ 70, 90, 160, 255 });
    if (!r->ran) {
        DrawText("O QUE O HISTÓRICO MOSTRA", 212, 42, 10, UI_DIM);
        draw_rate("Norte", 58, bias_train_rate(b, false), green);
        draw_rate("Sul", 76, bias_train_rate(b, true), orange);
        gfx_text_wrap("% de pedidos aprovados no passado.", 212, 96, VIEW_W - 232, 10, UI_DIM, -1);
    } else {
        DrawText("RESULTADO DO TESTE", 212, 42, 10, UI_DIM);
        draw_rate("Norte", 58, r->rate_a * k, green);
        draw_rate("Sul", 76, r->rate_b * k, orange);
        if (shown) {
            DrawText(TextFormat("Diferença: %.0f pts (máx. %.0f)", (double)r->disparity, (double)BIAS_MAX_DISPARITY), 212, 98, 10,
                     r->disparity <= BIAS_MAX_DISPARITY ? UI_GOOD : UI_BAD);
            DrawText(TextFormat("Acerto: %.0f%% (mín. %.0f%%)", (double)r->accuracy, (double)BIAS_MIN_ACCURACY), 212, 112, 10,
                     r->accuracy >= BIAS_MIN_ACCURACY ? UI_GOOD : UI_BAD);
        }
    }

    /* explicação */
    gfx_panel(8, 144, VIEW_W - 16, 50, (Color){ 20, 26, 58, 255 }, (Color){ 70, 90, 160, 255 });
    if (shown)
        gfx_text_wrap(explain(b), 14, 149, VIEW_W - 28, 10, r->solved ? UI_GOOD : UI_TEXT, -1);
    else if (r->ran)
        gfx_text_wrap("Calculando...", 14, 149, VIEW_W - 28, 10, UI_DIM, -1);
    else
        gfx_text_wrap("No passado, o Bairro Sul foi avaliado com mais rigor. Se o modelo aprender isso, vai repetir a injustiça. Ajuste os dados e teste.",
                      14, 149, VIEW_W - 28, 10, UI_DIM, -1);
    gfx_text_center(shown && r->solved ? "Enter: concluir   Esc: sair" : "Cima/Baixo: escolher   Esq/Dir: ajustar   Enter: testar   Esc: sair",
                    VIEW_W / 2, 200, 10, UI_DIM);
}