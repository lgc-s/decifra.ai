/* Entrada: traduz teclado e controle para as ações do jogo.
 *
 * Teclado:  setas/WASD mover | Z, Espaço ou K pular | X, Shift ou J dash
 *           C, Ctrl ou L agarrar | E ou Enter interagir/confirmar | Esc ou P pausar
 * Controle: direcional/analógico, A pular, X ou B dash, gatilhos (RB/RT) agarrar */
#ifndef INPUT_H
#define INPUT_H

#include "player/player.h"

typedef struct {
    PlayerInput pi;                /* ações de jogo (mantidas e "acabou de apertar") */
    bool up_p, down_p, left_p, right_p;   /* navegação em menus (borda) */
    bool confirm, back;            /* menus */
    bool interact;                 /* jogo: falar/usar */
    bool pause;
} GameInput;

void input_poll(GameInput *in);

#endif
