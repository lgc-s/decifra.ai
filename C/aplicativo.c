/* ============================================================================
 * DECIFRA.IA / REV.IA
 * Jogo de terminal sobre alfabetizacao em Inteligencia Artificial
 * CESAR School - Projetos 2 - Departamento de Alfabetizacao Algoritmica
 *
 * Compilar com:
 *   gcc main.c -lSDL2 -lSDL2_ttf -lm -o decifra_game
 *
 * Este arquivo estende a base de codigo original (menu + Ato 1) preservando
 * as funcoes auxiliares de renderizacao (draw_box, render_text, render_button,
 * render_progress_bar) e a struct GameEngine, adicionando:
 *   - Minigame do Cadeado Logico (STATE_MINIGAME_LOCK)
 *   - Ato 2: Rotina de Chamados em Split Screen (STATE_ATO2_TICKETS)
 *   - Ato 3: Escandalo com tema vermelho de alerta (STATE_ATO3_ESCANDALO)
 *   - Ato 4: Desfecho, relatorio final e certificado (STATE_ATO4_DESFECHO)
 *   - Efeitos de "Game Juice": screen shake, barra de confianca interpolada,
 *     efeito de maquina de escrever (typewriter) e textos flutuantes de combo
 *   - Persistencia em disco (save.dat e certificado_conclusao.txt)
 * ============================================================================ */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* --- CONFIGURACOES GERAIS DA JANELA E DO JOGO --- */
#define WINDOW_W            1024
#define WINDOW_H            640
#define QUIZ_BANK_SIZE       10
#define NUM_COUNTRIES         4
#define NUM_TICKETS           5
#define MAX_FLOATING_TEXTS   16
#define MINIGAME_TIME_LIMIT  30.0f   /* segundos */
#define TRUST_INITIAL        50.0f   /* confianca publica inicial (0-100) */

/* --- PALETA DE CORES DA INTERFACE (TEMA PADRAO - VERDE CRT) --- */
static const SDL_Color COLOR_BG       = {5, 12, 5, 255};       /* Fundo quase preto CRT */
static const SDL_Color COLOR_NEON     = {0, 255, 102, 255};    /* Verde Neon Principal */
static const SDL_Color COLOR_DARK_GRN = {0, 60, 25, 255};      /* Verde escuro / Trilhas */
static const SDL_Color COLOR_YELLOW   = {255, 204, 0, 255};    /* Alertas em Amarelo */
static const SDL_Color COLOR_TEXT_BLK = {5, 12, 5, 255};       /* Texto escuro sobre botao ativo */

/* --- PALETA DE CORES ALTERNATIVA (TEMA ATO 3 - VERMELHO ALERTA) --- */
static const SDL_Color COLOR_BG_RED     = {15, 3, 5, 255};     /* #0F0305 */
static const SDL_Color COLOR_ALERT_RED  = {255, 51, 102, 255}; /* #FF3366 */
static const SDL_Color COLOR_DARK_RED   = {70, 12, 22, 255};   /* Vermelho escuro / trilhas */

/* --- ESTADOS DO JOGO (EXPANDIDO) --- */
typedef enum {
    STATE_MENU,
    STATE_QUIZ_ATO1,
    STATE_MINIGAME_LOCK,
    STATE_ATO2_TICKETS,
    STATE_ATO3_ESCANDALO,
    STATE_ATO4_DESFECHO
} GameState;

/* ============================================================================
 * ESTRUTURAS DE DADOS
 * ============================================================================ */

/* --- Banco de perguntas de logica para computacao (Ato 1) --- */
typedef struct {
    char question[220];
    char hint[220];
    char options[4][160];
    int  correct_option;      /* indice 0-3 */
    char explanation[520];    /* tabela-verdade / regra de inferencia formal */
} Question;

/* --- No de pais usado no Minigame do Cadeado Logico --- */
typedef struct {
    char name[16];
    bool status;   /* true = VERIFICADO, false = BLOQUEADO */
} CountryNode;

/* --- Chamado (ticket) do Ato 2 --- */
typedef struct {
    char id[8];
    char description[260];
    char category_options[3][48];
    int  correct_category;    /* indice 0-2 */
    bool resolved;
    bool player_was_correct;
} Ticket;

/* --- Texto flutuante de combo/feedback ("Game Juice") --- */
typedef struct {
    char text[48];
    float x, y;
    float vy;          /* velocidade vertical (sobe = negativo), px/s */
    float life_s;
    float max_life_s;
    SDL_Color color;
    bool active;
} FloatingText;

/* --- ESTRUTURA PRINCIPAL DO JOGO (EXPANDIDA) --- */
typedef struct {
    /* Navegacao geral */
    GameState state;
    bool running;

    /* Menu */
    int selected_menu_option;

    /* Ato 1 - Quiz de logica */
    int  current_question_index;
    int  selected_quiz_option;
    bool showing_explanation;
    char explanation_buffer[520];

    /* Progresso / Pontuacao / "Game Juice" */
    int   score;
    float public_trust;     /* valor atual (0-100), interpolado suavemente */
    float target_trust;     /* valor alvo (0-100) */
    int   shake_timer;      /* ms restantes de screen shake */
    int   streak_counter;   /* acertos consecutivos */

    /* Efeito de maquina de escrever (typewriter), usado nos textos
       narrativos do Ato 3, Ato 4 e nas explicacoes do Ato 1 */
    int   typewriter_index;
    float typewriter_accumulator;

    /* Minigame do Cadeado Logico */
    CountryNode countries[NUM_COUNTRIES];
    int   minigame_selected_index;
    float minigame_time_remaining;
    int   minigame_puzzle_index;

    /* Ato 2 - Chamados (split screen) */
    Ticket tickets[NUM_TICKETS];
    int  current_ticket_index;
    int  selected_ticket_category;

    /* Ato 3 - Escandalo */
    int  selected_scandal_option;
    bool scandal_resolved;

    /* Ato 4 - Desfecho */
    bool ato4_files_written; /* garante que so gravamos os arquivos uma vez */

    /* Textos flutuantes ativos */
    FloatingText floating_texts[MAX_FLOATING_TEXTS];
} GameEngine;

/* ============================================================================
 * BANCO DE PERGUNTAS (ATO 1) - LOGICA PARA COMPUTACAO + CONCEITOS DE IA
 * ============================================================================ */
static Question quiz_bank[QUIZ_BANK_SIZE] = {
    {
        "O que significa \"vies algoritmico\"?",
        "Pense em como os dados de treino influenciam a decisao final.",
        {
            "Um erro de digitacao no codigo da IA",
            "Tendencia sistematica causada por dados ou regras de treino desbalanceados",
            "Uma falha de hardware do servidor",
            "Uma opiniao pessoal do programador"
        },
        1,
        "Vies algoritmico e uma tendencia sistematica e repetivel de erro, "
        "originada quando os dados (ou as regras) de treino nao representam "
        "de forma justa a populacao real. Nao e um bug pontual: e um padrao "
        "que se repete a cada nova decisao do sistema."
    },
    {
        "Um chatbot afirma, com total confianca, um fato historico que nunca aconteceu. Como se chama esse fenomeno?",
        "O sistema nao esta mentindo de proposito: ele esta \"inventando\" com confianca.",
        {
            "Alucinacao",
            "Overfitting",
            "Vies de amostragem",
            "Criptografia"
        },
        0,
        "Alucinacao ocorre quando um modelo gera uma informacao plausivel, "
        "porem falsa, apresentando-a como se fosse um fato verificado. O "
        "modelo nao \"sabe\" que esta errado: ele apenas prediz a sequencia "
        "de palavras mais provavel, sem checagem externa de veracidade."
    },
    {
        "Um modelo acerta 99% dos dados de treino, mas erra quase metade dos dados novos. Isso e chamado de:",
        "O modelo \"decorou\" em vez de \"aprender\" a generalizar.",
        {
            "Overfitting (sobreajuste)",
            "Responsabilidade algoritmica",
            "Bicondicional",
            "Modus Tollens"
        },
        0,
        "Overfitting acontece quando o modelo se ajusta excessivamente ao "
        "conjunto de treino - inclusive ao ruido - e perde a capacidade de "
        "generalizar para dados nunca vistos. Alta performance no treino e "
        "baixa performance em producao e o sintoma classico."
    },
    {
        "Uma empresa culpa apenas \"a IA\" por um acidente, sem investigar decisoes humanas de projeto. Isso fere o principio de:",
        "Quem projeta, treina e implanta o sistema continua sendo responsavel por ele.",
        {
            "Responsabilidade algoritmica",
            "Disjuncao logica",
            "Tautologia",
            "Contingencia"
        },
        0,
        "Responsabilidade algoritmica e o principio de que pessoas e "
        "organizacoes continuam responsaveis pelas decisoes de sistemas que "
        "constroem e operam. \"A IA decidiu sozinha\" nao exime a equipe "
        "humana de supervisao, auditoria e correcao."
    },
    {
        "Qual o valor logico de V ^ F (Verdadeiro CONJUNCAO Falso)?",
        "A conjuncao (^) so e verdadeira quando AMBOS os lados sao verdadeiros.",
        {
            "Verdadeiro",
            "Falso",
            "Indeterminado",
            "Depende do contexto"
        },
        1,
        "Tabela-verdade da conjuncao (P ^ Q): so resulta em V quando P e Q "
        "sao ambos V. Nos demais casos (V^F, F^V, F^F) o resultado e F. "
        "Como um dos operandos aqui e Falso, o resultado e Falso."
    },
    {
        "Qual o valor logico de F v V (Falso DISJUNCAO Verdadeiro)?",
        "A disjuncao (v) so e falsa quando AMBOS os lados sao falsos.",
        {
            "Falso",
            "Indeterminado",
            "Verdadeiro",
            "Nulo"
        },
        2,
        "Tabela-verdade da disjuncao (P v Q): so resulta em F quando P e Q "
        "sao ambos F. Em qualquer outro caso, basta um dos lados ser V para "
        "o resultado ser V. Aqui, V v F = V."
    },
    {
        "No XOR (OU exclusivo, simbolo +), qual o resultado de V + V?",
        "O XOR e verdadeiro somente quando os dois lados SAO DIFERENTES entre si.",
        {
            "Verdadeiro",
            "Falso",
            "Verdadeiro apenas se ambos forem falsos",
            "Depende da ordem dos operandos"
        },
        1,
        "O XOR (P + Q) resulta em V apenas quando P e Q tem valores "
        "DIFERENTES (V+F ou F+V). Quando os dois sao iguais (V+V ou F+F), "
        "o resultado e F. Por isso V + V = F."
    },
    {
        "Considere: \"Se chove (P), entao a rua fica molhada (Q)\". Sabendo que P e verdadeiro, o que se pode concluir por MODUS PONENS?",
        "Modus Ponens: de (P -> Q) e P, conclui-se Q.",
        {
            "Nao se pode concluir nada",
            "Q e verdadeiro: a rua fica molhada",
            "P e falso",
            "Q e falso"
        },
        1,
        "Modus Ponens e a regra de inferencia: a partir de (P -> Q) e da "
        "premissa P (verdadeira), conclui-se Q. Formalmente: [(P->Q) ^ P] -> Q "
        "e uma tautologia. Como \"chove\" e verdadeiro, \"a rua fica molhada\" "
        "tambem deve ser verdadeiro."
    },
    {
        "Ainda em \"Se chove (P), entao a rua fica molhada (Q)\": a rua NAO esta molhada (~Q). O que se conclui pela CONTRAPOSITIVA / MODUS TOLLENS?",
        "Contrapositiva: (P -> Q) equivale a (~Q -> ~P).",
        {
            "Nao chove: ~P e verdadeiro",
            "Chove: P e verdadeiro",
            "A pergunta nao tem solucao logica",
            "P e Q sao independentes"
        },
        0,
        "Modus Tollens usa a equivalencia (P -> Q) === (~Q -> ~P), chamada "
        "contrapositiva. Se a rua nao esta molhada (~Q e verdadeiro), entao, "
        "por contrapositiva, conclui-se ~P: nao esta chovendo."
    },
    {
        "Pelas Leis de De Morgan, a negacao ~(P ^ Q) e logicamente equivalente a:",
        "De Morgan \"distribui\" a negacao e troca o conectivo.",
        {
            "~P ^ ~Q",
            "P v Q",
            "~P v ~Q",
            "P ^ Q"
        },
        2,
        "Leis de De Morgan: ~(P ^ Q) === ~P v ~Q, e simetricamente "
        "~(P v Q) === ~P ^ ~Q. Ao negar uma conjuncao, nega-se cada termo "
        "e troca-se ^ por v (e vice-versa)."
    }
};

/* ============================================================================
 * MINIGAME DO CADEADO LOGICO - PUZZLES (usa Conjuncao, Disjuncao, Negacao,
 * Condicional, Bicondicional e uma variacao inspirada nas Leis de De Morgan)
 * ============================================================================ */
typedef bool (*PuzzleEvalFn)(bool bra, bool usa, bool deu, bool jpn);

static bool evaluate_puzzle_0(bool bra, bool usa, bool deu, bool jpn) {
    /* (BRA ^ USA) v (~DEU ^ JPN) */
    return (bra && usa) || (!deu && jpn);
}
static bool evaluate_puzzle_1(bool bra, bool usa, bool deu, bool jpn) {
    /* (BRA <-> USA) -> (DEU v JPN) */
    bool antecedente = (bra == usa);   /* Bicondicional BRA <-> USA */
    bool consequente = (deu || jpn);   /* Disjuncao DEU v JPN       */
    return (!antecedente) || consequente; /* Condicional: A -> C */
}
static bool evaluate_puzzle_2(bool bra, bool usa, bool deu, bool jpn) {
    /* ~(BRA ^ USA) <-> (~BRA v ~JPN) */
    (void)deu; /* nao utilizado neste puzzle */
    bool lado_esquerdo = !(bra && usa);
    bool lado_direito  = (!bra || !jpn);
    return lado_esquerdo == lado_direito;
}

static const char *puzzle_formulas[3] = {
    "(BRA ^ USA) v (~DEU ^ JPN)",
    "(BRA <-> USA) -> (DEU v JPN)",
    "~(BRA ^ USA) <-> (~BRA v ~JPN)"
};
static const char *puzzle_concepts[3] = {
    "Conceitos: Conjuncao (^), Disjuncao (v) e Negacao (~)",
    "Conceitos: Bicondicional (<->) e Condicional (->)",
    "Conceitos: Negacao e Disjuncao combinadas (inspirado em De Morgan)"
};
static const PuzzleEvalFn puzzle_evaluators[3] = {
    evaluate_puzzle_0, evaluate_puzzle_1, evaluate_puzzle_2
};

/* ============================================================================
 * TEXTOS NARRATIVOS (usados com efeito typewriter)
 * ============================================================================ */
static const char *ATO3_NARRATIVE_TEXT =
    "ALERTA: uma reportagem investigativa revelou falhas graves de vies e "
    "alucinacao no sistema REV.IA em producao. A confianca publica esta em "
    "queda livre. Como o departamento deve responder a crise?";

static const char *ATO4_NARRATIVE_TEXT =
    "Missao concluida. O relatorio final de alfabetizacao algoritmica foi "
    "compilado e um certificado de conclusao foi gravado em disco.";

/* ============================================================================
 * FUNCOES AUXILIARES NUMERICAS
 * ============================================================================ */
static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* --- HELPER DE RENDERIZACAO DE RETANGULOS COM BORDA (PRESERVADA) --- */
void draw_box(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; i++) {
        SDL_Rect r = {rect.x + i, rect.y + i, rect.w - (i * 2), rect.h - (i * 2)};
        SDL_RenderDrawRect(renderer, &r);
    }
}

/* --- RENDERIZACAO DE TEXTO COM SDL_TTF (PRESERVADA) --- */
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!text || strlen(text) == 0) return;
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

/* --- VARIANTE DE render_text COM CANAL ALFA (usada nos floating texts) ---
 * Nao substitui render_text (que e preservada intacta); e apenas uma
 * extensao necessaria para o efeito de "Combo Float Text" desaparecer
 * gradualmente. Libera surface/texture da mesma forma (zero memory leak). */
void render_text_alpha(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color, Uint8 alpha) {
    if (!text || strlen(text) == 0) return;
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

/* --- RENDERIZACAO DE TEXTO COM EFEITO TYPEWRITER ---
 * Desenha apenas os primeiros `reveal_count` caracteres de `full_text`. */
void render_text_typewriter(SDL_Renderer *renderer, TTF_Font *font, const char *full_text, int x, int y, SDL_Color color, int reveal_count) {
    int len = (int)strlen(full_text);
    if (reveal_count > len) reveal_count = len;
    if (reveal_count < 0) reveal_count = 0;

    char buffer[600];
    if (reveal_count >= (int)sizeof(buffer)) reveal_count = (int)sizeof(buffer) - 1;
    memcpy(buffer, full_text, (size_t)reveal_count);
    buffer[reveal_count] = '\0';
    render_text(renderer, font, buffer, x, y, color);
}

/* --- RENDERIZACAO DE BOTOES ESTILIZADOS (PRESERVADA E ESTENDIDA) ---
 * Estendida com parametros de cor de tema (primary_color / text_on_primary)
 * para permitir a troca dinamica de paleta exigida no Ato 3. O uso normal
 * (Menu, Ato 1, Ato 2, Ato 4) continua passando COLOR_NEON / COLOR_TEXT_BLK,
 * preservando o visual e o comportamento originais. */
void render_button(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, int w, int h, bool selected, SDL_Color primary_color, SDL_Color text_on_primary_color) {
    SDL_Rect rect = {x, y, w, h};
    if (selected) {
        SDL_SetRenderDrawColor(renderer, primary_color.r, primary_color.g, primary_color.b, primary_color.a);
        SDL_RenderFillRect(renderer, &rect);
        render_text(renderer, font, text, x + (w / 2) - ((int)strlen(text) * 4), y + (h / 2) - 10, text_on_primary_color);
    } else {
        draw_box(renderer, rect, primary_color, 1);
        render_text(renderer, font, text, x + (w / 2) - ((int)strlen(text) * 4), y + (h / 2) - 10, primary_color);
    }
}

/* --- BARRA DE PROGRESSO RETRO (PRESERVADA E ESTENDIDA) ---
 * Estendida com cor de preenchimento e de borda para suportar o tema do
 * Ato 3, e com clamp de seguranca em `progress`. */
void render_progress_bar(SDL_Renderer *renderer, int x, int y, int w, int h, float progress, SDL_Color fill_color, SDL_Color border_color) {
    progress = clampf(progress, 0.0f, 1.0f);
    SDL_Rect outer = {x, y, w, h};
    draw_box(renderer, outer, border_color, 1);

    SDL_Rect inner = {x + 2, y + 2, (int)((w - 4) * progress), h - 4};
    SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    SDL_RenderFillRect(renderer, &inner);
}

/* ============================================================================
 * FUNCOES DE "GAME JUICE"
 * ============================================================================ */

/* --- SCREEN SHAKE: aplica offsets aleatorios de renderizacao --- */
void apply_screen_shake(int *out_x, int *out_y, int shake_timer) {
    if (shake_timer > 0) {
        *out_x = (rand() % 9) - 4;  /* -4 .. +4 px */
        *out_y = (rand() % 9) - 4;
    } else {
        *out_x = 0;
        *out_y = 0;
    }
}

void trigger_screen_shake(GameEngine *engine, int duration_ms) {
    engine->shake_timer = duration_ms;
}

/* --- INTERPOLACAO SUAVE DA BARRA DE CONFIANCA ATE O VALOR ALVO --- */
void update_trust_interpolation(GameEngine *engine, float dt) {
    float diff = engine->target_trust - engine->public_trust;
    if (fabsf(diff) < 0.05f) {
        engine->public_trust = engine->target_trust;
        return;
    }
    engine->public_trust += diff * fminf(1.0f, dt * 4.0f);
}

/* --- COMBO FLOAT TEXT: spawn / update / render --- */
void spawn_floating_text(GameEngine *engine, const char *text, int x, int y, SDL_Color color) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (!engine->floating_texts[i].active) {
            FloatingText *ft = &engine->floating_texts[i];
            strncpy(ft->text, text, sizeof(ft->text) - 1);
            ft->text[sizeof(ft->text) - 1] = '\0';
            ft->x = (float)x;
            ft->y = (float)y;
            ft->vy = -35.0f;
            ft->max_life_s = 1.4f;
            ft->life_s = ft->max_life_s;
            ft->color = color;
            ft->active = true;
            return;
        }
    }
    /* Sem slots livres: ignora silenciosamente (nao critico para o jogo) */
}

void update_floating_texts(GameEngine *engine, float dt) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        FloatingText *ft = &engine->floating_texts[i];
        if (!ft->active) continue;
        ft->y += ft->vy * dt;
        ft->life_s -= dt;
        if (ft->life_s <= 0.0f) ft->active = false;
    }
}

void render_floating_texts(SDL_Renderer *renderer, TTF_Font *font, GameEngine *engine) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        FloatingText *ft = &engine->floating_texts[i];
        if (!ft->active) continue;
        float life_fraction = clampf(ft->life_s / ft->max_life_s, 0.0f, 1.0f);
        Uint8 alpha = (Uint8)(life_fraction * 255.0f);
        render_text_alpha(renderer, font, ft->text, (int)ft->x, (int)ft->y, ft->color, alpha);
    }
}

/* ============================================================================
 * PERSISTENCIA: save.dat E certificado_conclusao.txt
 * ============================================================================ */
typedef struct {
    int   score;
    int   act_reached;
    float public_trust;
} SaveData;

bool save_game(const GameEngine *engine) {
    FILE *f = fopen("save.dat", "wb");
    if (!f) return false;
    SaveData data;
    data.score = engine->score;
    data.act_reached = (int)engine->state;
    data.public_trust = engine->public_trust;
    size_t written = fwrite(&data, sizeof(SaveData), 1, f);
    fclose(f);
    return written == 1;
}

bool load_game(GameEngine *engine) {
    FILE *f = fopen("save.dat", "rb");
    if (!f) return false;
    SaveData data;
    size_t read_count = fread(&data, sizeof(SaveData), 1, f);
    fclose(f);
    if (read_count != 1) return false;

    engine->score = data.score;
    engine->state = (GameState)data.act_reached;
    engine->public_trust = data.public_trust;
    engine->target_trust = data.public_trust;
    return true;
}

void write_certificado(const GameEngine *engine) {
    FILE *f = fopen("certificado_conclusao.txt", "w");
    if (!f) return;

    const char *classificacao;
    if (engine->public_trust >= 80.0f)      classificacao = "IA CONFIAVEL E RESPONSAVEL";
    else if (engine->public_trust >= 50.0f) classificacao = "IA EM RECUPERACAO DE CONFIANCA";
    else                                     classificacao = "IA EM COLAPSO DE CONFIANCA PUBLICA";

    fprintf(f, "================================================\n");
    fprintf(f, " CERTIFICADO DE CONCLUSAO -- DECIFRA.IA / REV.IA \n");
    fprintf(f, "================================================\n\n");
    fprintf(f, "Pontuacao final (Ato 1 - Quiz de Logica): %d / %d\n", engine->score, QUIZ_BANK_SIZE);
    fprintf(f, "Indice de confianca publica final: %.1f / 100\n", engine->public_trust);
    fprintf(f, "Classificacao final: %s\n\n", classificacao);
    fprintf(f, "Resumo da missao:\n");
    fprintf(f, "- Atos concluidos: 1 (Onboarding), 2 (Chamados), 3 (Escandalo), 4 (Desfecho)\n");
    fprintf(f, "- Conceitos avaliados: vies algoritmico, alucinacao, overfitting,\n");
    fprintf(f, "  responsabilidade algoritmica, conectivos logicos (conjuncao,\n");
    fprintf(f, "  disjuncao, XOR, condicional, bicondicional), Modus Ponens,\n");
    fprintf(f, "  Modus Tollens / contrapositiva e Leis de De Morgan.\n\n");
    fprintf(f, "Departamento de Alfabetizacao Algoritmica -- Projeto Rev.IA\n");
    fprintf(f, "CESAR School -- Projetos 2\n");

    fclose(f);
}

/* ============================================================================
 * TELA 1: MENU INICIAL (PRESERVADA)
 * ============================================================================ */
void render_screen_menu(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    SDL_Rect header_box = {220, 60, 580, 100};
    draw_box(renderer, header_box, COLOR_NEON, 2);
    render_text(renderer, font_title, "REV.IA", 440, 75, COLOR_NEON);
    render_text(renderer, font_ui, "- - -  DECIFRA.IA  - - -", 390, 125, COLOR_NEON);

    render_text(renderer, font_ui, "um jogo de terminal sobre alfabetizacao em Inteligencia Artificial", 230, 180, COLOR_NEON);
    render_text(renderer, font_ui, "Departamento de Alfabetizacao Algoritmica - projeto Rev.IA", 280, 205, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 50, 240, 970, 240);

    const char *options[] = {"[1] Iniciar Jogo", "[2] Continuar", "[3] Configuracoes", "[4] Creditos", "[Q] Sair"};
    for (int i = 0; i < 5; i++) {
        render_button(renderer, font_ui, options[i], 360, 270 + (i * 55), 300, 45,
                       (engine->selected_menu_option == i), COLOR_NEON, COLOR_TEXT_BLK);
    }

    render_text(renderer, font_ui, "v0.2 - Rev.IA / Decifra.IA - CESAR School, Projetos 2", 40, 585, COLOR_NEON);
    render_text(renderer, font_ui, "> _", 940, 585, COLOR_NEON);
}

/* ============================================================================
 * TELA 2: QUIZ / ATO 1 ONBOARDING (AGORA DIRIGIDA POR quiz_bank)
 * ============================================================================ */
void render_screen_quiz(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "ATO 1 -- ONBOARDING", 40, 35, COLOR_NEON);

    char subtitle[64];
    snprintf(subtitle, sizeof(subtitle), "Manual de Treinamento -- Missao %d de %d",
             engine->current_question_index + 1, QUIZ_BANK_SIZE);
    render_text(renderer, font_ui, subtitle, 40, 70, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 95, 980, 95);

    render_text(renderer, font_ui, "Progresso do Ato 1:", 40, 110, COLOR_NEON);
    float progress = (float)engine->current_question_index / (float)QUIZ_BANK_SIZE;
    render_progress_bar(renderer, 220, 112, 450, 18, progress, COLOR_NEON, COLOR_DARK_GRN);

    if (engine->showing_explanation) {
        /* --- Painel de explicacao (exibido apos uma resposta errada) --- */
        SDL_Rect box = {40, 150, 940, 340};
        draw_box(renderer, box, COLOR_YELLOW, 2);
        render_text(renderer, font_ui, "RESPOSTA INCORRETA -- REVISE O CONCEITO:", 60, 165, COLOR_YELLOW);
        render_text_typewriter(renderer, font_ui, engine->explanation_buffer, 60, 200, COLOR_NEON, engine->typewriter_index);
        render_text(renderer, font_ui, "O Ato 1 sera reiniciado. Pressione ENTER para continuar_", 60, 460, COLOR_YELLOW);
        return;
    }

    Question *q = &quiz_bank[engine->current_question_index];

    SDL_Rect question_box = {40, 150, 940, 80};
    draw_box(renderer, question_box, COLOR_DARK_GRN, 1);
    char question_line[256];
    snprintf(question_line, sizeof(question_line), "Pergunta: %s", q->question);
    render_text(renderer, font_ui, question_line, 60, 165, COLOR_NEON);
    char hint_line[256];
    snprintf(hint_line, sizeof(hint_line), "(dica: %s)", q->hint);
    render_text(renderer, font_ui, hint_line, 60, 195, COLOR_NEON);

    for (int i = 0; i < 4; i++) {
        SDL_Rect opt_rect = {40, 250 + (i * 60), 940, 48};
        bool is_selected = (engine->selected_quiz_option == i);
        char label[200];
        snprintf(label, sizeof(label), "%c) %s", 'A' + i, q->options[i]);

        if (is_selected) {
            SDL_SetRenderDrawColor(renderer, COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, 255);
            SDL_RenderFillRect(renderer, &opt_rect);
            render_text(renderer, font_ui, label, 60, 264 + (i * 60), COLOR_TEXT_BLK);
        } else {
            draw_box(renderer, opt_rect, COLOR_DARK_GRN, 1);
            render_text(renderer, font_ui, label, 60, 264 + (i * 60), COLOR_NEON);
        }
    }

    render_text(renderer, font_ui, "Capacitacao obrigatoria -- Departamento de Alfabetizacao Algoritmica", 40, 520, COLOR_YELLOW);
    char score_line[64];
    snprintf(score_line, sizeof(score_line), "Acertos: %d/%d  |  Streak atual: %d", engine->score, QUIZ_BANK_SIZE, engine->streak_counter);
    render_text(renderer, font_ui, score_line, 40, 550, COLOR_NEON);
    render_text(renderer, font_ui, "> selecione uma opcao e pressione ENTER_", 40, 575, COLOR_NEON);
}

/* ============================================================================
 * TELA 3: MINIGAME DO CADEADO LOGICO
 * ============================================================================ */
void render_screen_minigame_lock(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "CADEADO LOGICO -- ACESSO RESTRITO", 40, 35, COLOR_NEON);
    render_text(renderer, font_ui, "Ajuste o status dos servidores para satisfazer a formula abaixo", 40, 70, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 95, 980, 95);

    /* Formula e conceito envolvido */
    SDL_Rect formula_box = {40, 115, 940, 70};
    draw_box(renderer, formula_box, COLOR_DARK_GRN, 1);
    char formula_line[128];
    snprintf(formula_line, sizeof(formula_line), "Formula: %s", puzzle_formulas[engine->minigame_puzzle_index]);
    render_text(renderer, font_ui, formula_line, 60, 128, COLOR_NEON);
    render_text(renderer, font_ui, puzzle_concepts[engine->minigame_puzzle_index], 60, 155, COLOR_NEON);

    /* Cronometro */
    render_text(renderer, font_ui, "Tempo restante:", 40, 200, COLOR_YELLOW);
    float time_progress = clampf(engine->minigame_time_remaining / MINIGAME_TIME_LIMIT, 0.0f, 1.0f);
    render_progress_bar(renderer, 220, 202, 450, 18, time_progress, COLOR_YELLOW, COLOR_DARK_GRN);
    char time_label[32];
    snprintf(time_label, sizeof(time_label), "%.1fs", engine->minigame_time_remaining > 0.0f ? engine->minigame_time_remaining : 0.0f);
    render_text(renderer, font_ui, time_label, 690, 202, COLOR_YELLOW);

    /* Lista de servidores/paises com status togglable */
    for (int i = 0; i < NUM_COUNTRIES; i++) {
        SDL_Rect row = {40, 250 + (i * 55), 940, 45};
        bool is_selected = (engine->minigame_selected_index == i);

        char status_text[64];
        snprintf(status_text, sizeof(status_text), "[%s] %s -- status: %s",
                 is_selected ? "*" : " ",
                 engine->countries[i].name,
                 engine->countries[i].status ? "VERIFICADO (V)" : "BLOQUEADO (F)");

        SDL_Color row_color = engine->countries[i].status ? COLOR_NEON : COLOR_DARK_GRN;
        if (is_selected) {
            SDL_SetRenderDrawColor(renderer, row_color.r, row_color.g, row_color.b, 255);
            SDL_RenderFillRect(renderer, &row);
            render_text(renderer, font_ui, status_text, 60, 262 + (i * 55), COLOR_TEXT_BLK);
        } else {
            draw_box(renderer, row, row_color, 1);
            render_text(renderer, font_ui, status_text, 60, 262 + (i * 55), row_color);
        }
    }

    render_text(renderer, font_ui, "> SETAS para navegar, ESPACO para alternar V/F, ENTER para tentar destravar_", 40, 520, COLOR_NEON);
    render_text(renderer, font_ui, "Falhar ou zerar o tempo custa confianca publica -- capriche na logica.", 40, 550, COLOR_YELLOW);
}

/* ============================================================================
 * TELA 4: ATO 2 -- ROTINA DE CHAMADOS EM SPLIT SCREEN
 * ============================================================================ */
void render_screen_ato2_tickets(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "ATO 2 -- FILA DE CHAMADOS", 40, 35, COLOR_NEON);
    render_text(renderer, font_ui, "Classifique cada chamado de acordo com o conceito de IA envolvido", 40, 70, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 95, 980, 95);

    /* --- PAINEL ESQUERDO: fila de chamados --- */
    SDL_Rect left_panel = {40, 115, 440, 460};
    draw_box(renderer, left_panel, COLOR_DARK_GRN, 1);
    render_text(renderer, font_ui, "FILA DE CHAMADOS:", 55, 125, COLOR_NEON);

    for (int i = 0; i < NUM_TICKETS; i++) {
        bool is_current = (i == engine->current_ticket_index);
        SDL_Color line_color = COLOR_NEON;
        char status_tag[16] = "PENDENTE";
        if (engine->tickets[i].resolved) {
            line_color = engine->tickets[i].player_was_correct ? COLOR_NEON : COLOR_YELLOW;
            snprintf(status_tag, sizeof(status_tag), "%s", engine->tickets[i].player_was_correct ? "OK" : "ERRO");
        }
        char line[96];
        snprintf(line, sizeof(line), "%s %s -- %s", is_current ? ">" : " ", engine->tickets[i].id, status_tag);
        render_text(renderer, font_ui, line, 60, 160 + (i * 32), line_color);
    }

    /* --- PAINEL DIREITO: detalhe do chamado atual --- */
    SDL_Rect right_panel = {500, 115, 480, 460};
    draw_box(renderer, right_panel, COLOR_DARK_GRN, 1);

    if (engine->current_ticket_index < NUM_TICKETS) {
        Ticket *t = &engine->tickets[engine->current_ticket_index];
        char header[32];
        snprintf(header, sizeof(header), "CHAMADO %s", t->id);
        render_text(renderer, font_ui, header, 515, 125, COLOR_NEON);

        render_text_typewriter(renderer, font_ui, t->description, 515, 155, COLOR_NEON, engine->typewriter_index);

        render_text(renderer, font_ui, "Classifique este chamado:", 515, 300, COLOR_YELLOW);
        for (int i = 0; i < 3; i++) {
            SDL_Rect opt_rect = {515, 330 + (i * 55), 450, 45};
            bool is_selected = (engine->selected_ticket_category == i);
            char label[64];
            snprintf(label, sizeof(label), "%c) %s", 'A' + i, t->category_options[i]);

            if (is_selected) {
                SDL_SetRenderDrawColor(renderer, COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, 255);
                SDL_RenderFillRect(renderer, &opt_rect);
                render_text(renderer, font_ui, label, 525, 343 + (i * 55), COLOR_TEXT_BLK);
            } else {
                draw_box(renderer, opt_rect, COLOR_DARK_GRN, 1);
                render_text(renderer, font_ui, label, 525, 343 + (i * 55), COLOR_NEON);
            }
        }
    } else {
        render_text(renderer, font_ui, "Todos os chamados foram processados.", 515, 200, COLOR_NEON);
    }

    render_text(renderer, font_ui, "> SETAS para escolher, ENTER para confirmar a classificacao_", 40, 585, COLOR_NEON);
}

/* ============================================================================
 * TELA 5: ATO 3 -- ESCANDALO (TEMA VERMELHO ALERTA)
 * ============================================================================ */
void render_screen_ato3_escandalo(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_ALERT_RED, 3);

    render_text(renderer, font_title, "ATO 3 -- ESCANDALO PUBLICO", 40, 35, COLOR_ALERT_RED);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_RED.r, COLOR_DARK_RED.g, COLOR_DARK_RED.b, 255);
    SDL_RenderDrawLine(renderer, 40, 80, 980, 80);

    SDL_Rect narrative_box = {40, 100, 940, 130};
    draw_box(renderer, narrative_box, COLOR_DARK_RED, 2);
    render_text_typewriter(renderer, font_ui, ATO3_NARRATIVE_TEXT, 60, 120, COLOR_ALERT_RED, engine->typewriter_index);

    render_text(renderer, font_ui, "Confianca publica atual:", 40, 250, COLOR_ALERT_RED);
    render_progress_bar(renderer, 280, 252, 400, 18, engine->public_trust / 100.0f, COLOR_ALERT_RED, COLOR_DARK_RED);

    bool narrative_fully_revealed = (engine->typewriter_index >= (int)strlen(ATO3_NARRATIVE_TEXT));

    if (narrative_fully_revealed) {
        const char *decisions[] = {
            "A) Negar publicamente qualquer falha do sistema",
            "B) Admitir o erro, publicar auditoria e corrigir o modelo",
            "C) Transferir a culpa para a equipe de dados terceirizada"
        };
        render_text(renderer, font_ui, "Decisao do departamento:", 40, 300, COLOR_YELLOW);
        for (int i = 0; i < 3; i++) {
            SDL_Rect opt_rect = {40, 335 + (i * 55), 940, 45};
            bool is_selected = (engine->selected_scandal_option == i);
            if (is_selected) {
                SDL_SetRenderDrawColor(renderer, COLOR_ALERT_RED.r, COLOR_ALERT_RED.g, COLOR_ALERT_RED.b, 255);
                SDL_RenderFillRect(renderer, &opt_rect);
                render_text(renderer, font_ui, decisions[i], 60, 348 + (i * 55), COLOR_BG_RED);
            } else {
                draw_box(renderer, opt_rect, COLOR_DARK_RED, 1);
                render_text(renderer, font_ui, decisions[i], 60, 348 + (i * 55), COLOR_ALERT_RED);
            }
        }
        render_text(renderer, font_ui, "> SETAS para escolher, ENTER para confirmar a decisao_", 40, 585, COLOR_ALERT_RED);
    } else {
        render_text(renderer, font_ui, "> _", 40, 585, COLOR_ALERT_RED);
    }
}

/* ============================================================================
 * TELA 6: ATO 4 -- DESFECHO E RELATORIO FINAL
 * ============================================================================ */
void render_screen_ato4_desfecho(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "ATO 4 -- DESFECHO", 40, 35, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 80, 980, 80);

    SDL_Rect narrative_box = {40, 100, 940, 90};
    draw_box(renderer, narrative_box, COLOR_DARK_GRN, 1);
    render_text_typewriter(renderer, font_ui, ATO4_NARRATIVE_TEXT, 60, 120, COLOR_NEON, engine->typewriter_index);

    SDL_Rect report_box = {40, 210, 940, 240};
    draw_box(renderer, report_box, COLOR_DARK_GRN, 1);
    render_text(renderer, font_ui, "RELATORIO FINAL:", 60, 225, COLOR_YELLOW);

    char line1[96];
    snprintf(line1, sizeof(line1), "Pontuacao do Quiz de Logica (Ato 1): %d / %d", engine->score, QUIZ_BANK_SIZE);
    render_text(renderer, font_ui, line1, 60, 260, COLOR_NEON);

    char line2[96];
    snprintf(line2, sizeof(line2), "Indice de confianca publica final: %.1f / 100", engine->public_trust);
    render_text(renderer, font_ui, line2, 60, 290, COLOR_NEON);
    render_progress_bar(renderer, 60, 315, 400, 18, engine->public_trust / 100.0f, COLOR_NEON, COLOR_DARK_GRN);

    const char *classificacao;
    if (engine->public_trust >= 80.0f)      classificacao = "IA CONFIAVEL E RESPONSAVEL";
    else if (engine->public_trust >= 50.0f) classificacao = "IA EM RECUPERACAO DE CONFIANCA";
    else                                     classificacao = "IA EM COLAPSO DE CONFIANCA PUBLICA";
    char line3[96];
    snprintf(line3, sizeof(line3), "Classificacao final: %s", classificacao);
    render_text(renderer, font_ui, line3, 60, 355, COLOR_YELLOW);

    render_text(renderer, font_ui, "Arquivos gravados: certificado_conclusao.txt e save.dat", 60, 400, COLOR_NEON);

    render_text(renderer, font_ui, "> Pressione ENTER para voltar ao menu, ou ESC para sair_", 40, 560, COLOR_NEON);
}

/* ============================================================================
 * INICIALIZACAO / RESET DE ESTADOS
 * ============================================================================ */
void reset_quiz_state(GameEngine *engine) {
    engine->current_question_index = 0;
    engine->selected_quiz_option = 0;
    engine->score = 0;
    engine->streak_counter = 0;
    engine->showing_explanation = false;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

void reset_minigame_state(GameEngine *engine) {
    const char *names[NUM_COUNTRIES] = {"BRA", "USA", "DEU", "JPN"};
    for (int i = 0; i < NUM_COUNTRIES; i++) {
        strncpy(engine->countries[i].name, names[i], sizeof(engine->countries[i].name) - 1);
        engine->countries[i].name[sizeof(engine->countries[i].name) - 1] = '\0';
        engine->countries[i].status = false;
    }
    engine->minigame_selected_index = 0;
    engine->minigame_time_remaining = MINIGAME_TIME_LIMIT;
    engine->minigame_puzzle_index = rand() % 3;
}

void reset_ato2_state(GameEngine *engine) {
    const char *ids[NUM_TICKETS] = {"#A17", "#A18", "#A19", "#A20", "#A21"};

    const char *descriptions[NUM_TICKETS] = {
        "O sistema recomendou 95% das vagas de programacao para candidatos "
        "do sexo masculino, mesmo com curriculos equivalentes.",
        "O chatbot afirmou com confianca que \"a Torre Eiffel foi construida "
        "em 1990, no Brasil\" -- informacao completamente falsa.",
        "O modelo teve 99% de acerto nos dados de treino, mas errou quase "
        "metade das previsoes em dados novos, nunca vistos antes.",
        "Um carro autonomo causou um acidente e a empresa alegou que \"a "
        "decisao foi da IA\", evitando qualquer responsabilizacao humana.",
        "Usuarios pediram para o sistema filtrar apenas curriculos \"com "
        "nomes familiares\"; o sistema recusou e sinalizou a solicitacao."
    };

    /* cada linha: {opcao0, opcao1, opcao2} + indice correto */
    const char *opts[NUM_TICKETS][3] = {
        {"Vies de amostragem nos dados de treino", "Falha de hardware", "Comportamento esperado do sistema"},
        {"Overfitting", "Alucinacao (invencao de fatos)", "Vies de amostragem"},
        {"Overfitting (decorou os dados de treino)", "Alucinacao", "Responsabilidade algoritmica"},
        {"Vies algoritmico", "Ausencia de responsabilidade algoritmica", "Overfitting"},
        {"Falha critica do sistema", "Vies induzido pelo usuario, corretamente recusado", "Alucinacao"}
    };
    const int correct[NUM_TICKETS] = {0, 1, 0, 1, 1};

    for (int i = 0; i < NUM_TICKETS; i++) {
        Ticket *t = &engine->tickets[i];
        strncpy(t->id, ids[i], sizeof(t->id) - 1); t->id[sizeof(t->id) - 1] = '\0';
        strncpy(t->description, descriptions[i], sizeof(t->description) - 1); t->description[sizeof(t->description) - 1] = '\0';
        for (int j = 0; j < 3; j++) {
            strncpy(t->category_options[j], opts[i][j], sizeof(t->category_options[j]) - 1);
            t->category_options[j][sizeof(t->category_options[j]) - 1] = '\0';
        }
        t->correct_category = correct[i];
        t->resolved = false;
        t->player_was_correct = false;
    }

    engine->current_ticket_index = 0;
    engine->selected_ticket_category = 0;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

void reset_ato3_state(GameEngine *engine) {
    engine->selected_scandal_option = 0;
    engine->scandal_resolved = false;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

void reset_ato4_state(GameEngine *engine) {
    engine->ato4_files_written = false;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

/* ============================================================================
 * LOOP PRINCIPAL & EVENTOS ENGINE
 * ============================================================================ */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        printf("Falha ao inicializar SDL2/TTF\n");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "decifra.exe -- terminal",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *font_ui = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
    TTF_Font *font_title = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 28);

    if (!font_ui) font_ui = TTF_OpenFont("C:\\Windows\\Fonts\\consola.ttf", 14);
    if (!font_title) font_title = TTF_OpenFont("C:\\Windows\\Fonts\\consolab.ttf", 28);

    GameEngine engine;
    memset(&engine, 0, sizeof(GameEngine));
    engine.state = STATE_MENU;
    engine.running = true;
    engine.selected_menu_option = 0;
    engine.public_trust = TRUST_INITIAL;
    engine.target_trust = TRUST_INITIAL;
    reset_quiz_state(&engine);

    Uint32 last_tick = SDL_GetTicks();

    SDL_Event e;
    while (engine.running) {
        Uint32 current_tick = SDL_GetTicks();
        float dt = (current_tick - last_tick) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f; /* evita "saltos" apos pausas longas */
        last_tick = current_tick;

        /* -------------------- EVENTOS -------------------- */
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) engine.running = false;

            if (e.type == SDL_KEYDOWN) {
                switch (engine.state) {

                case STATE_MENU:
                    if (e.key.keysym.sym == SDLK_UP) engine.selected_menu_option = (engine.selected_menu_option - 1 + 5) % 5;
                    if (e.key.keysym.sym == SDLK_DOWN) engine.selected_menu_option = (engine.selected_menu_option + 1) % 5;
                    if (e.key.keysym.sym == SDLK_RETURN) {
                        if (engine.selected_menu_option == 0) {
                            /* [1] Iniciar Jogo */
                            reset_quiz_state(&engine);
                            engine.public_trust = TRUST_INITIAL;
                            engine.target_trust = TRUST_INITIAL;
                            engine.state = STATE_QUIZ_ATO1;
                        } else if (engine.selected_menu_option == 1) {
                            /* [2] Continuar */
                            load_game(&engine); /* se falhar, permanece no menu */
                        } else if (engine.selected_menu_option == 4) {
                            /* [Q] Sair */
                            engine.running = false;
                        }
                    }
                    break;

                case STATE_QUIZ_ATO1:
                    if (engine.showing_explanation) {
                        if (e.key.keysym.sym == SDLK_RETURN) {
                            reset_quiz_state(&engine); /* reinicia o Ato 1 */
                        }
                    } else {
                        if (e.key.keysym.sym == SDLK_UP) engine.selected_quiz_option = (engine.selected_quiz_option - 1 + 4) % 4;
                        if (e.key.keysym.sym == SDLK_DOWN) engine.selected_quiz_option = (engine.selected_quiz_option + 1) % 4;
                        if (e.key.keysym.sym == SDLK_ESCAPE) engine.state = STATE_MENU;

                        if (e.key.keysym.sym == SDLK_RETURN) {
                            Question *q = &quiz_bank[engine.current_question_index];
                            if (engine.selected_quiz_option == q->correct_option) {
                                /* Acerto */
                                engine.score++;
                                engine.streak_counter++;
                                float bonus = 5.0f;
                                char combo_msg[64];
                                if (engine.streak_counter > 0 && engine.streak_counter % 3 == 0) {
                                    bonus = 10.0f;
                                    snprintf(combo_msg, sizeof(combo_msg), "%dx STREAK! +%d CONFIANCA", engine.streak_counter, (int)bonus);
                                } else {
                                    snprintf(combo_msg, sizeof(combo_msg), "+%d CONFIANCA", (int)bonus);
                                }
                                engine.target_trust = clampf(engine.target_trust + bonus, 0.0f, 100.0f);
                                spawn_floating_text(&engine, combo_msg, 700, 300, COLOR_NEON);

                                engine.current_question_index++;
                                engine.selected_quiz_option = 0;

                                if (engine.current_question_index >= QUIZ_BANK_SIZE) {
                                    reset_minigame_state(&engine);
                                    engine.state = STATE_MINIGAME_LOCK;
                                }
                            } else {
                                /* Erro: mostra explicacao formal e reinicia o Ato 1 ao confirmar */
                                engine.streak_counter = 0;
                                engine.target_trust = clampf(engine.target_trust - 5.0f, 0.0f, 100.0f);
                                trigger_screen_shake(&engine, 300);
                                spawn_floating_text(&engine, "-5 CONFIANCA", 700, 300, COLOR_YELLOW);

                                strncpy(engine.explanation_buffer, q->explanation, sizeof(engine.explanation_buffer) - 1);
                                engine.explanation_buffer[sizeof(engine.explanation_buffer) - 1] = '\0';
                                engine.showing_explanation = true;
                                engine.typewriter_index = 0;
                                engine.typewriter_accumulator = 0.0f;
                            }
                        }
                    }
                    break;

                case STATE_MINIGAME_LOCK:
                    if (e.key.keysym.sym == SDLK_UP) engine.minigame_selected_index = (engine.minigame_selected_index - 1 + NUM_COUNTRIES) % NUM_COUNTRIES;
                    if (e.key.keysym.sym == SDLK_DOWN) engine.minigame_selected_index = (engine.minigame_selected_index + 1) % NUM_COUNTRIES;
                    if (e.key.keysym.sym == SDLK_SPACE) {
                        engine.countries[engine.minigame_selected_index].status =
                            !engine.countries[engine.minigame_selected_index].status;
                    }
                    if (e.key.keysym.sym == SDLK_ESCAPE) engine.state = STATE_MENU;

                    if (e.key.keysym.sym == SDLK_RETURN) {
                        PuzzleEvalFn eval_fn = puzzle_evaluators[engine.minigame_puzzle_index];
                        bool solved = eval_fn(engine.countries[0].status, engine.countries[1].status,
                                               engine.countries[2].status, engine.countries[3].status);
                        if (solved) {
                            engine.target_trust = clampf(engine.target_trust + 10.0f, 0.0f, 100.0f);
                            spawn_floating_text(&engine, "ACESSO LIBERADO! +10 CONFIANCA", 640, 250, COLOR_NEON);
                            reset_ato2_state(&engine);
                            engine.state = STATE_ATO2_TICKETS;
                        } else {
                            engine.target_trust = clampf(engine.target_trust - 10.0f, 0.0f, 100.0f);
                            trigger_screen_shake(&engine, 400);
                            spawn_floating_text(&engine, "COMBINACAO INVALIDA -10", 640, 250, COLOR_YELLOW);
                        }
                    }
                    break;

                case STATE_ATO2_TICKETS:
                    if (engine.current_ticket_index < NUM_TICKETS) {
                        if (e.key.keysym.sym == SDLK_UP) engine.selected_ticket_category = (engine.selected_ticket_category - 1 + 3) % 3;
                        if (e.key.keysym.sym == SDLK_DOWN) engine.selected_ticket_category = (engine.selected_ticket_category + 1) % 3;

                        if (e.key.keysym.sym == SDLK_RETURN) {
                            Ticket *t = &engine.tickets[engine.current_ticket_index];
                            t->resolved = true;
                            t->player_was_correct = (engine.selected_ticket_category == t->correct_category);

                            if (t->player_was_correct) {
                                engine.streak_counter++;
                                engine.target_trust = clampf(engine.target_trust + 6.0f, 0.0f, 100.0f);
                                spawn_floating_text(&engine, "CHAMADO RESOLVIDO! +6 CONFIANCA", 700, 500, COLOR_NEON);
                            } else {
                                engine.streak_counter = 0;
                                engine.target_trust = clampf(engine.target_trust - 6.0f, 0.0f, 100.0f);
                                trigger_screen_shake(&engine, 300);
                                spawn_floating_text(&engine, "CLASSIFICACAO INCORRETA -6", 700, 500, COLOR_YELLOW);
                            }

                            engine.current_ticket_index++;
                            engine.selected_ticket_category = 0;
                            engine.typewriter_index = 0;
                            engine.typewriter_accumulator = 0.0f;

                            if (engine.current_ticket_index >= NUM_TICKETS) {
                                reset_ato3_state(&engine);
                                engine.state = STATE_ATO3_ESCANDALO;
                            }
                        }
                    }
                    if (e.key.keysym.sym == SDLK_ESCAPE) engine.state = STATE_MENU;
                    break;

                case STATE_ATO3_ESCANDALO: {
                    bool narrative_fully_revealed = (engine.typewriter_index >= (int)strlen(ATO3_NARRATIVE_TEXT));
                    if (narrative_fully_revealed && !engine.scandal_resolved) {
                        if (e.key.keysym.sym == SDLK_UP) engine.selected_scandal_option = (engine.selected_scandal_option - 1 + 3) % 3;
                        if (e.key.keysym.sym == SDLK_DOWN) engine.selected_scandal_option = (engine.selected_scandal_option + 1) % 3;

                        if (e.key.keysym.sym == SDLK_RETURN) {
                            float trust_delta;
                            const char *feedback;
                            if (engine.selected_scandal_option == 0) {
                                trust_delta = -20.0f;
                                feedback = "NEGACAO PUBLICA -20 CONFIANCA";
                            } else if (engine.selected_scandal_option == 1) {
                                trust_delta = 15.0f;
                                feedback = "TRANSPARENCIA E CORRECAO +15 CONFIANCA";
                            } else {
                                trust_delta = -10.0f;
                                feedback = "CULPA TERCEIRIZADA -10 CONFIANCA";
                            }
                            engine.target_trust = clampf(engine.target_trust + trust_delta, 0.0f, 100.0f);
                            if (trust_delta < 0.0f) trigger_screen_shake(&engine, 400);
                            spawn_floating_text(&engine, feedback, 500, 300, COLOR_ALERT_RED);

                            engine.scandal_resolved = true;
                            reset_ato4_state(&engine);
                            engine.state = STATE_ATO4_DESFECHO;
                        }
                    }
                    if (e.key.keysym.sym == SDLK_ESCAPE) engine.state = STATE_MENU;
                    break;
                }

                case STATE_ATO4_DESFECHO:
                    if (e.key.keysym.sym == SDLK_RETURN) {
                        engine.state = STATE_MENU;
                        engine.selected_menu_option = 0;
                    }
                    if (e.key.keysym.sym == SDLK_ESCAPE) engine.running = false;
                    break;
                }
            }
        }

        /* -------------------- ATUALIZACAO DE LOGICA -------------------- */
        update_trust_interpolation(&engine, dt);
        update_floating_texts(&engine, dt);

        if (engine.shake_timer > 0) {
            engine.shake_timer -= (int)(dt * 1000.0f);
            if (engine.shake_timer < 0) engine.shake_timer = 0;
        }

        if (engine.state == STATE_MINIGAME_LOCK) {
            engine.minigame_time_remaining -= dt;
            if (engine.minigame_time_remaining <= 0.0f) {
                trigger_screen_shake(&engine, 400);
                engine.target_trust = clampf(engine.target_trust - 10.0f, 0.0f, 100.0f);
                spawn_floating_text(&engine, "TEMPO ESGOTADO! -10 CONFIANCA", 640, 250, COLOR_YELLOW);
                reset_minigame_state(&engine);
            }
        }

        /* Efeito typewriter: avanca 1 caractere a cada ~25ms nas telas
           narrativas (explicacao do Ato 1, Ato 2, Ato 3 e Ato 4) */
        bool uses_typewriter = (engine.state == STATE_ATO2_TICKETS) ||
                                (engine.state == STATE_ATO3_ESCANDALO) ||
                                (engine.state == STATE_ATO4_DESFECHO) ||
                                (engine.state == STATE_QUIZ_ATO1 && engine.showing_explanation);
        if (uses_typewriter) {
            const float typewriter_speed = 0.025f; /* segundos por caractere */
            engine.typewriter_accumulator += dt;
            while (engine.typewriter_accumulator >= typewriter_speed) {
                engine.typewriter_accumulator -= typewriter_speed;
                engine.typewriter_index++;
            }
        }

        /* Grava o certificado e o save.dat uma unica vez ao entrar no Ato 4 */
        if (engine.state == STATE_ATO4_DESFECHO && !engine.ato4_files_written) {
            write_certificado(&engine);
            save_game(&engine);
            engine.ato4_files_written = true;
        }

        /* -------------------- RENDERIZACAO -------------------- */
        SDL_RenderSetViewport(renderer, NULL);
        SDL_Color bg_color = (engine.state == STATE_ATO3_ESCANDALO) ? COLOR_BG_RED : COLOR_BG;
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, 255);
        SDL_RenderClear(renderer);

        int shake_x = 0, shake_y = 0;
        apply_screen_shake(&shake_x, &shake_y, engine.shake_timer);
        SDL_Rect shake_viewport = {shake_x, shake_y, WINDOW_W, WINDOW_H};
        SDL_RenderSetViewport(renderer, &shake_viewport);

        switch (engine.state) {
            case STATE_MENU:
                render_screen_menu(renderer, font_title, font_ui, &engine);
                break;
            case STATE_QUIZ_ATO1:
                render_screen_quiz(renderer, font_title, font_ui, &engine);
                break;
            case STATE_MINIGAME_LOCK:
                render_screen_minigame_lock(renderer, font_title, font_ui, &engine);
                break;
            case STATE_ATO2_TICKETS:
                render_screen_ato2_tickets(renderer, font_title, font_ui, &engine);
                break;
            case STATE_ATO3_ESCANDALO:
                render_screen_ato3_escandalo(renderer, font_title, font_ui, &engine);
                break;
            case STATE_ATO4_DESFECHO:
                render_screen_ato4_desfecho(renderer, font_title, font_ui, &engine);
                break;
        }

        render_floating_texts(renderer, font_ui, &engine);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); /* ~60 FPS */
    }

    if (font_ui) TTF_CloseFont(font_ui);
    if (font_title) TTF_CloseFont(font_title);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}