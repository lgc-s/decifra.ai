#include "world/level.h"
#include "core/config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_err(char *err, size_t n, const char *fmt, const char *a, int b) {
    if (err && n) snprintf(err, n, fmt, a, b);
}

static void trim_eol(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

/* Posição do jogador (canto superior esquerdo) com os pés na base do tile (tx,ty). */
static Vec2f stand_pos(int tx, int ty) {
    Vec2f p = { tx * TILE_SIZE + (TILE_SIZE - PH_W) * 0.5f, (ty + 1) * TILE_SIZE - PH_H };
    return p;
}

static Vec2f tile_center(int tx, int ty) {
    Vec2f p = { tx * TILE_SIZE + TILE_SIZE * 0.5f, ty * TILE_SIZE + TILE_SIZE * 0.5f };
    return p;
}

bool level_load(Level *lv, const char *path, char *err, size_t errsz) {
    FILE *f = fopen(path, "r");
    if (!f) { set_err(err, errsz, "nao foi possivel abrir '%s'%.0d", path, 0); return false; }

    memset(lv, 0, sizeof *lv);
    lv->act = 1;
    lv->active_checkpoint = -1;

    struct { bool set; InteractKind kind; int id; } imap[10];
    memset(imap, 0, sizeof imap);

    static char rows[LEVEL_MAX_H][LEVEL_MAX_W + 2];
    memset(rows, 0, sizeof rows);

    char line[LEVEL_MAX_W + 64];
    bool in_map = false;
    int nrows = 0;
    bool ok = true;

    while (ok && fgets(line, sizeof line, f)) {
        trim_eol(line);
        if (!in_map) {
            if (line[0] == '\0' || line[0] == ';') continue;
            if (strncmp(line, "@name ", 6) == 0) {
                snprintf(lv->name, sizeof lv->name, "%s", line + 6);
            } else if (strncmp(line, "@act ", 5) == 0) {
                lv->act = atoi(line + 5);
            } else if (strncmp(line, "@palette ", 9) == 0) {
                lv->palette = atoi(line + 9);
            } else if (strncmp(line, "@zone ", 6) == 0) {
                int x, p;
                if (sscanf(line + 6, "%d %d", &x, &p) == 2 && lv->nzones < MAX_ZONES) {
                    lv->zones[lv->nzones].tile_x = x;
                    lv->zones[lv->nzones].palette = p;
                    lv->nzones++;
                }
            } else if (strncmp(line, "@int ", 5) == 0) {
                char d, kind[16];
                int id;
                if (sscanf(line + 5, " %c %15s %d", &d, kind, &id) == 3 && d >= '1' && d <= '9') {
                    int k = d - '0';
                    imap[k].set = true;
                    imap[k].id = id;
                    if (strcmp(kind, "dialogue") == 0) imap[k].kind = INT_DIALOGUE;
                    else if (strcmp(kind, "call") == 0) imap[k].kind = INT_CALL;
                    else if (strcmp(kind, "quiz") == 0) imap[k].kind = INT_QUIZ;
                    else if (strcmp(kind, "bias") == 0) imap[k].kind = INT_BIAS;
                    else { set_err(err, errsz, "tipo de interacao invalido em '%s'%.0d", path, 0); ok = false; }
                }
            } else if (strcmp(line, "@map") == 0) {
                in_map = true;
            }
        } else {
            size_t len = strlen(line);
            if (nrows >= LEVEL_MAX_H) { set_err(err, errsz, "'%s': linhas demais (max %d)", path, LEVEL_MAX_H); ok = false; break; }
            if (len > LEVEL_MAX_W) { set_err(err, errsz, "'%s': linha %d muito larga", path, nrows + 1); ok = false; break; }
            memcpy(rows[nrows], line, len);
            if ((int)len > lv->width) lv->width = (int)len;
            nrows++;
        }
    }
    fclose(f);
    if (!ok) return false;
    if (nrows == 0 || lv->width == 0) { set_err(err, errsz, "'%s': mapa vazio%.0d", path, 0); return false; }
    lv->height = nrows;

    bool has_spawn = false;
    int collectible_base = (lv->act - 1) * COLLECTIBLES_PER_ACT;

    for (int y = 0; y < nrows; y++) {
        for (int x = 0; x < lv->width; x++) {
            char c = rows[y][x];
            if (c == '\0') c = '.';
            switch (c) {
            case '.': case ' ': break;
            case '#': lv->tiles[y][x] = TILE_SOLID; break;
            case '^': lv->tiles[y][x] = TILE_SPIKE; break;
            case '~': lv->tiles[y][x] = TILE_CRUMBLE; break;
            case 'g': lv->tiles[y][x] = TILE_GHOST; break;
            case 'P':
                lv->spawn = stand_pos(x, y);
                has_spawn = true;
                break;
            case 'C':
                if (lv->ncheckpoints < MAX_CHECKPOINTS) {
                    Checkpoint *cp = &lv->checkpoints[lv->ncheckpoints++];
                    cp->center = tile_center(x, y);
                    cp->respawn = stand_pos(x, y);
                }
                break;
            case '*':
                if (lv->ncollectibles < COLLECTIBLES_PER_ACT) {
                    Collectible *co = &lv->collectibles[lv->ncollectibles];
                    co->pos = tile_center(x, y);
                    co->id = collectible_base + lv->ncollectibles;
                    lv->ncollectibles++;
                } else {
                    set_err(err, errsz, "'%s': colecionaveis demais (max %d)", path, COLLECTIBLES_PER_ACT);
                    return false;
                }
                break;
            case 'o':
                if (lv->ncrystals < MAX_CRYSTALS) lv->crystals[lv->ncrystals++].pos = tile_center(x, y);
                break;
            case 'E':
                lv->exit_pos = tile_center(x, y);
                lv->has_exit = true;
                break;
            default:
                if (c >= '1' && c <= '9') {
                    int k = c - '0';
                    if (!imap[k].set) { set_err(err, errsz, "'%s': digito %d sem @int", path, k); return false; }
                    if (lv->ninteractables < MAX_INTERACTABLES) {
                        Interactable *it = &lv->interactables[lv->ninteractables++];
                        it->pos = tile_center(x, y);
                        it->kind = imap[k].kind;
                        it->id = imap[k].id;
                        it->glyph = k;
                    }
                } else {
                    set_err(err, errsz, "'%s': caractere invalido '%c'", path, c);
                    return false;
                }
            }
        }
    }

    if (!has_spawn) { set_err(err, errsz, "'%s': falta o ponto P%.0d", path, 0); return false; }
    lv->respawn_pos = lv->spawn;
    return true;
}

void level_update(Level *lv, float dt, Rectf pb) {
    for (int y = 0; y < lv->height; y++) {
        for (int x = 0; x < lv->width; x++) {
            if (lv->tiles[y][x] != TILE_CRUMBLE) continue;
            float *t = &lv->crumble[y][x];
            if (*t > 0.0f) {
                *t -= dt;
                if (*t <= 0.0f) *t = -CRUMBLE_GONE;
            } else if (*t < 0.0f) {
                *t += dt;
                if (*t >= 0.0f) {
                    /* Só volta se o jogador não estiver no lugar. */
                    float tx = x * TILE_SIZE, ty = y * TILE_SIZE;
                    bool overlap = pb.x < tx + TILE_SIZE && pb.x + pb.w > tx &&
                                   pb.y < ty + TILE_SIZE && pb.y + pb.h > ty;
                    if (overlap) *t = -0.05f;
                    else *t = 0.0f;
                }
            }
        }
    }
}

void level_touch_below(Level *lv, Rectf box) {
    int ty = (int)floorf((box.y + box.h + 0.5f) / TILE_SIZE);
    int x0 = (int)floorf(box.x / TILE_SIZE);
    int x1 = (int)floorf((box.x + box.w - 0.01f) / TILE_SIZE);
    if (ty < 0 || ty >= lv->height) return;
    for (int x = x0; x <= x1; x++) {
        if (x < 0 || x >= lv->width) continue;
        if (lv->tiles[ty][x] == TILE_CRUMBLE && lv->crumble[ty][x] == 0.0f)
            lv->crumble[ty][x] = CRUMBLE_SHAKE;
    }
}

int level_zone_palette(const Level *lv, float world_x) {
    int tx = (int)(world_x / TILE_SIZE);
    int pal = lv->palette;
    int best = -1;
    for (int i = 0; i < lv->nzones; i++) {
        if (lv->zones[i].tile_x <= tx && lv->zones[i].tile_x > best) {
            best = lv->zones[i].tile_x;
            pal = lv->zones[i].palette;
        }
    }
    return pal;
}
