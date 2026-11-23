#include "world/camera.h"
#include "core/config.h"

#include <math.h>

static Vec2f clamp_to_level(const Level *lv, Vec2f p) {
    float maxx = lv->width * TILE_SIZE - VIEW_W;
    float maxy = lv->height * TILE_SIZE - VIEW_H;
    p.x = maxx > 0 ? clampf(p.x, 0, maxx) : maxx * 0.5f;
    p.y = maxy > 0 ? clampf(p.y, 0, maxy) : maxy * 0.5f;
    return p;
}

void camera_snap(GameCamera *c, const Level *lv, Vec2f focus) {
    Vec2f t = { focus.x - VIEW_W * 0.5f, focus.y - VIEW_H * 0.55f };
    c->pos = clamp_to_level(lv, t);
    c->shake = 0;
    c->lead = 0;
}

void camera_update(GameCamera *c, const Level *lv, Vec2f focus, float facing, float dt) {
    c->lead = lerpf(c->lead, facing * 28.0f, 1.0f - expf(-3.0f * dt));
    Vec2f target = { focus.x + c->lead - VIEW_W * 0.5f, focus.y - VIEW_H * 0.55f };
    target = clamp_to_level(lv, target);
    float k = 1.0f - expf(-9.0f * dt);       /* suavização independente do framerate */
    c->pos.x = lerpf(c->pos.x, target.x, k);
    c->pos.y = lerpf(c->pos.y, target.y, k);
    c->shake = approachf(c->shake, 0.0f, 30.0f * dt);
}

void camera_add_shake(GameCamera *c, float amount) {
    if (amount > c->shake) c->shake = amount;
}
