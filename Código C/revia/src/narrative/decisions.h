/* Cálculo dos finais a partir das decisões acumuladas. */
#ifndef DECISIONS_H
#define DECISIONS_H

#include "systems/progression.h"

#define NUM_ENDINGS 4

typedef enum {
    ENDING_CLARITY = 0,   /* confiança alta e viés corrigido */
    ENDING_REPAIR,        /* reparo lento */
    ENDING_DISTRUST,      /* desconfiança */
    ENDING_SILENCE        /* confiança perdida */
} EndingId;

typedef struct {
    const char *title;
    const char *lines[3];
} EndingInfo;

int decisions_good_count(const Progress *p);          /* chamados em que escolheu a melhor opção */
int decisions_compute_ending(const Progress *p);      /* EndingId */
const EndingInfo *ending_info(int id);

#endif
