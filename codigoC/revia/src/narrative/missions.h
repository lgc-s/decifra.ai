/* Chamados: problemas de cidadãos com decisões, consequências e explicação.
 * Texto original do projeto (placeholder até conferir com o repositório original). */
#ifndef MISSIONS_H
#define MISSIONS_H

#include "systems/progression.h"

typedef struct {
    const char *label;        /* texto da alternativa */
    const char *consequence;  /* o que acontece depois */
    int trust;                /* impacto na Confiança Pública */
} CallOption;

typedef struct {
    int act;
    const char *region;
    const char *title;
    const char *context;
    const char *problem;
    int nopts;                /* 2 a 4 */
    CallOption opts[4];
    const char *explanation;  /* conteúdo educativo */
} Call;

const Call *call_get(int id);

/* Registra a decisão. Devolve o impacto aplicado (0 se o chamado já estava concluído). */
int call_apply(Progress *p, int call_id, int choice);

/* Índice da melhor alternativa (maior impacto na confiança). */
int call_best_option(const Call *c);

#endif
