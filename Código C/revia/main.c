/*
 * REV.IA - Departamento de Alfabetização Algorítmica
 * Jogo 2D em linguagem C com a biblioteca raylib.
 *
 * Controles: WASD/setas = mover | E = interagir | TAB = caderno de evidências
 *            ESC = pausa | ENTER = confirmar | 1/2/3 = escolher | mouse também funciona nos menus
 */
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Constantes                                                          */
/* ------------------------------------------------------------------ */
#define W 960               /* resolução-base 16:9 */
#define H 540
#define WW 1900             /* largura do mundo (câmera acompanha o jogador) */
#define EV_N 4              /* total de evidências */
#define CH_N 2              /* total de chamados */
#define PQ_N 14             /* perguntas no banco */
#define QN 5                /* perguntas por sessão de quiz */
#define LK_N 3              /* desafios do cadeado lógico */
#define SAVE_FILE "save_slot1.dat"
#define CERT_FILE "certificado_conclusao.txt"
#define SAVE_MAGIC 0x52455631

static const Color VERDE   = {0, 255, 136, 255};
static const Color VERDE_E = {0, 110, 70, 255};
static const Color FUNDO   = {6, 12, 14, 255};
static const Color PAINEL  = {8, 26, 24, 235};
static const Color TEXTO   = {225, 240, 235, 255};
static const Color AMARELO = {255, 200, 40, 255};
static const Color VERM    = {255, 90, 90, 255};
static const Color CINZA   = {110, 130, 125, 255};

typedef enum {
    ST_MENU, ST_CONFIG, ST_AUDIO, ST_CRED, ST_INTRO, ST_PLAY, ST_DLG,
    ST_QUIZ, ST_LOCK, ST_NOTE, ST_PAUSE, ST_CRISE, ST_FIM
} Estado;

typedef enum { K_QUIZ, K_CIDADAO, K_EVID, K_LOCK, K_CRISE } TipoObj;
typedef enum { CH_NOVO, CH_INVESTIGANDO, CH_RESOLVIDO, CH_IGNORADO } EstadoChamado;

/* ------------------------------------------------------------------ */
/* Estruturas de dados                                                 */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *q, *op[3];
    int ok;                       /* índice da alternativa correta */
    const char *expl, *dica;
    int dif;                      /* 1 a 3 */
} Pergunta;

typedef struct {
    const char *nome, *categoria, *titulo, *relato, *decisao, *info;
    const char *op[3], *resp[3];
    int dConf[3], dPts[3], revela[3], estadoApos[3];
    const char *conclusao;
} Chamado;

typedef struct {
    const char *nome, *desc, *origem, *relev, *relacao;
    int chamado;
} Evidencia;

typedef struct {
    TipoObj tipo;
    Rectangle r;
    const char *rotulo;
    int id;                       /* evidência ou chamado */
} Objeto;

typedef struct {
    int magic, conf, pts, found[EV_N], chEst[CH_N];
    int quizFeito, lockAberto, tut, acertos, erros, qCursor, concluido, quizTempo;
    float tempo, px, py;
} Save;

typedef struct {
    Estado estado;
    int sair, menuSel, temSave;
    /* jogador */
    Vector2 pos;
    float anim;
    float distAndada;
    /* indicadores */
    int conf, pts, popup;
    float popupT;
    int found[EV_N], chEst[CH_N], chAtual;
    int quizFeito, lockAberto, crise;
    int tut, tutAtivo;
    int acertos, erros;
    float tempo;
    /* mensagens */
    char msg[160];
    float msgT;
    Color msgCor;
    /* diálogo */
    int dlgFase, dlgSel, dlgCh;
    /* quiz */
    int qOrdem[PQ_N], qCursor, qSel[QN], qI, qResp, qHint, qFeedback, qAcertos;
    float qTempo;
    int quizTempo;
    /* cadeado */
    int lkD, lkV[3], lkFase, lkOk, lkSel;
    float lkT, lkAux;
    /* caderno / crise / final */
    int nbSel, criseSel, criseRes, finalId;
    /* áudio */
    int audioOk;
    float volMusica, volSfx;
    Sound sOk, sBad, sClick, sAmb;
    Camera2D cam;
    /* ---- campos adicionados na continuação do projeto ---- */
    Estado volta, cfgVolta, audVolta;   /* para onde retornar de pausa/config/áudio */
    int pauseSel, cfgSel, audSel, confirma, confSel, introPg, introRev;
    int certOk, saveStatus, audioDev, audioOn;
    int qCur, qBeep, qRev, qGanho, qPts, lkFl;
    int criseDC, criseDP;
    const char *criseTxt;
    float tutT;
} Jogo;

/* ------------------------------------------------------------------ */
/* Banco de perguntas (separado da lógica de apresentação)             */
/* ------------------------------------------------------------------ */
static const Pergunta PERGUNTAS[PQ_N] = {
    {"O que é um algoritmo?",
     {"Uma sequência finita de passos para resolver um problema", "Uma máquina física que calcula", "Um tipo de linguagem falada"}, 0,
     "Algoritmo é uma sequência finita e não ambígua de instruções para resolver um problema ou executar uma tarefa.",
     "Pense em uma receita de bolo.", 1},
    {"Um modelo acerta 99% dos dados de treino, mas erra quase metade dos dados novos. Isso é chamado de:",
     {"Overfitting", "Viés de dados", "Falta de interpretabilidade"}, 0,
     "O modelo memorizou padrões específicos dos dados de treino e não consegue generalizar para dados novos: é o overfitting.",
     "Ele decorou a prova em vez de aprender a matéria.", 2},
    {"Um filtro antifraude bloqueia uma compra legítima. Esse erro é um:",
     {"Verdadeiro positivo", "Falso positivo", "Falso negativo"}, 1,
     "Falso positivo: o sistema diz 'fraude' quando não era. Falso negativo seria deixar passar uma fraude real.",
     "O sistema disse 'sim' quando a resposta era 'não'.", 1},
    {"Um exame automatizado não detecta uma doença que o paciente realmente tem. Esse erro é um:",
     {"Falso positivo", "Verdadeiro negativo", "Falso negativo"}, 2,
     "Falso negativo: o sistema diz 'não há doença' quando há. Em saúde, esse erro pode ter consequências graves.",
     "O sistema disse 'não' quando a resposta era 'sim'.", 2},
    {"Dados históricos de crédito refletem discriminação do passado. O que pode acontecer ao treinar um modelo com eles?",
     {"O modelo corrige a discriminação sozinho", "O modelo pode reproduzir e ampliar a discriminação", "Nada, pois algoritmos são neutros"}, 1,
     "Modelos aprendem padrões dos dados. Se os dados carregam viés, o modelo tende a reproduzi-lo, mesmo sem intenção.",
     "O modelo aprende com o que recebe.", 2},
    {"Vendas de sorvete e afogamentos sobem juntos no verão. A conclusão mais adequada é:",
     {"Sorvete causa afogamentos", "Há correlação, provavelmente explicada por um fator comum (calor)", "Afogamentos aumentam as vendas de sorvete"}, 1,
     "Correlação não implica causalidade. O calor leva mais gente à praia e ao sorvete, criando a associação.",
     "Existe um terceiro fator por trás dos dois.", 2},
    {"Qual prática reduz riscos de privacidade ao treinar um modelo?",
     {"Coletar o máximo possível de dados pessoais", "Minimizar e anonimizar os dados coletados", "Publicar a base bruta para todos"}, 1,
     "Coletar só o necessário (minimização) e anonimizar reduz o risco de vazamento e de identificação das pessoas.",
     "Quanto menos dado pessoal exposto, melhor.", 2},
    {"Para que serve a explicabilidade em um sistema de IA?",
     {"Deixar o sistema mais rápido", "Permitir entender por que o sistema chegou a uma decisão", "Esconder os dados de treino"}, 1,
     "Explicabilidade ajuda pessoas a compreender, contestar e auditar decisões automatizadas.",
     "Relaciona-se com transparência.", 1},
    {"Uma decisão automatizada negou um benefício por erro. Quem responde por ela?",
     {"Ninguém, foi o algoritmo", "As pessoas e instituições que implantaram e operam o sistema", "Apenas o cidadão afetado"}, 1,
     "Algoritmos não são responsáveis legais ou morais. A responsabilidade é humana e institucional.",
     "Sistemas não assumem responsabilidades.", 2},
    {"Um modelo para uma doença rara (1% dos casos) responde sempre 'saudável' e tem 99% de acurácia. O que isso mostra?",
     {"Que o modelo é perfeito", "Que acurácia sozinha pode enganar quando as classes são desbalanceadas", "Que faltam dados de privacidade"}, 1,
     "Com classes desbalanceadas, é preciso olhar também sensibilidade, precisão e falsos negativos.",
     "Ele nunca acerta um único doente.", 3},
    {"Qual conjunto de dados tende a produzir o pior modelo?",
     {"Representativo e atualizado", "Incompleto, desatualizado e com rótulos errados", "Diversificado e revisado"}, 1,
     "A qualidade dos dados limita a qualidade do modelo: dados ruins geram decisões ruins ('lixo entra, lixo sai').",
     "Lixo entra, lixo sai.", 1},
    {"O que significa o direito à revisão humana de decisões automatizadas?",
     {"Uma pessoa pode pedir que um humano reavalie a decisão", "Humanos precisam programar tudo do zero", "Só empresas podem pedir revisão"}, 0,
     "Quem é afetado por uma decisão automatizada pode solicitar revisão por uma pessoa, em vez de aceitar uma resposta definitiva.",
     "Existe sempre um recurso a uma pessoa.", 2},
    {"Um assistente de IA cita com confiança uma lei que não existe. Esse fenômeno é chamado de:",
     {"Criptografia", "Alucinação", "Overfitting de hardware"}, 1,
     "Alucinação é quando o modelo gera conteúdo plausível, porém falso. Por isso é preciso verificar as fontes.",
     "Soa convincente, mas é inventado.", 2},
    {"Dois modelos têm acurácia parecida, mas um erra muito mais com um grupo específico de pessoas. O que fazer?",
     {"Ignorar, pois a acurácia média é a mesma", "Auditar o desempenho por grupo e investigar a causa", "Esconder o resultado para não gerar desconfiança"}, 1,
     "Média alta pode esconder desigualdades. Auditar por grupo é essencial para detectar viés e corrigi-lo com transparência.",
     "Médias escondem diferenças.", 3},
};

/* ------------------------------------------------------------------ */
/* Chamados e evidências                                               */
/* ------------------------------------------------------------------ */
static const Chamado CHAMADOS[CH_N] = {
    {"Sr. Marcos", "Saúde e assistência pública", "Auxílio negado",
     "A NÓVA negou meu auxílio sem me explicar o motivo. Tentei pedir revisão, mas o sistema disse que a decisão era definitiva.",
     "Benefício negado (AUT-4471). Revisão bloqueada.",
     "Dados iniciais: renda cadastrada alta e endereço registrado fora da cidade.",
     {"Explicar que a decisão foi automatizada e investigar os motivos",
      "Consultar os dados do cadastro e verificar se há erros",
      "Ignorar a reclamação e encerrar o chamado"},
     {"Você explica que foi uma decisão automatizada, não uma sentença, e abre uma investigação. Agora é preciso achar o registro da decisão no servidor.",
      "No cadastro você encontra um endereço antigo e uma renda desatualizada. Isso pode ter pesado na decisão. Falta ver como a NÓVA usou esses dados.",
      "O chamado é encerrado sem investigação. O Sr. Marcos sai sem resposta e a notícia se espalha pela vizinhança."},
     {4, 3, -8}, {1, 1, 0}, {-1, 1, -1}, {CH_INVESTIGANDO, CH_INVESTIGANDO, CH_IGNORADO},
     "Com o registro e o cadastro em mãos, você mostra que a negação veio de dados desatualizados, sem justificativa registrada. A revisão humana é aberta. Chamado resolvido com base em evidências."},
    {"Dona Lúcia", "Crédito e acesso a recursos", "Microcrédito recusado",
     "Pedi um microcrédito para o meu ateliê e a NÓVA recusou. Vizinhos do meu bairro também foram recusados. Será que o problema é o CEP?",
     "Microcrédito negado (CRD-0932). Pontuação: 412.",
     "Dados iniciais: sem dívidas ativas e renda estável nos últimos 3 anos.",
     {"Pedir à NÓVA a justificativa e abrir uma auditoria",
      "Comparar as taxas de negação por bairro",
      "Dizer que a NÓVA não erra e encerrar o chamado"},
     {"Você abre uma auditoria. A NÓVA só informa uma pontuação, sem explicar. É preciso reunir os dados de treino do sistema.",
      "Você levanta os números: o bairro dela tem muito mais negações do que bairros com renda parecida. Um padrão importante entrou no caderno.",
      "Você repete que 'o sistema não erra'. Dona Lúcia sai indignada e a confiança na instituição cai."},
     {4, 3, -8}, {1, 1, 0}, {-1, 3, -1}, {CH_INVESTIGANDO, CH_INVESTIGANDO, CH_IGNORADO},
     "Os dados de treino e a distribuição dos erros mostram que a NÓVA aprendeu com um histórico que já negava crédito ao bairro. É viés de dados, não falha da cidadã. A decisão será revista."},
};

static const Evidencia EVIDENCIAS[EV_N] = {
    {"Registro de decisão AUT-4471", "A NÓVA negou o auxílio com 'pontuação de risco 0,81'. Nenhum motivo foi registrado e a revisão estava bloqueada.",
     "Servidor de decisões", "Mostra uma decisão sem justificativa e sem revisão humana.", "Combine com o Cadastro do Sr. Marcos.", 0},
    {"Cadastro do Sr. Marcos", "Endereço de seis anos atrás e renda desatualizada, usados no cálculo do risco.",
     "Terminal de cadastros", "Explica de onde veio o risco: dado desatualizado.", "Liga-se ao Registro AUT-4471.", 0},
    {"Dados de treino de crédito", "Em dez anos de histórico, 78% dos pedidos recusados vinham do Bairro Sul.",
     "Arquivo de treinamento", "Indica que a amostra de treino já era enviesada.", "Explica a Distribuição de erros por bairro.", 1},
    {"Distribuição de erros por bairro", "Moradores do Bairro Sul sofrem 3 vezes mais recusas indevidas que os demais.",
     "Relatório de auditoria", "Prova de padrão: o erro não é aleatório.", "Consequência dos Dados de treino de crédito.", 1},
};

/* Objetos interativos do mundo */
static const Objeto OBJETOS[] = {
    {K_QUIZ,     {160, 170, 70, 60},   "Terminal de Treinamento", 0},
    {K_CIDADAO,  {500, 380, 28, 36},   "Sr. Marcos", 0},
    {K_EVID,     {780, 165, 60, 80},   "Servidor de decisões", 0},
    {K_EVID,     {1000, 165, 60, 60},  "Terminal de cadastros", 1},
    {K_LOCK,     {1150, 165, 60, 60},  "Cadeado Lógico", 0},
    {K_CIDADAO,  {1340, 380, 28, 36},  "Dona Lúcia", 1},
    {K_EVID,     {1450, 165, 60, 60},  "Arquivo de treinamento", 2},
    {K_EVID,     {1620, 400, 60, 50},  "Relatório de auditoria", 3},
    {K_CRISE,    {1760, 165, 80, 70},  "Sala de Crise", 0},
};
#define OBJ_N ((int)(sizeof(OBJETOS) / sizeof(OBJETOS[0])))

static const Rectangle SOLIDOS[] = {
    {620, 270, 60, 50}, {900, 390, 80, 40}, {1450, 310, 70, 50},
};
#define SOL_N ((int)(sizeof(SOLIDOS) / sizeof(SOLIDOS[0])))
static const Rectangle BARREIRA = {1240, 150, 20, 380};

/* Desafios do Cadeado Lógico: a lógica é calculada de verdade em C */
static const char *LK_EXPR[LK_N] = {
    "(A AND B) OR (NOT C)",
    "(A OR B) AND NOT (B AND C)",
    "(A => B) AND (C XOR A)",
};
static const int LK_INI[LK_N][3] = {{0, 0, 1}, {0, 0, 0}, {0, 0, 0}};

static int lkAvalia(int d, const int *v)
{
    int A = v[0], B = v[1], C = v[2];
    switch (d) {
    case 0: return (A && B) || !C;
    case 1: return (A || B) && !(B && C);
    default: return (!A || B) && (C != A);
    }
}

/* ------------------------------------------------------------------ */
/* Utilidades de interface                                             */
/* ------------------------------------------------------------------ */
static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }

static Vector2 MouseV(void)
{
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float s = fminf(sw / W, sh / H);
    float ox = (sw - W * s) / 2, oy = (sh - H * s) / 2;
    Vector2 m = GetMousePosition();
    return (Vector2){(m.x - ox) / s, (m.y - oy) / s};
}

static int Confirma(void) { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE); }

static void TextoCentro(const char *t, int cx, int y, int sz, Color c)
{
    DrawText(t, cx - MeasureText(t, sz) / 2, y, sz, c);
}

/* Quebra automática de linha. Retorna o y final. */
static int TextoQuebra(const char *t, int x, int y, int w, int sz, Color c)
{
    char linha[640] = "", teste[640], palavra[64];
    const char *p = t;
    while (*p) {
        int n = 0;
        while (*p && *p != ' ' && n < 63) palavra[n++] = *p++;
        palavra[n] = 0;
        while (*p == ' ') p++;
        snprintf(teste, sizeof teste, "%s%s%s", linha, linha[0] ? " " : "", palavra);
        if (MeasureText(teste, sz) > w && linha[0]) {
            DrawText(linha, x, y, sz, c);
            y += sz + 4;
            snprintf(linha, sizeof linha, "%s", palavra);
        } else {
            snprintf(linha, sizeof linha, "%s", teste);
        }
    }
    if (linha[0]) { DrawText(linha, x, y, sz, c); y += sz + 4; }
    return y;
}

static void Painel(Rectangle r, Color borda)
{
    DrawRectangleRec(r, PAINEL);
    DrawRectangleLinesEx(r, 2, borda);
}

static void Botao(Rectangle r, const char *t, int sel, int ativo)
{
    Color c = !ativo ? CINZA : (sel ? VERDE : VERDE_E);
    DrawRectangleRec(r, sel && ativo ? (Color){0, 70, 45, 255} : (Color){10, 24, 24, 255});
    DrawRectangleLinesEx(r, sel ? 3 : 1, c);
    TextoCentro(t, (int)(r.x + r.width / 2), (int)(r.y + r.height / 2 - 9), 20, !ativo ? CINZA : (sel ? VERDE : TEXTO));
}

static void Msg(Jogo *g, const char *t, Color c)
{
    snprintf(g->msg, sizeof g->msg, "%s", t);
    g->msgT = 3.0f;
    g->msgCor = c;
}

static void Sfx(Jogo *g, Sound s)
{
    if (!g->audioOk || !s.stream.buffer) return;
    SetSoundVolume(s, g->volSfx);
    PlaySound(s);
}

/* Gera um tom senoidal em memória (sem arquivos externos) */
static Sound Tom(float f0, float f1, float dur, float vol, float f2)
{
    const int sr = 22050;
    int n = (int)(sr * dur);
    short *d = (short *)malloc((size_t)n * sizeof(short));
    if (!d) return (Sound){0};
    float ph = 0, ph2 = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        float f = f0 + (f1 - f0) * t / dur;
        ph += 6.2831853f * f / sr;
        ph2 += 6.2831853f * f2 / sr;
        float env = fminf(1.0f, fminf(i / (sr * 0.02f), (n - i) / (sr * 0.05f)));
        float v = sinf(ph) + (f2 > 0 ? 0.6f * sinf(ph2) : 0);
        d[i] = (short)(v * env * vol * 18000);
    }
    Wave w = {(unsigned)n, (unsigned)sr, 16, 1, d};
    Sound s = LoadSoundFromWave(w);
    free(d);
    return s;
}

/* ------------------------------------------------------------------ */
/* Regras do jogo                                                      */
/* ------------------------------------------------------------------ */
static void MudaConf(Jogo *g, int d)
{
    g->conf = clampi(g->conf + d, 0, 100);
    if (d != 0) { g->popup = d; g->popupT = 1.6f; }
}

static void GanhaPts(Jogo *g, int d) { g->pts += d; if (g->pts < 0) g->pts = 0; }

/* Avança o tutorial. Tolerante: se o jogador fez a ação fora de ordem, o passo é pulado. */
static void TutAvanca(Jogo *g, int passo)
{
    if (g->tutAtivo && g->tut <= passo) g->tut = passo + 1;
    if (g->tut > 5) g->tutAtivo = 0;
}

static int EvidDoChamado(const Jogo *g, int ch, int *total)
{
    int ok = 0, tot = 0;
    for (int i = 0; i < EV_N; i++)
        if (EVIDENCIAS[i].chamado == ch) { tot++; ok += g->found[i]; }
    if (total) *total = tot;
    return ok;
}

static int EvidTotais(const Jogo *g)
{
    int n = 0;
    for (int i = 0; i < EV_N; i++) n += g->found[i];
    return n;
}

static void Salvar(Jogo *g, int concluido)
{
    Save s;
    memset(&s, 0, sizeof s);
    s.magic = SAVE_MAGIC; s.conf = g->conf; s.pts = g->pts;
    memcpy(s.found, g->found, sizeof s.found);
    memcpy(s.chEst, g->chEst, sizeof s.chEst);
    s.quizFeito = g->quizFeito; s.lockAberto = g->lockAberto; s.tut = g->tut;
    s.acertos = g->acertos; s.erros = g->erros; s.qCursor = g->qCursor;
    s.concluido = concluido; s.quizTempo = g->quizTempo; s.tempo = g->tempo;
    s.px = g->pos.x; s.py = g->pos.y;
    FILE *f = fopen(SAVE_FILE, "wb");
    if (!f) return;
    fwrite(&s, sizeof s, 1, f);
    fclose(f);
    g->temSave = !concluido;
}

/* Valida o arquivo de save: 0 = não existe, 1 = válido, 2 = inválido/corrompido, 3 = campanha concluída */
static int ChecaSave(Save *s)
{
    FILE *f = fopen(SAVE_FILE, "rb");
    if (!f) return 0;
    int lido = fread(s, sizeof *s, 1, f) == 1;
    fclose(f);
    if (!lido || s->magic != SAVE_MAGIC) return 2;
    if (s->concluido) return 3;
    if (s->conf < 0 || s->conf > 100 || s->pts < 0 || s->tut < 0 || s->tut > 6) return 2;
    if (s->qCursor < 0 || s->qCursor >= PQ_N) return 2;
    if (s->px < 0 || s->px > WW || s->py < 0 || s->py > H) return 2;
    for (int i = 0; i < EV_N; i++) if (s->found[i] != 0 && s->found[i] != 1) return 2;
    for (int i = 0; i < CH_N; i++) if (s->chEst[i] < 0 || s->chEst[i] > CH_IGNORADO) return 2;
    return 1;
}

static int LerSave(Save *s) { return ChecaSave(s) == 1; }

static void NovoJogo(Jogo *g)
{
    g->conf = 50; g->pts = 0; g->popupT = 0;
    memset(g->found, 0, sizeof g->found);
    memset(g->chEst, 0, sizeof g->chEst);
    g->chAtual = 0; g->quizFeito = 0; g->lockAberto = 0; g->crise = 0;
    g->tut = 0; g->tutAtivo = 1; g->acertos = 0; g->erros = 0; g->tempo = 0;
    g->pos = (Vector2){120, 400}; g->distAndada = 0; g->msgT = 0;
    g->criseSel = 0; g->criseRes = 0; g->nbSel = 0; g->tutT = 0; g->certOk = 0;
}

static void CarregaJogo(Jogo *g, const Save *s)
{
    NovoJogo(g);
    g->conf = s->conf; g->pts = s->pts;
    memcpy(g->found, s->found, sizeof g->found);
    memcpy(g->chEst, s->chEst, sizeof g->chEst);
    g->quizFeito = s->quizFeito; g->lockAberto = s->lockAberto; g->tut = s->tut;
    g->tutAtivo = s->tut <= 5; g->acertos = s->acertos; g->erros = s->erros;
    g->qCursor = s->qCursor; g->quizTempo = s->quizTempo ? s->quizTempo : 15;
    g->tempo = s->tempo; g->pos = (Vector2){s->px, s->py};
}

static void EmbaralhaPerguntas(Jogo *g)
{
    for (int i = 0; i < PQ_N; i++) g->qOrdem[i] = i;
    for (int i = PQ_N - 1; i > 0; i--) {
        int j = rand() % (i + 1), t = g->qOrdem[i];
        g->qOrdem[i] = g->qOrdem[j]; g->qOrdem[j] = t;
    }
}

/* Escolhe QN perguntas sem repetir na sequência e ordena por dificuldade crescente */
static void IniciaQuiz(Jogo *g)
{
    for (int i = 0; i < QN; i++) g->qSel[i] = g->qOrdem[(g->qCursor + i) % PQ_N];
    g->qCursor = (g->qCursor + QN) % PQ_N;
    if (g->qCursor < QN) EmbaralhaPerguntas(g);
    for (int i = 0; i < QN; i++)
        for (int j = 0; j < QN - 1 - i; j++)
            if (PERGUNTAS[g->qSel[j]].dif > PERGUNTAS[g->qSel[j + 1]].dif) {
                int t = g->qSel[j]; g->qSel[j] = g->qSel[j + 1]; g->qSel[j + 1] = t;
            }
    g->qI = 0; g->qAcertos = 0; g->qResp = -1; g->qHint = 0; g->qFeedback = 0;
    g->qTempo = (float)g->quizTempo;
    g->qBeep = 99; g->qCur = 0; g->qRev = g->quizFeito; g->qPts = 0; g->qGanho = 0;
    g->estado = ST_QUIZ;
    TutAvanca(g, 1);
}

static void IniciaLock(Jogo *g)
{
    g->lkD = 0; g->lkOk = 0; g->lkFase = 0; g->lkSel = 0; g->lkT = 30; g->lkAux = 0; g->lkFl = 0;
    memcpy(g->lkV, LK_INI[0], sizeof g->lkV);
    g->estado = ST_LOCK;
}

static void AbreDialogo(Jogo *g, int ch)
{
    g->dlgCh = ch; g->chAtual = ch; g->dlgSel = 0;
    const Chamado *c = &CHAMADOS[ch];
    (void)c;
    if (g->chEst[ch] == CH_NOVO) g->dlgFase = 0;
    else if (g->chEst[ch] == CH_INVESTIGANDO) {
        int tot, ok = EvidDoChamado(g, ch, &tot);
        g->dlgFase = (ok >= tot) ? 3 : 4;
    } else g->dlgFase = 5;
    g->estado = ST_DLG;
    TutAvanca(g, 2);
}

/* Derrota: confiança pública zerada. Encerra a campanha e invalida o save. */
static void ChecaDerrota(Jogo *g)
{
    if (g->conf <= 0 && g->estado != ST_FIM) {
        g->finalId = 4; g->estado = ST_FIM;
        g->certOk = 0; g->pauseSel = 0;
        Salvar(g, 1);
    }
}

static const char *TituloFinal(int id)
{
    switch (id) {
    case 1: return "FINAL 1 - Cidade transparente";
    case 2: return "FINAL 2 - Cidade ética, porém informada";
    case 3: return "FINAL 3 - Confiança abalada";
    default: return "FINAL 4 - A NÓVA sem freios";
    }
}

static const char *TextoFinal(int id)
{
    switch (id) {
    case 1: return "A auditoria virou política pública: decisões da NÓVA agora trazem justificativa e revisão humana.";
    case 2: return "A cidade passou a questionar a NÓVA. Há muito a corrigir, mas as pessoas sabem onde procurar respostas.";
    case 3: return "Os problemas foram vistos, mas a falta de provas e de transparência deixou a população dividida.";
    default: return "Sem confiança e sem auditoria, a NÓVA segue decidindo em silêncio. Mas ainda dá tempo de recomeçar.";
    }
}

static void Concluir(Jogo *g)
{
    if (g->conf >= 75 && EvidTotais(g) >= 3) g->finalId = 1;
    else if (g->conf >= 55) g->finalId = 2;
    else if (g->conf >= 30) g->finalId = 3;
    else g->finalId = 4;
    g->estado = ST_FIM;
    g->pauseSel = 0;
    Salvar(g, 1);
    FILE *f = fopen(CERT_FILE, "w");
    if (f) {
        fprintf(f, "CERTIFICADO DE CONCLUSÃO - REV.IA\nDepartamento de Alfabetização Algorítmica\n\n");
        fprintf(f, "%s\nConfiança pública: %d\nPontos: %d\nEvidências: %d/%d\n", TituloFinal(g->finalId), g->conf, g->pts, EvidTotais(g), EV_N);
        fprintf(f, "Acertos no quiz: %d\nErros: %d\nTempo: %02d:%02d\n", g->acertos, g->erros, (int)g->tempo / 60, (int)g->tempo % 60);
        fclose(f);
    }
    g->certOk = (f != NULL);
}

/* ------------------------------------------------------------------ */
/* Mundo: colisão e interação                                          */
/* ------------------------------------------------------------------ */
static Rectangle RectJog(float x, float y) { return (Rectangle){x - 9, y - 8, 18, 14}; }

static int Colide(const Jogo *g, Rectangle r)
{
    for (int i = 0; i < SOL_N; i++) if (CheckCollisionRecs(r, SOLIDOS[i])) return 1;
    for (int i = 0; i < OBJ_N; i++) if (CheckCollisionRecs(r, OBJETOS[i].r)) return 1;
    if (!g->lockAberto && CheckCollisionRecs(r, BARREIRA)) return 1;
    return 0;
}

static int ObjetoProximo(const Jogo *g)
{
    Rectangle p = RectJog(g->pos.x, g->pos.y);
    p.x -= 26; p.y -= 26; p.width += 52; p.height += 52;
    int melhor = -1;
    float dmin = 1e9f;
    for (int i = 0; i < OBJ_N; i++) {
        if (!CheckCollisionRecs(p, OBJETOS[i].r)) continue;
        float dx = OBJETOS[i].r.x + OBJETOS[i].r.width / 2 - g->pos.x;
        float dy = OBJETOS[i].r.y + OBJETOS[i].r.height / 2 - g->pos.y;
        float d = dx * dx + dy * dy;
        if (d < dmin) { dmin = d; melhor = i; }
    }
    return melhor;
}

static void Interage(Jogo *g, int i)
{
    const Objeto *o = &OBJETOS[i];
    switch (o->tipo) {
    case K_QUIZ:
        IniciaQuiz(g);
        break;
    case K_CIDADAO:
        AbreDialogo(g, o->id);
        break;
    case K_EVID: {
        int ev = o->id, ch = EVIDENCIAS[ev].chamado;
        if (g->found[ev]) Msg(g, "Evidência já registrada no caderno.", CINZA);
        else if (g->chEst[ch] == CH_NOVO) Msg(g, "Atenda o chamado deste cidadão antes de investigar.", AMARELO);
        else if (g->chEst[ch] == CH_IGNORADO) Msg(g, "O chamado foi encerrado. Não há o que investigar aqui.", VERM);
        else {
            int tot;
            g->found[ev] = 1;
            GanhaPts(g, 2); MudaConf(g, 2);
            if (EvidDoChamado(g, ch, &tot) >= tot)
                Msg(g, "Evidências completas! Volte ao cidadão para concluir o caso.", VERDE);
            else
                Msg(g, TextFormat("Evidência registrada: %s", EVIDENCIAS[ev].nome), VERDE);
            Sfx(g, g->sOk);
            TutAvanca(g, 3);
            Salvar(g, 0);
        }
        break; }
    case K_LOCK:
        if (g->lockAberto) Msg(g, "Zona Crédito já liberada.", CINZA);
        else if (!g->quizFeito) Msg(g, "Zona bloqueada: conclua o treinamento (acerte 3 de 5).", AMARELO);
        else IniciaLock(g);
        break;
    case K_CRISE:
        if (g->chEst[0] == CH_NOVO || g->chEst[1] == CH_NOVO)
            Msg(g, "Atenda os dois chamados antes de acionar a Sala de Crise.", AMARELO);
        else { g->criseSel = 0; g->criseRes = 0; g->estado = ST_CRISE; }
        break;
    }
}

/* ================================================================== */
/*  CONTINUAÇÃO DO PROJETO                                             */
/*  Entrada, atualização de cada estado, desenho das telas e main().   */
/* ================================================================== */

#define VEL 175.0f              /* velocidade do jogador (px/s) */

static const int QT_OPC[4] = {10, 15, 20, 30};
static const char *CH_EST_TXT[4] = {"Novo", "Investigando", "Resolvido", "Encerrado sem investigação"};

enum { M_INI, M_CONT, M_CFG, M_AUD, M_CRED, M_SAIR };
static const char *MENU_TXT[6] = {"Iniciar jogo", "Continuar", "Configurações", "Áudio", "Créditos", "Sair"};

/* Textos da Sala de Crise (escolha ética final) */
static const char *CRISE_EVENTO =
    "A imprensa descobriu que moradores do Bairro Sul sofrem mais recusas automáticas. "
    "A prefeitura exige um posicionamento do Departamento sobre a NÓVA. O que você faz com o que descobriu?";
static const char *CRISE_OP[4] = {
    "Divulgar as evidências e solicitar uma revisão pública",
    "Solicitar uma auditoria formal antes de divulgar",
    "Corrigir silenciosamente o cadastro e encerrar o caso",
    "Ignorar as evidências"};
static const char *CRISE_DICA[4] = {
    "Mais transparência, mas arrisca conflito se as provas forem incompletas.",
    "Conclusões mais confiáveis, porém exige mais tempo.",
    "Resolve casos individuais, mas não enfrenta o problema estrutural.",
    "Evita atritos agora, mas deixa a população sem resposta."};

/* ------------------------------------------------------------------ */
/* Entrada (teclado e mouse)                                           */
/* ------------------------------------------------------------------ */
static int Sobre(Rectangle r) { return CheckCollisionPointRec(MouseV(), r); }
static int Clique(Rectangle r) { return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Sobre(r); }
static int MouseMexeu(void) { Vector2 d = GetMouseDelta(); return d.x != 0 || d.y != 0; }
static int Cima(void)  { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
static int Baixo(void) { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
static int Esq(void)   { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
static int Dir(void)   { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }
static int Voltar(void) { return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }

/* Navega em uma lista vertical de n botões. Retorna 1 se o item selecionado foi ativado. */
static int NavegaLista(int *sel, int n, float x, float y, float w, float h, float gap)
{
    if (Cima()) *sel = (*sel + n - 1) % n;
    if (Baixo()) *sel = (*sel + 1) % n;
    int ativa = Confirma();
    for (int i = 0; i < n; i++) {
        Rectangle r = {x, y + i * (h + gap), w, h};
        if (MouseMexeu() && Sobre(r)) *sel = i;
        if (Clique(r)) { *sel = i; ativa = 1; }
    }
    return ativa;
}

/* Texto que diminui a fonte até caber na largura máxima */
static void TextoFit(const char *t, int x, int y, int wmax, int sz, Color c)
{
    while (sz > 11 && MeasureText(t, sz) > wmax) sz--;
    DrawText(t, x, y, sz, c);
}

static void FundoTech(void)
{
    for (int x = 0; x < W; x += 40) DrawLine(x, 0, x, H, Fade(VERDE, 0.05f));
    for (int y = 0; y < H; y += 40) DrawLine(0, y, W, y, Fade(VERDE, 0.05f));
    int sy = (int)fmodf((float)GetTime() * 40.0f, (float)H);
    DrawRectangle(0, sy, W, 2, Fade(VERDE, 0.07f));
}

/* ------------------------------------------------------------------ */
/* Sessão: inicialização de partidas, áudio e câmera                   */
/* ------------------------------------------------------------------ */
static void AtualizaCamera(Jogo *g, float dt, int snap)
{
    float alvo = clampf(g->pos.x, W / 2.0f, WW - W / 2.0f);
    if (snap) g->cam.target.x = alvo;
    else g->cam.target.x += (alvo - g->cam.target.x) * fminf(1.0f, dt * 8.0f);
    g->cam.target.y = H / 2.0f;
    g->cam.offset = (Vector2){W / 2.0f, H / 2.0f};
    g->cam.rotation = 0;
    g->cam.zoom = 1;
}

static void PreparaSessao(Jogo *g)
{
    EmbaralhaPerguntas(g);
    g->tutT = 0;
    if (Colide(g, RectJog(g->pos.x, g->pos.y))) g->pos = (Vector2){120, 400};
    AtualizaCamera(g, 0, 1);
}

static void IniciarNovo(Jogo *g)
{
    NovoJogo(g);
    g->qCursor = 0;
    PreparaSessao(g);
}

static void AtualizaAudio(Jogo *g)
{
    g->audioOk = g->audioDev && g->audioOn;
    if (!g->audioDev || !g->sAmb.stream.buffer) return;
    if (g->audioOk && g->volMusica > 0.01f) {
        SetSoundVolume(g->sAmb, g->volMusica * 0.35f);
        if (!IsSoundPlaying(g->sAmb)) PlaySound(g->sAmb);
    } else if (IsSoundPlaying(g->sAmb)) {
        StopSound(g->sAmb);
    }
}

static void EntraPausa(Jogo *g)
{
    g->volta = g->estado;
    g->pauseSel = 0;
    g->confirma = 0;
    g->estado = ST_PAUSE;
}

/* ------------------------------------------------------------------ */
/* Menu principal, configurações, áudio, créditos e introdução         */
/* ------------------------------------------------------------------ */
static int MenuItens(const Jogo *g, int *ids)
{
    int n = 0;
    ids[n++] = M_INI;
    if (g->temSave) ids[n++] = M_CONT;
    ids[n++] = M_CFG; ids[n++] = M_AUD; ids[n++] = M_CRED; ids[n++] = M_SAIR;
    return n;
}

static void MenuAtiva(Jogo *g, int id)
{
    switch (id) {
    case M_INI:
        IniciarNovo(g);
        g->introPg = 0; g->introRev = 0; g->estado = ST_INTRO;
        break;
    case M_CONT: {
        Save s;
        if (LerSave(&s)) {
            CarregaJogo(g, &s);
            PreparaSessao(g);
            g->estado = ST_PLAY;
            Msg(g, "Progresso carregado.", VERDE);
        } else {
            g->temSave = 0; g->saveStatus = 2;
            Msg(g, "Arquivo de save inválido: inicie uma nova partida.", VERM);
        }
        break; }
    case M_CFG: g->cfgSel = 0; g->cfgVolta = ST_MENU; g->estado = ST_CONFIG; break;
    case M_AUD: g->audSel = 0; g->audVolta = ST_MENU; g->estado = ST_AUDIO; break;
    case M_CRED: g->estado = ST_CRED; break;
    case M_SAIR: g->sair = 1; break;
    }
}

static void AtualizaMenu(Jogo *g)
{
    int ids[6], n = MenuItens(g, ids);
    g->menuSel = clampi(g->menuSel, 0, n - 1);
    if (NavegaLista(&g->menuSel, n, 330, 236, 300, 38, 6)) {
        Sfx(g, g->sClick);
        MenuAtiva(g, ids[g->menuSel]);
    }
}

static void AtualizaConfig(Jogo *g)
{
    if (Voltar()) { g->estado = g->cfgVolta; return; }
    int ativa = NavegaLista(&g->cfgSel, 4, 250, 170, 460, 44, 12);
    if (g->cfgSel == 0) {
        int k = 1;
        for (int i = 0; i < 4; i++) if (QT_OPC[i] == g->quizTempo) k = i;
        int d = (Dir() ? 1 : 0) - (Esq() ? 1 : 0);
        if (ativa && d == 0) d = 1;
        if (d != 0) { k = (k + 4 + d) % 4; g->quizTempo = QT_OPC[k]; Sfx(g, g->sClick); }
    } else if (ativa && g->cfgSel == 1) {
        if (g->cfgVolta == ST_PAUSE) {            /* rever tutorial dentro da partida */
            g->tut = 0; g->tutAtivo = 1; g->tutT = 0;
            g->estado = g->volta;
            Msg(g, "Tutorial reiniciado.", VERDE);
        } else {                                   /* a partir do menu: mostra as instruções */
            g->introRev = 1; g->introPg = 2; g->estado = ST_INTRO;
        }
    } else if (ativa && g->cfgSel == 2) {
        g->audSel = 0; g->audVolta = ST_CONFIG; g->estado = ST_AUDIO;
    } else if (ativa && g->cfgSel == 3) {
        g->estado = g->cfgVolta;
    }
}

static void AtualizaAudioMenu(Jogo *g)
{
    if (Voltar()) { g->estado = g->audVolta; return; }
    int ativa = NavegaLista(&g->audSel, 4, 250, 170, 460, 44, 12);
    int d = (Dir() ? 1 : 0) - (Esq() ? 1 : 0);
    if (g->audSel == 0 && (ativa || d)) {
        g->audioOn = !g->audioOn;
        AtualizaAudio(g);
        Sfx(g, g->sClick);
    } else if (g->audSel == 1 && (ativa || d)) {
        g->volMusica = d ? clampf(g->volMusica + 0.1f * d, 0, 1) : (g->volMusica >= 0.95f ? 0 : g->volMusica + 0.1f);
        AtualizaAudio(g);
    } else if (g->audSel == 2 && (ativa || d)) {
        g->volSfx = d ? clampf(g->volSfx + 0.1f * d, 0, 1) : (g->volSfx >= 0.95f ? 0 : g->volSfx + 0.1f);
        Sfx(g, g->sOk);
    } else if (g->audSel == 3 && ativa) {
        g->estado = g->audVolta;
    }
}

static void AtualizaIntro(Jogo *g)
{
    int fim = Voltar();
    if (Confirma() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g->introPg++;
        if (g->introPg >= 3) fim = 1;
    }
    if (fim) {
        if (g->introRev) g->estado = ST_CONFIG;
        else { g->estado = ST_PLAY; Msg(g, "Primeiro chamado: Sr. Marcos. Antes, treine no terminal.", AMARELO); }
        g->introRev = 0;
    }
}

/* ------------------------------------------------------------------ */
/* Exploração                                                          */
/* ------------------------------------------------------------------ */
static void AtualizaJogador(Jogo *g, float dt)
{
    float dx = 0, dy = 0;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dx -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx += 1;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dy -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dy += 1;
    if (dx == 0 && dy == 0) return;
    float len = sqrtf(dx * dx + dy * dy);
    dx /= len; dy /= len;
    float passo = VEL * dt;
    float nx = clampf(g->pos.x + dx * passo, 14, WW - 14);
    if (!Colide(g, RectJog(nx, g->pos.y))) g->pos.x = nx;
    float ny = clampf(g->pos.y + dy * passo, 158, H - 18);
    if (!Colide(g, RectJog(g->pos.x, ny))) g->pos.y = ny;
    g->distAndada += passo;
    g->anim += dt * 9.0f;
    if (g->distAndada > 120) TutAvanca(g, 0);
}

static void AtualizaPlay(Jogo *g, float dt)
{
    g->tempo += dt;
    AtualizaJogador(g, dt);
    AtualizaCamera(g, dt, 0);
    if (g->tutAtivo && g->tut == 5) {
        g->tutT += dt;
        if (g->tutT > 7.0f) { g->tut = 6; g->tutAtivo = 0; }
    }
    int i = ObjetoProximo(g);
    if (i >= 0 && IsKeyPressed(KEY_E)) Interage(g, i);
    if (g->estado != ST_PLAY) return;
    if (IsKeyPressed(KEY_TAB)) { g->nbSel = 0; g->estado = ST_NOTE; TutAvanca(g, 4); return; }
    if (IsKeyPressed(KEY_ESCAPE)) EntraPausa(g);
}

static Rectangle NotaRect(int i) { return (Rectangle){60, 112 + i * 52, 320, 44}; }

static void AtualizaNota(Jogo *g)
{
    if (IsKeyPressed(KEY_TAB) || Voltar()) { g->estado = ST_PLAY; return; }
    if (Cima()) g->nbSel = (g->nbSel + EV_N - 1) % EV_N;
    if (Baixo()) g->nbSel = (g->nbSel + 1) % EV_N;
    for (int i = 0; i < EV_N; i++) {
        Rectangle r = NotaRect(i);
        if ((MouseMexeu() && Sobre(r)) || Clique(r)) g->nbSel = i;
    }
}

/* ------------------------------------------------------------------ */
/* Diálogo / chamados                                                  */
/* ------------------------------------------------------------------ */
static Rectangle DlgOp(int i) { return (Rectangle){60, 316 + i * 54, 840, 44}; }

/* Aplica as consequências da escolha do jogador no chamado */
static void EscolheOpcao(Jogo *g, int o)
{
    const Chamado *c = &CHAMADOS[g->dlgCh];
    g->dlgSel = o;
    g->dlgFase = 1;
    MudaConf(g, c->dConf[o]);
    GanhaPts(g, c->dPts[o]);
    g->chEst[g->dlgCh] = c->estadoApos[o];
    int ev = c->revela[o];
    if (ev >= 0 && !g->found[ev]) {
        g->found[ev] = 1;
        GanhaPts(g, 2);
        Msg(g, TextFormat("Evidência registrada: %s", EVIDENCIAS[ev].nome), VERDE);
    } else {
        Msg(g, "Chamado atualizado.", c->dConf[o] >= 0 ? VERDE : VERM);
    }
    Sfx(g, c->dConf[o] >= 0 ? g->sOk : g->sBad);
    TutAvanca(g, 2);
    Salvar(g, 0);
}

static void AtualizaDlg(Jogo *g)
{
    if (IsKeyPressed(KEY_ESCAPE)) { g->estado = ST_PLAY; return; }
    int ch = g->dlgCh;
    if (g->dlgFase == 0) {
        int o = -1;
        if (Cima()) g->dlgSel = (g->dlgSel + 2) % 3;
        if (Baixo()) g->dlgSel = (g->dlgSel + 1) % 3;
        if (IsKeyPressed(KEY_ONE)) o = 0;
        if (IsKeyPressed(KEY_TWO)) o = 1;
        if (IsKeyPressed(KEY_THREE)) o = 2;
        if (Confirma()) o = g->dlgSel;
        for (int i = 0; i < 3; i++) {
            Rectangle r = DlgOp(i);
            if (MouseMexeu() && Sobre(r)) g->dlgSel = i;
            if (Clique(r)) o = i;
        }
        if (o >= 0) EscolheOpcao(g, o);
        return;
    }
    if (Confirma() || IsKeyPressed(KEY_E) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g->dlgFase == 3) {                       /* evidências completas: resolve o chamado */
            g->chEst[ch] = CH_RESOLVIDO;
            GanhaPts(g, 5);
            MudaConf(g, 8);
            Msg(g, "Chamado resolvido com base em evidências.", VERDE);
            Sfx(g, g->sOk);
            Salvar(g, 0);
        }
        g->estado = ST_PLAY;
    }
}

/* ------------------------------------------------------------------ */
/* Quiz                                                                */
/* ------------------------------------------------------------------ */
static Rectangle QuizOp(int i) { return (Rectangle){70, 236 + i * 46, 820, 40}; }

static void QuizResponde(Jogo *g, int o)
{
    const Pergunta *p = &PERGUNTAS[g->qSel[g->qI]];
    g->qResp = o; g->qFeedback = 1; g->qGanho = 0;
    if (o == p->ok) {
        g->qAcertos++; g->acertos++;
        if (!g->qRev) {                              /* modo revisão não pontua */
            g->qGanho = g->qHint ? p->dif : p->dif * 2;
            GanhaPts(g, g->qGanho);
            g->qPts += g->qGanho;
        }
        Sfx(g, g->sOk);
    } else {
        g->erros++;
        Sfx(g, g->sBad);
    }
}

static void QuizAvanca(Jogo *g)
{
    g->qI++;
    if (g->qI >= QN) {
        g->qFeedback = 2;
        if (g->qAcertos >= 3 && !g->quizFeito) {
            g->quizFeito = 1;
            GanhaPts(g, 3); g->qPts += 3;
            if (g->qAcertos == QN) { GanhaPts(g, 2); g->qPts += 2; }
            MudaConf(g, 3);
            Salvar(g, 0);
        }
        return;
    }
    g->qResp = -1; g->qHint = 0; g->qFeedback = 0;
    g->qTempo = (float)g->quizTempo; g->qBeep = 99; g->qCur = 0;
}

static void AtualizaQuiz(Jogo *g, float dt)
{
    if (IsKeyPressed(KEY_ESCAPE)) { EntraPausa(g); return; }
    if (g->qFeedback == 0) {
        g->qTempo -= dt;
        int seg = (int)ceilf(g->qTempo);
        if (g->qTempo > 0 && seg <= 3 && seg != g->qBeep) { g->qBeep = seg; Sfx(g, g->sClick); }
        if (g->qTempo <= 0) { g->qTempo = 0; QuizResponde(g, -2); return; }
        int o = -1;
        if (Cima()) g->qCur = (g->qCur + 2) % 3;
        if (Baixo()) g->qCur = (g->qCur + 1) % 3;
        if (IsKeyPressed(KEY_ONE)) o = 0;
        if (IsKeyPressed(KEY_TWO)) o = 1;
        if (IsKeyPressed(KEY_THREE)) o = 2;
        if (Confirma()) o = g->qCur;
        if (IsKeyPressed(KEY_H)) g->qHint = 1;
        for (int i = 0; i < 3; i++) {
            Rectangle r = QuizOp(i);
            if (MouseMexeu() && Sobre(r)) g->qCur = i;
            if (Clique(r)) o = i;
        }
        if (o >= 0) QuizResponde(g, o);
    } else if (Confirma() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g->qFeedback == 1) QuizAvanca(g);
        else g->estado = ST_PLAY;
    }
}

/* ------------------------------------------------------------------ */
/* Cadeado lógico                                                      */
/* ------------------------------------------------------------------ */
static Rectangle LkRect(int i) { return (Rectangle){120 + i * 250, 270, 220, 92}; }

/* Procura (por força bruta) uma combinação que satisfaz o desafio d */
static int lkSolucao(int d, int *out)
{
    for (int m = 0; m < 8; m++) {
        int v[3] = {m & 1, (m >> 1) & 1, (m >> 2) & 1};
        if (lkAvalia(d, v)) { memcpy(out, v, sizeof v); return 1; }
    }
    return 0;
}

static void LkTenta(Jogo *g)
{
    if (lkAvalia(g->lkD, g->lkV)) {
        GanhaPts(g, g->lkT > 15 ? 4 : 3);
        Sfx(g, g->sOk);
        g->lkD++;
        g->lkFl = 1; g->lkAux = 1.4f;
        if (g->lkD >= LK_N) {
            g->lkFase = 1; g->lkOk = 1; g->lockAberto = 1;
            GanhaPts(g, 3); MudaConf(g, 3);
            Msg(g, "Cadeado aberto: Zona Crédito liberada.", VERDE);
            Salvar(g, 0);
        } else {
            memcpy(g->lkV, LK_INI[g->lkD], sizeof g->lkV);
            g->lkT = 30; g->lkSel = 0;
        }
    } else {
        g->lkFl = -1; g->lkAux = 1.4f;
        Sfx(g, g->sBad);
    }
}

static void AtualizaLock(Jogo *g, float dt)
{
    if (IsKeyPressed(KEY_ESCAPE)) { EntraPausa(g); return; }
    if (g->lkAux > 0) g->lkAux -= dt;
    if (g->lkFase != 0) {
        if (Confirma() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g->estado = ST_PLAY;
        return;
    }
    g->lkT -= dt;
    if (g->lkT <= 0) { g->lkT = 0; g->lkFase = 2; Sfx(g, g->sBad); return; }
    if (Esq()) g->lkSel = (g->lkSel + 2) % 3;
    if (Dir()) g->lkSel = (g->lkSel + 1) % 3;
    int t = -1;
    if (IsKeyPressed(KEY_ONE)) t = 0;
    if (IsKeyPressed(KEY_TWO)) t = 1;
    if (IsKeyPressed(KEY_THREE)) t = 2;
    if (IsKeyPressed(KEY_SPACE)) t = g->lkSel;
    for (int i = 0; i < 3; i++) if (Clique(LkRect(i))) t = i;
    if (t >= 0) { g->lkV[t] = !g->lkV[t]; g->lkSel = t; Sfx(g, g->sClick); }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) LkTenta(g);
}

/* ------------------------------------------------------------------ */
/* Sala de crise (escolha ética final)                                 */
/* ------------------------------------------------------------------ */
static Rectangle CriseOp(int i) { return (Rectangle){90, 262 + i * 50, 780, 42}; }

static void DecideCrise(Jogo *g, int o)
{
    int ev = EvidTotais(g), dc, dp;
    const char *t;
    switch (o) {
    case 0:
        if (ev >= 3) {
            dc = 10; dp = 6;
            t = "Com evidências sólidas, a divulgação pública pressiona a prefeitura a rever a NÓVA. Há conflito institucional, mas a transparência fortalece a confiança.";
        } else {
            dc = -6; dp = 1;
            t = "Os dados ainda estavam incompletos. A divulgação gera conflito e acusações sem provas suficientes minam a confiança na auditoria.";
        }
        break;
    case 1:
        if (ev >= 2) {
            dc = 6; dp = 5;
            t = "A auditoria formal leva mais tempo, mas produz conclusões confiáveis e difíceis de contestar. A divulgação será feita com base em provas.";
        } else {
            dc = 3; dp = 2;
            t = "A auditoria é aberta, mas com poucas evidências ela parte quase do zero. Era um caminho prudente, porém lento.";
        }
        g->tempo += 120.0f;
        break;
    case 2:
        dc = -6; dp = 1;
        t = "Os cadastros individuais são corrigidos, mas o padrão estrutural permanece escondido. Quando a imprensa descobre a correção silenciosa, a confiança cai.";
        break;
    default:
        dc = -12; dp = 0;
        t = "Nada muda. A população percebe que as evidências foram ignoradas e a confiança no Departamento despenca.";
        break;
    }
    g->crise = o + 1; g->criseRes = 1;
    g->criseTxt = t; g->criseDC = dc; g->criseDP = dp;
    MudaConf(g, dc);
    GanhaPts(g, dp);
    Sfx(g, dc >= 0 ? g->sOk : g->sBad);
}

static void AtualizaCrise(Jogo *g)
{
    if (g->criseRes == 0) {
        if (Voltar()) { g->estado = ST_PLAY; return; }
        int o = -1;
        if (Cima()) g->criseSel = (g->criseSel + 3) % 4;
        if (Baixo()) g->criseSel = (g->criseSel + 1) % 4;
        if (IsKeyPressed(KEY_ONE)) o = 0;
        if (IsKeyPressed(KEY_TWO)) o = 1;
        if (IsKeyPressed(KEY_THREE)) o = 2;
        if (IsKeyPressed(KEY_FOUR)) o = 3;
        if (Confirma()) o = g->criseSel;
        for (int i = 0; i < 4; i++) {
            Rectangle r = CriseOp(i);
            if (MouseMexeu() && Sobre(r)) g->criseSel = i;
            if (Clique(r)) o = i;
        }
        if (o >= 0) DecideCrise(g, o);
    } else if (Confirma() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Concluir(g);
    }
}

/* ------------------------------------------------------------------ */
/* Pausa e tela final                                                  */
/* ------------------------------------------------------------------ */
static const Rectangle CONF_SIM = {350, 300, 120, 40};
static const Rectangle CONF_NAO = {490, 300, 120, 40};

static void AtualizaPausa(Jogo *g)
{
    if (g->confirma) {                               /* janela de confirmação */
        if (Esq() || Dir()) g->confSel = !g->confSel;
        int sim = IsKeyPressed(KEY_S);
        int nao = IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE);
        if (IsKeyPressed(KEY_Y)) sim = 1;
        if (Clique(CONF_SIM)) sim = 1;
        if (Clique(CONF_NAO)) nao = 1;
        if (Confirma()) { if (g->confSel) sim = 1; else nao = 1; }
        if (sim) {
            if (g->confirma == 1) {                  /* reiniciar a fase */
                IniciarNovo(g);
                g->estado = ST_PLAY;
                Msg(g, "Fase reiniciada.", AMARELO);
            } else {                                 /* voltar ao menu (salva o progresso) */
                Salvar(g, 0);
                g->estado = ST_MENU; g->menuSel = 0;
            }
            g->confirma = 0;
        } else if (nao) {
            g->confirma = 0;
        }
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE)) { g->estado = g->volta; return; }
    if (!NavegaLista(&g->pauseSel, 4, 330, 190, 300, 44, 10)) return;
    switch (g->pauseSel) {
    case 0: g->estado = g->volta; break;
    case 1: g->confirma = 1; g->confSel = 0; break;
    case 2: g->cfgSel = 0; g->cfgVolta = ST_PAUSE; g->estado = ST_CONFIG; break;
    default: g->confirma = 2; g->confSel = 0; break;
    }
}

static void AtualizaFim(Jogo *g)
{
    if (!NavegaLista(&g->pauseSel, 3, 330, 400, 300, 38, 6)) return;
    switch (g->pauseSel) {
    case 0: IniciarNovo(g); g->estado = ST_PLAY; Msg(g, "Nova campanha iniciada.", VERDE); break;
    case 1: g->estado = ST_MENU; g->menuSel = 0; break;
    default: g->sair = 1; break;
    }
}

/* ------------------------------------------------------------------ */
/* Atualização geral                                                   */
/* ------------------------------------------------------------------ */
static void Atualiza(Jogo *g, float dt)
{
    if (g->msgT > 0) g->msgT -= dt;
    if (g->popupT > 0) g->popupT -= dt;
    AtualizaAudio(g);
    switch (g->estado) {
    case ST_MENU:   AtualizaMenu(g); break;
    case ST_CONFIG: AtualizaConfig(g); break;
    case ST_AUDIO:  AtualizaAudioMenu(g); break;
    case ST_CRED:
        if (Confirma() || Voltar() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g->estado = ST_MENU;
        break;
    case ST_INTRO:  AtualizaIntro(g); break;
    case ST_PLAY:   AtualizaPlay(g, dt); break;
    case ST_DLG:    AtualizaDlg(g); break;
    case ST_QUIZ:   AtualizaQuiz(g, dt); break;
    case ST_LOCK:   AtualizaLock(g, dt); break;
    case ST_NOTE:   AtualizaNota(g); break;
    case ST_PAUSE:  AtualizaPausa(g); break;
    case ST_CRISE:  AtualizaCrise(g); break;
    case ST_FIM:    AtualizaFim(g); break;
    }
    if (g->estado == ST_PLAY || g->estado == ST_DLG || g->estado == ST_QUIZ ||
        g->estado == ST_LOCK || g->estado == ST_NOTE)
        ChecaDerrota(g);
}

/* ================================================================== */
/* DESENHO                                                             */
/* ================================================================== */
static const char *Objetivo(const Jogo *g)
{
    int tot, ok;
    if (!g->quizFeito) return "Treine no Terminal de Treinamento (acerte 3 de 5) - [E]";
    if (g->chEst[0] == CH_NOVO) return "Atenda o chamado do Sr. Marcos - [E]";
    if (g->chEst[0] == CH_INVESTIGANDO) {
        ok = EvidDoChamado(g, 0, &tot);
        if (ok < tot) return TextFormat("Reúna evidências do caso do Sr. Marcos (%d/%d)", ok, tot);
        return "Volte ao Sr. Marcos e conclua o caso";
    }
    if (!g->lockAberto) return "Resolva o Cadeado Lógico para abrir a Zona Crédito";
    if (g->chEst[1] == CH_NOVO) return "Atenda o chamado de Dona Lúcia - [E]";
    if (g->chEst[1] == CH_INVESTIGANDO) {
        ok = EvidDoChamado(g, 1, &tot);
        if (ok < tot) return TextFormat("Reúna evidências do caso de Dona Lúcia (%d/%d)", ok, tot);
        return "Volte à Dona Lúcia e conclua o caso";
    }
    return "Vá à Sala de Crise (extremo leste) e decida";
}

static void DesenhaObjeto(const Jogo *g, int i)
{
    const Objeto *o = &OBJETOS[i];
    Rectangle r = o->r;
    float cx = r.x + r.width / 2;
    float pulso = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
    switch (o->tipo) {
    case K_QUIZ:
        DrawRectangleRec(r, (Color){10, 40, 36, 255});
        DrawRectangleLinesEx(r, 2, VERDE);
        DrawRectangle((int)r.x + 8, (int)r.y + 8, (int)r.width - 16, (int)r.height - 26, (Color){0, 60, 40, 255});
        TextoCentro("?", (int)cx, (int)r.y + 12, 22, VERDE);
        DrawRectangle((int)r.x + 10, (int)(r.y + r.height - 12), (int)r.width - 20, 4, VERDE_E);
        break;
    case K_CIDADAO: {
        Color cor = o->id == 0 ? (Color){120, 180, 255, 255} : (Color){255, 170, 200, 255};
        int est = g->chEst[o->id];
        DrawEllipse((int)cx, (int)(r.y + r.height), 14, 4, Fade(BLACK, 0.5f));
        DrawRectangleRounded(r, 0.4f, 6, cor);
        DrawCircle((int)cx, (int)r.y - 6, 10, (Color){235, 220, 200, 255});
        if (est == CH_NOVO) TextoCentro("!", (int)cx, (int)(r.y - 40 - pulso * 4), 24, AMARELO);
        else if (est == CH_INVESTIGANDO) TextoCentro("...", (int)cx, (int)r.y - 34, 20, AMARELO);
        else if (est == CH_RESOLVIDO) TextoCentro("OK", (int)cx, (int)r.y - 34, 18, VERDE);
        else TextoCentro("X", (int)cx, (int)r.y - 34, 18, VERM);
        break; }
    case K_EVID: {
        int f = g->found[o->id];
        int ch = EVIDENCIAS[o->id].chamado;
        int disp = !f && (g->chEst[ch] == CH_INVESTIGANDO);
        DrawRectangleRec(r, (Color){12, 30, 34, 255});
        DrawRectangleLinesEx(r, 2, f ? VERDE_E : (disp ? AMARELO : CINZA));
        for (int k = 0; k < 3; k++) {
            int yy = (int)r.y + 8 + k * (int)((r.height - 16) / 3);
            DrawRectangle((int)r.x + 6, yy, (int)r.width - 12, 8, (Color){20, 50, 50, 255});
            DrawCircle((int)r.x + (int)r.width - 12, yy + 4, 2, f ? VERDE : (k == (int)(GetTime() * 3) % 3 ? AMARELO : CINZA));
        }
        if (disp) TextoCentro("!", (int)cx, (int)(r.y - 24 - pulso * 3), 22, AMARELO);
        if (f) TextoCentro("OK", (int)cx, (int)r.y - 20, 16, VERDE);
        break; }
    case K_LOCK: {
        Color cor = g->lockAberto ? VERDE : VERM;
        DrawRectangleRec(r, (Color){12, 30, 34, 255});
        DrawRectangleLinesEx(r, 2, cor);
        DrawCircleLines((int)cx, (int)r.y + 18, 12, cor);
        DrawRectangle((int)r.x + 10, (int)r.y + 26, (int)r.width - 20, (int)r.height - 36, cor);
        TextoCentro(g->lockAberto ? "ABERTO" : "LOCK", (int)cx, (int)r.y + 36, 12, FUNDO);
        if (!g->lockAberto && g->quizFeito) TextoCentro("!", (int)cx, (int)(r.y - 24 - pulso * 3), 22, AMARELO);
        break; }
    case K_CRISE: {
        int livre = g->chEst[0] != CH_NOVO && g->chEst[1] != CH_NOVO;
        Color cor = livre ? AMARELO : CINZA;
        DrawRectangleRec(r, (Color){40, 24, 8, 255});
        DrawRectangleLinesEx(r, 3, cor);
        DrawRectangle((int)r.x + 6, (int)r.y + 6, (int)r.width - 12, 10, Fade(cor, 0.5f + 0.5f * pulso));
        TextoCentro("CRISE", (int)cx, (int)r.y + 30, 18, cor);
        if (livre) TextoCentro("!", (int)cx, (int)(r.y - 26 - pulso * 3), 22, AMARELO);
        break; }
    }
    TextoCentro(o->rotulo, (int)cx, (int)(r.y + r.height + 5), 12, CINZA);
}

static void DesenhaJogador(const Jogo *g)
{
    float bob = sinf(g->anim) * 2.0f;
    int x = (int)g->pos.x, y = (int)g->pos.y;
    DrawEllipse(x, y + 6, 10, 4, Fade(BLACK, 0.5f));
    DrawRectangle(x - 7, (int)(y - 18 + bob), 14, 18, VERDE);
    DrawRectangleLines(x - 7, (int)(y - 18 + bob), 14, 18, VERDE_E);
    DrawCircle(x, (int)(y - 24 + bob), 7, TEXTO);
    DrawRectangle(x - 4, (int)(y - 26 + bob), 8, 3, FUNDO);
}

static void DesenhaMundo(const Jogo *g)
{
    DrawRectangle(0, 56, WW, 94, (Color){8, 20, 22, 255});
    DrawLine(0, 150, WW, 150, VERDE_E);
    for (int x = 0; x <= WW; x += 40) DrawLine(x, 150, x, H, Fade(VERDE, 0.06f));
    for (int y = 150; y <= H; y += 40) DrawLine(0, y, WW, y, Fade(VERDE, 0.06f));
    DrawRectangle(1260, 150, WW - 1260, H - 150, (Color){0, 30, 50, 70});
    DrawText("DEPARTAMENTO DE ALFABETIZAÇÃO ALGORÍTMICA", 20, 100, 20, VERDE_E);
    DrawText("NÚCLEO DE CRÉDITO E BENEFÍCIOS", 1290, 100, 20, VERDE_E);
    DrawRectangleLines(0, 56, WW, H - 56, VERDE_E);

    for (int i = 0; i < SOL_N; i++) {
        Rectangle s = SOLIDOS[i];
        DrawRectangleRec(s, (Color){20, 40, 38, 255});
        DrawRectangleLinesEx(s, 2, CINZA);
        DrawLine((int)s.x, (int)s.y, (int)(s.x + s.width), (int)(s.y + s.height), CINZA);
        DrawLine((int)(s.x + s.width), (int)s.y, (int)s.x, (int)(s.y + s.height), CINZA);
    }
    if (!g->lockAberto) {
        DrawRectangleRec(BARREIRA, Fade(VERM, 0.35f));
        DrawRectangleLinesEx(BARREIRA, 2, VERM);
        DrawTextPro(GetFontDefault(), "ZONA BLOQUEADA", (Vector2){BARREIRA.x + 18, BARREIRA.y + 300},
                    (Vector2){0, 0}, -90.0f, 16, 2, VERM);
    } else {
        DrawRectangleLinesEx(BARREIRA, 1, Fade(VERDE, 0.3f));
    }

    /* objetos "atrás" do jogador, depois o jogador, depois os objetos "à frente" */
    for (int i = 0; i < OBJ_N; i++)
        if (OBJETOS[i].r.y + OBJETOS[i].r.height <= g->pos.y) DesenhaObjeto(g, i);
    DesenhaJogador(g);
    for (int i = 0; i < OBJ_N; i++)
        if (OBJETOS[i].r.y + OBJETOS[i].r.height > g->pos.y) DesenhaObjeto(g, i);

    if (g->estado == ST_PLAY) {                      /* indicador de interação */
        int p = ObjetoProximo(g);
        if (p >= 0) {
            const char *t = TextFormat("[E] %s", OBJETOS[p].rotulo);
            int tw = MeasureText(t, 16) + 16;
            Rectangle b = {OBJETOS[p].r.x + OBJETOS[p].r.width / 2 - tw / 2.0f, OBJETOS[p].r.y - 62, (float)tw, 24};
            if (b.y < 60) b.y = 60;
            Painel(b, AMARELO);
            DrawText(t, (int)b.x + 8, (int)b.y + 4, 16, AMARELO);
        }
    }
}

static void DesenhaHUD(const Jogo *g)
{
    Painel((Rectangle){6, 4, W - 12, 50}, VERDE_E);
    DrawText("CONFIANÇA PÚBLICA", 16, 9, 14, VERDE);
    Rectangle b = {16, 28, 190, 14};
    Color cc = g->conf >= 60 ? VERDE : (g->conf >= 30 ? AMARELO : VERM);
    DrawRectangleRec(b, (Color){10, 30, 28, 255});
    DrawRectangle((int)b.x, (int)b.y, (int)(b.width * g->conf / 100.0f), (int)b.height, cc);
    DrawRectangleLinesEx(b, 1, VERDE_E);
    DrawText(TextFormat("%d", g->conf), 214, 26, 18, cc);
    if (g->popupT > 0)
        DrawText(TextFormat("%+d", g->popup), 248, 26 - (int)((1.6f - g->popupT) * 8), 18, g->popup > 0 ? VERDE : VERM);
    DrawText("PONTOS", 296, 9, 14, VERDE);
    DrawText(TextFormat("%d", g->pts), 296, 26, 20, TEXTO);
    int tot;
    int okCh = EvidDoChamado(g, g->chAtual, &tot);
    DrawText("EVIDÊNCIAS", 380, 9, 14, VERDE);
    DrawText(TextFormat("%d/%d  (caso %d/%d)", EvidTotais(g), EV_N, okCh, tot), 380, 28, 15, TEXTO);
    DrawText("OBJETIVO", 520, 9, 14, VERDE);
    TextoFit(Objetivo(g), 520, 28, 330, 15, TEXTO);
    DrawText("TEMPO", 872, 9, 14, VERDE);
    DrawText(TextFormat("%02d:%02d", (int)g->tempo / 60, (int)g->tempo % 60), 872, 26, 18, TEXTO);
}

static void DesenhaMinimapa(const Jogo *g)
{
    Rectangle m = {W - 168, H - 50, 158, 40};
    Painel(m, VERDE_E);
    float sx = (m.width - 8) / WW, sy = (m.height - 8) / (float)(H - 150);
    float vx = g->cam.target.x - W / 2.0f;
    DrawRectangleLines((int)(m.x + 4 + vx * sx), (int)m.y + 4, (int)(W * sx), (int)m.height - 8, Fade(VERDE, 0.5f));
    for (int i = 0; i < OBJ_N; i++) {
        const Objeto *o = &OBJETOS[i];
        Color c = CINZA;
        switch (o->tipo) {
        case K_QUIZ: c = VERDE; break;
        case K_CIDADAO: c = AMARELO; break;
        case K_EVID: c = g->found[o->id] ? CINZA : (Color){90, 190, 255, 255}; break;
        case K_LOCK: c = g->lockAberto ? VERDE : VERM; break;
        case K_CRISE: c = (Color){255, 140, 40, 255}; break;
        }
        DrawRectangle((int)(m.x + 4 + (o->r.x + o->r.width / 2) * sx) - 1,
                      (int)(m.y + 4 + (o->r.y + o->r.height / 2 - 150) * sy) - 1, 3, 3, c);
    }
    if (((int)(GetTime() * 3)) % 2 == 0)
        DrawRectangle((int)(m.x + 4 + g->pos.x * sx) - 2, (int)(m.y + 4 + (g->pos.y - 150) * sy) - 2, 4, 4, WHITE);
}

static void DesenhaPlay(const Jogo *g)
{
    BeginMode2D(g->cam);
    DesenhaMundo(g);
    EndMode2D();
    DesenhaHUD(g);
    if (g->estado != ST_PLAY && g->estado != ST_PAUSE) return;
    DesenhaMinimapa(g);
    if (g->tutAtivo) {
        static const char *TUT[6] = {
            "Mova-se com WASD ou setas.",
            "Aproxime-se do Terminal de Treinamento e pressione E.",
            "Fale com o Sr. Marcos (E) para abrir o chamado dele.",
            "Escolha como agir e colete evidências nos terminais (E).",
            "Pressione TAB para abrir o Caderno de Evidências.",
            "Acompanhe CONFIANÇA PÚBLICA e PONTOS no topo da tela."};
        int p = clampi(g->tut, 0, 5);
        Rectangle t = {110, H - 100, 660, 40};
        Painel(t, AMARELO);
        TextoFit(TextFormat("TUTORIAL %d/6: %s", p + 1, TUT[p]), (int)t.x + 12, (int)t.y + 11, 636, 18, AMARELO);
    }
    if (g->msgT > 0) {
        Rectangle m = {90, H - 52, 690, 40};
        Painel(m, g->msgCor);
        TextoFit(g->msg, (int)m.x + 12, (int)m.y + 11, 666, 18, g->msgCor);
    }
}

static void Escurece(void) { DrawRectangle(0, 58, W, H - 58, Fade(BLACK, 0.7f)); }

static void DesenhaMenu(const Jogo *g)
{
    FundoTech();
    TextoCentro("REV.IA", W / 2, 40, 72, VERDE);
    TextoCentro("Departamento de Alfabetização Algorítmica", W / 2, 118, 24, TEXTO);
    TextoCentro("A cidade confia nas decisões da NÓVA. Mas quem audita a inteligência artificial?", W / 2, 160, 16, CINZA);
    int ids[6], n = MenuItens(g, ids);
    for (int i = 0; i < n; i++)
        Botao((Rectangle){330, 236 + i * 44.0f, 300, 38}, MENU_TXT[ids[i]], i == g->menuSel, 1);
    if (g->msgT > 0) TextoCentro(g->msg, W / 2, 500, 16, g->msgCor);
    else if (g->saveStatus == 2) TextoCentro("Arquivo de save inválido ou corrompido: inicie uma nova partida.", W / 2, 500, 16, VERM);
    else TextoCentro("Setas/W-S + ENTER, ou use o mouse", W / 2, 510, 14, CINZA);
}

static void DesenhaConfig(const Jogo *g)
{
    FundoTech();
    TextoCentro("CONFIGURAÇÕES", W / 2, 80, 36, VERDE);
    Botao((Rectangle){250, 170, 460, 44}, TextFormat("Tempo por pergunta do quiz: %d s", g->quizTempo), g->cfgSel == 0, 1);
    Botao((Rectangle){250, 226, 460, 44}, "Rever tutorial", g->cfgSel == 1, 1);
    Botao((Rectangle){250, 282, 460, 44}, "Áudio...", g->cfgSel == 2, 1);
    Botao((Rectangle){250, 338, 460, 44}, "Voltar", g->cfgSel == 3, 1);
    TextoCentro("Esquerda/direita altera o tempo do quiz. ESC volta.", W / 2, 420, 14, CINZA);
}

static void DesenhaAudioMenu(const Jogo *g)
{
    FundoTech();
    TextoCentro("ÁUDIO", W / 2, 80, 36, VERDE);
    Botao((Rectangle){250, 170, 460, 44}, g->audioOn ? "Áudio: LIGADO" : "Áudio: DESLIGADO", g->audSel == 0, 1);
    Botao((Rectangle){250, 226, 460, 44}, TextFormat("Música ambiente: %d%%", (int)(g->volMusica * 100 + 0.5f)), g->audSel == 1, 1);
    Botao((Rectangle){250, 282, 460, 44}, TextFormat("Efeitos sonoros: %d%%", (int)(g->volSfx * 100 + 0.5f)), g->audSel == 2, 1);
    Botao((Rectangle){250, 338, 460, 44}, "Voltar", g->audSel == 3, 1);
    TextoCentro("Esquerda/direita ajusta o volume. ESC volta.", W / 2, 420, 14, CINZA);
    if (!g->audioDev) TextoCentro("Dispositivo de áudio indisponível: o jogo segue sem som.", W / 2, 450, 16, AMARELO);
}

static void DesenhaCred(void)
{
    FundoTech();
    TextoCentro("CRÉDITOS", W / 2, 70, 36, VERDE);
    Painel((Rectangle){120, 130, 720, 300}, VERDE_E);
    TextoCentro("REV.IA - Departamento de Alfabetização Algorítmica", W / 2, 150, 22, TEXTO);
    TextoCentro("Jogo educativo sobre algoritmos, dados, viés, privacidade,", W / 2, 200, 18, TEXTO);
    TextoCentro("transparência e responsabilidade na inteligência artificial.", W / 2, 224, 18, TEXTO);
    TextoCentro("Desenvolvido em linguagem C com a biblioteca raylib.", W / 2, 280, 18, VERDE);
    TextoCentro("A cidade, os cidadãos e a IA NÓVA são fictícios.", W / 2, 330, 16, CINZA);
    TextoCentro("Obrigado por jogar!", W / 2, 380, 20, AMARELO);
    TextoCentro("ENTER ou ESC para voltar", W / 2, 480, 14, CINZA);
}

static void DesenhaIntro(const Jogo *g)
{
    FundoTech();
    Painel((Rectangle){80, 90, 800, 360}, VERDE);
    int pg = clampi(g->introPg, 0, 2);
    if (pg == 0) {
        DrawText("BEM-VINDO(A), DECIFRADOR(A)", 110, 110, 28, VERDE);
        TextoQuebra("Você acabou de ser contratado(a) pelo Departamento de Alfabetização Algorítmica. "
                    "Seu trabalho é ajudar a cidade a entender como decisões automatizadas afetam a vida das pessoas.",
                    110, 170, 740, 22, TEXTO);
    } else if (pg == 1) {
        DrawText("A NÓVA", 110, 110, 28, VERDE);
        TextoQuebra("Uma inteligência artificial chamada NÓVA foi implantada em quase todos os serviços públicos: "
                    "crédito, saúde, trânsito, notícias e benefícios. Ninguém sabe ao certo como ela decide, e muita gente acredita "
                    "que, se foi o sistema, está certo. Sua missão é auditar essas decisões com evidências.",
                    110, 170, 740, 22, TEXTO);
    } else {
        DrawText("COMO JOGAR", 110, 110, 28, VERDE);
        DrawText("WASD / setas ... mover", 110, 160, 20, TEXTO);
        DrawText("E ... interagir (terminais, cidadãos, evidências)", 110, 188, 20, TEXTO);
        DrawText("TAB ... caderno de evidências", 110, 216, 20, TEXTO);
        DrawText("ESC ... pausa      1/2/3 ou mouse ... escolher respostas", 110, 244, 20, TEXTO);
        TextoQuebra("Seu primeiro chamado já chegou: o Sr. Marcos teve um auxílio negado. "
                    "Antes, faça o treinamento no terminal à esquerda.", 110, 300, 740, 20, AMARELO);
    }
    DrawText(TextFormat("%d/3", pg + 1), 110, 410, 16, CINZA);
    TextoCentro("ENTER ou clique: continuar   |   ESC: pular", W / 2, 415, 16, CINZA);
}

/* ---- Caderno ---- */
static int Campo(const char *rot, const char *txt, int x, int y, int w)
{
    DrawText(rot, x, y, 14, VERDE);
    return TextoQuebra(txt, x, y + 18, w, 17, TEXTO) + 6;
}

static void DesenhaNota(const Jogo *g)
{
    Escurece();
    Painel((Rectangle){40, 66, 880, 440}, VERDE);
    DrawText("CADERNO DE EVIDÊNCIAS", 60, 78, 22, VERDE);
    DrawText(TextFormat("Coletadas: %d/%d", EvidTotais(g), EV_N), 700, 82, 18, TEXTO);
    for (int i = 0; i < EV_N; i++) {
        Rectangle r = NotaRect(i);
        int sel = (i == g->nbSel);
        DrawRectangleRec(r, sel ? (Color){0, 60, 40, 255} : (Color){10, 24, 24, 255});
        DrawRectangleLinesEx(r, sel ? 3 : 1, g->found[i] ? VERDE : CINZA);
        if (g->found[i]) TextoFit(EVIDENCIAS[i].nome, (int)r.x + 10, (int)r.y + 13, 300, 17, TEXTO);
        else DrawText("??? (não encontrada)", (int)r.x + 10, (int)r.y + 13, 17, CINZA);
    }
    /* chamado atual */
    const Chamado *c = &CHAMADOS[g->chAtual];
    DrawRectangleLines(60, 330, 320, 166, VERDE_E);
    DrawText("CHAMADO ATUAL", 70, 336, 14, VERDE);
    TextoFit(TextFormat("%s - %s", c->nome, c->titulo), 70, 354, 300, 16, AMARELO);
    int y = TextoQuebra(c->decisao, 70, 376, 300, 14, TEXTO);
    TextoQuebra(c->info, 70, y + 4, 300, 14, CINZA);
    /* detalhe */
    const Evidencia *e = &EVIDENCIAS[g->nbSel];
    if (g->found[g->nbSel]) {
        DrawText(e->nome, 410, 112, 22, AMARELO);
        int yy = 146;
        yy = Campo("Descrição", e->desc, 410, yy, 490);
        yy = Campo("Origem", e->origem, 410, yy, 490);
        yy = Campo("Relevância para a investigação", e->relev, 410, yy, 490);
        yy = Campo("Relação com outras evidências", e->relacao, 410, yy, 490);
        (void)yy;
    } else {
        DrawText("Evidência ainda não encontrada.", 410, 120, 20, CINZA);
        TextoQuebra("Atenda o chamado do cidadão e investigue os terminais do mapa.", 410, 154, 490, 17, CINZA);
    }
    int tot, ok = EvidDoChamado(g, g->chAtual, &tot);
    DrawText(TextFormat("Caso atual: %d/%d evidências", ok, tot), 410, 392, 16, TEXTO);
    DrawText(TextFormat("%s: %s", CHAMADOS[0].nome, CH_EST_TXT[g->chEst[0]]), 410, 416, 15, CINZA);
    DrawText(TextFormat("%s: %s", CHAMADOS[1].nome, CH_EST_TXT[g->chEst[1]]), 410, 436, 15, CINZA);
    DrawText("Setas ou mouse: selecionar  |  TAB/ESC: fechar", 410, 470, 14, CINZA);
}

/* ---- Diálogo ---- */
static void DesenhaDlg(const Jogo *g)
{
    const Chamado *c = &CHAMADOS[g->dlgCh];
    int o = clampi(g->dlgSel, 0, 2);
    Escurece();
    Painel((Rectangle){30, 64, 900, 446}, VERDE);
    DrawText(TextFormat("CHAMADO #%02d  |  %s", g->dlgCh + 1, c->categoria), 56, 76, 16, CINZA);
    DrawText(TextFormat("%s - %s", c->nome, c->titulo), 56, 98, 24, VERDE);
    if (g->dlgFase == 0) {
        int y = TextoQuebra(TextFormat("\"%s\"", c->relato), 56, 136, 840, 18, TEXTO);
        DrawText("Decisão da NÓVA:", 56, y + 6, 15, VERDE);
        y = TextoQuebra(c->decisao, 56, y + 24, 840, 17, AMARELO);
        DrawText("Informações iniciais:", 56, y + 4, 15, VERDE);
        TextoQuebra(c->info, 56, y + 22, 840, 17, TEXTO);
        DrawText("Como você age?", 56, 292, 16, VERDE);
        for (int i = 0; i < 3; i++) {
            Rectangle r = DlgOp(i);
            int sel = (i == g->dlgSel);
            DrawRectangleRec(r, sel ? (Color){0, 70, 45, 255} : (Color){10, 24, 24, 255});
            DrawRectangleLinesEx(r, sel ? 3 : 1, sel ? VERDE : VERDE_E);
            TextoFit(TextFormat("%d) %s", i + 1, c->op[i]), (int)r.x + 14, (int)r.y + 13, 810, 18, sel ? VERDE : TEXTO);
        }
        TextoCentro("1-3, setas + ENTER ou mouse  |  ESC: sair do chamado", W / 2, 484, 14, CINZA);
    } else if (g->dlgFase == 1) {
        TextoQuebra(c->resp[o], 56, 150, 840, 21, TEXTO);
        DrawText(TextFormat("Confiança pública: %+d     Pontos: %+d%s", c->dConf[o], c->dPts[o],
                            c->revela[o] >= 0 ? "     (+2 evidência registrada no caderno)" : ""),
                 56, 330, 18, c->dConf[o] >= 0 ? VERDE : VERM);
        TextoCentro("ENTER: continuar", W / 2, 480, 16, CINZA);
    } else if (g->dlgFase == 3) {
        DrawText("Evidências reunidas! Conclusão do caso:", 56, 140, 18, VERDE);
        TextoQuebra(c->conclusao, 56, 172, 840, 21, TEXTO);
        TextoCentro("ENTER: concluir o caso (+5 pontos, +8 confiança)", W / 2, 480, 16, AMARELO);
    } else if (g->dlgFase == 4) {
        char falta[200] = "";
        for (int i = 0; i < EV_N; i++)
            if (EVIDENCIAS[i].chamado == g->dlgCh && !g->found[i]) {
                if (falta[0]) strncat(falta, ", ", sizeof falta - strlen(falta) - 1);
                strncat(falta, EVIDENCIAS[i].origem, sizeof falta - strlen(falta) - 1);
            }
        TextoQuebra("O caso segue em investigação. Ainda faltam evidências para sustentar uma conclusão.", 56, 150, 840, 21, TEXTO);
        DrawText("Procure em:", 56, 220, 16, VERDE);
        TextoQuebra(falta, 56, 244, 840, 20, AMARELO);
        TextoCentro("ENTER: voltar", W / 2, 480, 16, CINZA);
    } else {
        TextoQuebra(g->chEst[g->dlgCh] == CH_RESOLVIDO
                        ? "Este caso está resolvido. Obrigado(a) pelo trabalho com evidências."
                        : "Este chamado foi encerrado sem investigação. Não há mais o que fazer aqui.",
                    56, 150, 840, 21, TEXTO);
        TextoCentro("ENTER: voltar", W / 2, 480, 16, CINZA);
    }
}

/* ---- Quiz ---- */
static void DesenhaQuiz(const Jogo *g)
{
    Escurece();
    Painel((Rectangle){40, 64, 880, 440}, VERDE);
    if (g->qFeedback == 2) {
        DrawText("TREINAMENTO ALGORÍTMICO - RESULTADO", 70, 80, 22, VERDE);
        TextoCentro(TextFormat("Você acertou %d de %d perguntas.", g->qAcertos, QN), W / 2, 170, 28, TEXTO);
        if (g->qAcertos >= 3)
            TextoCentro("Treinamento concluído! O Cadeado Lógico pode ser tentado.", W / 2, 230, 20, VERDE);
        else
            TextoCentro("São necessários 3 acertos. Você pode repetir o treinamento.", W / 2, 230, 20, AMARELO);
        if (g->qRev) TextoCentro("Modo revisão: sem pontos.", W / 2, 270, 16, CINZA);
        else TextoCentro(TextFormat("Pontos nesta rodada: +%d", g->qPts), W / 2, 270, 18, VERDE);
        TextoCentro("ENTER ou clique: continuar", W / 2, 440, 16, CINZA);
        return;
    }
    const Pergunta *p = &PERGUNTAS[g->qSel[clampi(g->qI, 0, QN - 1)]];
    DrawText(TextFormat("TREINAMENTO ALGORÍTMICO  |  Pergunta %d/%d  |  Dificuldade %d/3%s", g->qI + 1, QN, p->dif,
                        g->qRev ? "  |  revisão" : ""), 70, 76, 18, VERDE);
    float fr = g->quizTempo > 0 ? clampf(g->qTempo / (float)g->quizTempo, 0, 1) : 0;
    Color ct = fr > 0.5f ? VERDE : (fr > 0.25f ? AMARELO : VERM);
    DrawRectangle(70, 100, 820, 10, (Color){10, 30, 28, 255});
    DrawRectangle(70, 100, (int)(820 * fr), 10, ct);
    DrawRectangleLines(70, 100, 820, 10, VERDE_E);
    if (g->qFeedback == 0) DrawText(TextFormat("TEMPO: %d s", (int)ceilf(g->qTempo)), 790, 76, 18, ct);
    TextoQuebra(p->q, 70, 122, 820, 22, TEXTO);
    for (int i = 0; i < 3; i++) {
        Rectangle r = QuizOp(i);
        Color borda = (g->qFeedback == 0 && i == g->qCur) ? VERDE : VERDE_E;
        Color cor = TEXTO;
        const char *suf = "";
        if (g->qFeedback == 1) {
            if (i == p->ok) { borda = VERDE; cor = VERDE; suf = "   [CORRETA]"; }
            else if (i == g->qResp) { borda = VERM; cor = VERM; suf = "   [SUA RESPOSTA]"; }
            else borda = CINZA;
        }
        DrawRectangleRec(r, (g->qFeedback == 0 && i == g->qCur) ? (Color){0, 70, 45, 255} : (Color){10, 24, 24, 255});
        DrawRectangleLinesEx(r, (g->qFeedback == 0 && i == g->qCur) || (g->qFeedback == 1 && i == p->ok) ? 3 : 1, borda);
        TextoFit(TextFormat("%c) %s%s", 'A' + i, p->op[i], suf), (int)r.x + 14, (int)r.y + 11, 790, 18, cor);
    }
    if (g->qFeedback == 0) {
        if (g->qHint) TextoQuebra(TextFormat("Dica: %s", p->dica), 70, 376, 820, 17, AMARELO);
        else DrawText("[H] mostrar dica (vale menos pontos)", 70, 376, 15, CINZA);
        TextoCentro("1-3, setas + ENTER ou mouse  |  ESC: pausa", W / 2, 474, 14, CINZA);
    } else {
        if (g->qResp == -2) DrawText("Tempo esgotado! Pergunta não respondida.", 70, 374, 18, VERM);
        else if (g->qResp == p->ok)
            DrawText(g->qRev ? "Correto!" : TextFormat("Correto! +%d ponto(s)", g->qGanho), 70, 374, 18, VERDE);
        else DrawText("Resposta incorreta: consulte a explicação.", 70, 374, 18, VERM);
        TextoQuebra(p->expl, 70, 400, 820, 17, TEXTO);
        TextoCentro("ENTER ou clique: continuar", W / 2, 474, 14, CINZA);
    }
}

/* ---- Cadeado lógico ---- */
static void DesenhaLock(const Jogo *g)
{
    Escurece();
    Painel((Rectangle){60, 70, 840, 400}, VERDE);
    int d = clampi(g->lkD, 0, LK_N - 1);
    DrawText(TextFormat("CADEADO LÓGICO  -  desafio %d de %d", d + 1, LK_N), 84, 84, 22, VERDE);
    float fr = clampf(g->lkT / 30.0f, 0, 1);
    Color ct = fr > 0.5f ? VERDE : (fr > 0.25f ? AMARELO : VERM);
    DrawRectangle(84, 118, 792, 10, (Color){10, 30, 28, 255});
    DrawRectangle(84, 118, (int)(792 * fr), 10, ct);
    DrawRectangleLines(84, 118, 792, 10, VERDE_E);
    DrawText(TextFormat("TEMPO RESTANTE: %d s", (int)ceilf(g->lkT)), 84, 134, 16, ct);

    if (g->lkFase == 1) {
        TextoCentro("CADEADO ABERTO", W / 2, 200, 40, VERDE);
        TextoCentro("Zona Crédito liberada! Você resolveu os 3 desafios.", W / 2, 260, 20, TEXTO);
        TextoCentro("ENTER ou clique: continuar", W / 2, 420, 16, CINZA);
        return;
    }
    if (g->lkFase == 2) {
        int s[3] = {0, 0, 0};
        TextoCentro("TEMPO ESGOTADO", W / 2, 190, 40, VERM);
        TextoCentro(LK_EXPR[d], W / 2, 250, 28, AMARELO);
        if (lkSolucao(d, s))
            TextoCentro(TextFormat("Uma solução: A = %c, B = %c, C = %c", s[0] ? 'V' : 'F', s[1] ? 'V' : 'F', s[2] ? 'V' : 'F'),
                        W / 2, 300, 20, TEXTO);
        TextoCentro("O cadeado continua trancado. Você pode tentar de novo no terminal.", W / 2, 350, 18, TEXTO);
        TextoCentro("ENTER ou clique: continuar", W / 2, 420, 16, CINZA);
        return;
    }
    TextoCentro("Torne a expressão VERDADEIRA (V):", W / 2, 160, 18, TEXTO);
    TextoCentro(LK_EXPR[d], W / 2, 188, 32, AMARELO);
    TextoCentro(TextFormat("Valores atuais:  A = %c    B = %c    C = %c", g->lkV[0] ? 'V' : 'F', g->lkV[1] ? 'V' : 'F', g->lkV[2] ? 'V' : 'F'),
                W / 2, 236, 18, TEXTO);
    for (int i = 0; i < 3; i++) {
        Rectangle r = LkRect(i);
        int v = g->lkV[i];
        DrawRectangleRec(r, v ? (Color){0, 70, 45, 255} : (Color){70, 24, 24, 255});
        DrawRectangleLinesEx(r, g->lkSel == i ? 4 : 2, v ? VERDE : VERM);
        DrawText(TextFormat("[%d]  %c", i + 1, 'A' + i), (int)r.x + 12, (int)r.y + 8, 24, TEXTO);
        DrawRectangle((int)r.x + 14, (int)r.y + 48, 60, 24, (Color){10, 24, 24, 255});
        DrawRectangleLines((int)r.x + 14, (int)r.y + 48, 60, 24, v ? VERDE : VERM);
        DrawRectangle((int)r.x + 14 + (v ? 32 : 0), (int)r.y + 48, 28, 24, v ? VERDE : VERM);
        DrawText(v ? "V (verdadeiro)" : "F (falso)", (int)r.x + 84, (int)r.y + 52, 16, v ? VERDE : VERM);
    }
    int res = lkAvalia(d, g->lkV);
    DrawText(res ? "RESULTADO:  VERDADEIRO (V)" : "RESULTADO:  FALSO (F)", 120, 380, 22, res ? VERDE : VERM);
    DrawText(res ? "CADEADO: pronto para abrir (ENTER)" : "CADEADO: TRANCADO", 520, 384, 18, res ? VERDE : VERM);
    if (g->lkAux > 0)
        TextoCentro(g->lkFl > 0 ? "Correto! Próximo desafio." : "Ainda falso: o cadeado continua trancado.", W / 2, 416, 18, g->lkFl > 0 ? VERDE : VERM);
    TextoCentro("1/2/3 ou clique: alternar  |  setas + ESPAÇO: alternar  |  ENTER: abrir  |  ESC: pausa", W / 2, 446, 14, CINZA);
}

/* ---- Sala de crise ---- */
static void DesenhaCrise(const Jogo *g)
{
    Escurece();
    Painel((Rectangle){60, 70, 840, 400}, AMARELO);
    DrawText("SALA DE CRISE", 84, 82, 24, AMARELO);
    if (g->criseRes == 0) {
        TextoQuebra(CRISE_EVENTO, 84, 118, 780, 18, TEXTO);
        DrawText(TextFormat("Evidências reunidas: %d/%d", EvidTotais(g), EV_N), 84, 204, 17, EvidTotais(g) >= 3 ? VERDE : AMARELO);
        DrawText("Como o Departamento deve agir?  (1-4, setas + ENTER ou mouse | ESC: voltar)", 84, 234, 15, VERDE);
        for (int i = 0; i < 4; i++) {
            Rectangle r = CriseOp(i);
            int sel = (i == g->criseSel);
            DrawRectangleRec(r, sel ? (Color){0, 70, 45, 255} : (Color){10, 24, 24, 255});
            DrawRectangleLinesEx(r, sel ? 3 : 1, sel ? VERDE : VERDE_E);
            TextoFit(TextFormat("%d) %s", i + 1, CRISE_OP[i]), (int)r.x + 12, (int)r.y + 4, 756, 18, sel ? VERDE : TEXTO);
            TextoFit(CRISE_DICA[i], (int)r.x + 30, (int)r.y + 25, 740, 12, CINZA);
        }
    } else {
        TextoQuebra(CRISE_OP[g->crise - 1], 84, 118, 780, 20, AMARELO);
        TextoQuebra(g->criseTxt, 84, 160, 780, 21, TEXTO);
        DrawText(TextFormat("Confiança pública: %+d     Pontos: %+d", g->criseDC, g->criseDP), 84, 320, 20, g->criseDC >= 0 ? VERDE : VERM);
        TextoCentro("ENTER: encerrar a auditoria geral da NÓVA", W / 2, 420, 18, AMARELO);
    }
}

/* ---- Pausa ---- */
static void DesenhaPausa(const Jogo *g)
{
    DrawRectangle(0, 0, W, H, Fade(BLACK, 0.6f));
    Painel((Rectangle){290, 120, 380, 300}, VERDE);
    TextoCentro("PAUSA", W / 2, 134, 32, VERDE);
    static const char *IT[4] = {"Continuar", "Reiniciar a fase", "Configurações", "Voltar ao menu principal"};
    for (int i = 0; i < 4; i++) Botao((Rectangle){330, 190 + i * 54.0f, 300, 44}, IT[i], i == g->pauseSel, 1);
    if (g->confirma) {
        DrawRectangle(0, 0, W, H, Fade(BLACK, 0.5f));
        Painel((Rectangle){300, 200, 360, 160}, AMARELO);
        TextoCentro(g->confirma == 1 ? "Reiniciar a fase?" : "Voltar ao menu principal?", W / 2, 214, 22, AMARELO);
        TextoCentro(g->confirma == 1 ? "O progresso atual será perdido." : "O progresso será salvo.", W / 2, 250, 16, TEXTO);
        Botao(CONF_SIM, "Sim", g->confSel == 1, 1);
        Botao(CONF_NAO, "Não", g->confSel == 0, 1);
    }
}

/* ---- Tela final ---- */
static void DesenhaFim(const Jogo *g)
{
    FundoTech();
    int idf = clampi(g->finalId, 1, 4);
    TextoCentro("AUDITORIA GERAL DA NÓVA", W / 2, 24, 18, CINZA);
    TextoCentro(TituloFinal(idf), W / 2, 50, 30, idf <= 2 ? VERDE : (idf == 3 ? AMARELO : VERM));
    TextoQuebra(TextoFinal(idf), 100, 108, 760, 20, TEXTO);
    Painel((Rectangle){100, 170, 760, 170}, VERDE_E);
    int res = (g->chEst[0] == CH_RESOLVIDO) + (g->chEst[1] == CH_RESOLVIDO);
    DrawText(TextFormat("Confiança pública: %d", g->conf), 124, 184, 20, TEXTO);
    DrawText(TextFormat("Pontos: %d", g->pts), 124, 214, 20, TEXTO);
    DrawText(TextFormat("Evidências: %d/%d", EvidTotais(g), EV_N), 124, 244, 20, TEXTO);
    DrawText(TextFormat("Chamados resolvidos: %d/%d", res, CH_N), 124, 274, 20, TEXTO);
    DrawText(TextFormat("Quiz: %d acertos, %d erros", g->acertos, g->erros), 500, 184, 20, TEXTO);
    DrawText(TextFormat("Tempo: %02d:%02d", (int)g->tempo / 60, (int)g->tempo % 60), 500, 214, 20, TEXTO);
    if (g->crise > 0) DrawText(TextFormat("Decisão final: opção %d", g->crise), 500, 244, 20, TEXTO);
    if (g->certOk) TextoCentro("Certificado salvo em: " CERT_FILE, W / 2, 352, 16, VERDE);
    else if (g->conf <= 0) TextoCentro("A confiança pública chegou a zero: a campanha foi encerrada.", W / 2, 352, 16, VERM);
    static const char *IT[3] = {"Jogar novamente", "Menu principal", "Sair"};
    for (int i = 0; i < 3; i++) Botao((Rectangle){330, 400 + i * 44.0f, 300, 38}, IT[i], i == g->pauseSel, 1);
}

static void DesenhaTela(const Jogo *g, Estado e)
{
    switch (e) {
    case ST_MENU:   DesenhaMenu(g); break;
    case ST_CONFIG: DesenhaConfig(g); break;
    case ST_AUDIO:  DesenhaAudioMenu(g); break;
    case ST_CRED:   DesenhaCred(); break;
    case ST_INTRO:  DesenhaIntro(g); break;
    case ST_PLAY:   DesenhaPlay(g); break;
    case ST_DLG:    DesenhaPlay(g); DesenhaDlg(g); break;
    case ST_QUIZ:   DesenhaPlay(g); DesenhaQuiz(g); break;
    case ST_LOCK:   DesenhaPlay(g); DesenhaLock(g); break;
    case ST_NOTE:   DesenhaPlay(g); DesenhaNota(g); break;
    case ST_CRISE:  DesenhaPlay(g); DesenhaCrise(g); break;
    case ST_PAUSE:  DesenhaTela(g, g->volta); DesenhaPausa(g); break;
    case ST_FIM:    DesenhaFim(g); break;
    }
}

/* ================================================================== */
/* main: janela, recursos, laço principal e encerramento              */
/* ================================================================== */
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(W, H, "REV.IA - Departamento de Alfabetização Algorítmica");
    SetWindowMinSize(480, 270);
    SetExitKey(KEY_NULL);                      /* ESC é usado para pausar, não para fechar */
    SetTargetFPS(60);
    srand((unsigned)time(NULL));

    Jogo g;
    memset(&g, 0, sizeof g);

    /* áudio: gerado em memória; o jogo funciona mesmo sem dispositivo de áudio */
    InitAudioDevice();
    g.audioDev = IsAudioDeviceReady();
    g.audioOn = 1; g.volMusica = 0.5f; g.volSfx = 0.7f;
    if (g.audioDev) {
        g.sOk = Tom(520, 880, 0.18f, 0.5f, 0);
        g.sBad = Tom(240, 130, 0.30f, 0.5f, 0);
        g.sClick = Tom(700, 700, 0.05f, 0.4f, 0);
        g.sAmb = Tom(110, 110, 4.0f, 0.5f, 165);
    }
    g.audioOk = g.audioDev;

    RenderTexture2D alvo = LoadRenderTexture(W, H);
    SetTextureFilter(alvo.texture, TEXTURE_FILTER_BILINEAR);

    g.quizTempo = 15;
    IniciarNovo(&g);
    Save tmp;
    g.saveStatus = ChecaSave(&tmp);
    g.temSave = (g.saveStatus == 1);
    g.estado = ST_MENU;

    while (!g.sair && !WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        Atualiza(&g, dt);

        BeginTextureMode(alvo);
        ClearBackground(FUNDO);
        DesenhaTela(&g, g.estado);
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        float s = fminf(sw / W, sh / H);
        Rectangle src = {0, 0, (float)W, -(float)H};
        Rectangle dst = {(sw - W * s) / 2, (sh - H * s) / 2, W * s, H * s};
        DrawTexturePro(alvo.texture, src, dst, (Vector2){0, 0}, 0, WHITE);
        EndDrawing();
    }

    /* liberação de recursos */
    UnloadRenderTexture(alvo);
    if (g.audioDev) {
        StopSound(g.sAmb);
        UnloadSound(g.sOk); UnloadSound(g.sBad); UnloadSound(g.sClick); UnloadSound(g.sAmb);
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}