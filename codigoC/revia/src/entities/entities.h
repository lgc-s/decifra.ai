/* Entidades da fase: checkpoints, cristais, colecionáveis e interativos. */
#ifndef ENTITIES_H
#define ENTITIES_H

#include <stdint.h>
#include "player/player.h"
#include "world/level.h"

typedef struct {
    int  near_interact;     /* índice em lv->interactables, ou -1 */
    int  collected_id;      /* id global do colecionável pego neste passo, ou -1 */
    bool checkpoint;        /* novo checkpoint ativado */
    bool crystal;           /* cristal recarregou o dash */
    bool at_exit;           /* jogador está na saída */
} EntityEvents;

/* Prepara a fase para jogar: marca colecionáveis já pegos e zera checkpoints. */
void entities_prepare(Level *lv, uint32_t collected_mask);

void entities_update(Level *lv, Player *p, float dt, EntityEvents *ev);

#endif
