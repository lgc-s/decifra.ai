#include "entities/entities.h"
#include "core/config.h"

#include <string.h>

static float dist2(Vec2f a, Vec2f b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

void entities_prepare(Level *lv, uint32_t collected_mask) {
    lv->active_checkpoint = -1;
    lv->respawn_pos = lv->spawn;
    for (int i = 0; i < lv->ncollectibles; i++)
        lv->collectibles[i].taken = (collected_mask >> lv->collectibles[i].id) & 1u;
    for (int i = 0; i < lv->ncrystals; i++) lv->crystals[i].cooldown = 0;
}

void entities_update(Level *lv, Player *p, float dt, EntityEvents *ev) {
    memset(ev, 0, sizeof *ev);
    ev->near_interact = -1;
    ev->collected_id = -1;

    Vec2f c = { p->pos.x + PH_W * 0.5f, p->pos.y + PH_H * 0.5f };

    for (int i = 0; i < lv->ncheckpoints; i++) {
        if (i != lv->active_checkpoint && dist2(c, lv->checkpoints[i].center) < 12.0f * 12.0f) {
            lv->active_checkpoint = i;
            lv->respawn_pos = lv->checkpoints[i].respawn;
            ev->checkpoint = true;
        }
    }

    for (int i = 0; i < lv->ncrystals; i++) {
        Crystal *cr = &lv->crystals[i];
        if (cr->cooldown > 0) { cr->cooldown -= dt; continue; }
        if (!p->dash_ready && dist2(c, cr->pos) < 10.0f * 10.0f) {
            p->dash_ready = true;
            cr->cooldown = CRYSTAL_RESPAWN;
            ev->crystal = true;
        }
    }

    for (int i = 0; i < lv->ncollectibles; i++) {
        Collectible *co = &lv->collectibles[i];
        if (!co->taken && dist2(c, co->pos) < 10.0f * 10.0f) {
            co->taken = true;
            ev->collected_id = co->id;
        }
    }

    float best = 16.0f * 16.0f;
    for (int i = 0; i < lv->ninteractables; i++) {
        float d = dist2(c, lv->interactables[i].pos);
        if (d < best) { best = d; ev->near_interact = i; }
    }

    if (lv->has_exit && dist2(c, lv->exit_pos) < 12.0f * 12.0f) ev->at_exit = true;
}
