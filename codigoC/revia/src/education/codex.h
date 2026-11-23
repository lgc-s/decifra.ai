/* Enciclopédia (Manual de treinamento): conceitos desbloqueados por colecionáveis. */
#ifndef CODEX_H
#define CODEX_H

#include "systems/progression.h"

typedef struct { const char *title; const char *text; } CodexEntry;

/* id 0..NUM_COLLECTIBLES-1 (3 por ato, na ordem em que aparecem na fase). */
const CodexEntry *codex_get(int id);

#endif
