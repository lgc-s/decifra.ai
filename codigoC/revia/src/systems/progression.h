/* Estado persistente da campanha e regras de progressão entre atos. */
#ifndef PROGRESSION_H
#define PROGRESSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NUM_ACTS 4
#define NUM_CALLS 7
#define NUM_QUIZ_SESSIONS 4
#define QUIZ_LEN 5
#define NUM_COLLECTIBLES 12
#define TRUST_START 50

/* Tudo o que é salvo em disco. Só tipos de tamanho fixo (layout estável). */
typedef struct {
    int32_t  current_act;                       /* ato em andamento (1..4) */
    int32_t  unlocked_act;                      /* maior ato liberado */
    int32_t  trust;                             /* Confiança Pública, 0..100 */
    int32_t  difficulty;                        /* config.h: Difficulty */
    int32_t  volume;                            /* 0..10 */
    int32_t  ending;                            /* final alcançado, -1 se nenhum */
    uint32_t calls_done;                        /* bitmask de chamados concluídos */
    int8_t   call_choice[NUM_CALLS];            /* decisão tomada em cada chamado, -1 = nenhuma */
    uint32_t quiz_done;                         /* bitmask de sessões concluídas */
    uint16_t quiz_correct_mask[NUM_QUIZ_SESSIONS]; /* acertos da última tentativa (bit por questão) */
    uint8_t  quiz_attempts[NUM_QUIZ_SESSIONS];
    uint32_t collected;                         /* bitmask de colecionáveis */
    uint8_t  bias_played;
    uint8_t  bias_solved;
    uint8_t  evidence;                          /* bitmask das 3 evidências do Ato 3 */
    uint8_t  reserved;
    uint32_t deaths;
    float    play_time;                         /* segundos */
} Progress;

void progress_init(Progress *p);
void progress_add_trust(Progress *p, int delta);
int  progress_calls_done_count(const Progress *p);
int  progress_collected_count(const Progress *p);

/* Verifica se o ato pode ser concluído. Se não, escreve em msg o que falta. */
bool progress_act_ready(const Progress *p, int act, char *msg, size_t msgsz);
/* Registra a conclusão do ato (libera o seguinte). */
void progress_complete_act(Progress *p, int act);

const char *act_title(int act);          /* 1..4 */
const char *act_path(int act);           /* caminho relativo da fase */

#endif
