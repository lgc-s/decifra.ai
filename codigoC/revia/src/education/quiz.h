/* Sistema de perguntas: 4 sessões de QUIZ_LEN questões, dificuldade crescente.
 * Errar NÃO reinicia a sessão: a questão conta como errada e o jogador segue até o fim.
 * Ao terminar, a Confiança Pública recebe a soma: acertos * QUIZ_TRUST_RIGHT - erros * QUIZ_TRUST_WRONG.
 * Conteúdo original do projeto: confira/substitua pelo banco do repositório original. */
#ifndef QUIZ_H
#define QUIZ_H

#include <stdbool.h>
#include <stdint.h>
#include "systems/progression.h"

#define QUIZ_POOL 12          /* o banco de cada sessão tem 12; cada tentativa sorteia QUIZ_LEN */
#define QUIZ_TRUST_RIGHT 2    /* Confiança Pública ganha por acerto */
#define QUIZ_TRUST_WRONG 3    /* Confiança Pública perdida por erro */

typedef struct {
    const char *question;
    const char *options[4];   /* options[0] é SEMPRE a correta; a ordem é embaralhada na exibição */
    const char *explanation;
} QuizQuestion;

typedef struct {
    int session;                       /* 0..3 */
    int pool_idx[QUIZ_LEN];            /* questões escolhidas (em ordem crescente de dificuldade) */
    int perm[QUIZ_LEN][4];             /* perm[q][i] = índice original da opção exibida na posição i */
    int cur;                           /* questão atual 0..QUIZ_LEN-1 */
    int correct_count;
    int wrong_count;
    bool answered;                     /* já respondeu a questão atual */
    bool last_correct;                 /* a última resposta estava certa? */
    bool finished;                     /* respondeu todas as questões */
    uint16_t correct_mask;             /* bit q = acertou a questão q */
    uint16_t wrong_mask;               /* bit q = errou a questão q */
    uint32_t rng;
} QuizRun;

void quiz_start(QuizRun *r, int session, uint32_t seed);
/* Igual a quiz_start, mas a dificuldade do jogo (0 normal, 1 assistido, 2 desafio) muda o sorteio:
 * assistido sorteia entre as questões mais fáceis, desafio entre as mais difíceis. */
void quiz_start_diff(QuizRun *r, int session, uint32_t seed, int difficulty);
const QuizQuestion *quiz_current(const QuizRun *r);
const char *quiz_option_text(const QuizRun *r, int display_idx);
int  quiz_correct_display_index(const QuizRun *r);
/* Responde com a alternativa exibida na posição display_idx. Devolve se acertou. */
bool quiz_answer(QuizRun *r, int display_idx);
/* Avança para a próxima questão (ou marca como concluída). Vale para acertos e erros. */
void quiz_next(QuizRun *r);
/* Soma da sessão: acertos * QUIZ_TRUST_RIGHT - erros * QUIZ_TRUST_WRONG. */
int  quiz_trust_delta(const QuizRun *r);

const char *quiz_session_title(int session);

#endif