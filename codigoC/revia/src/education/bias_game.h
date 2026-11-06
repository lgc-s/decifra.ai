/* Minigame de viés algorítmico: o jogador monta o conjunto de treino do
 * "crédito da NÓVA" e vê, em uma simulação, como os dados produzem (ou não)
 * resultados injustos entre o Bairro Norte (A) e o Bairro Sul (B).
 *
 * Modelo simplificado e determinístico, pensado para ensinar:
 *  - o histórico rotula o Bairro Sul com mais rigor (viés nos rótulos);
 *  - o modelo aprende a taxa de aprovação de cada grupo e a soma ao escore;
 *  - remover a coluna 'bairro' não basta se o CEP (proxy) continua;
 *  - corrigir os rótulos históricos elimina a causa do viés.
 */
#ifndef BIAS_GAME_H
#define BIAS_GAME_H

#include <stdbool.h>
#include <stdint.h>

#define BIAS_POOL 40
#define BIAS_TEST 30
#define BIAS_MAX_DISPARITY 15.0f    /* pontos percentuais */
#define BIAS_SEED 2025u             /* semente fixa: o caso é sempre o mesmo */
#define BIAS_MIN_ACCURACY  80.0f    /* % */

typedef struct {
    int q;                 /* qualificação 0..100 (mesma distribuição nos dois grupos) */
    int noise;             /* ruído do "mundo real" aplicado ao rótulo correto */
    bool hist_label;       /* rótulo histórico (viciado contra o grupo B) */
} BiasRecord;

typedef struct {
    bool  ran;
    float train_rate_a, train_rate_b;   /* % de aprovados nos dados de treino */
    float rate_a, rate_b;               /* % de aprovados pelo modelo no teste */
    float disparity;                    /* |rate_a - rate_b| em pontos percentuais */
    float accuracy;                     /* % de acerto contra o rótulo correto */
    bool  solved;
} BiasResult;

typedef struct {
    int  n_a, n_b;          /* amostras de cada grupo no treino (0..BIAS_POOL) */
    bool use_group;         /* usar a coluna 'bairro' */
    bool use_proxy;         /* usar o CEP (proxy do bairro, correlação 70%) */
    bool fix_labels;        /* corrigir os rótulos históricos */
    BiasRecord pool_a[BIAS_POOL], pool_b[BIAS_POOL];
    BiasRecord test_a[BIAS_TEST], test_b[BIAS_TEST];
    BiasResult result;
} BiasGame;

void bias_init(BiasGame *g, uint32_t seed);
void bias_run(BiasGame *g);

/* Taxa de aprovação (%) nos dados de treino selecionados, conforme os rótulos escolhidos. */
float bias_train_rate(const BiasGame *g, bool group_b);

/* Rótulo (aprovado?) do exemplo idx do grupo, conforme "corrigir rótulos" estiver ligado. */
bool bias_label(const BiasGame *g, bool group_b, int idx);

#endif
