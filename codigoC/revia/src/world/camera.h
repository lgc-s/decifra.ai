/* Câmera 2D com acompanhamento suave, antecipação e tremor. */
#ifndef CAMERA_H
#define CAMERA_H

#include "world/level.h"

typedef struct {
    Vec2f pos;      /* canto superior esquerdo da tela, em pixels do mundo */
    float shake;    /* intensidade atual do tremor */
    float lead;     /* antecipação horizontal suavizada */
} GameCamera;

void camera_snap(GameCamera *c, const Level *lv, Vec2f focus);
void camera_update(GameCamera *c, const Level *lv, Vec2f focus, float facing, float dt);
void camera_add_shake(GameCamera *c, float amount);

#endif
