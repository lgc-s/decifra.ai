#include "ui/screens.h"

#include "render/gfx.h"
#include "systems/save.h"

#include <stdio.h>
#include <string.h>

/* Desenha uma lista vertical centralizada; o item selecionado é destacado. */
static void draw_list(const char *const *items, int n, int cursor, int cx, int y0, int step, const bool *disabled) {
    for (int i = 0; i < n; i++) {
        bool sel = i == cursor;
        bool off = disabled && disabled[i];
        Color c = off ? (Color){ 90, 98, 130, 255 } : (sel ? UI_ACCENT : UI_TEXT);
        if (sel) {
            int w = MeasureText(items[i], 10);
            DrawRectangle(cx - w / 2 - 14, y0 + i * step + 4, 4, 4, UI_ACCENT);
            DrawRectangle(cx + w / 2 + 10, y0 + i * step + 4, 4, 4, UI_ACCENT);
        }
        gfx_text_center(items[i], cx, y0 + i * step, 10, c);
    }
}

static int wrap_cursor(int c, int delta, int n) { return (c + delta + n) % n; }

/* ------------------------------------------------------------------ */
/* Menu principal                                                      */
/* ------------------------------------------------------------------ */

enum { M_CONTINUE, M_NEW, M_SELECT, M_CODEX, M_SETTINGS, M_CREDITS, M_QUIT };

static int build_items(const Game *g, const char **labels, int *ids) {
    int n = 0;
    if (g->has_save) { labels[n] = "Continuar"; ids[n++] = M_CONTINUE; }
    labels[n] = "Novo jogo"; ids[n++] = M_NEW;
    if (g->has_save) { labels[n] = "Selecionar ato"; ids[n++] = M_SELECT; }
    labels[n] = "Enciclopédia"; ids[n++] = M_CODEX;
    labels[n] = "Configurações"; ids[n++] = M_SETTINGS;
    labels[n] = "Créditos"; ids[n++] = M_CREDITS;
    labels[n] = "Sair"; ids[n++] = M_QUIT;
    return n;
}

void menu_update(Game *g, float dt) {
    (void)dt;
    const char *labels[8]; int ids[8];
    int n = build_items(g, labels, ids);
    if (g->menu_cursor >= n) g->menu_cursor = 0;

    if (g->in.up_p)   { g->menu_cursor = wrap_cursor(g->menu_cursor, -1, n); audio_play(&g->audio, SFX_SELECT); }
    if (g->in.down_p) { g->menu_cursor = wrap_cursor(g->menu_cursor, 1, n);  audio_play(&g->audio, SFX_SELECT); }
    if (!g->in.confirm) return;
    audio_play(&g->audio, SFX_CONFIRM);

    switch (ids[g->menu_cursor]) {
    case M_CONTINUE: game_continue(g); break;
    case M_NEW:
        if (g->has_save && !g->confirm_new) {
            g->confirm_new = true;
            g->confirm_new_t = 4.0f;
            game_toast(g, "Isso apaga o progresso salvo. Confirme de novo para começar.");
        } else {
            g->confirm_new = false;
            game_new_game(g);
        }
        break;
    case M_SELECT: g->state = ST_LEVELSELECT; g->select_cursor = g->prog.unlocked_act - 1; break;
    case M_CODEX: game_open_sub(g, ST_CODEX); break;
    case M_SETTINGS: game_open_sub(g, ST_SETTINGS); break;
    case M_CREDITS: game_fade_to(g, FA_TO_STATE, ST_CREDITS); break;
    case M_QUIT: game_quit(g); break;
    }
}

void menu_draw(const Game *g) {
    int cx = VIEW_W / 2;
    /* título */
    DrawText("REV.IA", cx - MeasureText("REV.IA", 40) / 2 + 2, 22, 40, (Color){ 20, 60, 90, 255 });
    DrawText("REV.IA", cx - MeasureText("REV.IA", 40) / 2, 20, 40, UI_TEXT);
    gfx_text_center("DECIFRA.IA", cx, 62, 20, UI_ACCENT);
    gfx_text_center("Alfabetização algorítmica em uma cidade que não entende a própria IA", cx, 88, 10, UI_DIM);

    const char *labels[8]; int ids[8];
    int n = build_items(g, labels, ids);
    draw_list(labels, n, g->menu_cursor, cx, 108, 13, NULL);

    DrawText("Setas: mover   Z / Enter: confirmar   X / Esc: voltar   F11: tela cheia", 10, VIEW_H - 14, 10, UI_DIM);
    gfx_text_right("v" GAME_VERSION, VIEW_W - 6, VIEW_H - 24, 10, UI_DIM);
    if (g->load_error[0]) {
        gfx_panel(8, 6, VIEW_W - 16, 16, (Color){ 60, 14, 22, 235 }, UI_BAD);
        DrawText(g->load_error, 14, 10, 10, UI_TEXT);
    }
}

/* ------------------------------------------------------------------ */
/* Seleção de ato                                                      */
/* ------------------------------------------------------------------ */

void select_update(Game *g, float dt) {
    (void)dt;
    if (g->in.up_p)   { g->select_cursor = wrap_cursor(g->select_cursor, -1, NUM_ACTS); audio_play(&g->audio, SFX_SELECT); }
    if (g->in.down_p) { g->select_cursor = wrap_cursor(g->select_cursor, 1, NUM_ACTS);  audio_play(&g->audio, SFX_SELECT); }
    if (g->in.back) { g->state = ST_MENU; return; }
    if (g->in.confirm) {
        if (g->select_cursor + 1 <= g->prog.unlocked_act) {
            audio_play(&g->audio, SFX_CONFIRM);
            game_start_act(g, g->select_cursor + 1);
        } else {
            audio_play(&g->audio, SFX_WRONG);
        }
    }
}

void select_draw(const Game *g) {
    gfx_panel(40, 30, VIEW_W - 80, 150, UI_PANEL, UI_BORDER);
    gfx_text_center("SELECIONAR ATO", VIEW_W / 2, 40, 10, UI_ACCENT);
    for (int i = 0; i < NUM_ACTS; i++) {
        bool open = i + 1 <= g->prog.unlocked_act;
        bool sel = i == g->select_cursor;
        int y = 62 + i * 26;
        Color c = !open ? (Color){ 90, 98, 130, 255 } : (sel ? UI_ACCENT : UI_TEXT);
        if (sel) DrawRectangle(50, y - 3, VIEW_W - 100, 22, (Color){ 40, 56, 100, 160 });
        DrawText(act_title(i + 1), 58, y, 10, c);
        const char *status = !open ? "Bloqueado" : (i + 1 < g->prog.unlocked_act ? "Concluído" : "Disponível");
        gfx_text_right(status, VIEW_W - 58, y, 10, open ? (i + 1 < g->prog.unlocked_act ? UI_GOOD : UI_WARN) : c);
        DrawText(open ? "Rejogar não altera as decisões já tomadas." : "Conclua o ato anterior.", 58, y + 11, 10, UI_DIM);
    }
    gfx_text_center("Z / Enter: jogar     X / Esc: voltar", VIEW_W / 2, 164, 10, UI_DIM);
}

/* ------------------------------------------------------------------ */
/* Configurações                                                       */
/* ------------------------------------------------------------------ */

enum { SET_DIFF, SET_VOL, SET_FULL, SET_BACK, SET_COUNT };

void settings_update(Game *g, float dt) {
    (void)dt;
    if (g->in.up_p)   { g->settings_cursor = wrap_cursor(g->settings_cursor, -1, SET_COUNT); audio_play(&g->audio, SFX_SELECT); }
    if (g->in.down_p) { g->settings_cursor = wrap_cursor(g->settings_cursor, 1, SET_COUNT);  audio_play(&g->audio, SFX_SELECT); }
    int dir = (g->in.right_p ? 1 : 0) - (g->in.left_p ? 1 : 0);

    switch (g->settings_cursor) {
    case SET_DIFF:
        if (dir) {
            g->prog.difficulty = (g->prog.difficulty + dir + DIFF_COUNT) % DIFF_COUNT;
            game_apply_settings(g);
            audio_play(&g->audio, SFX_SELECT);
        }
        break;
    case SET_VOL:
        if (dir) {
            int v = g->prog.volume + dir;
            g->prog.volume = v < 0 ? 0 : (v > 10 ? 10 : v);
            game_apply_settings(g);
            audio_play(&g->audio, SFX_SELECT);
        }
        break;
    case SET_FULL:
        if (g->in.confirm || dir) { ToggleFullscreen(); audio_play(&g->audio, SFX_SELECT); }
        break;
    case SET_BACK:
        if (g->in.confirm) { audio_play(&g->audio, SFX_CONFIRM); game_close_sub(g); return; }
        break;
    }
    if (g->in.back) game_close_sub(g);
}

static const char *difficulty_desc(int d) {
    switch (d) {
    case DIFF_ASSIST: return "Mais tolerância nos comandos, mais energia de escalada, dash ilimitado.";
    case DIFF_CHALLENGE: return "Tolerância mínima nos comandos e menos energia de escalada.";
    default: return "Equilíbrio de referência.";
    }
}

void settings_draw(const Game *g) {
    gfx_panel(50, 30, VIEW_W - 100, 150, UI_PANEL, UI_BORDER);
    gfx_text_center("CONFIGURAÇÕES", VIEW_W / 2, 40, 10, UI_ACCENT);

    char row[SET_COUNT][64];
    snprintf(row[SET_DIFF], sizeof row[0], "Dificuldade:  < %s >", config_difficulty_name((Difficulty)g->prog.difficulty));
    snprintf(row[SET_VOL], sizeof row[0], "Volume:  < %d >", g->prog.volume);
    snprintf(row[SET_FULL], sizeof row[0], "Tela cheia (F11):  %s", IsWindowFullscreen() ? "ligada" : "desligada");
    snprintf(row[SET_BACK], sizeof row[0], "Voltar");
    for (int i = 0; i < SET_COUNT; i++) {
        bool sel = i == g->settings_cursor;
        gfx_text_center(row[i], VIEW_W / 2, 64 + i * 16, 10, sel ? UI_ACCENT : UI_TEXT);
    }
    gfx_text_wrap(difficulty_desc(g->prog.difficulty), 66, 132, VIEW_W - 132, 10, UI_DIM, -1);
    gfx_text_center("Esq/Dir: ajustar     Z / Enter: confirmar     X / Esc: voltar", VIEW_W / 2, 164, 10, UI_DIM);
}

/* ------------------------------------------------------------------ */
/* Pausa                                                               */
/* ------------------------------------------------------------------ */

enum { P_CONTINUE, P_RESTART, P_CODEX, P_SETTINGS, P_MENU, P_COUNT };

void pause_update(Game *g, float dt) {
    (void)dt;
    if (g->in.up_p)   { g->pause_cursor = wrap_cursor(g->pause_cursor, -1, P_COUNT); audio_play(&g->audio, SFX_SELECT); }
    if (g->in.down_p) { g->pause_cursor = wrap_cursor(g->pause_cursor, 1, P_COUNT);  audio_play(&g->audio, SFX_SELECT); }
    if (g->in.pause || g->in.back) { game_back_to_play(g); return; }
    if (!g->in.confirm) return;
    audio_play(&g->audio, SFX_CONFIRM);
    switch (g->pause_cursor) {
    case P_CONTINUE: game_back_to_play(g); break;
    case P_RESTART:  game_respawn_now(g); game_back_to_play(g); break;
    case P_CODEX:    game_open_sub(g, ST_CODEX); break;
    case P_SETTINGS: game_open_sub(g, ST_SETTINGS); break;
    case P_MENU:     game_save(g); game_fade_to(g, FA_TO_MENU, 0); break;
    }
}

void pause_draw(const Game *g) {
    static const char *items[P_COUNT] = { "Continuar", "Voltar ao último checkpoint", "Enciclopédia", "Configurações", "Salvar e ir ao menu" };
    gfx_panel(VIEW_W / 2 - 100, 46, 200, 124, UI_PANEL, UI_BORDER);
    gfx_text_center("PAUSA", VIEW_W / 2, 54, 20, UI_TEXT);
    draw_list(items, P_COUNT, g->pause_cursor, VIEW_W / 2, 88, 16, NULL);
}

/* ------------------------------------------------------------------ */
/* Créditos                                                            */
/* ------------------------------------------------------------------ */

static const char *CREDITS[] = {
    "REV.IA - DECIFRA.IA", "",
    "Um jogo de plataforma sobre alfabetização algorítmica", "",
        "Desenvolvido pela equipe", "#DECIFRA.IA", "", "CESAR School", "",
    "Conceito e história original", "Projeto Decifra.ai", "",
    "Reconstrução em plataforma 2D", "Linguagem C e Raylib", "",
    "Arte, animações e efeitos", "desenhados por código (pixel art procedural)", "",
    "Efeitos sonoros e trilhas", "sintetizados por código", "",
    "Inspiração de gameplay", "a precisão e a filosofia de level design de Celeste", "",
    "Nenhum recurso de terceiros foi copiado.", "",
    "Obrigado por decifrar a NÓVA.", "",
};

void credits_update(Game *g, float dt) {
    g->credits.y -= 24.0f * dt * (IsKeyDown(KEY_DOWN) ? 3.0f : 1.0f);
    float end = g->credits.y + (float)(sizeof CREDITS / sizeof *CREDITS) * 14.0f;
    if (g->in.confirm || g->in.back || end < 0) game_fade_to(g, FA_TO_MENU, 0);
}

void credits_draw(const Game *g) {
    ui_dim(120);
    int n = (int)(sizeof CREDITS / sizeof *CREDITS);
    for (int i = 0; i < n; i++) {
        int y = (int)(g->credits.y + (float)i * 14.0f);
        if (y < -12 || y > VIEW_H) continue;
        bool head = i == 0;
        const char *line = CREDITS[i];
        bool team = line[0] == '#';                 /* "#" no começo = nome da equipe, em destaque */
        if (team) line++;
        gfx_text_center(line, VIEW_W / 2, y, (head || team) ? 20 : 10,
            head ? UI_ACCENT : (team ? UI_GOOD : (i % 3 == 0 ? UI_TEXT : UI_DIM)));
    }
    gfx_text_center("Enter: voltar ao menu", VIEW_W / 2, VIEW_H - 12, 10, UI_DIM);
}
