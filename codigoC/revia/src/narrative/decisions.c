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
        "A NÓVA agora responde a quem ela afeta: auditorias abertas, dados refeitos e erros assumidos em público.",
        "Quem um dia foi negado por um algoritmo ganhou o direito de perguntar por quê, e de ser ouvido.",
        "O Departamento deixou de ser um segredo. Virou a escola da cidade." },
      "A CIDADE AGORA PERGUNTA." },
    { "Reparo Lento", {
        "Parte dos erros foi corrigida. O resto continua escondido atrás de telas que ninguém sabe explicar.",
        "A cidade ainda usa a NÓVA, mas agora desconfia. Desconfiar é um começo.",
        "Cada decisão adiada tem nome e endereço. Alguém ainda espera uma resposta." },
      "O TRABALHO MAL COMEÇOU." },
    { "Desconfiança", {
        "As falhas vieram à tona tarde demais. Famílias desligaram os serviços e voltaram às filas e aos papéis.",
        "A cidade se dividiu: quem não confia em nada e quem confia sem olhar. Os dois lados perdem.",
        "Reconstruir uma confiança quebrada leva anos. Perdê-la de novo leva um único erro." },
      "A CONFIANÇA SE QUEBROU." },
    { "Silêncio", {
        "Ninguém escutou o Departamento. Os alertas morreram em relatórios que ninguém leu.",
        "A NÓVA segue decidindo crédito, saúde e notícias, e quase ninguém sabe como.",
        "Quem foi prejudicado não recebeu explicação. Quem pediu ajuda não recebeu resposta." },
      "NINGUÉM VIU. ELA DECIDIU." },
};

const EndingInfo *ending_info(int id) {
    if (id < 0 || id >= NUM_ENDINGS) return NULL;
    return &ENDINGS[id];
}
