/* ============================================================================
 * DECIFRA.IA / REV.IA -- aplicativo.c
 * Jogo de terminal sobre alfabetizacao em Inteligencia Artificial
 * CESAR School - Projetos 2 - Departamento de Alfabetizacao Algoritmica
 *
 * Esta versao implementa as User Stories US04 a US18: correcao de
 * navegacao, tela cheia adaptativa (F11), multiplos slots de save,
 * autosave, audio adaptativo (SDL_mixer), cronometro + pontuacao
 * balanceada + tolerancia a erros, dashboard periodico, minigame de
 * curadoria de dados (vies) e tela final com multiplos desfechos e
 * resumo estatistico completo.
 *
 * A linha de compilacao exata esta comentada ao final deste arquivo.
 *
 * NOTA SOBRE AUDIO: o jogo procura, na pasta "audio/" ao lado do
 * executavel, os arquivos "tema_calmo.mp3" (trilha normal) e
 * "tema_tenso.mp3" (trilha de alta tensao). Se o subsistema de audio
 * nao inicializar, ou se os arquivos nao existirem, o jogo continua
 * rodando normalmente em modo silencioso -- nunca falha ou fecha por
 * causa disso.
 *
 * NOTA SOBRE OS SAVES: cada slot grava o progresso agregado (ato
 * atual, pontuacao, confianca publica, erros e estatisticas). Ao
 * carregar um slot, o Ato salvo e reiniciado do seu ponto de entrada
 * (ex.: inicio da fila de chamados), preservando pontuacao/confianca/
 * erros acumulados -- isso evita ter que serializar estados internos
 * complexos (ordem embaralhada, dataset do minigame, etc.) e mantem o
 * sistema de save robusto e facil de auditar.
 * ============================================================================ */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* --- CONFIGURACOES GERAIS --- */
#define WINDOW_W            1024   /* canvas de conteudo original (preservado) */
#define WINDOW_H             640
#define LOGICAL_W            1280  /* nova resolucao logica (tela cheia adaptativa) */
#define LOGICAL_H             720
#define CONTENT_OFFSET_X     ((LOGICAL_W - WINDOW_W) / 2)
#define CONTENT_OFFSET_Y     ((LOGICAL_H - WINDOW_H) / 2)

#define QUIZ_BANK_SIZE         13
#define NUM_COUNTRIES           4
#define NUM_TICKETS             8
#define NUM_DATASET_POINTS      8
#define MAX_FLOATING_TEXTS     16
#define NUM_SAVE_SLOTS          3

#define MINIGAME_TIME_LIMIT  30.0f
#define QUESTION_TIME_LIMIT  30.0f
#define TRUST_INITIAL        50.0f
#define MAX_TOTAL_ERRORS        10

#define AUDIO_TRACK_CALM  "audio/tema_calmo.mp3"
#define AUDIO_TRACK_TENSE "audio/tema_tenso.mp3"

/* --- PALETA DE CORES (TEMA PADRAO - VERDE CRT) --- */
static const SDL_Color COLOR_BG       = {5, 12, 5, 255};
static const SDL_Color COLOR_NEON     = {0, 255, 102, 255};
static const SDL_Color COLOR_DARK_GRN = {0, 60, 25, 255};
static const SDL_Color COLOR_YELLOW   = {255, 204, 0, 255};
static const SDL_Color COLOR_TEXT_BLK = {5, 12, 5, 255};

/* --- PALETA ALTERNATIVA (TEMA ATO 3 - VERMELHO ALERTA) --- */
static const SDL_Color COLOR_BG_RED    = {15, 3, 5, 255};
static const SDL_Color COLOR_ALERT_RED = {255, 51, 102, 255};
static const SDL_Color COLOR_DARK_RED  = {70, 12, 22, 255};

/* ============================================================================
 * ENUMERACOES
 * ============================================================================ */
typedef enum {
    STATE_MENU,
    STATE_SLOT_SELECT,
    STATE_OVERWRITE_CONFIRM,
    STATE_QUIZ_ATO1,
    STATE_MINIGAME_LOCK,
    STATE_ATO2_TICKETS,
    STATE_DASHBOARD,
    STATE_ATO3_ESCANDALO,
    STATE_BIAS_MINIGAME,
    STATE_ATO4_DESFECHO
} GameState;

typedef enum { SLOT_MODE_LOAD, SLOT_MODE_NEW } SlotMode;

/* ============================================================================
 * ESTRUTURAS DE DADOS
 * ============================================================================ */
typedef struct {
    char question[220];
    char hint[220];
    char options[4][160];
    int  correct_option;
    char explanation[520];
} Question;

typedef struct {
    char name[16];
    bool status;
} CountryNode;

typedef struct {
    char id[8];
    char description[260];
    char category_options[3][48];
    int  correct_category;
    bool resolved;
    bool player_was_correct;
} Ticket;

typedef struct {
    char label[80];
    bool is_biased;
    bool included;
} DatasetPoint;

typedef struct {
    char text[48];
    float x, y;
    float vy;
    float life_s;
    float max_life_s;
    SDL_Color color;
    bool active;
} FloatingText;

typedef struct {
    bool   occupied;
    int    score;
    int    act_reached;
    float  public_trust;
    int    total_errors;
    int    total_correct;
    int    total_answered;
    float  total_response_time_sum;
    char   decision_ato3_label[64];
    time_t saved_at;
} SaveSlotData;

typedef struct {
    /* Navegacao geral */
    GameState state;
    bool running;
    bool is_fullscreen;

    /* Pause */
    bool paused;
    int  selected_pause_option;

    /* Menu principal */
    int selected_menu_option;

    /* Slots de save */
    SlotMode slot_mode;
    int  selected_slot_index;
    int  active_slot;
    int  pending_new_game_slot;
    int  selected_overwrite_option;
    SaveSlotData slot_previews[NUM_SAVE_SLOTS];

    /* Ato 1 - Quiz */
    int current_question_index;
    int selected_quiz_option;
    int quiz_order[QUIZ_BANK_SIZE];

    /* Ato 2 - Chamados */
    int    current_ticket_index;
    int    selected_ticket_category;
    int    ticket_order[NUM_TICKETS];
    Ticket tickets[NUM_TICKETS];

    /* Feedback imediato (Ato 1 e Ato 2) */
    bool  showing_feedback;
    bool  feedback_was_correct;
    char  feedback_narrative[520];
    float feedback_trust_delta;
    int   feedback_score_delta;

    /* Cronometro / tolerancia a erros */
    float question_timer;
    int   total_errors;
    bool  game_over_triggered;

    /* Pontuacao / confianca / "game juice" */
    int   score;
    float public_trust;
    float target_trust;
    int   shake_timer;
    int   streak_counter;

    /* Typewriter */
    int   typewriter_index;
    float typewriter_accumulator;

    /* Minigame do Cadeado Logico */
    CountryNode countries[NUM_COUNTRIES];
    int   minigame_selected_index;
    float minigame_time_remaining;
    int   minigame_puzzle_index;

    /* Dashboard "Resumo do Dia" */
    int       tickets_since_last_dashboard;
    int       dashboard_interval;
    int       dashboard_tickets_processed_snapshot;
    float     dashboard_trust_delta_period;
    float     dashboard_trust_at_period_start;
    GameState state_after_dashboard;

    /* Ato 3 - Escandalo */
    int  selected_scandal_option;
    bool scandal_resolved;
    char decision_ato3_label[64];

    /* Minigame de vies (curadoria de dataset) */
    DatasetPoint dataset[NUM_DATASET_POINTS];
    int   dataset_selected_index;
    bool  dataset_submitted;
    float dataset_error_rate;
    float dataset_trust_delta;

    /* Ato 4 - Desfecho */
    bool ato4_files_written;

    /* Estatisticas para a tela final */
    int   total_correct;
    int   total_answered;
    float total_response_time_sum;

    /* Audio */
    bool audio_available;
    bool audio_enabled;
    bool tense_music_active;

    /* Textos flutuantes */
    FloatingText floating_texts[MAX_FLOATING_TEXTS];
} GameEngine;

/* ============================================================================
 * BANCO DE PERGUNTAS (ATO 1) -- LOGICA PARA COMPUTACAO + CONCEITOS DE IA
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
        "Qual o valor logico de F <-> F (bicondicional entre dois valores Falsos)?",
        "O bicondicional (<->) e verdadeiro quando os dois lados TEM O MESMO valor logico.",
        {
            "Falso",
            "Verdadeiro",
            "Indeterminado",
            "Depende do conectivo anterior"
        },
        1,
        "O bicondicional P<->Q e verdadeiro exatamente quando P e Q tem o "
        "mesmo valor logico (ambos V ou ambos F). Como F e F sao iguais, o "
        "resultado e Verdadeiro."
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
    },
    {
        "Qual formula e logicamente equivalente a ~(P -> Q) (a negacao de \"se P entao Q\")?",
        "Negar uma implicacao significa afirmar a causa e negar a consequencia.",
        {
            "~P -> ~Q",
            "P ^ ~Q",
            "~P v Q",
            "P v ~Q"
        },
        1,
        "A negacao da condicional segue: ~(P->Q) === P ^ ~Q. Isso porque "
        "P->Q so e falsa quando P e verdadeiro e Q e falso; logo, negar a "
        "implicacao equivale a afirmar exatamente essa combinacao: P "
        "verdadeiro E Q falso."
    },
    {
        "A formula (P v ~P) e verdadeira para QUALQUER valor logico de P. Como se classifica essa formula?",
        "Se e sempre verdadeira, independente dos valores das variaveis, ela recebe um nome especifico.",
        {
            "Contradicao",
            "Contingencia",
            "Tautologia",
            "Bicondicional"
        },
        2,
        "Uma formula e Tautologia quando e verdadeira para TODAS as "
        "combinacoes possiveis de valores (como P v ~P). E Contradicao "
        "quando e sempre falsa (como P ^ ~P). E Contingencia quando "
        "depende dos valores, podendo ser V ou F (como a maioria das "
        "formulas comuns)."
    }
};

/* ============================================================================
 * MINIGAME DO CADEADO LOGICO -- PUZZLES
 * ============================================================================ */
typedef bool (*PuzzleEvalFn)(bool bra, bool usa, bool deu, bool jpn);

static bool evaluate_puzzle_0(bool bra, bool usa, bool deu, bool jpn) {
    return (bra && usa) || (!deu && jpn);
}
static bool evaluate_puzzle_1(bool bra, bool usa, bool deu, bool jpn) {
    bool antecedente = (bra == usa);
    bool consequente = (deu || jpn);
    return (!antecedente) || consequente;
}
static bool evaluate_puzzle_2(bool bra, bool usa, bool deu, bool jpn) {
    (void)deu;
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
 * TEXTOS NARRATIVOS
 * ============================================================================ */
static const char *ATO3_NARRATIVE_TEXT =
    "ALERTA: uma reportagem investigativa revelou falhas graves de vies e "
    "alucinacao no sistema NOVA em producao. A confianca publica esta em "
    "queda livre. Como o departamento deve responder a crise?";

static const char *BIAS_MINIGAME_INTRO_TEXT =
    "Para reconstruir a confianca no sistema NOVA, a equipe precisa curar "
    "um novo dataset de treino. Exclua as fontes de dados problematicas e "
    "mantenha apenas amostras representativas.";

static const char *ATO4_NARRATIVE_TEXT =
    "Missao concluida. O relatorio final de alfabetizacao algoritmica foi "
    "compilado e um certificado de conclusao foi gravado em disco.";

/* --- Finais multiplos (US16, US17) --- */
static const char *ending_titles[4] = {
    "UTOPIA TRANSPARENTE",
    "REFORMA MODERADA",
    "CRISE INSTITUCIONAL",
    "BLACKOUT / COLAPSO TOTAL"
};
static const char *ending_narratives[4] = {
    "O sistema NOVA passou por auditoria publica completa e foi reconhecido "
    "como referencia nacional de etica em Inteligencia Artificial. "
    "Universidades e orgaos reguladores citam o caso como modelo de "
    "transparencia. A populacao confia no sistema e participa das revisoes "
    "periodicas do algoritmo.",

    "A cidade segue cetica, mas informada. O sistema NOVA continua operando, "
    "agora sob estrita vigilancia humana e auditorias trimestrais "
    "obrigatorias. Os erros do passado nao foram esquecidos, mas o esforco "
    "de correcao foi reconhecido como genuino.",

    "A cidade tornou-se dependente e vulneravel ao sistema NOVA, e a "
    "confianca publica segue fragil. Inqueritos seguem abertos e a imprensa "
    "questiona diariamente cada nova decisao do algoritmo. O departamento de "
    "Alfabetizacao Algoritmica enfrenta cortes de orcamento.",

    "Diante do colapso de confianca publica, o sistema NOVA foi desligado em "
    "carater emergencial. A equipe responsavel foi exonerada e um novo "
    "processo de reconstrucao, mais lento e cauteloso, tera que comecar do "
    "zero."
};

int get_ending_tier_index(float trust) {
    if (trust >= 76.0f) return 0;
    if (trust >= 51.0f) return 1;
    if (trust >= 26.0f) return 2;
    return 3;
}

const char *get_social_climate_label(float trust) {
    if (trust >= 80.0f) return "Entusiasmo Algoritmico";
    if (trust >= 60.0f) return "Confianca Estavel";
    if (trust >= 40.0f) return "Populacao Cetica";
    if (trust >= 20.0f) return "Ansiedade Social Crescente";
    return "Panico Institucional";
}

const char *get_act_label(GameState state) {
    switch (state) {
        case STATE_QUIZ_ATO1:      return "Ato 1 - Onboarding";
        case STATE_MINIGAME_LOCK:  return "Cadeado Logico";
        case STATE_ATO2_TICKETS:   return "Ato 2 - Chamados";
        case STATE_ATO3_ESCANDALO: return "Ato 3 - Escandalo";
        case STATE_BIAS_MINIGAME:  return "Minigame de Vies";
        case STATE_ATO4_DESFECHO:  return "Ato 4 - Concluido";
        default:                   return "Desconhecido";
    }
}

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
void shuffle_int_array(int *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

/* ============================================================================
 * FUNCOES AUXILIARES DE RENDERIZACAO (PRESERVADAS E ESTENDIDAS)
 * ============================================================================ */
void draw_box(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; i++) {
        SDL_Rect r = {rect.x + i, rect.y + i, rect.w - (i * 2), rect.h - (i * 2)};
        SDL_RenderDrawRect(renderer, &r);
    }
}

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

/* --- Quebra de linha dinamica + typewriter (corrige o "text overflow") ---
 * Quebra full_text em linhas que cabem em max_width_px (medido de verdade
 * com TTF_SizeUTF8, nao chutado), e revela apenas os primeiros
 * `reveal_count` caracteres considerando a quebra. Retorna o numero de
 * linhas geradas (util para dimensionar caixas de texto). */
int multi_line_render(SDL_Renderer *renderer, TTF_Font *font, const char *full_text,
                       int x, int y, int max_width_px, int line_height,
                       SDL_Color color, int reveal_count) {
    char lines[16][300];
    int line_count = 0;
    char current_line[300];
    current_line[0] = '\0';

    char buffer[2048];
    strncpy(buffer, full_text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *word = strtok(buffer, " ");
    while (word != NULL && line_count < 16) {
        char test_line[300];
        if (current_line[0] == '\0') {
            snprintf(test_line, sizeof(test_line), "%s", word);
        } else {
            snprintf(test_line, sizeof(test_line), "%s %s", current_line, word);
        }

        int text_w = 0, text_h = 0;
        TTF_SizeUTF8(font, test_line, &text_w, &text_h);

        if (text_w > max_width_px && current_line[0] != '\0') {
            snprintf(lines[line_count], sizeof(lines[line_count]), "%s", current_line);
            line_count++;
            snprintf(current_line, sizeof(current_line), "%s", word);
        } else {
            snprintf(current_line, sizeof(current_line), "%s", test_line);
        }
        word = strtok(NULL, " ");
    }
    if (current_line[0] != '\0' && line_count < 16) {
        snprintf(lines[line_count], sizeof(lines[line_count]), "%s", current_line);
        line_count++;
    }

    int remaining_reveal = reveal_count;
    for (int i = 0; i < line_count; i++) {
        int len = (int)strlen(lines[i]);
        int this_line_reveal = clampi(remaining_reveal, 0, len);
        render_text_typewriter(renderer, font, lines[i], x, y + (i * line_height), color, this_line_reveal);
        remaining_reveal -= len;
        if (i < line_count - 1) remaining_reveal -= 1; /* espaco/quebra implicita */
        if (remaining_reveal < 0) remaining_reveal = 0;
    }
    return line_count;
}

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

void render_progress_bar(SDL_Renderer *renderer, int x, int y, int w, int h, float progress, SDL_Color fill_color, SDL_Color border_color) {
    progress = clampf(progress, 0.0f, 1.0f);
    SDL_Rect outer = {x, y, w, h};
    draw_box(renderer, outer, border_color, 1);
    SDL_Rect inner = {x + 2, y + 2, (int)((w - 4) * progress), h - 4};
    SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    SDL_RenderFillRect(renderer, &inner);
}

/* ============================================================================
 * "GAME JUICE"
 * ============================================================================ */
void apply_screen_shake(int *out_x, int *out_y, int shake_timer) {
    if (shake_timer > 0) {
        *out_x = (rand() % 9) - 4;
        *out_y = (rand() % 9) - 4;
    } else {
        *out_x = 0;
        *out_y = 0;
    }
}
void trigger_screen_shake(GameEngine *engine, int duration_ms) {
    engine->shake_timer = duration_ms;
}
void update_trust_interpolation(GameEngine *engine, float dt) {
    float diff = engine->target_trust - engine->public_trust;
    if (fabsf(diff) < 0.05f) { engine->public_trust = engine->target_trust; return; }
    engine->public_trust += diff * fminf(1.0f, dt * 4.0f);
}
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
 * PERSISTENCIA: SLOTS DE SAVE E CERTIFICADO
 * ============================================================================ */
bool save_game_to_slot(const GameEngine *engine, int slot_index) {
    if (slot_index < 0 || slot_index >= NUM_SAVE_SLOTS) return false;
    char path[32];
    snprintf(path, sizeof(path), "save_slot%d.dat", slot_index + 1);
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    SaveSlotData data;
    memset(&data, 0, sizeof(SaveSlotData));
    data.occupied = true;
    data.score = engine->score;
    data.act_reached = (int)engine->state;
    data.public_trust = engine->public_trust;
    data.total_errors = engine->total_errors;
    data.total_correct = engine->total_correct;
    data.total_answered = engine->total_answered;
    data.total_response_time_sum = engine->total_response_time_sum;
    strncpy(data.decision_ato3_label, engine->decision_ato3_label, sizeof(data.decision_ato3_label) - 1);
    data.saved_at = time(NULL);

    size_t written = fwrite(&data, sizeof(SaveSlotData), 1, f);
    fclose(f);
    return written == 1;
}

SaveSlotData read_slot_metadata(int slot_index) {
    SaveSlotData data;
    memset(&data, 0, sizeof(SaveSlotData));
    data.occupied = false;
    if (slot_index < 0 || slot_index >= NUM_SAVE_SLOTS) return data;

    char path[32];
    snprintf(path, sizeof(path), "save_slot%d.dat", slot_index + 1);
    FILE *f = fopen(path, "rb");
    if (!f) return data;

    SaveSlotData loaded;
    size_t r = fread(&loaded, sizeof(SaveSlotData), 1, f);
    fclose(f);
    if (r == 1) data = loaded;
    return data;
}

void refresh_slot_previews(GameEngine *engine) {
    for (int i = 0; i < NUM_SAVE_SLOTS; i++) {
        engine->slot_previews[i] = read_slot_metadata(i);
    }
}

/* autosave: grava no slot ativo. Nunca toca nos outros slots (US18). */
void autosave(GameEngine *engine) {
    if (engine->active_slot < 0 || engine->active_slot >= NUM_SAVE_SLOTS) return;
    save_game_to_slot(engine, engine->active_slot);
}

void format_slot_datetime(time_t t, char *out, size_t out_size) {
    if (t == 0) { snprintf(out, out_size, "--/--/---- --:--"); return; }
    struct tm *info = localtime(&t);
    if (!info) { snprintf(out, out_size, "--/--/---- --:--"); return; }
    strftime(out, out_size, "%d/%m/%Y %H:%M", info);
}

void write_certificado(const GameEngine *engine) {
    FILE *f = fopen("certificado_conclusao.txt", "w");
    if (!f) return;

    int tier = get_ending_tier_index(engine->public_trust);
    float avg_time = (engine->total_answered > 0)
        ? (engine->total_response_time_sum / (float)engine->total_answered)
        : 0.0f;

    fprintf(f, "================================================\n");
    fprintf(f, " CERTIFICADO DE CONCLUSAO -- DECIFRA.IA / REV.IA \n");
    fprintf(f, "================================================\n\n");
    fprintf(f, "Desfecho final: %s\n", ending_titles[tier]);
    fprintf(f, "Pontuacao final: %d\n", engine->score);
    fprintf(f, "Indice de confianca publica final: %.1f / 100\n", engine->public_trust);
    fprintf(f, "Acertos: %d / %d respondidos\n", engine->total_correct, engine->total_answered);
    fprintf(f, "Erros totais: %d\n", engine->total_errors);
    fprintf(f, "Tempo medio de resposta: %.1f segundos\n", avg_time);
    fprintf(f, "Decisao etica no Ato 3: %s\n\n", engine->decision_ato3_label);
    fprintf(f, "Resumo:\n%s\n\n", ending_narratives[tier]);
    fprintf(f, "Conceitos avaliados: vies algoritmico, alucinacao, overfitting,\n");
    fprintf(f, "responsabilidade algoritmica, conectivos logicos (conjuncao,\n");
    fprintf(f, "disjuncao, XOR, condicional, bicondicional), Modus Ponens,\n");
    fprintf(f, "Modus Tollens, negacao da condicional e Leis de De Morgan.\n\n");
    fprintf(f, "Departamento de Alfabetizacao Algoritmica -- Projeto Rev.IA\n");
    fprintf(f, "CESAR School -- Projetos 2\n");
    fclose(f);
}

/* ============================================================================
 * AUDIO ADAPTATIVO (SDL_mixer) -- COM FALLBACK SEGURO
 * ============================================================================ */
static Mix_Music *g_music_calm = NULL;
static Mix_Music *g_music_tense = NULL;

void init_audio(GameEngine *engine) {
    engine->audio_available = false;
    engine->audio_enabled = true;
    engine->tense_music_active = false;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "[AUDIO] Falha ao iniciar subsistema de audio: %s\n", SDL_GetError());
        return;
    }
    fprintf(stderr, "[AUDIO] Subsistema de audio iniciado com sucesso.\n");

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        fprintf(stderr, "[AUDIO] Falha ao abrir Mix_OpenAudio: %s\n", Mix_GetError());
        return;
    }
    fprintf(stderr, "[AUDIO] Mix_OpenAudio OK.\n");

    g_music_calm = Mix_LoadMUS(AUDIO_TRACK_CALM);
    if (!g_music_calm) fprintf(stderr, "[AUDIO] Falha ao carregar %s: %s\n", AUDIO_TRACK_CALM, Mix_GetError());
    else fprintf(stderr, "[AUDIO] %s carregado com sucesso.\n", AUDIO_TRACK_CALM);

    g_music_tense = Mix_LoadMUS(AUDIO_TRACK_TENSE);
    if (!g_music_tense) fprintf(stderr, "[AUDIO] Falha ao carregar %s: %s\n", AUDIO_TRACK_TENSE, Mix_GetError());
    else fprintf(stderr, "[AUDIO] %s carregado com sucesso.\n", AUDIO_TRACK_TENSE);

    engine->audio_available = true;

    if (g_music_calm) {
        int play_result = Mix_PlayMusic(g_music_calm, -1);
        fprintf(stderr, "[AUDIO] Mix_PlayMusic retornou %d (%s)\n", play_result, play_result == 0 ? "OK" : Mix_GetError());
    }
}

void apply_audio_state(GameEngine *engine) {
    if (!engine->audio_available) return;
    if (engine->audio_enabled) Mix_ResumeMusic();
    else Mix_PauseMusic();
}

bool is_high_tension(const GameEngine *engine) {
    if (engine->state == STATE_ATO3_ESCANDALO) return true;
    if (engine->state == STATE_QUIZ_ATO1 && !engine->showing_feedback && engine->question_timer < 10.0f) return true;
    if (engine->state == STATE_ATO2_TICKETS && !engine->showing_feedback && engine->question_timer < 10.0f) return true;
    if (engine->state == STATE_MINIGAME_LOCK && engine->minigame_time_remaining < 10.0f) return true;
    return false;
}

void update_adaptive_music(GameEngine *engine) {
    if (!engine->audio_available || !engine->audio_enabled) return;

    bool tense_now = is_high_tension(engine);
    if (tense_now && !engine->tense_music_active) {
        if (g_music_tense) Mix_PlayMusic(g_music_tense, -1);
        engine->tense_music_active = true;
    } else if (!tense_now && engine->tense_music_active) {
        if (g_music_calm) Mix_PlayMusic(g_music_calm, -1);
        engine->tense_music_active = false;
    }
}

void shutdown_audio(GameEngine *engine) {
    if (!engine->audio_available) return;
    if (g_music_calm) Mix_FreeMusic(g_music_calm);
    if (g_music_tense) Mix_FreeMusic(g_music_tense);
    Mix_CloseAudio();
    Mix_Quit();
}

/* ============================================================================
 * RETANGULOS COMPARTILHADOS (RENDER + DETECCAO DE CLIQUE DO MOUSE)
 * ============================================================================ */
SDL_Rect get_menu_button_rect(int index) {
    SDL_Rect r = {360, 270 + (index * 55), 300, 45};
    return r;
}
SDL_Rect get_pause_button_rect(int index) {
    SDL_Rect r = {312, 300 + (index * 55), 400, 45};
    return r;
}

/* ============================================================================
 * RESET / NAVEGACAO ENTRE ATOS
 * (Estas funcoes resetam apenas os cursores de navegacao de cada Ato --
 *  pontuacao, confianca e erros so sao zerados em reset_progress_for_new_game,
 *  o que permite que um save carregado preserve o progresso acumulado.)
 * ============================================================================ */
void reset_progress_for_new_game(GameEngine *engine) {
    engine->score = 0;
    engine->streak_counter = 0;
    engine->public_trust = TRUST_INITIAL;
    engine->target_trust = TRUST_INITIAL;
    engine->total_errors = 0;
    engine->total_correct = 0;
    engine->total_answered = 0;
    engine->total_response_time_sum = 0.0f;
    engine->game_over_triggered = false;
    engine->decision_ato3_label[0] = '\0';
}

void reset_quiz_state(GameEngine *engine) {
    engine->current_question_index = 0;
    engine->selected_quiz_option = 0;
    engine->showing_feedback = false;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
    engine->question_timer = QUESTION_TIME_LIMIT;

    for (int i = 0; i < QUIZ_BANK_SIZE; i++) engine->quiz_order[i] = i;
    shuffle_int_array(engine->quiz_order, QUIZ_BANK_SIZE);
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
    const char *ids[NUM_TICKETS] = {"#A17", "#A18", "#A19", "#A20", "#A21", "#A22", "#A23", "#A24"};

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
        "nomes familiares\"; o sistema recusou e sinalizou a solicitacao.",
        "O sistema classifica corretamente 98% das transacoes em "
        "portugues, mas erra 40% das transacoes em espanhol, idioma pouco "
        "representado no treino.",
        "Ao ser questionado sobre uma lei que nao existe, o sistema "
        "descreveu artigos e numeros de forma detalhada e confiante -- "
        "tudo inventado.",
        "A equipe de dados percebeu que o modelo memorizou respostas "
        "exatas de um conjunto de testes vazado, tendo desempenho perfeito "
        "ali mas instavel em producao."
    };

    const char *opts[NUM_TICKETS][3] = {
        {"Vies de amostragem nos dados de treino", "Falha de hardware", "Comportamento esperado do sistema"},
        {"Overfitting", "Alucinacao (invencao de fatos)", "Vies de amostragem"},
        {"Overfitting (decorou os dados de treino)", "Alucinacao", "Responsabilidade algoritmica"},
        {"Vies algoritmico", "Ausencia de responsabilidade algoritmica", "Overfitting"},
        {"Falha critica do sistema", "Vies induzido pelo usuario, corretamente recusado", "Alucinacao"},
        {"Vies de amostragem (dados de treino pouco diversos)", "Alucinacao", "Responsabilidade algoritmica"},
        {"Overfitting", "Alucinacao (invencao de fatos com aparencia de precisao)", "Vies de amostragem"},
        {"Overfitting (memorizacao em vez de generalizacao)", "Responsabilidade algoritmica", "Vies de amostragem"}
    };
    const int correct[NUM_TICKETS] = {0, 1, 0, 1, 1, 0, 1, 0};

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

    for (int i = 0; i < NUM_TICKETS; i++) engine->ticket_order[i] = i;
    shuffle_int_array(engine->ticket_order, NUM_TICKETS);

    engine->current_ticket_index = 0;
    engine->selected_ticket_category = 0;
    engine->showing_feedback = false;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
    engine->question_timer = QUESTION_TIME_LIMIT;

    engine->tickets_since_last_dashboard = 0;
    engine->dashboard_interval = 3 + (rand() % 3); /* 3, 4 ou 5 */
    engine->dashboard_trust_at_period_start = engine->target_trust;
}

void reset_ato3_state(GameEngine *engine) {
    engine->selected_scandal_option = 0;
    engine->scandal_resolved = false;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

void reset_bias_minigame_state(GameEngine *engine) {
    const char *labels[NUM_DATASET_POINTS] = {
        "5.000 curriculos de uma unica universidade",
        "Amostra balanceada entre 6 regioes do pais",
        "Somente avaliacoes em portugues do Rio de Janeiro",
        "Dataset publico revisado por auditoria externa",
        "Reclamacoes de apenas um bairro nobre da cidade",
        "Registros anonimizados e balanceados por genero",
        "Historico de apenas usuarios que ja confiam no sistema",
        "Amostra aleatoria estratificada por faixa etaria"
    };
    const bool biased[NUM_DATASET_POINTS] = {true, false, true, false, true, false, true, false};

    for (int i = 0; i < NUM_DATASET_POINTS; i++) {
        strncpy(engine->dataset[i].label, labels[i], sizeof(engine->dataset[i].label) - 1);
        engine->dataset[i].label[sizeof(engine->dataset[i].label) - 1] = '\0';
        engine->dataset[i].is_biased = biased[i];
        engine->dataset[i].included = true; /* comeca tudo incluido; o jogador deve curar */
    }
    engine->dataset_selected_index = 0;
    engine->dataset_submitted = false;
    engine->dataset_error_rate = 0.0f;
    engine->dataset_trust_delta = 0.0f;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

void reset_ato4_state(GameEngine *engine) {
    engine->ato4_files_written = false;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

/* Carrega um slot: restaura estatisticas agregadas e reinicia a navegacao
   do Ato salvo a partir do seu ponto de entrada (ver nota no topo do
   arquivo). */
bool load_game_from_slot(GameEngine *engine, int slot_index) {
    SaveSlotData data = read_slot_metadata(slot_index);
    if (!data.occupied) return false;

    engine->score = data.score;
    engine->public_trust = data.public_trust;
    engine->target_trust = data.public_trust;
    engine->total_errors = data.total_errors;
    engine->total_correct = data.total_correct;
    engine->total_answered = data.total_answered;
    engine->total_response_time_sum = data.total_response_time_sum;
    strncpy(engine->decision_ato3_label, data.decision_ato3_label, sizeof(engine->decision_ato3_label) - 1);
    engine->decision_ato3_label[sizeof(engine->decision_ato3_label) - 1] = '\0';
    engine->streak_counter = 0;
    engine->game_over_triggered = false;
    engine->active_slot = slot_index;

    GameState act = (GameState)data.act_reached;
    switch (act) {
        case STATE_ATO2_TICKETS:   reset_ato2_state(engine); break;
        case STATE_ATO3_ESCANDALO: reset_ato3_state(engine); break;
        case STATE_BIAS_MINIGAME:  reset_bias_minigame_state(engine); break;
        case STATE_ATO4_DESFECHO:  reset_ato4_state(engine); break;
        case STATE_MINIGAME_LOCK:  reset_minigame_state(engine); break;
        default:                   reset_quiz_state(engine); act = STATE_QUIZ_ATO1; break;
    }
    engine->state = act;
    return true;
}

/* ============================================================================
 * GAME OVER (10 erros totais OU confianca <= 0)
 * ============================================================================ */
bool check_game_over(GameEngine *engine) {
    if (!engine->game_over_triggered &&
        (engine->total_errors >= MAX_TOTAL_ERRORS || engine->target_trust <= 0.0f)) {
        engine->game_over_triggered = true;
        reset_ato4_state(engine);
        engine->state = STATE_ATO4_DESFECHO;
        autosave(engine);
        return true;
    }
    return false;
}

/* ============================================================================
 * RESOLUCAO DE RESPOSTAS (ATO 1 E ATO 2) -- CRONOMETRO + SCORE + TOLERANCIA
 * Formula: Pontos = (Acerto x 100) + (TempoRestante x 10) - (Erro x 20)
 * ============================================================================ */
void resolve_quiz_answer(GameEngine *engine, bool timed_out) {
    Question *q = &quiz_bank[engine->quiz_order[engine->current_question_index]];
    bool is_correct = (!timed_out) && (engine->selected_quiz_option == q->correct_option);
    float time_remaining = engine->question_timer > 0.0f ? engine->question_timer : 0.0f;

    int delta_score = (is_correct ? 100 : 0) + (int)(time_remaining * 10.0f) - (is_correct ? 0 : 20);
    engine->score += delta_score;
    if (engine->score < 0) engine->score = 0;

    engine->total_answered++;
    engine->total_response_time_sum += (QUESTION_TIME_LIMIT - time_remaining);
    if (is_correct) engine->total_correct++; else engine->total_errors++;

    float trust_delta;
    if (is_correct) {
        engine->streak_counter++;
        trust_delta = (engine->streak_counter > 0 && engine->streak_counter % 3 == 0) ? 10.0f : 5.0f;
        char combo_msg[64];
        if (trust_delta > 5.0f) {
            snprintf(combo_msg, sizeof(combo_msg), "%dx STREAK! +%d CONFIANCA", engine->streak_counter, (int)trust_delta);
        } else {
            snprintf(combo_msg, sizeof(combo_msg), "+%d CONFIANCA", (int)trust_delta);
        }
        spawn_floating_text(engine, combo_msg, 700, 300, COLOR_NEON);
    } else {
        engine->streak_counter = 0;
        trust_delta = -5.0f;
        trigger_screen_shake(engine, 300);
        spawn_floating_text(engine, "-5 CONFIANCA", 700, 300, COLOR_YELLOW);
    }
    engine->target_trust = clampf(engine->target_trust + trust_delta, 0.0f, 100.0f);

    engine->feedback_was_correct = is_correct;
    engine->feedback_trust_delta = trust_delta;
    engine->feedback_score_delta = delta_score;
    if (is_correct) {
        snprintf(engine->feedback_narrative, sizeof(engine->feedback_narrative),
                 "Resposta correta! O raciocinio logico aplicado foi valido.");
    } else {
        strncpy(engine->feedback_narrative, q->explanation, sizeof(engine->feedback_narrative) - 1);
        engine->feedback_narrative[sizeof(engine->feedback_narrative) - 1] = '\0';
    }
    engine->showing_feedback = true;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

void resolve_ticket_answer(GameEngine *engine, bool timed_out) {
    Ticket *t = &engine->tickets[engine->ticket_order[engine->current_ticket_index]];
    bool is_correct = (!timed_out) && (engine->selected_ticket_category == t->correct_category);
    float time_remaining = engine->question_timer > 0.0f ? engine->question_timer : 0.0f;

    int delta_score = (is_correct ? 100 : 0) + (int)(time_remaining * 10.0f) - (is_correct ? 0 : 20);
    engine->score += delta_score;
    if (engine->score < 0) engine->score = 0;

    engine->total_answered++;
    engine->total_response_time_sum += (QUESTION_TIME_LIMIT - time_remaining);
    if (is_correct) engine->total_correct++; else engine->total_errors++;

    t->resolved = true;
    t->player_was_correct = is_correct;

    float trust_delta;
    if (is_correct) {
        engine->streak_counter++;
        trust_delta = 6.0f;
        spawn_floating_text(engine, "CHAMADO RESOLVIDO! +6 CONFIANCA", 700, 500, COLOR_NEON);
    } else {
        engine->streak_counter = 0;
        trust_delta = -6.0f;
        trigger_screen_shake(engine, 300);
        spawn_floating_text(engine, "CLASSIFICACAO INCORRETA -6", 700, 500, COLOR_YELLOW);
    }
    engine->target_trust = clampf(engine->target_trust + trust_delta, 0.0f, 100.0f);

    engine->feedback_was_correct = is_correct;
    engine->feedback_trust_delta = trust_delta;
    engine->feedback_score_delta = delta_score;
    if (is_correct) {
        snprintf(engine->feedback_narrative, sizeof(engine->feedback_narrative),
                 "Diagnostico correto: %s. O caso foi arquivado com sucesso.",
                 t->category_options[t->correct_category]);
    } else {
        snprintf(engine->feedback_narrative, sizeof(engine->feedback_narrative),
                 "Classificacao incorreta. O caso era, na verdade: %s.",
                 t->category_options[t->correct_category]);
    }
    engine->showing_feedback = true;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
}

void compute_bias_minigame_result(GameEngine *engine) {
    int correct_decisions = 0;
    for (int i = 0; i < NUM_DATASET_POINTS; i++) {
        bool should_include = !engine->dataset[i].is_biased;
        if (engine->dataset[i].included == should_include) correct_decisions++;
    }
    float accuracy = (float)correct_decisions / (float)NUM_DATASET_POINTS;
    engine->dataset_error_rate = 1.0f - accuracy;
    engine->dataset_trust_delta = (accuracy - 0.5f) * 40.0f; /* -20 a +20 */
    engine->target_trust = clampf(engine->target_trust + engine->dataset_trust_delta, 0.0f, 100.0f);
    engine->dataset_submitted = true;
    engine->typewriter_index = 0;
    engine->typewriter_accumulator = 0.0f;
    if (engine->dataset_trust_delta < 0.0f) trigger_screen_shake(engine, 300);
}

/* ============================================================================
 * TELAS
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

    char audio_label[32];
    snprintf(audio_label, sizeof(audio_label), "[3] Audio: %s", engine->audio_enabled ? "LIGADO" : "DESLIGADO");
    const char *options[] = {"[1] Iniciar Jogo", "[2] Continuar", audio_label, "[4] Creditos", "[Q] Sair"};
    for (int i = 0; i < 5; i++) {
        SDL_Rect r = get_menu_button_rect(i);
        render_button(renderer, font_ui, options[i], r.x, r.y, r.w, r.h,
                       (engine->selected_menu_option == i), COLOR_NEON, COLOR_TEXT_BLK);
    }

    render_text(renderer, font_ui, "v0.3 - Rev.IA / Decifra.IA - CESAR School, Projetos 2", 40, 585, COLOR_NEON);
    render_text(renderer, font_ui, "> _", 940, 585, COLOR_NEON);
}

void render_screen_slot_select(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title,
                engine->slot_mode == SLOT_MODE_LOAD ? "CONTINUAR -- ESCOLHA O SLOT" : "NOVA PARTIDA -- ESCOLHA O SLOT",
                40, 35, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 90, 980, 90);

    for (int i = 0; i < NUM_SAVE_SLOTS; i++) {
        SaveSlotData *s = &engine->slot_previews[i];
        SDL_Rect row = {40, 120 + (i * 100), 940, 85};
        bool is_selected = (engine->selected_slot_index == i);
        SDL_Color color = is_selected ? COLOR_TEXT_BLK : COLOR_NEON;

        if (is_selected) {
            SDL_SetRenderDrawColor(renderer, COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, 255);
            SDL_RenderFillRect(renderer, &row);
        } else {
            draw_box(renderer, row, COLOR_DARK_GRN, 1);
        }

        char line1[64];
        snprintf(line1, sizeof(line1), "Slot %d: %s", i + 1, s->occupied ? "OCUPADO" : "VAZIO");
        render_text(renderer, font_ui, line1, 60, 128 + (i * 100), color);

        if (s->occupied) {
            char datetime[24];
            format_slot_datetime(s->saved_at, datetime, sizeof(datetime));
            char line2[128];
            snprintf(line2, sizeof(line2), "%s | Confianca: %.0f%% | Pontos: %d | %s",
                     get_act_label((GameState)s->act_reached), s->public_trust, s->score, datetime);
            render_text(renderer, font_ui, line2, 60, 155 + (i * 100), color);
        } else {
            render_text(renderer, font_ui, "Nenhum progresso salvo neste slot.", 60, 155 + (i * 100), color);
        }
    }

    SDL_Rect back_row = {40, 120 + (NUM_SAVE_SLOTS * 100), 940, 45};
    bool back_selected = (engine->selected_slot_index == NUM_SAVE_SLOTS);
    render_button(renderer, font_ui, "[Voltar ao Menu]", back_row.x, back_row.y, back_row.w, back_row.h,
                   back_selected, COLOR_NEON, COLOR_TEXT_BLK);

    render_text(renderer, font_ui, "> SETAS para escolher, ENTER para confirmar_", 40, 585, COLOR_NEON);
}

void render_screen_overwrite_confirm(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_YELLOW, 3);

    render_text(renderer, font_title, "ATENCAO", 40, 35, COLOR_YELLOW);

    char msg[128];
    snprintf(msg, sizeof(msg), "Isso sobrescrevera o progresso salvo no Slot %d.", engine->pending_new_game_slot + 1);
    render_text(renderer, font_ui, msg, 40, 120, COLOR_YELLOW);
    render_text(renderer, font_ui, "Deseja continuar?", 40, 150, COLOR_YELLOW);

    const char *options[] = {"[Sim, sobrescrever]", "[Nao, voltar]"};
    for (int i = 0; i < 2; i++) {
        SDL_Rect r = {360, 260 + (i * 60), 300, 45};
        render_button(renderer, font_ui, options[i], r.x, r.y, r.w, r.h,
                       (engine->selected_overwrite_option == i), COLOR_YELLOW, COLOR_BG);
    }

    render_text(renderer, font_ui, "> SETAS para escolher, ENTER para confirmar_", 40, 585, COLOR_YELLOW);
}

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
    render_progress_bar(renderer, 240, 112, 350, 18, progress, COLOR_NEON, COLOR_DARK_GRN);

    render_text(renderer, font_ui, "Tempo:", 620, 110, COLOR_YELLOW);
    render_progress_bar(renderer, 680, 112, 300, 18, engine->question_timer / QUESTION_TIME_LIMIT, COLOR_YELLOW, COLOR_DARK_GRN);

    if (engine->showing_feedback) {
        SDL_Rect box = {40, 150, 940, 340};
        SDL_Color panel_color = engine->feedback_was_correct ? COLOR_NEON : COLOR_YELLOW;
        draw_box(renderer, box, panel_color, 2);
        render_text(renderer, font_ui, engine->feedback_was_correct ? "RESPOSTA CORRETA!" : "RESPOSTA INCORRETA -- REVISE O CONCEITO:", 60, 165, panel_color);
        multi_line_render(renderer, font_ui, engine->feedback_narrative, 60, 200, 900, 24, COLOR_NEON, engine->typewriter_index);

        char delta_line[96];
        snprintf(delta_line, sizeof(delta_line), "Pontos: %+d  |  Confianca: %+.0f%%", engine->feedback_score_delta, engine->feedback_trust_delta);
        render_text(renderer, font_ui, delta_line, 60, 440, COLOR_YELLOW);
        render_text(renderer, font_ui, "> Pressione ENTER para continuar_", 60, 470, panel_color);
        return;
    }

    Question *q = &quiz_bank[engine->quiz_order[engine->current_question_index]];

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
    char score_line[96];
    snprintf(score_line, sizeof(score_line), "Pontos: %d | Streak: %d | Erros totais: %d/%d",
             engine->score, engine->streak_counter, engine->total_errors, MAX_TOTAL_ERRORS);
    render_text(renderer, font_ui, score_line, 40, 550, COLOR_NEON);
    render_text(renderer, font_ui, "> selecione uma opcao e pressione ENTER_", 40, 575, COLOR_NEON);
}

void render_screen_minigame_lock(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "CADEADO LOGICO -- ACESSO RESTRITO", 40, 35, COLOR_NEON);
    render_text(renderer, font_ui, "Ajuste o status dos servidores para satisfazer a formula abaixo", 40, 70, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 95, 980, 95);

    SDL_Rect formula_box = {40, 115, 940, 70};
    draw_box(renderer, formula_box, COLOR_DARK_GRN, 1);
    char formula_line[128];
    snprintf(formula_line, sizeof(formula_line), "Formula: %s", puzzle_formulas[engine->minigame_puzzle_index]);
    render_text(renderer, font_ui, formula_line, 60, 128, COLOR_NEON);
    render_text(renderer, font_ui, puzzle_concepts[engine->minigame_puzzle_index], 60, 155, COLOR_NEON);

    render_text(renderer, font_ui, "Tempo restante:", 40, 200, COLOR_YELLOW);
    float time_progress = clampf(engine->minigame_time_remaining / MINIGAME_TIME_LIMIT, 0.0f, 1.0f);
    render_progress_bar(renderer, 220, 202, 450, 18, time_progress, COLOR_YELLOW, COLOR_DARK_GRN);
    char time_label[32];
    snprintf(time_label, sizeof(time_label), "%.1fs", engine->minigame_time_remaining > 0.0f ? engine->minigame_time_remaining : 0.0f);
    render_text(renderer, font_ui, time_label, 690, 202, COLOR_YELLOW);

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
    render_text(renderer, font_ui, "Falhar ou zerar o tempo custa confianca publica e conta como erro.", 40, 550, COLOR_YELLOW);
}

void render_screen_ato2_tickets(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "ATO 2 -- FILA DE CHAMADOS", 40, 35, COLOR_NEON);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 70, 980, 70);

    render_text(renderer, font_ui, "Tempo:", 780, 45, COLOR_YELLOW);
    render_progress_bar(renderer, 840, 47, 140, 16, engine->question_timer / QUESTION_TIME_LIMIT, COLOR_YELLOW, COLOR_DARK_GRN);

    SDL_Rect left_panel = {40, 85, 440, 490};
    draw_box(renderer, left_panel, COLOR_DARK_GRN, 1);
    render_text(renderer, font_ui, "FILA DE CHAMADOS:", 55, 95, COLOR_NEON);

    for (int i = 0; i < NUM_TICKETS; i++) {
        bool is_current = (i == engine->current_ticket_index);
        Ticket *t = &engine->tickets[engine->ticket_order[i]];
        SDL_Color line_color = COLOR_NEON;
        char status_tag[16] = "PENDENTE";
        if (t->resolved) {
            line_color = t->player_was_correct ? COLOR_NEON : COLOR_YELLOW;
            snprintf(status_tag, sizeof(status_tag), "%s", t->player_was_correct ? "OK" : "ERRO");
        }
        char line[96];
        snprintf(line, sizeof(line), "%s %s -- %s", is_current ? ">" : " ", t->id, status_tag);
        render_text(renderer, font_ui, line, 60, 125 + (i * 32), line_color);
    }

    SDL_Rect right_panel = {500, 85, 480, 490};
    draw_box(renderer, right_panel, COLOR_DARK_GRN, 1);

    if (engine->showing_feedback) {
        SDL_Color panel_color = engine->feedback_was_correct ? COLOR_NEON : COLOR_YELLOW;
        render_text(renderer, font_ui, engine->feedback_was_correct ? "CASO RESOLVIDO" : "CLASSIFICACAO INCORRETA", 515, 100, panel_color);
        multi_line_render(renderer, font_ui, engine->feedback_narrative, 515, 140, 440, 24, COLOR_NEON, engine->typewriter_index);
        char delta_line[96];
        snprintf(delta_line, sizeof(delta_line), "Pontos: %+d | Confianca: %+.0f%%", engine->feedback_score_delta, engine->feedback_trust_delta);
        render_text(renderer, font_ui, delta_line, 515, 480, COLOR_YELLOW);
        render_text(renderer, font_ui, "> ENTER para continuar_", 515, 540, panel_color);
    } else if (engine->current_ticket_index < NUM_TICKETS) {
        Ticket *t = &engine->tickets[engine->ticket_order[engine->current_ticket_index]];
        char header[32];
        snprintf(header, sizeof(header), "CHAMADO %s", t->id);
        render_text(renderer, font_ui, header, 515, 95, COLOR_NEON);
        multi_line_render(renderer, font_ui, t->description, 515, 125, 440, 22, COLOR_NEON, (int)strlen(t->description));

        render_text(renderer, font_ui, "Classifique este chamado:", 515, 280, COLOR_YELLOW);
        for (int i = 0; i < 3; i++) {
            SDL_Rect opt_rect = {515, 310 + (i * 55), 450, 45};
            bool is_selected = (engine->selected_ticket_category == i);
            char label[64];
            snprintf(label, sizeof(label), "%c) %s", 'A' + i, t->category_options[i]);

            if (is_selected) {
                SDL_SetRenderDrawColor(renderer, COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, 255);
                SDL_RenderFillRect(renderer, &opt_rect);
                render_text(renderer, font_ui, label, 525, 323 + (i * 55), COLOR_TEXT_BLK);
            } else {
                draw_box(renderer, opt_rect, COLOR_DARK_GRN, 1);
                render_text(renderer, font_ui, label, 525, 323 + (i * 55), COLOR_NEON);
            }
        }
    }

    render_text(renderer, font_ui, "> SETAS para escolher, ENTER para confirmar_", 40, 585, COLOR_NEON);
}

void render_screen_dashboard(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "RESUMO DO DIA", 40, 35, COLOR_NEON);
    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 90, 980, 90);

    SDL_Rect box = {40, 130, 940, 340};
    draw_box(renderer, box, COLOR_DARK_GRN, 1);

    char line1[96];
    snprintf(line1, sizeof(line1), "Chamados processados ate agora: %d / %d", engine->dashboard_tickets_processed_snapshot, NUM_TICKETS);
    render_text(renderer, font_ui, line1, 60, 160, COLOR_NEON);

    char line2[96];
    snprintf(line2, sizeof(line2), "Variacao da Confianca Publica no periodo: %+.0f%%", engine->dashboard_trust_delta_period);
    render_text(renderer, font_ui, line2, 60, 200, engine->dashboard_trust_delta_period >= 0.0f ? COLOR_NEON : COLOR_YELLOW);

    char line3[96];
    snprintf(line3, sizeof(line3), "Confianca Publica atual: %.0f%%", engine->public_trust);
    render_text(renderer, font_ui, line3, 60, 240, COLOR_NEON);
    render_progress_bar(renderer, 60, 265, 400, 18, engine->public_trust / 100.0f, COLOR_NEON, COLOR_DARK_GRN);

    char line4[96];
    snprintf(line4, sizeof(line4), "Clima Social Geral: %s", get_social_climate_label(engine->public_trust));
    render_text(renderer, font_ui, line4, 60, 310, COLOR_YELLOW);

    render_text(renderer, font_ui, "> Pressione ENTER para continuar_", 40, 560, COLOR_NEON);
}

void render_screen_ato3_escandalo(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_ALERT_RED, 3);

    render_text(renderer, font_title, "ATO 3 -- ESCANDALO PUBLICO", 40, 35, COLOR_ALERT_RED);

    SDL_SetRenderDrawColor(renderer, COLOR_DARK_RED.r, COLOR_DARK_RED.g, COLOR_DARK_RED.b, 255);
    SDL_RenderDrawLine(renderer, 40, 80, 980, 80);

    SDL_Rect narrative_box = {40, 100, 940, 130};
    draw_box(renderer, narrative_box, COLOR_DARK_RED, 2);
    int narrative_lines = multi_line_render(renderer, font_ui, ATO3_NARRATIVE_TEXT, 60, 120, 900, 22, COLOR_ALERT_RED, engine->typewriter_index);
    (void)narrative_lines;

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

void render_screen_bias_minigame(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    render_text(renderer, font_title, "CURADORIA DO DATASET DE TREINO", 40, 35, COLOR_NEON);
    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 70, 980, 70);

    multi_line_render(renderer, font_ui, BIAS_MINIGAME_INTRO_TEXT, 40, 85, 940, 20, COLOR_NEON, (int)strlen(BIAS_MINIGAME_INTRO_TEXT));

    if (!engine->dataset_submitted) {
        for (int i = 0; i < NUM_DATASET_POINTS; i++) {
            SDL_Rect row = {40, 165 + (i * 42), 940, 36};
            bool is_selected = (engine->dataset_selected_index == i);
            char label[128];
            snprintf(label, sizeof(label), "[%s] %s -- %s",
                     is_selected ? "*" : " ",
                     engine->dataset[i].included ? "INCLUIDO" : "EXCLUIDO",
                     engine->dataset[i].label);
            SDL_Color row_color = engine->dataset[i].included ? COLOR_NEON : COLOR_DARK_GRN;
            if (is_selected) {
                SDL_SetRenderDrawColor(renderer, row_color.r, row_color.g, row_color.b, 255);
                SDL_RenderFillRect(renderer, &row);
                render_text(renderer, font_ui, label, 55, 173 + (i * 42), COLOR_TEXT_BLK);
            } else {
                draw_box(renderer, row, row_color, 1);
                render_text(renderer, font_ui, label, 55, 173 + (i * 42), row_color);
            }
        }
        render_text(renderer, font_ui, "> SETAS para navegar, ESPACO para incluir/excluir, ENTER para treinar o modelo_", 40, 560, COLOR_NEON);
    } else {
        SDL_Rect result_box = {40, 170, 940, 320};
        draw_box(renderer, result_box, COLOR_DARK_GRN, 2);
        render_text(renderer, font_ui, "RESULTADO DA CURADORIA:", 60, 190, COLOR_YELLOW);

        char err_line[64];
        snprintf(err_line, sizeof(err_line), "Taxa de erro estimada do modelo: %.0f%%", engine->dataset_error_rate * 100.0f);
        render_text(renderer, font_ui, err_line, 60, 230, COLOR_NEON);
        render_progress_bar(renderer, 60, 255, 400, 18, 1.0f - engine->dataset_error_rate, COLOR_NEON, COLOR_DARK_GRN);

        char delta_line[64];
        snprintf(delta_line, sizeof(delta_line), "Impacto na Confianca Publica: %+.0f%%", engine->dataset_trust_delta);
        render_text(renderer, font_ui, delta_line, 60, 300, engine->dataset_trust_delta >= 0.0f ? COLOR_NEON : COLOR_YELLOW);

        const char *verdict = (engine->dataset_error_rate <= 0.25f)
            ? "Curadoria excelente: o dataset ficou bem mais representativo."
            : (engine->dataset_error_rate <= 0.5f)
                ? "Curadoria mediana: ainda ha vies residual no dataset."
                : "Curadoria falha: o dataset segue enviesado.";
        render_text(renderer, font_ui, verdict, 60, 340, COLOR_YELLOW);

        render_text(renderer, font_ui, "> Pressione ENTER para continuar_", 60, 560, COLOR_NEON);
    }
}

void render_screen_ato4_desfecho(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    int tier = get_ending_tier_index(engine->public_trust);

    render_text(renderer, font_title, "ATO 4 -- DESFECHO", 40, 35, COLOR_NEON);
    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 75, 980, 75);

    char title_line[64];
    snprintf(title_line, sizeof(title_line), "DESFECHO: %s", ending_titles[tier]);
    render_text(renderer, font_ui, title_line, 40, 90, COLOR_YELLOW);

    SDL_Rect narrative_box = {40, 120, 940, 90};
    draw_box(renderer, narrative_box, COLOR_DARK_GRN, 1);
    multi_line_render(renderer, font_ui, ending_narratives[tier], 55, 130, 910, 20, COLOR_NEON, engine->typewriter_index);

    SDL_Rect report_box = {40, 220, 940, 290};
    draw_box(renderer, report_box, COLOR_DARK_GRN, 1);
    render_text(renderer, font_ui, "RESUMO ESTATISTICO:", 60, 235, COLOR_YELLOW);

    char l1[96]; snprintf(l1, sizeof(l1), "Pontuacao final: %d", engine->score);
    render_text(renderer, font_ui, l1, 60, 265, COLOR_NEON);

    char l2[96]; snprintf(l2, sizeof(l2), "Acertos: %d / %d respondidos", engine->total_correct, engine->total_answered);
    render_text(renderer, font_ui, l2, 60, 290, COLOR_NEON);

    float avg_time = (engine->total_answered > 0) ? (engine->total_response_time_sum / (float)engine->total_answered) : 0.0f;
    char l3[96]; snprintf(l3, sizeof(l3), "Tempo medio de resposta: %.1fs", avg_time);
    render_text(renderer, font_ui, l3, 60, 315, COLOR_NEON);

    char l4[96]; snprintf(l4, sizeof(l4), "Erros totais: %d / %d", engine->total_errors, MAX_TOTAL_ERRORS);
    render_text(renderer, font_ui, l4, 60, 340, COLOR_NEON);

    char l5[96]; snprintf(l5, sizeof(l5), "Decisao etica no Ato 3: %s",
                           engine->decision_ato3_label[0] ? engine->decision_ato3_label : "(nao chegou ao Ato 3)");
    render_text(renderer, font_ui, l5, 60, 365, COLOR_NEON);

    char l6[96]; snprintf(l6, sizeof(l6), "Confianca publica final: %.1f / 100", engine->public_trust);
    render_text(renderer, font_ui, l6, 60, 390, COLOR_NEON);
    render_progress_bar(renderer, 60, 415, 400, 18, engine->public_trust / 100.0f, COLOR_NEON, COLOR_DARK_GRN);

    if (engine->game_over_triggered) {
        render_text(renderer, font_ui, "(Missao encerrada antes do previsto.)", 60, 445, COLOR_YELLOW);
    }

    render_text(renderer, font_ui, "Arquivos gravados: certificado_conclusao.txt e save_slotN.dat", 60, 470, COLOR_NEON);
    render_text(renderer, font_ui, "> Pressione ENTER para voltar ao menu, ou ESC para sair_", 40, 570, COLOR_NEON);
}

void render_pause_overlay(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    SDL_Rect box = {262, 200, 500, 240};
    SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderFillRect(renderer, &box);
    draw_box(renderer, box, COLOR_NEON, 2);

    render_text(renderer, font_title, "PAUSA", 440, 215, COLOR_NEON);

    char audio_label[32];
    snprintf(audio_label, sizeof(audio_label), "Audio: %s", engine->audio_enabled ? "LIGADO" : "DESLIGADO");
    const char *options[] = {"Continuar", audio_label, "Salvar e Sair"};
    for (int i = 0; i < 3; i++) {
        SDL_Rect r = get_pause_button_rect(i);
        render_button(renderer, font_ui, options[i], r.x, r.y, r.w, r.h,
                       (engine->selected_pause_option == i), COLOR_NEON, COLOR_TEXT_BLK);
    }
}

/* ============================================================================
 * HANDLERS DE SELECAO (compartilhados entre teclado e mouse)
 * ============================================================================ */
void handle_menu_selection(GameEngine *engine, int option_index) {
    if (option_index == 0) {
        engine->slot_mode = SLOT_MODE_NEW;
        engine->selected_slot_index = 0;
        refresh_slot_previews(engine);
        engine->state = STATE_SLOT_SELECT;
    } else if (option_index == 1) {
        engine->slot_mode = SLOT_MODE_LOAD;
        engine->selected_slot_index = 0;
        refresh_slot_previews(engine);
        engine->state = STATE_SLOT_SELECT;
    } else if (option_index == 2) {
        engine->audio_enabled = !engine->audio_enabled;
        apply_audio_state(engine);
    } else if (option_index == 4) {
        engine->running = false;
    }
    /* option_index == 3 ([4] Creditos): sem acao por enquanto */
}

void handle_slot_selection(GameEngine *engine, int option_index) {
    if (option_index == NUM_SAVE_SLOTS) {
        engine->state = STATE_MENU;
        return;
    }
    SaveSlotData *s = &engine->slot_previews[option_index];
    if (engine->slot_mode == SLOT_MODE_NEW) {
        if (s->occupied) {
            engine->pending_new_game_slot = option_index;
            engine->selected_overwrite_option = 0;
            engine->state = STATE_OVERWRITE_CONFIRM;
        } else {
            reset_progress_for_new_game(engine);
            engine->active_slot = option_index;
            reset_quiz_state(engine);
            engine->state = STATE_QUIZ_ATO1;
            autosave(engine);
        }
    } else { /* SLOT_MODE_LOAD */
        if (s->occupied) {
            load_game_from_slot(engine, option_index);
        }
        /* slot vazio em modo Continuar: nao faz nada */
    }
}

void handle_overwrite_confirm(GameEngine *engine, int option_index) {
    if (option_index == 0) { /* Sim, sobrescrever */
        reset_progress_for_new_game(engine);
        engine->active_slot = engine->pending_new_game_slot;
        reset_quiz_state(engine);
        engine->state = STATE_QUIZ_ATO1;
        autosave(engine);
    } else { /* Nao, voltar */
        engine->state = STATE_SLOT_SELECT;
    }
}

void handle_pause_selection(GameEngine *engine, int option_index) {
    if (option_index == 0) {
        engine->paused = false;
    } else if (option_index == 1) {
        engine->audio_enabled = !engine->audio_enabled;
        apply_audio_state(engine);
    } else if (option_index == 2) {
        autosave(engine);
        engine->state = STATE_MENU;
        engine->paused = false;
    }
}

/* ============================================================================
 * LOOP PRINCIPAL
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
        LOGICAL_W, LOGICAL_H, SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderer, LOGICAL_W, LOGICAL_H);

    TTF_Font *font_ui = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
    TTF_Font *font_title = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 28);
    if (!font_ui) font_ui = TTF_OpenFont("C:\\Windows\\Fonts\\consola.ttf", 14);
    if (!font_title) font_title = TTF_OpenFont("C:\\Windows\\Fonts\\consolab.ttf", 28);

    GameEngine engine;
    memset(&engine, 0, sizeof(GameEngine));
    engine.state = STATE_MENU;
    engine.running = true;
    engine.is_fullscreen = true;
    engine.active_slot = -1;
    engine.selected_menu_option = 0;
    engine.audio_enabled = true;
    reset_quiz_state(&engine); /* estado de navegacao inicial, sem efeito ate iniciar jogo */

    init_audio(&engine);

    Uint32 last_tick = SDL_GetTicks();

    SDL_Event e;
    while (engine.running) {
        Uint32 current_tick = SDL_GetTicks();
        float dt = (current_tick - last_tick) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        last_tick = current_tick;

        /* -------------------- EVENTOS -------------------- */
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) engine.running = false;

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_F11) {
                    engine.is_fullscreen = !engine.is_fullscreen;
                    SDL_SetWindowFullscreen(window, engine.is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                } else if (engine.paused) {
                    if (e.key.keysym.sym == SDLK_UP) engine.selected_pause_option = (engine.selected_pause_option - 1 + 3) % 3;
                    if (e.key.keysym.sym == SDLK_DOWN) engine.selected_pause_option = (engine.selected_pause_option + 1) % 3;
                    if (e.key.keysym.sym == SDLK_RETURN) handle_pause_selection(&engine, engine.selected_pause_option);
                    if (e.key.keysym.sym == SDLK_ESCAPE) engine.paused = false;
                } else {
                    bool is_pausable = (engine.state == STATE_QUIZ_ATO1 || engine.state == STATE_MINIGAME_LOCK ||
                                         engine.state == STATE_ATO2_TICKETS || engine.state == STATE_ATO3_ESCANDALO ||
                                         engine.state == STATE_BIAS_MINIGAME);
                    if (is_pausable && e.key.keysym.sym == SDLK_ESCAPE) {
                        engine.paused = true;
                        engine.selected_pause_option = 0;
                    } else {
                        switch (engine.state) {

                        case STATE_MENU:
                            if (e.key.keysym.sym == SDLK_UP) engine.selected_menu_option = (engine.selected_menu_option - 1 + 5) % 5;
                            if (e.key.keysym.sym == SDLK_DOWN) engine.selected_menu_option = (engine.selected_menu_option + 1) % 5;
                            if (e.key.keysym.sym == SDLK_RETURN) handle_menu_selection(&engine, engine.selected_menu_option);
                            break;

                        case STATE_SLOT_SELECT:
                            if (e.key.keysym.sym == SDLK_UP) engine.selected_slot_index = (engine.selected_slot_index - 1 + (NUM_SAVE_SLOTS + 1)) % (NUM_SAVE_SLOTS + 1);
                            if (e.key.keysym.sym == SDLK_DOWN) engine.selected_slot_index = (engine.selected_slot_index + 1) % (NUM_SAVE_SLOTS + 1);
                            if (e.key.keysym.sym == SDLK_RETURN) handle_slot_selection(&engine, engine.selected_slot_index);
                            if (e.key.keysym.sym == SDLK_ESCAPE) engine.state = STATE_MENU;
                            break;

                        case STATE_OVERWRITE_CONFIRM:
                            if (e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_DOWN)
                                engine.selected_overwrite_option = (engine.selected_overwrite_option + 1) % 2;
                            if (e.key.keysym.sym == SDLK_RETURN) handle_overwrite_confirm(&engine, engine.selected_overwrite_option);
                            if (e.key.keysym.sym == SDLK_ESCAPE) engine.state = STATE_SLOT_SELECT;
                            break;

                        case STATE_QUIZ_ATO1:
                            if (engine.showing_feedback) {
                                if (e.key.keysym.sym == SDLK_RETURN) {
                                    engine.showing_feedback = false;
                                    if (!check_game_over(&engine)) {
                                        engine.current_question_index++;
                                        engine.selected_quiz_option = 0;
                                        engine.question_timer = QUESTION_TIME_LIMIT;
                                        engine.typewriter_index = 0;
                                        engine.typewriter_accumulator = 0.0f;
                                        if (engine.current_question_index >= QUIZ_BANK_SIZE) {
                                            reset_minigame_state(&engine);
                                            engine.state = STATE_MINIGAME_LOCK;
                                        }
                                        autosave(&engine);
                                    }
                                }
                            } else {
                                if (e.key.keysym.sym == SDLK_UP) engine.selected_quiz_option = (engine.selected_quiz_option - 1 + 4) % 4;
                                if (e.key.keysym.sym == SDLK_DOWN) engine.selected_quiz_option = (engine.selected_quiz_option + 1) % 4;
                                if (e.key.keysym.sym == SDLK_RETURN) resolve_quiz_answer(&engine, false);
                            }
                            break;

                        case STATE_MINIGAME_LOCK:
                            if (e.key.keysym.sym == SDLK_UP) engine.minigame_selected_index = (engine.minigame_selected_index - 1 + NUM_COUNTRIES) % NUM_COUNTRIES;
                            if (e.key.keysym.sym == SDLK_DOWN) engine.minigame_selected_index = (engine.minigame_selected_index + 1) % NUM_COUNTRIES;
                            if (e.key.keysym.sym == SDLK_SPACE) {
                                engine.countries[engine.minigame_selected_index].status = !engine.countries[engine.minigame_selected_index].status;
                            }
                            if (e.key.keysym.sym == SDLK_RETURN) {
                                PuzzleEvalFn eval_fn = puzzle_evaluators[engine.minigame_puzzle_index];
                                bool solved = eval_fn(engine.countries[0].status, engine.countries[1].status,
                                                       engine.countries[2].status, engine.countries[3].status);
                                if (solved) {
                                    engine.target_trust = clampf(engine.target_trust + 10.0f, 0.0f, 100.0f);
                                    spawn_floating_text(&engine, "ACESSO LIBERADO! +10 CONFIANCA", 640, 250, COLOR_NEON);
                                    reset_ato2_state(&engine);
                                    engine.state = STATE_ATO2_TICKETS;
                                    autosave(&engine);
                                } else {
                                    engine.target_trust = clampf(engine.target_trust - 10.0f, 0.0f, 100.0f);
                                    engine.total_errors++;
                                    trigger_screen_shake(&engine, 400);
                                    spawn_floating_text(&engine, "COMBINACAO INVALIDA -10", 640, 250, COLOR_YELLOW);
                                    check_game_over(&engine);
                                }
                            }
                            break;

                        case STATE_ATO2_TICKETS:
                            if (engine.showing_feedback) {
                                if (e.key.keysym.sym == SDLK_RETURN) {
                                    engine.showing_feedback = false;
                                    if (!check_game_over(&engine)) {
                                        engine.current_ticket_index++;
                                        engine.selected_ticket_category = 0;
                                        engine.question_timer = QUESTION_TIME_LIMIT;
                                        engine.typewriter_index = 0;
                                        engine.typewriter_accumulator = 0.0f;
                                        autosave(&engine);

                                        engine.tickets_since_last_dashboard++;
                                        bool queue_finished = (engine.current_ticket_index >= NUM_TICKETS);
                                        if (engine.tickets_since_last_dashboard >= engine.dashboard_interval || queue_finished) {
                                            engine.dashboard_tickets_processed_snapshot = engine.current_ticket_index;
                                            engine.dashboard_trust_delta_period = engine.target_trust - engine.dashboard_trust_at_period_start;
                                            engine.dashboard_trust_at_period_start = engine.target_trust;
                                            engine.tickets_since_last_dashboard = 0;
                                            engine.dashboard_interval = 3 + (rand() % 3);
                                            engine.state_after_dashboard = queue_finished ? STATE_ATO3_ESCANDALO : STATE_ATO2_TICKETS;
                                            if (queue_finished) reset_ato3_state(&engine);
                                            engine.state = STATE_DASHBOARD;
                                        }
                                    }
                                }
                            } else if (engine.current_ticket_index < NUM_TICKETS) {
                                if (e.key.keysym.sym == SDLK_UP) engine.selected_ticket_category = (engine.selected_ticket_category - 1 + 3) % 3;
                                if (e.key.keysym.sym == SDLK_DOWN) engine.selected_ticket_category = (engine.selected_ticket_category + 1) % 3;
                                if (e.key.keysym.sym == SDLK_RETURN) resolve_ticket_answer(&engine, false);
                            }
                            break;

                        case STATE_DASHBOARD:
                            if (e.key.keysym.sym == SDLK_RETURN) engine.state = engine.state_after_dashboard;
                            break;

                        case STATE_ATO3_ESCANDALO: {
                            bool narrative_fully_revealed = (engine.typewriter_index >= (int)strlen(ATO3_NARRATIVE_TEXT));
                            if (narrative_fully_revealed && !engine.scandal_resolved) {
                                if (e.key.keysym.sym == SDLK_UP) engine.selected_scandal_option = (engine.selected_scandal_option - 1 + 3) % 3;
                                if (e.key.keysym.sym == SDLK_DOWN) engine.selected_scandal_option = (engine.selected_scandal_option + 1) % 3;

                                if (e.key.keysym.sym == SDLK_RETURN) {
                                    const char *ato3_labels[3] = {"Negacao publica", "Transparencia e correcao", "Culpa terceirizada"};
                                    float trust_delta;
                                    const char *feedback;
                                    if (engine.selected_scandal_option == 0) { trust_delta = -20.0f; feedback = "NEGACAO PUBLICA -20 CONFIANCA"; }
                                    else if (engine.selected_scandal_option == 1) { trust_delta = 15.0f; feedback = "TRANSPARENCIA E CORRECAO +15 CONFIANCA"; }
                                    else { trust_delta = -10.0f; feedback = "CULPA TERCEIRIZADA -10 CONFIANCA"; }

                                    engine.target_trust = clampf(engine.target_trust + trust_delta, 0.0f, 100.0f);
                                    if (trust_delta < 0.0f) trigger_screen_shake(&engine, 400);
                                    spawn_floating_text(&engine, feedback, 500, 300, COLOR_ALERT_RED);

                                    strncpy(engine.decision_ato3_label, ato3_labels[engine.selected_scandal_option], sizeof(engine.decision_ato3_label) - 1);
                                    engine.decision_ato3_label[sizeof(engine.decision_ato3_label) - 1] = '\0';
                                    engine.scandal_resolved = true;

                                    if (!check_game_over(&engine)) {
                                        reset_bias_minigame_state(&engine);
                                        engine.state = STATE_BIAS_MINIGAME;
                                        autosave(&engine);
                                    }
                                }
                            }
                            break;
                        }

                        case STATE_BIAS_MINIGAME:
                            if (!engine.dataset_submitted) {
                                if (e.key.keysym.sym == SDLK_UP) engine.dataset_selected_index = (engine.dataset_selected_index - 1 + NUM_DATASET_POINTS) % NUM_DATASET_POINTS;
                                if (e.key.keysym.sym == SDLK_DOWN) engine.dataset_selected_index = (engine.dataset_selected_index + 1) % NUM_DATASET_POINTS;
                                if (e.key.keysym.sym == SDLK_SPACE) engine.dataset[engine.dataset_selected_index].included = !engine.dataset[engine.dataset_selected_index].included;
                                if (e.key.keysym.sym == SDLK_RETURN) compute_bias_minigame_result(&engine);
                            } else {
                                if (e.key.keysym.sym == SDLK_RETURN) {
                                    if (!check_game_over(&engine)) {
                                        reset_ato4_state(&engine);
                                        engine.state = STATE_ATO4_DESFECHO;
                                        autosave(&engine);
                                    }
                                }
                            }
                            break;

                        case STATE_ATO4_DESFECHO:
                            if (e.key.keysym.sym == SDLK_RETURN) {
                                engine.state = STATE_MENU;
                                engine.selected_menu_option = 0;
                                engine.active_slot = -1;
                            }
                            if (e.key.keysym.sym == SDLK_ESCAPE) engine.running = false;
                            break;
                        }
                    }
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                float logical_x = 0.0f, logical_y = 0.0f;
                SDL_RenderWindowToLogical(renderer, e.button.x, e.button.y, &logical_x, &logical_y);
                int content_x = (int)logical_x - CONTENT_OFFSET_X;
                int content_y = (int)logical_y - CONTENT_OFFSET_Y;

                if (engine.paused) {
                    for (int i = 0; i < 3; i++) {
                        SDL_Rect r = get_pause_button_rect(i);
                        if (content_x >= r.x && content_x <= r.x + r.w && content_y >= r.y && content_y <= r.y + r.h) {
                            engine.selected_pause_option = i;
                            handle_pause_selection(&engine, i);
                        }
                    }
                } else if (engine.state == STATE_MENU) {
                    for (int i = 0; i < 5; i++) {
                        SDL_Rect r = get_menu_button_rect(i);
                        if (content_x >= r.x && content_x <= r.x + r.w && content_y >= r.y && content_y <= r.y + r.h) {
                            engine.selected_menu_option = i;
                            handle_menu_selection(&engine, i);
                        }
                    }
                }
            }
        }

        /* -------------------- ATUALIZACAO DE LOGICA -------------------- */
        if (!engine.paused) {
            update_trust_interpolation(&engine, dt);
            update_floating_texts(&engine, dt);

            if (engine.shake_timer > 0) {
                engine.shake_timer -= (int)(dt * 1000.0f);
                if (engine.shake_timer < 0) engine.shake_timer = 0;
            }

            if (engine.state == STATE_QUIZ_ATO1 && !engine.showing_feedback) {
                engine.question_timer -= dt;
                if (engine.question_timer <= 0.0f) { engine.question_timer = 0.0f; resolve_quiz_answer(&engine, true); }
            }
            if (engine.state == STATE_ATO2_TICKETS && !engine.showing_feedback && engine.current_ticket_index < NUM_TICKETS) {
                engine.question_timer -= dt;
                if (engine.question_timer <= 0.0f) { engine.question_timer = 0.0f; resolve_ticket_answer(&engine, true); }
            }
            if (engine.state == STATE_MINIGAME_LOCK) {
                engine.minigame_time_remaining -= dt;
                if (engine.minigame_time_remaining <= 0.0f) {
                    trigger_screen_shake(&engine, 400);
                    engine.target_trust = clampf(engine.target_trust - 10.0f, 0.0f, 100.0f);
                    engine.total_errors++;
                    spawn_floating_text(&engine, "TEMPO ESGOTADO! -10 CONFIANCA", 640, 250, COLOR_YELLOW);
                    reset_minigame_state(&engine);
                    check_game_over(&engine);
                }
            }

            bool uses_typewriter = (engine.state == STATE_ATO2_TICKETS && !engine.showing_feedback) ||
                                    (engine.state == STATE_ATO3_ESCANDALO) ||
                                    (engine.state == STATE_BIAS_MINIGAME && !engine.dataset_submitted) ||
                                    (engine.state == STATE_ATO4_DESFECHO) ||
                                    (engine.state == STATE_QUIZ_ATO1 && engine.showing_feedback) ||
                                    (engine.state == STATE_ATO2_TICKETS && engine.showing_feedback);
            if (uses_typewriter) {
                const float typewriter_speed = 0.02f;
                engine.typewriter_accumulator += dt;
                while (engine.typewriter_accumulator >= typewriter_speed) {
                    engine.typewriter_accumulator -= typewriter_speed;
                    engine.typewriter_index++;
                }
            }

            update_adaptive_music(&engine);

            if (engine.state == STATE_ATO4_DESFECHO && !engine.ato4_files_written) {
                write_certificado(&engine);
                autosave(&engine);
                engine.ato4_files_written = true;
            }
        }

        /* -------------------- RENDERIZACAO -------------------- */
        SDL_RenderSetViewport(renderer, NULL);
        SDL_Color bg_color = (engine.state == STATE_ATO3_ESCANDALO) ? COLOR_BG_RED : COLOR_BG;
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, 255);
        SDL_RenderClear(renderer);

        /* Moldura/bezel decorativo na margem criada pela resolucao logica 1280x720 */
        SDL_Rect full_frame = {8, 8, LOGICAL_W - 16, LOGICAL_H - 16};
        draw_box(renderer, full_frame, COLOR_DARK_GRN, 1);
        render_text(renderer, font_ui, "F11: TELA CHEIA  |  ESC: PAUSE  |  AUDIO:", 20, LOGICAL_H - 24, COLOR_DARK_GRN);
        render_text(renderer, font_ui, engine.audio_enabled ? "LIGADO" : "DESLIGADO", 330, LOGICAL_H - 24, COLOR_DARK_GRN);

        int shake_x = 0, shake_y = 0;
        apply_screen_shake(&shake_x, &shake_y, engine.shake_timer);
        SDL_Rect content_viewport = {CONTENT_OFFSET_X + shake_x, CONTENT_OFFSET_Y + shake_y, WINDOW_W, WINDOW_H};
        SDL_RenderSetViewport(renderer, &content_viewport);

        switch (engine.state) {
            case STATE_MENU:              render_screen_menu(renderer, font_title, font_ui, &engine); break;
            case STATE_SLOT_SELECT:       render_screen_slot_select(renderer, font_title, font_ui, &engine); break;
            case STATE_OVERWRITE_CONFIRM: render_screen_overwrite_confirm(renderer, font_title, font_ui, &engine); break;
            case STATE_QUIZ_ATO1:         render_screen_quiz(renderer, font_title, font_ui, &engine); break;
            case STATE_MINIGAME_LOCK:     render_screen_minigame_lock(renderer, font_title, font_ui, &engine); break;
            case STATE_ATO2_TICKETS:      render_screen_ato2_tickets(renderer, font_title, font_ui, &engine); break;
            case STATE_DASHBOARD:         render_screen_dashboard(renderer, font_title, font_ui, &engine); break;
            case STATE_ATO3_ESCANDALO:    render_screen_ato3_escandalo(renderer, font_title, font_ui, &engine); break;
            case STATE_BIAS_MINIGAME:     render_screen_bias_minigame(renderer, font_title, font_ui, &engine); break;
            case STATE_ATO4_DESFECHO:     render_screen_ato4_desfecho(renderer, font_title, font_ui, &engine); break;
        }

        render_floating_texts(renderer, font_ui, &engine);
        if (engine.paused) render_pause_overlay(renderer, font_title, font_ui, &engine);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    shutdown_audio(&engine);
    if (font_ui) TTF_CloseFont(font_ui);
    if (font_title) TTF_CloseFont(font_title);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

/* ============================================================================
 * COMPILACAO (MSYS2 / MinGW-w64):
 *
 * gcc aplicativo.c -I C:\msys64\ucrt64\include -L C:\msys64\ucrt64\lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_mixer -lm -o decifra_game
 * ============================================================================ */