/* Utilitários de desenho 2D e de texto (sobre a Raylib). */
#ifndef GFX_H
#define GFX_H

#include "raylib.h"

#define UI_TEXT   (Color){ 236, 240, 255, 255 }
#define UI_DIM    (Color){ 150, 160, 190, 255 }
#define UI_ACCENT (Color){ 120, 230, 255, 255 }
#define UI_GOOD   (Color){ 120, 230, 150, 255 }
#define UI_BAD    (Color){ 255, 110, 120, 255 }
#define UI_WARN   (Color){ 255, 214, 110, 255 }
#define UI_PANEL  (Color){ 14, 16, 34, 235 }
#define UI_BORDER (Color){ 110, 130, 200, 255 }

unsigned gfx_hash(int a, int b);

/* Painel com borda de 1px e cantos recortados (estilo pixel art). */
void gfx_panel(int x, int y, int w, int h, Color fill, Color border);
void gfx_text_shadow(const char *t, int x, int y, int size, Color c);
void gfx_text_center(const char *t, int cx, int y, int size, Color c);
void gfx_text_right(const char *t, int rx, int y, int size, Color c);

/* Texto com quebra de linha em maxw pixels. max_bytes < 0 desenha tudo; senão só os
 * primeiros max_bytes (efeito de máquina de escrever). Devolve o y logo após o texto. */
int gfx_text_wrap(const char *text, int x, int y, int maxw, int size, Color c, int max_bytes);
/* Mesma medição, sem desenhar. */
int gfx_text_wrap_height(const char *text, int maxw, int size);

void gfx_bar(int x, int y, int w, int h, float frac, Color fill, Color back);

#endif
