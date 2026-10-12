#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* --- DIMENSOES DA JANELA E LIMITES DO QUIZ --- */
#define LARGURA_JANELA     1000
#define ALTURA_JANELA       650
#define TOTAL_PERGUNTAS      13
#define TOTAL_OPCOES          4
#define TAM_PERGUNTA        260
#define TAM_DICA            220
#define TAM_OPCAO           170
#define TAM_EXPLICACAO      480

/* --- REGRAS DA MISSAO --- */
#define TEMPO_POR_PERGUNTA 30.0f
#define CONFIANCA_INICIAL  50.0f

/* --- IDENTIFICADORES DAS TELAS (inteiro simples, sem enum customizado) --- */
#define TELA_MENU         0
#define TELA_INSTRUCOES    1
#define TELA_QUIZ          2
#define TELA_RESULTADO     3

/* ============================================================================
 * BANCO DE PERGUNTAS -- matrizes simples de char, sem struct nem ponteiro
 * proprio. Cada linha de "perguntas" corresponde a mesma linha de "dicas",
 * "opcoes", "resposta_certa" e "explicacoes".
 * ============================================================================ */
static const char perguntas[TOTAL_PERGUNTAS][TAM_PERGUNTA] = {
    "O que significa \"vies algoritmico\"?",
    "Um chatbot afirma, com confianca total, um fato historico que nunca aconteceu. Como se chama isso?",
    "Um modelo acerta 99% dos dados de treino, mas erra quase metade dos dados novos. Isso e chamado de:",
    "Uma empresa culpa apenas \"a IA\" por um acidente, sem investigar decisoes humanas de projeto. Isso fere o principio de:",
    "Qual o valor logico de V ^ F (Verdadeiro CONJUNCAO Falso)?",
    "Qual o valor logico de F v V (Falso DISJUNCAO Verdadeiro)?",
    "No XOR (OU exclusivo), qual o resultado de V + V?",
    "Se chove (P), entao a rua fica molhada (Q). Sabendo que P e verdadeiro, o que se conclui por MODUS PONENS?",
    "Qual o valor logico de F <-> F (bicondicional entre dois valores Falsos)?",
    "Ainda em \"se chove, a rua molha\": a rua NAO esta molhada (~Q). O que se conclui por MODUS TOLLENS?",
    "Pelas Leis de De Morgan, a negacao ~(P ^ Q) e logicamente equivalente a:",
    "Qual formula e logicamente equivalente a ~(P -> Q), a negacao de \"se P entao Q\"?",
    "A formula (P v ~P) e verdadeira para qualquer valor logico de P. Como se classifica essa formula?"
};

static const char dicas[TOTAL_PERGUNTAS][TAM_DICA] = {
    "Pense em como os dados de treino influenciam a decisao final.",
    "O sistema nao esta mentindo de proposito: ele esta inventando com confianca.",
    "O modelo decorou em vez de aprender a generalizar.",
    "Quem projeta, treina e implanta o sistema continua responsavel por ele.",
    "A conjuncao so e verdadeira quando os dois lados sao verdadeiros.",
    "A disjuncao so e falsa quando os dois lados sao falsos.",
    "O XOR e verdadeiro somente quando os dois lados sao diferentes entre si.",
    "Modus Ponens: de (P -> Q) e P, conclui-se Q.",
    "O bicondicional e verdadeiro quando os dois lados tem o mesmo valor.",
    "Contrapositiva: (P -> Q) equivale a (~Q -> ~P).",
    "De Morgan distribui a negacao e troca o conectivo.",
    "Negar uma implicacao significa afirmar a causa e negar a consequencia.",
    "Se e sempre verdadeira, a formula recebe um nome especifico."
};

static const char opcoes[TOTAL_PERGUNTAS][TOTAL_OPCOES][TAM_OPCAO] = {
    {"Um erro de digitacao no codigo",
     "Tendencia sistematica causada por dados de treino desbalanceados",
     "Uma falha de hardware do servidor",
     "Uma opiniao pessoal do programador"},

    {"Alucinacao",
     "Overfitting",
     "Vies de amostragem",
     "Criptografia"},

    {"Overfitting (sobreajuste)",
     "Responsabilidade algoritmica",
     "Bicondicional",
     "Modus Tollens"},

    {"Responsabilidade algoritmica",
     "Disjuncao logica",
     "Tautologia",
     "Contingencia"},

    {"Verdadeiro", "Falso", "Indeterminado", "Depende do contexto"},

    {"Falso", "Indeterminado", "Verdadeiro", "Nulo"},

    {"Verdadeiro", "Falso",
     "Verdadeiro apenas se ambos forem falsos",
     "Depende da ordem dos operandos"},

    {"Nao se pode concluir nada",
     "Q e verdadeiro: a rua fica molhada",
     "P e falso",
     "Q e falso"},

    {"Falso", "Verdadeiro", "Indeterminado", "Depende do conectivo anterior"},

    {"Nao chove: ~P e verdadeiro",
     "Chove: P e verdadeiro",
     "A pergunta nao tem solucao logica",
     "P e Q sao independentes"},

    {"~P ^ ~Q", "P v Q", "~P v ~Q", "P ^ Q"},

    {"~P -> ~Q", "P ^ ~Q", "~P v Q", "P v ~Q"},

    {"Contradicao", "Contingencia", "Tautologia", "Bicondicional"}
};

/* indice (0 a 3) da alternativa correta de cada pergunta */
static const int resposta_certa[TOTAL_PERGUNTAS] = {
    1, 0, 0, 0, 1, 2, 1, 1, 1, 0, 2, 1, 2
};

static const char explicacoes[TOTAL_PERGUNTAS][TAM_EXPLICACAO] = {
    "Vies algoritmico e uma tendencia sistematica e repetivel de erro, originada "
    "quando os dados ou as regras de treino nao representam de forma justa a "
    "populacao real. Nao e um bug pontual: e um padrao que se repete a cada "
    "nova decisao do sistema.",

    "Alucinacao ocorre quando um modelo gera uma informacao plausivel, porem "
    "falsa, apresentando-a como se fosse um fato verificado. O modelo nao sabe "
    "que esta errado: apenas prediz a sequencia de palavras mais provavel.",

    "Overfitting acontece quando o modelo se ajusta excessivamente ao conjunto "
    "de treino, inclusive ao ruido, e perde a capacidade de generalizar para "
    "dados nunca vistos. Alta performance no treino e baixa em producao e o "
    "sintoma classico.",

    "Responsabilidade algoritmica e o principio de que pessoas e organizacoes "
    "continuam responsaveis pelas decisoes de sistemas que constroem e operam. "
    "\"A IA decidiu sozinha\" nao exime a equipe humana de supervisao e correcao.",

    "Tabela-verdade da conjuncao (P ^ Q): so resulta em Verdadeiro quando P e Q "
    "sao ambos verdadeiros. Como um dos operandos aqui e Falso, o resultado e "
    "Falso.",

    "Tabela-verdade da disjuncao (P v Q): so resulta em Falso quando P e Q sao "
    "ambos falsos. Em qualquer outro caso, basta um dos lados ser Verdadeiro "
    "para o resultado ser Verdadeiro.",

    "O XOR (P + Q) resulta em Verdadeiro apenas quando P e Q tem valores "
    "diferentes. Quando os dois sao iguais, o resultado e Falso. Por isso "
    "V + V = Falso.",

    "Modus Ponens e a regra de inferencia: a partir de (P -> Q) e da premissa P "
    "verdadeira, conclui-se Q. Como \"chove\" e verdadeiro, \"a rua fica "
    "molhada\" tambem deve ser verdadeiro.",

    "O bicondicional P<->Q e verdadeiro exatamente quando P e Q tem o mesmo "
    "valor logico, ambos verdadeiros ou ambos falsos. Como Falso e Falso sao "
    "iguais, o resultado e Verdadeiro.",

    "Modus Tollens usa a equivalencia (P -> Q) equivale a (~Q -> ~P), chamada "
    "contrapositiva. Se a rua nao esta molhada, entao, por contrapositiva, "
    "conclui-se que nao esta chovendo.",

    "Leis de De Morgan: ~(P ^ Q) equivale a ~P v ~Q. Ao negar uma conjuncao, "
    "nega-se cada termo e troca-se o conectivo E pelo conectivo OU.",

    "A negacao da condicional segue: ~(P->Q) equivale a P ^ ~Q. Isso porque "
    "P->Q so e falsa quando P e verdadeiro e Q e falso; negar a implicacao "
    "equivale a afirmar exatamente essa combinacao.",

    "Uma formula e Tautologia quando e verdadeira para todas as combinacoes "
    "possiveis de valores, como P v ~P. E Contradicao quando e sempre falsa, "
    "e Contingencia quando depende dos valores das variaveis."
};

/* ============================================================================
 * FUNCOES DE DESENHO -- recebem apenas ponteiros nativos da SDL2/SDL_ttf
 * (SDL_Renderer*, TTF_Font*) e tipos basicos (int, SDL_Color por valor).
 * Nao existe nenhuma struct nem ponteiro definido pelo programador aqui:
 * essas funcoes apenas organizam codigo repetido (principio DRY).
 * ============================================================================ */

/* desenha um unico texto na posicao (x, y) usando a cor informada */
void desenha_texto(SDL_Renderer *tela, TTF_Font *fonte, const char *texto, int x, int y, SDL_Color cor) {
    SDL_Surface *superficie = TTF_RenderUTF8_Blended(fonte, texto, cor);
    SDL_Texture *textura = SDL_CreateTextureFromSurface(tela, superficie);
    SDL_Rect destino = {x, y, superficie->w, superficie->h};
    SDL_RenderCopy(tela, textura, NULL, &destino);
    SDL_FreeSurface(superficie);
    SDL_DestroyTexture(textura);
}

/* quebra um texto longo em varias linhas que cabem em largura_max pixels,
   medindo a largura real com TTF_SizeUTF8 (evita estourar a borda da tela) */
void desenha_texto_longo(SDL_Renderer *tela, TTF_Font *fonte, const char *texto_original,
                          int x, int y, int largura_max, int altura_linha, SDL_Color cor) {
    char buffer[1000];
    char linha_atual[300];
    char *palavra;
    int y_atual = y;

    snprintf(buffer, sizeof(buffer), "%s", texto_original);
    linha_atual[0] = '\0';

    palavra = strtok(buffer, " ");
    while (palavra != NULL) {
        char linha_teste[300];
        if (linha_atual[0] == '\0') {
            snprintf(linha_teste, sizeof(linha_teste), "%s", palavra);
        } else {
            snprintf(linha_teste, sizeof(linha_teste), "%s %s", linha_atual, palavra);
        }

        int largura_texto = 0;
        int altura_texto = 0;
        TTF_SizeUTF8(fonte, linha_teste, &largura_texto, &altura_texto);

        if (largura_texto > largura_max && linha_atual[0] != '\0') {
            desenha_texto(tela, fonte, linha_atual, x, y_atual, cor);
            y_atual += altura_linha;
            snprintf(linha_atual, sizeof(linha_atual), "%s", palavra);
        } else {
            snprintf(linha_atual, sizeof(linha_atual), "%s", linha_teste);
        }

        palavra = strtok(NULL, " ");
    }

    if (linha_atual[0] != '\0') {
        desenha_texto(tela, fonte, linha_atual, x, y_atual, cor);
    }
}

/* preenche um retangulo solido -- usado em paineis, botoes e barras */
void desenha_retangulo(SDL_Renderer *tela, int x, int y, int largura, int altura, SDL_Color cor) {
    SDL_Rect retangulo = {x, y, largura, altura};
    SDL_SetRenderDrawColor(tela, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(tela, &retangulo);
}

/* desenha apenas a borda de um retangulo -- usado para destacar paineis */
void desenha_borda(SDL_Renderer *tela, int x, int y, int largura, int altura, SDL_Color cor) {
    SDL_Rect retangulo = {x, y, largura, altura};
    SDL_SetRenderDrawColor(tela, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderDrawRect(tela, &retangulo);
}

/* desenha uma barra de progresso: fundo cinza + preenchimento proporcional
   a "proporcao" (0.0 a 1.0), usada tanto para o tempo quanto a confianca */
void desenha_barra(SDL_Renderer *tela, int x, int y, int largura, int altura, float proporcao, SDL_Color cor_fundo, SDL_Color cor_preenchida) {
    desenha_retangulo(tela, x, y, largura, altura, cor_fundo);
    int largura_preenchida = (int)(largura * proporcao);
    desenha_retangulo(tela, x, y, largura_preenchida, altura, cor_preenchida);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    /* --- inicializacao dos modulos de video, fonte e audio da SDL2 --- */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erro ao iniciar SDL2: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        printf("Erro ao iniciar SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *janela = SDL_CreateWindow(
        "Decifra.IA -- Missao de Etica e Logica em IA",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        LARGURA_JANELA, ALTURA_JANELA, SDL_WINDOW_SHOWN);

    SDL_Renderer *tela = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *fonte = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 17);
    if (fonte == NULL) {
        fonte = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 17);
    }
    TTF_Font *fonte_titulo = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 28);
    if (fonte_titulo == NULL) {
        fonte_titulo = TTF_OpenFont("C:\\Windows\\Fonts\\arialbd.ttf", 28);
    }

    if (fonte == NULL || fonte_titulo == NULL) {
        printf("Erro: nenhuma fonte TTF foi encontrada no sistema.\n");
        SDL_DestroyRenderer(tela);
        SDL_DestroyWindow(janela);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* --- audio de fundo (SDL_mixer), com fallback seguro se nao encontrar --- */
    bool musica_disponivel = false;
    bool musica_ligada = true;
    Mix_Music *musica_fundo = NULL;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0 && Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0) {
        musica_disponivel = true;
        musica_fundo = Mix_LoadMUS("audio/tema_calmo.mp3");
        if (musica_fundo != NULL) {
            Mix_PlayMusic(musica_fundo, -1);
        }
    }

    /* --- paleta de cores da interface (Slate/Navy, Cyan, Amber, Charcoal, Ice) --- */
    SDL_Color cor_fundo    = {10, 14, 23, 255};
    SDL_Color cor_destaque = {0, 229, 255, 255};
    SDL_Color cor_alerta   = {255, 191, 0, 255};
    SDL_Color cor_painel   = {22, 31, 48, 255};
    SDL_Color cor_texto    = {235, 240, 245, 255};

    /* --- variaveis primitivas que guardam todo o estado do jogo ---
       nenhuma struct customizada: apenas int, float e bool soltos. */
    int tela_atual = TELA_MENU;
    int opcao_menu = 0;
    int pergunta_atual = 0;
    int opcao_selecionada = 0;
    int pontuacao = 0;
    int acertos = 0;
    int erros = 0;
    int streak = 0;
    float confianca = CONFIANCA_INICIAL;
    float tempo_restante = TEMPO_POR_PERGUNTA;
    bool mostrando_feedback = false;
    bool ultima_resposta_certa = false;
    int pontos_ganhos_rodada = 0;
    float confianca_ganha_rodada = 0.0f;
    bool rodando = true;

    Uint32 ultimo_tick = SDL_GetTicks();
    SDL_Event evento;

    /* --- LOOP PRINCIPAL DA PARTIDA (do-while obrigatorio) ---
       Usamos do-while porque o conteudo do loop (capturar eventos,
       atualizar estado e desenhar a tela) precisa rodar pelo menos uma
       vez antes de qualquer verificacao de saida, e porque esse mesmo
       loop e responsavel tanto por jogar a missao quanto por voltar ao
       menu e reiniciar uma nova tentativa, sem nunca sair do programa. */
    do {
        Uint32 tick_atual = SDL_GetTicks();
        float dt = (tick_atual - ultimo_tick) / 1000.0f;
        if (dt > 0.1f) {
            dt = 0.1f;
        }
        ultimo_tick = tick_atual;

        /* --- captura de eventos: teclado, mouse e fechamento da janela --- */
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            }

            if (evento.type == SDL_KEYDOWN) {

                if (tela_atual == TELA_MENU) {
                    if (evento.key.keysym.sym == SDLK_UP) {
                        opcao_menu--;
                    }
                    if (evento.key.keysym.sym == SDLK_DOWN) {
                        opcao_menu++;
                    }
                    /* validacao de entrada com while: mantem a opcao do menu
                       sempre dentro do intervalo valido de 0 a 3 */
                    while (opcao_menu < 0) {
                        opcao_menu += 4;
                    }
                    while (opcao_menu > 3) {
                        opcao_menu -= 4;
                    }

                    if (evento.key.keysym.sym == SDLK_RETURN) {
                        if (opcao_menu == 0) {
                            pergunta_atual = 0;
                            opcao_selecionada = 0;
                            pontuacao = 0;
                            acertos = 0;
                            erros = 0;
                            streak = 0;
                            confianca = CONFIANCA_INICIAL;
                            tempo_restante = TEMPO_POR_PERGUNTA;
                            mostrando_feedback = false;
                            tela_atual = TELA_QUIZ;
                        } else if (opcao_menu == 1) {
                            tela_atual = TELA_INSTRUCOES;
                        } else if (opcao_menu == 2) {
                            musica_ligada = !musica_ligada;
                            if (musica_disponivel && musica_fundo != NULL) {
                                if (musica_ligada) {
                                    Mix_ResumeMusic();
                                } else {
                                    Mix_PauseMusic();
                                }
                            }
                        } else {
                            rodando = false;
                        }
                    }
                }

                else if (tela_atual == TELA_INSTRUCOES) {
                    if (evento.key.keysym.sym == SDLK_RETURN || evento.key.keysym.sym == SDLK_ESCAPE) {
                        tela_atual = TELA_MENU;
                    }
                }

                else if (tela_atual == TELA_QUIZ) {
                    if (mostrando_feedback) {
                        if (evento.key.keysym.sym == SDLK_RETURN) {
                            mostrando_feedback = false;
                            pergunta_atual++;
                            opcao_selecionada = 0;
                            tempo_restante = TEMPO_POR_PERGUNTA;

                            if (pergunta_atual >= TOTAL_PERGUNTAS) {
                                tela_atual = TELA_RESULTADO;
                            }
                        }
                    } else {
                        if (evento.key.keysym.sym == SDLK_UP) {
                            opcao_selecionada--;
                        }
                        if (evento.key.keysym.sym == SDLK_DOWN) {
                            opcao_selecionada++;
                        }
                        /* validacao de entrada com while: garante que a opcao
                           selecionada nunca saia do intervalo de 0 a 3 */
                        while (opcao_selecionada < 0) {
                            opcao_selecionada += TOTAL_OPCOES;
                        }
                        while (opcao_selecionada >= TOTAL_OPCOES) {
                            opcao_selecionada -= TOTAL_OPCOES;
                        }

                        if (evento.key.keysym.sym == SDLK_RETURN) {
                            bool acertou = (opcao_selecionada == resposta_certa[pergunta_atual]);
                            int bonus_tempo = (int)(tempo_restante * 10.0f);
                            int delta_pontos = acertou ? (100 + bonus_tempo) : (bonus_tempo - 20);

                            pontuacao += delta_pontos;
                            if (pontuacao < 0) {
                                pontuacao = 0;
                            }

                            if (acertou) {
                                acertos++;
                                streak++;
                                /* a cada 3 acertos seguidos, bonus extra de confianca */
                                confianca_ganha_rodada = (streak % 3 == 0) ? 10.0f : 5.0f;
                                confianca += confianca_ganha_rodada;
                            } else {
                                erros++;
                                streak = 0;
                                confianca_ganha_rodada = -8.0f;
                                confianca -= 8.0f;
                            }

                            /* validacao de estado com while: confianca sempre
                               entre 0 e 100 */
                            while (confianca > 100.0f) {
                                confianca = 100.0f;
                            }
                            while (confianca < 0.0f) {
                                confianca = 0.0f;
                            }

                            ultima_resposta_certa = acertou;
                            pontos_ganhos_rodada = delta_pontos;
                            mostrando_feedback = true;
                        }
                    }
                }

                else if (tela_atual == TELA_RESULTADO) {
                    if (evento.key.keysym.sym == SDLK_r) {
                        pergunta_atual = 0;
                        opcao_selecionada = 0;
                        pontuacao = 0;
                        acertos = 0;
                        erros = 0;
                        streak = 0;
                        confianca = CONFIANCA_INICIAL;
                        tempo_restante = TEMPO_POR_PERGUNTA;
                        mostrando_feedback = false;
                        tela_atual = TELA_QUIZ;
                    }
                    if (evento.key.keysym.sym == SDLK_RETURN) {
                        tela_atual = TELA_MENU;
                    }
                    if (evento.key.keysym.sym == SDLK_ESCAPE || evento.key.keysym.sym == SDLK_q) {
                        rodando = false;
                    }
                }
            }

            /* clique do mouse: so o menu principal tem botoes clicaveis */
            if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT && tela_atual == TELA_MENU) {
                int mx = evento.button.x;
                int my = evento.button.y;
                int i;
                for (i = 0; i < 4; i++) {
                    int y_botao = 230 + i * 70;
                    if (mx >= 60 && mx <= 940 && my >= y_botao && my <= y_botao + 50) {
                        opcao_menu = i;
                    }
                }
            }
        }

        /* --- atualizacao do cronometro da pergunta atual --- */
        if (tela_atual == TELA_QUIZ && !mostrando_feedback) {
            tempo_restante -= dt;
            if (tempo_restante <= 0.0f) {
                tempo_restante = 0.0f;
                erros++;
                streak = 0;
                confianca_ganha_rodada = -8.0f;
                confianca -= 8.0f;
                while (confianca < 0.0f) {
                    confianca = 0.0f;
                }
                ultima_resposta_certa = false;
                pontos_ganhos_rodada = 0;
                mostrando_feedback = true;
            }
        }

        /* --- desenho da tela a cada quadro --- */
        SDL_SetRenderDrawColor(tela, cor_fundo.r, cor_fundo.g, cor_fundo.b, 255);
        SDL_RenderClear(tela);

        if (tela_atual == TELA_MENU) {
            desenha_texto(tela, fonte_titulo, "DECIFRA.IA", 60, 50, cor_destaque);
            desenha_texto(tela, fonte, "Missao de alfabetizacao em etica e logica para Inteligencia Artificial", 60, 100, cor_texto);

            char rotulos[4][64];
            snprintf(rotulos[0], sizeof(rotulos[0]), "[1] Iniciar Missao");
            snprintf(rotulos[1], sizeof(rotulos[1]), "[2] Instrucoes & Etica em IA");
            snprintf(rotulos[2], sizeof(rotulos[2]), "[3] Musica: %s", musica_ligada ? "LIGADA" : "DESLIGADA");
            snprintf(rotulos[3], sizeof(rotulos[3]), "[4] Sair");

            int i;
            for (i = 0; i < 4; i++) {
                int y_botao = 230 + i * 70;
                SDL_Color cor_botao = (i == opcao_menu) ? cor_destaque : cor_painel;
                desenha_retangulo(tela, 60, y_botao, 880, 50, cor_botao);
                desenha_borda(tela, 60, y_botao, 880, 50, cor_destaque);
                SDL_Color cor_rotulo = (i == opcao_menu) ? cor_fundo : cor_texto;
                desenha_texto(tela, fonte, rotulos[i], 80, y_botao + 12, cor_rotulo);
            }

            desenha_texto(tela, fonte, "Setas ou mouse para escolher, ENTER para confirmar", 60, 600, cor_alerta);

        } else if (tela_atual == TELA_INSTRUCOES) {
            desenha_texto(tela, fonte_titulo, "INSTRUCOES & ETICA EM IA", 60, 50, cor_destaque);

            desenha_retangulo(tela, 60, 110, 880, 420, cor_painel);
            desenha_borda(tela, 60, 110, 880, 420, cor_destaque);

            desenha_texto_longo(tela, fonte,
                "Esta missao testa seu entendimento sobre conceitos criticos de etica em "
                "Inteligencia Artificial, como vies algoritmico, alucinacao, overfitting e "
                "responsabilidade algoritmica, alem de logica proposicional: conjuncao, "
                "disjuncao, XOR, Modus Ponens, Modus Tollens, bicondicional, Leis de De "
                "Morgan e tautologia.",
                80, 140, 820, 26, cor_texto);

            desenha_texto_longo(tela, fonte,
                "Cada pergunta tem um cronometro de 30 segundos: responder mais rapido "
                "rende mais pontos. Acertar aumenta sua Confianca Publica; errar ou "
                "estourar o tempo reduz essa confianca. A cada 3 acertos seguidos voce "
                "ganha um bonus extra de confianca.",
                80, 280, 820, 26, cor_texto);

            desenha_texto(tela, fonte, "Pressione ENTER ou ESC para voltar ao menu", 60, 580, cor_alerta);

        } else if (tela_atual == TELA_QUIZ) {
            desenha_texto(tela, fonte_titulo, "ATO 1 -- MISSAO DE ONBOARDING", 40, 25, cor_destaque);

            char texto_progresso[64];
            snprintf(texto_progresso, sizeof(texto_progresso), "Pergunta %d de %d", pergunta_atual + 1, TOTAL_PERGUNTAS);
            desenha_texto(tela, fonte, texto_progresso, 40, 65, cor_texto);

            desenha_texto(tela, fonte, "Tempo:", 650, 65, cor_alerta);
            desenha_barra(tela, 720, 68, 240, 18, tempo_restante / TEMPO_POR_PERGUNTA, cor_painel, cor_alerta);

            if (mostrando_feedback) {
                SDL_Color cor_painel_feedback = ultima_resposta_certa ? cor_destaque : cor_alerta;
                desenha_retangulo(tela, 40, 110, 920, 320, cor_painel);
                desenha_borda(tela, 40, 110, 920, 320, cor_painel_feedback);

                const char *titulo_feedback = ultima_resposta_certa ? "RESPOSTA CORRETA!" : "RESPOSTA INCORRETA -- REVISE O CONCEITO";
                desenha_texto(tela, fonte, titulo_feedback, 60, 130, cor_painel_feedback);

                if (ultima_resposta_certa) {
                    desenha_texto(tela, fonte, "Raciocinio logico aplicado corretamente.", 60, 170, cor_texto);
                } else {
                    desenha_texto_longo(tela, fonte, explicacoes[pergunta_atual], 60, 170, 860, 24, cor_texto);
                }

                char texto_delta[96];
                snprintf(texto_delta, sizeof(texto_delta), "Pontos: %+d   Confianca: %+.0f%%", pontos_ganhos_rodada, confianca_ganha_rodada);
                desenha_texto(tela, fonte, texto_delta, 60, 380, cor_alerta);
                desenha_texto(tela, fonte, "Pressione ENTER para continuar", 60, 410, cor_painel_feedback);

            } else {
                desenha_retangulo(tela, 40, 110, 920, 100, cor_painel);
                desenha_borda(tela, 40, 110, 920, 100, cor_destaque);
                desenha_texto_longo(tela, fonte, perguntas[pergunta_atual], 60, 125, 880, 24, cor_texto);

                char texto_dica[260];
                snprintf(texto_dica, sizeof(texto_dica), "Dica: %s", dicas[pergunta_atual]);
                desenha_texto(tela, fonte, texto_dica, 60, 178, cor_alerta);

                int i;
                for (i = 0; i < TOTAL_OPCOES; i++) {
                    int y_opcao = 220 + i * 58;
                    SDL_Color cor_opcao = (i == opcao_selecionada) ? cor_destaque : cor_painel;
                    desenha_retangulo(tela, 40, y_opcao, 920, 48, cor_opcao);
                    desenha_borda(tela, 40, y_opcao, 920, 48, cor_destaque);

                    SDL_Color cor_texto_opcao = (i == opcao_selecionada) ? cor_fundo : cor_texto;
                    char rotulo_opcao[200];
                    snprintf(rotulo_opcao, sizeof(rotulo_opcao), "%c) %s", 'A' + i, opcoes[pergunta_atual][i]);
                    desenha_texto(tela, fonte, rotulo_opcao, 60, y_opcao + 12, cor_texto_opcao);
                }
            }

            desenha_texto(tela, fonte, "Confianca Publica:", 40, 470, cor_texto);
            desenha_barra(tela, 230, 470, 300, 18, confianca / 100.0f, cor_painel, cor_destaque);

            char texto_status[120];
            snprintf(texto_status, sizeof(texto_status), "Pontos: %d   Acertos: %d   Erros: %d   Streak: %d",
                     pontuacao, acertos, erros, streak);
            desenha_texto(tela, fonte, texto_status, 40, 510, cor_texto);

            if (!mostrando_feedback) {
                desenha_texto(tela, fonte, "Setas para escolher, ENTER para confirmar", 40, 600, cor_alerta);
            }

        } else {
            desenha_texto(tela, fonte_titulo, "MISSAO CONCLUIDA", 60, 50, cor_destaque);

            desenha_retangulo(tela, 60, 110, 880, 380, cor_painel);
            desenha_borda(tela, 60, 110, 880, 380, cor_destaque);

            char texto_pontos[80];
            snprintf(texto_pontos, sizeof(texto_pontos), "Pontuacao final: %d", pontuacao);
            desenha_texto(tela, fonte, texto_pontos, 90, 140, cor_texto);

            char texto_acertos[80];
            snprintf(texto_acertos, sizeof(texto_acertos), "Acertos: %d de %d perguntas", acertos, TOTAL_PERGUNTAS);
            desenha_texto(tela, fonte, texto_acertos, 90, 175, cor_texto);

            char texto_confianca[80];
            snprintf(texto_confianca, sizeof(texto_confianca), "Confianca Publica final: %.1f%%", confianca);
            desenha_texto(tela, fonte, texto_confianca, 90, 210, cor_texto);
            desenha_barra(tela, 90, 240, 400, 20, confianca / 100.0f, cor_fundo, cor_destaque);

            const char *classificacao;
            if (confianca >= 80.0f) {
                classificacao = "Sistema de IA referencia em confianca e etica";
            } else if (confianca >= 50.0f && confianca < 80.0f) {
                classificacao = "Sistema de IA em recuperacao de confianca";
            } else {
                classificacao = "Sistema de IA em colapso de confianca publica";
            }
            desenha_texto(tela, fonte, classificacao, 90, 290, cor_alerta);

            desenha_texto(tela, fonte, "Pressione R para reiniciar a missao", 90, 400, cor_texto);
            desenha_texto(tela, fonte, "Pressione ENTER para voltar ao menu", 90, 430, cor_texto);
            desenha_texto(tela, fonte, "Pressione ESC para sair", 90, 460, cor_texto);
        }

        SDL_RenderPresent(tela);
        SDL_Delay(16);

    } while (rodando);

    /* --- libera toda a memoria usada pela SDL2 antes de encerrar --- */
    if (musica_fundo != NULL) {
        Mix_FreeMusic(musica_fundo);
    }
    if (musica_disponivel) {
        Mix_CloseAudio();
        Mix_Quit();
    }
    TTF_CloseFont(fonte);
    TTF_CloseFont(fonte_titulo);
    SDL_DestroyRenderer(tela);
    SDL_DestroyWindow(janela);
    TTF_Quit();
    SDL_Quit();

    return 0;
}