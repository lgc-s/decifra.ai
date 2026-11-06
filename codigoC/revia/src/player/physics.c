#include "player/physics.h"
#include "world/collision.h"
#include "core/config.h"

#include <math.h>

bool phys_move_x(const Level *lv, Vec2f *pos, float w, float h, float amount) {
    float remaining = amount;
    while (absf(remaining) > 1e-5f) {
        float step = clampf(remaining, -1.0f, 1.0f);
        float nx = pos->x + step;
        if (col_rect_solid(lv, nx, pos->y, w, h)) {
            if (step > 0) {
                int tx = (int)floorf((nx + w - 0.001f) / TILE_SIZE);
                pos->x = tx * TILE_SIZE - w;
            } else {
                int tx = (int)floorf(nx / TILE_SIZE);
                pos->x = (tx + 1) * TILE_SIZE;
            }
            return true;
        }
        pos->x = nx;
        remaining -= step;
    }
    return false;
}

bool phys_move_y(const Level *lv, Vec2f *pos, float w, float h, float amount) {
    float remaining = amount;
    while (absf(remaining) > 1e-5f) {
        float step = clampf(remaining, -1.0f, 1.0f);
        float ny = pos->y + step;
        if (col_rect_solid(lv, pos->x, ny, w, h)) {
            if (step > 0) {
                int ty = (int)floorf((ny + h - 0.001f) / TILE_SIZE);
                pos->y = ty * TILE_SIZE - h;
            } else {
                int ty = (int)floorf(ny / TILE_SIZE);
                pos->y = (ty + 1) * TILE_SIZE;
            }
            return true;
        }
        pos->y = ny;
        remaining -= step;
    }
    return false;
}
