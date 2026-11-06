/* Testes da lógica do jogo, sem janela e sem Raylib.
 * Compilar/rodar:  make test */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/config.h"
#include "education/bias_game.h"
#include "education/codex.h"
#include "education/quiz.h"
#include "entities/entities.h"
#include "narrative/decisions.h"
#include "narrative/dialogue.h"
#include "narrative/missions.h"
#include "player/player.h"
#include "systems/progression.h"
#include "systems/save.h"
#include "world/level.h"

static int g_fail = 0, g_total = 0;

#define CHECK(cond, ...) do { g_total++; if (!(cond)) { g_fail++; printf("  FALHOU: "); printf(__VA_ARGS__); printf("  (%s:%d)\n", __FILE__, __LINE__); } } while (0)

/* ---------- utilitários de simulação ---------- */
static const char *TMP_LEVEL = "test_tmp_level.txt";

static bool load_ascii(Level *lv, const char **rows, int n) {
    FILE *f = fopen(TMP_LEVEL, "w");
    fprintf(f, "@name teste\n@act 1\n@palette 0\n@map\n");
    for (int i = 0; i < n; i++) fprintf(f, "%s\n", rows[i]);
    fclose(f);
    char err[128];
    bool ok = level_load(lv, TMP_LEVEL, err, sizeof err);
    if (!ok) printf("  erro ao carregar: %s\n", err);
    return ok;
}

static void step(Player *p, Level *lv, PlayerInput in, const Tuning *tu) {
    player_update(p, &in, lv, tu, FIXED_DT);
    level_update(lv, FIXED_DT, player_hitbox(p));
    EntityEvents ev;
    entities_update(lv, p, FIXED_DT, &ev);
}

/* ---------- carregamento das fases reais ---------- */
static void test_levels(void) {
    printf("[fases]\n");
    for (int act = 1; act <= NUM_ACTS; act++) {
        static Level lv;
        char err[160] = "";
        bool ok = level_load(&lv, act_path(act), err, sizeof err);
        CHECK(ok, "ato %d: %s", act, err);
        if (!ok) continue;
        CHECK(lv.act == act, "ato %d: campo @act incorreto (%d)", act, lv.act);
        CHECK(lv.ncollectibles == 3, "ato %d: %d colecionaveis", act, lv.ncollectibles);
        CHECK(lv.has_exit, "ato %d: sem saida", act);
        CHECK(lv.ninteractables >= 5, "ato %d: poucos interativos (%d)", act, lv.ninteractables);
        CHECK(lv.width <= LEVEL_MAX_W && lv.height == 36, "ato %d: tamanho %dx%d", act, lv.width, lv.height);
        /* o jogador começa sobre o chão */
        Player p; player_reset(&p, lv.spawn);
        Tuning tu = config_tuning(DIFF_NORMAL);
        PlayerInput none = {0};
        for (int i = 0; i < 30; i++) step(&p, &lv, none, &tu);
        CHECK(p.on_ground && p.state != PS_DEAD, "ato %d: jogador nao pousa no inicio", act);
        /* ids dos colecionáveis dentro do ato */
        for (int i = 0; i < lv.ncollectibles; i++)
            CHECK(lv.collectibles[i].id >= (act - 1) * 3 && lv.collectibles[i].id < act * 3, "ato %d: id de colecionavel fora do ato", act);
        /* referências válidas */
        for (int i = 0; i < lv.ninteractables; i++) {
            const Interactable *it = &lv.interactables[i];
            if (it->kind == INT_DIALOGUE) CHECK(dialogue_get(it->id) != NULL, "ato %d: dialogo %d inexistente", act, it->id);
            if (it->kind == INT_CALL) CHECK(call_get(it->id) != NULL, "ato %d: chamado %d inexistente", act, it->id);
            if (it->kind == INT_QUIZ) CHECK(it->id >= 0 && it->id < NUM_QUIZ_SESSIONS, "ato %d: sessao de quiz invalida", act);
        }
    }
    Level bad; char err[128];
    CHECK(!level_load(&bad, "nao/existe.txt", err, sizeof err), "arquivo inexistente deveria falhar");
}

/* ---------- física ---------- */
static const char *FLAT[] = {
    "....................",
    "....................",
    "....................",
    "....................",
    "....................",
    "....................",
    "....................",
    "..P.................",
    "####################",
    "####################",
};

static void test_jump(void) {
    printf("[pulo]\n");
    Level lv; Tuning tu = config_tuning(DIFF_NORMAL);
    if (!load_ascii(&lv, FLAT, 10)) { CHECK(0, "mapa"); return; }
    Player p; player_reset(&p, lv.spawn);
    PlayerInput none = {0};
    for (int i = 0; i < 10; i++) step(&p, &lv, none, &tu);
    float ground_y = p.pos.y;
    CHECK(p.on_ground, "deveria estar no chao");

    /* pulo longo (botão mantido) */
    float min_y = ground_y;
    PlayerInput in = {0};
    in.jump_pressed = true; in.jump_held = true;
    step(&p, &lv, in, &tu);
    in.jump_pressed = false;
    for (int i = 0; i < 90; i++) { step(&p, &lv, in, &tu); if (p.pos.y < min_y) min_y = p.pos.y; if (p.on_ground) break; }
    float full = ground_y - min_y;
    CHECK(full > 32.0f && full < 48.0f, "altura do pulo longo = %.1f px (esperado ~36-44)", full);

    /* pulo curto (botão solto logo) */
    for (int i = 0; i < 20; i++) step(&p, &lv, none, &tu);
    min_y = p.pos.y; ground_y = p.pos.y;
    in.jump_pressed = true; in.jump_held = true;
    step(&p, &lv, in, &tu);
    in.jump_pressed = false; in.jump_held = false;
    for (int i = 0; i < 90; i++) { step(&p, &lv, in, &tu); if (p.pos.y < min_y) min_y = p.pos.y; if (p.on_ground) break; }
    float shortj = ground_y - min_y;
    CHECK(shortj < full * 0.6f && shortj > 4.0f, "pulo curto = %.1f px (longo = %.1f)", shortj, full);
}

static const char *GAP5[] = {
    "........................", "........................", "........................", "........................",
    "........................", "........................", "........................",
    "..P.....................",
    "#######.....#############",
    "#######.....#############",
};

static void test_coyote_gap(void) {
    printf("[coyote e vao de 5 tiles]\n");
    Level lv; Tuning tu = config_tuning(DIFF_NORMAL);
    if (!load_ascii(&lv, GAP5, 10)) { CHECK(0, "mapa"); return; }
    Player p; player_reset(&p, lv.spawn);
    PlayerInput none = {0};
    for (int i = 0; i < 10; i++) step(&p, &lv, none, &tu);

    PlayerInput run = {0}; run.right = true;
    bool jumped = false, landed = false;
    for (int f = 0; f < 400 && !landed; f++) {
        PlayerInput in = run;
        in.jump_held = jumped;
        /* pula no instante exato em que o pé passa da borda (coyote time) */
        if (!jumped && p.pos.x > 7 * 8 - PH_W + 2.0f) { in.jump_pressed = true; in.jump_held = true; jumped = true; }
        step(&p, &lv, in, &tu);
        if (jumped && p.on_ground && p.pos.x > 12 * 8) landed = true;
        if (p.state == PS_DEAD) break;
    }
    CHECK(landed, "nao atravessou o vao de 5 tiles (x=%.1f, estado=%d)", p.pos.x, p.state);
}

static const char *DASHGAP[] = {
    "..............................", "..............................", "..............................", "..............................",
    "..............................", "..............................", "..............................",
    "..P...........................",
    "########........###############",
    "########........###############",
};

static bool try_dash_cross(Level *lv, int jump_off, int dash_delay, bool assist) {
    Tuning tu = config_tuning(assist ? DIFF_ASSIST : DIFF_NORMAL);
    Player p; player_reset(&p, lv->spawn);
    PlayerInput none = {0};
    for (int i = 0; i < 10; i++) step(&p, lv, none, &tu);
    PlayerInput run = {0}; run.right = true;
    int jumped_at = -1;
    for (int f = 0; f < 400; f++) {
        PlayerInput in = run;
        in.jump_held = jumped_at >= 0 && f - jumped_at < 20;
        if (jumped_at < 0 && p.pos.x + PH_W >= 8 * 8 - jump_off) { in.jump_pressed = true; in.jump_held = true; jumped_at = f; }
        if (jumped_at >= 0 && f - jumped_at == dash_delay) in.dash_pressed = true;
        step(&p, lv, in, &tu);
        if (p.state == PS_DEAD) return false;
        if (jumped_at >= 0 && p.on_ground && p.pos.x > 16 * 8) return true;
    }
    return false;
}

static void test_dash(void) {
    printf("[dash]\n");
    Level lv;
    if (!load_ascii(&lv, DASHGAP, 10)) { CHECK(0, "mapa"); return; }
    /* sem dash, vão de 8 tiles não pode ser vencido */
    {
        Tuning tu = config_tuning(DIFF_NORMAL);
        Player p; player_reset(&p, lv.spawn);
        PlayerInput none = {0};
        for (int i = 0; i < 10; i++) step(&p, &lv, none, &tu);
        PlayerInput run = {0}; run.right = true;
        bool jumped = false, crossed = false;
        for (int f = 0; f < 300; f++) {
            PlayerInput in = run; in.jump_held = jumped && f < 40;
            if (!jumped && p.pos.x + PH_W >= 8 * 8) { in.jump_pressed = true; in.jump_held = true; jumped = true; }
            step(&p, &lv, in, &tu);
            if (jumped && p.on_ground && p.pos.x > 16 * 8) crossed = true;
            if (p.state == PS_DEAD) break;
        }
        CHECK(!crossed, "o vao de 8 tiles nao deveria ser vencido sem dash");
    }
    int ok_count = 0;
    for (int d = 2; d <= 24; d++) if (try_dash_cross(&lv, 0, d, false)) ok_count++;
    CHECK(ok_count >= 6, "vao de 8 com dash: so %d tempos de dash funcionam", ok_count);
    CHECK(try_dash_cross(&lv, 0, 10, false), "dash apos ~0,17s do pulo deveria vencer o vao");
}

static const char *CHAIN[] = {
    "..................................", "..................................", "..................................", "..................................",
    "..................................", "..................................",
    "..................................",
    "..P.....................o...o.....",
    "########....................#######",
    "########....................#######",
};

static void test_crystal_chain(void) {
    printf("[cristais: vao de 12 tiles]\n");
    /* mapa próprio: 12 tiles de vão com cristais a 4 e 8 tiles da borda, 3 acima do chão */
    const char *rows[] = {
        "..................................",
        "..................................",
        "..................................",
        "..................................",
        "..................................",
        "..................................",
        "............o...o.................",
        "..P...............................",
        "########............##############",
        "########............##############",
    };
    (void)CHAIN;
    Level lv;
    if (!load_ascii(&lv, rows, 10)) { CHECK(0, "mapa"); return; }
    Tuning tu = config_tuning(DIFF_NORMAL);
    bool crossed_any = false;
    for (int delay = 2; delay <= 14 && !crossed_any; delay++) {
        entities_prepare(&lv, 0);
        Player p; player_reset(&p, lv.spawn);
        PlayerInput none = {0};
        for (int i = 0; i < 10; i++) step(&p, &lv, none, &tu);
        PlayerInput run = {0}; run.right = true;
        int jumped_at = -1, dashes = 0, last_dash_f = -100;
        for (int f = 0; f < 500; f++) {
            PlayerInput in = run;
            in.jump_held = jumped_at >= 0 && f - jumped_at < 20;
            if (jumped_at < 0 && p.pos.x + PH_W >= 8 * 8) { in.jump_pressed = true; in.jump_held = true; jumped_at = f; }
            /* primeiro dash após 'delay' frames; depois, dash de novo sempre que recarregar */
            if (jumped_at >= 0 && p.dash_ready && f - jumped_at >= delay && f - last_dash_f > 14 && !p.on_ground) {
                in.dash_pressed = true; last_dash_f = f; dashes++;
            }
            step(&p, &lv, in, &tu);
            if (p.state == PS_DEAD) break;
            if (jumped_at >= 0 && p.on_ground && p.pos.x > 20 * 8) { crossed_any = true; break; }
        }
    }
    CHECK(crossed_any, "nao foi possivel atravessar 12 tiles com dois cristais");
}

static const char *CLIMB[] = {
    "........................", "........................", "........................",
    "..........##############",   /* topo do bloco: linha 3 */
    "..........##############", "..........##############", "..........##############",
    "..........##############", "..........##############", "..........##############",
    "..........##############", "..........##############", "..........##############",
    "..P.......##############",
    "########################",
    "########################",
};

static void test_climb(void) {
    printf("[escalada de 10 tiles com borda]\n");
    Level lv;
    if (!load_ascii(&lv, CLIMB, 16)) { CHECK(0, "mapa"); return; }
    Tuning tu = config_tuning(DIFF_NORMAL);
    Player p; player_reset(&p, lv.spawn);
    PlayerInput none = {0};
    for (int i = 0; i < 10; i++) step(&p, &lv, none, &tu);

    bool reached = false, grabbed = false;
    for (int f = 0; f < 600; f++) {
        PlayerInput in = {0};
        in.right = true;
        in.grab_held = true;
        if (p.wall_dir == 1) in.up = true;
        step(&p, &lv, in, &tu);
        if (p.state == PS_CLIMB) grabbed = true;
        if (p.state == PS_DEAD) break;
        if (p.on_ground && p.pos.y + PH_H <= 3 * 8 + 0.5f && p.pos.x > 10 * 8) { reached = true; break; }
    }
    CHECK(grabbed, "nao agarrou na parede");
    CHECK(reached, "nao chegou ao topo da escalada (y=%.1f x=%.1f estado=%d energia=%.0f)", p.pos.y, p.pos.x, p.state, p.stamina);

    /* wall jump: agarra, depois pula para longe da parede */
    player_reset(&p, lv.spawn);
    for (int i = 0; i < 10; i++) step(&p, &lv, none, &tu);
    bool kicked = false;
    for (int f = 0; f < 200 && !kicked; f++) {
        PlayerInput in = {0}; in.right = true; in.grab_held = true;
        if (p.wall_dir == 1 && p.state == PS_CLIMB) in.up = (f < 60);
        if (p.state == PS_CLIMB && f > 40) { in.jump_pressed = true; in.jump_held = true; }
        step(&p, &lv, in, &tu);
        if (p.events & EV_WALLJUMP) { kicked = true; CHECK(p.vel.x < -50.0f, "wall jump deve afastar da parede (vx=%.1f)", p.vel.x); }
    }
    CHECK(kicked, "wall jump nao aconteceu");
}

static void test_hazards_and_crumble(void) {
    printf("[espinhos, queda e plataformas frageis]\n");
    const char *spike[] = {
        "....................", "....................", "....................", "....................",
        "....................", "....................", "....................",
        "..P.....^^..........",
        "####################", "####################",
    };
    Level lv; Tuning tu = config_tuning(DIFF_NORMAL);
    if (!load_ascii(&lv, spike, 10)) { CHECK(0, "mapa"); return; }
    Player p; player_reset(&p, lv.spawn);
    PlayerInput run = {0}; run.right = true;
    for (int f = 0; f < 120 && p.state != PS_DEAD; f++) step(&p, &lv, run, &tu);
    CHECK(p.state == PS_DEAD, "espinhos deveriam matar");

    const char *pit[] = {
        "....................", "....................", "....................", "....................",
        "....................", "....................", "....................",
        "..P.................",
        "#####...............", "#####...............",
    };
    if (!load_ascii(&lv, pit, 10)) { CHECK(0, "mapa"); return; }
    player_reset(&p, lv.spawn);
    for (int f = 0; f < 300 && p.state != PS_DEAD; f++) step(&p, &lv, run, &tu);
    CHECK(p.state == PS_DEAD, "cair do mapa deveria matar");

    const char *cr[] = {
        "....................", "....................", "....................", "....................",
        "....................", "....................", "....................",
        "..P.................",
        "..~~................", "....................",
    };
    if (!load_ascii(&lv, cr, 10)) { CHECK(0, "mapa"); return; }
    player_reset(&p, lv.spawn);
    PlayerInput none = {0};
    for (int f = 0; f < 10; f++) step(&p, &lv, none, &tu);
    CHECK(p.on_ground, "deveria estar sobre a plataforma fragil");
    for (int f = 0; f < 8; f++) step(&p, &lv, none, &tu);      /* ~0,3 s no total: ainda existe */
    CHECK(lv.crumble[8][2] > 0.0f && p.state != PS_DEAD, "plataforma deveria estar tremendo");
    for (int f = 0; f < 120 && p.state != PS_DEAD; f++) step(&p, &lv, none, &tu);
    CHECK(p.state == PS_DEAD, "plataforma deveria desmoronar e o jogador cair");
}

static void test_assist_and_tuning(void) {
    printf("[dificuldade]\n");
    Tuning n = config_tuning(DIFF_NORMAL), a = config_tuning(DIFF_ASSIST), c = config_tuning(DIFF_CHALLENGE);
    CHECK(a.coyote_time > n.coyote_time && n.coyote_time > c.coyote_time, "coyote deve crescer com assistencia");
    CHECK(a.infinite_dash && !n.infinite_dash && !c.infinite_dash, "dash infinito so no assistido");
}

/* ---------- sistemas educacionais e narrativos ---------- */
static void test_quiz(void) {
    printf("[quiz]\n");
    for (int s = 0; s < NUM_QUIZ_SESSIONS; s++) {
        QuizRun r; quiz_start(&r, s, 1234u + (uint32_t)s);
        int seen = 0;
        while (!r.finished) {
            const QuizQuestion *q = quiz_current(&r);
            CHECK(q != NULL, "questao nula");
            if (!q) break;
            int good = quiz_correct_display_index(&r);
            CHECK(strcmp(quiz_option_text(&r, good), q->options[0]) == 0, "indice da resposta correta incoerente");
            CHECK(quiz_answer(&r, good), "resposta correta deveria acertar");
            quiz_next(&r);
            seen++;
            if (seen > 20) break;
        }
        CHECK(r.finished && r.correct_count == QUIZ_LEN && r.wrong_count == 0 && seen == QUIZ_LEN,
              "sessao %d: todas certas (%d)", s, r.correct_count);
        CHECK(r.correct_mask == (1u << QUIZ_LEN) - 1u && r.wrong_mask == 0, "mascaras incorretas");
        CHECK(quiz_trust_delta(&r) == QUIZ_LEN * QUIZ_TRUST_RIGHT, "soma de confianca com tudo certo");
        for (int i = 0; i < QUIZ_LEN - 1; i++)
            CHECK(r.pool_idx[i] < r.pool_idx[i + 1], "questoes distintas, em ordem crescente de dificuldade");
    }
    /* errar NÃO reinicia: a questão conta como errada e a sessão segue até o fim */
    QuizRun r; quiz_start(&r, 0, 42);
    int wrong = (quiz_correct_display_index(&r) + 1) % 4;
    CHECK(!quiz_answer(&r, wrong), "resposta errada deveria errar");
    CHECK(r.wrong_count == 1 && (r.wrong_mask & 1u) && !r.finished, "erro deveria ser contado");
    quiz_next(&r);
    CHECK(r.cur == 1 && !r.answered && !r.finished, "deve avancar para a proxima apos errar");
    /* tudo errado: termina e a soma é negativa */
    QuizRun w; quiz_start(&w, 2, 7);
    for (int i = 0; i < QUIZ_LEN; i++) {
        quiz_answer(&w, (quiz_correct_display_index(&w) + 1) % 4);
        quiz_next(&w);
    }
    CHECK(w.finished && w.correct_count == 0 && w.wrong_count == QUIZ_LEN, "sessao toda errada deve terminar");
    CHECK(quiz_trust_delta(&w) == -QUIZ_LEN * QUIZ_TRUST_WRONG, "soma de confianca com tudo errado");
    /* misto: 1 erro e o resto certo */
    QuizRun m; quiz_start(&m, 1, 99);
    for (int i = 0; i < QUIZ_LEN; i++) {
        int good = quiz_correct_display_index(&m);
        quiz_answer(&m, i == 1 ? (good + 1) % 4 : good);
        quiz_next(&m);
    }
    CHECK(m.finished && m.wrong_count == 1 && m.correct_count == QUIZ_LEN - 1, "sessao mista");
    CHECK(quiz_trust_delta(&m) == (QUIZ_LEN - 1) * QUIZ_TRUST_RIGHT - QUIZ_TRUST_WRONG, "soma de confianca mista");
    /* a ultima resposta nao pode ser alterada depois de terminar */
    int before = m.correct_count + m.wrong_count;
    quiz_answer(&m, 0); quiz_next(&m);
    CHECK(m.correct_count + m.wrong_count == before, "sessao concluida nao aceita novas respostas");
    /* variações: com sementes diferentes, a seleção ou a ordem das alternativas muda */
    QuizRun a, b; quiz_start(&a, 1, 1); quiz_start(&b, 1, 99999);
    bool differs = memcmp(a.pool_idx, b.pool_idx, sizeof a.pool_idx) != 0 || memcmp(a.perm, b.perm, sizeof a.perm) != 0;
    CHECK(differs, "sementes diferentes deveriam gerar variacoes");
}

static void test_bias(void) {
    printf("[minigame de vies]\n");
    BiasGame g; bias_init(&g, BIAS_SEED);
    bias_run(&g);
    CHECK(g.result.disparity > 15.0f && !g.result.solved, "configuracao inicial deveria ser injusta (disparidade %.1f)", g.result.disparity);

    bias_init(&g, BIAS_SEED); g.n_a = 30; g.n_b = 30;       /* só equilibrar a quantidade não basta */
    bias_run(&g);
    CHECK(!g.result.solved, "equilibrar so a quantidade nao deveria resolver (disp %.1f)", g.result.disparity);

    bias_init(&g, BIAS_SEED); g.use_group = false;           /* tirar o bairro resolve (sem proxy) */
    bias_run(&g);
    CHECK(g.result.solved, "sem bairro e sem CEP deveria resolver (disp %.1f, acc %.1f)", g.result.disparity, g.result.accuracy);

    bias_init(&g, BIAS_SEED); g.use_group = false; g.use_proxy = true;   /* o CEP vaza o bairro */
    bias_run(&g);
    CHECK(!g.result.solved, "o CEP (proxy) deveria manter o vies (disp %.1f)", g.result.disparity);

    bias_init(&g, BIAS_SEED); g.n_a = 30; g.n_b = 30; g.fix_labels = true;   /* corrigir os rótulos resolve a causa */
    bias_run(&g);
    CHECK(g.result.solved, "corrigir rotulos deveria resolver (disp %.1f, acc %.1f)", g.result.disparity, g.result.accuracy);

    bias_init(&g, BIAS_SEED); g.n_a = 0; g.n_b = 0; bias_run(&g);        /* sem dados não quebra */
    CHECK(g.result.ran, "execucao sem dados falhou");
}

static void test_progression_and_endings(void) {
    printf("[progressao, chamados e finais]\n");
    Progress p; progress_init(&p);
    char msg[128];
    CHECK(!progress_act_ready(&p, 1, msg, sizeof msg) && msg[0], "ato 1 nao deveria estar pronto");
    p.quiz_done |= 1u;
    CHECK(progress_act_ready(&p, 1, msg, sizeof msg), "ato 1 deveria estar pronto apos o quiz");
    progress_complete_act(&p, 1);
    CHECK(p.unlocked_act == 2 && p.current_act == 2, "ato 2 deveria liberar");

    p.quiz_done |= 2u;
    for (int i = 0; i < 3; i++) call_apply(&p, i, 0);
    CHECK(!progress_act_ready(&p, 2, msg, sizeof msg), "ato 2 exige os 4 chamados");
    call_apply(&p, 3, 0);
    CHECK(progress_act_ready(&p, 2, msg, sizeof msg), "ato 2 deveria estar pronto");

    int t = p.trust;
    CHECK(call_apply(&p, 0, 1) == 0 && p.trust == t, "chamado concluido nao pode ser refeito");

    /* confiança limitada a 0..100 */
    progress_add_trust(&p, 500); CHECK(p.trust == 100, "limite superior");
    progress_add_trust(&p, -500); CHECK(p.trust == 0, "limite inferior");

    /* finais */
    Progress q; progress_init(&q);
    q.trust = 90; q.bias_solved = 1; CHECK(decisions_compute_ending(&q) == ENDING_CLARITY, "final 1");
    q.bias_solved = 0;                CHECK(decisions_compute_ending(&q) == ENDING_REPAIR, "confianca alta sem corrigir o vies nao e o melhor final");
    q.trust = 40;                     CHECK(decisions_compute_ending(&q) == ENDING_DISTRUST, "final 3");
    q.trust = 10;                     CHECK(decisions_compute_ending(&q) == ENDING_SILENCE, "final 4");
    for (int i = 0; i < NUM_ENDINGS; i++) CHECK(ending_info(i) != NULL && ending_info(i)->title[0], "texto do final %d", i);

    /* todos os chamados têm 2-4 alternativas, e pelo menos uma melhora a confiança */
    for (int i = 0; i < NUM_CALLS; i++) {
        const Call *c = call_get(i);
        CHECK(c && c->nopts >= 2 && c->nopts <= 4, "chamado %d: numero de alternativas", i);
        CHECK(c && c->opts[call_best_option(c)].trust > 0, "chamado %d: sem opcao boa", i);
    }
    for (int i = 0; i < NUM_COLLECTIBLES; i++) CHECK(codex_get(i) != NULL, "enciclopedia %d", i);
}

static void test_save(void) {
    printf("[salvamento]\n");
    const char *path = "test_save.dat";
    Progress a; progress_init(&a);
    a.current_act = 3; a.unlocked_act = 3; a.trust = 73; a.calls_done = 0x1F; a.call_choice[2] = 1;
    a.collected = 0x2A5; a.quiz_done = 3; a.ending = 1; a.bias_played = 1; a.quiz_correct_mask[1] = 0x3FF;
    a.play_time = 1234.5f; a.deaths = 17;
    CHECK(save_write(&a, path), "falha ao salvar");
    Progress b; progress_init(&b);
    CHECK(save_read(&b, path), "falha ao carregar");
    CHECK(memcmp(&a, &b, sizeof a) == 0, "dados carregados diferentes dos salvos");

    /* arquivo corrompido deve ser rejeitado */
    FILE *f = fopen(path, "r+b"); fseek(f, 40, SEEK_SET); fputc(0x7F, f); fclose(f);
    Progress c; progress_init(&c);
    CHECK(!save_read(&c, path), "save corrompido deveria ser rejeitado");
    remove(path);
    CHECK(!save_read(&c, "nao_existe.dat"), "save inexistente");
}

int main(void) {
    test_levels();
    test_jump();
    test_coyote_gap();
    test_dash();
    test_crystal_chain();
    test_climb();
    test_hazards_and_crumble();
    test_assist_and_tuning();
    test_quiz();
    test_bias();
    test_progression_and_endings();
    test_save();
    remove(TMP_LEVEL);
    printf("\n%d verificacoes, %d falhas\n", g_total, g_fail);
    return g_fail ? 1 : 0;
}
