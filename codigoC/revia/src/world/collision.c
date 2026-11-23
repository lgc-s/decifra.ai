#include "world/collision.h"
#include "core/config.h"

#include <math.h>

bool col_tile_solid(const Level *lv, int tx, int ty) {
    if (tx < 0 || tx >= lv->width) return true;
    if (ty < 0 || ty >= lv->height) return false;
    unsigned char t = lv->tiles[ty][tx];
    if (t == TILE_SOLID) return true;
    if (t == TILE_CRUMBLE) return lv->crumble[ty][tx] >= 0.0f;   /* ausente quando < 0 */
    return false;
}

bool col_rect_solid(const Level *lv, float x, float y, float w, float h) {
    int x0 = (int)floorf(x / TILE_SIZE);
    int x1 = (int)floorf((x + w - 0.001f) / TILE_SIZE);
    int y0 = (int)floorf(y / TILE_SIZE);
    int y1 = (int)floorf((y + h - 0.001f) / TILE_SIZE);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (col_tile_solid(lv, tx, ty)) return true;
    return false;
}

bool col_rect_hazard(const Level *lv, Rectf r) {
    int x0 = (int)floorf(r.x / TILE_SIZE);
    int x1 = (int)floorf((r.x + r.w - 0.001f) / TILE_SIZE);
    int y0 = (int)floorf(r.y / TILE_SIZE);
    int y1 = (int)floorf((r.y + r.h - 0.001f) / TILE_SIZE);
    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            if (tx < 0 || tx >= lv->width || ty < 0 || ty >= lv->height) continue;
            if (lv->tiles[ty][tx] != TILE_SPIKE) continue;
            /* hitbox do espinho: metade inferior do tile */
            float sy = ty * TILE_SIZE + TILE_SIZE * 0.4f;
            float sx = tx * TILE_SIZE + 1.0f;
            if (r.x < sx + TILE_SIZE - 2.0f && r.x + r.w > sx &&
                r.y < sy + TILE_SIZE * 0.6f && r.y + r.h > sy)
                return true;
        }
    }
    return false;
}
