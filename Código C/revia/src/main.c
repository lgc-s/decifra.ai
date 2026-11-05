/* REV.IA - DECIFRA.IA
 * Ponto de entrada: toda a lógica fica nos módulos (core/, player/, world/...). */
#include "core/game.h"

int main(void) {
    Game *game = game_create();
    if (!game) return 1;
    game_run(game);
    game_destroy(game);
    return 0;
}
