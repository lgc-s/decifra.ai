/* Estado global do jogo e máquina de estados.
 * A lógica de regras fica nos módulos player/world/narrative/education/systems;
 * render/ e ui/ apenas leem o estado e desenham. */
#ifndef GAME_H
#define GAME_H

#include "raylib.h"

#include "audio.h"
#include "config.h"
#include "input.h"
#include "education/bias_game.h"
#include "education/quiz.h"
#include "entities/entities.h"
#include "entities/hazards.h"
#include "narrative/dialogue.h"
#include "player/player.h"
#include "render/fx.h"
#include "render/palette.h"
#include "systems/progression.h"
#include "world/camera.h"
#include "world/level.h"

typedef enum {
    ST_MENU, ST_LEVELSELECT, ST_SETTINGS, ST_CODEX, ST_CREDITS,
    ST_PLAY, ST_DIALOGUE, ST_CALL, ST_QUIZ, ST_BIAS, ST_PAUSE, ST_ACTDONE, ST_ENDING
} GameState;

typedef enum { FA_NONE, FA_LOAD_ACT, FA_TO_MENU, FA_TO_STATE } FadeAction;

typedef struct Game {
    GameState state, return_state;      /* return_state: para onde voltam Configurações/Enciclopédia */
    bool running;

    Progress prog;
    bool has_save;
    Level level;
    Player player;
    GameCamera cam;
    Tuning tuning;
    EntityEvents ev;
    Hazards haz;                         /* bugs no chão e objetos que caem do céu */
    int playing_act;

    Palette pal;                         /* paleta atual (muda suavemente entre regiões) */
    Fx fx;
    Audio audio;
    GameInput in;
    RenderTexture2D target;

    float time, accumulator;
    bool jump_latch, dash_latch;         /* garantem que um aperto não se perca entre passos */
    float fade, fade_target;
    FadeAction fade_action;
    int fade_arg;
    float death_timer, run_dust_t, banner_t, exit_msg_cd;
    char banner[96];
    char toast[128];
    float toast_t;
    char load_error[200];

    /* menus */
    int menu_cursor, select_cursor, settings_cursor, pause_cursor, codex_cursor;
    bool confirm_new;
    float confirm_new_t;

    /* telas de conteúdo */
    struct { const Dialogue *d; int id, line; float shown; } dlg;
    struct { int id, phase, cursor, delta; } call;              /* phase 0 escolhe, 1 resultado */
    struct { QuizRun run; int cursor, phase; float t; int delta; bool first; } quiz;   /* phase 0 pergunta, 1 feedback, 2 resultado */
    struct { BiasGame g; int cursor; float anim; } bias;
    struct { float t; } actdone;
    struct { float t; } ending;
    struct { float y; } credits;
} Game;

Game *game_create(void);
void  game_run(Game *g);
void  game_destroy(Game *g);

/* --- API usada pelas telas (ui/) --- */
void game_toast(Game *g, const char *fmt, ...);
void game_save(Game *g);
void game_apply_settings(Game *g);
void game_fade_to(Game *g, FadeAction action, int arg);
void game_new_game(Game *g);
void game_continue(Game *g);
void game_start_act(Game *g, int act);          /* com fade */
void game_start_dialogue(Game *g, int id);
void game_close_dialogue(Game *g);
void game_start_call(Game *g, int id);
void game_start_quiz(Game *g, int session);
void game_start_bias(Game *g);
void game_back_to_play(Game *g);
void game_respawn_now(Game *g);
void game_finish_act(Game *g);
void game_open_sub(Game *g, GameState sub);     /* Configurações/Enciclopédia, voltando depois */
void game_close_sub(Game *g);
void game_quit(Game *g);

#endif
