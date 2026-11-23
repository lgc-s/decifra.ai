/* Telas do jogo: cada uma tem update (lógica de entrada) e draw (desenho). */
#ifndef SCREENS_H
#define SCREENS_H

#include "core/game.h"

/* hud.c */
void hud_draw(const Game *g);
void hud_draw_toast(const Game *g);

/* menu.c */
void menu_update(Game *g, float dt);       void menu_draw(const Game *g);
void select_update(Game *g, float dt);     void select_draw(const Game *g);
void settings_update(Game *g, float dt);   void settings_draw(const Game *g);
void pause_update(Game *g, float dt);      void pause_draw(const Game *g);
void credits_update(Game *g, float dt);    void credits_draw(const Game *g);

/* character_ui.c */
void character_update(Game *g, float dt);  void character_draw(const Game *g);

/* dialog_ui.c, call_ui.c, quiz_ui.c, bias_ui.c, codex_ui.c, ending.c */
void dialogue_update(Game *g, float dt);   void dialogue_draw(const Game *g);
void call_update(Game *g, float dt);       void call_draw(const Game *g);
void quiz_update(Game *g, float dt);       void quiz_draw(const Game *g);
void bias_update(Game *g, float dt);       void bias_draw(const Game *g);
void codex_update(Game *g, float dt);      void codex_draw(const Game *g);
void actdone_update(Game *g, float dt);    void actdone_draw(const Game *g);
void ending_update(Game *g, float dt);     void ending_draw(const Game *g);

/* utilitário comum: escurece o que estiver atrás da tela */
void ui_dim(int alpha);

#endif
