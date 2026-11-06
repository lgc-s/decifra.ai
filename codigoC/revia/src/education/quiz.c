#include "education/quiz.h"

#include <string.h>

/* Em cada questão, options[0] é a resposta correta. */
static const QuizQuestion POOL[NUM_QUIZ_SESSIONS][QUIZ_POOL] = {
  { /* Sessão 1 - Fundamentos (Ato 1) */
    { "Um sistema de IA aprende principalmente a partir de...",
      { "dados e exemplos", "magia", "sorte", "regras que ninguém escreveu" },
      "Modelos de IA ajustam seus parâmetros a partir de exemplos. Sem bons dados, não há bom modelo." },
    { "O que são 'dados de treinamento'?",
      { "Exemplos usados para ajustar o modelo", "O manual do usuário", "A senha do sistema", "Os erros do computador" },
      "São os exemplos com os quais o modelo aprende padrões." },
    { "Qual afirmação sobre IA é a mais correta?",
      { "Ela reconhece padrões estatísticos nos dados", "Ela entende o mundo como uma pessoa", "Ela nunca erra", "Ela tem opinião própria" },
      "A IA calcula padrões. Isso pode ser muito útil, mas não é compreensão humana." },
    { "Se há poucos exemplos de um grupo nos dados, o modelo tende a...",
      { "errar mais com esse grupo", "acertar mais com esse grupo", "ignorar os outros grupos", "ficar mais rápido" },
      "Grupos sub-representados são aprendidos pior, e os erros aparecem justamente neles." },
    { "O que é overfitting?",
      { "Decorar o treino e generalizar mal", "Treinar com poucos dados de propósito", "Ter um modelo muito rápido", "Apagar dados antigos" },
      "No overfitting o modelo vai bem nos exemplos de treino, mas falha em dados novos." },
    { "Para que servem os dados de teste?",
      { "Avaliar o modelo com exemplos que ele não viu", "Treinar mais rápido", "Guardar senhas", "Substituir os dados de treino" },
      "O teste mede se o modelo funciona fora do que ele já viu." },
    { "Um modelo com 95% de acerto geral ainda pode ser injusto?",
      { "Sim, pode errar muito mais em um subgrupo", "Não, 95% garante justiça", "Só se for lento", "Só se usar poucos dados" },
      "A média esconde diferenças: é preciso olhar o desempenho por grupo." },
    { "O que é um rótulo (label) no aprendizado supervisionado?",
      { "A resposta esperada de cada exemplo", "O nome do arquivo", "O título do projeto", "A cor do gráfico" },
      "O rótulo diz ao modelo qual era a resposta certa para aquele exemplo." },
    { "Treinar um modelo é, em essência...",
      { "ajustar parâmetros para reduzir o erro", "escrever todas as regras à mão", "copiar a internet", "desligar e ligar o sistema" },
      "O treinamento procura parâmetros que errem menos nos exemplos." },
    { "Uma IA generalista como a NÓVA...",
      { "atua em várias tarefas, mas depende de dados e tem limites", "não precisa de dados", "é infalível", "só funciona no trânsito" },
      "Ser generalista amplia o alcance, mas não elimina erros nem a dependência dos dados." },
    { "Correlação nos dados significa que...",
      { "duas coisas variam juntas, sem provar causa", "uma causa a outra", "os dados estão certos", "o modelo é justo" },
      "Correlação não é causalidade: modelos podem aprender padrões enganosos." },
    { "Ao suspeitar do erro de uma IA, um bom primeiro passo é...",
      { "olhar os dados e as evidências do caso", "culpar a pessoa que reclamou", "ignorar o caso", "reiniciar tudo" },
      "Investigar dados e evidências é o trabalho do Decifrador." },
  },
  { /* Sessão 2 - Dados e alucinações (Ato 2) */
    { "Dado incompleto é aquele em que...",
      { "faltam informações relevantes", "sobram informações", "tudo está correto", "só há números" },
      "Dados incompletos levam a conclusões distorcidas." },
    { "Qualidade de dados envolve...",
      { "completude, precisão, atualidade e representatividade", "apenas a quantidade", "só a cor do gráfico", "somente o formato do arquivo" },
      "Mais dados não é o mesmo que melhores dados." },
    { "O que é uma alucinação de IA?",
      { "Resposta plausível, porém falsa ou inventada", "Um erro de energia", "Um vírus", "Uma pergunta repetida" },
      "A IA pode gerar texto convincente sem base nos fatos." },
    { "Um modelo inventa uma citação com tom confiante. O que fazer?",
      { "Verificar na fonte original", "Confiar, pois ele parece seguro", "Compartilhar logo", "Apagar o histórico" },
      "O tom confiante não é prova. Cheque a fonte." },
    { "Por que modelos de linguagem alucinam?",
      { "Geram o texto mais provável, sem garantia de fatos", "Pesquisam sempre na internet", "Têm vontade de mentir", "Sofrem com o clima" },
      "Eles preveem palavras prováveis; isso não assegura que a informação seja verdadeira." },
    { "Decisão automatizada é...",
      { "decisão tomada por sistema com pouca intervenção humana", "decisão sempre correta", "decisão tomada em grupo", "decisão sem dados" },
      "Quando afetam pessoas, essas decisões merecem transparência e revisão." },
    { "A LGPD brasileira prevê que a pessoa possa...",
      { "pedir revisão de decisões automatizadas", "apagar o código da IA", "proibir qualquer IA", "receber todos os dados do sistema" },
      "A lei prevê o direito de solicitar revisão de decisões tomadas apenas por tratamento automatizado." },
    { "Sensores de trânsito só em bairros centrais causam...",
      { "amostragem enviesada", "mais segurança", "ruas vazias", "dados perfeitos" },
      "Onde não há dados, o sistema aprende pior." },
    { "Dados desatualizados podem...",
      { "gerar decisões baseadas em uma realidade que mudou", "melhorar o modelo", "reduzir custos sem risco", "evitar vieses" },
      "O mundo muda; dados antigos podem levar a erros novos." },
    { "Qual prática ajuda a reduzir o risco de alucinações?",
      { "Pedir fontes e conferir as informações", "Usar frases mais longas", "Pedir que a IA 'tenha certeza'", "Apagar a pergunta" },
      "Conferir fontes é a defesa mais simples e eficaz." },
    { "Dados duplicados ou inconsistentes podem...",
      { "distorcer o aprendizado do modelo", "tornar o modelo mais justo", "acelerar a internet", "não ter efeito algum" },
      "Registros repetidos ou contraditórios confundem o treinamento." },
    { "Uma IA responde com muita confiança. Isso indica...",
      { "nada sobre a veracidade", "que a resposta é verdadeira", "que a fonte é oficial", "que não há erro" },
      "Tom e certeza aparente não medem a verdade." },
  },
  { /* Sessão 3 - Viés (Ato 3) */
    { "Viés algorítmico é...",
      { "resultado sistematicamente desigual para certos grupos", "um defeito de tela", "um tipo de gráfico", "um ataque hacker" },
      "O viés aparece como padrões de erro ou tratamento desigual entre grupos." },
    { "De onde pode vir o viés de um sistema?",
      { "Dos dados históricos, dos rótulos e das escolhas de projeto", "Só do hardware", "Só da internet lenta", "De nenhum lugar" },
      "Dados, rótulos e decisões de projeto carregam escolhas humanas." },
    { "O que é uma variável proxy?",
      { "Uma variável aparentemente neutra que revela um atributo sensível", "Um servidor de rede", "Um tipo de senha", "A variável mais importante" },
      "O CEP, por exemplo, pode funcionar como proxy para o bairro e outros atributos." },
    { "Remover a coluna 'bairro' garante justiça?",
      { "Não, outras variáveis podem funcionar como proxy", "Sim, sempre", "Só no período da manhã", "Sim, se o modelo for grande" },
      "Se o CEP ou outra variável ainda indica o bairro, o viés continua." },
    { "Dados históricos de decisões discriminatórias...",
      { "podem ensinar o modelo a repetir a discriminação", "sempre corrigem o problema", "são neutros", "não influenciam o modelo" },
      "O modelo aprende o que está nos dados, inclusive as injustiças." },
    { "Qual medida compara tratamento entre grupos?",
      { "A taxa de aprovação por grupo", "O tamanho do arquivo", "A velocidade do processador", "O número de linhas de código" },
      "Comparar taxas por grupo ajuda a revelar disparidades." },
    { "Rebalancear a amostra de treino ajuda quando...",
      { "um grupo está sub-representado", "todos os grupos já estão iguais", "o modelo é muito rápido", "não há dados" },
      "Mais exemplos do grupo sub-representado reduzem erros nele, mas não corrigem rótulos injustos." },
    { "Uma auditoria algorítmica independente serve para...",
      { "examinar dados, métricas e impacto por grupo", "acelerar o sistema", "esconder falhas", "trocar a equipe" },
      "Quem audita de fora ajuda a ganhar confiança e a achar falhas." },
    { "Justiça entre grupos e acurácia geral...",
      { "podem entrar em tensão e exigem critérios abertos", "nunca se relacionam", "são a mesma coisa", "não importam" },
      "Escolher o equilíbrio é uma decisão social, não apenas técnica." },
    { "Corrigir rótulos históricos significa...",
      { "revisar decisões passadas que foram injustas", "apagar todos os dados", "mudar a cor do site", "treinar menos" },
      "Rótulos injustos ensinam injustiça ao modelo." },
    { "Quem deve definir o que é um resultado justo?",
      { "Um processo transparente que inclua os afetados", "Só quem programou", "Só a própria IA", "Ninguém" },
      "Justiça é uma discussão pública, não um detalhe técnico." },
    { "Um modelo é neutro porque é só matemática?",
      { "Não, ele reflete dados e escolhas humanas", "Sim, matemática é neutra sempre", "Sim, se for grande", "Depende do dia" },
      "A matemática é neutra; os dados e as decisões de projeto, não." },
  },
  { /* Sessão 4 - Uso responsável e impacto social (Ato 4) */
    { "Uso responsável de IA generativa inclui...",
      { "conferir, citar e assumir a responsabilidade pelo resultado", "copiar sem ler", "esconder o uso", "confiar sempre" },
      "Quem usa a ferramenta responde pelo que publica." },
    { "Ao usar IA pública, dados pessoais e sensíveis...",
      { "devem ser evitados nos pedidos", "devem sempre ser incluídos", "ficam sempre protegidos", "não importam" },
      "Nem toda ferramenta garante sigilo. Evite expor dados sensíveis." },
    { "Transparência algorítmica é...",
      { "explicar critérios, limites e impactos de um sistema", "esconder o código", "usar menos dados", "trocar o modelo toda semana" },
      "Quem é afetado precisa poder entender e questionar." },
    { "O que é 'humano no circuito'?",
      { "Pessoa que revisa e pode reverter decisões críticas", "Um robô novo", "Um tipo de sensor", "A IA que se revisa sozinha" },
      "A supervisão humana é essencial em decisões que afetam vidas." },
    { "Automação em saúde deve...",
      { "apoiar profissionais, não substituí-los em decisões críticas", "dispensar médicos", "agir sem registro", "operar sem testes" },
      "A decisão crítica precisa de responsabilidade humana." },
    { "Quando uma IA erra com impacto social, o ideal é...",
      { "registrar, corrigir, comunicar e reparar", "negar o erro", "apagar os registros", "trocar o nome do sistema" },
      "Assumir e reparar fortalece a confiança." },
    { "Alfabetização algorítmica significa...",
      { "entender o básico para questionar e usar IA com crítica", "programar qualquer IA", "evitar toda tecnologia", "decorar nomes de modelos" },
      "Não é preciso ser especialista: é preciso saber perguntar." },
    { "Um erro de IA em um serviço público...",
      { "pode afetar milhares de pessoas rapidamente", "afeta sempre uma só pessoa", "nunca é grave", "some sozinho" },
      "Escala amplia o benefício e também o dano." },
    { "A confiança pública depende de...",
      { "transparência, responsabilidade e correção de erros", "propaganda", "silêncio", "velocidade" },
      "Confiar é consequência de agir de forma verificável." },
    { "Expandir a IA para mais serviços sem avaliar o impacto é...",
      { "arriscado", "sempre seguro", "obrigatório", "irrelevante" },
      "Mais alcance exige mais cuidado e avaliação prévia." },
    { "O direito de contestar uma decisão automatizada deve ser...",
      { "acessível e compreensível", "escondido", "só para especialistas", "opcional para o sistema" },
      "Se ninguém consegue contestar, o direito existe só no papel." },
    { "A melhor resposta ao descobrir um erro grave é...",
      { "assumir, corrigir e comunicar", "esperar passar", "culpar o usuário", "mudar de assunto" },
      "A transparência diante do erro é o que reconstrói a confiança." },
  },
};

static const char *TITLES[NUM_QUIZ_SESSIONS] = {
    "Avaliação 1: Fundamentos",
    "Avaliação 2: Dados e alucinações",
    "Avaliação 3: Viés algorítmico",
    "Avaliação 4: Uso responsável",
};

const char *quiz_session_title(int s) {
    return (s >= 0 && s < NUM_QUIZ_SESSIONS) ? TITLES[s] : "?";
}

static uint32_t rng_next(uint32_t *s) {
    *s = *s * 1664525u + 1013904223u;
    return *s >> 8;
}

void quiz_start(QuizRun *r, int session, uint32_t seed) {
    memset(r, 0, sizeof *r);
    r->session = session;
    r->rng = seed ? seed : 12345u;

       /* Sorteia QUIZ_LEN questões distintas do banco e as põe em ordem crescente de dificuldade. */
    int idx[QUIZ_POOL];
    for (int i = 0; i < QUIZ_POOL; i++) idx[i] = i;
    for (int i = 0; i < QUIZ_LEN; i++) {                     /* Fisher-Yates parcial */
        int j = i + (int)(rng_next(&r->rng) % (uint32_t)(QUIZ_POOL - i));
        int t = idx[i]; idx[i] = idx[j]; idx[j] = t;
    }
    for (int i = 1; i < QUIZ_LEN; i++) {                     /* ordena os sorteados */
        int v = idx[i], k = i - 1;
        while (k >= 0 && idx[k] > v) { idx[k + 1] = idx[k]; k--; }
        idx[k + 1] = v;
    }
    for (int i = 0; i < QUIZ_LEN; i++) r->pool_idx[i] = idx[i];

    /* Embaralha as alternativas de cada questão (Fisher-Yates). */
    for (int q = 0; q < QUIZ_LEN; q++) {
        for (int i = 0; i < 4; i++) r->perm[q][i] = i;
        for (int i = 3; i > 0; i--) {
            int j = (int)(rng_next(&r->rng) % (uint32_t)(i + 1));
            int t = r->perm[q][i]; r->perm[q][i] = r->perm[q][j]; r->perm[q][j] = t;
        }
    }
}

const QuizQuestion *quiz_current(const QuizRun *r) {
    if (r->cur < 0 || r->cur >= QUIZ_LEN) return 0;
    return &POOL[r->session][r->pool_idx[r->cur]];
}

const char *quiz_option_text(const QuizRun *r, int display_idx) {
    const QuizQuestion *q = quiz_current(r);
    if (!q || display_idx < 0 || display_idx > 3) return "";
    return q->options[r->perm[r->cur][display_idx]];
}

int quiz_correct_display_index(const QuizRun *r) {
    for (int i = 0; i < 4; i++)
        if (r->perm[r->cur][i] == 0) return i;
    return 0;
}

bool quiz_answer(QuizRun *r, int display_idx) {
    if (r->answered || r->finished) return r->last_correct;
    bool ok = r->perm[r->cur][display_idx] == 0;
    r->answered = true;
    r->last_correct = ok;
    if (ok) { r->correct_count++; r->correct_mask |= (uint16_t)(1u << r->cur); }
    else    { r->wrong_count++;   r->wrong_mask   |= (uint16_t)(1u << r->cur); }
    return ok;
}

void quiz_next(QuizRun *r) {
    if (!r->answered || r->finished) return;
    r->answered = false;
    r->cur++;
    if (r->cur >= QUIZ_LEN) {
        r->cur = QUIZ_LEN - 1;
        r->finished = true;
    }
}

int quiz_trust_delta(const QuizRun *r) {
    return r->correct_count * QUIZ_TRUST_RIGHT - r->wrong_count * QUIZ_TRUST_WRONG;
}