/* Diálogos curtos de NPCs, terminais e placas.
 * ATENÇÃO: o texto é original deste projeto, escrito a partir do briefing.
 * Se o repositório original tiver falas/personagens, troque o conteúdo em dialogue.c. */
#ifndef DIALOGUE_H
#define DIALOGUE_H

#define DIALOGUE_COUNT 17
#define DIALOGUE_MAX_LINES 4

typedef struct { const char *speaker; const char *text; } DialogueLine;
typedef struct { int nlines; DialogueLine lines[DIALOGUE_MAX_LINES]; } Dialogue;

/* Devolve NULL para id inválido. */
const Dialogue *dialogue_get(int id);

#endif
