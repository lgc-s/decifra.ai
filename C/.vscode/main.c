#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* --- PALETA DE CORES DA INTERFACE --- */
static const SDL_Color COLOR_BG       = {5, 12, 5, 255};       // Fundo quase preto CRT
static const SDL_Color COLOR_NEON     = {0, 255, 102, 255};    // Verde Neon Principal
static const SDL_Color COLOR_DARK_GRN = {0, 60, 25, 255};     // Verde escuro / Trilhas
static const SDL_Color COLOR_YELLOW   = {255, 204, 0, 255};    // Alertas em Amarelo
static const SDL_Color COLOR_TEXT_BLK = {5, 12, 5, 255};       // Texto escuro sobre botão ativo

/* --- ESTADOS DO JOGO --- */
typedef enum {
    STATE_MENU,
    STATE_QUIZ_ATO1
} GameState;

/* --- ESTRUTURA DO JOGO --- */
typedef struct {
    int selected_menu_option;
    int selected_quiz_option;
    int current_question_index;
    int score;
    GameState state;
    bool running;
} GameEngine;

/* --- HELPER DE RENDERIZAÇÃO DE RETÂNGULOS COM BORDA --- */
void draw_box(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color, int thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; i++) {
        SDL_Rect r = {rect.x + i, rect.y + i, rect.w - (i * 2), rect.h - (i * 2)};
        SDL_RenderDrawRect(renderer, &r);
    }
}

/* --- RENDERIZAÇÃO DE TEXTO COM SDL_TTF --- */
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!text || strlen(text) == 0) return;
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

/* --- RENDERIZAÇÃO DE BOTÕES ESTILIZADOS --- */
void render_button(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, int w, int h, bool selected) {
    SDL_Rect rect = {x, y, w, h};
    if (selected) {
        // Fundo preenchido em verde neon quando selecionado
        SDL_SetRenderDrawColor(renderer, COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, COLOR_NEON.a);
        SDL_RenderFillRect(renderer, &rect);
        render_text(renderer, font, text, x + (w / 2) - (strlen(text) * 4), y + (h / 2) - 10, COLOR_TEXT_BLK);
    } else {
        // Apenas borda vazada
        draw_box(renderer, rect, COLOR_NEON, 1);
        render_text(renderer, font, text, x + (w / 2) - (strlen(text) * 4), y + (h / 2) - 10, COLOR_NEON);
    }
}

/* --- BARRA DE PROGRESSO RETRO --- */
void render_progress_bar(SDL_Renderer *renderer, int x, int y, int w, int h, float progress) {
    SDL_Rect outer = {x, y, w, h};
    draw_box(renderer, outer, COLOR_DARK_GRN, 1);
    
    SDL_Rect inner = {x + 2, y + 2, (int)((w - 4) * progress), h - 4};
    SDL_SetRenderDrawColor(renderer, COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, COLOR_NEON.a);
    SDL_RenderFillRect(renderer, &inner);
}

/* ============================================================================
 * TELA 1: MENU INICIAL (REPRODUZ IMPRESSÃOmenu_inicial.png)
 * ============================================================================ */
void render_screen_menu(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    // Moldura Externa da Janela
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    // Header Principal (Caixa REV.IA)
    SDL_Rect header_box = {220, 60, 580, 100};
    draw_box(renderer, header_box, COLOR_NEON, 2);
    render_text(renderer, font_title, "REV.IA", 440, 75, COLOR_NEON);
    render_text(renderer, font_ui, "- - -  DECIFRA.IA  - - -", 390, 125, COLOR_NEON);

    // Subtítulo descritivo
    render_text(renderer, font_ui, "um jogo de terminal sobre alfabetizacao em Inteligencia Artificial", 230, 180, COLOR_NEON);
    render_text(renderer, font_ui, "Departamento de Alfabetizacao Algoritmica - projeto Rev.IA", 280, 205, COLOR_NEON);

    // Divisor
    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 50, 240, 970, 240);

    // Botões de Navegação
    const char *options[] = {"[1] Iniciar Jogo", "[2] Continuar", "[3] Configuracoes", "[4] Creditos", "[Q] Sair"};
    for (int i = 0; i < 5; i++) {
        render_button(renderer, font_ui, options[i], 360, 270 + (i * 55), 300, 45, (engine->selected_menu_option == i));
    }

    // Rodapé de Sistema
    render_text(renderer, font_ui, "v0.1 - Rev.IA / Decifra.IA - CESAR School, Projetos 2", 40, 585, COLOR_NEON);
    render_text(renderer, font_ui, "> _", 940, 585, COLOR_NEON);
}

/* ============================================================================
 * TELA 2: QUIZ / ATO 1 ONBOARDING (REPRODUZ IMPRESSÃO ato1_onboarding.png)
 * ============================================================================ */
void render_screen_quiz(SDL_Renderer *renderer, TTF_Font *font_title, TTF_Font *font_ui, GameEngine *engine) {
    // Moldura Externa
    SDL_Rect outer_frame = {20, 20, 984, 600};
    draw_box(renderer, outer_frame, COLOR_NEON, 2);

    // Título do Ato
    render_text(renderer, font_title, "ATO 1 -- ONBOARDING", 40, 35, COLOR_NEON);
    render_text(renderer, font_ui, "Manual de Treinamento -- Missao 2 de 5", 40, 70, COLOR_NEON);

    // Divisor Superior
    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRN.r, COLOR_DARK_GRN.g, COLOR_DARK_GRN.b, 255);
    SDL_RenderDrawLine(renderer, 40, 95, 980, 95);

    // Progresso do Ato
    render_text(renderer, font_ui, "Progresso do Ato 1:", 40, 110, COLOR_NEON);
    render_progress_bar(renderer, 220, 112, 450, 18, 0.35f);

    // Caixa de Pergunta
    SDL_Rect question_box = {40, 150, 940, 80};
    draw_box(renderer, question_box, COLOR_DARK_GRN, 1);
    render_text(renderer, font_ui, "Pergunta: O que significa \"vies algoritmico\"?", 60, 165, COLOR_NEON);
    render_text(renderer, font_ui, "(dica: pense em como os dados de treino influenciam a decisao final)", 60, 195, COLOR_NEON);

    // Opções de Resposta
    const char *answers[] = {
        "A) Um erro de digitacao no codigo da IA",
        "B) Tendencia sistematica causada por dados ou regras de treino desbalanceados",
        "C) Uma falha de hardware do servidor",
        "D) Uma opiniao pessoal do programador"
    };

    for (int i = 0; i < 4; i++) {
        SDL_Rect opt_rect = {40, 250 + (i * 60), 940, 48};
        bool is_selected = (engine->selected_quiz_option == i);
        
        if (is_selected) {
            SDL_SetRenderDrawColor(renderer, COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, 255);
            SDL_RenderFillRect(renderer, &opt_rect);
            render_text(renderer, font_ui, answers[i], 60, 264 + (i * 60), COLOR_TEXT_BLK);
        } else {
            draw_box(renderer, opt_rect, COLOR_DARK_GRN, 1);
            render_text(renderer, font_ui, answers[i], 60, 264 + (i * 60), COLOR_NEON);
        }
    }

    // Alerta de Rodapé
    render_text(renderer, font_ui, "Capacitacao obrigatoria -- Departamento de Alfabetizacao Algoritmica", 40, 520, COLOR_YELLOW);
    render_text(renderer, font_ui, "Acertos necessarios para avancar de fase: 10/10", 40, 550, COLOR_NEON);
    render_text(renderer, font_ui, "> selecione uma opcao e pressione ENTER_", 40, 575, COLOR_NEON);
}

/* ============================================================================
 * LOOP PRINCIPAL & EVENTOS ENGINE
 * ============================================================================ */
int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        printf("Falha ao inicializar SDL2/TTF\n");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "decifra.exe -- terminal",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 640, SDL_WINDOW_SHOWN
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Carregamento de fonte monospace padrão do sistema (ou substitua por caminho local ttf)
    TTF_Font *font_ui = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
    TTF_Font *font_title = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 28);

    if (!font_ui) font_ui = TTF_OpenFont("C:\\Windows\\Fonts\\consola.ttf", 14);
    if (!font_title) font_title = TTF_OpenFont("C:\\Windows\\Fonts\\consolab.ttf", 28);

    GameEngine engine = {
        .selected_menu_option = 0,
        .selected_quiz_option = 1, // Pré-selecionado 'B' idêntico ao screenshot
        .state = STATE_MENU,
        .running = true
    };

    SDL_Event e;
    while (engine.running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) engine.running = false;
            
            if (e.type == SDL_KEYDOWN) {
                if (engine.state == STATE_MENU) {
                    if (e.key.keysym.sym == SDLK_UP) engine.selected_menu_option = (engine.selected_menu_option - 1 + 5) % 5;
                    if (e.key.keysym.sym == SDLK_DOWN) engine.selected_menu_option = (engine.selected_menu_option + 1) % 5;
                    if (e.key.keysym.sym == SDLK_RETURN) {
                        if (engine.selected_menu_option == 0) engine.state = STATE_QUIZ_ATO1;
                        if (engine.selected_menu_option == 4) engine.running = false;
                    }
                } else if (engine.state == STATE_QUIZ_ATO1) {
                    if (e.key.keysym.sym == SDLK_UP) engine.selected_quiz_option = (engine.selected_quiz_option - 1 + 4) % 4;
                    if (e.key.keysym.sym == SDLK_DOWN) engine.selected_quiz_option = (engine.selected_quiz_option + 1) % 4;
                    if (e.key.keysym.sym == SDLK_ESCAPE) engine.state = STATE_MENU;
                }
            }
        }

        // Renderização da Tela
        SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
        SDL_RenderClear(renderer);

        if (engine.state == STATE_MENU) {
            render_screen_menu(renderer, font_title, font_ui, &engine);
        } else if (engine.state == STATE_QUIZ_ATO1) {
            render_screen_quiz(renderer, font_title, font_ui, &engine);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    if (font_ui) TTF_CloseFont(font_ui);
    if (font_title) TTF_CloseFont(font_title);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}