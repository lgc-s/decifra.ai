/* Fase: grade de tiles + entidades, carregada de um arquivo de texto.
 *
 * Formato do arquivo (linhas que começam com ';' são comentários):
 *   @name <texto>          nome exibido da fase
 *   @act <1..4>            ato a que pertence
 *   @palette <n>           paleta padrão
 *   @zone <tile_x> <n>     a partir da coluna tile_x, usa a paleta n
 *   @int <dígito> <dialogue|call|quiz|bias> <id>
 *   @map                   a partir daqui, uma linha de texto por linha de tiles
 *
 * Legenda do mapa:
 *   #  sólido        ^  espinhos        ~  plataforma que desmorona
 *   g  plataforma ilusória (não sustenta)
 *   P  início        C  checkpoint      *  colecionável (3 por fase)
 *   o  cristal de dash                  E  saída da fase
 *   1-9 objeto interativo (definido por @int)
 *   .  vazio
 */
#ifndef LEVEL_H
#define LEVEL_H

#include <stddef.h>
#include "core/types.h"

#define LEVEL_MAX_W 256
#define LEVEL_MAX_H 48
#define MAX_CHECKPOINTS 16
#define MAX_CRYSTALS 24
#define MAX_COLLECTIBLES 4
#define MAX_INTERACTABLES 12
#define MAX_ZONES 8
#define COLLECTIBLES_PER_ACT 3

typedef enum {
    TILE_EMPTY = 0, TILE_SOLID, TILE_SPIKE, TILE_CRUMBLE, TILE_GHOST
} TileType;

typedef enum { INT_DIALOGUE = 0, INT_CALL, INT_QUIZ, INT_BIAS } InteractKind;

typedef struct { Vec2f center; Vec2f respawn; } Checkpoint;
typedef struct { Vec2f pos; float cooldown; } Crystal;          /* cooldown > 0: inativo */
typedef struct { Vec2f pos; int id; bool taken; } Collectible;
typedef struct { Vec2f pos; InteractKind kind; int id; int glyph; } Interactable;
typedef struct { int tile_x; int palette; } Zone;

typedef struct {
    char name[64];
    int act;
    int palette;
    int width, height;                                /* em tiles */
    unsigned char tiles[LEVEL_MAX_H][LEVEL_MAX_W];
    float crumble[LEVEL_MAX_H][LEVEL_MAX_W];          /* 0 inteiro, >0 tremendo, <0 ausente */

    Vec2f spawn;                                      /* posição inicial do jogador */
    Vec2f respawn_pos;                                /* último checkpoint */
    int active_checkpoint;
    Checkpoint checkpoints[MAX_CHECKPOINTS]; int ncheckpoints;
    Crystal crystals[MAX_CRYSTALS];          int ncrystals;
    Collectible collectibles[MAX_COLLECTIBLES]; int ncollectibles;
    Interactable interactables[MAX_INTERACTABLES]; int ninteractables;
    Vec2f exit_pos; bool has_exit;
    Zone zones[MAX_ZONES]; int nzones;
} Level;

/* Carrega a fase. Em caso de erro devolve false e escreve a causa em err. */
bool level_load(Level *lv, const char *path, char *err, size_t errsz);

/* Avança timers de plataformas que desmoronam. player_box evita "emparedar" o jogador. */
void level_update(Level *lv, float dt, Rectf player_box);

/* Chamado quando o jogador está apoiado: faz plataformas frágeis começarem a tremer. */
void level_touch_below(Level *lv, Rectf box);

/* Paleta a usar na coluna do mundo world_x (pixels). */
int level_zone_palette(const Level *lv, float world_x);

#endif
