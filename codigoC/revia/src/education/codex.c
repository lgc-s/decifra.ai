#include "education/codex.h"

#include <stddef.h>

static const CodexEntry ENTRIES[NUM_COLLECTIBLES] = {
    /* Ato 1 */
    { "Inteligência artificial",
      "Software que aprende padrões a partir de exemplos. Ela não 'entende' como uma pessoa: calcula probabilidades." },
    { "Dados",
      "Os exemplos que alimentam a IA. Dados faltando, errados ou desatualizados produzem decisões ruins." },
    { "Treinamento de modelos",
      "Processo de ajustar parâmetros para errar menos nos exemplos. Depois o modelo é avaliado com dados de teste." },
    /* Ato 2 */
    { "Qualidade dos dados",
      "Completude, precisão, atualidade e representatividade. Quem não aparece nos dados fica de fora das decisões." },
    { "Alucinações",
      "Respostas plausíveis, porém inventadas. Confira fontes, principalmente quando o texto soa muito seguro." },
    { "Decisão automatizada",
      "Escolha feita por um sistema com pouca ou nenhuma intervenção humana. Pessoas afetadas podem pedir explicação e revisão." },
    /* Ato 3 */
    { "Viés algorítmico",
      "Resultados sistematicamente desiguais entre grupos. Surge de dados, rótulos ou escolhas de projeto." },
    { "Variável proxy",
      "Dado aparentemente neutro, como o CEP, que revela um atributo sensível. Tirar uma coluna nem sempre resolve." },
    { "Auditoria algorítmica",
      "Exame independente de dados, métricas e impacto por grupo, para achar e corrigir falhas." },
    /* Ato 4 */
    { "IA generativa responsável",
      "Conferir, citar, evitar dados sensíveis e assumir a responsabilidade pelo que for publicado." },
    { "Transparência",
      "Explicar critérios, limites e impactos de um sistema, para que as pessoas possam questioná-lo." },
    { "Impacto social",
      "Em serviços públicos, um erro atinge muita gente de uma vez. Avaliar o impacto antes de expandir é cuidado básico." },
};

const CodexEntry *codex_get(int id) {
    if (id < 0 || id >= NUM_COLLECTIBLES) return NULL;
    return &ENTRIES[id];
}
