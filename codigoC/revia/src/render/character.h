/* Personagens jogáveis: cada um muda as cores do sprite (jaleco, cabelo, calça).
 * Ideia original de Davi Braz (GP01), adaptada para a versão modular. */
#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include "systems/progression.h"     /* NUM_CHARACTERS */

typedef struct {
    const char *name;
    Color coat, coat2;               /* jaleco e sombra do jaleco */
    Color pants, skin, hair;
} CharacterStyle;

/* Estilo do personagem id (valores fora da faixa viram o primeiro). */
const CharacterStyle *character_style(int id);

/* Desenha o personagem parado, de frente para a direita. (cx, y) = centro e topo do sprite;
 * s = escala (1 = tamanho no mundo); bob = balanço em pixels do mundo (só a cabeça). */
void character_draw_preview(int cx, int y, int id, int s, int bob);

#endif