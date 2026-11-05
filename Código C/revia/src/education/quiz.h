/* Sistema de perguntas: 4 sessões de 10 questões, dificuldade crescente.
 * Todas as 10 precisam estar certas; ao errar, a sessão é reiniciada com
 * variações (outras questões do banco e alternativas embaralhadas).
 * Conteúdo original do projeto: confira/substitua pelo banco do repositório original. */
#ifndef QUIZ_H
#define QUIZ_H

#include <stdbool.h>
#include <stdint.h>
#include "systems/progression.h"

#define QUIZ_POOL 12   /* o banco de cada sessão tem 12; cada tentativa usa 10 */

typedef struct {
    const char *question;
    const char *options[4];   /* options[0] é SEMPRE a correta; a ordem é embaralhada na exibição */
    const char *explanation;
} QuizQuestion;

typedef struct {
    int session;                       /* 0..3 */
    int pool_idx[QUIZ_LEN];            /* questões escolhidas (em ordem crescente de dificuldade) */
    int perm[QUIZ_LEN][4];             /* perm[q][i] = índice original da opção exibida na posição i */
    int cur;                           /* questão atual 0..9 */
    int correct_count;
    bool answered;                     /* já respondeu a questão atual */
    bool last_correct;
    bool failed;                       /* errou: sessão precisa ser reiniciada */
    bool finished;                     /* acertou todas */
    uint16_t correct_mask;             /* bit q = acertou a questão q */
    uint32_t rng;
} QuizRun;

void quiz_start(QuizRun *r, int session, uint32_t seed);
const QuizQuestion *quiz_current(const QuizRun *r);
const char *quiz_option_text(const QuizRun *r, int display_idx);
int  quiz_correct_display_index(const QuizRun *r);
/* Responde com a alternativa exibida na posição display_idx. Devolve se acertou. */
bool quiz_answer(QuizRun *r, int display_idx);
/* Avança para a próxima questão (ou marca como concluída). */
void quiz_next(QuizRun *r);

const char *quiz_session_title(int session);

#endif
