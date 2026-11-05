/* Constantes centralizadas do jogo: janela, física, tempos e dificuldade.
 * Para ajustar a "sensação" do personagem, altere apenas os PH_* abaixo. */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define GAME_TITLE   "REV.IA - DECIFRA.IA"
#define GAME_VERSION "0.1.0"

/* --- Tela e laço principal --- */
#define TILE_SIZE      8
#define VIEW_W         384          /* resolução interna 16:9 (escalada para a janela) */
#define VIEW_H         216
#define FIXED_DT       (1.0f / 60.0f)
#define MAX_FRAME_TIME 0.25f
#define MAX_STEPS      5

/* --- Física do jogador (pixels e segundos da resolução interna) --- */
#define PH_W              6.0f
#define PH_H              10.0f
#define PH_RUN_MAX        92.0f
#define PH_RUN_ACCEL      900.0f
#define PH_AIR_ACCEL      650.0f
#define PH_FRICTION_GROUND 1100.0f
#define PH_FRICTION_AIR   350.0f
#define PH_OVER_DECEL     250.0f    /* desaceleração quando acima da velocidade máxima */
#define PH_GRAVITY        820.0f
#define PH_FALL_MAX       170.0f
#define PH_APEX_THRESH    40.0f     /* perto do ápice a gravidade cai pela metade */
#define PH_JUMP_SPEED     245.0f
#define PH_JUMP_BOOST     18.0f
#define PH_JUMP_CUT       0.45f     /* fator ao soltar o botão (altura variável) */
#define PH_WALL_SLIDE     38.0f
#define PH_WJ_X           135.0f
#define PH_WJ_Y           225.0f
#define PH_WJ_LOCK        0.16f
#define PH_CLIMB_UP       42.0f
#define PH_CLIMB_DOWN     80.0f
#define PH_STAMINA_UP     38.0f
#define PH_STAMINA_HOLD   9.0f
#define PH_STAMINA_JUMP   25.0f
#define PH_LEDGE_HOP_Y    150.0f
#define PH_LEDGE_HOP_X    65.0f
#define PH_DASH_SPEED     230.0f
#define PH_DASH_TIME      0.16f
#define PH_DASH_COOLDOWN  0.26f
#define PH_DASH_END_SPEED 95.0f
#define PH_DASHJUMP_X     125.0f

/* --- Elementos do cenário --- */
#define CRUMBLE_SHAKE     0.35f     /* tempo tremendo antes de sumir */
#define CRUMBLE_GONE      2.2f      /* tempo ausente antes de voltar */
#define CRYSTAL_RESPAWN   2.5f

/* --- Dificuldade e acessibilidade --- */
typedef enum { DIFF_NORMAL = 0, DIFF_ASSIST = 1, DIFF_CHALLENGE = 2, DIFF_COUNT } Difficulty;

typedef struct {
    float coyote_time;       /* tolerância para pular logo após sair da borda */
    float jump_buffer_time;  /* tolerância para apertar pulo um pouco antes de pousar */
    float stamina_max;       /* energia de escalada */
    float respawn_delay;     /* tempo até renascer */
    bool  infinite_dash;     /* dash ilimitado (modo assistido) */
} Tuning;

Tuning      config_tuning(Difficulty d);
const char *config_difficulty_name(Difficulty d);

#endif
