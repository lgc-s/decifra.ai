#include "narrative/decisions.h"
#include "narrative/missions.h"

#include <stddef.h>

int decisions_good_count(const Progress *p) {
    int n = 0;
    for (int i = 0; i < NUM_CALLS; i++) {
        const Call *c = call_get(i);
        if (c && p->call_choice[i] >= 0 && p->call_choice[i] == call_best_option(c)) n++;
    }
    return n;
}

int decisions_compute_ending(const Progress *p) {
    if (p->trust >= 75 && p->bias_solved) return ENDING_CLARITY;
    if (p->trust >= 50) return ENDING_REPAIR;
    if (p->trust >= 25) return ENDING_DISTRUST;
    return ENDING_SILENCE;
}

static const EndingInfo ENDINGS[NUM_ENDINGS] = {
    { "Cidade Decifrada", {
        "A NÓVA passa por auditorias abertas e os dados enviesados são refeitos.",
        "Os cidadãos aprendem a questionar, e a confiança volta pouco a pouco.",
        "O Departamento deixa de ser um segredo: vira a escola da cidade." } },
    { "Reparo Lento", {
        "Parte dos erros foi corrigida, mas ainda há muita coisa sem explicação.",
        "A cidade segue usando a NÓVA, agora com mais perguntas do que antes.",
        "O trabalho do Decifrador está só começando." } },
    { "Desconfiança", {
        "As falhas vieram à tona tarde demais, e muita gente desliga os serviços automatizados.",
        "A cidade se divide entre quem desconfia de tudo e quem ainda confia sem olhar.",
        "Reconstruir a confiança vai levar anos." } },
    { "Silêncio", {
        "Sem confiança, ninguém escuta o Departamento de Alfabetização Algorítmica.",
        "A NÓVA continua decidindo, e quase ninguém sabe como.",
        "Ainda dá tempo de recomeçar e tomar decisões melhores." } },
};

const EndingInfo *ending_info(int id) {
    if (id < 0 || id >= NUM_ENDINGS) return NULL;
    return &ENDINGS[id];
}
