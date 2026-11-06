#include "narrative/missions.h"

#include <stddef.h>

static const Call CALLS[NUM_CALLS] = {
    /* 0 - Trânsito */
    { 2, "Trânsito", "Sinal que nunca abre",
      "A dona de uma padaria da Zona Sul diz que o semáforo inteligente da NÓVA deixa a rua dela no vermelho por vários minutos.",
      "Os registros mostram sensores só nas avenidas centrais. O que o Decifrador faz?",
      4, {
        { "Pedir sensores em todos os bairros e revisar os dados",
          "A NÓVA passa a receber dados de toda a cidade e a espera na Zona Sul cai.", 8 },
        { "Dizer que o algoritmo está certo",
          "A cidadã se sente ignorada e o caso vira reclamação pública.", -10 },
        { "Desligar o sistema de trânsito por hoje",
          "A cidade para por horas. O erro some, mas ninguém confia na pressa.", -4 },
        { "Registrar os horários e abrir chamado formal",
          "O registro vira prova, mas o problema segue sem correção por ora.", 3 } },
      "Sensores só em alguns bairros geram amostragem enviesada: o sistema aprende o fluxo de onde há dados. Corrigir a cobertura dos dados ataca a causa." },

    /* 1 - Crédito */
    { 2, "Crédito", "Crédito negado sem motivo",
      "Um entregador teve o crédito recusado pela NÓVA e a tela só diz: 'perfil de risco alto'.",
      "Ele quer saber por quê. Como o Decifrador responde?",
      4, {
        { "Pedir revisão humana e os critérios usados",
          "Uma analista revisa o caso e acha um cadastro desatualizado. O crédito é aprovado.", 8 },
        { "Explicar que a IA nunca erra",
          "Ele desiste e conta a história a todo o bairro.", -12 },
        { "Sugerir trocar o endereço no cadastro",
          "Isso é fraude e piora a situação dele.", -8 },
        { "Ensinar como contestar a decisão",
          "Ele aprende o caminho formal, mas o caso ainda depende de uma resposta.", 4 } },
      "Decisões automatizadas relevantes podem ser contestadas. Explicação e revisão humana reduzem erros e dão transparência ao processo." },

    /* 2 - Notícias */
    { 2, "Notícias", "A citação que ninguém disse",
      "O resumo de notícias da NÓVA atribuiu uma frase ao prefeito. Ele nunca disse aquilo.",
      "O texto já apareceu em milhares de telas. O que fazer?",
      3, {
        { "Checar a fonte e publicar a correção",
          "A correção chega a quem viu o erro e a redação passa a conferir citações.", 8 },
        { "Apagar o resumo em silêncio",
          "O erro some, mas quem leu continua acreditando nele.", -5 },
        { "Deixar como está: ninguém vai notar",
          "A mentira ganha força e as notícias da cidade perdem credibilidade.", -12 } },
      "Alucinação é uma resposta plausível, porém inventada. O tom confiante não prova nada: confira a fonte original e corrija publicamente." },

    /* 3 - Saúde */
    { 2, "Saúde", "Triagem com 98% de certeza",
      "Uma paciente recebeu da NÓVA uma sugestão de tratamento com 98% de confiança e pergunta se pode segui-la.",
      "O que o Decifrador orienta?",
      3, {
        { "Usar como apoio e confirmar com um profissional",
          "A médica confirma uma parte e corrige outra. A paciente fica segura.", 8 },
        { "Seguir exatamente o que a IA indicou",
          "Um detalhe errado passa batido e a paciente volta pior.", -12 },
        { "Ignorar tudo o que vem da IA",
          "Você perde um bom apoio de triagem e a paciente fica confusa.", -3 } },
      "Porcentagem de confiança não é garantia. Em saúde, a IA apoia a decisão, mas a responsabilidade e a revisão são de profissionais." },

    /* 4 - Escândalo */
    { 3, "Crédito", "O relatório que ninguém quer",
      "Os dados mostram que o crédito da NÓVA aprova bem menos pedidos do Bairro Sul, com a mesma renda e o mesmo histórico.",
      "A cidade ainda confia na NÓVA. Como o Decifrador age?",
      3, {
        { "Divulgar os dados e pedir auditoria independente",
          "A cidade se abala, mas ganha um processo claro de reparo.", 12 },
        { "Esconder para proteger a confiança",
          "O vazamento chega semanas depois e a confiança despenca.", -18 },
        { "Passar só para um jornal amigo",
          "O caso vira disputa política e as pessoas afetadas ficam sem resposta.", -6 } },
      "Esconder falhas destrói a confiança. Divulgar com evidências e pedir auditoria independente permite corrigir dados, rótulos e critérios." },

    /* 5 - Desfecho */
    { 4, "Prefeitura", "Expandir a NÓVA?",
      "A prefeitura quer ligar a NÓVA a mais serviços ainda esta semana, sem avaliar o impacto.",
      "Você é chamado a opinar no conselho.",
      3, {
        { "Exigir avaliação de impacto e transparência",
          "O conselho aceita auditorias periódicas antes de qualquer expansão.", 10 },
        { "Apoiar a expansão imediata",
          "A NÓVA cresce sem controle e os erros se multiplicam.", -12 },
        { "Exigir que tudo seja desligado",
          "A cidade para e o debate vira briga, não solução.", -3 } },
      "Mais escala significa que um erro atinge mais gente. Avaliar o impacto antes de expandir faz parte do uso responsável." },

    /* 6 - Desfecho */
    { 4, "Assembleia", "A assembleia dos cidadãos",
      "Cidadãos lotam o auditório e perguntam o que vai mudar a partir de agora.",
      "O que o Decifrador promete?",
      3, {
        { "Oficinas abertas de alfabetização algorítmica",
          "As pessoas aprendem a questionar e a cobrar a NÓVA.", 10 },
        { "Pedir que confiem e esperem",
          "A plateia sai desconfiada e sem respostas.", -10 },
        { "Culpar quem usa a NÓVA",
          "A culpa cai nas pessoas erradas e a raiva cresce.", -6 } },
      "Alfabetização algorítmica é o que permite aos cidadãos questionar, usar e vigiar a IA com consciência." },
};

const Call *call_get(int id) {
    if (id < 0 || id >= NUM_CALLS) return NULL;
    return &CALLS[id];
}

int call_best_option(const Call *c) {
    int best = 0;
    for (int i = 1; i < c->nopts; i++)
        if (c->opts[i].trust > c->opts[best].trust) best = i;
    return best;
}

int call_apply(Progress *p, int call_id, int choice) {
    const Call *c = call_get(call_id);
    if (!c || choice < 0 || choice >= c->nopts) return 0;
    if ((p->calls_done >> call_id) & 1u) return 0;
    p->calls_done |= 1u << call_id;
    p->call_choice[call_id] = (int8_t)choice;
    int before = p->trust;
    progress_add_trust(p, c->opts[choice].trust);
    return p->trust - before;
}
