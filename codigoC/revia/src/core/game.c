#include "core/game.h"

#include "narrative/decisions.h"
#include "narrative/missions.h"
#include "education/codex.h"
#include "render/gfx.h"
#include "render/world_draw.h"
#include "systems/save.h"
#include "ui/screens.h"

#include <math.h>
#include <stdarg.h>
#ifdef REVIA_SMOKE
#include <stdlib.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FADE_SPEED 3.2f

/* ------------------------------------------------------------------ */
/* Utilidades                                                          */
/* ------------------------------------------------------------------ */

void game_toast(Game *g, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g->toast, sizeof g->toast, fmt, ap);
    va_end(ap);
    g->toast_t = 3.0f;
}

void game_save(Game *g) {
    if (!save_write(&g->prog, SAVE_FILE)) {
        game_toast(g, "Não foi possível salvar o progresso!");
        return;
    }
    g->has_save = true;
}

void game_apply_settings(Game *g) {
    g->tuning = config_tuning((Difficulty)g->prog.difficulty);
    audio_set_volume(&g->audio, g->prog.volume);
}

void game_quit(Game *g) {
#ifdef REVIA_SMOKE
    (void)g;                      /* no teste automático, "Sair" não encerra a execução */
#else
    g->running = false;
#endif
}

static int act_track(int act) { return act >= 1 && act <= 4 ? act - 1 : 0; }

/* O jogo precisa achar levels/ mesmo se for executado de outra pasta. */
static void locate_assets(void) {
    if (DirectoryExists("levels")) return;
    const char *app = GetApplicationDirectory();
    static const char *rel[] = { "", "..", "../..", "../../.." };
    char path[512];
    for (int i = 0; i < 4; i++) {
        snprintf(path, sizeof path, "%s%s", app, rel[i]);
        if (ChangeDirectory(path) && DirectoryExists("levels")) return;
    }
}

/* ------------------------------------------------------------------ */
/* Transições                                                          */
/* ------------------------------------------------------------------ */

void game_fade_to(Game *g, FadeAction action, int arg) {
    if (g->fade_action != FA_NONE) return;
    g->fade_action = action;
    g->fade_arg = arg;
    g->fade_target = 1.0f;
}

static bool load_act(Game *g, int act) {
    char err[160] = "";
    if (act < 1 || act > NUM_ACTS || !level_load(&g->level, act_path(act), err, sizeof err)) {
        snprintf(g->load_error, sizeof g->load_error, "Erro ao carregar o Ato %d: %s", act, err[0] ? err : "ato inválido");
        g->state = ST_MENU;
        audio_music(&g->audio, 0);
        return false;
    }
    g->load_error[0] = '\0';
    g->playing_act = act;
    entities_prepare(&g->level, g->prog.collected);
    hazards_init(&g->haz, &g->level, (Difficulty)g->prog.difficulty);
    player_reset(&g->player, g->level.spawn);
    g->player.stamina = g->tuning.stamina_max;
    camera_snap(&g->cam, &g->level, (Vec2f){ g->player.pos.x, g->player.pos.y });
    fx_clear(&g->fx);
    memset(&g->ev, 0, sizeof g->ev);
    g->ev.near_interact = -1;
    g->pal = *palette_get(g->level.palette);
    g->accumulator = 0;
    g->death_timer = 0;
    snprintf(g->banner, sizeof g->banner, "%s", g->level.name);
    g->banner_t = 4.0f;
    g->state = ST_PLAY;
    audio_music(&g->audio, act_track(act));
    return true;
}

static void run_fade_action(Game *g) {
    switch (g->fade_action) {
    case FA_LOAD_ACT: load_act(g, g->fade_arg); break;
    case FA_TO_MENU:
        g->state = ST_MENU;
        g->menu_cursor = 0;
        g->pal = *palette_get(0);
        fx_clear(&g->fx);
        audio_music(&g->audio, 0);
        break;
    case FA_TO_STATE:
        g->state = (GameState)g->fade_arg;
        if (g->state == ST_ENDING) { g->ending.t = 0; g->pal = *palette_get(6); fx_clear(&g->fx); audio_music(&g->audio, 3); }
        if (g->state == ST_CREDITS) { g->credits.y = VIEW_H; audio_music(&g->audio, 0); }
        break;
    default: break;
    }
    g->fade_action = FA_NONE;
    g->fade_target = 0.0f;
}

void game_start_act(Game *g, int act) { game_fade_to(g, FA_LOAD_ACT, act); }

void game_new_game(Game *g) {
    int diff = g->prog.difficulty, vol = g->prog.volume;
    progress_init(&g->prog);
    g->prog.difficulty = diff;
    g->prog.volume = vol;
    game_apply_settings(g);
    game_save(g);
    game_start_act(g, 1);
}

void game_continue(Game *g) {
    int act = g->prog.current_act;
    game_start_act(g, act < 1 ? 1 : act);
}

void game_open_sub(Game *g, GameState sub) {
    g->return_state = g->state;
    g->state = sub;
    g->settings_cursor = 0;
    g->codex_cursor = 0;
}

void game_close_sub(Game *g) {
    game_save(g);
    g->state = g->return_state;
}

/* ------------------------------------------------------------------ */
/* Telas de conteúdo (início e fim)                                    */
/* ------------------------------------------------------------------ */

void game_start_dialogue(Game *g, int id) {
    const Dialogue *d = dialogue_get(id);
    if (!d) return;
    g->dlg.d = d; g->dlg.id = id; g->dlg.line = 0; g->dlg.shown = 0;
    g->state = ST_DIALOGUE;
    audio_play(&g->audio, SFX_SELECT);
}

void game_close_dialogue(Game *g) {
    /* As falas de evidência do Ato 3 contam para a investigação. */
    if (g->dlg.id >= 11 && g->dlg.id <= 13) {
        unsigned bit = 1u << (g->dlg.id - 11);
        if (!(g->prog.evidence & bit)) {
            g->prog.evidence |= (uint8_t)bit;
            int n = (g->prog.evidence & 1) + ((g->prog.evidence >> 1) & 1) + ((g->prog.evidence >> 2) & 1);
            game_toast(g, "Evidência registrada (%d/3)", n);
            audio_play(&g->audio, SFX_COLLECT);
            game_save(g);
        }
    }
    g->state = ST_PLAY;
}

void game_start_call(Game *g, int id) {
    if (!call_get(id)) return;
    g->call.id = id; g->call.phase = 0; g->call.cursor = 0; g->call.delta = 0;
    g->state = ST_CALL;
    audio_play(&g->audio, SFX_CONFIRM);
}

void game_start_quiz(Game *g, int session) {
    if (session < 0 || session >= NUM_QUIZ_SESSIONS) return;
    uint32_t seed = (uint32_t)(GetTime() * 1000.0) * 2654435761u + (uint32_t)g->prog.quiz_attempts[session] * 7919u + 17u;
    quiz_start(&g->quiz.run, session, seed);
    g->quiz.cursor = 0; g->quiz.phase = 0; g->quiz.t = 0; g->quiz.delta = 0; g->quiz.first = false;
    g->state = ST_QUIZ;
    audio_play(&g->audio, SFX_CONFIRM);
}

void game_start_bias(Game *g) {
    bias_init(&g->bias.g, BIAS_SEED);
    g->bias.cursor = 0; g->bias.anim = 1.0f;
    g->state = ST_BIAS;
    audio_play(&g->audio, SFX_CONFIRM);
}

void game_back_to_play(Game *g) { g->state = ST_PLAY; }

void game_respawn_now(Game *g) {
    player_reset(&g->player, g->level.respawn_pos);
    g->player.stamina = g->tuning.stamina_max;
    camera_snap(&g->cam, &g->level, g->player.pos);
    g->death_timer = 0;
}

void game_finish_act(Game *g) {
    int act = g->level.act;
    progress_complete_act(&g->prog, act);
    if (act == NUM_ACTS) g->prog.ending = decisions_compute_ending(&g->prog);
    game_save(g);
    g->actdone.t = 0;
    g->state = ST_ACTDONE;
    audio_play(&g->audio, SFX_CHECKPOINT);
}

/* ------------------------------------------------------------------ */
/* Jogo (passo fixo)                                                   */
/* ------------------------------------------------------------------ */

static void handle_player_events(Game *g) {
    Player *p = &g->player;
    float fx_ = p->pos.x + PH_W * 0.5f, fy = p->pos.y + PH_H;
    if (p->events & EV_JUMP)     { audio_play(&g->audio, SFX_JUMP); fx_dust(&g->fx, fx_, fy, p->facing, g->pal.tile_top); }
    if (p->events & EV_WALLJUMP) { audio_play(&g->audio, SFX_WALLJUMP); fx_burst(&g->fx, fx_ - p->facing * 3, fy - 4, 6, 40, g->pal.tile_top, 0.3f); }
    if (p->events & EV_DASH)     { audio_play(&g->audio, SFX_DASH); camera_add_shake(&g->cam, 1.0f); }
    if (p->events & EV_LAND)     { audio_play(&g->audio, SFX_LAND); fx_dust(&g->fx, fx_ - 2, fy, 1, g->pal.tile_top); fx_dust(&g->fx, fx_ + 2, fy, -1, g->pal.tile_top); }
    if (p->events & EV_DEATH) {
        audio_play(&g->audio, SFX_DEATH);
        fx_burst(&g->fx, fx_, p->pos.y + PH_H * 0.5f, 28, 110, g->pal.accent, 0.7f);
        camera_add_shake(&g->cam, 4.0f);
        g->death_timer = g->tuning.respawn_delay;
        g->prog.deaths++;
    }
    if (p->state == PS_DASH)
        fx_trail(&g->fx, fx_ - 1, p->pos.y + 4, g->player.dash_ready ? g->pal.accent : (Color){ 190, 150, 255, 255 });
}

static void handle_hazard_events(Game *g, const HazardEvents *hev) {
    if (hev->landed) {                 /* objeto bateu no chão: poeira e som (só se estiver perto) */
        float dx = hev->land_x - g->player.pos.x;
        if (dx > -170.0f && dx < 170.0f) {
            audio_play(&g->audio, SFX_LAND);
            camera_add_shake(&g->cam, 0.8f);
        }
        fx_burst(&g->fx, hev->land_x, hev->land_y - 1.0f, 8, 55, g->pal.spike, 0.35f);
    }
}

static void handle_entity_events(Game *g) {
    EntityEvents *ev = &g->ev;
    Player *p = &g->player;
    if (ev->checkpoint) {
        audio_play(&g->audio, SFX_CHECKPOINT);
        fx_burst(&g->fx, p->pos.x + 3, p->pos.y, 14, 60, g->pal.accent, 0.6f);
        game_toast(g, "Checkpoint");
        game_save(g);
    }
    if (ev->crystal) {
        audio_play(&g->audio, SFX_CRYSTAL);
        fx_burst(&g->fx, p->pos.x + 3, p->pos.y + 5, 10, 70, g->pal.accent, 0.4f);
    }
    if (ev->collected_id >= 0) {
        g->prog.collected |= 1u << ev->collected_id;
        const CodexEntry *c = codex_get(ev->collected_id);
        audio_play(&g->audio, SFX_COLLECT);
        fx_burst(&g->fx, p->pos.x + 3, p->pos.y + 5, 16, 80, (Color){ 255, 226, 120, 255 }, 0.7f);
        game_toast(g, "Chip de dados: %s (veja a Enciclopédia)", c ? c->title : "?");
        game_save(g);
    }
    if (ev->at_exit && g->player.state != PS_DEAD) {
        char msg[128];
        if (progress_act_ready(&g->prog, g->level.act, msg, sizeof msg)) game_finish_act(g);
        else if (g->exit_msg_cd <= 0) { game_toast(g, "%s", msg); g->exit_msg_cd = 3.0f; }
    }
}

static void interact(Game *g) {
    if (g->ev.near_interact < 0 || g->player.state == PS_DEAD) return;
    const Interactable *it = &g->level.interactables[g->ev.near_interact];
    switch (it->kind) {
    case INT_DIALOGUE: game_start_dialogue(g, it->id); break;
    case INT_CALL:
        if ((g->prog.calls_done >> it->id) & 1u) game_toast(g, "Chamado já concluído.");
        else game_start_call(g, it->id);
        break;
    case INT_QUIZ:
        if ((g->prog.quiz_done >> it->id) & 1u) game_toast(g, "Avaliação já concluída.");
        else game_start_quiz(g, it->id);
        break;
    case INT_BIAS: game_start_bias(g); break;
    }
}

static void play_step(Game *g) {
    PlayerInput pi = g->in.pi;
    pi.jump_pressed = g->jump_latch;
    pi.dash_pressed = g->dash_latch;
    g->jump_latch = g->dash_latch = false;

    Player *p = &g->player;
    if (p->state == PS_DEAD) {
        p->events = 0;
        g->death_timer -= FIXED_DT;
        if (g->death_timer <= 0) game_respawn_now(g);
    } else {
        player_update(p, &pi, &g->level, &g->tuning, FIXED_DT);
        level_update(&g->level, FIXED_DT, player_hitbox(p));
        {   /* perigos móveis: podem matar o jogador neste mesmo passo */
            HazardEvents hev;
            hazards_update(&g->haz, &g->level, p, FIXED_DT, &hev);
            handle_hazard_events(g, &hev);
        }
        handle_player_events(g);
        entities_update(&g->level, p, FIXED_DT, &g->ev);
        handle_entity_events(g);
        if (g->state != ST_PLAY) return;             /* saiu da fase neste passo */
        /* poeira ao correr */
        g->run_dust_t -= FIXED_DT;
        if (p->on_ground && fabsf(p->vel.x) > 60.0f && g->run_dust_t <= 0) {
            fx_dust(&g->fx, p->pos.x + PH_W * 0.5f, p->pos.y + PH_H, p->vel.x > 0 ? 1 : -1, g->pal.tile_top);
            g->run_dust_t = 0.12f;
        }
    }
    Vec2f focus = { p->pos.x + PH_W * 0.5f, p->pos.y + PH_H * 0.5f };
    camera_update(&g->cam, &g->level, focus, (float)p->facing, FIXED_DT);
    g->prog.play_time += FIXED_DT;
}

static void play_update(Game *g, float dt) {
    if (g->in.pi.jump_pressed) g->jump_latch = true;
    if (g->in.pi.dash_pressed) g->dash_latch = true;

    if (g->in.pause) { g->pause_cursor = 0; g->state = ST_PAUSE; audio_play(&g->audio, SFX_SELECT); return; }
    if (g->in.interact) interact(g);
    if (g->state != ST_PLAY) return;

    g->accumulator += dt;
    int steps = 0;
    while (g->accumulator >= FIXED_DT && steps < MAX_STEPS && g->state == ST_PLAY) {
        play_step(g);
        g->accumulator -= FIXED_DT;
        steps++;
    }
    if (steps == MAX_STEPS) g->accumulator = 0;      /* evita espiral de atraso */
    if (g->exit_msg_cd > 0) g->exit_msg_cd -= dt;
}

/* Vida do cenário: partículas e paleta continuam animando por trás das telas. */
static void ambient_update(Game *g, float dt) {
    bool in_world = g->state == ST_PLAY || g->state == ST_DIALOGUE || g->state == ST_CALL ||
                    g->state == ST_QUIZ || g->state == ST_BIAS || g->state == ST_ACTDONE;
    if (in_world) {
        int target = level_zone_palette(&g->level, g->cam.pos.x + VIEW_W * 0.5f);
        palette_approach(&g->pal, palette_get(target), 1.0f - expf(-2.2f * dt));
        fx_update(&g->fx, dt, g->cam.pos.x, g->cam.pos.y, &g->pal, true);
    } else if (g->state == ST_PAUSE) {
        /* congelado */
    } else {
        int target = g->state == ST_ENDING ? 6 : 0;
        palette_approach(&g->pal, palette_get(target), 1.0f - expf(-2.2f * dt));
        fx_update(&g->fx, dt, 0.0f, 0.0f, &g->pal, true);
    }
    if (g->banner_t > 0) g->banner_t -= dt;
    if (g->toast_t > 0) g->toast_t -= dt;
    if (g->confirm_new_t > 0) { g->confirm_new_t -= dt; if (g->confirm_new_t <= 0) g->confirm_new = false; }
}

/* ------------------------------------------------------------------ */
/* Quadro                                                              */
/* ------------------------------------------------------------------ */

#ifdef REVIA_SMOKE
static long g_state_frames[16];
static int g_max_act_seen, g_ending_seen, g_deaths_seen;
#endif

#ifdef REVIA_SMOKE
/* Só na build de teste: abre as telas de conteúdo e conclui atos, para que o fuzz as exercite. */
static void smoke_scene(Game *g, const char *sc) {
    int a = 0, b = 0, c = 0;
    g->prog.difficulty = DIFF_NORMAL;
    if (sscanf(sc, "play:%d:%d", &a, &b) == 2) {
        load_act(g, a);
        if (b >= 0 && b < g->level.ncheckpoints) {
            g->player.pos = g->level.checkpoints[b].respawn;
            camera_snap(&g->cam, &g->level, g->player.pos);
            g->pal = *palette_get(level_zone_palette(&g->level, g->player.pos.x));
        }
        g->banner_t = b < 0 ? 3.0f : 0.0f;
    } else if (sscanf(sc, "dlg:%d", &a) == 1) {
        load_act(g, 1); g->banner_t = 0; game_start_dialogue(g, a);
        g->dlg.shown = 999;
    } else if (sscanf(sc, "call:%d:%d", &a, &b) == 2) {
        load_act(g, 2); g->banner_t = 0; game_start_call(g, a);
        if (b) { g->call.cursor = 0; g->call.delta = call_get(a)->opts[0].trust; g->call.phase = 1; }
    } else if (sscanf(sc, "quiz:%d:%d:%d", &a, &b, &c) == 3) {
        load_act(g, 1); g->banner_t = 0; game_start_quiz(g, a);
        for (int i = 0; i < b; i++) { quiz_answer(&g->quiz.run, quiz_correct_display_index(&g->quiz.run)); quiz_next(&g->quiz.run); }
        g->quiz.cursor = 1;
        if (c == 1) quiz_answer(&g->quiz.run, quiz_correct_display_index(&g->quiz.run));
        if (c == 2) quiz_answer(&g->quiz.run, (quiz_correct_display_index(&g->quiz.run) + 1) % 4);
        g->quiz.phase = c ? 1 : 0;
    } else if (sscanf(sc, "quizpool:%d:%d:%d", &a, &b, &c) == 3) {
        load_act(g, 1); g->banner_t = 0; game_start_quiz(g, a);
        g->quiz.run.pool_idx[0] = b; g->quiz.run.cur = 0; g->quiz.cursor = 0;
        if (c == 1) quiz_answer(&g->quiz.run, quiz_correct_display_index(&g->quiz.run));
        if (c == 2) quiz_answer(&g->quiz.run, (quiz_correct_display_index(&g->quiz.run) + 1) % 4);
        g->quiz.phase = c ? 1 : 0;
    } else if (sscanf(sc, "bias:%d", &a) == 1) {
        load_act(g, 3); g->banner_t = 0; game_start_bias(g);
        if (a == 1) { bias_run(&g->bias.g); g->bias.anim = 1.0f; }
        if (a == 2) { g->bias.g.n_a = 30; g->bias.g.n_b = 30; g->bias.g.fix_labels = true; bias_run(&g->bias.g); g->bias.anim = 1.0f; g->bias.cursor = 5; }
    } else if (strcmp(sc, "codex") == 0) {
        load_act(g, 1); g->banner_t = 0; g->prog.collected = 0x155; game_open_sub(g, ST_CODEX); g->codex_cursor = 2;
    } else if (strcmp(sc, "pause") == 0)    { load_act(g, 1); g->banner_t = 0; g->state = ST_PAUSE;
    } else if (strcmp(sc, "settings") == 0) { g->state = ST_SETTINGS; g->return_state = ST_MENU;
    } else if (strcmp(sc, "select") == 0)   { g->prog.unlocked_act = 3; g->state = ST_LEVELSELECT; g->select_cursor = 2;
    } else if (strcmp(sc, "credits") == 0)  { g->state = ST_CREDITS; g->credits.y = 40;
    } else if (sscanf(sc, "actdone:%d", &a) == 1) {
        load_act(g, a); g->banner_t = 0; g->prog.trust = 72; g->prog.calls_done = 0x1F; g->prog.collected = 0x77;
        g->prog.play_time = 1234; g->prog.deaths = 31; g->state = ST_ACTDONE; g->actdone.t = 5;
    } else if (sscanf(sc, "ending:%d", &a) == 1) {
        g->prog.ending = a; g->prog.trust = 80 - a * 25; g->prog.call_choice[0] = 0; g->prog.bias_solved = (uint8_t)(a == 0);
        g->prog.play_time = 3700; g->pal = *palette_get(6); g->state = ST_ENDING; g->ending.t = 12;
    }
}

static void smoke_inject(Game *g) {
    static long frame = 0;
    static uint32_t rng = 7;
    frame++;
    const char *scene = getenv("SMOKE_SCENE");
    if (scene) { if (frame == 5) smoke_scene(g, scene); return; }      /* modo "cena": nada além disso */
    rng = rng * 1664525u + 1013904223u;
    if (g->state == ST_QUIZ && g->quiz.phase == 0 && (rng >> 8) % 10 != 0) {
        g->quiz.cursor = quiz_correct_display_index(&g->quiz.run);   /* quase sempre acerta */
        if (frame % 6 == 0) g->in.confirm = true;
    } else if (g->state == ST_QUIZ && frame % 6 == 0) {
        g->in.confirm = true;
    }
    if (g->state != ST_PLAY || frame % 700 != 0) return;
    switch ((frame / 700) % 5) {
    case 0: game_start_call(g, (int)((rng >> 8) % NUM_CALLS)); break;
    case 1: game_start_quiz(g, (int)((rng >> 8) % NUM_QUIZ_SESSIONS)); break;
    case 2: game_start_bias(g); break;
    case 3: case 4:
        g->prog.quiz_done = 0xF; g->prog.calls_done = 0x7F; g->prog.bias_played = 1; g->prog.evidence = 7;
        g->prog.trust = (int)((rng >> 8) % 101);
        g->prog.bias_solved = (uint8_t)((rng >> 12) & 1u);
        game_finish_act(g);
        break;
    }
}
#endif

static void game_update(Game *g, float dt) {
    input_poll(&g->in);
#ifdef REVIA_SMOKE
    smoke_inject(g);
    g_state_frames[g->state]++;
    if (g->state == ST_PLAY && g->level.act > g_max_act_seen) g_max_act_seen = g->level.act;
    if (g->state == ST_ENDING) g_ending_seen = 1;
    g_deaths_seen = (int)g->prog.deaths;
#endif

    /* fade */
    g->fade = approachf(g->fade, g->fade_target, FADE_SPEED * dt);
    if (g->fade_action != FA_NONE && g->fade >= 1.0f) run_fade_action(g);
    bool busy = g->fade_action != FA_NONE;

    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    ambient_update(g, dt);
    if (busy) {
        g->in.confirm = g->in.back = g->in.interact = g->in.pause = false;
        g->in.up_p = g->in.down_p = g->in.left_p = g->in.right_p = false;
        if (g->state != ST_PLAY) return;
    }

    switch (g->state) {
    case ST_MENU:        menu_update(g, dt); break;
    case ST_LEVELSELECT: select_update(g, dt); break;
    case ST_SETTINGS:    settings_update(g, dt); break;
    case ST_CODEX:       codex_update(g, dt); break;
    case ST_CREDITS:     credits_update(g, dt); break;
    case ST_PLAY:        play_update(g, dt); break;
    case ST_DIALOGUE:    dialogue_update(g, dt); break;
    case ST_CALL:        call_update(g, dt); break;
    case ST_QUIZ:        quiz_update(g, dt); break;
    case ST_BIAS:        bias_update(g, dt); break;
    case ST_PAUSE:       pause_update(g, dt); break;
    case ST_ACTDONE:     actdone_update(g, dt); break;
    case ST_ENDING:      ending_update(g, dt); break;
    }
    audio_update(&g->audio);
}

static bool state_uses_world(GameState s) {
    return s == ST_PLAY || s == ST_DIALOGUE || s == ST_CALL || s == ST_QUIZ || s == ST_BIAS ||
           s == ST_PAUSE || s == ST_ACTDONE;
}

static void game_draw(Game *g) {
    BeginTextureMode(g->target);
    ClearBackground(BLACK);

    GameState s = g->state;
    GameState base = (s == ST_SETTINGS || s == ST_CODEX) ? g->return_state : s;
    if (state_uses_world(base)) {
        draw_world(g);
        if (base == ST_PLAY || base == ST_DIALOGUE || base == ST_PAUSE) hud_draw(g);
        if (base == ST_CALL || base == ST_QUIZ || base == ST_BIAS || base == ST_PAUSE ||
            base == ST_ACTDONE || s == ST_SETTINGS || s == ST_CODEX) ui_dim(150);
    } else if (base == ST_ENDING) {
        draw_background(&g->pal, g->time * 8.0f, 40.0f, g->time);
        fx_draw(&g->fx, 0, 0);
        draw_vignette();
    } else {
        draw_menu_scene(g, g->time * 6.0f);
    }

    switch (s) {
    case ST_MENU:        menu_draw(g); break;
    case ST_LEVELSELECT: select_draw(g); break;
    case ST_SETTINGS:    settings_draw(g); break;
    case ST_CODEX:       codex_draw(g); break;
    case ST_CREDITS:     credits_draw(g); break;
    case ST_DIALOGUE:    dialogue_draw(g); break;
    case ST_CALL:        call_draw(g); break;
    case ST_QUIZ:        quiz_draw(g); break;
    case ST_BIAS:        bias_draw(g); break;
    case ST_PAUSE:       pause_draw(g); break;
    case ST_ACTDONE:     actdone_draw(g); break;
    case ST_ENDING:      ending_draw(g); break;
    case ST_PLAY: break;
    }
    if (s != ST_MENU || g->toast_t > 0) hud_draw_toast(g);
    if (g->fade > 0.001f) DrawRectangle(0, 0, VIEW_W, VIEW_H, Fade(BLACK, g->fade));
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float scale = fminf(sw / VIEW_W, sh / VIEW_H);
    float dw = VIEW_W * scale, dh = VIEW_H * scale;
    Rectangle src = { 0, 0, (float)VIEW_W, -(float)VIEW_H };          /* textura invertida em Y */
    Rectangle dst = { floorf((sw - dw) * 0.5f), floorf((sh - dh) * 0.5f), dw, dh };
    DrawTexturePro(g->target.texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndDrawing();
}

/* ------------------------------------------------------------------ */
/* Ciclo de vida                                                       */
/* ------------------------------------------------------------------ */

Game *game_create(void) {
    Game *g = calloc(1, sizeof *g);
    if (!g) return NULL;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, GAME_TITLE);
    SetExitKey(KEY_NULL);                  /* Esc pausa; não fecha o jogo */
    SetWindowMinSize(VIEW_W, VIEW_H);
    SetTargetFPS(60);

    g->target = LoadRenderTexture(VIEW_W, VIEW_H);
    if (g->target.id == 0) {
        fprintf(stderr, "Erro: não foi possível criar a textura de renderização.\n");
        CloseWindow();
        free(g);
        return NULL;
    }
    SetTextureFilter(g->target.texture, TEXTURE_FILTER_POINT);
    locate_assets();

    audio_init(&g->audio);
    fx_init(&g->fx);
    progress_init(&g->prog);
    Progress saved;
    if (save_read(&saved, SAVE_FILE)) { g->prog = saved; g->has_save = true; }
    game_apply_settings(g);

    g->pal = *palette_get(0);
    g->state = ST_MENU;
    g->return_state = ST_MENU;
    g->running = true;
    g->ev.near_interact = -1;
    g->fade = 1.0f;                        /* entra com fade-in */
    g->fade_target = 0.0f;
    audio_music(&g->audio, 0);
    return g;
}

void game_run(Game *g) {
    while (g->running && !WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > MAX_FRAME_TIME) dt = MAX_FRAME_TIME;
        g->time += dt;
        game_update(g, dt);
        game_draw(g);
    }
    if (g->state != ST_MENU || g->has_save) game_save(g);
}

void game_destroy(Game *g) {
    if (!g) return;
#ifdef REVIA_SMOKE
    static const char *names[] = { "menu", "selecao", "config", "enciclopedia", "creditos", "jogo", "dialogo",
                                   "chamado", "quiz", "vies", "pausa", "ato-concluido", "final" };
    fprintf(stderr, "[smoke] quadros por tela:");
    for (int i = 0; i < 13; i++) fprintf(stderr, " %s=%ld", names[i], g_state_frames[i]);
    fprintf(stderr, "\n[smoke] maior ato jogado=%d, mortes=%d, viu final=%d, confianca=%d\n",
            g_max_act_seen, g_deaths_seen, g_ending_seen, g->prog.trust);
#endif
    audio_shutdown(&g->audio);
    UnloadRenderTexture(g->target);
    CloseWindow();
    free(g);
}
